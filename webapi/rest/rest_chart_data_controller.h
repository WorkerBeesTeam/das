#ifndef DAS_REST_CHART_DATA_CONTROLLER_H
#define DAS_REST_CHART_DATA_CONTROLLER_H

#include <served/served.hpp>

namespace Das {
namespace Rest {

class Chart_Data_Controller
{
public:
    Chart_Data_Controller(served::multiplexer& mux, const std::string& scheme_path);

private:
    void get_chart_value(served::response& res, const served::request& req);
    void get_chart_param(served::response& res, const served::request& req);
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CHART_DATA_CONTROLLER_H
