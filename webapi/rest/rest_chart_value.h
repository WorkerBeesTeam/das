#ifndef DAS_REST_CHART_VALUE_H
#define DAS_REST_CHART_VALUE_H

#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

#include <QSqlQuery>

#include <served/served.hpp>

namespace Das {
namespace Rest {

class Chart_Value
{
public:
    Chart_Value(served::multiplexer& mux, const std::string& scheme_path);
private:
    void list(served::response& res, const served::request& req);

    struct Time_Range {
        int64_t _from;
        int64_t _to;
    };

    QString get_base_sql(const QString &what = QString()) const;
    QString get_where(const served::request &req, const Time_Range& time_range, const QString &scheme_where) const;
    Time_Range get_time_range(const std::string& from_str, const std::string& to_str) const;
    virtual QString get_time_field_name() const;
    QString get_time_range_where(const Time_Range& time_range) const;
    virtual QString get_scheme_field_name() const;
    QString get_scheme_where(const served::request &req) const;
    virtual QString get_data_field_name() const;
    QString get_data_in_string(const std::string& param) const;
    QString get_limits(const std::string& offset_str, const std::string& limit_str, uint32_t &offset, uint32_t &limit) const;
    QString get_limit_suffix(uint32_t offset, uint32_t limit) const;
    int64_t get_count_all(int64_t count, uint32_t offset, uint32_t limit, const QString &where);
    int64_t fill_datamap(std::map<uint32_t, std::map<qint64, picojson::object>>& data_map, const QString &sql) const;
    picojson::object get_data_item(const QSqlQuery& query, int64_t timestamp) const;
    void fill_object(picojson::object& obj, const QSqlQuery& q) const;
    void fill_results(picojson::array& results, const std::map<uint32_t, std::map<qint64, picojson::object>>& data_map,
                      const Time_Range &time_range, const QString &scheme_where) const;
    picojson::object get_one_point(int64_t timestamp, const QString &scheme_where, uint32_t item_id, bool is_before_range_point) const;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CHART_VALUE_H
