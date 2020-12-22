
#include <filesystem>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iwlib.h>
#define KILO	1e3

#include <arpa/inet.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include <Helpz/zfile.h>

#include "system_informator.h"

namespace Das {
namespace Router {

namespace fs = std::filesystem;
using namespace std;
using namespace Helpz;

System_Informator::System_Informator()
{
    _last_proc_stat = get_proc_stat();
}

double System_Informator::get_cpu_temp() const
{
    const string file_name = "/sys/class/thermal/thermal_zone0/temp";
    const string text = File::read_all(file_name);
    int temp = stoi(text) / 100;
    return temp / 10.;
}

int System_Informator::get_cpu_load()
{
    pair<size_t, size_t> proc_stat = get_proc_stat();
    int utilization = (proc_stat.first - _last_proc_stat.first) * 100 / (proc_stat.second - _last_proc_stat.second);
    _last_proc_stat = proc_stat;
    return utilization;
}

pair<size_t, size_t> System_Informator::get_proc_stat()
{
    const string text = File::read_all("/proc/stat", 64);
    if (text.size() > 30)
    {
        vector<string> time_vect;
        const string first_line = text.substr(5, text.find('\n') - 5);
        boost::split(time_vect, first_line, [](char c){ return c == ' '; });

        if (time_vect.size() >= 4)
        {
            size_t first = stoul(time_vect.at(0)) + stoul(time_vect.at(2));
            size_t second = first + stoul(time_vect.at(3));
            return make_pair(first, second);
        }
    }

    return pair<size_t, size_t>{0, 0};
}

int System_Informator::get_mem_usage() const
{
    const string text = File::read_all("/proc/meminfo", 200);

    vector<string> lines, parts;
    boost::split(lines, text, [](char c){ return c == '\n'; });

    if (lines.size() >= 5)
    {
        auto get_line_value = [&lines, &parts](int index)
        {
            parts.clear();
            boost::split(parts, lines.at(index), boost::is_space());
            return parts.size() > 2 ? stoul(parts.at(parts.size() - 2)) : 0;
        };

        const size_t total = get_line_value(0);
        const size_t total_used = total - get_line_value(1);
        const size_t non_cached_used = total_used - (get_line_value(3) + get_line_value(4));
        return non_cached_used / (total / 100.f);
    }

    return -1;
}

int System_Informator::get_disk_space_usage() const
{
    fs::space_info tmp = fs::space(".");
    size_t used = tmp.capacity - tmp.available;
    return used / (tmp.capacity / 100.);
}

vector<string> System_Informator::get_net_info() const
{
    string text = File::read_all("/proc/net/route", 1024);

    vector<string> lines;
    boost::split(lines, text, [](char c){ return c == '\n'; });

    long destination, gateway;
    char iface[IF_NAMESIZE];

    size_t if_name_len;
    int	skfd = iw_sockets_open();
    int channel;

    vector<string> net_vect;

    for (const std::string& line: lines)
    {
        if (sscanf(line.c_str(), "%s %lx %lx", iface, &destination, &gateway) == 3)
        {
            if (destination == 0) /* default */
            {
                text.clear();

                struct ifreq ifr;
                if_name_len = strlen(iface);
                if (if_name_len < sizeof(ifr.ifr_name))
                {
                    memcpy(ifr.ifr_name, iface, if_name_len);
                    ifr.ifr_name[if_name_len] = 0;

                    if (ioctl(skfd, SIOCGIFADDR, &ifr) != -1)
                    {
                        struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
                        text = inet_ntoa(ipaddr->sin_addr);
                    }
                }

                if (text.empty())
                    text = inet_ntoa(*(struct in_addr *) &gateway);
                text += ' ';
                text += iface;

                channel = get_channel(skfd, iface);
                if (channel >= 0)
                {
                    text += ' ';
                    text += get_essid(skfd, iface);
                    text += ' ';
                    text += to_string(get_signal(skfd, iface));
                    text += "dBm (";
                    text += to_string(channel);
                    text += ')';
                }

                net_vect.push_back(text);
            }
        }
    }

    if (skfd >= 0)
        iw_sockets_close(skfd);
    return net_vect;
}

std::string System_Informator::get_essid(int skfd, const char * ifname) const
{
    struct iwreq wrq;
    char essid[IW_ESSID_MAX_SIZE + 1] = {0};

    wrq.u.essid.pointer = (caddr_t) essid;
    wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
    wrq.u.essid.flags = 0;
    if(iw_get_ext(skfd, ifname, SIOCGIWESSID, &wrq) < 0)
        return {};
    return essid;
}

int System_Informator::get_channel(int skfd, const char * ifname) const
{
    struct iwreq		wrq;
    struct iw_range	range;
    double		freq;
    int			channel;

    /* Get frequency / channel */
    if(iw_get_ext(skfd, ifname, SIOCGIWFREQ, &wrq) < 0)
        return(-1);

    /* Convert to channel */
    if(iw_get_range_info(skfd, ifname, &range) < 0)
        return(-2);
    freq = iw_freq2float(&(wrq.u.freq));
    if(freq < KILO)
        channel = (int) freq;
    else
    {
        channel = iw_freq_to_channel(freq, &range);
        if(channel < 0)
            return(-3);
    }

    return channel;
}

int System_Informator::get_signal(int skfd, const char *ifname) const
{
    struct iwreq wrq;
    iw_statistics stats;

    wrq.u.data.pointer = (caddr_t) &stats;
    wrq.u.data.length = sizeof(iw_statistics);
    wrq.u.data.flags = 0;
    if(iw_get_ext(skfd, ifname, SIOCGIWSTATS, &wrq) < 0)
        return -255;
    return stats.qual.level - 256;
}

bool System_Informator::check_internet_connection() const
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    const std::string url = "http://connectivitycheck.gstatic.com/generate_204";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

} // namespace Router
} // namespace Das
