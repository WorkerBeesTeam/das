#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "device_item_value.h"

namespace Das {
namespace DB {

Device_Item_Value::Device_Item_Value(qint64 timestamp_msecs, uint32_t user_id, uint32_t device_item_id, const QVariant& raw, const QVariant& display, bool flag) :
    Log_Base_Item(0, timestamp_msecs, user_id, flag), item_id_(device_item_id), raw_value_(raw), value_(display)
{
}

uint32_t Device_Item_Value::item_id() const { return item_id_; }
void Device_Item_Value::set_item_id(uint32_t device_item_id) { item_id_ = device_item_id; }

const QVariant &Device_Item_Value::raw_value() const { return raw_value_; }
void Device_Item_Value::set_raw_value(const QVariant& raw) { raw_value_ = raw; }

QVariant Device_Item_Value::raw_value_to_db() const
{
    return prepare_value(raw_value_);
}

void Device_Item_Value::set_raw_value_from_db(const QVariant& value)
{
    raw_value_ = variant_from_string(value);
}

const QVariant &Device_Item_Value::value() const { return value_; }
void Device_Item_Value::set_value(const QVariant& display) { value_ = display; }

QVariant Device_Item_Value::value_to_db() const
{
    return prepare_value(value_);
}

void Device_Item_Value::set_value_from_db(const QVariant& value)
{
    value_ = variant_from_string(value);
}

/*static*/ QVariant Device_Item_Value::prepare_value(const QVariant &var)
{
    if (!var.isValid())
        return var;
    else if (var.type() == QVariant::Double)
    {
        double val = var.toDouble();
        if (val != val)
            return "NaN";
    }
    else if (var.type() == QVariant::List || var.type() == QVariant::Map)
        return QJsonDocument::fromVariant(var).toJson(QJsonDocument::Compact);
    else if (var.type() == QVariant::ByteArray)
    {
//        const QByteArray data = var.toByteArray();
//        if (data.startsWith("base64:"))
//            return "base64:" + data.toBase64();
    }
    return var;
//    return var.toString();
}

/*static*/ QVariant Device_Item_Value::variant_from_string(const QVariant &var)
{
    if (!var.isValid() || var.type() != QVariant::String)
        return var;

    QString text = var.toString();
    if (text.isEmpty())
        return QVariant();

    if (text.length() > 2)
    {
        if ((text.at(0) == '[' && text.at(text.length() - 1) == ']') || (text.at(0) == '{' && text.at(text.length() - 1) == '}'))
        {
            QJsonParseError parse_error;
            parse_error.error = QJsonParseError::NoError;
            QJsonDocument doc = QJsonDocument::fromJson(var.toByteArray(), &parse_error);
            if (parse_error.error == QJsonParseError::NoError)
            {
                if (doc.isArray())
                    return doc.array().toVariantList();
                if (doc.isObject())
                    return doc.object().toVariantMap();
            }
            else
                return text;
        }
        else if (text.startsWith("base64:"))
        {
            text.remove(0, 7);
            return QByteArray::fromBase64(text.toLocal8Bit());
        }
    }

    bool ok = false;
    int int_val = text.toInt(&ok);
    if (ok && text == QString::number(int_val))
        return int_val;

    double dbl_val = text.toDouble(&ok);
    if (ok)
        return dbl_val;

    if (text.indexOf('.') != -1 || text.indexOf(',') != -1) {
        QString t = text;
        t = t.indexOf('.') != -1 ? t.replace('.', ',') : t.replace(',', '.');
        dbl_val = t.toDouble(&ok);
        if (ok)
            return dbl_val;
    }

    if (text.toLower() == "true")
        return true;
    else if (text.toLower() == "false")
        return false;

    return text;
}

QDataStream& operator>>(QDataStream& ds, Device_Item_Value& item)
{
    return ds >> static_cast<Log_Base_Item&>(item) >> item.item_id_ >> item.raw_value_ >> item.value_;
}

QDataStream& operator<<(QDataStream& ds, const Device_Item_Value& item)
{
    return ds << static_cast<const Log_Base_Item&>(item) << item.item_id() << item.raw_value() << item.value();
}

} // namespace DB
} // namespace Das
