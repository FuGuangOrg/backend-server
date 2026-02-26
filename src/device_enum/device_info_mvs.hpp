#pragma once

#include "device_info.hpp"
#include "MvCameraControl.h"

// 使用 MVS SDK枚举得到的设备信息
struct DEVICE_ENUM_EXPORT st_device_info_mvs : public st_device_info
{
	MV_CC_DEVICE_INFO m_cc_device_info;
	
	st_device_info_mvs(const MV_CC_DEVICE_INFO* device_info = nullptr)
		: m_cc_device_info(MV_CC_DEVICE_INFO())
	{
		m_type_sdk = SDK_MVS;
		memset(&m_cc_device_info, 0, sizeof(MV_CC_DEVICE_INFO));
		if (device_info != nullptr)
		{
			m_cc_device_info = *device_info;
		}
	}

	int get_device_type() const override
	{
		if (m_cc_device_info.nTLayerType == MV_GIGE_DEVICE)
		{
			return TYPE_DEVICE_GIGE;
		}
		else if (m_cc_device_info.nTLayerType == MV_USB_DEVICE)
		{
			return TYPE_DEVICE_USB;
		}
		else if (m_cc_device_info.nTLayerType == MV_GENTL_CAMERALINK_DEVICE)
		{
			return TYPE_DEVICE_GENTL_CAMERALINK;
		}
		else if (m_cc_device_info.nTLayerType == MV_GENTL_CXP_DEVICE)
		{
			return TYPE_DEVICE_GENTL_CXP;
		}
		else if (m_cc_device_info.nTLayerType == MV_GENTL_XOF_DEVICE)
		{
			return TYPE_DEVICE_GENTL_XOF;
		}
		return TYPE_DEVICE_UNKNOW;
	}

	virtual ~st_device_info_mvs() = default;
};