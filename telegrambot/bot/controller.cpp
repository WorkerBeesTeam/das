#include <vector>
#include <string>
#include <algorithm>

#include <filesystem>

#include <boost/algorithm/string.hpp>

#include <QDebug>
#include <QDateTime>
#include <QDataStream>
#include <QCryptographicHash>

//#define WEBHOOK
#include <tgbot/TgException.h>
#ifndef WEBHOOK
#include <tgbot/net/TgLongPoll.h>
#endif

#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include <dbus/dbus_interface.h>
#include <Das/db/dig_status_type.h>
#include <Das/log/log_base_item.h>
#include <Das/commands.h>
#include <plus/das/database.h>

#include "db/tg_auth.h"
#include "db/tg_user.h"
#include "db/tg_chat.h"
#include "db/tg_subscriber.h"

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
    QThread(), Bot_Base(dbus_iface),
    stop_flag_(false), bot_(nullptr), server_(nullptr),
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

void Controller::stop()
{
    stop_flag_ = true;
    if (server_)
        server_->stop();
}

void Controller::send_message(int64_t chat_id, const string& text) const
{
    try
    {
        bot_->getApi().sendMessage(chat_id, text, false, 0, make_shared<TgBot::GenericReply>(), "Markdown");
    }
    catch(const TgBot::TgException& e)
    {
        std::cerr << "Fail send message to " << chat_id << ' ' << e.what() << " text: " << text << std::endl;
    }
    catch(...) { std::cerr << "Send message unknown exception" << std::endl; }
}

void Controller::init()
{
    bot_ = new TgBot::Bot(_conf._token);

    bot_user_ = bot_->getApi().getMe();
    if (bot_user_)
        qDebug() << "Bot initialized. Id:" << bot_user_->id << "Name:" << bot_user_->username.c_str();
    else
        qCritical() << "Can't initialize bot";

    bot_->getEvents().onCommand("start", [this](TgBot::Message::Ptr message)
    {
        uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id, true);
        if (user_id == 0)
            send_authorization_message(*message);
        else
        {
            // TODO: logout keyboard button
            bot_->getApi().sendMessage(message->chat->id, "Вы уже авторизованы", false, 0, make_shared<TgBot::GenericReply>(), "Markdown");
        }
    });
    bot_->getEvents().onCommand("find", [this](TgBot::Message::Ptr message)
    {
        uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id);
        if (user_id != 0)
            find(user_id, std::move(message));
    });
    bot_->getEvents().onCommand("list", [this](TgBot::Message::Ptr message)
    {
        uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id);
        if (user_id != 0)
            list(user_id, std::move(message));
    });
    bot_->getEvents().onCommand("report", [this](TgBot::Message::Ptr message)
    {
        uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id);
        if (user_id != 0)
            report(std::move(message));
    });

    auto inform_func = [this](TgBot::Message::Ptr message)
    {
        uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id);
        if (user_id != 0)
            inform_onoff(user_id, message->chat);
    };
    bot_->getEvents().onCommand("inform", inform_func); // deprecated
    bot_->getEvents().onCommand("inform_onoff", inform_func);

    bot_->getEvents().onCommand("help", [this](TgBot::Message::Ptr message)
    {
        uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id);
        if (user_id != 0)
            help(std::move(message));
    });

    bot_->getEvents().onCallbackQuery([this](TgBot::CallbackQuery::Ptr q)
    {
        uint32_t user_id = get_authorized_user_id(q->from->id, q->message->chat->id);

        qDebug() << "Data:" << q->data.c_str()
                 << "Text:" << q->message->text.c_str()
                 << "ChatInstance:" << q->chatInstance.c_str()
                 << "InlineMessageId:" << q->inlineMessageId.c_str()
                 << "Msg User id:" << q->message->from->id
                 << "Tg User id:" << q->from->id
                 << "User id:" << user_id;

        if (user_id == 0)
            return;

        std::string answer_text;
        try
        {
            answer_text = process_directory(user_id, q->message, q->data, q->from->id);
        }
        catch(const std::exception& e)
        {
            qCritical() << "onCallBack process_directory exception:" << e.what();
        }

        try
        {
            bot_->getApi().answerCallbackQuery(q->id, answer_text);
        }
        catch(const std::exception& e)
        {
            qCritical() << "onCallBack exception:" << e.what();
        }
    });

    bot_->getEvents().onAnyMessage([this](TgBot::Message::Ptr message) { anyMessage(message); });
}

