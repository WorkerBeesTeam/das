#ifndef DAS_COMMAND_LINE_PARSER_H
#define DAS_COMMAND_LINE_PARSER_H

#include <QObject>

#include "work_object.h"

namespace Das {
namespace Server {

class Command_Line_Parser : public QObject
{
    Q_OBJECT
public:
    Command_Line_Parser(Work_Object* work_object);

    void process_commands(const QStringList &args);
private:
    void print_connected_schemes(bool with_version) const;

    Work_Object* work_object_;
};

} // namespace Server
} // namespace Das

#endif // DAS_COMMAND_LINE_PARSER_H
