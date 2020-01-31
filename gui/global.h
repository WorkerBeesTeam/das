#ifndef DAS_GUI_GLOBAL_H
#define DAS_GUI_GLOBAL_H

#include "Das/proto_scheme.h"

namespace Das {
namespace Gui {
    class Global {
    public:
        Global() {
            m_obj = this;
            prj_ = new Proto_Scheme(nullptr);
        }
        static Global* instance() { return m_obj; }
        Proto_Scheme *prj() { return prj_; }
    protected:
        Proto_Scheme* prj_;
    private:
        static Global * m_obj;
    };

    Global* g();

} // namespace Gui
} // namespace Das

#endif // DAS_GUI_GLOBAL_H
