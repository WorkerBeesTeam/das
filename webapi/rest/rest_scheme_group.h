#ifndef DAS_REST_SCHEME_GROUP_H
#define DAS_REST_SCHEME_GROUP_H

#include <served/served.hpp>

namespace Das {
namespace Rest {

class Scheme_Group
{
public:
    Scheme_Group(served::multiplexer &mux);
private:
    void get_list(served::response& res, const served::request& req);

};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_SCHEME_GROUP_H
