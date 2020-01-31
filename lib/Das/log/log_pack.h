#ifndef DAS_LOG_PACK_H
#define DAS_LOG_PACK_H

#include <QString>
#include <QVariant>
#include <QDataStream>
#include <QLoggingCategory>

#include <Helpz/db_meta.h>

#include <Das/db/schemed_model.h>

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(Sync_Log)

class Log_Base_Item : public Database::Schemed_Model
{
public:
    Log_Base_Item(uint32_t id, qint64 timestamp_msecs = 0, uint32_t user_id = 0, bool flag = false);
    Log_Base_Item(Log_Base_Item&&) = default;
    Log_Base_Item(const Log_Base_Item&) = default;
    Log_Base_Item& operator =(Log_Base_Item&&) = default;
    Log_Base_Item& operator =(const Log_Base_Item&) = default;

    void set_current_timestamp();

    uint32_t id() const;
    void set_id(uint32_t id);

    qint64 timestamp_msecs() const;
    void set_timestamp_msecs(qint64 timestamp_msecs);

    uint32_t user_id() const;
    void set_user_id(uint32_t user_id);

    bool flag() const;
    void set_flag(bool flag);

    enum Log_Base_Flags : qint64
    {
        LOG_FLAG = 0x80000000000000
    };

protected:
    bool flag_;
private:
    uint32_t id_, user_id_;
    qint64 timestamp_msecs_; // Milliseconds

    friend QDataStream &operator>>(QDataStream& ds, Log_Base_Item& item);
};

QDataStream &operator<<(QDataStream& ds, const Log_Base_Item& item);
QDataStream &operator>>(QDataStream& ds, Log_Base_Item& item);

class Log_Value_Item : public Log_Base_Item
{
    HELPZ_DB_META(Log_Value_Item, "log_data", "hld", 7, DB_AN(id), DB_A(timestamp_msecs), DB_AN(user_id),
                  DB_A(item_id), DB_A(raw_value), DB_A(value), DB_A(scheme_id))
public:
    Log_Value_Item(qint64 timestamp_msecs = 0, uint32_t user_id = 0, bool need_to_save = false, uint32_t item_id = 0,
                   const QVariant& raw_value = {}, const QVariant& value = {});
    Log_Value_Item(Log_Value_Item&&) = default;
    Log_Value_Item(const Log_Value_Item&) = default;
    Log_Value_Item& operator =(Log_Value_Item&&) = default;
    Log_Value_Item& operator =(const Log_Value_Item&) = default;

    uint32_t item_id() const;
    void set_item_id(uint32_t item_id);

    QVariant value() const;
    void set_value(const QVariant& value);

    QVariant raw_value() const;
    void set_raw_value(const QVariant& raw_value);

    bool need_to_save() const;
    void set_need_to_save(bool state);
private:
    uint32_t item_id_;
    QVariant raw_value_, value_;

    friend QDataStream &operator>>(QDataStream& ds, Log_Value_Item& item);
};

QDataStream &operator<<(QDataStream& ds, const Log_Value_Item& item);
QDataStream &operator>>(QDataStream& ds, Log_Value_Item& item);

class Log_Event_Item : public Log_Base_Item
{
    HELPZ_DB_META(Log_Event_Item, "log_event", "hle", 7, DB_AN(id), DB_A(timestamp_msecs), DB_AN(user_id),
                  DB_A(type_id), DB_A(category), DB_A(text), DB_A(scheme_id))
public:
    Log_Event_Item(qint64 timestamp_msecs = 0, uint32_t user_id = 0, bool need_to_inform = false, uint8_t type_id = 0,
                   const QString& category = {}, const QString& text = {});
    Log_Event_Item(Log_Event_Item&&) = default;
    Log_Event_Item(const Log_Event_Item&) = default;
    Log_Event_Item& operator =(Log_Event_Item&&) = default;
    Log_Event_Item& operator =(const Log_Event_Item&) = default;

    enum Event_Types {
        ET_DEBUG = 0,
        ET_WARNING,
        ET_CRITICAL,
        ET_FATAL,
        ET_INFO,
        ET_USER
    };

    uint8_t type_id() const;
    void set_type_id(uint8_t type_id);

    QString category() const;
    void set_category(QString category);

    QString text() const;
    void set_text(QString text);

    bool need_to_inform() const;
    void set_need_to_inform(bool state);

    QString toString() const;
private:
    uint8_t type_id_;
    QString category_, text_;

    friend QDataStream &operator>>(QDataStream& ds, Log_Event_Item& item);
};

QDataStream &operator<<(QDataStream& ds, const Log_Event_Item& item);
QDataStream &operator>>(QDataStream& ds, Log_Event_Item& item);

} // namespace Das

using Log_Value_Item = Das::Log_Value_Item;
using Log_Event_Item = Das::Log_Event_Item;

Q_DECLARE_METATYPE(Log_Value_Item)
Q_DECLARE_METATYPE(Log_Event_Item)

#endif // DAS_LOG_PACK_H
