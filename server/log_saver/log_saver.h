#ifndef DAS_SERVER_LOG_SAVER_SAVER_H
#define DAS_SERVER_LOG_SAVER_SAVER_H

#include <type_traits>
#include <queue>

#include "log_saver_cache.h"

namespace Das {
namespace Server {
namespace Log_Saver {

template<typename T>
class Saver :
        public conditional<Cache_Type<T>::Is_Comparable::value,
                 Saver_Cache<typename Cache_Type<T>::Type>, No_Cache>::type
{
public:
    virtual ~Saver() = default;

    template<template<typename...> class Container>
    void add(uint32_t scheme_id, const Container<T>& data)
    {
        for (const T& item: data)
        {
            if constexpr (is_same<T, Log_Value_Item>::value)
                if (!item.need_to_save())
                    continue;

            _data.push(item);
            _data.back().set_scheme_id(scheme_id);
        }

        if constexpr (Cache_Type<T>::Is_Comparable::value)
            Saver_Cache<typename Cache_Type<T>::Type>::add_cache(scheme_id, data);
    }

    bool empty_data() const override
    {
        return _data.empty();
    }

    shared_ptr<Data> get_data_pack(int max_pack_size = 100) override
    {
        if constexpr (Cache_Type<T>::Is_Comparable::value)
            if (Saver_Cache<typename Cache_Type<T>::Type>::is_cache_ready_to_save())
                return Saver_Cache<typename Cache_Type<T>::Type>::get_cache_data_pack();

        vector<T> pack;
        for (int i = 0; i < max_pack_size && !_data.empty(); ++i)
        {
            pack.push_back(move(_data.front()));
            _data.pop();
        }
        return make_shared<Data_Impl<T>>(Data::Log, move(pack));
    }

private:

    void process_log(shared_ptr<Data> data) override
    {
        auto t_data = static_pointer_cast<Data_Impl<T>>(data);
        vector<T>& pack = t_data->_data;

        QVariantList values_pack;
        for (const T& item: pack)
            values_pack += T::to_variantlist(item);

        Base& db = Base::get_thread_local_instance();

        auto table = db_table<T>();
        const QString sql = get_custom_q_array(table, values_pack.size() / table.field_names().size());
        if (!db.exec(sql, values_pack).isActive())
        {
            for (int i = 0; i < values_pack.size(); )
            {
                auto dbg = qCritical().noquote() << "Lost log " << typeid(T).name();
                for (const QString& name: table.field_names())
                {
                    dbg << name << '=' << values_pack.at(i).toString() << ' ';
                    ++i;
                }
            }
            // TODO: Try to insert each separately. With check duplicate?

            Saver_Base::save_dump_to_file(typeid(T).hash_code(), values_pack);
        }
    }

    QString get_custom_q_array(const Table& table, int row_count) const
    {
        // " + (force ? QString("IGNORE ") : QString()) + "
        return "INSERT IGNORE INTO " +
                table.name() + '(' + table.field_names().join(',') + ") VALUES" +
                Base::get_q_array(table.field_names().size(), row_count);
    }

    queue<T> _data;
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_SAVER_H
