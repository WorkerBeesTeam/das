#ifndef DAS_LIBRARY_H
#define DAS_LIBRARY_H

#include <QtCore/qglobal.h>

#if defined(DAS_LIBRARY)
#  define DAS_LIBRARY_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define DAS_LIBRARY_SHARED_EXPORT Q_DECL_IMPORT
#endif

#define SET_DAS_META(name) \
    QCoreApplication::setApplicationName(name); \
    QCoreApplication::setOrganizationDomain("DeviceAccess.ru"); \
    QCoreApplication::setOrganizationName(QCoreApplication::organizationDomain()); \
    QCoreApplication::setApplicationVersion(Das::getVersionString());


#endif // DAS_LIBRARY_H
