#ifndef DAS_BOT_H
#define DAS_BOT_H

#include <maxbot/types/Recipient.h>
#include <string>
#include <vector>
#include <thread>

#define HAVE_CURL
#include <maxbot/net/CurlHttpClient.h>
#include <maxbot/net/BotWebhookTcpServer.h>
//#include <maxbot/bot.h>

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

struct Config
{
    uint16_t _port = 8443;
    std::string _token;
    std::string _webhook_url, _webhookSecret;
    std::string _auth_base_url = "https://deviceaccess.ru/max_auth/";
    std::string _templates_path;
    std::string _help_file_path;
};

class Controller : public QObject, public Bot_Base
{
    Q_OBJECT
public:
    Controller(DBus::Interface* dbus_iface, Config config);
    ~Controller();

	void start();
	void quit();
    void stop();
    void send_message(int64_t chatId, const std::string& text) const;

protected:
    void init();
    void run();
    void anyMessage(MaxBot::Message::Ptr message);

	void processDirectoryPage(uint32_t user_id, const std::vector<std::string>& cmd, MaxBot::UpdateMessageCallback::Ptr cb);
	void processDirectoryScheme(uint32_t user_id, const std::vector<std::string>& cmd, MaxBot::UpdateMessageCallback::Ptr cb);
	void processDirectorySubscriber(uint32_t user_id, const std::vector<std::string>& cmd, MaxBot::UpdateMessageCallback::Ptr cb);
    void process_directory(uint32_t user_id, MaxBot::UpdateMessageCallback::Ptr cb);
    // Helpers
    std::map<uint32_t, std::string> list_schemes_names(uint32_t user_id, uint32_t page_number, const std::string &search_text, size_t& result_count) const;
    std::string getReportFilepathForUser(MaxBot::User::Ptr user) const;
    std::unordered_map<uint32_t, std::string> get_sub_base_for_scheme(const Scheme_Item& scheme) const;
    std::unordered_map<uint32_t, std::string> get_sub_1_names_for_scheme(const Scheme_Item& scheme) const;

    uint32_t get_authorized_user_id(uint32_t user_id, const MaxBot::Message::Ptr& msg) const;

    // Bot helpers
    void send_schemes_list(uint32_t user_id, const MaxBot::Message::Ptr& msg, uint32_t current_page = 0,
                           const std::string& callbackId = {}, const std::string& search_text = {}) const;
    void sendSchemeMenu(MaxBot::UpdateMessageCallback::Ptr cb, const Scheme_Item& scheme, uint32_t user_id) const;
    void send_authorization_message(const MaxBot::Message::Ptr& msg) const;

    // Chat methods
    void find(uint32_t user_id, MaxBot::Message::Ptr message);
    void find(uint32_t user_id, const MaxBot::Message::Ptr& msg, std::string text);
    void list(uint32_t user_id, MaxBot::Message::Ptr message) const;
    void report(MaxBot::Message::Ptr message) const;
    void inform_onoff(uint32_t user_id, MaxBot::Message::Ptr msg, const std::string& callbackId = {});
    void help(MaxBot::Message::Ptr message);
    void help_send_file(int64_t chat_id) const;

    // Inline button query methods
    void status(const Scheme_Item& scheme, MaxBot::Message::Ptr message);
    void elements(uint32_t user_id, const Scheme_Item& scheme, MaxBot::Message::Ptr message, std::vector<std::string>::const_iterator begin, const std::vector<std::string>& cmd, const std::string& msg_data);
    void restart(uint32_t user_id, const Scheme_Item& scheme, MaxBot::Message::Ptr message);
    void menu_sub_list(const Scheme_Item& scheme, MaxBot::UpdateMessageCallback::Ptr cb, const std::string& action);
    void menu_sub_1(const Scheme_Item& scheme, MaxBot::UpdateMessageCallback::Ptr cb, const std::string& sub_id);
    void menu_sub_2(const Scheme_Item& scheme, MaxBot::UpdateMessageCallback::Ptr cb, const std::string &sub_id);

    void sub_2(const Scheme_Item& scheme, MaxBot::Message::Ptr message, const std::string& sub_2_type);
    void sub_1(const Scheme_Item& scheme, MaxBot::Message::Ptr message, uint32_t sub_id, uint32_t sub_1_id);
public slots:
    void finished();
    void send_user_authorized(qint64 external_user_id);
private:
    void fill_templates();

    Scheme_Item get_scheme(uint32_t user_id, const std::string& scheme_id) const;
    void fill_scheme(uint32_t user_id, Scheme_Item& scheme) const;

    bool stop_flag_;
	std::shared_ptr<MaxBot::CurlHttpClient> _botClient;
	std::shared_ptr<MaxBot::Bot> bot_;
    MaxBot::User::Ptr bot_user_;

    MaxBot::BotWebhookTcpServer* server_;
    Config _conf;

    const uint32_t schemes_per_page_ = 5;

    struct Waited_Item {
        int32_t external_user_id_;
        std::chrono::system_clock::time_point expired_time_;
        std::string data_;
        Scheme_Item scheme_;
    };

    std::map<int64_t, Waited_Item> waited_map_;

    std::set<User_Menu::Item> user_menu_set_;

	std::thread _th;
};

} // namespace Bot
} // namespace Das

#endif // DAS_BOT_H
