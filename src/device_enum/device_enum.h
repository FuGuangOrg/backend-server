/***************************
 * 设备枚举工程,检测所有连接的设备
 * 可能会使用多个SDK
 **************************/
#pragma once


#include "device_info.hpp"

class DEVICE_ENUM_EXPORT interface_device_enum
{
public:
    explicit interface_device_enum() = default;
    ~interface_device_enum() = default;

    virtual int initialize_sdk() = 0;
    virtual int finalize_sdk() = 0;
    virtual std::vector<st_device_info*> enumerate_devices() = 0;

private:
    
};
