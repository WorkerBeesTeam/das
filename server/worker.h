#ifndef DAS_SERVER_WORKER_H
#define DAS_SERVER_WORKER_H

#include <QTimer>

#include <Helpz/service.h>
#include <Helpz/settingshelper.h>
#include <Helpz/db_thread.h>

#include "command_line_parser.h"
#include "server.h"
#include "work_object.h"

namespace Das {
namespace Server {

class Worker : public QObject, public Server::Work_Object
{
    Q_OBJECT

public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

private:
    void init_logging(QSettings* s);
public:
signals:
    void processCommands(const QStringList &args);
public slots:
private slots:
    void on_timer();
private:
    Command_Line_Parser cl_parser_;

    QTimer timer_;
};

typedef Helpz::Service::Impl<Worker> Service;

} // namespace Server
} // namespace Das

#endif // DAS_SERVER_WORKER_H
