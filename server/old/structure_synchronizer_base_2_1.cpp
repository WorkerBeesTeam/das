#include <QCryptographicHash>

#include <Helpz/net_protocol.h>
#include <Helpz/db_builder.h>

#include <Das/commands.h>
#include <Das/db/translation.h>

#include <plus/das/database_delete_info.h>
#include "structure_synchronizer_base_2_1.h"

namespace Das {
namespace Ver_2_1 {

QDataStream &operator>>(QDataStream &ds, Ver_2_1_DIG_Status &item)
{
    uint32_t id;
    ds >> id;
    item.set_id(id);
    return ds >> static_cast<DIG_Status&>(item);
}

QDataStream &operator<<(QDataStream &ds, const Ver_2_1_DIG_Status &item)
{
    return ds << item.id() << static_cast<const DIG_Status&>(item);
}

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

QString Structure_Synchronizer_Base::common_db_name() const
{
    return common_db_name_;
}

void Structure_Synchronizer_Base::set_common_db_name(const QString &common_db_name)
{
    common_db_name_ = common_db_name;
}

void Structure_Synchronizer_Base::process_modify_message(uint32_t user_id, uint8_t struct_type, QIODevice *data_dev, const QString& db_name, std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix)
{
    auto v = Helpz::Network::Protocol::DATASTREAM_VERSION;

    switch (struct_type)
    {
    case STRUCT_TYPE_DEVICES:               Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Device>                ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_CHECKER_TYPES:         Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Plugin_Type>           ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_DEVICE_ITEMS:          Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Database::Device_Item> ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_DEVICE_Device_ITEM_TYPES:     Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Device_Item_Type>             ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_SAVE_TIMERS:           Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Save_Timer>            ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_SECTIONS:              Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Section>               ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_GROUPS:                Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Database::Device_Item_Group>  ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_GROUP_TYPES:           Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<DIG_Type>       ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_GROUP_DIG_MODES:      Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<DIG_Mode>             ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_GROUP_DIG_PARAM_TYPES:     Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<DIG_Param_Type>            ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_TYPE:     Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<DIG_Status_Type>           ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_CATEGORY:     Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<DIG_Status_Category>           ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_DIG_PARAM:           Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Dig_Param>           ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_SIGNS:                 Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Sign_Item>             ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_SCRIPTS:               Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Code_Item>             ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_VIEW:                  Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<View>                  ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_VIEW_ITEM:             Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<View_Item>             ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_TRANSLATIONS:          Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Translation>           ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_AUTH_GROUP:            Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Auth_Group>            ,this, user_id, struct_type, common_db_name_, get_bad_fix); break;
    case STRUCT_TYPE_AUTH_GROUP_PERMISSION: Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Auth_Group_Permission> ,this, user_id, struct_type, common_db_name_, get_bad_fix); break;
    case STRUCT_TYPE_AUTH_USER:             Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Auth_User>             ,this, user_id, struct_type, common_db_name_, get_bad_fix); break;
    case STRUCT_TYPE_AUTH_USER_GROUP:       Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Auth_User_Group>       ,this, user_id, struct_type, common_db_name_, get_bad_fix); break;

    case STRUCT_TYPE_DEVICE_ITEM_VALUES:    Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Device_Item_Value>     ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_DIG_MODE_ITEM:            Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<DIG_Mode_Item>            ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_GROUP_STATUS:          Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<Ver_2_1_DIG_Status>     ,this, user_id, struct_type, db_name, get_bad_fix); break;
    case STRUCT_TYPE_DIG_PARAM_VALUE:     Helpz::apply_parse(*data_dev, v, &Structure_Synchronizer_Base::modify<DIG_Param_Value>     ,this, user_id, struct_type, db_name, get_bad_fix); break;

    default:
        qCWarning(Struct_Log) << "process_modify_message unprocessed" << int(struct_type) << static_cast<StructureType>(struct_type) << data_dev->bytesAvailable();
        qCDebug(Struct_Log) << "pmm unproc" << data_dev->readAll().toHex();
        break;
    }
}

QByteArray Structure_Synchronizer_Base::get_structure_hash(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QDataStream ds(&buffer);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    add_structure_data(struct_type, ds, db, db_name);

    return QCryptographicHash::hash(buffer.buffer(), QCryptographicHash::Sha1);
}