void Controller::run()
{
    qDebug() << "started thread";
    try
    {
        if (_conf._token.empty())
        {
            qWarning() << "TgBot token is empty";
            return;
        }
        init();

#ifdef WEBHOOK
        server_ = new TgBot::TgWebhookTcpServer(_conf._port, "/", bot_->getEventHandler());
        TgBot::InputFile::Ptr certificate;
        if (!_conf._webhook_cert.empty())
        {
            certificate = TgBot::InputFile::fromFile(_conf._webhook_cert, "application/x-pem-file"); // or text/plain // or application/x-x509-ca-cert
        }
        bot_->getApi().setWebhook(_conf._webhook_url, std::move(certificate));
        server_->start();
#else
        qDebug() << "Use debug TgLongPoll";
        TgBot::TgLongPoll longPoll(*bot_);
        while (!stop_flag_)
        {
            longPoll.start();
        }
#endif
    }
    catch (const std::exception &e)
    {
        qCritical() << "TgBot exception:" << e.what();
    }
    catch (...)
    {
        qCritical() << "TgBot unknown exception";
    }

#ifdef WEBHOOK
    if (server_)
    {
        bot_->getApi().deleteWebhook();

        delete server_;
        server_ = nullptr;
    }
#endif

    if (bot_user_)
        bot_user_.reset();

    if (bot_)
    {
        delete bot_;
        bot_ = nullptr;
    }
}

void Controller::anyMessage(TgBot::Message::Ptr message)
{
    if (!message)
        return;

    auto dbg = qDebug() << "Chat:" << message->chat->id << message->chat->title.c_str() << "Sender:" << message->from->id << message->from->username.c_str();

    if (message->groupChatCreated)
    {
        dbg << "Group chat created";
    }
    else if (message->migrateToChatId != 0 || message->migrateFromChatId != 0)
    {
        dbg << "Chat migrate from" << message->migrateFromChatId << "to" << message->migrateToChatId;
    }
    else if (message->leftChatMember)
    {
        if (message->leftChatMember->isBot)
        {
            if (bot_user_->id == message->leftChatMember->id)
            {
                dbg << "this bot was removed from chat";

                Base& db = Base::get_thread_local_instance();

                QString field_name = db_table<Tg_Subscriber>().field_names().at(Tg_Subscriber::COL_chat_id);
                db.del(db_table_name<Tg_Subscriber>(), field_name + '=' + QString::number(message->chat->id));

                field_name = db_table<DB::Tg_Chat>().field_names().at(DB::Tg_Chat::COL_id);
                db.del(db_table_name<DB::Tg_Chat>(), field_name + '=' + QString::number(message->chat->id));
            }
        }
    }
    else if (!message->newChatMembers.empty())
    {
        for (TgBot::User::Ptr user: message->newChatMembers)
        {
            if (user->id == bot_user_->id)
            {
                dbg << "this bot was added to chat";
                break;
            }
        }
    }
    else
    {
        dbg << "Text:" << message->text.c_str();

        if (!message->text.empty())
        {
            if (message->text == "/help")
                return;

            auto it = waited_map_.find(message->chat->id);
            if (it != waited_map_.end() && it->second.tg_user_id_ == message->from->id)
            {
                uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id);
                if (user_id && !it->second.data_.empty())
                {
                    const vector<string> cmd = StringTools::split(it->second.data_, '.');
                    if (cmd.front() == "scheme")
                    {
                        Elements elements(*this, user_id, it->second.scheme_, cmd, it->second.data_, message->text);
                        elements.generate_answer();

                        if (!elements.text_.empty())
                            bot_->getApi().sendMessage(message->chat->id, elements.text_, false, 0, elements.keyboard_, "Markdown");
                    }
                    else if (cmd.front() == "find")
                    {
                        find(user_id, message->chat, message->text);
                    }
                    else if (cmd.front() == "help")
                        return;
                }

                waited_map_.erase(it);
            }
#ifdef QT_DEBUG
            send_message(message->chat->id, "*You send*: " + prepare_str(message->text));
#endif
        }
    }
}

