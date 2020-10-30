#ifndef DAS_REST_CHART_VALUE_H
#define DAS_REST_CHART_VALUE_H

#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

#include <QSqlQuery>

#include <served/served.hpp>

#include <Helpz/db_base.h>

namespace Das {
namespace Rest {

struct Chart_Value_Config
{
    uint32_t _close_to_now_sec = 10 * 60; // 10 minute
    uint32_t _minute_ratio_sec = 2 * 60 * 60; // 2 hour
    uint32_t _hour_ratio_sec = 2 * 24 * 60 * 60; // 2 day
    uint32_t _day_ratio_sec = 2 * 30.5 * 24 * 60 * 60; // 2 month
};

class Chart_Value
{
public:
    Chart_Value();
    virtual ~Chart_Value() = default;

    static Chart_Value_Config& config();

    std::string operator()(const served::request& req);
protected:
    enum Field_Type
    {
        FT_TIME,
        FT_ITEM_ID,
        FT_USER_ID,
        FT_VALUE,

        FT_SCHEME_ID
    };

    virtual std::string permission_name() const;
    virtual QString get_table_name() const;
    virtual QString get_field_name(Field_Type field_type) const;
    virtual QString get_additional_field_names() const;
    virtual void fill_additional_fields(picojson::object& obj, const QSqlQuery& q, const QVariant& value) const;
private:
    void parse_params(const served::request& req);

    struct Time_Range {
        int64_t _from;
        int64_t _to;
    };

    QString get_where() const;
    Time_Range get_time_range(const std::string& from_str, const std::string& to_str) const;
    QString get_time_range_where() const;
    QString get_scheme_where(const served::request &req) const;
    void parse_data_in(const std::string& param);
    QString get_data_in_where() const;
    void parse_limits(const std::string& offset_str, const std::string& limit_str);
    QString get_limit_suffix(uint32_t offset, uint32_t limit) const;
    int64_t fill_datamap();
    QString get_full_sql() const;
    QString get_base_sql(const QString &what = QString()) const;
    picojson::object get_data_item(const QSqlQuery& query, int64_t timestamp) const;
    picojson::value variant_to_json(const QVariant& value) const;
    void fill_object(picojson::object& obj, const QSqlQuery& q) const;
    void fill_results(picojson::array& results) const;
    QString get_one_point_sql(int64_t timestamp, const QString &item_id, bool is_before_range_point) const;

    bool _range_in_past;
    bool _range_close_to_now;
    uint32_t _offset, _limit;
    Time_Range _time_range;
    QString _scheme_where, _where;
    QStringList _data_in_list;

    std::map<uint32_t/*item_id*/, std::map<int64_t/*timestamp*/, picojson::object>> _data_map;
    std::map<uint32_t/*item_id*/, picojson::object> _before_range_point_map, _after_range_point_map;

    Helpz::DB::Base& _db;
};

} // namespace Rest
} // namespace Das

#endif // DAS_REST_CHART_VALUE_H
