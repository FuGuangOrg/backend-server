#pragma once

#include <QtCore/qglobal.h>

#ifdef BUILD_STATIC
	define EVENT_BUS_EXPORT
#else
	# if defined(EVENT_BUS_LIB)
	#  define EVENT_BUS_EXPORT Q_DECL_EXPORT
	# else
	#  define EVENT_BUS_EXPORT Q_DECL_IMPORT
	# endif
#endif