string Controller::process_directory(uint32_t user_id, TgBot::Message::Ptr message, const string &msg_data, int32_t tg_user_id)
{
    const vector<string> cmd = StringTools::split(msg_data, '.');
    const string directory = cmd.at(0);

    if (directory == "page")
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
        send_schemes_list(user_id, message->chat, current_page, message, search_text);
    }
    else if (directory == "list")
    {
        send_schemes_list(user_id, message->chat, 0, message);
    }
    else if (directory == "scheme")
    {
        if (cmd.size() < 2)
            throw std::runtime_error("Unknown scheme argument count: " + to_string(cmd.size()));

        const Scheme_Item scheme = get_scheme(user_id, cmd.at(1));

        if (!scheme.id())
            throw std::runtime_error("Unknown scheme id");

        if (cmd.size() >= 3)
        {
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
                    send_message(message->chat->id, text);
                }
            }
            else if (action == "status")
                status(scheme, message);
            else if (action == "elem")
            {
                Elements elements(*this, user_id, scheme, cmd, msg_data);
                elements.generate_answer();

                if (elements.skip_edit_)
                {
                    if (!elements.text_.empty())
                    {
                        Waited_Item& waited_item = waited_map_[message->chat->id];
                        waited_item.tg_user_id_ = tg_user_id;
                        waited_item.expired_time_ = std::chrono::system_clock::now() + 30s;
                        waited_item.data_ = msg_data;
                        waited_item.scheme_ = scheme;

                        send_message(message->chat->id, elements.text_);
                    }
                }
                else if (elements.text_.empty())
                    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", elements.keyboard_);
                else
                    bot_->getApi().editMessageText(elements.text_, message->chat->id, message->messageId, "", "Markdown", false, elements.keyboard_);
            }

            else if (action == "menu_sub_1")
                menu_sub_list(scheme, message, "sub_1");
            else if (action == "menu_sub_2")
                menu_sub_list(scheme, message, "sub_2");
            else if (action == "sub" && cmd.size() >= 5)
            {
//                    uint32_t sub_id = std::stoi(cmd.at(3));
                const string &sub_id = cmd.at(3);
                const string &sub_action = cmd.at(4);

                if (sub_action == "sub_1")
                {
                    if (cmd.size() > 5)
                    {
                        const string &sub_1_id = cmd.at(5);
                        sub_1(scheme, message, std::stoi(sub_id), std::stoi(sub_1_id));
                    }
                    else
                        menu_sub_1(scheme, message, sub_id);
                }
                else if (sub_action == "sub_2")
                {
                    if (cmd.size() > 5)
                    {
                        const string &sub_2_type = cmd.at(5);
                        sub_2(scheme, message, sub_2_type);
                    }
                    else
                        menu_sub_2(scheme, message, sub_id);
                }
                else
                {
                    throw std::runtime_error("Invalid sub action: " + sub_action);
                }
            }
            else if (action == "restart")
            {
                restart(user_id, scheme, message);
            }
            else
            {
                throw std::runtime_error("Unknown action for scheme: " + action);
            }
        }
        else
            sendSchemeMenu(message, scheme, user_id);
    }
    else if (directory == "subscriber")
    {
        if (cmd.size() < 2)
            throw std::runtime_error("Unknown subscriber argument count: " + to_string(cmd.size()));

        const char* group_id_str = cmd.at(1).c_str();
        uint32_t group_id = atoi(group_id_str);
        qDebug() << "subscriber scheme group:" << group_id_str;

        const QString sql =
                "SELECT sg.id, tgs.id FROM das_scheme_group sg "
                "LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.id "
                "LEFT JOIN das_tg_subscriber tgs ON tgs.group_id = sg.id AND tgs.chat_id = %1 "
                "WHERE sgu.user_id = %2 AND sg.id = %3 ORDER BY sg.id";

        Base& db = Base::get_thread_local_instance();
        QSqlQuery q = db.exec(sql.arg(message->chat->id).arg(user_id).arg(group_id));
        if (!q.next())
            throw std::runtime_error("Unknown scheme group for user");

        if (q.value(1).isNull())
        {
            db.insert(db_table<DB::Tg_Chat>(), DB::Tg_Chat::to_variantlist(DB::Tg_Chat{message->chat->id, tg_user_id}));

            Table table = db_table<Tg_Subscriber>();
            table.field_names().removeAt(0);
            if (!db.insert(table, {(qint64)message->chat->id, group_id}))
                throw std::runtime_error("Failed add subscriber");
        }
        else
        {
            if (!db.del(Tg_Subscriber::table_name(), "id=" + q.value(1).toString()).isActive())
                throw std::runtime_error("Failed remove subscriber");
        }

        inform_onoff(user_id, message->chat, message);
    }
    else
    {
        throw std::runtime_error("Unhandled directory for callback: " + directory);
    }

    return {};
}

