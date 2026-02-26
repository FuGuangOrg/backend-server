#include "device_enum_factory.h"
//#include "device_enum_mvs.h"
#include "device_enum_dvp2.h"

#include <QMap>

QMap<TYPE_SDK, interface_device_enum*> device_enum_factory::m_map_device_enums;

interface_device_enum* device_enum_factory::create_device_enum(TYPE_SDK sdk_type)
{
	if (m_map_device_enums.contains(sdk_type))
		return m_map_device_enums[sdk_type];
	interface_device_enum* device_enum(nullptr);
	switch (sdk_type)
	{
		case SDK_MVS:
			//device_enum = new device_enum_mvs();
			break;
		case SDK_DVP2:
			device_enum = new device_enum_dvp2();
			break;
		default:
			break;
	}
	if (device_enum != nullptr)
	{
		m_map_device_enums[sdk_type] = device_enum;
	}
	return device_enum;
}

void device_enum_factory::release_device_enums()
{
	for (auto enum_obj : m_map_device_enums)
	{
		delete enum_obj;
	}
	m_map_device_enums.clear();
}