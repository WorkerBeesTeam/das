#ifndef DAS_SERVER_LOG_SAVER_BASE_H
#define DAS_SERVER_LOG_SAVER_BASE_H

#include <mutex>

#include <QVariantList>

#include <Helpz/db_table.h>

#include "log_saver_data.h"

namespace Das {
namespace Server {
namespace Log_Saver {

using namespace Helpz::DB;

class Saver_Base
{
public:
    virtual ~Saver_Base() = default;

    bool empty() const;
    virtual bool empty_data() const = 0;
    virtual bool empty_cache() const = 0;
    virtual shared_ptr<Data> get_data_pack(int max_pack_size = 100) = 0;
    virtual void erase_empty_cache() = 0;

    void process_data_pack(shared_ptr<Data> data);
    virtual void process_log(shared_ptr<Data> data) = 0;
    virtual void process_cache(shared_ptr<Data> data) = 0;

    template<typename T>
    QString get_update_where(const Table& table, const vector<uint32_t>& compared_fields) const
    {
        QString where;
        for (uint32_t field: compared_fields)
        {
            if (!where.isEmpty())
                where += " AND ";
            where += table.field_names().at(field);
            if (field == T::COL_timestamp_msecs)
                where += '<'; // = for timestamp_msecs = 0, < for others
            where += "=?";
        }
        return where;
    }

    template<typename T>
    QString get_compare_where(const vector<T>& pack, const Table& table,
                              const vector<uint32_t>& compared_fields, QVariantList& values, bool add_where_keyword = true) const
    {
        QString item_template = get_where_item_template(table, compared_fields);
        QString where;

        for (const T& item: pack)
        {
            if (where.isEmpty())
            {
                if (add_where_keyword)
                    where = "WHERE ";
                where += '(';
            }
            else
                where += ") OR (";
            where += item_template;

            for (uint32_t field_index: compared_fields)
                values.push_back(T::value_getter(item, field_index));
        }
        where += ')';
        return where;
    }

    QString get_where_item_template(const Table& table, const vector<uint32_t>& compared_fields) const;

protected:
    void save_dump_to_file(size_t type_code, const QVariantList& data);
    QVariantList load_dump_file(size_t type_code);
private:
    mutex _fail_mutex;
};

} // namespace Log_Saver
} // namespace Server
} // namespace Das

#endif // DAS_SERVER_LOG_SAVER_BASE_H
