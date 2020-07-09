#ifndef DAS_ROUTER_SYSTEM_INFORMATOR_H
#define DAS_ROUTER_SYSTEM_INFORMATOR_H

#include <string>
#include <vector>

namespace Das {
namespace Router {

class System_Informator
{
public:
    System_Informator();

    double get_cpu_temp() const;

    int get_cpu_load();
private:
    std::pair<std::size_t, std::size_t> get_proc_stat();
    std::pair<std::size_t, std::size_t> _last_proc_stat;

public:
    int get_mem_usage() const;
    int get_disk_space_usage() const;

    std::vector<std::string> get_net_info() const;
    std::string get_essid(int skfd, const char * ifname) const;
    int get_channel(int skfd, const char * ifname) const;

    bool check_internet_connection() const;
};

} // namespace Router
} // namespace Das

#endif // DAS_ROUTER_SYSTEM_INFORMATOR_H
