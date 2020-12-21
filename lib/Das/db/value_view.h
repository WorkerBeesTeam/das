#ifndef DAS_DB_VALUE_VIEW_H
#define DAS_DB_VALUE_VIEW_H

#include <Helpz/db_meta.h>

#include <Das/db/schemed_model.h>

namespace Das {
namespace DB {

class Value_View : public Schemed_Model
{
    HELPZ_DB_META(Value_View, "value_view", "vv", DB_A(item_type), DB_AT(value), DB_AT(view), DB_A(scheme_id))
public:
    Value_View(uint32_t item_type = 0, const QVariant& value = {}, const QVariant& view = {});

    uint32_t item_type() const;
    void set_item_type(uint32_t item_type);

    QVariant value() const;
    void set_value(const QVariant& value);
    QVariant value_to_db() const;
    void set_value_from_db(const QVariant& value);

    QVariant view() const;
    void set_view(const QVariant& view);
    QVariant view_to_db() const;
    void set_view_from_db(const QVariant& view);

private:
    uint32_t _item_type;
    QVariant _value, _view;

    friend QDataStream &operator>>(QDataStream &ds, Value_View& item);
};

QDataStream &operator>>(QDataStream &ds, Value_View& item);
QDataStream &operator<<(QDataStream &ds, const Value_View& item);

} // namespace DB
} // namespace Das

#endif // DAS_DB_VALUE_VIEW_H