QByteArray Structure_Synchronizer_Base::get_structure_hash_for_all(Helpz::Database::Base& db, const QString& db_name)
{
    std::vector<uint8_t> struct_type_array = get_main_table_types();
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QDataStream ds(&buffer);
    ds.setVersion(Helpz::Network::Protocol::DATASTREAM_VERSION);
    for (const uint8_t struct_type: struct_type_array)
        add_structure_data(struct_type, ds, db, db_name);

    return QCryptographicHash::hash(buffer.buffer(), QCryptographicHash::Sha1);
}

Helpz::Database::Thread *Structure_Synchronizer_Base::db_thread() const
{
    return db_thread_;
}

std::vector<uint8_t> Structure_Synchronizer_Base::get_main_table_types() const
{
    return {
        STRUCT_TYPE_DEVICES,
        STRUCT_TYPE_CHECKER_TYPES,
        STRUCT_TYPE_DEVICE_ITEMS,
        STRUCT_TYPE_DEVICE_Device_ITEM_TYPES,
        STRUCT_TYPE_SAVE_TIMERS,
        STRUCT_TYPE_SECTIONS,
        STRUCT_TYPE_GROUPS,
        STRUCT_TYPE_GROUP_TYPES,
        STRUCT_TYPE_GROUP_DIG_MODES,
        STRUCT_TYPE_GROUP_DIG_PARAM_TYPES,
        STRUCT_TYPE_GROUP_DIG_STATUS_TYPE,
        STRUCT_TYPE_GROUP_DIG_STATUS_CATEGORY,
        STRUCT_TYPE_DIG_PARAM,
        STRUCT_TYPE_SIGNS,
        STRUCT_TYPE_SCRIPTS,
        STRUCT_TYPE_VIEW,
        STRUCT_TYPE_VIEW_ITEM,
        STRUCT_TYPE_TRANSLATIONS,
        STRUCT_TYPE_AUTH_GROUP,
        STRUCT_TYPE_AUTH_GROUP_PERMISSION,
        STRUCT_TYPE_AUTH_USER,
        STRUCT_TYPE_AUTH_USER_GROUP
    };
}

bool Structure_Synchronizer_Base::is_main_table(uint8_t struct_type) const
{
    if (!struct_type)
    {
        return true;
    }
    std::vector<uint8_t> struct_type_array = get_main_table_types();
    return std::find(struct_type_array.cbegin(), struct_type_array.cend(), struct_type) != struct_type_array.cend();
}

QString Structure_Synchronizer_Base::get_suffix(uint8_t /*struct_type*/) { return {}; }

void Structure_Synchronizer_Base::add_structure_data(uint8_t struct_type, QDataStream& ds, Helpz::Database::Base& db, const QString& db_name)
{
    add_structure_template(struct_type, ds, db, db_name);
}

void Structure_Synchronizer_Base::add_structure_items_data(uint8_t struct_type, const QVector<uint32_t>& id_vect, QDataStream& ds, Helpz::Database::Base& db, const QString& db_name)
{
    if (id_vect.isEmpty())
    {
        ds << uint32_t(0);
    }
    else
    {
        add_structure_template(struct_type, ds, db, db_name, id_vect);
    }
}

