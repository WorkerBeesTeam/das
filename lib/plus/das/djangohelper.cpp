#include <iostream>
#include <botan-2/botan/parsing.h>
#include <botan-2/botan/pbkdf2.h>
#include <botan-2/botan/base64.h>

#include <QDebug>
#include <QProcess>
#include <QDir>
#include <QRegularExpression>

#include <Helpz/settingshelper.h>

#include "djangohelper.h"

namespace Das {

bool DjangoHelper::compare_passhash(const std::string& password, const std::string& password_hash) const
{
    std::vector<std::string> data_list = Botan::split_on(password_hash, '$');
    if (data_list.size() != 4 || data_list.at(0) != "pbkdf2_sha256")
    {
        std::cerr << "Unknown algoritm: " << data_list.at(0) << std::endl;
        return false;
    }
    auto pbkdf_prf = Botan::MessageAuthenticationCode::create("HMAC(SHA-256)");
    Botan::PKCS5_PBKDF2 kdf(pbkdf_prf.release()); // takes ownership of pointer

    std::size_t kdf_iterations = std::stoul(data_list.at(1));
    const size_t PASSHASH9_PBKDF_OUTPUT_LEN = 32; // 256 bits output
    const std::string& salt = data_list.at(2);
    Botan::secure_vector<uint8_t>
            cmp = kdf.derive_key(PASSHASH9_PBKDF_OUTPUT_LEN, password,
                                 reinterpret_cast<const uint8_t*>(salt.data()), salt.size(),
                                 kdf_iterations).bits_of(),
            hash = Botan::base64_decode(data_list.at(3));

    return Botan::constant_time_compare(cmp.data(), hash.data(), std::min(cmp.size(), hash.size()));
}

DjangoHelper::DjangoHelper(const QString& django_manage_file_path, QObject *parent) :
    QObject(parent),
    manage(django_manage_file_path)
{
    std::cout << "Django helper inited" << std::endl;
}

QUuid DjangoHelper::addHouse(const QVariant& group_id, const QString& name, const QString& latin, const QString& description)
{
    if (name.isEmpty() || latin.isEmpty() || !group_id.isValid())
        return {};

    QString n_name = name, n_latin = latin, n_desc = description;
    n_name =  '"' + n_name.replace('"', '\'') + '"';
    n_latin = '"' + n_latin.replace('"', '\'') + '"';
    n_desc = '"' + n_desc.replace('"', '\'') + '"';
    // FIXIT: Security fail, not realy escaped text
    QByteArray out = start("adddas", { n_name, n_latin,
                     "--description", n_desc,
                     "--group-id", group_id.toString() });

    if (!out.size())
        return {};

    QRegularExpression re("Device ([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})");
    auto match = re.match(out);
    if (!match.hasMatch())
        return {};

    return QUuid(match.captured(1));
}

QByteArray DjangoHelper::start(const QString& command,  const QStringList &arguments)
{
    QProcess proc;
    proc.setProgram("python3");
    proc.setWorkingDirectory(QFileInfo(manage).absolutePath());
//    proc.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args{ manage, command };
    args << arguments;
    proc.setArguments(args);
    proc.start();
    if (!proc.waitForStarted(2500) || !proc.waitForFinished(2500))
    {
        proc.terminate();
        qWarning() << "Lost django run:" << command << arguments;
    }

    if (proc.exitCode() != 0)
        qWarning() << proc.exitCode() << proc.readAllStandardError().constData();
    return proc.readAllStandardOutput();
}

} // namespace Das
