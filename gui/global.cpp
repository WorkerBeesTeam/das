
#include "global.h"

namespace Das {
namespace Gui {

Global * Global::m_obj = nullptr;

Global *g() { return Global::instance(); }

} // namespace Gui
} // namespace Das
