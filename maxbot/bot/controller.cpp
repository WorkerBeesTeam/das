#include <maxbot/types/AttachmentRequest.h>
#include <maxbot/types/CallbackAnswer.h>
#include <maxbot/types/Message.h>
#include <maxbot/types/NewMessageBody.h>
#include <maxbot/types/UpdateBotAddedToChat.h>
#include <maxbot/types/UpdateBotRemovedFromChat.h>
#include <maxbot/types/SubscriptionRequestBody.h>
#include <maxbot/BotException.h>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <algorithm>

#include <filesystem>

#include <boost/algorithm/string.hpp>

#include <QDebug>
#include <QDateTime>
#include <QDataStream>
#include <QIODevice>
#include <QCryptographicHash>

//#define WEBHOOK
#include <maxbot/BotException.h>
#ifndef WEBHOOK
#include <maxbot/net/BotLongPoll.h>
#endif

#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include <dbus/dbus_interface.h>
#include <Das/db/dig_status_type.h>
#include <Das/log/log_base_item.h>
#include <Das/commands.h>
#include <plus/das/database.h>

#include "../db/auth.h"
#include "../db/user.h"
#include "../db/chat.h"
#include "../db/subscriber.h"

#include "user_menu/connection_state.h"
#include "elements.h"
#include "controller.h"

