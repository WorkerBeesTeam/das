#ifndef DAS_DB_LOG_EVENT_ITEM_H
#define DAS_DB_LOG_EVENT_ITEM_H

#include <Helpz/db_meta.h>

#include <Das/log/log_base_item.h>

namespace Das {
namespace DB {

class Log_Event_Item : public Log_Base_Item
{
    HELPZ_DB_META(Log_Event_Item, "log_event", "le", DB_A(timestamp_msecs), DB_AN(user_id),
                  DB_A(type_id), DB_A(category), DB_A(text), DB_A(scheme_id))
public:
    explicit Log_Event_Item(qint64 timestamp_msecs = 0, uint32_t user_id = 0, bool need_to_inform = false,
                            uint8_t type_id = 0, const QString& category = {}, const QString& text = {},
                            uint32_t scheme_id = default_scheme_id());

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

    const QString& category() const;
    void set_category(const QString &category);

    const QString& text() const;
    void set_text(const QString &text);

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

} // namespace DB
} // namespace Das

using Log_Event_Item = Das::DB::Log_Event_Item;
Q_DECLARE_METATYPE(Log_Event_Item)

#endif // DAS_DB_LOG_EVENT_ITEM_H
