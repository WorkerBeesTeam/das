#include <Helpz/db_builder.h>

#include "status_helper.h"

namespace Das {

/*static*/ std::vector<Status_Helper::Section> Status_Helper::get_group_names(const QSet<uint32_t>& group_id_set, Helpz::DB::Base& db,
                                                                              const Scheme_Info& scheme)
{
    if (group_id_set.empty())
        return {};

    std::vector<Status_Helper::Section> sct_vect;
    std::vector<Status_Helper::Section>::iterator it;
    std::vector<Status_Helper::Section::Group>::iterator group_it;

    QString sql = "SELECT s.id, s.name, g.id, g.title, gt.title FROM das_device_item_group g "
                  "LEFT JOIN das_section s ON s.id = g.section_id "
                  "LEFT JOIN das_dig_type gt ON gt.id = g.type_id "
                  "WHERE g.%1 AND g.";

    sql += Helpz::DB::get_db_field_in_sql("id", group_id_set);

    QSqlQuery q = db.exec(sql.arg(scheme.ids_to_sql()));
    while (q.next())
    {
        it = std::find(sct_vect.begin(), sct_vect.end(), q.value(0).toUInt());
        if (it == sct_vect.end())
        {
            it = sct_vect.emplace(sct_vect.end(), Status_Helper::Section{q.value(0).toUInt(), q.value(1).toString().toStdString(), {}});
        }

        group_it = std::find(it->group_vect_.begin(), it->group_vect_.end(), q.value(2).toUInt());
        if (group_it == it->group_vect_.end())
        {
            std::string name = q.value(3).toString().toStdString();
            if (name.empty())
                name = q.value(4).toString().toStdString();

            group_it = it->group_vect_.emplace(it->group_vect_.end(), Status_Helper::Section::Group{q.value(2).toUInt(), name, {}});
        }
    }

    return sct_vect;
}

/*static*/ std::map<uint32_t, QString> Status_Helper::get_user_names(const QSet<uint32_t> &user_id_set, Helpz::DB::Base &db,
                                                                     const Scheme_Info &scheme)
{
    if (user_id_set.empty())
        return {};

    QString sql =
R"sql(SELECT u.id, u.username, u.first_name, u.last_name FROM das_user u
LEFT JOIN das_scheme_group_user sgu ON sgu.user_id = u.id
LEFT JOIN das_scheme_groups sg ON sg.scheme_group_id = sgu.group_id
WHERE sg.%1 AND u.%2
GROUP BY u.id)sql";

    QSqlQuery q = db.exec(sql
                          .arg(scheme.ids_to_sql())
                          .arg(Helpz::DB::get_db_field_in_sql("id", user_id_set)));

    std::map<uint32_t, QString> user_name_map;
    QString name, last_name;

    while (q.next())
    {
        name = q.value(2).toString();
        last_name = q.value(3).toString();

        if (!last_name.isEmpty())
        {
            if (!name.isEmpty())
                name += ' ';
            name += last_name;
        }

        if (name.isEmpty())
            name = q.value(1).toString();
        user_name_map.emplace(q.value(0).toUInt(), std::move(name));
    }

    return user_name_map;
}

/*static*/ void Status_Helper::fill_group_status_text(std::vector<Section>& group_names, const std::map<uint32_t, QString>& user_name_map,
                                                      const DIG_Status_Type& info, const DIG_Status& item, bool is_up)
{
    if (!info.inform)
        return;

    std::string icon;
    if (is_up)
    {
        icon = "🆙";
    }
    else
    {
        switch (info.category_id)
        {
        case 2: icon = "✅"; break;
        case 3: icon = "⚠️"; break;
        case 4: icon = "🚨"; break;
        default:
            break;
        }
    }

    QString message = info.text();
    for (const QString& arg: item.args())
    {
        if (!arg.isEmpty())
            message = message.arg(arg);
    }

    auto get_user_name_msg = [&user_name_map](uint32_t id) -> QString
    {
        auto it = user_name_map.find(id);

        QString msg = " 👤 **";
        msg += it == user_name_map.cend() ? "Unknown" : it->second;
        msg += "**";
        return msg;
    };

    if (is_up)
    {
        message = "Снято cостояние \"`" + message + "`\"!";
        if (item.user_id())
            message += get_user_name_msg(item.user_id());
    }
    else if (item.user_id())
        message += get_user_name_msg(item.user_id());

    for (Status_Helper::Section& sct: group_names)
    {
        for (Status_Helper::Section::Group& group: sct.group_vect_)
        {
            if (group.id_ == item.group_id())
            {
                group.status_vect_.push_back(Status_Helper::Section::Group::Status{info.id(), icon, message.toStdString()});
            }
        }
    }
}

void Status_Helper::fill_dig_status_text(std::vector<Section>& group_names, const std::map<uint32_t, QString> &user_name_map,
                                         const QVector<DIG_Status_Type>& info_vect, const DIG_Status& item, bool is_up)
{
    for (const DIG_Status_Type& info: info_vect)
    {
        if (info.id() == item.status_id())
        {
            fill_group_status_text(group_names, user_name_map, info, item, is_up);
            return;
        }
    }
}

} // namespace Das
