
#include "serverapicall.h"
#include "templatemodel.h"

namespace Das {
namespace Gui {

Proto_Scheme *TemplateGlobal::prj() { return ServerApiCall::instance()->prj(); }
const Proto_Scheme *TemplateGlobal::prj() const { return ServerApiCall::instance()->prj(); }

// ------------------------------------

TemplateItem::TemplateItem(Section *section) : sct(section) {}

// ------------------------------------

} // namespace Gui
} // namespace Das