// Chat methods
void Controller::find(uint32_t user_id, TgBot::Message::Ptr message)
{
    const string find_str = "/find ";
    if (message->text.size() < find_str.size() + 1)
    {
        Waited_Item& waited_item = waited_map_[message->chat->id];
        waited_item.tg_user_id_ = message->from->id;
        waited_item.expired_time_ = std::chrono::system_clock::now() + 30s;
        waited_item.data_ = "find";

        send_message(message->chat->id, "Отправьте текст для поиска аппарата:");
    }
    else
    {
        string search_text = message->text.substr(find_str.size());
        find(user_id, message->chat, std::move(search_text));
    }
}

void Controller::find(uint32_t user_id, TgBot::Chat::Ptr chat, string text)
{
    text.erase(std::remove(text.begin(), text.end(), '.'), text.end());
    text.erase(std::remove(text.begin(), text.end(), '\''), text.end());
    text.erase(std::remove(text.begin(), text.end(), '"'), text.end());
    text.erase(std::remove(text.begin(), text.end(), ';'), text.end());
    boost::trim(text);

    qDebug() << "Searching for:" << text.c_str();
    send_schemes_list(user_id, chat, 0, nullptr, text);
}

void Controller::list(uint32_t user_id, TgBot::Message::Ptr message) const
{
    send_schemes_list(user_id, message->chat);
}

void Controller::report(TgBot::Message::Ptr message) const
{
    try {
        TgBot::InputFile::Ptr file = TgBot::InputFile::fromFile(getReportFilepathForUser(message->from), REPORT_MIME);
        bot_->getApi().sendDocument(message->chat->id, file);
    } catch (const std::exception& e) {
        bot_->getApi().sendMessage(message->chat->id, std::string("Ошибка при чтении файла: ") + e.what());
    }
}

void Controller::inform_onoff(uint32_t user_id, TgBot::Chat::Ptr chat, TgBot::Message::Ptr msg_to_update)
{
    const QString sql =
            "SELECT sg.id, sg.name, tgs.id FROM das_scheme_group sg "
            "LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.id "
            "LEFt join das_tg_subscriber tgs ON tgs.group_id = sg.id AND tgs.chat_id = %1 "
            "WHERE sgu.user_id = %2 ORDER BY sg.id";

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql.arg(chat->id).arg(user_id));

    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
    while (q.next())
    {
        TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
        if (!q.value(2).isNull())
            button->text = "✅ ";
        button->text += q.value(1).toString().toStdString();
        button->callbackData = string("subscriber.") + q.value(0).toString().toStdString();

        keyboard->inlineKeyboard.push_back({button});
    }

    string text = "Группы аппаратов:";
    if (msg_to_update)
        bot_->getApi().editMessageReplyMarkup(chat->id, msg_to_update->messageId, "", keyboard);
    else
        bot_->getApi().sendMessage(chat->id, text, false, 0, keyboard);
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

