#include <sstream>

#include <QSqlError>

#include <Helpz/db_builder.h>

#include "db/tg_subscriber.h"
#include "informer.h"

namespace Das {

Q_LOGGING_CATEGORY(Inf_Detail_log, "informer.detail", QtInfoMsg)

using namespace Helpz::DB;

Informer::Data::Data(const Scheme_Info &scheme, std::chrono::time_point<std::chrono::system_clock> expired_time,
                     const QVector<DIG_Status> &add_vect, const QVector<DIG_Status> &del_vect) :
    expired_time_(expired_time),
    scheme_(scheme), add_vect_(add_vect), del_vect_(del_vect)
{
}

// ---------------------------------------------------------------------------

Informer::Informer(bool skip_connected_event, int event_timeout_secs) :
    break_flag_(false),
    skip_connected_event_(skip_connected_event),
    event_timeout_(event_timeout_secs)
{
    thread_ = new std::thread(&Informer::run, this);
}

Informer::~Informer()
{
    break_flag_ = true;
    cond_.notify_one();
    if (thread_->joinable())
    {
        thread_->join();
    }
    delete thread_;
}

void Informer::connected(const Scheme_Info& scheme)
{
    const QVector<DIG_Status> data{DIG_Status{}};
    auto expired_time = std::chrono::system_clock::now() + event_timeout_;
    add_data(std::make_shared<Data>(scheme, expired_time, data), skip_connected_event_);
}

void Informer::disconnected(const Scheme_Info& scheme, bool just_now)
{
    QVector<DIG_Status> data{DIG_Status{}};
    if (just_now)
        data.first().set_args(QStringList{QString()});

    auto expired_time = std::chrono::system_clock::now() + event_timeout_;
    add_data(std::make_shared<Data>(scheme, expired_time, QVector<DIG_Status>{}, data));
}

void Informer::change_status(const Scheme_Info &scheme, const QVector<DIG_Status> &pack)
{
    using TIME_T = std::chrono::system_clock::time_point;
    struct Status_Data {
        QVector<DIG_Status> add_vect_, del_vect_;
    };

    std::map<TIME_T, Status_Data> status_map;

    for (const DIG_Status& status: pack)
    {
        TIME_T expired_time;

        if (status.timestamp_msecs() == 0)
        {
            expired_time = std::chrono::system_clock::now();
        }
        else
        {
            typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> __from;
            expired_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>
                   (__from(std::chrono::milliseconds(status.timestamp_msecs())));
        }

        expired_time += event_timeout_;

        Status_Data& data = status_map[expired_time];

        if (status.is_removed())
            data.del_vect_.push_back(status);
        else
            data.add_vect_.push_back(status);

        qCDebug(Inf_Detail_log) << "change_status scheme:" << scheme.id() << "group:" << status.group_id() << "status:" << status.status_id() << (status.is_removed() ? "REMOVED" : "");
    }

    for (const auto& it: status_map)
        add_data(std::make_shared<Data>(scheme, it.first, it.second.add_vect_, it.second.del_vect_));
}

void Informer::send_event_messages(const Scheme_Info &scheme, const QVector<Log_Event_Item> &event_pack)
{
    QString text;

    for (const Log_Event_Item& item: event_pack)
    {
        if (!item.need_to_inform())
            continue;

        text += '\n';
        switch (item.type_id())
        {
        case Log_Event_Item::ET_DEBUG: text += "⚪️";  break;
        case Log_Event_Item::ET_WARNING: text += "🔶";  break;
        case Log_Event_Item::ET_CRITICAL: text += "🔴";  break;
        case Log_Event_Item::ET_FATAL: text += "🛑";  break;
        case Log_Event_Item::ET_INFO: text += "🔵";  break;

        default: text += "❕"; break;
        }

        text += ' ';
        text += item.category();
        text += ": ";
        text += item.text();
    }

    if (text.isEmpty())
        return;

    const QString scheme_title = get_scheme_title(scheme.id());
    if (scheme_title.isEmpty())
    {
        qCritical() << "Can't get scheme name for id:" << scheme.id() << "events lost:" << qPrintable(text);
        return;
    }

    Base& db = Base::get_thread_local_instance();
    QVector<Tg_Subscriber> subscribers = db_build_list<Tg_Subscriber>(db, scheme.scheme_groups(), {},
                                                                      Tg_Subscriber::COL_group_id);
    if (subscribers.empty())
        return;

    const std::string data = '*' + scheme_title.toStdString() + "*\n" + text.toStdString();

    for (Tg_Subscriber subscriber: subscribers)
    {
        if (subscriber.chat_id())
            send_message_signal_(subscriber.chat_id(), data);
    }
}

void erase_two_vectors(QVector<DIG_Status>& origin_v, QVector<DIG_Status>& diff_v, QVector<DIG_Status>& same_v)
{
    bool finded;
    for (auto origin_it = origin_v.begin(); origin_it != origin_v.end(); )
    {
        finded = false;

        for (auto diff_it = diff_v.begin(); diff_it != diff_v.end(); ++diff_it)
        {
            if (origin_it->group_id() == diff_it->group_id() &&
                origin_it->status_id() == diff_it->status_id())
            {
                diff_v.erase(diff_it);
                finded = true;
                break;
            }
        }

        if (finded)
        {
            origin_it = origin_v.erase(origin_it);
        }
        else
        {
            finded = false;
            for (const DIG_Status& item: same_v)
            {
                if (origin_it->group_id() == item.group_id() &&
                    origin_it->status_id() == item.status_id())
                {
                    finded = true;
                    break;
                }
            }

            if (finded)
                origin_it = origin_v.erase(origin_it);
            else
                ++origin_it;
        }
    }
}

void Informer::add_data(std::shared_ptr<Data>&& data_ptr, bool remove_only)
{
    std::lock_guard lock(mutex_);

    auto it = schemedata_map_.find(data_ptr->scheme_.id());
    if (it != schemedata_map_.end())
    {
        std::vector<std::shared_ptr<Data>>& schemedata = it->second;
        for (auto d_it = schemedata.begin(); d_it != schemedata.end();)
        {
            std::shared_ptr<Data>& data = *d_it;
            erase_two_vectors(data->add_vect_, data_ptr->del_vect_, data_ptr->add_vect_);
            erase_two_vectors(data->del_vect_, data_ptr->add_vect_, data_ptr->del_vect_);
            if (data->add_vect_.empty() && data->del_vect_.empty())
            {
                d_it = schemedata.erase(d_it);
            }
            else
                ++d_it;
        }
    }

    if (!remove_only && (!data_ptr->add_vect_.empty() || !data_ptr->del_vect_.empty()))
    {
        if (it != schemedata_map_.end())
            it->second.push_back(data_ptr);
        else
            schemedata_map_.emplace(data_ptr->scheme_.id(), std::vector<std::shared_ptr<Data>>{data_ptr});

        data_queue_.push(std::move(data_ptr));
        cond_.notify_one();
    }
    else if (it != schemedata_map_.end() && it->second.empty())
            schemedata_map_.erase(it);
}

void Informer::run()
{
    std::unique_lock lock(mutex_, std::defer_lock);

    while (true)
    {
//        if (prepared_data_map_.empty())
//            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        lock.lock();

        if (!break_flag_ && !data_queue_.empty())
        {
            cond_.wait_until(lock, data_queue_.front()->expired_time_);
        }
        else
        {
            cond_.wait(lock, [this]() { return !prepared_data_map_.empty() || !data_queue_.empty() || break_flag_; });
        }

        if (break_flag_)
        {
            while (!data_queue_.empty())
                process_data(pop_data().get());
            if (!prepared_data_map_.empty())
                send_message(prepared_data_map_);
            break;
        }

        if (!data_queue_.empty() && data_queue_.front()->expired_time_ <= std::chrono::system_clock::now())
        {
            std::shared_ptr<Data> data_ptr = pop_data();
            lock.unlock();

            process_data(data_ptr.get());
        }
        else if (!prepared_data_map_.empty())
        {
            std::map<uint32_t, Prepared_Data> data_map(std::move(prepared_data_map_));
            lock.unlock();

            send_message(data_map);
        }
        else
            lock.unlock();
    }
}

std::shared_ptr<Informer::Data> Informer::pop_data()
{
    std::shared_ptr<Data> data_ptr{std::move(data_queue_.front())};
    data_queue_.pop();

    auto it = schemedata_map_.find(data_ptr->scheme_.id());
    if (it != schemedata_map_.end())
    {
        std::vector<std::shared_ptr<Data>>& schemedata = it->second;
        schemedata.erase(std::remove(schemedata.begin(), schemedata.end(), data_ptr), schemedata.end());
        if (schemedata.empty())
            schemedata_map_.erase(it);
    }
    return data_ptr;
}

void Informer::process_data(Data* data)
{
    QString message = get_status_text(data);
    if (message.isEmpty())
        return;

    std::lock_guard lock(mutex_); // for prepared_data_map_
    auto it = prepared_data_map_.find(data->scheme_.id());
    if (it == prepared_data_map_.end())
    {
        Base& db = Base::get_thread_local_instance();
        QVector<Tg_Subscriber> subscribers = db_build_list<Tg_Subscriber>(db, data->scheme_.scheme_groups(), {},
                                                                          Tg_Subscriber::COL_group_id);
        if (subscribers.empty())
            return;

        Prepared_Data p_data;
        for (const Tg_Subscriber& ts: subscribers)
            if (ts.chat_id())
                p_data.chat_set_.insert(ts.chat_id());
        if (p_data.chat_set_.empty())
            return;

        const QString scheme_title = get_scheme_title(data->scheme_.id());
        if (scheme_title.isEmpty())
        {
            qCritical() << "Can't get scheme name for id:" << data->scheme_.id();
            return;
        }
        p_data.title_ = '*' + scheme_title + '*';

        it = prepared_data_map_.emplace(data->scheme_.id(), std::move(p_data)).first;
    }

    Prepared_Data& p_data = it->second;
    p_data.text_ += '\n' + message;
}

QString Informer::get_scheme_title(uint32_t scheme_id)
{
    QString title;
    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.select({"das_scheme", {}, {"title", "name"}}, "WHERE id=" + QString::number(scheme_id));
    if (q.next())
    {
        title = q.value(0).toString();
        if (title.isEmpty())
            title = q.value(1).toString();
    }
    return title;
}

bool is_connected_type(const QVector<DIG_Status>& vect)
{
    if (vect.size() == 1)
    {
        const DIG_Status& item = vect.front();
        if (item.id() == 0 && item.group_id() == 0 && item.status_id() == 0)
            return true;
    }
    return false;
}

QString Informer::get_status_text(Data* data) const
{
    if (data->add_vect_.empty() && data->del_vect_.empty())
        return {};

    if (is_connected_type(data->add_vect_))
        return "🚀 На связи!";

    if (is_connected_type(data->del_vect_))
    {
        if (data->del_vect_.front().args().empty())
            return "💢 Отключен.";
        else
            return {};
    }

    QSet<uint32_t> info_id_set, group_id_set;
    for (const DIG_Status& item: data->add_vect_)
    {
        info_id_set.insert(item.status_id());
        group_id_set.insert(item.group_id());
    }
    for (const DIG_Status& item: data->del_vect_)
    {
        info_id_set.insert(item.status_id());
        group_id_set.insert(item.group_id());
    }

    uint32_t scheme_id = data->scheme_.parent_id_or_id();

    Base& db = Base::get_thread_local_instance();
    std::vector<Informer::Section> group_names = get_group_names(group_id_set, db, scheme_id);

    QString suffix = db_get_items_list_suffix<DIG_Status_Type>(info_id_set);
    suffix += " AND scheme_id = ";
    suffix += QString::number(scheme_id);

    const QVector<DIG_Status_Type> info_vect = db_build_list<DIG_Status_Type>(db, suffix);

    for (const DIG_Status& item: data->add_vect_)
        fill_dig_status_text(group_names, info_vect, item);
    for (const DIG_Status& item: data->del_vect_)
        fill_dig_status_text(group_names, info_vect, item, true);

    QStringList text_list;
    for (const Informer::Section& sct: group_names)
        for (const Informer::Section::Group& group: sct.group_vect_)
            for (const std::pair<QString, QString>& p: group.status_text_vect_)
                text_list.push_back(p.first + ' ' + sct.name_ + " - " + group.name_ + ": " + p.second);
    if (!text_list.isEmpty())
        return text_list.join("\n");
    return {};
}

void Informer::send_message(const std::map<uint32_t, Prepared_Data>& prepared_data_map)
{
    std::map<int64_t, QString> message_map;
    for (const auto& it: prepared_data_map)
    {
        const Prepared_Data& data = it.second;
        for (int64_t chat_id: it.second.chat_set_)
        {
            if (!data.text_.isEmpty())
                message_map[chat_id] += data.title_ + data.text_ + "\n\n";
        }
    }

    for (const auto& it: message_map)
    {
        qCDebug(Inf_Detail_log) << "send_message chat:" << it.first << "text:" << it.second;
        send_message_signal_(it.first, it.second.toStdString());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
//    QMetaObject::invokeMethod(django_, "sendCriticalMessage", Qt::QueuedConnection, Q_ARG(quint32, schemeid), Q_ARG(QString, message));
}

} // namespace Das
