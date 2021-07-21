#ifndef DAS_DIG_STATUS_PROTOTYPE_H
#define DAS_DIG_STATUS_PROTOTYPE_H

#include <QObject>

#include <QtScript/QScriptable>
#include <QtScript/QScriptValue>

#include <Das/db/dig_status.h>

namespace Das {

class DIG_Status_Prototype : public QObject, public QScriptable
{
	Q_OBJECT
	Q_PROPERTY(qint64 ts READ ts WRITE set_ts)
	Q_PROPERTY(uint32_t user_id READ user_id WRITE set_user_id)
	Q_PROPERTY(uint32_t dig_id READ dig_id WRITE set_dig_id)
	Q_PROPERTY(uint32_t status_id READ status_id WRITE set_status_id)
	Q_PROPERTY(uint8_t direction READ direction WRITE set_direction)
	Q_PROPERTY(bool is_removed READ is_removed)
public:
	DIG_Status_Prototype(QObject* parent = nullptr);

	qint64 ts() const;
	Q_INVOKABLE void set_ts(qint64 value);

	uint32_t user_id() const;
	Q_INVOKABLE void set_user_id(uint32_t value);

	uint32_t dig_id() const;
	Q_INVOKABLE void set_dig_id(uint32_t value);

	uint32_t status_id() const;
	Q_INVOKABLE void set_status_id(uint32_t value);

	QStringList args() const;
	Q_INVOKABLE void set_args(const QStringList& value);

	uint8_t direction() const;
	Q_INVOKABLE void set_direction(uint8_t value);

	Q_INVOKABLE bool is_removed() const;

public slots:
	QString toString() const;

private:
	DIG_Status* self() const;
};

} // namespace Das

#endif // DAS_DIG_STATUS_PROTOTYPE_H
