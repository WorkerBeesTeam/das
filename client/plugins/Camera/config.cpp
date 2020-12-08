#include <QVariant>

#include "config.h"

namespace Das {
namespace Camera {

/*static*/ Config &Config::instance()
{
    static Config conf;
    return conf;
}

} // namespace Camera
} // namespace Das