namespace Das {
namespace Bot {

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

using namespace Helpz::DB;

Controller::Controller(DBus::Interface *dbus_iface, Config config) :
	Bot_Base(dbus_iface),
	stop_flag_(false), server_(nullptr),
	_conf(std::move(config))
{
	try
	{
		fill_templates();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Bot: Fail init templates " << e.what() << std::endl;
	}
}

Controller::~Controller()
{
	stop();
}

void Controller::start()
{
	if (!_th.joinable())
		_th = std::thread(&Controller::run, this);
}

void Controller::quit()
{
	stop();
}

void Controller::stop()
{
	stop_flag_ = true;
	if (server_)
		server_->stop();
	if (_th.joinable())
		_th.join();
}

void Controller::send_message(int64_t chatId, const string& text) const
{
	try
	{
		auto msg = std::make_shared<MaxBot::NewMessageBody>();
		msg->text = text;
		msg->format = "markdown";
		bot_->getApi().sendMessage(chatId, 0, std::move(msg));
	}
	catch(const MaxBot::BotException& e)
	{
		std::cerr << "Fail send message to chatId=" << chatId << ' ' << e.what() << " msg_text: " << text << std::endl;
	}
	catch(...) { std::cerr << "Send message unknown exception" << std::endl; }
}

void Controller::init()
{
	_botClient = std::make_shared<MaxBot::CurlHttpClient>(_conf._token);
	bot_ = std::make_shared<MaxBot::Bot>(*_botClient);

	bot_user_ = bot_->getApi().getMe();
	if (bot_user_)
		qDebug() << "Bot initialized. Id:" << bot_user_->user_id << "Name:" << bot_user_->username.c_str();
	else
		qCritical() << "Can't initialize bot";

	bot_->getEvents().onCommand("start", [this](MaxBot::Message::Ptr msg)
	{
		if (!msg->sender || !msg->recipient)
			return;
		uint32_t user_id = get_authorized_user_id(msg->sender->user_id, nullptr);
		if (user_id == 0)
			send_authorization_message(msg);
		else
			send_message(msg->recipient->chat_id, "Вы уже авторизованы");
	});
	bot_->getEvents().onCommand("find", [this](MaxBot::Message::Ptr msg)
	{
		if (!msg->sender || !msg->recipient)
			return;
		uint32_t user_id = get_authorized_user_id(msg->sender->user_id, msg);
		if (user_id != 0)
			find(user_id, std::move(msg));
	});
	bot_->getEvents().onCommand("list", [this](MaxBot::Message::Ptr msg)
	{
		if (!msg->sender || !msg->recipient)
			return;
		uint32_t user_id = get_authorized_user_id(msg->sender->user_id, msg);
		if (user_id != 0)
			list(user_id, std::move(msg));
	});
	bot_->getEvents().onCommand("report", [this](MaxBot::Message::Ptr msg)
	{
		if (!msg->sender || !msg->recipient)
			return;
		uint32_t user_id = get_authorized_user_id(msg->sender->user_id, msg);
		if (user_id != 0)
			report(std::move(msg));
	});

	auto inform_func = [this](MaxBot::Message::Ptr msg)
	{
		if (!msg->sender || !msg->recipient)
			return;
		uint32_t user_id = get_authorized_user_id(msg->sender->user_id, msg);
		if (user_id != 0)
			inform_onoff(user_id, std::move(msg));
	};
	bot_->getEvents().onCommand("inform", inform_func); // deprecated
	bot_->getEvents().onCommand("inform_onoff", inform_func);

	bot_->getEvents().onCommand("help", [this](MaxBot::Message::Ptr msg)
	{
		if (!msg->sender || !msg->recipient)
			return;
		uint32_t user_id = get_authorized_user_id(msg->sender->user_id, msg);
		if (user_id != 0)
			help(std::move(msg));
	});

	bot_->getEvents().onMessageCallback([this](MaxBot::UpdateMessageCallback::Ptr q)
	{
		if (!q || !q->callback || !q->callback->user || !q->message || !q->message->recipient)
		{
			qDebug() << "onMessageCallback invalid arg"
				<< !!q << (q && q->callback) << (q && q->callback && q->callback->user) << (q && q->message) << (q && q->message && q->message->recipient);
			return;
		}

		uint32_t user_id = get_authorized_user_id(q->callback->user->user_id, nullptr);
		qDebug() << "Data:" << q->callback->payload.c_str()
				 << "Text:" << q->message->body->text.c_str()
				 << "User id:" << user_id;
		if (user_id == 0)
			return;

		try
		{
			process_directory(user_id, q);
		}
		catch(const std::exception& e)
		{
			qCritical() << "onCallBack process_directory exception:" << e.what();
			try
			{
				auto answer = std::make_shared<MaxBot::CallbackAnswer>();
				answer->notification = "Во время обработки команды произошла ошибка";
				bot_->getApi().answerCallbackQuery(q->callback->callback_id, std::move(answer));
			}
			catch(const std::exception& e)
			{
				qCritical() << "onCallBack exception:" << e.what();
			}
		}
	});

	bot_->getEvents().onBotStarted([](MaxBot::UpdateBotStarted::Ptr q) {
		if (q->user)
			qDebug() << "Bot start required from user:" << q->chat_id << QString::fromStdString(q->payload) << QString::fromStdString(q->user->username);
	});
	bot_->getEvents().onBotStopped([](MaxBot::UpdateBotStopped::Ptr q) {
		if (q->user)
			qDebug() << "Bot stopped for user:" << q->chat_id << QString::fromStdString(q->user->username);
	});
	bot_->getEvents().onBotAddedToChat([](MaxBot::UpdateBotAddedToChat::Ptr q) {
		if (q->user)
			qDebug() << "Bot added to chat:" << q->chat_id << QString::fromStdString(q->user->username);
	});
	bot_->getEvents().onBotRemovedFromChat([](MaxBot::UpdateBotRemovedFromChat::Ptr q) {
		if (!q->user)
			return;
		qDebug() << "Bot removed from chat:" << q->chat_id << QString::fromStdString(q->user->username);

		Base& db = Base::get_thread_local_instance();

		QString field_name = db_table<MaxBot_Subscriber>().field_names().at(MaxBot_Subscriber::COL_chat_id);
		db.del(db_table_name<MaxBot_Subscriber>(), field_name + '=' + QString::number(q->chat_id));

		field_name = db_table<DB::MaxBot_Chat>().field_names().at(DB::MaxBot_Chat::COL_id);
		db.del(db_table_name<DB::MaxBot_Chat>(), field_name + '=' + QString::number(q->chat_id));
	});

	bot_->getEvents().onAnyMessage([this](MaxBot::Message::Ptr message) { anyMessage(message); });
}

void Controller::run()
{
	qDebug() << "started thread";
	try
	{
		if (_conf._token.empty())
		{
			qWarning() << "MaxBot token is empty";
			return;
		}
		init();

#ifdef WEBHOOK
		server_ = new MaxBot::BotWebhookTcpServer(_conf._port, "/", bot_->getEventHandler());
		auto req = std::make_shared<MaxBot::SubscriptionRequestBody>();
		req->url = _conf._webhook_url;
		req->secret = _conf._webhookSecret;
		bot_->getApi().setWebhook(std::move(req));
		qDebug() << "MaxBot configured webhook:" << QString::fromStdString(_conf._webhook_url);
		server_->start();
#else
		qDebug() << "Use debug BotLongPoll";
		MaxBot::BotLongPoll longPoll(*bot_);
		while (!stop_flag_)
		{
			longPoll.start();
		}
#endif
	}
	catch (const MaxBot::BotException &e)
	{
		qCritical() << "MaxBot exception:" << e.what()
			<< "urlPath:" << QString::fromStdString(e.urlPath)
			<< "customMethod:" << QString::fromStdString(e.method);
	}
	catch (const std::exception &e)
	{
		qCritical() << "MaxBot exception:" << e.what();
	}
	catch (...)
	{
		qCritical() << "MaxBot unknown exception";
	}

#ifdef WEBHOOK
	if (server_)
	{
		bot_->getApi().deleteWebhook(_conf._webhook_url);

		delete server_;
		server_ = nullptr;
	}
#endif

	if (bot_user_)
		bot_user_.reset();

	bot_.reset();
	_botClient.reset();
}

void Controller::anyMessage(MaxBot::Message::Ptr msg)
{
	if (!msg || !msg->body || msg->body->text.empty())
	{
		qWarning() << "Invalid message:" << !!msg << (msg && msg->body) << (msg && msg->body && !msg->body->text.empty());
		return;
	}

	qDebug() << "Chat:" << msg->recipient->chat_id << "Sender:" << msg->sender->user_id
		<< QString::fromStdString(msg->sender->username) << "Text:" << QString::fromStdString(msg->body->text);
	if (msg->body->text == "/help")
		return;

	auto it = waited_map_.find(msg->recipient->chat_id);
	if (it != waited_map_.end() && it->second.external_user_id_ == msg->sender->user_id)
	{
		uint32_t user_id = get_authorized_user_id(msg->sender->user_id, msg);
		if (user_id && !it->second.data_.empty())
		{
			const vector<string> cmd = StringTools::split(it->second.data_, '.');
			if (cmd.front() == "scheme")
			{
				Elements elements(*this, user_id, it->second.scheme_, cmd, it->second.data_, msg->body->text);
				elements.generate_answer();
				if (elements._msg && !elements._msg->text.empty())
					bot_->getApi().sendMessage(msg->recipient->chat_id, 0, std::move(elements._msg));
			}
			else if (cmd.front() == "find")
				find(user_id, msg, msg->body->text);
			else if (cmd.front() == "help")
				return;
		}

		waited_map_.erase(it);
	}
#ifdef QT_DEBUG
	send_message(msg->recipient->chat_id, "*You send*: " + prepare_str(msg->body->text));
#endif
}

void Controller::processDirectoryPage(uint32_t user_id, const std::vector<std::string>& cmd, MaxBot::UpdateMessageCallback::Ptr cb)
{
	if (cmd.size() < 3)
		throw std::runtime_error("Unknown pagination argument count: " + to_string(cmd.size()));

	const string direction = cmd.at(1);
	uint32_t current_page = static_cast<uint32_t> (atoi(cmd.at(2).c_str()));
	if (direction == "next")
		++current_page;
	else if (direction == "prev")
		--current_page;
	else
		throw std::runtime_error("Unknown pagination direction: " + direction);

	const string search_text = cmd.size() > 3 ? cmd.at(3) : std::string();
	send_schemes_list(user_id, nullptr, current_page, cb->callback->callback_id, search_text);
}

void Controller::processDirectoryScheme(uint32_t user_id, const std::vector<std::string>& cmd, MaxBot::UpdateMessageCallback::Ptr cb)
{
	if (cmd.size() < 2)
		throw std::runtime_error("Unknown scheme argument count: " + to_string(cmd.size()));

	const Scheme_Item scheme = get_scheme(user_id, cmd.at(1));
	if (!scheme.id())
		throw std::runtime_error("Unknown scheme id");

	if (cmd.size() < 3)
	{
		sendSchemeMenu(std::move(cb), scheme, user_id);
		return;
	}

	const string &action = cmd.at(2);
	if (action == "user_menu")
	{
		if (cmd.size() < 4)
			throw std::runtime_error("Unknown user_menu argument count: " + to_string(cmd.size()));

		uint32_t index = stoul(cmd.at(3));
		if (index < user_menu_set_.size())
		{
			auto it = user_menu_set_.begin();
			std::advance(it, index);

			const std::string text = it->get_text(user_id, scheme);
			send_message(cb->message->recipient->chat_id, text);
		}
	}
	else if (action == "status")
		status(scheme, cb->message);
	else if (action == "elem")
	{
		Elements elements(*this, user_id, scheme, cmd, cb->callback->payload);
		elements.generate_answer();
		if (!elements._msg)
			return;

		if (elements.skip_edit_)
		{
			if (!elements._msg->text.empty())
			{
				Waited_Item& waited_item = waited_map_[cb->message->recipient->chat_id];
				waited_item.external_user_id_ = cb->callback->user->user_id;
				waited_item.expired_time_ = std::chrono::system_clock::now() + 30s;
				waited_item.data_ = cb->callback->payload;
				waited_item.scheme_ = scheme;

				send_message(cb->message->recipient->chat_id, elements._msg->text);
			}
		}
		else
		{
			auto answer = std::make_shared<MaxBot::CallbackAnswer>();
			answer->message = std::move(elements._msg);
			bot_->getApi().answerCallbackQuery(cb->callback->callback_id, std::move(answer));
		}
	}

	else if (action == "menu_sub_1")
		menu_sub_list(scheme, cb, "sub_1");
	else if (action == "menu_sub_2")
		menu_sub_list(scheme, cb, "sub_2");
	else if (action == "sub" && cmd.size() >= 5)
	{
//			  uint32_t sub_id = std::stoi(cmd.at(3));
		const string &sub_id = cmd.at(3);
		const string &sub_action = cmd.at(4);

		if (sub_action == "sub_1")
		{
			if (cmd.size() > 5)
			{
				const string &sub_1_id = cmd.at(5);
				sub_1(scheme, cb->message, std::stoi(sub_id), std::stoi(sub_1_id));
			}
			else
				menu_sub_1(scheme, cb, sub_id);
		}
		else if (sub_action == "sub_2")
		{
			if (cmd.size() > 5)
			{
				const string &sub_2_type = cmd.at(5);
				sub_2(scheme, cb->message, sub_2_type);
			}
			else
				menu_sub_2(scheme, cb, sub_id);
		}
		else
			throw std::runtime_error("Invalid sub action: " + sub_action);
	}
	else if (action == "restart")
		restart(user_id, scheme, cb->message);
	else
		throw std::runtime_error("Unknown action for scheme: " + action);
}

void Controller::processDirectorySubscriber(uint32_t user_id, const std::vector<std::string>& cmd, MaxBot::UpdateMessageCallback::Ptr cb)
{
	if (cmd.size() < 2)
		throw std::runtime_error("Unknown subscriber argument count: " + to_string(cmd.size()));

	const char* group_id_str = cmd.at(1).c_str();
	uint32_t group_id = atoi(group_id_str);
	qDebug() << "subscriber scheme group:" << group_id_str;

	const QString sql =
			"SELECT sg.id, mbs.id FROM das_scheme_group sg "
			"LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.id "
			"LEFT JOIN das_maxbot_subscriber mbs ON mbs.group_id = sg.id AND mbs.chat_id = %1 "
			"WHERE sgu.user_id = %2 AND sg.id = %3 ORDER BY sg.id";

	Base& db = Base::get_thread_local_instance();
	QSqlQuery q = db.exec(sql.arg(cb->message->recipient->chat_id).arg(user_id).arg(group_id));
	if (!q.next())
		throw std::runtime_error("Unknown scheme group for user");

	if (q.value(1).isNull())
	{
		db.insert(db_table<DB::MaxBot_Chat>(), DB::MaxBot_Chat::to_variantlist(DB::MaxBot_Chat{cb->message->recipient->chat_id, cb->callback->user->user_id}));

		Table table = db_table<MaxBot_Subscriber>();
		table.field_names().removeAt(0);
		if (!db.insert(table, {(qint64)cb->message->recipient->chat_id, group_id}))
			throw std::runtime_error("Failed add subscriber");
	}
	else
	{
		if (!db.del(MaxBot_Subscriber::table_name(), "id=" + q.value(1).toString()).isActive())
			throw std::runtime_error("Failed remove subscriber");
	}

	inform_onoff(user_id, cb->message, cb->callback->callback_id);
}

void Controller::process_directory(uint32_t user_id, MaxBot::UpdateMessageCallback::Ptr cb)
{
	//process_directory(user_id, q->message, q->callback->payload, q->callback->user->user_id);
	//			auto answer = std::make_shared<MaxBot::CallbackAnswer>();
	//			answer->notification = "Во время обработки команды произошла ошибка";
	//			bot_->getApi().answerCallbackQuery(q->callback->callback_id, std::move(answer));
	const vector<string> cmd = StringTools::split(cb->callback->payload, '.');
	const string directory = cmd.at(0);

	if (directory == "page")
		processDirectoryPage(user_id, cmd, std::move(cb));
	else if (directory == "list")
		send_schemes_list(user_id, nullptr, 0, cb->callback->callback_id);
	else if (directory == "scheme")
		processDirectoryScheme(user_id, cmd, std::move(cb));
	else if (directory == "subscriber")
		processDirectorySubscriber(user_id, cmd, std::move(cb));
	else
		throw std::runtime_error("Unhandled directory for callback: " + directory);
}

// Chat methods
void Controller::find(uint32_t user_id, MaxBot::Message::Ptr msg)
{
	const string find_str = "/find ";
	if (msg->body->text.size() < find_str.size() + 1)
	{
		Waited_Item& waited_item = waited_map_[msg->recipient->chat_id];
		waited_item.external_user_id_ = msg->sender->user_id;
		waited_item.expired_time_ = std::chrono::system_clock::now() + 30s;
		waited_item.data_ = "find";

		send_message(msg->recipient->chat_id, "Отправьте текст для поиска аппарата:");
	}
	else
	{
		string search_text = msg->body->text.substr(find_str.size());
		find(user_id, msg, std::move(search_text));
	}
}

void Controller::find(uint32_t user_id, const MaxBot::Message::Ptr& msg, string text)
{
	text.erase(std::remove(text.begin(), text.end(), '.'), text.end());
	text.erase(std::remove(text.begin(), text.end(), '\''), text.end());
	text.erase(std::remove(text.begin(), text.end(), '"'), text.end());
	text.erase(std::remove(text.begin(), text.end(), ';'), text.end());
	boost::trim(text);

	qDebug() << "Searching for:" << text.c_str();
	send_schemes_list(user_id, msg, 0, {}, text);
}

void Controller::list(uint32_t user_id, MaxBot::Message::Ptr msg) const
{
	send_schemes_list(user_id, msg);
}

void Controller::report(MaxBot::Message::Ptr msg) const
{
	return;
	try {
		// TODO: нет создания файла
		// TODO: нет заливки файла и получения уникального токена
		auto fileReq = std::make_shared<MaxBot::FileAttachmentRequest>();
		fileReq->payload = std::make_shared<MaxBot::UploadedInfo>();
		fileReq->payload->token = ""; // TODO: установить токен полученный после заливки
		auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
		attachment->_data = std::move(fileReq);
		auto newMsg = std::make_shared<MaxBot::NewMessageBody>();
		newMsg->text = "Отчёт";
		newMsg->attachments.emplace_back(std::move(attachment));
		bot_->getApi().sendMessage(msg->recipient->chat_id, 0, std::move(newMsg));
	} catch (const std::exception& e) {
		send_message(msg->recipient->chat_id, std::string("Ошибка при чтении файла: ") + e.what());
	}
}

void Controller::inform_onoff(uint32_t user_id, MaxBot::Message::Ptr msg, const std::string& callbackId)
{
	const QString sql =
			"SELECT sg.id, sg.name, mbs.id FROM das_scheme_group sg "
			"LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.id "
			"LEFt join das_maxbot_subscriber mbs ON mbs.group_id = sg.id AND mbs.chat_id = %1 "
			"WHERE sgu.user_id = %2 ORDER BY sg.id";

	Base& db = Base::get_thread_local_instance();
	QSqlQuery q = db.exec(sql.arg(msg->recipient->chat_id).arg(user_id));

	auto keyboard = std::make_shared<MaxBot::InlineKeyboardAttachmentRequest>();
	while (q.next())
	{
		std::string text;
		if (!q.value(2).isNull())
			text = "✅ ";
		text += q.value(1).toString().toStdString();
		auto data = string("subscriber.") + q.value(0).toString().toStdString();
		keyboard->payload.buttons.push_back(Bot_Base::makeInlineButtonRow(data, text));
	}

	auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
	attachment->_data = std::move(keyboard);
	auto newMsg = std::make_shared<MaxBot::NewMessageBody>();
	newMsg->text = "Группы аппаратов:";
	newMsg->attachments.emplace_back(std::move(attachment));

	if (!callbackId.empty())
	{
		auto answer = std::make_shared<MaxBot::CallbackAnswer>();
		answer->message = std::move(newMsg);
		bot_->getApi().answerCallbackQuery(callbackId, std::move(answer));
	}
	else
		bot_->getApi().sendMessage(msg->recipient->chat_id, 0, std::move(newMsg));
}

string mimetype_from_extension(const string& ext)
{
	if (ext == ".ods")
		return "application/vnd.oasis.opendocument.spreadsheet";
	else if (ext == ".odt")
		return "application/vnd.oasis.opendocument.text";
	else if (ext == ".xlsx")
		return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	else if (ext == ".docx")
		return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	else if (ext == ".pdf")
		return "application/pdf";
	else if (ext == ".txt")
		return "text/plain";
	return {};
}

void Controller::help(MaxBot::Message::Ptr msg)
{
	string text = "Найти инструкцию по работе с ботом вы можете на сайте, выбрав любой аппарат и перейдя в раздел \"Справка\".";

	const fs::path help_file_path(_conf._help_file_path);
	qDebug() << QString::fromStdString(help_file_path)
			 << "Empty" << _conf._help_file_path.empty()
			 << "Exist" << fs::exists(help_file_path)
			 << "Res" << (!_conf._help_file_path.empty() && fs::exists(help_file_path));

	if (!_conf._help_file_path.empty() && fs::exists(help_file_path))
	{
		const string ext = help_file_path.extension();
		const string mime_type = mimetype_from_extension(ext);

		if (!mime_type.empty())
		{
			auto it = waited_map_.find(msg->recipient->chat_id);
			if (it != waited_map_.end()
				&& it->second.external_user_id_ == msg->sender->user_id
				&& it->second.data_ == "help")
			{
				MaxBot::InputFile::Ptr file = MaxBot::InputFile::fromFile(help_file_path, mime_type);
				bot_->getApi().sendDocument(msg->recipient->chat_id, file);

				waited_map_.erase(it);
				return;
			}
			else
			{
				text += " Или отправьте команду /help ещё раз чтобы получить файл справки.";

				Waited_Item& waited_item = waited_map_[msg->recipient->chat_id];
				waited_item.external_user_id_ = msg->sender->user_id;
				waited_item.expired_time_ = std::chrono::system_clock::now() + 30s;
				waited_item.data_ = "help";
			}
		}
	}

	send_message(msg->recipient->chat_id, text);
}

void Controller::help_send_file(int64_t chat_id) const
{
	const fs::path help_file_path(_conf._help_file_path);
	if (!_conf._help_file_path.empty() && fs::exists(help_file_path))
	{
		const string ext = help_file_path.extension();
		const string mime_type = mimetype_from_extension(ext);

		if (!mime_type.empty())
		{
		}
	}
}

void Controller::status(const Scheme_Item& scheme, MaxBot::Message::Ptr msg)
{
	Scheme_Status scheme_status;
	QMetaObject::invokeMethod(dbus_iface_, "get_scheme_status", Qt::BlockingQueuedConnection,
		Q_RETURN_ARG(Scheme_Status, scheme_status),
		Q_ARG(uint32_t, scheme.id()));

	Base& db = Base::get_thread_local_instance();
	QString sql, status_text, status_sql;

	std::string text = '*' + scheme.title_.toStdString() + "*\n";

	text += User_Menu::Connection_State::to_string(scheme_status.connection_state_) + '\n';

	if (!scheme_status.status_set_.empty())
	{
		sql = "SELECT dig.id, s.name, dig.title, gt.title "
			  "FROM das_device_item_group dig "
			  "LEFT JOIN das_section s ON s.id = dig.section_id "
			  "LEFT JOIN das_dig_type gt ON gt.id = dig.type_id "
			  "WHERE dig.%1 AND dig.id IN (";
		sql = sql.arg(scheme.ids_to_sql());

		status_sql = "WHERE ";
		status_sql += scheme.ids_to_sql();
		status_sql += " AND id IN (";

		for (const DIG_Status& status: scheme_status.status_set_)
		{
			sql += QString::number(status.group_id()) + ',';
			status_sql += QString::number(status.status_id()) + ',';
		}

		sql.replace(sql.size() - 1, 1, QChar(')'));
		status_sql.replace(status_sql.size() - 1, 1, QChar(')'));

		status_sql = db.select_query(db_table<DIG_Status_Type>(), status_sql, {
										 DIG_Status_Type::COL_id,
										 DIG_Status_Type::COL_text,
										 DIG_Status_Type::COL_category_id
									 });

		std::string group_title;
		std::map<uint32_t, std::string> group_title_map;
		QSqlQuery q = db.exec(sql);
		while(q.next())
		{
			group_title = q.value(2).toString().toStdString();
			if (group_title.empty())
				group_title = q.value(3).toString().toStdString();

			group_title.insert(0, q.value(1).toString().toStdString() + ' ');
			group_title_map.emplace(q.value(0).toUInt(), group_title);
		}

		std::map<uint32_t, std::pair<QString, uint32_t>> status_map;
		q = db.exec(status_sql);
		while(q.next())
			status_map.emplace(q.value(0).toUInt(), std::make_pair(q.value(1).toString(), q.value(2).toUInt()));

		for (const DIG_Status& status: scheme_status.status_set_)
		{
			auto status_it = status_map.find(status.status_id());
			auto group_it = group_title_map.find(status.group_id());

			if (status_it == status_map.cend() || group_it == group_title_map.cend())
				continue;

			text += '\n';
			text += default_status_category_emoji(status_it->second.second);

			status_text = status_it->second.first;
			for (const QString& arg: status.args())
				status_text = status_text.arg(arg);

			text += ' ' + group_it->second + ": " + status_text.toStdString();
		}
	}

	send_message(msg->recipient->chat_id, text);
}

void Controller::elements(uint32_t user_id, const Scheme_Item &scheme, MaxBot::Message::Ptr message,
				   vector<string>::const_iterator begin, const std::vector<string> &cmd, const std::string& msg_data)
{
	Q_UNUSED(user_id)
	Q_UNUSED(scheme)
	Q_UNUSED(message)
	Q_UNUSED(begin)
	Q_UNUSED(cmd)
	Q_UNUSED(msg_data)
}

void Controller::restart(uint32_t user_id, const Scheme_Item& scheme, MaxBot::Message::Ptr msg)
{
	if (!DB::Helper::is_admin(user_id))
		return;

	QMetaObject::invokeMethod(dbus_iface_, "send_message_to_scheme", Qt::QueuedConnection,
		Q_ARG(uint32_t, scheme.id()), Q_ARG(uint8_t, Das::WS_RESTART), Q_ARG(uint32_t, user_id), Q_ARG(QByteArray, QByteArray()));

	send_message(msg->recipient->chat_id, "🔄 Команда на перезагрузку отправлена!");
}

void Controller::menu_sub_list(const Scheme_Item& scheme, MaxBot::UpdateMessageCallback::Ptr cb, const string& action)
{
	auto keyboard = std::make_shared<MaxBot::InlineKeyboardAttachmentRequest>();

	const string base_data = "scheme." + to_string(scheme.id());
	for (const auto &base_it: get_sub_base_for_scheme(scheme))
	{
		const string data = base_data + ".sub." + to_string(base_it.first) + '.' + action;
		keyboard->payload.buttons.push_back(Bot_Base::makeInlineButtonRow(data, base_it.second));
	}
	keyboard->payload.buttons.push_back(Bot_Base::makeInlineButtonRow(base_data, "Назад (<<)"));

	auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
	attachment->_data = std::move(keyboard);
	auto newMsg = std::make_shared<MaxBot::NewMessageBody>();
	newMsg->attachments.emplace_back(std::move(attachment));
	auto answer = std::make_shared<MaxBot::CallbackAnswer>();
	answer->message = std::move(newMsg);
	bot_->getApi().answerCallbackQuery(cb->callback->callback_id, std::move(answer));
}

void Controller::menu_sub_1(const Scheme_Item& scheme, MaxBot::UpdateMessageCallback::Ptr cb, const string &sub_id)
{
	auto keyboard = std::make_shared<MaxBot::InlineKeyboardAttachmentRequest>();

	const string base_data = "scheme." + to_string(scheme.id());
	const string str = base_data + ".sub." + sub_id + ".sub_1.";
	for (const auto &sub_1_it: get_sub_base_for_scheme(scheme))
		keyboard->payload.buttons.push_back(makeInlineButtonRow(str + to_string(sub_1_it.first), sub_1_it.second));
	keyboard->payload.buttons.push_back(Bot_Base::makeInlineButtonRow(base_data, "Назад (<<)"));

	auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
	attachment->_data = std::move(keyboard);
	auto newMsg = std::make_shared<MaxBot::NewMessageBody>();
	newMsg->attachments.emplace_back(std::move(attachment));
	auto answer = std::make_shared<MaxBot::CallbackAnswer>();
	answer->message = std::move(newMsg);
	bot_->getApi().answerCallbackQuery(cb->callback->callback_id, std::move(answer));
}

void Controller::sub_1(const Scheme_Item& scheme, MaxBot::Message::Ptr msg, uint32_t sub_id, uint32_t sub_1_id)
{
	const string text = "(Ещё не реализованно) Sub 1: " + to_string(sub_1_id)
			+ " на голове " + to_string(sub_id) + " в " + to_string(scheme.id());
	send_message(msg->recipient->chat_id, text);
}

void Controller::menu_sub_2(const Scheme_Item& scheme, MaxBot::UpdateMessageCallback::Ptr cb, const string& sub_id)
{
	auto keyboard = std::make_shared<MaxBot::InlineKeyboardAttachmentRequest>();

	const string base_data = "scheme." + to_string(scheme.id());
	const string str = base_data + ".sub." + sub_id + ".sub_2.";
	vector<MaxBot::Button::Ptr> replaceRow
	{
		Controller::makeInlineButton(str + "action_1", "Действие 1"),
				Controller::makeInlineButton(str + "action_2", "Действие 2"),
				Controller::makeInlineButton(str + "action_3", "Действие 3")
	};
	keyboard->payload.buttons.emplace_back(std::move(replaceRow));
	keyboard->payload.buttons.emplace_back(Bot_Base::makeInlineButtonRow(base_data, "Назад (<<)"));

	auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
	attachment->_data = std::move(keyboard);
	auto newMsg = std::make_shared<MaxBot::NewMessageBody>();
	newMsg->attachments.emplace_back(std::move(attachment));
	auto answer = std::make_shared<MaxBot::CallbackAnswer>();
	answer->message = std::move(newMsg);
	bot_->getApi().answerCallbackQuery(cb->callback->callback_id, std::move(answer));
}

void Controller::sub_2(const Scheme_Item& scheme, MaxBot::Message::Ptr msg, const std::string &sub_2_type)
{
	const string text = "(Ещё не реализованно) Действие Sub 2 тип: " + sub_2_type + " в " + to_string(scheme.id());
	send_message(msg->recipient->chat_id, text);
}

// Helpers

map<uint32_t, string> Controller::list_schemes_names(uint32_t user_id, uint32_t page_number, const string &search_text, size_t &result_count) const
{
	QVariantList values;
	QString sql;
	if (!search_text.empty())
	{
		sql = " AND s.title COLLATE UTF8_GENERAL_CI LIKE ? ESCAPE '@'";
		values.push_back('%' + QString::fromStdString(search_text)
						 .replace('@', "@@").replace('_', "@_").replace('@', "@@") + '%');
		values.push_back(values.back());
	}

	sql = QString(" FROM das_scheme s "
			"LEFT JOIN das_scheme_groups sg ON sg.scheme_id = s.id "
			"LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.scheme_group_id "
			"WHERE sgu.user_id = %1%2 GROUP BY s.id").arg(user_id).arg(sql);

	sql = QString("SELECT s.id, s.title, s.parent_id%1 LIMIT %2, %3;\nSELECT COUNT(*) FROM (SELECT 1%1) as t;")
			.arg(sql).arg(schemes_per_page_ * page_number).arg(schemes_per_page_);

	struct MyScheme : Scheme_Info
	{
		MyScheme(uint32_t id, set<uint32_t> extending_ids, const QString& title) :
			Scheme_Info(id, extending_ids), _title(title) {}
		QString _title;
	};

	Base& db = Base::get_thread_local_instance();
	QSqlQuery q = db.exec(sql, values);

	vector<MyScheme> scheme_vect;
	while (q.next())
		scheme_vect.push_back(MyScheme{q.value(0).toUInt(), {q.value(2).toUInt()}, q.value(1).toString()});

	result_count = q.nextResult() && q.next() ? q.value(0).toUInt() : 0;

	map<uint32_t, string> res;

	const QString status_sql = "SELECT category_id FROM das_dig_status_type WHERE %1 AND id IN (%2) "
							   "ORDER BY category_id DESC LIMIT 1";

	QString status_id_sep;
	Scheme_Status scheme_status;
	std::string name;
	for (const MyScheme& scheme : scheme_vect)
	{
		QMetaObject::invokeMethod(dbus_iface_, "get_scheme_status", Qt::BlockingQueuedConnection,
			Q_RETURN_ARG(Scheme_Status, scheme_status),
			Q_ARG(uint32_t, scheme.id()));

		name = User_Menu::Connection_State::get_emoji(scheme_status.connection_state_);

		status_id_sep.clear();
		for (const DIG_Status& status: scheme_status.status_set_)
		{
			status_id_sep += QString::number(status.status_id());
			status_id_sep += ',';
		}
		if (!status_id_sep.isEmpty())
		{
			status_id_sep.remove(status_id_sep.size() - 1, 1);
			QSqlQuery status_q = db.exec(status_sql.arg(scheme.ids_to_sql()).arg(status_id_sep));
			if (status_q.next())
				name += default_status_category_emoji(status_q.value(0).toUInt());
		}

		name += ' ';
		name += scheme._title.toStdString();
		res.emplace(scheme.id(), name);
	}

	return res;
}

string Controller::getReportFilepathForUser(MaxBot::User::Ptr user) const
{
	Q_UNUSED(user)
	// TODO: generate filepath
	return "/opt/book1.xlsx";
}

unordered_map<uint32_t, string> Controller::get_sub_base_for_scheme(const Scheme_Item& scheme) const
{
	Q_UNUSED(scheme)
	return unordered_map<uint32_t, string> { {2, "Base 2"}, {1, "Base 1"} };
}

unordered_map<uint32_t, string> Controller::get_sub_1_names_for_scheme(const Scheme_Item& scheme) const
{
	Q_UNUSED(scheme)
	return unordered_map<uint32_t, string> { {2, "Item 2"}, {1, "Item 1"} };
}

uint32_t Controller::get_authorized_user_id(uint32_t user_id, const MaxBot::Message::Ptr& msg) const
{
	uint32_t das_user_id = 0;

	Base& db = Base::get_thread_local_instance();
	QSqlQuery q = db.select({MaxBot_User::table_name(), {}, {"user_id"}}, "WHERE id=" + QString::number(user_id));
	if (q.isActive() && q.next())
		das_user_id = q.value(0).toUInt();

	if (!das_user_id && msg)
		send_message(msg->recipient->chat_id, "Для этого действия вам необходимо авторизоваться в личном чате с ботом");
	return das_user_id;
}

void Controller::send_schemes_list(
	uint32_t user_id, const MaxBot::Message::Ptr& msg, uint32_t current_page,
	const std::string& callbackId, const string &search_text) const
{
	auto keyboard = std::make_shared<MaxBot::InlineKeyboardAttachmentRequest>();

	size_t schemes_count;
	map<uint32_t, string> schemes_map = list_schemes_names(user_id, current_page, search_text, schemes_count);

	for (const auto &scheme: schemes_map)
		keyboard->payload.buttons.emplace_back(Bot_Base::makeInlineButtonRow("scheme." + to_string(scheme.first), scheme.second));

	vector<MaxBot::Button::Ptr> row;
	if (current_page > 0)
		row.emplace_back(Bot_Base::makeInlineButton("page.prev." + to_string(current_page) + '.' + search_text, "<<<"));

	if ((current_page + 1) * schemes_per_page_ < schemes_count)
		row.emplace_back(Bot_Base::makeInlineButton("page.next." + to_string(current_page) + '.' + search_text, ">>>"));

	if (!row.empty())
		keyboard->payload.buttons.push_back(row);

	auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
	attachment->_data = std::move(keyboard);
	auto newMsg = std::make_shared<MaxBot::NewMessageBody>();
	newMsg->attachments.emplace_back(std::move(attachment));

	newMsg->format = "markdown";
	newMsg->text = schemes_map.empty() ? "Нет" : "Список";
	newMsg->text += " аппаратов";
	if (!search_text.empty())
		newMsg->text += " найденых по тексту _" + prepare_str(search_text) + '_';
	if (current_page > 0 || schemes_count >= schemes_per_page_)
		newMsg->text += " (стр. " + std::to_string(current_page + 1) + " из " + std::to_string(static_cast<int>(schemes_count / schemes_per_page_) + 1) + ')';
	newMsg->text += ':';

	if (!callbackId.empty())
	{
		auto answer = std::make_shared<MaxBot::CallbackAnswer>();
		answer->message = std::move(newMsg);
		bot_->getApi().answerCallbackQuery(callbackId, std::move(answer));
		return;
	}

	bot_->getApi().sendMessage(msg->recipient->chat_id, 0, std::move(newMsg));
}

void Controller::sendSchemeMenu(MaxBot::UpdateMessageCallback::Ptr cb, const Scheme_Item& scheme, uint32_t user_id) const
{
	auto keyboard = std::make_shared<MaxBot::InlineKeyboardAttachmentRequest>();

	const string base_data = "scheme." + to_string(scheme.id());

	int i = 0;
	for (const User_Menu::Item& item: user_menu_set_)
		keyboard->payload.buttons.push_back(makeInlineButtonRow(base_data + ".user_menu." + std::to_string(i++), item.name()));

	keyboard->payload.buttons.push_back(makeInlineButtonRow(base_data + ".status", "Состояние"));
	keyboard->payload.buttons.push_back(makeInlineButtonRow(base_data + ".elem", "Элементы"));

	if (DB::Helper::is_admin(user_id)) // isAdmin
		keyboard->payload.buttons.push_back(makeInlineButtonRow(base_data + ".restart", "Перезагрузка"));
	keyboard->payload.buttons.push_back(makeInlineButtonRow("list", "Назад (<<)"));

	auto attachment = std::make_shared<MaxBot::AttachmentRequest>();
	attachment->_data = std::move(keyboard);
	auto newMsg = std::make_shared<MaxBot::NewMessageBody>();
	newMsg->attachments.emplace_back(std::move(attachment));
	auto answer = std::make_shared<MaxBot::CallbackAnswer>();
	answer->message = std::move(newMsg);
	bot_->getApi().answerCallbackQuery(cb->callback->callback_id, std::move(answer));
}

void Controller::send_authorization_message(const MaxBot::Message::Ptr& msg) const
{
	if (msg->recipient->chat_type != "dialog")
	{
		send_message(msg->recipient->chat_id, "Чтобы управлять аппратом вы должны сначала авторизоваться в личном чате с ботом");
		return;
	}

	const int64_t chat_id = msg->recipient->chat_id;
	Base& db = Base::get_thread_local_instance();

	const auto user = msg->sender;
	MaxBot_User external_user(user->user_id, 0,
					QString::fromStdString(user->first_name),
					QString::fromStdString(user->last_name),
					QString::fromStdString(user->username),
					"ru", chat_id);

	const QString suffix = "ON DUPLICATE KEY UPDATE first_name=VALUES(first_name), last_name=VALUES(last_name),"
						   "user_name=VALUES(user_name), lang=VALUES(lang), private_chat_id=VALUES(private_chat_id)";
	if (!db.insert(db_table<MaxBot_User>(), MaxBot_User::to_variantlist(external_user), nullptr, suffix))
	{
		send_message(msg->recipient->chat_id, "Ошибка во время добавления пользователя");
		return;
	}

	db.insert(db_table<DB::MaxBot_Chat>(), DB::MaxBot_Chat::to_variantlist(DB::MaxBot_Chat{chat_id, external_user.id()}),
			nullptr, "ON DUPLICATE KEY UPDATE id=id");

	QByteArray data;
	QDataStream ds(&data, QIODevice::WriteOnly);

	qint64 now = DB::Log_Base_Item::current_timestamp();
	ds << MaxBot_User::to_variantlist(external_user) << now << "SomePassword";
	data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

	const MaxBot_Auth auth(user->user_id, now + (3 * 60 * 1000), QString::fromLatin1(data));

	if (db.insert(db_table<MaxBot_Auth>(), MaxBot_Auth::to_variantlist(auth), nullptr,
			  "ON DUPLICATE KEY UPDATE expired = VALUES(expired), token = VALUES(token)"))
	{
		std::string text = "Чтобы продолжить, пожалуйста перейдите по [ссылке](";
		text += _conf._auth_base_url;
		text += "max";
		text += auth.token().toStdString();
		text += ") и авторизуйтесь.";
		send_message(msg->recipient->chat_id, text);
	}
	else
		send_message(msg->recipient->chat_id, "Ошибка во время инициализации привязки пользователя");
}

void Controller::finished()
{
	qDebug() << "finished";
	bot_->getApi().deleteWebhook(_conf._webhook_url);
}

void Controller::send_user_authorized(qint64 external_user_id)
{
	auto msg = std::make_shared<MaxBot::NewMessageBody>();
	msg->text = "Вы успешно авторизованы!";
	bot_->getApi().sendMessage(0, external_user_id, std::move(msg));
}

void Controller::fill_templates()
{
	for(auto& p: fs::directory_iterator(_conf._templates_path))
		if (fs::is_regular_file(p.path()))
			user_menu_set_.emplace(p.path(), dbus_iface_);
}

Scheme_Item Controller::get_scheme(uint32_t user_id, const string &scheme_id) const
{
	Scheme_Item scheme{std::stoi(scheme_id)};
	fill_scheme(user_id, scheme);
	return scheme;
}

void Controller::fill_scheme(uint32_t user_id, Scheme_Item &scheme) const
{
	if (scheme.id() == 0)
		return;

	const QString sql = "SELECT s.parent_id, s.title FROM das_scheme s "
			"LEFT JOIN das_scheme_groups sg ON sg.scheme_id = s.id "
			"LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.scheme_group_id "
			"WHERE sgu.user_id = " + QString::number(user_id) + " AND s.id = " + QString::number(scheme.id());

	Base& db = Base::get_thread_local_instance();
	QSqlQuery q = db.exec(sql);
	if (q.isActive() && q.next())
	{
		scheme.set_extending_scheme_ids({q.value(0).toUInt()});
		scheme.title_ = q.value(1).toString();
	}
	else
		scheme.set_id(0);
}

} // namespace Bot
} // namespace Das
