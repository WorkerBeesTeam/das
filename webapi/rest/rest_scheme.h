#ifndef DAS_REST_SCHEME_H
#define DAS_REST_SCHEME_H

#include <served/served.hpp>

#include <plus/das/scheme_info.h>

namespace Das {

namespace DBus {
class Interface;
} // namespace Dbus

namespace Rest {

class Chart;
class Chart_Data_Controller;
class Scheme_Structure;
class Log;
class Mnemoscheme;

class Scheme
{
public:
    Scheme(served::multiplexer &mux, DBus::Interface *dbus_iface);

    static Scheme_Info get_info(const served::request& req);
    static Scheme_Info get_info(uint32_t scheme_id);
private:
    void get_dig_status(served::response& res, const served::request& req);
    void get_dig_status_type(served::response& res, const served::request& req);
    void get_device_item_value(served::response& res, const served::request& req);
    void get_disabled_status(served::response& res, const served::request& req);
    void del_disabled_status(served::response& res, const served::request& req);
    void add_disabled_status(served::response& res, const served::request& req);
    void get(served::response& res, const served::request& req);
    void get_list(served::response& res, const served::request& req);
    void create(served::response& res, const served::request& req);

    void set_name(served::response& res, const served::request& req);
    void copy(served::response& res, const served::request& req);

    DBus::Interface *dbus_iface_;

    std::shared_ptr<Scheme_Structure> structure_;
    std::shared_ptr<Chart> chart_;
    std::shared_ptr<Chart_Data_Controller> chart_data_;
    std::shared_ptr<Log> _log;
    std::shared_ptr<Mnemoscheme> _mnemoscheme;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_SCHEME_H
