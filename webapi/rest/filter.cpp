#include <QDateTime>

#include "filter.h"

namespace Das {
namespace Rest {

Filter::Filter(const served::parameters& params, const QStringList& table_column_names, std::function<QVariant(std::size_t)> value_getter)
{
    items_ = parse_parameters(params, table_column_names, value_getter);
}

Filter::Result Filter::get_result(bool is_first)
{
    return get_result(items_, is_first);
}

Filter::Result Filter::get_result(const std::vector<Filter::Item> &items, bool is_first)
{
    QString suffix;
    QVariantList values;

    for (const Item& filter: items)
    {
        if (is_first)
            is_first = false;
        else if (filter.is_or_)
            suffix += " OR";
        else
            suffix += " AND";

        suffix += " s.";
        suffix += filter.field_name_;
        suffix += ' ';
        if (filter.is_null_)
        {
            suffix += "IS";
            if (filter.is_not_)
                suffix += " NOT";
            suffix += " NULL";
        }
        else
        {
            if (filter.is_like_)
                suffix += "LIKE ";
            else
            {
                if (filter.is_not_)
                    suffix += '!';
                suffix += "= ";
            }

            suffix += '?';

            values.push_back(filter.value_);
        }
    }

    return Filter::Result{std::move(suffix), std::move(values)};
}

std::vector<Filter::Item> Filter::parse_parameters(const served::parameters &params, const QStringList &table_column_names, std::function<QVariant(std::size_t)> value_getter)
{
    Item item;
    std::vector<Item> filters;

    int param_index;
    std::string value;

    for (auto it: params)
    {
        std::cerr << "query " << it.first << ' ' << it.second << std::endl;
        if (it.first.empty()) continue;

        item.field_name_ = QString::fromStdString(it.first);

        item.is_not_ = item.field_name_.back() == '!';
        if (item.is_not_)
        {
            item.field_name_.remove(item.field_name_.size() - 1, 1);
            if (item.field_name_.isEmpty()) continue;
        }

        item.is_like_ = item.field_name_.back() == '~';
        if (item.is_like_)
        {
            item.field_name_.remove(item.field_name_.size() - 1, 1);
            if (item.field_name_.isEmpty()) continue;
        }

        item.is_or_ = item.field_name_.front() == '-';
        if (item.is_or_)
        {
            item.field_name_.remove(0, 1);
            if (item.field_name_.isEmpty()) continue;
        }

        param_index = table_column_names.indexOf(item.field_name_);
        if (param_index == -1)
            continue;

        item.is_null_ = it.second == "null";
        if (!item.is_null_)
        {
            item.value_ = value_getter(param_index);

            switch (item.value_.type())
            {
            case QVariant::String:
            {
                value = it.second;
                if (item.is_like_)
                {
                    value.insert(0, 1, '%');
                    value.push_back('%');
                }
                item.value_ = QString::fromStdString(value);
                break;
            }
            case QVariant::DateTime:    item.value_ = QDateTime::fromString(QString::fromStdString(it.second)); break;
            case QVariant::Int:         item.value_ = std::stoi(it.second); break;
            case QVariant::UInt:        item.value_ = static_cast<uint32_t>(std::stoul(it.second)); break;
            case QVariant::LongLong:    item.value_ = std::stoll(it.second); break;
            case QVariant::ULongLong:   item.value_ = std::stoull(it.second); break;
            case QVariant::Double:      item.value_ = std::stod(it.second); break;
            default:
                continue;
            }
        }

        filters.push_back(item);
    }

    return filters;
}

} // namespace Rest
} // namespace Das
