#ifndef DJANGOHELPER_H
#define DJANGOHELPER_H

#include <QObject>
#include <QVariant>
#include <QUuid>

namespace Das {

class DjangoHelper : public QObject
{
    Q_OBJECT
public:
    virtual bool compare_passhash(const std::string& password, const std::string& password_hash) const;

    explicit DjangoHelper(const QString& django_manage_file_path, QObject *parent = nullptr);

signals:
public slots:
    QUuid addHouse(const QVariant &group_id, const QString &name, const QString &latin, const QString &description);
private:
    QByteArray start(const QString &command, const QStringList& arguments);
    const QString manage;
};

} // namespace Das

#endif // DJANGOHELPER_H