QVector<QPair<uint32_t, uint16_t>> Structure_Synchronizer_Base::get_structure_hash_vect_by_type(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name)
{
    switch (struct_type)
    {
    case STRUCT_TYPE_DEVICES:               return get_structure_hash_vect<Device>               (struct_type, db, db_name); break;
    case STRUCT_TYPE_CHECKER_TYPES:         return get_structure_hash_vect<Plugin_Type>          (struct_type, db, db_name); break;
    case STRUCT_TYPE_DEVICE_ITEMS:          return get_structure_hash_vect<Device_Item>           (struct_type, db, db_name); break;
    case STRUCT_TYPE_DEVICE_Device_ITEM_TYPES:     return get_structure_hash_vect<Device_Item_Type>            (struct_type, db, db_name); break;
    case STRUCT_TYPE_SECTIONS:              return get_structure_hash_vect<Section>              (struct_type, db, db_name); break;
    case STRUCT_TYPE_GROUPS:                return get_structure_hash_vect<Device_item_Group>            (struct_type, db, db_name); break;
    case STRUCT_TYPE_GROUP_TYPES:           return get_structure_hash_vect<DIG_Type>      (struct_type, db, db_name); break;
    case STRUCT_TYPE_GROUP_DIG_MODES:      return get_structure_hash_vect<DIG_Mode>            (struct_type, db, db_name); break;
    case STRUCT_TYPE_DIG_PARAM:           return get_structure_hash_vect<Dig_Param>          (struct_type, db, db_name); break;
    case STRUCT_TYPE_GROUP_DIG_PARAM_TYPES:     return get_structure_hash_vect<DIG_Param_Type>           (struct_type, db, db_name); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_TYPE:     return get_structure_hash_vect<DIG_Status_Type>          (struct_type, db, db_name); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_CATEGORY:     return get_structure_hash_vect<DIG_Status_Category>          (struct_type, db, db_name); break;
    case STRUCT_TYPE_SIGNS:                 return get_structure_hash_vect<Sign_Item>            (struct_type, db, db_name); break;
    case STRUCT_TYPE_SCRIPTS:               return get_structure_hash_vect<Code_Item>            (struct_type, db, db_name); break;
    case STRUCT_TYPE_VIEW:                  return get_structure_hash_vect<View>                 (struct_type, db, db_name); break;
    case STRUCT_TYPE_VIEW_ITEM:             return get_structure_hash_vect<View_Item>            (struct_type, db, db_name); break;
    case STRUCT_TYPE_SAVE_TIMERS:           return get_structure_hash_vect<Save_Timer>           (struct_type, db, db_name); break;
    case STRUCT_TYPE_TRANSLATIONS:          return get_structure_hash_vect<Translation>          (struct_type, db, db_name); break;
    case STRUCT_TYPE_AUTH_GROUP:            return get_structure_hash_vect<Auth_Group>           (struct_type, db, common_db_name()); break;
    case STRUCT_TYPE_AUTH_GROUP_PERMISSION: return get_structure_hash_vect<Auth_Group_Permission>(struct_type, db, common_db_name()); break;
    case STRUCT_TYPE_AUTH_USER:             return get_structure_hash_vect<Auth_User>            (struct_type, db, common_db_name()); break;
    case STRUCT_TYPE_AUTH_USER_GROUP:       return get_structure_hash_vect<Auth_User_Group>      (struct_type, db, common_db_name()); break;

    case STRUCT_TYPE_DEVICE_ITEM_VALUES:    return get_structure_hash_vect<Device_Item_Value>    (struct_type, db, db_name); break;
    case STRUCT_TYPE_DIG_MODE_ITEM:            return get_structure_hash_vect<DIG_Mode_Item>           (struct_type, db, db_name); break;
    case STRUCT_TYPE_GROUP_STATUS:          return get_structure_hash_vect<Ver_2_1_DIG_Status>    (struct_type, db, db_name); break;
    case STRUCT_TYPE_DIG_PARAM_VALUE:     return get_structure_hash_vect<DIG_Param_Value>    (struct_type, db, db_name); break;

    default:
        qCWarning(Struct_Log) << "get_structure_hash_vect_by_type unprocessed" << struct_type;
        break;
    }
    return {};
}

template<typename T>
QString Structure_Synchronizer_Base::get_db_list_suffix(uint8_t struct_type, const QVector<uint32_t>& id_vect)
{
    QString first_suffix = get_suffix(struct_type);
    QString second_suffix = Helpz::Database::get_items_list_suffix<T>(id_vect);

    if (!first_suffix.isEmpty() && !second_suffix.isEmpty())
    {
        /* Если оба суффикса не пусты, то есть два варианта положения first_suffix, до second_suffix и после.
         * Если first_suffix начинается с WHERE или WHERE вообще отсутствует (Например ORDER BY), то
         * мы меняем суффиксы местами, в противном случае first_suffix может начинатся например с LEFT JOIN.
         * Дальше во втором суммифксе мы заменяем WHERE на AND если он там есть. В конце мы объединяем первый и второй суффикс */
        if (first_suffix.indexOf("WHERE", Qt::CaseInsensitive) <= 0)
        {
            std::swap(first_suffix, second_suffix);
        }

        if (second_suffix.indexOf("WHERE", Qt::CaseInsensitive) != -1)
        {
            second_suffix.replace("WHERE", "AND", Qt::CaseInsensitive);
        }
    }

    return first_suffix + second_suffix;
}

