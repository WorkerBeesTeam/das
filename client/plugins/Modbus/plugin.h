#ifndef DAS_MODBUS_PLUGIN_H
#define DAS_MODBUS_PLUGIN_H

#include "modbus_plugin_base.h"

namespace Das {
namespace Modbus {

class DAS_PLUGIN_SHARED_EXPORT Modbus_Plugin final : public Modbus_Plugin_Base
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DasCheckerInterface_iid FILE "checkerinfo.json")
    Q_INTERFACES(Das::Checker::Interface)
};

} // namespace Modbus
} // namespace Das

#endif // DAS_MODBUSPLUGIN_H
