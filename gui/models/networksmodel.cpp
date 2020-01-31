
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

#include "networksmodel.h"

namespace Das {
namespace Gui {

NetworksModel::NetworksModel()
{
    m_items.clear();
    for (QNetworkInterface& it: QNetworkInterface::allInterfaces())
        if ((it.flags() & QNetworkInterface::IsLoopBack) == 0 &&
                it.addressEntries().size())
            m_items.push_back(it);
}

QVariant NetworksModel::data(const QModelIndex &index, int role) const
{
    if (role == NameRole)
        return m_items.at(index.row()).humanReadableName();
    else if (role == ValueRole)
    {
        auto adrs = m_items.at(index.row()).addressEntries();
        if (adrs.size())
            return adrs.first().ip().toString();
    }
    return QVariant();
}

// --------- WiFiModel ----------

QString getWiFiInterface()
{
    QFile file("/proc/net/dev");
    if (file.open(QIODevice::ReadOnly))
    {
        int i;
        auto lines = file.readAll().split('\n');
        for (const QByteArray& line: lines)
        {
            i = 0;
            while(i < line.length() &&
                  line.at(i) == ' ') ++i;
            if (i < line.length() && line.at(i) == 'w')
                return line.mid(i, line.indexOf(':') - i);
        }
    }
    return QString();
}

QList<QString> wirelessList()
{
    QList<QString> wifis;

    QString dev_name = getWiFiInterface();
    if (dev_name.length())
    {
        QProcess p;
        p.start("iwlist", QStringList() << dev_name << "scan");
        p.waitForFinished();
        auto data = p.readAll();

        QRegularExpression re("Encryption key:(\\w+)\\n\\s+ESSID:\"(.+)\"");
        QRegularExpressionMatchIterator i = re.globalMatch(data);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            wifis << match.captured(2);
        }

        if (!wifis.length())
            wifis << "SSIDs not found" << QString::fromLocal8Bit(data).split('\n') << p.errorString().split('\n');
    }
    else
        wifis << "Wireless device not found";
    return wifis;
}

WiFiModel::WiFiModel()
{
    m_items = wirelessList();
}

QVariant WiFiModel::data(const QModelIndex &index, int role) const
{
    if (role == NameRole)
        return m_items.at(index.row());
    return QVariant();
}

} // namespace Gui
} // namespace Das
