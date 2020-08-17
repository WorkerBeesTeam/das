#ifndef DAS_REST_JSON_HELPER_H
#define DAS_REST_JSON_HELPER_H

#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>

#include <Helpz/db_builder.h>

namespace Das {
namespace Rest {

template<typename T>
void fill_json_object(QJsonObject& obj, const T& item, const QStringList& names)
{
    for (int i = 0; i < T::COL_COUNT; ++i)
    {
        obj.insert(names.at(i), QJsonValue::fromVariant(T::value_getter(item, i)));
    }
}

template<typename T>
QJsonObject get_json_object(const QSqlQuery& query, const QStringList& names)
{
    QJsonObject obj;

    if (query.record().count() >= T::COL_COUNT)
    {
        const T item = Helpz::DB::db_build<T>(query);
        fill_json_object(obj, item, names);
    }
    return obj;
}

template<typename T>
QJsonObject get_json_object(const QSqlQuery& query)
{
    const QStringList names = T::table_column_names();
    return get_json_object<T>(query, names);
}

template<typename T>
QJsonArray gen_json_array(const QString& suffix = QString(), const QVariantList& values = QVariantList())
{
    Helpz::DB::Base& db = Helpz::DB::Base::get_thread_local_instance();
    QStringList names = T::table_column_names();
    QJsonArray json_array;
    auto q = db.select(Helpz::DB::db_table<T>(), suffix, values);
    while(q.next())
    {
        json_array.push_back(get_json_object<T>(q, names));
    }

    return json_array;
}

template<typename T>
std::string gen_json_list(const QString& suffix = QString(), const QVariantList& values = QVariantList())
{
    return QJsonDocument(gen_json_array<T>(suffix, values)).toJson().toStdString();
}

template<typename T, typename... Args>
void set_variant_from_json_value(QVariant& value, const picojson::value& json_value)
{
    if (json_value.is<T>())
    {
        if constexpr (std::is_same<std::string, T>::value)
        {
            const QString str_value = QString::fromStdString(json_value.get<std::string>());
            if (str_value.isEmpty())
                value = QString("");
            else if (str_value.front() >= '1' && str_value.front() <= '2')
            {
                const QDateTime date_val = QDateTime::fromString(str_value, Qt::ISODateWithMs);
                if (date_val.isValid())
                    value = date_val;
                else
                    value = str_value;
            }
            else
                value = str_value;
        }
        else
            value = json_value.get<T>();
    }
    else
    {
        if constexpr (sizeof...(Args))
            set_variant_from_json_value<Args...>(value, json_value);
        else
            value.clear();
    }
}

template<typename T>
T from_json(const picojson::object& obj)
{
    const QStringList names = T::table_column_names();
    int index;
    QVariant value;
    T item;

    for (auto it: obj)
    {
        index = names.indexOf(QString::fromStdString(it.first));
        if (index == -1)
            continue;

        set_variant_from_json_value<bool, double, std::string>(value, it.second);
        T::value_setter(item, index, value);
    }

    return item;
}

// stoa_or - Example: auto dig = stoa_or(std::stoul, str, 1000000UL)
template<typename T = unsigned long>
T stoa_or(const std::string& text, T default_value = static_cast<T>(0), T(*func)(const std::string&, size_t*, int) = std::stoul)
{
    try {
        return func(text, 0, 10);
    } catch(...) {}
    return default_value;
}

} // namespace Rest
} // namespace Das

#endif // DAS_REST_JSON_HELPER_H