void Controller::help(TgBot::Message::Ptr message)
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
            auto it = waited_map_.find(message->chat->id);
            if (it != waited_map_.end()
                && it->second.tg_user_id_ == message->from->id
                && it->second.data_ == "help")
            {
                TgBot::InputFile::Ptr file = TgBot::InputFile::fromFile(help_file_path, mime_type);
                bot_->getApi().sendDocument(message->chat->id, file);

                waited_map_.erase(it);
                return;
            }
            else
            {
                text += " Или отправьте команду /help ещё раз чтобы получить файл справки.";

                Waited_Item& waited_item = waited_map_[message->chat->id];
                waited_item.tg_user_id_ = message->from->id;
                waited_item.expired_time_ = std::chrono::system_clock::now() + 30s;
                waited_item.data_ = "help";
            }
        }
    }

    bot_->getApi().sendMessage(message->chat->id, text);
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

void Controller::status(const Scheme_Item& scheme, TgBot::Message::Ptr message)
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

    send_message(message->chat->id, text);
}

void Controller::elements(uint32_t user_id, const Scheme_Item &scheme, TgBot::Message::Ptr message,
                   vector<string>::const_iterator begin, const std::vector<string> &cmd, const std::string& msg_data)
{
    Q_UNUSED(user_id)
    Q_UNUSED(scheme)
    Q_UNUSED(message)
    Q_UNUSED(begin)
    Q_UNUSED(cmd)
    Q_UNUSED(msg_data)
}

void Controller::restart(uint32_t user_id, const Scheme_Item& scheme, TgBot::Message::Ptr message)
{
    if (!DB::Helper::is_admin(user_id))
        return;

    QMetaObject::invokeMethod(dbus_iface_, "send_message_to_scheme", Qt::QueuedConnection,
        Q_ARG(uint32_t, scheme.id()), Q_ARG(uint8_t, Das::WS_RESTART), Q_ARG(uint32_t, user_id), Q_ARG(QByteArray, QByteArray()));

    bot_->getApi().sendMessage(message->chat->id, "🔄 Команда на перезагрузку отправлена!");
}

void Controller::sub_1_list(const Scheme_Item& scheme, TgBot::Message::Ptr message)
{
    const unordered_map<uint32_t, string> sub_1_names = get_sub_1_names_for_scheme(scheme);
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());
    for (const auto &sub_1_it: sub_1_names)
    {
        const string data = base_data + ".menu_sub_1." + to_string(sub_1_it.first);
        keyboard->inlineKeyboard.push_back(Controller::makeInlineButtonRow(data, sub_1_it.second));
    }

    keyboard->inlineKeyboard.push_back(Controller::makeInlineButtonRow(base_data, "Назад (<<)"));
    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
}

void Controller::menu_sub_list(const Scheme_Item& scheme, TgBot::Message::Ptr message, const string& action)
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());
    for (const auto &base_it: get_sub_base_for_scheme(scheme))
    {
        const string data = base_data + ".sub." + to_string(base_it.first) + '.' + action;
        keyboard->inlineKeyboard.push_back(makeInlineButtonRow(data, base_it.second));
    }

    keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data, "Назад (<<)"));
    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
}

void Controller::menu_sub_1(const Scheme_Item& scheme, TgBot::Message::Ptr message, const string &sub_id)
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());
    const string str = base_data + ".sub." + sub_id + ".sub_1.";
    for (const auto &sub_1_it: get_sub_base_for_scheme(scheme))
    {
        keyboard->inlineKeyboard.push_back(makeInlineButtonRow(str + to_string(sub_1_it.first), sub_1_it.second));
    }

    keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data, "Назад (<<)"));
    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
}

void Controller::sub_1(const Scheme_Item& scheme, TgBot::Message::Ptr message, uint32_t sub_id, uint32_t sub_1_id)
{
    const string text = "(Ещё не реализованно) Sub 1: " + to_string(sub_1_id)
            + " на голове " + to_string(sub_id) + " в " + to_string(scheme.id());
    bot_->getApi().sendMessage(message->chat->id, text);
}

