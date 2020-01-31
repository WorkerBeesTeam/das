#include <QDebug>
#include <QCommandLineParser>

#include <Helpz/dtls_version.h>
#include <Helpz/net_version.h>
#include <Helpz/dtls_server.h>
#include <Helpz/dtls_server_thread.h>

#include <Das/lib.h>

#include "server.h"
#include "server_protocol.h"
#include "command_line_parser.h"

std::ostream& operator <<(std::ostream& s, const QString& str)
{
    return s << str.toStdString();
}

namespace Das {

QString getVersionString();

namespace Server {

Command_Line_Parser::Command_Line_Parser(Work_Object* work_object) :
    work_object_(work_object)
{
}

void Command_Line_Parser::process_commands(const QStringList& args)
{
    const QCommandLineOption o_list_version{ { "v", "list_version" }, QCoreApplication::translate("main", "List versions of connected schemes.")};
    const QCommandLineOption o_list{ { "l", "list" }, QCoreApplication::translate("main", "List of connected schemes.")};
    const QCommandLineOption o_id{ {"pi", "scheme_id"}, QCoreApplication::translate("main", "Scheme id."), "prj_id"};
    const QCommandLineOption o_user_id{ {"ui", "user_id"}, QCoreApplication::translate("main", "User id."), "user_id"};
    const QCommandLineOption o_devitem_id{ {"dii", "devitem_id"}, QCoreApplication::translate("main", "Device Item id."), "di_id"};
    const QCommandLineOption o_send_file{ "send_file", QCoreApplication::translate("main", "Send file to schemes."), "file_path"};
    const QCommandLineOption o_send_file_name{ "send_file_name", QCoreApplication::translate("main", "Send file name to schemes."), "file_name"};

    QList<QCommandLineOption> opt{
        o_list_version, o_list, o_id, o_user_id, o_devitem_id, o_send_file, o_send_file_name,
/*
        { { "d", "dev", "device" }, QCoreApplication::translate("main", "Device"), "ethX" },


        // A boolean option with a single name (-p)
        {"p",
            QCoreApplication::translate("main", "Show progress during copy")},
        // A boolean option with multiple names (-f, --force)
        {{"f", "force"},
            QCoreApplication::translate("main", "Overwrite existing files.")},
        // An option with a value
        {{"t", "target-directory"},
            QCoreApplication::translate("main", "Copy all source files into <directory>."),
            QCoreApplication::translate("main", "directory")},
*/
    };

    QCommandLineParser parser;
    parser.setApplicationDescription("Das Server");
    parser.addHelpOption();
    parser.addOptions(opt);

    parser.process(args);

    if (parser.isSet(o_list_version) || parser.isSet(o_list))
    {
        print_connected_schemes(parser.isSet(o_list_version));
    }
    else if (parser.isSet(o_id))
    {
        uint32_t scheme_id = parser.value(o_id).toInt();

        if (parser.isSet(o_user_id))
        {
            uint32_t user_id = parser.value(o_user_id).toUInt();

            if (parser.isSet(o_send_file) && parser.isSet(o_send_file_name) && parser.isSet(o_devitem_id))
            {
                if (work_object_->server_thread_ && work_object_->server_thread_->server())
                {
                    auto node = work_object_->server_thread_->server()->find_client([scheme_id](const Helpz::Network::Protocol* protocol) -> bool
                    {
                        return static_cast<const Protocol_Base*>(protocol)->id() == scheme_id;
                    });

                    if (node)
                    {
                        auto proto = std::static_pointer_cast<Protocol_Base>(node->protocol());
                        if (proto)
                        {
                            uint32_t dev_item_id = parser.value(o_devitem_id).toUInt();
                            QString file_name = parser.value(o_send_file_name);
                            QString file_path = parser.value(o_send_file);

                            proto->send_file(user_id, dev_item_id, file_name, file_path);
                        }
                    }
                    else
                    {
                        std::cout << "Command file: Scheme id not found " << scheme_id << std::endl;
                    }
                }
                else
                {
                    std::cout << "Command file: Server not started" << std::endl;
                }
            }
            else if (true) // not file
            {
                std::cout << "Command not file" << std::endl;
            }
        }
        else // without user
        {
            std::cout << "Command withot user_id" << std::endl;
        }
    }
    else
    {
        std::cout << "Your arguments ";
        std::copy(args.begin(), args.end(),
                      std::ostream_iterator<QString>(std::cout, " "));
        std::cout << std::endl << parser.helpText().toStdString() << std::endl;
    }
}

void Command_Line_Parser::print_connected_schemes(bool with_version) const
{
    if (!work_object_->server_thread_ || !work_object_->server_thread_->server())
    {
        std::cerr << "Server not started" << std::endl;
        return;
    }

    {
        std::cout << "List of connected schemes: ";
        if (with_version)
            std::cout   << "[Server " << getVersionString()
                        << "][Lib " << Lib::ver_str()
                        << "][DTLS " << Helpz::DTLS::ver_str()
                        << "][Net " << Helpz::Network::ver_str() << ']';
        std::cout << std::endl;
    }

    int n = 0;
    work_object_->server_thread_->server()->find_client([with_version, &n](const Helpz::Network::Protocol* protocol) -> bool
    {
        auto proto = static_cast<const Protocol_Base*>(protocol);
        auto node = std::static_pointer_cast<const Helpz::DTLS::Node>(protocol->writer());
        if (!protocol || !node)
            return false;

        std::cout << ++n << " | " << node->title() << ' ' << proto->id();

        if (with_version)
            std::cout << ' ' << proto->version();
        std::cout << std::endl;

        return false;
    });
}

} // namespace Server
} // namespace Das
