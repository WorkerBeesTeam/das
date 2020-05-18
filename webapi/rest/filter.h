#ifndef DAS_REST_FILTER_H
#define DAS_REST_FILTER_H

#include <functional>

#include <served/served.hpp>

#include <QVariant>

namespace Das {
namespace Rest {

class Filter
{
public:
    struct Item
    {
        bool is_not_, is_null_, is_like_, is_or_;
        QString field_name_;
        QVariant value_;
    };

    struct Result
    {
        QString suffix_;
        QVariantList values_;
    };

    Filter(const served::parameters& params, const QStringList& table_column_names, std::function<QVariant(std::size_t)> value_getter);

    Result get_result(bool is_first = true);
    Result get_result(const std::vector<Item>& items, bool is_first = true);

    std::vector<Item> items_;
private:
    std::vector<Item> parse_parameters(const served::parameters& params, const QStringList& table_column_names, std::function<QVariant(std::size_t)> value_getter);
};

template<typename T>
Filter get_filter(const served::parameters& params)
{
    T obj;
    return Filter(params, T::table_column_names(), std::bind(T::value_getter, std::cref(obj), std::placeholders::_1));
}

template<typename T>
Filter::Result get_filter_result(const served::parameters& params, bool is_first = true)
{
    return get_filter<T>(params).get_result(is_first);
}

} // namespace Rest
} // namespace Das

#endif // DAS_REST_FILTER_H
