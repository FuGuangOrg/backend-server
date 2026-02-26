#pragma once

#include "DVPCamera.h"

#include "device_enum_factory.h"
#include "device_info_dvp2.hpp"

//使用度申科技的 SDK 执行枚举
class  DEVICE_ENUM_EXPORT device_enum_dvp2 : public interface_device_enum
{
    explicit device_enum_dvp2();
    ~device_enum_dvp2() = default;
public:
    virtual std::vector<st_device_info*> enumerate_devices() override;

    virtual int initialize_sdk() override;
    virtual int finalize_sdk() override;

    //加载各种类型的相机信息
    static st_device_info_dvp2*  get_dvp2_camera_info(const dvpCameraInfo& camera_info);
    //static st_mvs_device_info* get_GigE_camera_info(MV_CC_DEVICE_INFO* device_info);
    //static st_mvs_device_info* get_usb_camera_info(MV_CC_DEVICE_INFO* device_info);
    //static st_mvs_device_info* get_camlink_camera_info(MV_CC_DEVICE_INFO* device_info);
    //static st_mvs_device_info* get_cxp_camera_info(MV_CC_DEVICE_INFO* device_info);
    //static st_mvs_device_info* get_xof_camera_info(MV_CC_DEVICE_INFO* device_info);

private:
    friend class device_enum_factory;
    static bool m_sdk_initialized;
    static inline int max_camera_count{ 16 };           //最大支持的设备数量
};
