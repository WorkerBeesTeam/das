#include "lib.h"

#define STR(x) #x

namespace Das {

quint8 Lib::ver_major() { return VER_MJ; }
quint8 Lib::ver_minor() { return VER_MN; }
int Lib::ver_build() { return VER_B; }
QString Lib::ver_str() { return STR(VER_MJ) "." STR(VER_MN) "." STR(VER_B); }

} // namespace Das
