#include <QCryptographicHash>

#include <Helpz/net_protocol.h>
#include <Helpz/db_builder.h>

#include <Das/db/translation.h>
#include <Das/device.h>
#include <Das/commands.h>

#include "database_delete_info.h"
#include "structure_synchronizer_base.h"

namespace Das {

Q_LOGGING_CATEGORY(Struct_Log, "struct")
Q_LOGGING_CATEGORY(Struct_Detail_Log, "struct.detail", QtInfoMsg)

namespace Ver_2_4 {

Uncheck_Foreign::Uncheck_Foreign(Helpz::Database::Base *p) : p_(p) { p->exec("SET foreign_key_checks=0;"); }
Uncheck_Foreign::~Uncheck_Foreign() { p_->exec("SET foreign_key_checks=1;"); }

// ------------------------------------------------------------------------------------------------------

Structure_Synchronizer_Base::Structure_Synchronizer_Base(Helpz::Database::Thread *db_thread) :
    modified_(false), db_thread_(db_thread)
{
}

Structure_Synchronizer_Base::~Structure_Synchronizer_Base()
{
}

bool Structure_Synchronizer_Base::modified() const
{
    return modified_;
}

void Structure_Synchronizer_Base::set_modified(bool modified)
{
    modified_ = modified;
}

void Structure_Synchronizer_Base::process_modify_message(uint32_t user_id, uint8_t struct_type, QIODevice *data_dev, uint32_t scheme_id, std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix)
{
    if (!is_can_modify(struct_type))
    {
        qCWarning(Struct_Log) << "Attempt to modify" << int(struct_type) << "user" << user_id << "scheme" << scheme_id;
        return;
    }

    using T = Structure_Synchronizer_Base;
    auto v = Helpz::Network::Protocol::DATASTREAM_VERSION;

    switch (struct_type)
    {
    case ST_DEVICE:                Helpz::apply_parse(*data_dev, v, &T::modify<Device>                         ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_PLUGIN_TYPE:           Helpz::apply_parse(*data_dev, v, &T::modify<Plugin_Type>                    ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DEVICE_ITEM:           Helpz::apply_parse(*data_dev, v, &T::modify<Database::Device_Item>          ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DEVICE_ITEM_TYPE:      Helpz::apply_parse(*data_dev, v, &T::modify<Device_Item_Type>               ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_SAVE_TIMER:            Helpz::apply_parse(*data_dev, v, &T::modify<Save_Timer>                     ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_SECTION:               Helpz::apply_parse(*data_dev, v, &T::modify<Section>                        ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DEVICE_ITEM_GROUP:     Helpz::apply_parse(*data_dev, v, &T::modify<Database::Device_Item_Group>    ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_TYPE:              Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Type>                       ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_MODE:              Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Mode>                       ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_PARAM_TYPE:        Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Param_Type>                 ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_STATUS_TYPE:       Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Status_Type>                ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_STATUS_CATEGORY:   Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Status_Category>            ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_PARAM:             Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Param>                      ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_SIGN_TYPE:             Helpz::apply_parse(*data_dev, v, &T::modify<Sign_Type>                      ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_CODES:                 Helpz::apply_parse(*data_dev, v, &T::modify<Code_Item>                      ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_TRANSLATION:           Helpz::apply_parse(*data_dev, v, &T::modify<Translation>                    ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_AUTH_GROUP:            Helpz::apply_parse(*data_dev, v, &T::modify<Auth_Group>                     ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_AUTH_GROUP_PERMISSION: Helpz::apply_parse(*data_dev, v, &T::modify<Auth_Group_Permission>          ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_USER:                  Helpz::apply_parse(*data_dev, v, &T::modify<User>                           ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_USER_GROUP:            Helpz::apply_parse(*data_dev, v, &T::modify<User_Groups>                    ,this, user_id, struct_type, scheme_id, get_bad_fix); break;

    case ST_DEVICE_ITEM_VALUE:     Helpz::apply_parse(*data_dev, v, &T::modify<Device_Item_Value>              ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_MODE_ITEM:         Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Mode_Item>                  ,this, user_id, struct_type, scheme_id, get_bad_fix); break;
    case ST_DIG_PARAM_VALUE:       Helpz::apply_parse(*data_dev, v, &T::modify<DIG_Param_Value>                ,this, user_id, struct_type, scheme_id, get_bad_fix); break;

    default:
        qCWarning(Struct_Log) << "process_modify_message unprocessed" << int(struct_type) << static_cast<Structure_Type>(struct_type) << data_dev->bytesAvailable();
        qCDebug(Struct_Log) << "pmm unproc" << data_dev->readAll().toHex();
        break;
    }
}

QByteArray Structure_Synchronizer_Base::get_structure_hash(uint8_t struct_type, Helpz::Database::Base& db, uint32_t scheme_id)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QDataStream ds(&buffer);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    add_structure_data(struct_type, ds, db, scheme_id);

    return QCryptographicHash::hash(buffer.buffer(), QCryptographicHash::Sha1);
}

QByteArray Structure_Synchronizer_Base::get_structure_hash_for_all(Helpz::Database::Base& db, uint32_t scheme_id)
{
    std::vector<uint8_t> struct_type_array = get_main_table_types();
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QDataStream ds(&buffer);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    for (const uint8_t struct_type: struct_type_array)
        add_structure_data(struct_type, ds, db, scheme_id);

    return QCryptographicHash::hash(buffer.buffer(), QCryptographicHash::Sha1);
}

Helpz::Database::Thread *Structure_Synchronizer_Base::db_thread() const
{
    return db_thread_;
}

std::vector<uint8_t> Structure_Synchronizer_Base::get_main_table_types() const
{
    return {
        ST_DEVICE,
        ST_PLUGIN_TYPE,
        ST_DEVICE_ITEM,
        ST_DEVICE_ITEM_TYPE,
        ST_SAVE_TIMER,
        ST_SECTION,
        ST_DEVICE_ITEM_GROUP,
        ST_DIG_TYPE,
        ST_DIG_MODE,
        ST_DIG_PARAM_TYPE,
        ST_DIG_STATUS_TYPE,
        ST_DIG_STATUS_CATEGORY,
        ST_DIG_PARAM,
        ST_SIGN_TYPE,
        ST_CODES,
        ST_TRANSLATION,
        ST_AUTH_GROUP,
        ST_AUTH_GROUP_PERMISSION,
        ST_USER,
        ST_USER_GROUP
    };
}

bool Structure_Synchronizer_Base::is_main_table(uint8_t struct_type) const
{
    if (!struct_type)
        return true;
    std::vector<uint8_t> struct_type_array = get_main_table_types();
    return std::find(struct_type_array.cbegin(), struct_type_array.cend(), struct_type) != struct_type_array.cend();
}

bool Structure_Synchronizer_Base::is_can_modify(uint8_t /*struct_type*/) const
{
    return true;
}

void Structure_Synchronizer_Base::fill_suffix(uint8_t /*struct_type*/, QString &/*where_str*/) {}

void Structure_Synchronizer_Base::add_structure_data(uint8_t struct_type, QDataStream& ds, Helpz::Database::Base& db, uint32_t scheme_id)
{
    add_structure_template(struct_type, ds, db, scheme_id);
}

void Structure_Synchronizer_Base::add_structure_items_data(uint8_t struct_type, const QVector<uint32_t>& id_vect, QDataStream& ds, Helpz::Database::Base& db, uint32_t scheme_id)
{
    if (id_vect.isEmpty())
    {
        ds << uint32_t(0);
    }
    else
    {
        add_structure_template(struct_type, ds, db, scheme_id, id_vect);
    }
}

QMap<uint32_t, uint16_t> Structure_Synchronizer_Base::get_structure_hash_map_by_type(uint8_t struct_type, Helpz::Database::Base& db, uint32_t scheme_id)
{
    switch (struct_type)
    {
    case ST_DEVICE:                return get_structure_hash_map<Device>                  (struct_type, db, scheme_id); break;
    case ST_PLUGIN_TYPE:           return get_structure_hash_map<Plugin_Type>             (struct_type, db, scheme_id); break;
    case ST_DEVICE_ITEM:           return get_structure_hash_map<Device_Item>             (struct_type, db, scheme_id); break;
    case ST_DEVICE_ITEM_TYPE:      return get_structure_hash_map<Device_Item_Type>        (struct_type, db, scheme_id); break;
    case ST_SECTION:               return get_structure_hash_map<Section>                 (struct_type, db, scheme_id); break;
    case ST_DEVICE_ITEM_GROUP:     return get_structure_hash_map<Device_item_Group>       (struct_type, db, scheme_id); break;
    case ST_DIG_TYPE:              return get_structure_hash_map<DIG_Type>                (struct_type, db, scheme_id); break;
    case ST_DIG_MODE:              return get_structure_hash_map<DIG_Mode>                (struct_type, db, scheme_id); break;
    case ST_DIG_PARAM:             return get_structure_hash_map<DIG_Param>               (struct_type, db, scheme_id); break;
    case ST_DIG_PARAM_TYPE:        return get_structure_hash_map<DIG_Param_Type>          (struct_type, db, scheme_id); break;
    case ST_DIG_STATUS_TYPE:       return get_structure_hash_map<DIG_Status_Type>         (struct_type, db, scheme_id); break;
    case ST_DIG_STATUS_CATEGORY:   return get_structure_hash_map<DIG_Status_Category>     (struct_type, db, scheme_id); break;
    case ST_SIGN_TYPE:             return get_structure_hash_map<Sign_Type>               (struct_type, db, scheme_id); break;
    case ST_CODES:                 return get_structure_hash_map<Code_Item>               (struct_type, db, scheme_id); break;
    case ST_SAVE_TIMER:            return get_structure_hash_map<Save_Timer>              (struct_type, db, scheme_id); break;
    case ST_TRANSLATION:           return get_structure_hash_map<Translation>             (struct_type, db, scheme_id); break;
    case ST_AUTH_GROUP:            return get_structure_hash_map<Auth_Group>              (struct_type, db, scheme_id); break;
    case ST_AUTH_GROUP_PERMISSION: return get_structure_hash_map<Auth_Group_Permission>   (struct_type, db, scheme_id); break;
    case ST_USER:                  return get_structure_hash_map<User>                    (struct_type, db, scheme_id); break;
    case ST_USER_GROUP:            return get_structure_hash_map<User_Groups>             (struct_type, db, scheme_id); break;

    case ST_DEVICE_ITEM_VALUE:     return get_structure_hash_map<Device_Item_Value>       (struct_type, db, scheme_id); break;
    case ST_DIG_MODE_ITEM:         return get_structure_hash_map<DIG_Mode_Item>           (struct_type, db, scheme_id); break;
    case ST_DIG_PARAM_VALUE:       return get_structure_hash_map<DIG_Param_Value>         (struct_type, db, scheme_id); break;

    default:
        qCWarning(Struct_Log) << "get_structure_hash_map_by_type unprocessed" << struct_type;
        break;
    }
    return {};
}

template<typename T>
QString Structure_Synchronizer_Base::get_db_list_suffix(uint8_t struct_type, const QVector<uint32_t>& id_vect, uint32_t scheme_id)
{
    QString suffix = Helpz::Database::db_get_items_list_suffix<T>(id_vect, Database::Scheme_Table_Helper<T>::pk_num);

    if (Database::has_scheme_id<T>())
    {
        if (suffix.isEmpty())
            suffix = "WHERE";
        else
            suffix += " AND";

        suffix += " scheme_id = " + QString::number(scheme_id);
    }

    fill_suffix(struct_type, suffix);
    return suffix;
}

template<typename T>
QVector<T> Structure_Synchronizer_Base::get_db_list(uint8_t struct_type, Helpz::Database::Base& db, uint32_t scheme_id, const QVector<uint32_t>& id_vect)
{
    return Helpz::Database::db_build_list<T>(db, get_db_list_suffix<T>(struct_type, id_vect, scheme_id));
}

void Structure_Synchronizer_Base::add_structure_template(uint8_t struct_type, QDataStream& ds, Helpz::Database::Base& db, uint32_t scheme_id, const QVector<uint32_t>& id_vect)
{
    switch (struct_type)
    {
    case ST_DEVICE:                ds << get_db_list<Device>               (struct_type, db, scheme_id, id_vect); break;
    case ST_PLUGIN_TYPE:           ds << get_db_list<Plugin_Type>          (struct_type, db, scheme_id, id_vect); break;
    case ST_DEVICE_ITEM:           ds << get_db_list<Device_Item>          (struct_type, db, scheme_id, id_vect); break;
    case ST_DEVICE_ITEM_TYPE:      ds << get_db_list<Device_Item_Type>     (struct_type, db, scheme_id, id_vect); break;
    case ST_SAVE_TIMER:            ds << get_db_list<Save_Timer>           (struct_type, db, scheme_id, id_vect); break;
    case ST_SECTION:               ds << get_db_list<Section>              (struct_type, db, scheme_id, id_vect); break;
    case ST_DEVICE_ITEM_GROUP:     ds << get_db_list<Device_item_Group>    (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_TYPE:              ds << get_db_list<DIG_Type>             (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_MODE:              ds << get_db_list<DIG_Mode>             (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_PARAM_TYPE:        ds << get_db_list<DIG_Param_Type>       (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_STATUS_TYPE:       ds << get_db_list<DIG_Status_Type>      (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_STATUS_CATEGORY:   ds << get_db_list<DIG_Status_Category>  (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_PARAM:             ds << get_db_list<DIG_Param>            (struct_type, db, scheme_id, id_vect); break;
    case ST_SIGN_TYPE:             ds << get_db_list<Sign_Type>            (struct_type, db, scheme_id, id_vect); break;
    case ST_CODES:                 ds << get_db_list<Code_Item>            (struct_type, db, scheme_id, id_vect); break;
    case ST_TRANSLATION:           ds << get_db_list<Translation>          (struct_type, db, scheme_id, id_vect); break;
    case ST_AUTH_GROUP:            ds << get_db_list<Auth_Group>           (struct_type, db, scheme_id, id_vect); break;
    case ST_AUTH_GROUP_PERMISSION: ds << get_db_list<Auth_Group_Permission>(struct_type, db, scheme_id, id_vect); break;
    case ST_USER:                  ds << get_db_list<User>                 (struct_type, db, scheme_id, id_vect); break;
    case ST_USER_GROUP:            ds << get_db_list<User_Groups>          (struct_type, db, scheme_id, id_vect); break;

    case ST_DEVICE_ITEM_VALUE:     ds << get_db_list<Device_Item_Value>    (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_MODE_ITEM:         ds << get_db_list<DIG_Mode_Item>        (struct_type, db, scheme_id, id_vect); break;
    case ST_DIG_PARAM_VALUE:       ds << get_db_list<DIG_Param_Value>      (struct_type, db, scheme_id, id_vect); break;

    default:
        qCWarning(Struct_Log) << "add_structure_data unprocessed" << static_cast<Structure_Type>(struct_type);
        break;
    }
}

template<typename T>
QMap<uint32_t, uint16_t> Structure_Synchronizer_Base::get_structure_hash_map(uint8_t struct_type, Helpz::Database::Base& db, uint32_t scheme_id)
{
    using Helper = Database::Scheme_Table_Helper<T>;
    using PK_Type = typename Helper::PK_Type;

    QMap<uint32_t, uint16_t> hash_map;

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);

    QString suffix;
    if (Database::has_scheme_id<T>())
            suffix = "WHERE scheme_id = " + QString::number(scheme_id);

    fill_suffix(struct_type, suffix);

    QVector<T> items = Helpz::Database::db_build_list<T>(db, suffix);
    for (const T& item: items)
    {
        ds << item;

        const PK_Type pk_value = Helper::get_pk(item);
        const uint16_t checksum = qChecksum(data.constData(), data.size());
        hash_map.insert(pk_value, checksum);

        ds.device()->seek(0);
        data.clear();
    }

    return hash_map;
}

template<typename T>
void Structure_Synchronizer_Base::modify(QVector<T>&& upd_vect, QVector<T>&& insrt_vect, QVector<uint32_t>&& del_vect, uint32_t user_id,
                                    uint8_t struct_type, uint32_t scheme_id, std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix)
{
    db_thread_->add([=](Helpz::Database::Base* db) mutable
    {
        auto start_point = std::chrono::system_clock::now();

        std::shared_ptr<Bad_Fix> bad_fix;
        Structure_Synchronizer_Base* self = nullptr;
        if (get_bad_fix)
        {
            bad_fix = get_bad_fix();
            if (bad_fix)
            {
                self = bad_fix->get_structure_synchronizer();
            }
        }
        else
        {
            self = this;
        }

        if (self)
        {
            bool ok = self->modify_table(struct_type, *db, upd_vect, insrt_vect, del_vect, scheme_id);
            if (ok)
            {
                if (!self->modified_ && self->is_main_table(struct_type))
                {
                    self->modified_ = true;
                }

                qCInfo(Struct_Log).nospace() << user_id << "|modify " << static_cast<Structure_Type>(struct_type) << ' '
                                             << upd_vect.size() << " update, " << insrt_vect.size() << " insert, " << del_vect.size() << " del";

                QByteArray buffer;
                QDataStream data_stream(&buffer, QIODevice::WriteOnly);
                data_stream.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
                data_stream << user_id << struct_type << upd_vect << insrt_vect << del_vect;
                self->send_modify_response(struct_type, buffer, user_id);
            }
        }

        std::chrono::milliseconds delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_point);
        if (delta > std::chrono::milliseconds(100))
        {
            std::cout << "-----------> db freeze " << delta.count() << "ms Structure_Synchronizer_Base::modify vs"
                      << (upd_vect.size() + insrt_vect.size() + del_vect.size()) << " scheme_id " << scheme_id << ' ' << typeid(T).name() << std::endl;
        }
    });
}

template<class T>
bool Structure_Synchronizer_Base::modify_table(uint8_t struct_type, Helpz::Database::Base& db,
                                          const QVector<T>& update_vect,
                                          QVector<T>& insert_vect,
                                          const QVector<uint32_t>& delete_vect, uint32_t scheme_id)
{
    using namespace Helpz::Database;
    using Helper = Database::Scheme_Table_Helper<T>;
    using PK_Type = typename Helper::PK_Type;

    Table table = db_table<T>();

    Uncheck_Foreign uncheck_foreign(&db);

    // DELETE
    if (!Database::db_delete_rows<T>(db, delete_vect, scheme_id))
    {
        qWarning() << "modify_table: Failed delete row in" << table.name() << "scheme_id" << scheme_id;
        return false;
    }

    // UPDATE
    std::vector<const T*> items;
    QVector<PK_Type> id_vect;
    for (const T& item: update_vect)
    {
        id_vect.push_back(item.id());
        items.push_back(&item);
    }

    if (id_vect.size())
    {
        int compare_column_count = table.field_names().size();
        if (Database::has_scheme_id<T>())
            --compare_column_count;

        std::vector<Update_Info> update_list;

        PK_Type id_value;
        QVariant sql_value, client_value;

        QSqlQuery q = db.select(table, get_db_list_suffix<T>(struct_type, id_vect, scheme_id));
        if (!q.isActive())
        {
            qWarning() << "modify_table: Failed select from" << table.name() << "scheme_id" << scheme_id;
            return false;
        }

        while (q.next())
        {
            id_value = Helper::get_query_pk(q);

            for (auto it = items.begin(); it <= items.end(); it++)
            {
                if (it == items.end())
                {
                    qWarning() << "That imposible. In modify_table item id not fount" << id_value << "table" << table.name();
                }
                else if (id_value == Helper::get_pk(**it))
                {
                    Update_Info ui;
                    for (int pos = 1; pos < compare_column_count; ++pos)
                    {
                        sql_value = q.isNull(pos) ? QVariant() : q.value(pos);
                        client_value = T::value_getter(**it, pos);
                        if (sql_value != client_value)
                        {
                            ui.changed_field_names_.push_back(table.field_names().at(pos));
                            ui.changed_fields_.push_back(client_value);
                        }
                        else if (sql_value.type() != client_value.type() && sql_value.toString() != client_value.toString())
                        {
                            qDebug() << typeid(T).name() << table.name() << table.field_names().at(pos)
                                     << "is same value but diffrent types" << sql_value << client_value;
                        }
                    }

                    if (!ui.changed_fields_.empty())
                    {
                        ui.changed_fields_.push_back(id_value);
                        update_list.push_back(std::move(ui));
                    }

                    items.erase(it--);
                    break;
                }
            }
        }

        if (!update_list.empty())
        {
            const QString where = table.field_names().at(Helper::pk_num) + "=?" +
                    (Database::has_scheme_id<T>() ? " AND scheme_id=" + QString::number(scheme_id) : QString());

            Table upd_table{table.name(), {}, {}};

            for (const Update_Info& ui: update_list)
            {
                upd_table.set_field_names(ui.changed_field_names_);
                if (!db.update(upd_table, ui.changed_fields_, where).isActive())
                {
                    qWarning() << "modify_table: Failed update row in" << table.name() << "scheme_id" << scheme_id;
                    return false;
                }
            }
        }
    }

    // Insert if row for update in not found
    for (const T* item: items)
        qCCritical(Struct_Log) << "Item:" << item->id() << "type:" << typeid(T).name() << "not updated!";

    // INSERT

    if (Helper::pk_num != T::COL_id)
        table.field_names().removeFirst();

    QVariant id;
    QVariantList values;

    for (T& item: insert_vect)
    {
        if constexpr (Database::has_scheme_id<T>())
            item.set_scheme_id(scheme_id);

        values = T::to_variantlist(item);
        if (Helper::pk_num != T::COL_id)
            values.removeFirst();
//        if (is_need_scheme_id_insert<T>())
//            values.push_back(scheme_id);

        if (!db.insert(table, values, &id))
        {
            qWarning() << "modify_table: Failed insert row to" << table.name() << "scheme_id" << scheme_id;
            return false;
        }

        T::value_setter(item, T::COL_id, id);
    }

    return true;
}

} // namespace Ver_2_4
} // namespace Das
