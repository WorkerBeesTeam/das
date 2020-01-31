#include <vector>
#include <string>
#include <algorithm>

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
#include <Das/commands.h>

#include "db/tg_auth.h"
#include "db/tg_user.h"
#include "db/tg_subscriber.h"

#include "bot.h"

namespace Das {

using namespace std;
using namespace Helpz::Database;

Bot::Bot(DBus::Interface *dbus_iface, const string &token, const string& webhook_url, uint16_t port, const string &webhook_cert) :
    QThread(),
    stop_flag_(false), port_(port), bot_(nullptr), server_(nullptr), token_(token),
    webhook_url_(webhook_url), webhook_cert_(webhook_cert),
    dbus_iface_(dbus_iface)
{
}

Bot::~Bot()
{
    stop();
}

void Bot::stop()
{
    stop_flag_ = true;
    if (server_)
        server_->stop();
}

void Bot::send_message(int64_t chat_id, const string& text) const
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

void Bot::init()
{
    bot_ = new TgBot::Bot(token_);

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
    bot_->getEvents().onCommand("inform_onoff", [this](TgBot::Message::Ptr message)
    {
        uint32_t user_id = get_authorized_user_id(message->from->id, message->chat->id);
        if (user_id != 0)
            inform_onoff(user_id, message->chat);
    });
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

        const vector<string> cmd = StringTools::split(q->data, '.');
        try
        {
            process_directory(user_id, cmd, q->message);
        }
        catch(const std::exception& e)
        {
            qCritical() << "onCallBack exception:" << e.what();
        }

        bot_->getApi().answerCallbackQuery(q->id);
    });

    bot_->getEvents().onAnyMessage([this](TgBot::Message::Ptr message) { anyMessage(message); });
}

void Bot::run()
{
    qDebug() << "started thread";
    try
    {
        if (token_.empty())
        {
            qWarning() << "TgBot token is empty";
            return;
        }
        init();

#ifdef WEBHOOK
        server_ = new TgBot::TgWebhookTcpServer(port_, "/", bot_->getEventHandler());
        TgBot::InputFile::Ptr certificate;
        if (!webhook_cert_.empty())
        {
            certificate = TgBot::InputFile::fromFile(webhook_cert_, "application/x-pem-file"); // or text/plain // or application/x-x509-ca-cert
        }
        bot_->getApi().setWebhook(webhook_url_, std::move(certificate));
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

void Bot::anyMessage(TgBot::Message::Ptr message)
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

#ifdef QT_DEBUG
        if (!message->text.empty())
            bot_->getApi().sendMessage(message->chat->id, "*You send*: " + message->text, false, 0, make_shared<TgBot::GenericReply>(), "Markdown");
#endif
    }
}

