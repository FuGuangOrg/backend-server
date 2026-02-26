#pragma once
#include "device_enum.h"


class DEVICE_ENUM_EXPORT device_enum_factory
{
public:
	static interface_device_enum* create_device_enum(TYPE_SDK sdk_type);
	static void release_device_enums();
private:
	static QMap<TYPE_SDK, interface_device_enum*> m_map_device_enums;
};
