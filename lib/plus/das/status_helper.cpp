#include "status_helper.h"

namespace Das {

/*static*/ std::vector<Status_Helper::Section> Status_Helper::get_group_names(const QSet<uint32_t>& group_id_set, Helpz::DB::Base& db, uint32_t scheme_id)
{
    if (group_id_set.empty())
        return {};

    std::vector<Status_Helper::Section> sct_vect;
    std::vector<Status_Helper::Section>::iterator it;
    std::vector<Status_Helper::Section::Group>::iterator group_it;

    QString sql = "SELECT s.id, s.name, g.id, g.title, gt.title FROM das_device_item_group g "
                  "LEFT JOIN das_section s ON s.id = g.section_id AND s.scheme_id = %1 "
                  "LEFT JOIN das_dig_type gt ON gt.id = g.type_id AND gt.scheme_id = %1 "
                  "WHERE g.scheme_id = %1 AND g.id IN (";

    for (uint32_t id: group_id_set)
        sql += QString::number(id) + ',';
    sql[sql.size() - 1] = ')';

    QSqlQuery q = db.exec(sql.arg(scheme_id));
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

/*static*/ void Status_Helper::fill_group_status_text(std::vector<Section>& group_names, const DIG_Status_Type& info, const DIG_Status& item, bool is_up)
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

    if (is_up)
        message = "Состояние \"`" + message + "`\" снято!";

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

void Status_Helper::fill_dig_status_text(std::vector<Section>& group_names, const QVector<DIG_Status_Type>& info_vect, const DIG_Status& item, bool is_up)
{
    for (const DIG_Status_Type& info: info_vect)
    {
        if (info.id() == item.status_id())
        {
            fill_group_status_text(group_names, info, item, is_up);
            return;
        }
    }
}

} // namespace Das
