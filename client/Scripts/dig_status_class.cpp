#include <QtScript/QScriptClassPropertyIterator>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

#include "dig_status_prototype.h"
#include "dig_status_class.h"

namespace Das {

DIG_Status_Class::DIG_Status_Class(QScriptEngine* engine) :
	QObject(engine), QScriptClass(engine)
{
	qScriptRegisterMetaType<DIG_Status>(engine, toScriptValue, fromScriptValue);

	_proto = engine->newQObject(new DIG_Status_Prototype(this),
							   QScriptEngine::QtOwnership,
							   QScriptEngine::SkipMethodsInEnumeration
							   | QScriptEngine::ExcludeSuperClassMethods
							   | QScriptEngine::ExcludeSuperClassProperties);
	QScriptValue global = engine->globalObject();
	_proto.setPrototype(global.property("Object").property("prototype"));

	//engine->setDefaultPrototype(qMetaTypeId<Param*>(), proto);

	_f_ctor = engine->newFunction(construct, _proto);
	_f_ctor.setData(engine->toScriptValue(this));
}

QScriptValue DIG_Status_Class::new_instance(qint64 timestamp_msecs, uint32_t user_id, uint32_t group_id, uint32_t status_id,
											const QStringList& args, DIG_Status::Status_Direction direction)
{
	return new_instance(DIG_Status(timestamp_msecs, user_id, group_id, status_id, args, direction));
}

QScriptValue DIG_Status_Class::new_instance(const DIG_Status& status)
{
	return new_instance(this, status);
}

/*static*/ QScriptValue DIG_Status_Class::new_instance(DIG_Status_Class* dig_status_class, const DIG_Status& status)
{
	QScriptValue data = dig_status_class->engine()->newVariant(QVariant::fromValue(status));
	return dig_status_class->engine()->newObject(dig_status_class, data);
}

QScriptClass::QueryFlags DIG_Status_Class::queryProperty(const QScriptValue& object, const QScriptString& name, QueryFlags flags, uint* id)
{
	return QScriptClass::queryProperty(object, name, flags, id);
}

QScriptValue DIG_Status_Class::property(const QScriptValue& object, const QScriptString& name, uint id)
{
	return QScriptClass::property(object, name, id);
}

void DIG_Status_Class::setProperty(QScriptValue& object, const QScriptString& name, uint id, const QScriptValue& value)
{
	return QScriptClass::setProperty(object, name, id, value);
}

QScriptValue::PropertyFlags DIG_Status_Class::propertyFlags(const QScriptValue& object, const QScriptString& name, uint id)
{
	return QScriptClass::propertyFlags(object, name, id);
}

QScriptClassPropertyIterator* DIG_Status_Class::newIterator(const QScriptValue& object)
{
	return QScriptClass::newIterator(object);
}

QString DIG_Status_Class::name() const { return QLatin1String("DIG_Status"); }
QScriptValue DIG_Status_Class::prototype() const { return _proto; }
QScriptValue DIG_Status_Class::constructor() { return _f_ctor; }

/*static*/ QScriptValue DIG_Status_Class::construct(QScriptContext* ctx, QScriptEngine*)
{
	DIG_Status_Class *cls = qscriptvalue_cast<DIG_Status_Class*>(ctx->callee().data());
	if (!cls)
		return QScriptValue();
	QScriptValue arg = ctx->argument(0);
	if (arg.instanceOf(ctx->callee()))
		return cls->new_instance(qscriptvalue_cast<DIG_Status>(arg));

	qint64 timestamp_msecs = ctx->argumentCount() > 0 ? arg.toNumber() : 0;
	uint32_t user_id = ctx->argumentCount() > 1 ? ctx->argument(1).toUInt32() : 0;
	uint32_t group_id = ctx->argumentCount() > 2 ? ctx->argument(2).toUInt32() : 0;
	uint32_t status_id = ctx->argumentCount() > 3 ? ctx->argument(3).toUInt32() : 0;
	QStringList args = ctx->argumentCount() > 4 ? ctx->argument(4).toVariant().toStringList() : QStringList();
	DIG_Status::Status_Direction direction = ctx->argumentCount() > 5 ?
				static_cast<DIG_Status::Status_Direction>(ctx->argument(5).toUInt32()) : DIG_Status::SD_ADD;

	return cls->new_instance(timestamp_msecs, user_id, group_id, status_id, args, direction);
}

/*static*/ QScriptValue DIG_Status_Class::toScriptValue(QScriptEngine* eng, const DIG_Status& status)
{
	QScriptValue ctor = eng->globalObject().property("DIG_Status");
	DIG_Status_Class *cls = qscriptvalue_cast<DIG_Status_Class*>(ctor.data());
	if (!cls)
		return eng->newVariant(QVariant::fromValue(status));
	return cls->new_instance(status);
}

/*static*/ void DIG_Status_Class::fromScriptValue(const QScriptValue& obj, DIG_Status& status)
{
	status = qvariant_cast<DIG_Status>(obj.data().toVariant());
}

} // namespace Das
