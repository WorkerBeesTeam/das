#define PICOJSON_USE_INT64
#include <picojson/picojson.h>

#include <QtScript/QScriptEngine>

#include "dig_status_prototype.h"

namespace Das {

DIG_Status_Prototype::DIG_Status_Prototype(QObject* parent) :
	QObject(parent)
{
}

DIG_Status* DIG_Status_Prototype::self() const
{
	return qscriptvalue_cast<DIG_Status*>(thisObject().data());
}

qint64 DIG_Status_Prototype::ts() const { return self()->timestamp_msecs(); }
void DIG_Status_Prototype::set_ts(qint64 value) { self()->set_timestamp_msecs(value); }

uint32_t DIG_Status_Prototype::user_id() const { return self()->user_id(); }
void DIG_Status_Prototype::set_user_id(uint32_t value) { self()->set_user_id(value); }

uint32_t DIG_Status_Prototype::dig_id() const { return self()->group_id(); }
void DIG_Status_Prototype::set_dig_id(uint32_t value) { self()->set_group_id(value); }

uint32_t DIG_Status_Prototype::status_id() const { return self()->status_id(); }
void DIG_Status_Prototype::set_status_id(uint32_t value) { self()->set_status_id(value); }

QStringList DIG_Status_Prototype::args() const { return self()->args(); }
void DIG_Status_Prototype::set_args(const QStringList& value) { self()->set_args(value); }

uint8_t DIG_Status_Prototype::direction() const { return self()->direction(); }
void DIG_Status_Prototype::set_direction(uint8_t value) { self()->set_direction(value); }

bool DIG_Status_Prototype::is_removed() const { return self()->is_removed(); }

QString DIG_Status_Prototype::toString() const
{
	picojson::object obj;
	obj.emplace("ts",			static_cast<int64_t>(ts()));
	obj.emplace("user_id",		static_cast<int64_t>(user_id()));
	obj.emplace("dig_id",		static_cast<int64_t>(dig_id()));
	obj.emplace("status_id",	static_cast<int64_t>(status_id()));
	obj.emplace("is_removed", is_removed());
	return QString::fromStdString(picojson::value(std::move(obj)).serialize());
}

} // namespace Das