void Bot::process_directory(uint32_t user_id, const vector<string>& cmd, TgBot::Message::Ptr message)
{
    const string directory = cmd.at(0);

    if (directory == "page")
    {
        if (cmd.size() < 4)
            throw std::runtime_error("Unknown pagination argument count: " + to_string(cmd.size()));

        const string direction = cmd.at(1);
        uint32_t current_page = static_cast<uint32_t> (atoi(cmd.at(2).c_str()));
        if (direction == "next")
            ++current_page;
        else if (direction == "prev")
            --current_page;
        else
            throw std::runtime_error("Unknown pagination direction: " + direction);

        send_schemes_list(user_id, message->chat, current_page, message, cmd.at(3));
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
            if (action == "status")
            {
                status(scheme, message);
            }
            else if (action == "menu_sub_1")
            {
                menu_sub_list(scheme, message, "sub_1");
            }
            else if (action == "menu_sub_2")
            {
                menu_sub_list(scheme, message, "sub_2");
            }
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
            sendSchemeMenu(message, scheme);
    }
    else if (directory == "subscriber")
    {
        if (cmd.size() < 2)
            throw std::runtime_error("Unknown subscriber argument count: " + to_string(cmd.size()));

        const char* group_id_str = cmd.at(1).c_str();
        uint32_t group_id = atoi(group_id_str);
        qDebug() << "subscriber team:" << group_id_str;

        const QString sql =
                "SELECT sg.id, tgs.id FROM das_scheme_group sg "
                "LEFT JOIN das_scheme_group_user sgu ON sgu.group_id = sg.id "
                "LEFt join das_tg_subscriber tgs ON tgs.group_id = sg.id AND tgs.chat_id = %1 "
                "WHERE sgu.user_id = %2 AND sg.id = %3 ORDER BY sg.id";

        Base& db = Base::get_thread_local_instance();
        QSqlQuery q = db.exec(sql.arg(message->chat->id).arg(user_id).arg(group_id));
        if (!q.next())
            throw std::runtime_error("Unknown scheme group for user");

        if (q.value(1).isNull())
        {
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
}

// Chat methods
void Bot::find(uint32_t user_id, TgBot::Message::Ptr message) const
{
    const string find_str = "/find ";
    if (message->text.size() < find_str.size() + 1)
    {
        help(message);
        return;
    }

    string search_text = message->text.substr(find_str.size());
    search_text.erase(std::remove(search_text.begin(), search_text.end(), '.'), search_text.end());
    search_text.erase(std::remove(search_text.begin(), search_text.end(), '\''), search_text.end());
    search_text.erase(std::remove(search_text.begin(), search_text.end(), '"'), search_text.end());
    search_text.erase(std::remove(search_text.begin(), search_text.end(), ';'), search_text.end());
    boost::trim(search_text);

    qDebug() << "Searching for:" << search_text.c_str();
    send_schemes_list(user_id, message->chat, 0, nullptr, search_text);
}

void Bot::list(uint32_t user_id, TgBot::Message::Ptr message) const
{
    send_schemes_list(user_id, message->chat);
}

void Bot::report(TgBot::Message::Ptr message) const
{
    TgBot::InputFile::Ptr file = TgBot::InputFile::fromFile(getReportFilepathForUser(message->from), REPORT_MIME);
    bot_->getApi().sendDocument(message->chat->id, file);
}

void Bot::inform_onoff(uint32_t user_id, TgBot::Chat::Ptr chat, TgBot::Message::Ptr msg_to_update)
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

void Bot::help(TgBot::Message::Ptr message) const
{
    bot_->getApi().sendMessage(message->chat->id, "(Ещё не реализованно) TODO: insert text here");
}

void Bot::status(const Scheme_Item& scheme, TgBot::Message::Ptr message)
{
    Scheme_Status scheme_status;
    QMetaObject::invokeMethod(dbus_iface_, "get_scheme_status", Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(Scheme_Status, scheme_status),
        Q_ARG(uint32_t, scheme.id()));

    Base& db = Base::get_thread_local_instance();
    QString group_title, sql, status_text, status_sql,
            text = '*' + scheme.title_ + "*\n";

    if ((scheme_status.connection_state_ & ~CS_FLAGS) >= CS_CONNECTED_JUST_NOW)
        text += "🚀 На связи!\n";
    else
    {
        text += "💢 Не подключен!\n";

        scheme_status.status_set_ =
                db_build_list<DIG_Status, std::set>(db, "WHERE scheme_id = " + QString::number(scheme.id()));
    }

    if (!scheme_status.status_set_.empty())
    {
        sql = "SELECT dig.id, s.name, dig.title, gt.title "
              "FROM das_device_item_group dig "
              "LEFT JOIN das_section s ON s.id = dig.section_id AND s.scheme_id = %1 "
              "LEFT JOIN das_grouptype gt ON gt.id = dig.type_id AND gt.scheme_id = %1 "
              "WHERE dig.scheme_id = %1 dig.id IN (";
        sql = sql.arg(scheme.parent_id_or_id());

        status_sql = "SELECT id, text, category_id FROM das_status_type WHERE scheme_id = ";
        status_sql += QString::number(scheme.parent_id_or_id());
        status_sql += " AND id IN (";

        for (const DIG_Status& status: scheme_status.status_set_)
        {
            sql += QString::number(status.group_id()) + ',';
            status_sql += QString::number(status.status_id()) + ',';
        }

        sql.replace(sql.size() - 1, 1, QChar(')'));
        status_sql.replace(status_sql.size() - 1, 1, QChar(')'));

        std::map<uint32_t, QString> group_title_map;
        QSqlQuery q = db.exec(sql);
        while(q.next())
        {
            group_title = q.value(2).toString();
            if (group_title.isEmpty())
            {
                group_title = q.value(3).toString();
            }
            group_title.insert(0, q.value(1).toString() + ' ');
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
            switch (status_it->second.second)
            {
            case 2: text += "✅"; break;
            case 3: text += "⚠️"; break;
            case 4: text += "🚨"; break;
            default:
                break;
            }

            status_text = status_it->second.first;
            for (const QString& arg: status.args())
                status_text = status_text.arg(arg);

            text += ' ' + group_it->second + ": " + status_text;
        }
    }

    send_message(message->chat->id, text.toStdString());
}

void Bot::restart(uint32_t user_id, const Scheme_Item& scheme, TgBot::Message::Ptr message)
{
    QMetaObject::invokeMethod(dbus_iface_, "send_message_to_scheme", Qt::QueuedConnection,
        Q_ARG(uint32_t, scheme.id()), Q_ARG(uint8_t, Das::WS_RESTART), Q_ARG(uint32_t, user_id), Q_ARG(QByteArray, QByteArray()));

    bot_->getApi().sendMessage(message->chat->id, "🔄 Команда на перезагрузку отправлена!");
}

void Bot::sub_1_list(const Scheme_Item& scheme, TgBot::Message::Ptr message)
{
    const unordered_map<uint32_t, string> sub_1_names = get_sub_1_names_for_scheme(scheme);
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());
    for (const auto &sub_1_it: sub_1_names)
    {
        const string data = base_data + ".menu_sub_1." + to_string(sub_1_it.first);
        keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow(data, sub_1_it.second));
    }

    keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow(base_data, "Назад (<<)"));
    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
}

