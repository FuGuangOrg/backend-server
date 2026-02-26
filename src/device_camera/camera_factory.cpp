#include "camera_factory.h"
//#include "../device_enum/device_info_mvs.hpp"
#include "../device_enum/device_info_dvp2.hpp"

//#include "camera_mvs.h"
#include "camera_dvp2.h"


interface_camera* camera_factory::create_camera(st_device_info* device_info)
{
	if(device_info == nullptr)
	{
		return nullptr;
	}
	interface_camera* camera(nullptr);
	switch (device_info->m_type_sdk)
	{
	case SDK_MVS:
		{
			//st_device_info_mvs* mvs_device_info = dynamic_cast<st_device_info_mvs*>(device_info);
			//camera = new mvs_camera(&mvs_device_info->m_cc_device_info,device_info->m_unique_id);
			break;
		}
	case SDK_DVP2:
	{
		camera = new dvp2_camera(device_info->get_item(QString::fromStdString("设备名称")),
			device_info->get_item(QString::fromStdString("友好设备名称")),device_info->m_unique_id);
		break;
	}
	default:
		break;
	}
	return camera;
}