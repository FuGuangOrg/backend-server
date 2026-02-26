#pragma once

#include <QtCore/qglobal.h>

#ifdef BUILD_STATIC
# define DEVICE_ENUM_EXPORT
#else
# if defined(DEVICE_ENUM_LIB)
#  define DEVICE_ENUM_EXPORT Q_DECL_EXPORT
# else
#  define DEVICE_ENUM_EXPORT Q_DECL_IMPORT
# endif
#endif
