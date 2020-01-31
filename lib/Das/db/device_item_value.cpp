#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "device_item_value.h"

namespace Das {
namespace Database {

Device_Item_Value::Device_Item_Value(uint32_t device_item_id, const QVariant& raw, const QVariant& display) :
    id_(0), device_item_id_(device_item_id), raw_(raw), display_(display)
{
}

uint32_t Device_Item_Value::id() const { return id_; }
void Device_Item_Value::set_id(uint32_t id) { id_ = id; }

uint32_t Device_Item_Value::device_item_id() const { return device_item_id_; }
void Device_Item_Value::set_device_item_id(uint32_t device_item_id) { device_item_id_ = device_item_id; }

QVariant Device_Item_Value::raw() const { return raw_; }
void Device_Item_Value::set_raw(const QVariant& raw) { raw_ = raw; }

QVariant Device_Item_Value::raw_to_db() const
{
    return prepare_value(raw_);
}

void Device_Item_Value::set_raw_from_db(const QVariant& value)
{
    raw_ = variant_from_string(value);
}

QVariant Device_Item_Value::display() const { return display_; }
void Device_Item_Value::set_display(const QVariant& display) { display_ = display; }

QVariant Device_Item_Value::display_to_db() const
{
    return prepare_value(display_);
}

void Device_Item_Value::set_display_from_db(const QVariant& value)
{
    display_ = variant_from_string(value);
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
    return ds >> item.device_item_id_ >> item.raw_ >> item.display_;
}

QDataStream& operator<<(QDataStream& ds, const Device_Item_Value& item)
{
    return ds << item.device_item_id() << item.raw() << item.display();
}

} // namespace Database
} // namespace Das
