#ifndef DAS_BOT_H
#define DAS_BOT_H

#include <string>
#include <vector>

#include <QThread>

#include <tgbot/net/TgWebhookTcpServer.h>
//#include <tgbot/tgbot.h>

#define EXCEL_FILE_TYPE_XLSX

#ifdef EXCEL_FILE_TYPE_XLSX
    #define REPORT_MIME "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
#else
    #define REPORT_MIME "application/vnd.ms-excel"
#endif

#include "bot_base.h"
#include "scheme_item.h"

#include "user_menu/item.h"

namespace Das {
namespace Bot {

class Controller : public QThread, public Bot_Base
{
    Q_OBJECT
public:
    Controller(DBus::Interface* dbus_iface, const std::string& token,
        const std::string& webhook_url, uint16_t port = 8443, const std::string& webhook_cert = {},
        const std::string& auth_base_url = "https://deviceaccess.ru/tg_auth/", const std::string& templates_path = {});
    ~Controller();

    void stop();
    void send_message(int64_t chat_id, const std::string& text) const;

protected:
    void init();
    void run() override;
    void anyMessage(TgBot::Message::Ptr message);

    std::string process_directory(uint32_t user_id, TgBot::Message::Ptr message, const std::string& msg_data, int32_t tg_user_id);
    // Helpers
    std::map<uint32_t, std::string> list_schemes_names(uint32_t user_id, uint32_t page_number, const std::string &search_text) const;
    std::string getReportFilepathForUser(TgBot::User::Ptr user) const;
    std::unordered_map<uint32_t, std::string> get_sub_base_for_scheme(const Scheme_Item& scheme) const;
    std::unordered_map<uint32_t, std::string> get_sub_1_names_for_scheme(const Scheme_Item& scheme) const;

    uint32_t get_authorized_user_id(uint32_t user_id, int64_t chat_id, bool skip_message = false) const;

    // Bot helpers
    void send_schemes_list(uint32_t user_id, TgBot::Chat::Ptr chat, uint32_t current_page = 0,
                             TgBot::Message::Ptr msg_to_update = nullptr, const std::string& search_text = {}) const;
    void sendSchemeMenu(TgBot::Message::Ptr message, const Scheme_Item& scheme) const;
    void send_authorization_message(const TgBot::Message &msg) const;

    // Chat methods
    void find(uint32_t user_id, TgBot::Message::Ptr message) const;
    void list(uint32_t user_id, TgBot::Message::Ptr message) const;
    void report(TgBot::Message::Ptr message) const;
    void inform_onoff(uint32_t user_id, TgBot::Chat::Ptr chat, TgBot::Message::Ptr msg_to_update = nullptr);
    void help(TgBot::Message::Ptr) const;

    // Inline button query methods
    void status(const Scheme_Item& scheme, TgBot::Message::Ptr message);
    void elements(uint32_t user_id, const Scheme_Item& scheme, TgBot::Message::Ptr message, std::vector<std::string>::const_iterator begin, const std::vector<std::string>& cmd, const std::string& msg_data);
    void restart(uint32_t user_id, const Scheme_Item& scheme, TgBot::Message::Ptr message);
    void sub_1_list(const Scheme_Item& scheme, TgBot::Message::Ptr message);
    void menu_sub_list(const Scheme_Item& scheme, TgBot::Message::Ptr message, const std::string& action);
    void menu_sub_1(const Scheme_Item& scheme, TgBot::Message::Ptr message, const std::string& sub_id);
    void menu_sub_2(const Scheme_Item& scheme, TgBot::Message::Ptr message, const std::string &sub_id);

    void sub_2(const Scheme_Item& scheme, TgBot::Message::Ptr message, const std::string& sub_2_type);
    void sub_1(const Scheme_Item& scheme, TgBot::Message::Ptr message, uint32_t sub_id, uint32_t sub_1_id);
public slots:
    void finished();
private:
    void fill_templates(const std::string& templates_path);

    Scheme_Item get_scheme(uint32_t user_id, const std::string& scheme_id) const;
    void fill_scheme(uint32_t user_id, Scheme_Item& scheme) const;

    bool stop_flag_;
    uint16_t port_;
    TgBot::Bot* bot_;
    TgBot::User::Ptr bot_user_;

    TgBot::TgWebhookTcpServer* server_;
    std::string token_, webhook_url_, webhook_cert_, auth_base_url_;

    const uint32_t schemes_per_page_ = 5;

    struct Waited_Item {
        int32_t tg_user_id_;
        int64_t time_;
        std::string data_;
        Scheme_Item scheme_;
    };

    std::map<int64_t, Waited_Item> waited_map_;

    std::set<User_Menu::Item> user_menu_set_;
};

} // namespace Bot
} // namespace Das

#endif // DAS_BOT_H
