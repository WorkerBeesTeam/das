#ifndef DAS_DIG_STATUS_CLASS_H
#define DAS_DIG_STATUS_CLASS_H

#include <QObject>
#include <QtScript/QScriptClass>

#include <Das/db/dig_status.h>

QT_BEGIN_NAMESPACE
class QScriptContext;
QT_END_NAMESPACE

namespace Das {

class DIG_Status_Class : public QObject, public QScriptClass
{
	Q_OBJECT
public:
	DIG_Status_Class(QScriptEngine *engine);

	QScriptValue new_instance(qint64 timestamp_msecs = 0, uint32_t user_id = 0, uint32_t group_id = 0,
							  uint32_t status_id = 0, const QStringList& args = QStringList(), DIG_Status::Status_Direction direction = DIG_Status::SD_ADD);
	QScriptValue new_instance(const DIG_Status &status);
	static QScriptValue new_instance(DIG_Status_Class *dig_status_class, const DIG_Status& status);

	QueryFlags queryProperty(const QScriptValue &object,
							 const QScriptString &name,
							 QueryFlags flags, uint *id) override;

	QScriptValue property(const QScriptValue &object,
						  const QScriptString &name, uint id) override;

	void setProperty(QScriptValue &object, const QScriptString &name,
					 uint id, const QScriptValue &value) override;

	QScriptValue::PropertyFlags propertyFlags(
		const QScriptValue &object, const QScriptString &name, uint id) override;

	QScriptClassPropertyIterator *newIterator(const QScriptValue &object) override;

	QString name() const override;
	QScriptValue prototype() const override;

	QScriptValue constructor();

private:
	static QScriptValue construct(QScriptContext *ctx, QScriptEngine *);

	static QScriptValue toScriptValue(QScriptEngine *eng, const DIG_Status &status);
	static void fromScriptValue(const QScriptValue &obj, DIG_Status& status);

	QScriptValue _proto, _f_ctor;
};

} // namespace Das

#endif // DAS_DIG_STATUS_CLASS_H
