#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "device_item_value.h"

namespace Das {
namespace DB {

Device_Item_Value_Base::Device_Item_Value_Base(qint64 timestamp_msecs, uint32_t user_id, uint32_t device_item_id, const QVariant& raw, const QVariant& display, bool flag) :
    Log_Base_Item(timestamp_msecs, user_id, flag), item_id_(device_item_id), raw_value_(raw), value_(display)
{
}

uint32_t Device_Item_Value_Base::item_id() const { return item_id_; }
void Device_Item_Value_Base::set_item_id(uint32_t device_item_id) { item_id_ = device_item_id; }

const QVariant &Device_Item_Value_Base::raw_value() const { return raw_value_; }
void Device_Item_Value_Base::set_raw_value(const QVariant& raw) { raw_value_ = raw; }

QVariant Device_Item_Value_Base::raw_value_to_db() const
{
    return prepare_value(raw_value_);
}

void Device_Item_Value_Base::set_raw_value_from_db(const QVariant& value)
{
    raw_value_ = variant_from_string(value);
}

const QVariant &Device_Item_Value_Base::value() const { return value_; }
void Device_Item_Value_Base::set_value(const QVariant& display) { value_ = display; }

QVariant Device_Item_Value_Base::value_to_db() const
{
    return prepare_value(value_);
}

void Device_Item_Value_Base::set_value_from_db(const QVariant& value)
{
    value_ = variant_from_string(value);
}

/*static*/ QVariant Device_Item_Value_Base::prepare_value(const QVariant &var)
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
        auto is_printable = [](const QString& text)
        {
            // TODO: learn about how to detect non-printable chars and
            // if is detected, then convert to base64.
            // https://github.com/BurninLeo/see-non-printable-characters/blob/main/view-chars.php
            (void)text;
            return true;
        };
        const QByteArray data = var.toByteArray();
        if (!is_printable(data))
            return "base64:" + data.toBase64();
    }
    return var;
//    return var.toString();
}

/*static*/ QVariant Device_Item_Value_Base::variant_from_string(const QVariant &var)
{
    if (!var.isValid() || (var.type() != QVariant::String && var.type() != QVariant::ByteArray))
        return var;

    QString text = var.toString();
    if (text.isEmpty())
        return QVariant();

    if (text.length() > 2)
    {
        if (text.length() == 4 && text.toLower() == "true")
            return true;
        else if (text.length() == 5 && text.toLower() == "false")
            return false;
        else if ((text.at(0) == '[' && text.at(text.length() - 1) == ']') || (text.at(0) == '{' && text.at(text.length() - 1) == '}'))
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

    return text;
}

bool Device_Item_Value_Base::is_big_value() const
{
    return is_big_value(raw_value_) || is_big_value(value_);
}

bool Device_Item_Value_Base::is_big_value(const QVariant &val)
{
    if (val.data_ptr().type == QVariant::ByteArray)
        return static_cast<const QByteArray*>(val.constData())->size() > 8192;
    else if (val.data_ptr().type == QVariant::String)
        return static_cast<const QString*>(val.constData())->size() > 8192;
    return false;
}

QDataStream& operator>>(QDataStream& ds, Device_Item_Value_Base& item)
{
    return ds >> static_cast<Log_Base_Item&>(item) >> item.item_id_ >> item.raw_value_ >> item.value_;
}

QDataStream& operator<<(QDataStream& ds, const Device_Item_Value_Base& item)
{
    return ds << static_cast<const Log_Base_Item&>(item) << item.item_id() << item.raw_value() << item.value();
}

} // namespace DB
} // namespace Das