template<typename T>
QVector<T> Structure_Synchronizer_Base::get_db_list(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name, const QVector<uint32_t>& id_vect)
{
    return Helpz::Database::db_build_list<T>(db, get_db_list_suffix<T>(struct_type, id_vect), db_name);
}

void Structure_Synchronizer_Base::add_structure_template(uint8_t struct_type, QDataStream& ds, Helpz::Database::Base& db, const QString& db_name, const QVector<uint32_t>& id_vect)
{
    switch (struct_type)
    {
    case STRUCT_TYPE_DEVICES:               ds << get_db_list<Device>               (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_CHECKER_TYPES:         ds << get_db_list<Plugin_Type>          (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_DEVICE_ITEMS:          ds << get_db_list<Device_Item>           (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_DEVICE_Device_ITEM_TYPES:     ds << get_db_list<Device_Item_Type>            (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_SAVE_TIMERS:           ds << get_db_list<Save_Timer>           (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_SECTIONS:              ds << get_db_list<Section>              (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_GROUPS:                ds << get_db_list<Device_item_Group>            (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_GROUP_TYPES:           ds << get_db_list<DIG_Type>      (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_GROUP_DIG_MODES:      ds << get_db_list<DIG_Mode>            (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_GROUP_DIG_PARAM_TYPES:     ds << get_db_list<DIG_Param_Type>           (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_TYPE:     ds << get_db_list<DIG_Status_Type>          (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_GROUP_DIG_STATUS_CATEGORY:     ds << get_db_list<DIG_Status_Category>          (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_DIG_PARAM:           ds << get_db_list<Dig_Param>          (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_SIGNS:                 ds << get_db_list<Sign_Item>            (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_SCRIPTS:               ds << get_db_list<Code_Item>            (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_VIEW:                  ds << get_db_list<View>                 (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_VIEW_ITEM:             ds << get_db_list<View_Item>            (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_TRANSLATIONS:          ds << get_db_list<Translation>          (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_AUTH_GROUP:            ds << get_db_list<Auth_Group>           (struct_type, db, common_db_name_, id_vect); break;
    case STRUCT_TYPE_AUTH_GROUP_PERMISSION: ds << get_db_list<Auth_Group_Permission>(struct_type, db, common_db_name_, id_vect); break;
    case STRUCT_TYPE_AUTH_USER:             ds << get_db_list<Auth_User>            (struct_type, db, common_db_name_, id_vect); break;
    case STRUCT_TYPE_AUTH_USER_GROUP:       ds << get_db_list<Auth_User_Group>      (struct_type, db, common_db_name_, id_vect); break;

    case STRUCT_TYPE_DEVICE_ITEM_VALUES:    ds << get_db_list<Device_Item_Value>    (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_DIG_MODE_ITEM:            ds << get_db_list<DIG_Mode_Item>           (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_GROUP_STATUS:          ds << get_db_list<Ver_2_1_DIG_Status>    (struct_type, db, db_name, id_vect); break;
    case STRUCT_TYPE_DIG_PARAM_VALUE:     ds << get_db_list<DIG_Param_Value>    (struct_type, db, db_name, id_vect); break;

    default:
        qCWarning(Struct_Log) << "add_structure_data unprocessed" << static_cast<StructureType>(struct_type);
        break;
    }
}

template<typename T>
QVector<QPair<uint32_t, uint16_t>> Structure_Synchronizer_Base::get_structure_hash_vect(uint8_t struct_type, Helpz::Database::Base& db, const QString& db_name)
{
    QVector<QPair<uint32_t, uint16_t>> hash_vect;

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);

    QVector<T> items = Helpz::Database::db_build_list<T>(db, get_suffix(struct_type), db_name);
    for (const T& item: items)
    {
        ds << item;

        hash_vect.push_back(qMakePair(item.id(), qChecksum(data.constData(), data.size())));
        ds.device()->seek(0);
        data.clear();
    }

    return hash_vect;
}

template<typename T>
void Structure_Synchronizer_Base::modify(QVector<T>&& upd_vect, QVector<T>&& insrt_vect, QVector<uint32_t>&& del_vect, uint32_t user_id,
                                    uint8_t struct_type, const QString& db_name, std::function<std::shared_ptr<Bad_Fix>()> get_bad_fix)
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
            bool ok = self->modify_table(struct_type, *db, upd_vect, insrt_vect, del_vect, db_name);
            if (ok)
            {
                if (!self->modified_ && self->is_main_table(struct_type))
                {
                    self->modified_ = true;
                }

                qCInfo(Struct_Log).nospace() << user_id << "|modify " << static_cast<StructureType>(struct_type) << ' '
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
                      << (upd_vect.size() + insrt_vect.size() + del_vect.size()) << " name " << db_name.toStdString() << ' ' << typeid(T).name() << std::endl;
        }
    });
}

template<class T>
bool Structure_Synchronizer_Base::modify_table(uint8_t struct_type, Helpz::Database::Base& db,
                                          const QVector<T>& update_vect,
                                          QVector<T>& insert_vect,
                                          const QVector<uint32_t>& delete_vect,
                                          const QString& db_name)
{
    Helpz::Database::Table table{ Helpz::Database::db_table<T>(db_name) };

    struct UncheckForeign
    {
        UncheckForeign(Helpz::Database::Base* p) : p_(p) { p->exec("SET foreign_key_checks=0;"); }
        ~UncheckForeign() { p_->exec("SET foreign_key_checks=1;"); }
        Helpz::Database::Base* p_;
    } uncheck_foreign(&db);

    // DELETE
    if (!Database::db_delete_rows<T>(db, delete_vect, db_name))
    {
        return false;
    }

    // UPDATE
    std::vector<const T*> items;
    QVector<uint32_t> id_vect;
    for (const T& item: update_vect)
    {
        id_vect.push_back(item.id());
        items.push_back(&item);
    }
    if (id_vect.size())
    {
        struct UpdateInfo {
            uint32_t id;
            QStringList changedFieldNames;
            QVariantList changedFields;
        };
        std::vector<UpdateInfo> update_list;

        uint32_t id_value;
        QVariant value;

        QSqlQuery q = db.select(table, get_db_list_suffix<T>(struct_type, id_vect));
        while (q.next())
        {
            id_value = q.value(0).toUInt();
            items.erase(std::remove_if(items.begin(), items.end(), [&](const T* item)
            {
                if (item->id() != id_value)
                {
                    return false;
                }

                UpdateInfo ui;
                for (int pos = 1; pos < table.field_names().size(); ++pos)
                {
                    value = T::value_getter(*item, pos);
                    if ((q.isNull(pos) ? QVariant() : q.value(pos)) != value)
                    {
                        ui.changedFieldNames.push_back(table.field_names().at(pos));
                        ui.changedFields.push_back(value);
                    }
                }

                if (ui.changedFields.size())
                {
                    ui.id = q.value(0).toUInt();
                    update_list.push_back(std::move(ui));
                }
                return true;
            }), items.end());
        }

        for (const UpdateInfo& ui: update_list)
        {
            if (!db.update({table.name(), {}, ui.changedFieldNames}, ui.changedFields, table.field_names().first() + '=' + QString::number(ui.id)))
            {
                return false;
            }
        }
    }

    // Insert if row for update in not found
    for (const T* item: items)
    {
        qCCritical(Struct_Log) << "Item:" << item->id() << "type:" << typeid(T).name() << "not updated!";
    }

    // INSERT
    QVariantList values;
    QVariant id;
    for (T& item: insert_vect)
    {
        values.clear();
        for (int pos = 0; pos < table.field_names().size(); ++pos)
            values.push_back(T::value_getter(item, pos));

        if (!db.insert(table, values, &id))
            return false;
        item.set_id(id.toUInt());
    }

    return true;
}

} // namespace Ver_2_1
} // namespace Das