void Controller::menu_sub_2(const Scheme_Item& scheme, TgBot::Message::Ptr message, const string& sub_id)
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());
    const string str = base_data + ".sub." + sub_id + ".sub_2.";
    vector<TgBot::InlineKeyboardButton::Ptr > replaceRow
    {
        Controller::makeInlineButton(str + "action_1", "Действие 1"),
                Controller::makeInlineButton(str + "action_2", "Действие 2"),
                Controller::makeInlineButton(str + "action_3", "Действие 3")
    };

    keyboard->inlineKeyboard.push_back(replaceRow);
    keyboard->inlineKeyboard.push_back(Controller::makeInlineButtonRow(base_data, "Назад (<<)"));

    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
}

void Controller::sub_2(const Scheme_Item& scheme, TgBot::Message::Ptr message, const std::string &sub_2_type)
{
    const string text = "(Ещё не реализованно) Действие Sub 2 тип: " + sub_2_type + " в " + to_string(scheme.id());
    bot_->getApi().sendMessage(message->chat->id, text);
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

string Controller::getReportFilepathForUser(TgBot::User::Ptr user) const
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

uint32_t Controller::get_authorized_user_id(uint32_t user_id, int64_t chat_id, bool skip_message) const
{
    uint32_t das_user_id = 0;

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.select({Tg_User::table_name(), {}, {"user_id"}}, "WHERE id=" + QString::number(user_id));
    if (q.isActive() && q.next())
        das_user_id = q.value(0).toUInt();

    if (!skip_message && !das_user_id)
        send_message(chat_id, "Для этого действия вам необходимо авторизоваться в личном чате с ботом");
    return das_user_id;
}

void Controller::send_schemes_list(uint32_t user_id, TgBot::Chat::Ptr chat, uint32_t current_page,
                              TgBot::Message::Ptr msg_to_update, const string &search_text) const
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

    size_t schemes_count;
    map<uint32_t, string> schemes_map = list_schemes_names(user_id, current_page, search_text, schemes_count);

    for (const auto &scheme: schemes_map)
    {
        TgBot::InlineKeyboardButton::Ptr button = std::make_shared<TgBot::InlineKeyboardButton>();
        button->text = scheme.second;
        button->callbackData = string("scheme.") + to_string(scheme.first);

        keyboard->inlineKeyboard.push_back({button});
    }

    vector<TgBot::InlineKeyboardButton::Ptr> row;
    if (current_page > 0)
    {
        TgBot::InlineKeyboardButton::Ptr prev_page_button = std::make_shared<TgBot::InlineKeyboardButton>();
        prev_page_button->text = "<<<";	// TODO: here you can change text of "previous page" button
        prev_page_button->callbackData = string("page.prev.") + to_string(current_page) + '.' + search_text;
        row.push_back(prev_page_button);
    }

    if ((current_page + 1) * schemes_per_page_ < schemes_count)
    {
        TgBot::InlineKeyboardButton::Ptr next_page_button = std::make_shared<TgBot::InlineKeyboardButton>();
        next_page_button->text = ">>>";	// TODO: here you can change text of "next page" button
        next_page_button->callbackData = string("page.next.") + to_string(current_page) + '.' + search_text;
        row.push_back(next_page_button);
    }

    if (!row.empty())
        keyboard->inlineKeyboard.push_back(row);

    string text = schemes_map.empty() ? "Нет" : "Список";

    text += " аппаратов";
    if (!search_text.empty())
    {
        text += " найденых по тексту _";
        text += prepare_str(search_text);
        text += "_";
    }

    if (current_page > 0 || schemes_count >= schemes_per_page_)
    {
        text += " (стр. ";
        text += to_string(current_page + 1);
        text += " из ";
        text += to_string(static_cast<int>(schemes_count / schemes_per_page_) + 1);
        text += ')';
    }

    text += ':';

    if (msg_to_update)
    {
//        bot_->getApi().editMessageReplyMarkup(chat->id, msg_to_update->messageId, "", keyboard);
        bot_->getApi().editMessageText(text, chat->id, msg_to_update->messageId, "", "Markdown", false, keyboard);
    }
    else
    {
        bot_->getApi().sendMessage(chat->id, text, false, 0, keyboard, "Markdown", false);
    }
}

