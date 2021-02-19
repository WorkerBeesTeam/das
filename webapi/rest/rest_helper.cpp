
#include <served/status.hpp>
#include <served/request_error.hpp>

#include "rest_helper.h"

namespace Das {
namespace Rest {

template<typename T>
T parse_picojson(const std::string& json_raw, const std::string& type_err_text)
{
    picojson::value val;
    const std::string err = picojson::parse(val, json_raw);
    if (!err.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, err);
    if (!val.is<T>())
        throw served::request_error(served::status_4XX::BAD_REQUEST, type_err_text);
    T& parsed_json = val.get<T>();
    if (parsed_json.empty())
        throw served::request_error(served::status_4XX::BAD_REQUEST, "Empty");
    return std::move(parsed_json);
}

picojson::object Helper::parse_object(const std::string& json_raw)
{
    return parse_picojson<picojson::object>(json_raw, "Object is expected");
}

picojson::array Helper::parse_array(const std::string &json_raw)
{
    return parse_picojson<picojson::array>(json_raw, "Array is expected");
}

} // namespace Rest
} // namespace Das
