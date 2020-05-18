#ifndef DAS_COMMAND_LINE_PARSER_H
#define DAS_COMMAND_LINE_PARSER_H

#include <QObject>

namespace Das {
namespace Server {

class Worker;
class Command_Line_Parser : public QObject
{
    Q_OBJECT
public:
    Command_Line_Parser(Worker* work_object);

    void process_commands(const QStringList &args);
private:
    void print_connected_schemes(bool with_version) const;

    Worker* work_object_;
};

} // namespace Server
} // namespace Das

#endif // DAS_COMMAND_LINE_PARSER_H
