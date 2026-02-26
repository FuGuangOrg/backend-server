#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(AUTO_FOCUS_LIB)
#  define AUTO_FOCUS_EXPORT Q_DECL_EXPORT
# else
#  define AUTO_FOCUS_EXPORT Q_DECL_IMPORT
# endif
#else
# define AUTO_FOCUS_EXPORT
#endif