void Bot::menu_sub_list(const Scheme_Item& scheme, TgBot::Message::Ptr message, const string& action)
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

void Bot::menu_sub_1(const Scheme_Item& scheme, TgBot::Message::Ptr message, const string &sub_id)
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

void Bot::sub_1(const Scheme_Item& scheme, TgBot::Message::Ptr message, uint32_t sub_id, uint32_t sub_1_id)
{
    const string text = "(Ещё не реализованно) Sub 1: " + to_string(sub_1_id)
            + " на голове " + to_string(sub_id) + " в " + to_string(scheme.id());
    bot_->getApi().sendMessage(message->chat->id, text);
}

void Bot::menu_sub_2(const Scheme_Item& scheme, TgBot::Message::Ptr message, const string& sub_id)
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());
    const string str = base_data + ".sub." + sub_id + ".sub_2.";
    vector<TgBot::InlineKeyboardButton::Ptr > replaceRow
    {
        Bot::makeInlineButton(str + "action_1", "Действие 1"),
                Bot::makeInlineButton(str + "action_2", "Действие 2"),
                Bot::makeInlineButton(str + "action_3", "Действие 3")
    };

    keyboard->inlineKeyboard.push_back(replaceRow);
    keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow(base_data, "Назад (<<)"));

    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
}

void Bot::sub_2(const Scheme_Item& scheme, TgBot::Message::Ptr message, const std::string &sub_2_type)
{
    const string text = "(Ещё не реализованно) Действие Sub 2 тип: " + sub_2_type + " в " + to_string(scheme.id());
    bot_->getApi().sendMessage(message->chat->id, text);
}

// Helpers

map<uint32_t, string> Bot::list_schemes_names(uint32_t user_id, uint32_t page_number, const string &search_text) const
{
    QString search_cond;
    const QString sql = "SELECT s.id, s.title FROM das_scheme s "
            "LEFT JOIN das_scheme_groups sg ON sg.scheme_id = s.id "
            "LEFT JOIN das_scheme_group_user sgu ON sgu.team_id = sg.scheme_group_id "
            "WHERE sgu.user_id = %1%4 GROUP BY s.id LIMIT %2, %3";

    if (!search_text.empty())
        search_cond = " AND s.title LIKE '%" + QString::fromStdString(search_text) + "%'";

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql.arg(user_id).arg(page_number).arg(schemes_per_page_).arg(search_cond));

    map<uint32_t, string> res;

    while (q.next())
        res.emplace(q.value(0).toUInt(), q.value(1).toString().toStdString());
    return res;
}

string Bot::getReportFilepathForUser(TgBot::User::Ptr user) const
{
    // TODO: generate filepath
    return "/opt/book1.xlsx";
}

unordered_map<uint32_t, string> Bot::get_sub_base_for_scheme(const Scheme_Item& scheme) const
{
    return unordered_map<uint32_t, string> { {2, "Base 2"}, {1, "Base 1"} };
}

unordered_map<uint32_t, string> Bot::get_sub_1_names_for_scheme(const Scheme_Item& scheme) const
{
    return unordered_map<uint32_t, string> { {2, "Item 2"}, {1, "Item 1"} };
}

uint32_t Bot::get_authorized_user_id(uint32_t user_id, int64_t chat_id, bool skip_message) const
{
    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.select({Tg_User::table_name(), {}, {"user_id"}}, "WHERE id=" + QString::number(user_id));
    if (q.isActive() && q.next())
        return q.value(0).toUInt();

    if (!skip_message)
        send_message(chat_id, "Для этого действия вам необходимо авторизоваться в личном чате с ботом");
    return 0;
}

