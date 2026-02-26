#pragma once

#include "device_camera_global.h"
#include "interface_camera.h"

#include "../device_enum/device_info.hpp"

class DEVICE_CAMERA_EXPORT camera_factory
{
public:
	static interface_camera* create_camera(st_device_info* device_info);

};
