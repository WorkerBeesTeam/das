#ifndef DAS_REST_HELPER_H
#define DAS_REST_HELPER_H

#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

namespace Das {
namespace Rest {

struct Helper
{
    static picojson::object parse_object(const std::string& json_raw);
    static picojson::array parse_array(const std::string& json_raw);
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_HELPER_H