void Bot::send_schemes_list(uint32_t user_id, TgBot::Chat::Ptr chat, uint32_t current_page,
                              TgBot::Message::Ptr msg_to_update, const string &search_text) const
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    map<uint32_t, string> schemes_map = list_schemes_names(user_id, current_page, search_text);

    for (const auto &scheme: schemes_map)
    {
        TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
        button->text = scheme.second;
        button->callbackData = string("scheme.") + to_string(scheme.first);

        keyboard->inlineKeyboard.push_back({button});
    }

    vector<TgBot::InlineKeyboardButton::Ptr> row;
    if (current_page > 0)
    {
        TgBot::InlineKeyboardButton::Ptr prev_page_button(new TgBot::InlineKeyboardButton);
        prev_page_button->text = "<<<";	// TODO: here you can change text of "previous page" button
        prev_page_button->callbackData = string("page.prev.") + to_string(current_page) + '.' + search_text;
        row.push_back(prev_page_button);
    }

    if (schemes_map.size() >= schemes_per_page_)
    {
        TgBot::InlineKeyboardButton::Ptr next_page_button(new TgBot::InlineKeyboardButton);
        next_page_button->text = ">>>";	// TODO: here you can change text of "next page" button
        next_page_button->callbackData = string("page.next.") + to_string(current_page) + '.' + search_text;
        row.push_back(next_page_button);
    }

    if (!row.empty())
        keyboard->inlineKeyboard.push_back(row);

    const string text = "Список аппаратов:";
    if (msg_to_update)
    {
//        bot_->getApi().editMessageReplyMarkup(chat->id, msg_to_update->messageId, "", keyboard);
        bot_->getApi().editMessageText(text, chat->id, msg_to_update->messageId, "", "", false, keyboard);
    }
    else
    {
        bot_->getApi().sendMessage(chat->id, text, false, 0, keyboard, "Markdown", false);
    }
}

void Bot::sendSchemeMenu(TgBot::Message::Ptr message, const Scheme_Item& scheme) const
{
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    const string base_data = "scheme." + to_string(scheme.id());
    keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow(base_data + ".status", "Состояние"));
    keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow(base_data + ".menu_sub_1", "Под меню 1")),
    keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow(base_data + ".menu_sub_2", "Под меню 2"));
    keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow(base_data + ".restart", "Перезагрузка"));
    keyboard->inlineKeyboard.push_back(Bot::makeInlineButtonRow("list", "Назад (<<)"));

//    bot_->getApi().editMessageReplyMarkup(message->chat->id, message->messageId, "", keyboard);
    bot_->getApi().editMessageText(scheme.title_.toStdString(), message->chat->id, message->messageId, "", "", false, keyboard);
}

void Bot::send_authorization_message(const TgBot::Message& msg) const
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
                     QString::fromStdString(user->languageCode));

    const QString suffix = "ON DUPLICATE KEY UPDATE first_name=VALUES(first_name), last_name=VALUES(last_name),"
                           "user_name=VALUES(user_name), lang=VALUES(lang)";
    if (!db.insert(db_table<Tg_User>(), Tg_User::to_variantlist(tg_user), nullptr, suffix))
    {
        send_message(chat_id, "Ошибка во время добавления пользователя");
        return;
    }

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    ds << Tg_User::to_variantlist(tg_user) << now << "SomePassword";
    data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

    const Tg_Auth auth(user->id, now + (3 * 60 * 1000), QString::fromLatin1(data));

    if (db.insert(db_table<Tg_Auth>(), Tg_Auth::to_variantlist(auth), nullptr,
              "ON DUPLICATE KEY UPDATE expired = VALUES(expired), token = VALUES(token)"))
    {
        std::string text = "Чтобы продолжить, пожалуйста перейдите по ссылке ниже и авторизуйтесь."
                           "\n\nhttps://deviceaccess.ru/tg_auth/";
        text += auth.token().toStdString();
//        send_message(chat_id, text);

        bot_->getApi().sendMessage(chat_id, text);
    }
    else
        send_message(chat_id, "Ошибка во время инициализации привязки пользователя");
}

TgBot::InlineKeyboardButton::Ptr Bot::makeInlineButton(const string &data, const string &text)
{
    TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
    button->text = text;
    button->callbackData = data;
    return button;
}

std::vector<TgBot::InlineKeyboardButton::Ptr > Bot::makeInlineButtonRow(const string &data, const string &text)
{
    return vector<TgBot::InlineKeyboardButton::Ptr>
    {
        Bot::makeInlineButton(data, text)
    };
}

void Bot::finished()
{
    qDebug() << "finished";
    bot_->getApi().deleteWebhook();
}

Scheme_Item Bot::get_scheme(uint32_t user_id, const string &scheme_id) const
{
    Scheme_Item scheme{std::atoi(scheme_id.c_str())};
    fill_scheme(user_id, scheme);
    return scheme;
}

void Bot::fill_scheme(uint32_t user_id, Scheme_Item &scheme) const
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
        scheme.set_parent_id(q.value(0).toUInt());
        scheme.title_ = q.value(1).toString();
    }
    else
        scheme.set_id(0);
}

}	// namespace Das
