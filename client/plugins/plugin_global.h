#ifndef DAS_PLUGIN_GLOBAL_H
#define DAS_PLUGIN_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DAS_PLUGIN_LIBRARY)
#  define DAS_PLUGIN_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define DAS_PLUGIN_SHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // DAS_PLUGIN_GLOBAL_H
