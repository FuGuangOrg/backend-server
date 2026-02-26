#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(DEVICE_CAMERA_LIB)
#  define DEVICE_CAMERA_EXPORT Q_DECL_EXPORT
# else
#  define DEVICE_CAMERA_EXPORT Q_DECL_IMPORT
# endif
#else
# define DEVICE_CAMERA_EXPORT
#endif
