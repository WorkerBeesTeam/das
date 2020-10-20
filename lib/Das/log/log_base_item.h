#ifndef DAS_DB_LOG_BASE_ITEM_H
#define DAS_DB_LOG_BASE_ITEM_H

#include <QLoggingCategory>
#include <QDataStream>

#include <Das/db/schemed_model.h>

namespace Das {

Q_DECLARE_LOGGING_CATEGORY(Sync_Log)

namespace DB {

class Log_Base_Item : public DB::Schemed_Model
{
public:
    explicit Log_Base_Item(qint64 timestamp_msecs = 0, uint32_t user_id = 0, bool flag = false);

    static qint64 current_timestamp();
    void set_current_timestamp();

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
    uint32_t user_id_;
    qint64 timestamp_msecs_; // Milliseconds

    friend QDataStream &operator>>(QDataStream& ds, Log_Base_Item& item);
};

QDataStream &operator<<(QDataStream& ds, const Log_Base_Item& item);
QDataStream &operator>>(QDataStream& ds, Log_Base_Item& item);

} // namespace DB
} // namespace Das

#endif // DAS_DB_LOG_BASE_ITEM_H
