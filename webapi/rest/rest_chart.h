#ifndef DAS_REST_CHART_H
#define DAS_REST_CHART_H

#include <served/served.hpp>

namespace Das {
namespace Rest {

class Chart
{
public:
    Chart(served::multiplexer& mux, const std::string& scheme_path);
private:
    void remove(served::response& res, const served::request& req);
    void upsert(served::response& res, const served::request& req);
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CHART_H
