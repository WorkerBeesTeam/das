#ifndef DAS_ROUTER_WPA_SUPPLICANT_CONFIG_H
#define DAS_ROUTER_WPA_SUPPLICANT_CONFIG_H

#include <string>
#include <vector>

namespace Das {
namespace Router {

class WPA_Supplicant_Config
{
public:
    explicit WPA_Supplicant_Config(const std::string& file_name);

    std::string get_str() const;
    void set_str(std::string text);

private:
    struct Networt_Item
    {
        std::string _ssid;
        std::string _psk;
    };

    std::vector<Networt_Item> parse() const;
    void write(const std::vector<Networt_Item> &items);

    const std::string _file_name;

    struct Phrases
    {
        const std::string _net = "network={";
        const std::string _ssid = "ssid";
        const std::string _psk = "psk";
    };
};

} // namespace Router
} // namespace Das

#endif // DAS_ROUTER_WPA_SUPPLICANT_CONFIG_H
