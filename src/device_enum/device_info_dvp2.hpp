#pragma once
#include "device_info.hpp"
#include "dvpParam.h"

// 使用 DVP2 SDK枚举得到的设备信息
struct DEVICE_ENUM_EXPORT st_device_info_dvp2 : public st_device_info
{
	st_device_info_dvp2()
	{
		m_type_sdk = SDK_DVP2;
	}
	int get_device_type() const override
	{
		//获取设备类型，只能从型号接口信息得到
		QString type_string = get_item(QString::fromStdString("接口信息"));
		if(type_string == "Network Camera" || type_string == "GigE")
		{
			return TYPE_DEVICE_GIGE;
		}
		else if(type_string == "USB")
		{
			return TYPE_DEVICE_USB;
		}
		return TYPE_DEVICE_UNKNOW;
	}

	virtual ~st_device_info_dvp2() = default;
};