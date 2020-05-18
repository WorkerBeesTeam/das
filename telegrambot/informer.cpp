#include <sstream>

#include <QSqlError>

#include <Helpz/db_builder.h>

#include "db/tg_subscriber.h"
#include "informer.h"

namespace Das {

Q_LOGGING_CATEGORY(Inf_Detail_log, "informer.detail", QtInfoMsg)

using namespace Helpz::DB;

Informer::Connection_State_Item::Connection_State_Item(const Scheme_Info& scheme, std::chrono::time_point<std::chrono::system_clock> expired_time,
                                                       bool is_connected, bool just_now) :
    Item{is_connected ? Item::T_CONNECTED : Item::T_DISCONNECTED, expired_time, scheme},
    just_now_(just_now)
{
}

Informer::Status::Status(const Scheme_Info &scheme, std::chrono::time_point<std::chrono::system_clock> expired_time,
                     const QVector<DIG_Status> &add_vect, const QVector<DIG_Status> &del_vect) :
    Item{Item::T_STATUS, expired_time, scheme},
    add_vect_(add_vect), del_vect_(del_vect)
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
        thread_->join();

    delete thread_;
}

void Informer::connected(const Scheme_Info& scheme)
{
    auto expired_time = std::chrono::system_clock::now() + event_timeout_;
    add_connection_state(std::make_shared<Connection_State_Item>(scheme, expired_time, /*is_connected=*/true));
}

void Informer::disconnected(const Scheme_Info& scheme, bool just_now)
{
    auto expired_time = std::chrono::system_clock::now() + event_timeout_;
    add_connection_state(std::make_shared<Connection_State_Item>(scheme, expired_time, /*is_connected=*/false, /*just_now=*/just_now));
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
        add_status(std::make_shared<Status>(scheme, it.first, it.second.add_vect_, it.second.del_vect_));
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

    const std::string scheme_title = get_scheme_title(scheme.id());
    if (scheme_title.empty())
    {
        qCritical() << "Can't get scheme name for id:" << scheme.id() << "events lost:" << qPrintable(text);
        return;
    }

    Base& db = Base::get_thread_local_instance();
    QVector<Tg_Subscriber> subscribers = db_build_list<Tg_Subscriber>(db, scheme.scheme_groups(), {},
                                                                      Tg_Subscriber::COL_group_id);
    if (subscribers.empty())
        return;

    const std::string data = '*' + scheme_title + "*\n" + text.toStdString();

    for (Tg_Subscriber subscriber: subscribers)
    {
        if (subscriber.chat_id())
            send_message_signal_(subscriber.chat_id(), data);
    }
}

