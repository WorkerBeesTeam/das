#ifndef PARAMGROUPPROTOTYPE_H
#define PARAMGROUPPROTOTYPE_H

#include <QObject>

#include <QtScript/QScriptable>
#include <QtScript/QScriptValue>

#include "Das/param/paramgroup.h"

namespace Das {

class ParamGroupClass;
class ParamGroupPrototype : public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(uint32_t type READ type)
    Q_PROPERTY(int value_type READ value_type)
    Q_PROPERTY(int length READ length)
    Q_PROPERTY(QScriptValue value READ value WRITE set_value)
public:
    explicit ParamGroupPrototype(ParamGroupClass *parent = 0);

    QString title() const;

    int value_type() const;
    uint32_t type() const;
    QString name() const;
    int length() const;

    QScriptValue value() const;

public slots:
    void set_value(const QScriptValue& value) const;

    QScriptValue byTypeId(uint32_t DIG_param_type) const;

    QScriptValue toJSON() const;
    QString toString() const;
    QScriptValue valueOf() const;
private:
    Param *thisParam() const;
    static Param empty_param;

    ParamGroupClass *pgClass;
};

} // namespace Das

#endif // PARAMGROUPPROTOTYPE_H
