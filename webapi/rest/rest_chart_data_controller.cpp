
#include "rest_chart_param.h"
#include "rest_chart_data_controller.h"

namespace Das {
namespace Rest {

Chart_Data_Controller::Chart_Data_Controller(served::multiplexer &mux, const std::string &scheme_path)
{
    const std::string url = scheme_path + "/chart_";
    mux.handle(url + "value/").get([this](served::response& res, const served::request& req) { get_chart_value(res, req); });
    mux.handle(url + "param/").get([this](served::response& res, const served::request& req) { get_chart_param(res, req); });
}

void Chart_Data_Controller::get_chart_value(served::response &res, const served::request &req)
{
    Chart_Value chart_value;
    res << chart_value(req);
}

void Chart_Data_Controller::get_chart_param(served::response &res, const served::request &req)
{
    Chart_Param chart_param;
    res << chart_param(req);
}

} // namespace Rest
} // namespace Das