void Informer::add_connection_state(std::shared_ptr<Informer::Connection_State_Item> &&item_ptr)
{
    bool remove_only = item_ptr->type_ == Item::T_CONNECTED && skip_connected_event_;
    auto revert_type = item_ptr->type_ == Item::T_CONNECTED ? Item::T_DISCONNECTED : Item::T_CONNECTED;
    bool is_removed = false;

    std::lock_guard lock(mutex_);

    auto it = schemedata_map_.find(item_ptr->scheme_.id());
    if (it != schemedata_map_.end())
    {
        std::vector<std::shared_ptr<Item>>& schemedata = it->second;
        for (auto d_it = schemedata.begin(); d_it != schemedata.end();)
        {
            if (d_it->get()->type_ == revert_type)
            {
                is_removed = true;
                d_it = schemedata.erase(d_it);
            }
            else
            {
                if (d_it->get()->type_ == item_ptr->type_)
                {
                    is_removed = true;
                    *d_it = item_ptr;
                }
                ++d_it;
            }
        }

        if (!is_removed && !remove_only)
            schemedata.push_back(item_ptr);

        if (schemedata.empty())
            schemedata_map_.erase(it);
    }
    else if (!remove_only)
    {
        schemedata_map_.emplace(item_ptr->scheme_.id(), std::vector<std::shared_ptr<Item>>{item_ptr});
    }

    if (!is_removed && !remove_only)
    {
        data_queue_.push(std::move(item_ptr));
        cond_.notify_one();
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

void Informer::add_status(std::shared_ptr<Status>&& item_ptr)
{
    std::lock_guard lock(mutex_);

    auto it = schemedata_map_.find(item_ptr->scheme_.id());
    if (it != schemedata_map_.end())
    {
        std::vector<std::shared_ptr<Item>>& schemedata = it->second;
        for (auto d_it = schemedata.begin(); d_it != schemedata.end();)
        {
            if (d_it->get()->type_ != Item::T_STATUS)
            {
                ++d_it;
                continue;
            }

            Status* data = static_cast<Status*>(d_it->get());
            erase_two_vectors(data->add_vect_, item_ptr->del_vect_, item_ptr->add_vect_);
            erase_two_vectors(data->del_vect_, item_ptr->add_vect_, item_ptr->del_vect_);
            if (data->add_vect_.empty() && data->del_vect_.empty())
            {
                d_it = schemedata.erase(d_it);
            }
            else
                ++d_it;
        }

        if (!item_ptr->add_vect_.empty() || !item_ptr->del_vect_.empty())
            schemedata.push_back(item_ptr);

        if (schemedata.empty())
            schemedata_map_.erase(it);
    }
    else
    {
        schemedata_map_.emplace(item_ptr->scheme_.id(), std::vector<std::shared_ptr<Item>>{item_ptr});
    }

    if (!item_ptr->add_vect_.empty() || !item_ptr->del_vect_.empty())
    {
        data_queue_.push(std::move(item_ptr));
        cond_.notify_one();
    }
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
            std::shared_ptr<Item> data_ptr = pop_data();
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

std::shared_ptr<Informer::Item> Informer::pop_data()
{
    std::shared_ptr<Item> data_ptr{std::move(data_queue_.front())};
    data_queue_.pop();

    auto it = schemedata_map_.find(data_ptr->scheme_.id());
    if (it != schemedata_map_.end())
    {
        std::vector<std::shared_ptr<Item>>& schemedata = it->second;
        schemedata.erase(std::remove(schemedata.begin(), schemedata.end(), data_ptr), schemedata.end());
        if (schemedata.empty())
            schemedata_map_.erase(it);
    }
    return data_ptr;
}

std::set<int64_t> get_disabled_chats(uint32_t status_type_id)
{
    const QString sql =
            "SELECT id FROM ( "
            "    SELECT * FROM ( "
            "        SELECT tgc.id, ds.status_id FROM das_user_groups ug "
            "        LEFT JOIN das_disabled_status ds ON ds.group_id = ug.group_id AND ds.status_id = %1 "
            "        LEFT JOIN das_tg_user tgu ON tgu.user_id = ug.user_id "
            "        LEFT JOIN das_tg_chat tgc ON tgc.admin_id = tgu.id "
            "        WHERE tgc.id IS NOT NULL "
            "        ORDER BY ds.id "
            "        LIMIT 18446744073709551615 "
            "    ) t2 "
            "    GROUP BY id "
            ") t1 "
            "WHERE status_id IS NOT NULL";

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql.arg(status_type_id));

    std::set<int64_t> disabled_chats;
    while (q.next())
        disabled_chats.insert(q.value(0).toLongLong());
    return disabled_chats;
}

void Informer::process_data(Item* data)
{
    std::map<uint32_t, std::string> text_map = get_data_text(data);
    if (text_map.empty())
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

        const std::string scheme_title = get_scheme_title(data->scheme_.id());
        if (scheme_title.empty())
        {
            qCritical() << "Can't get scheme name for id:" << data->scheme_.id();
            return;
        }
        p_data.title_ = '*' + scheme_title + '*';

        it = prepared_data_map_.emplace(data->scheme_.id(), std::move(p_data)).first;
    }

    Prepared_Data& p_data = it->second;
    for (auto it = text_map.cbegin(); it != text_map.cend(); ++it)
    {
        p_data.items_.push_back(
                    Prepared_Data::Data_Item{
                        std::move(it->second),
                        get_disabled_chats(it->first)
                    });
    }
}

std::string Informer::get_scheme_title(uint32_t scheme_id)
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
    return title.toStdString();
}

std::map<uint32_t, std::string> Informer::get_data_text(Informer::Item *data) const
{
    switch (data->type_)
    {
    case Item::T_STATUS:
        return get_status_text(static_cast<Status*>(data));

    case Item::T_CONNECTED:
        return {{0, "🚀 На связи!"}};

    case Item::T_DISCONNECTED:
    {
        auto conn_state = static_cast<Connection_State_Item*>(data);
        if (!conn_state->just_now_)
            return {{0, "💢 Отключен."}};
        break;
    }

    default:
        break;
    }

    return {};
}

std::map<uint32_t, std::string> Informer::get_status_text(Status *data) const
{
    if (data->add_vect_.empty() && data->del_vect_.empty())
        return {};

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

    std::map<uint32_t, std::string> text_map;
    for (const Informer::Section& sct: group_names)
        for (const Informer::Section::Group& group: sct.group_vect_)
            for (const Informer::Section::Group::Status& s: group.status_vect_)
                text_map.emplace(s.type_id_, s.icon_ + ' ' + sct.name_ + " - " + group.name_ + ": " + s.text_);
    return text_map;
}

void Informer::send_message(const std::map<uint32_t, Prepared_Data>& prepared_data_map)
{
    std::map<int64_t, QString> message_map;
    for (const auto& it: prepared_data_map)
    {
        const Prepared_Data& p_data = it.second;
        if (p_data.items_.empty())
            continue;

        for (int64_t chat_id: p_data.chat_set_)
        {
            std::string text = p_data.title_;

            for (const Prepared_Data::Data_Item& item: p_data.items_)
            {
                if (item.disabled_chats_.find(chat_id) == item.disabled_chats_.cend())
                    text += '\n' + item.text_;
            }
            text += "\n\n";

            qCDebug(Inf_Detail_log) << "send_message chat:" << chat_id << "text:" << text.c_str();
            send_message_signal_(chat_id, text);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

} // namespace Das
