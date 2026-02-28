#include "device_enum_dvp2.h"

bool device_enum_dvp2::m_sdk_initialized = false;

device_enum_dvp2::device_enum_dvp2()
{
    initialize_sdk();
}

int device_enum_dvp2::initialize_sdk()
{
	m_sdk_initialized = true;
    return 0;
}

int device_enum_dvp2::finalize_sdk()
{
	m_sdk_initialized = false;
    return 0;
}

std::vector<st_device_info*> device_enum_dvp2::enumerate_devices()
{
    initialize_sdk();
    std::vector<st_device_info*> device_info_list;
    if (!m_sdk_initialized)
    {
        return device_info_list;
    }
    dvpUint32 camera_count(0);
    dvpStatus ret = dvpRefresh(&camera_count);
    if(ret != DVP_STATUS_OK || camera_count == 0)
    {
        return device_info_list;
    }
    if(camera_count > static_cast<dvpUint32>(max_camera_count))
    {
        camera_count = static_cast<dvpUint32>(max_camera_count);
    }
    dvpCameraInfo camera_info;

	int index(0);
	// 逐个枚举检测到的设备
	for (dvpUint32 i = 0; i < camera_count; i++)
	{
        memset(&camera_info, 0, sizeof(dvpCameraInfo));
		dvpStatus status = dvpEnum(i, &camera_info);
		if (status == DVP_STATUS_OK)
		{
            st_device_info_dvp2* device_info = get_dvp2_camera_info(camera_info);
            if(device_info != nullptr)
            {
                device_info_list.emplace_back(device_info);
            }
		}
	}
    return device_info_list;
}

st_device_info_dvp2* device_enum_dvp2::get_dvp2_camera_info(const dvpCameraInfo& camera_info)
{
    st_device_info_dvp2* st_info = new st_device_info_dvp2();
    //制造商名称
    std::string manufacturer_name = camera_info.Manufacturer;
    st_info->set_item(QString::fromStdString("制造商"), QString::fromStdString(manufacturer_name));
    //供应商名称
    std::string vendor_name = camera_info.Vendor;
    st_info->set_item("供应商", vendor_name);
    //设备型号
    std::string model_name = camera_info.Model;
    st_info->set_item("设备型号", model_name);
    
    //设备系列
    std::string family_name = camera_info.Family;
    st_info->set_item("所属系列", family_name);
    //序列号
    std::string serial_number = camera_info.OriginalSerialNumber;
    st_info->set_item("序列号", serial_number);
    //设备名称
    std::string user_name = camera_info.UserID;
    st_info->set_item("设备名称", user_name);
    //友好设备名称
    std::string friendly_name = camera_info.FriendlyName;
    st_info->set_item("友好设备名称", friendly_name);
    //传感器信息
    std::string sensor_info = camera_info.SensorInfo;
    st_info->set_item("传感器信息", sensor_info);
    //固件版本
    std::string firmware_version = camera_info.FirmwareVersion;
    st_info->set_item("固件版本", firmware_version);
    //硬件版本
    std::string hardware_version = camera_info.HardwareVersion;
    st_info->set_item("硬件版本", hardware_version);
    //设备驱动版本
    std::string driver_version = camera_info.DscamVersion;
    st_info->set_item("驱动版本", driver_version);
    //内核驱动版本
    std::string kernel_version = camera_info.KernelVersion;
    st_info->set_item("内核驱动版本", kernel_version);
    //接口信息
    std::string port_info = camera_info.PortInfo;
    st_info->set_item("接口信息", port_info);
    //唯一标识符
    st_info->m_unique_id = QString::fromStdString(serial_number + "(" + model_name + ")");
    return st_info;
}
