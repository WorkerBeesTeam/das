#include <set>

#include <Helpz/zstring.h>
#include <Helpz/zfile.h>
#include <Helpz/settings.h>

#include "wpa_supplicant_config.h"

namespace Das {
namespace Router {

using namespace Helpz;

WPA_Supplicant_Config::WPA_Supplicant_Config(const std::string &file_name) :
    _file_name(file_name)
{
}

std::string WPA_Supplicant_Config::get_str() const
{
    std::vector<Networt_Item> items = parse();
    std::string res;

    auto add_str = [&res](const std::string& str)
    {
        bool need_quote = str.find(' ') != std::string::npos;

        if (need_quote) res += '"';
        res += str;
        if (need_quote) res += '"';
    };

    for (auto it = items.cbegin(); it != items.cend(); ++it)
    {
        if (it != items.cbegin())
            res += ';';
        add_str(it->_ssid);
        res += ' ';
        add_str(it->_psk);
    }

    return res;
}

void WPA_Supplicant_Config::set_str(std::string text)
{
    String::trim(text);

    bool quoted;
    char end_ch;

    enum Step_Type { SSID, PSK, STEP_COUNT };
    int step = SSID;

    std::size_t pos = 0, pos_end;
    std::string value, ssid, psk;

    std::vector<Networt_Item> items;

    while (pos < text.size())
    {
        quoted = text.at(pos) == '"';
        if (quoted)
            ++pos;

        end_ch = quoted ? '"' :
                          step == SSID ? ' ' : ';';

        pos_end = text.find(end_ch, pos);
        if (pos_end == std::string::npos)
        {
            if (quoted || step == SSID)
                break;
            pos_end = text.size();
        }

        value = text.substr(pos, pos_end - pos);
        if (!quoted)
            String::trim(value);
        (step == SSID ? ssid : psk) = value;

        pos = pos_end;

        if (quoted)
            ++pos; // skip quote

        if (step == SSID && (pos >= text.size() || text.at(pos) != ' ')) // checking for a separator
            break;

        while (pos < text.size() && text.at(pos) == ' ')
            ++pos; // skip separator

        if (step == PSK)
        {
            if (!ssid.empty() && psk.size() >= 8)
                items.push_back({ ssid, psk });
            ssid.clear();
            psk.clear();

            while (pos < text.size() && (text.at(pos) == ' ' || text.at(pos) == ';'))
                ++pos;
        }

        ++step %= STEP_COUNT;
    }

    write(items);
}

std::vector<WPA_Supplicant_Config::Networt_Item> WPA_Supplicant_Config::parse() const
{
    const std::string text = File::read_all(_file_name);

    const Phrases phrase;
    std::size_t pos_end = text.find(phrase._net);
    if (pos_end == std::string::npos)
        return {};

    auto remove_quote = [](std::string& value)
    {
        if (!value.empty() && value.front() == '"')
            value.erase(value.begin());
        if (!value.empty() && value.back() == '"')
            value.erase(--value.end());
    };

    std::string net;
    std::size_t pos = 0;
    --pos_end;

    Networt_Item item;
    std::vector<Networt_Item> items;

    while ((pos = text.find(phrase._net, pos_end + 1)) != std::string::npos)
    {
        pos += phrase._net.size() + 1;

        pos_end = text.find('}', pos);
        if (pos_end == std::string::npos)
            break;

        net = text.substr(pos, pos_end - pos);

        item._ssid = Settings::get_param_value(net, phrase._ssid);
        remove_quote(item._ssid);

        item._psk = Settings::get_param_value(net, phrase._psk);
        remove_quote(item._psk);

        items.push_back(item);
    }

    return items;
}

void WPA_Supplicant_Config::write(const std::vector<Networt_Item>& items)
{
    std::string net;
    net.reserve(45 * items.size());

    std::set<std::string> ssid_set;

    const Phrases phrase;
    for (const Networt_Item& item: items)
    {
        if (item._psk.size() < 8 || ssid_set.find(item._ssid) != ssid_set.cend())
            continue;
        ssid_set.insert(item._ssid);

        net += phrase._net;
        net += "\n    ";
        net += phrase._ssid;
        net += "=\"";
        net += item._ssid;
        net += "\"\n    ";
        net += phrase._psk;
        net += "=\"";
        net += item._psk;
        net += "\"\n}\n\n";
    }

    File file(_file_name, File::READ_WRITE);
    std::string text = file.read_all();

    std::size_t begin_pos = text.find(phrase._net);
    if (begin_pos == std::string::npos)
        begin_pos = text.size();

    text.replace(begin_pos, text.size() - begin_pos, net);
    file.rewrite(text);
}


} // namespace Router
} // namespace Das
