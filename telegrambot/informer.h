#ifndef DAS_SERVER_INFORMER_H
#define DAS_SERVER_INFORMER_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/signals2/signal.hpp>

#include <Das/log/log_pack.h>
#include <plus/das/scheme_info.h>
#include <plus/das/status_helper.h>

namespace Helpz {
namespace DB {
class Base;
}
}

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(Inf_Detail_log)

class Informer : public Status_Helper
{
public:
    Informer(bool skip_connected_event, int event_timeout_secs);
    ~Informer();

    boost::signals2::signal<void (int64_t, const std::string&)> send_message_signal_;

    void connected(const Scheme_Info& scheme);
    void disconnected(const Scheme_Info& scheme, bool just_now);
    void change_status(const Scheme_Info& scheme, const QVector<DIG_Status> &pack);

    void send_event_messages(const Scheme_Info &scheme, const QVector<Log_Event_Item> &event_pack);
private:
    struct Data
    {
        Data(const Scheme_Info& scheme,
             std::chrono::time_point<std::chrono::system_clock> expired_time,
             const QVector<DIG_Status>& add_vect,
             const QVector<DIG_Status>& del_vect = {});
        std::chrono::time_point<std::chrono::system_clock> expired_time_;
        Scheme_Info scheme_;
        QVector<DIG_Status> add_vect_, del_vect_;
    };

    struct Prepared_Data
    {
        QString title_, text_;
        std::set<int64_t> chat_set_;
    };

    void add_data(std::shared_ptr<Data>&& data_ptr, bool remove_only = false);

    void run();
    std::shared_ptr<Data> pop_data();
    void process_data(Data* data);
    QString get_scheme_title(uint32_t scheme_id);
    QString get_status_text(Data* data) const;
    void send_message(const std::map<uint32_t, Prepared_Data>& prepared_data_map);

    bool break_flag_, skip_connected_event_;
    std::thread* thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    std::queue<std::shared_ptr<Data>> data_queue_;
    std::map<uint32_t, std::vector<std::shared_ptr<Data>>> schemedata_map_;
    std::map<uint32_t, Prepared_Data> prepared_data_map_;

    std::chrono::seconds event_timeout_;
    /*
     * В очереди лежат данные в порядке добавления и поток ожидает до время истечения отправки данных
     * в schemedata_map_ лежат данные каждого проекта
     * Перед добавлением данных ищем такие же данные в противоположном массиве,
     * если находим, то удаляем, из обоих мест, оставшиеся данные добавляем в очередь и карту, как новый Data с новым временем 
     */
};

} // namespace Das

#endif // DAS_SERVER_INFORMER_H