void Controller::sendSchemeMenu(TgBot::Message::Ptr message, const Scheme_Item& scheme, uint32_t user_id) const
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());

    int i = 0;
    for (const User_Menu::Item& item: user_menu_set_)
        keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data + ".user_menu." + std::to_string(i++), item.name()));

    keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data + ".status", "Состояние"));
    keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data + ".elem", "Элементы"));
//    keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data + ".menu_sub_1", "Под меню 1")),
//    keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data + ".menu_sub_2", "Под меню 2"));

    if (DB::Helper::is_admin(user_id)) // isAdmin
        keyboard->inlineKeyboard.push_back(makeInlineButtonRow(base_data + ".restart", "Перезагрузка"));
    keyboard->inlineKeyboard.push_back(makeInlineButtonRow("list", "Назад (<<)"));

//    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
    bot_->getApi().editMessageText(scheme.title_.toStdString(), message->chat->id, message->messageId, "", "", false, keyboard);
}

void Controller::send_authorization_message(const TgBot::Message& msg) const
{
    const int64_t chat_id = msg.chat->id;
    if (msg.chat->type != TgBot::Chat::Type::Private)
    {
        send_message(chat_id, "Чтобы управлять аппратом вы должны сначала авторизоваться в личном чате с ботом");
        return;
    }

    Base& db = Base::get_thread_local_instance();

    const auto user = msg.from;
    Tg_User tg_user(user->id, 0,
                    QString::fromStdString(user->firstName),
                    QString::fromStdString(user->lastName),
                    QString::fromStdString(user->username),
                    QString::fromStdString(user->languageCode),
                    chat_id);

    const QString suffix = "ON DUPLICATE KEY UPDATE first_name=VALUES(first_name), last_name=VALUES(last_name),"
                           "user_name=VALUES(user_name), lang=VALUES(lang), private_chat_id=VALUES(private_chat_id)";
    if (!db.insert(db_table<Tg_User>(), Tg_User::to_variantlist(tg_user), nullptr, suffix))
    {
        send_message(chat_id, "Ошибка во время добавления пользователя");
        return;
    }

    db.insert(db_table<DB::Tg_Chat>(), DB::Tg_Chat::to_variantlist(DB::Tg_Chat{chat_id, tg_user.id()}));

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);

    qint64 now = DB::Log_Base_Item::current_timestamp();
    ds << Tg_User::to_variantlist(tg_user) << now << "SomePassword";
    data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

    const Tg_Auth auth(user->id, now + (3 * 60 * 1000), QString::fromLatin1(data));

    if (db.insert(db_table<Tg_Auth>(), Tg_Auth::to_variantlist(auth), nullptr,
              "ON DUPLICATE KEY UPDATE expired = VALUES(expired), token = VALUES(token)"))
    {
        std::string text = "Чтобы продолжить, пожалуйста перейдите по ссылке ниже и авторизуйтесь.\n\n";
        text += _conf._auth_base_url;
        text += auth.token().toStdString();
//        send_message(chat_id, text);

        bot_->getApi().sendMessage(chat_id, text);
    }
    else
        send_message(chat_id, "Ошибка во время инициализации привязки пользователя");
}

void Controller::finished()
{
    qDebug() << "finished";
    bot_->getApi().deleteWebhook();
}

void Controller::send_user_authorized(qint64 tg_user_id)
{
    send_message(tg_user_id, "Вы успешно авторизованы!");
}

void Controller::fill_templates()
{
    for(auto& p: fs::directory_iterator(_conf._templates_path))
        if (fs::is_regular_file(p.path()))
            user_menu_set_.emplace(p.path(), dbus_iface_);
}

Scheme_Item Controller::get_scheme(uint32_t user_id, const string &scheme_id) const
{
    Scheme_Item scheme{std::atoi(scheme_id.c_str())};
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
