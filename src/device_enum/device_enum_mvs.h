#pragma once

#include "device_enum_factory.h"
//#include "device_info_mvs.hpp"

//使用海康威视的 SDK 执行枚举
class  DEVICE_ENUM_EXPORT device_enum_mvs : public interface_device_enum
{
    explicit device_enum_mvs();
    ~device_enum_mvs() = default;
public:
    virtual std::vector<st_device_info*> enumerate_devices() override;

    virtual int initialize_sdk() override;
    virtual int finalize_sdk() override;

    //加载各种类型的相机信息
    static st_device_info_mvs* get_GigE_camera_info(MV_CC_DEVICE_INFO* device_info);
    static st_device_info_mvs* get_usb_camera_info(MV_CC_DEVICE_INFO* device_info);
    static st_device_info_mvs* get_camlink_camera_info(MV_CC_DEVICE_INFO* device_info);
    static st_device_info_mvs* get_cxp_camera_info(MV_CC_DEVICE_INFO* device_info);
    static st_device_info_mvs* get_xof_camera_info(MV_CC_DEVICE_INFO* device_info);

private:
    friend class device_enum_factory;
    static bool m_sdk_initialized;
};
