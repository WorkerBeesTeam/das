#ifndef DAS_SERVER_LOG_SAVER_LAYERS_FILLER_H
#define DAS_SERVER_LOG_SAVER_LAYERS_FILLER_H

#include <set>
#include <mutex>

#include <Das/log/log_value_item.h>

#include <plus/das/log_saver_def.h>

namespace Das {
namespace Log_Saver {

class Layers_Filler_Time_Item
{
public:
    Layers_Filler_Time_Item();
    pair<qint64, map<uint32_t, qint64>> get(qint64 estimated_time);
    void set(qint64 ts);
    void set_scheme_time(const map<uint32_t, qint64>& scheme_time);
    void load();
    void save();

private:
    mutex _mutex;
    qint64 _time, _estimated_time;
    map<uint32_t, qint64> _scheme_time;
};

class Layers_Filler
{
public:
    static Layers_Filler_Time_Item& start_filling_time();

    static qint64 get_prev_time(qint64 ts, qint64 time_count);
    static qint64 get_next_time(qint64 ts, qint64 time_count);

    Layers_Filler();

    void operator ()();

private:
    using Data_Type = map<uint32_t/*scheme_id*/, map<uint32_t/*item_id*/, vector<DB::Log_Value_Item>>>;

    qint64 fill_layer(qint64 time_count, const QString& name, qint64 last_end_time);
    qint64 get_start_timestamp(qint64 time_count) const;
    qint64 get_final_timestamp(qint64 time_count) const;
    set<uint32_t> get_discrete(const set<uint32_t> &item_ids);
    vector<DB::Log_Value_Item> get_average_data(const Data_Type &data, const set<uint32_t> &discrete_ids);
    int process_data(Data_Type &data, const QString& name, const set<uint32_t> &discrete_ids);

    Helpz::DB::Table _log_value_table, _layer_table;

    map<qint64, QString> _layers_info;
};

} // namespace Log_Saver
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_LAYERS_FILLER_H
