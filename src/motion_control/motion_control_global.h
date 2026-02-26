#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MOTION_CONTROL_LIB)
#  define MOTION_CONTROL_EXPORT Q_DECL_EXPORT
# else
#  define MOTION_CONTROL_EXPORT Q_DECL_IMPORT
# endif
#else
# define MOTION_CONTROL_EXPORT
#endif
