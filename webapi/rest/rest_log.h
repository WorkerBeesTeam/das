#ifndef DAS_REST_LOG_H
#define DAS_REST_LOG_H

#include <served/served.hpp>

#include <plus/das/scheme_info.h>

namespace Das {
namespace Rest {

struct Log_Time_Range {
    int64_t _from;
    int64_t _to;
};

struct Log_Query_Param
{
    uint32_t _limit;
    Scheme_Info _scheme;
    Log_Time_Range _time_range;

    bool _case_sensitive;
    std::string _filter;

    std::string _dig_id, _dig_param_id, _item_id;
};

class Log
{
public:
    Log(served::multiplexer& mux, const std::string& scheme_path);

    static Log_Time_Range get_time_range(const std::string &from_str, const std::string &to_str);
    static Log_Time_Range parse_time_range(const served::request &req);

    static std::vector<std::string> parse_data_in(const std::string &param);

private:
    template<typename T>
    void add_getter_handler(served::multiplexer& mux, const std::string& base_path, const std::string& name);

    template<typename T>
    void log_getter(served::response &res, const served::request &req, const std::string& name);

    Log_Query_Param parse_params(const served::request& req);
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_LOG_H
