#ifndef DAS_LIB_H
#define DAS_LIB_H

#include <QString>

namespace Das {
namespace Lib {
    quint8 ver_major();
    quint8 ver_minor();
    int ver_build();
    QString ver_str();
} // namespace Lib
} // namespace Das

#endif // DAS_LIB_H
