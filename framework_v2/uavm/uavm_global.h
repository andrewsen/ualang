#ifndef UAVM_GLOBAL_H
#define UAVM_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(UAVM_LIBRARY)
#  define UAVMSHARED_EXPORT Q_DECL_EXPORT
#else
#  define UAVMSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // UAVM_GLOBAL_H
