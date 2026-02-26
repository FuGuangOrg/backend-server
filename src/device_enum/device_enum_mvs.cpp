#include "device_enum_mvs.h"

bool device_enum_mvs::m_sdk_initialized = false;

device_enum_mvs::device_enum_mvs()
{
    initialize_sdk();
}

int device_enum_mvs::initialize_sdk()
{
    if (!m_sdk_initialized)
    {
        // 调用 MVS SDK 初始化函数
        int ret = MV_CC_Initialize();
        if (ret != MV_OK)
        {
            return ret;
        }
        m_sdk_initialized = true;
    }
    return MV_OK;
}

int device_enum_mvs::finalize_sdk()
{
    if (m_sdk_initialized)
    {
        int ret = MV_CC_Finalize();
        if (ret != MV_OK)
        {
            return ret;
        }
        m_sdk_initialized = false;
    }
    return MV_OK;
}

std::vector<st_device_info*> device_enum_mvs::enumerate_devices()
{
    initialize_sdk();
    std::vector<st_device_info*> device_list;
    if (!m_sdk_initialized)
    {
        return device_list;
    }
    MV_CC_DEVICE_INFO_LIST enum_device_list;
    memset(&enum_device_list, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int ret = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE | MV_CAMERALINK_DEVICE |
				MV_GENTL_CAMERALINK_DEVICE | MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE, &enum_device_list);
    if (ret != MV_OK)
    {
        return device_list;
    }
    for (unsigned i = 0; i < enum_device_list.nDeviceNum; i++)
    {
        MV_CC_DEVICE_INFO* device_info = enum_device_list.pDeviceInfo[i];
        if (device_info == nullptr)
        {
            continue;
        }
        st_device_info_mvs* st_info(nullptr);
        if (device_info->nTLayerType == MV_GIGE_DEVICE)
        {
            st_info = get_GigE_camera_info(device_info);
        }
        else if (device_info->nTLayerType == MV_USB_DEVICE)
        {
            st_info = get_usb_camera_info(device_info);
        }
        else if (device_info->nTLayerType == MV_GENTL_CAMERALINK_DEVICE)
        {
            st_info = get_camlink_camera_info(device_info);
        }
        else if (device_info->nTLayerType == MV_GENTL_CXP_DEVICE)
        {
            st_info = get_cxp_camera_info(device_info);
        }
        else if (device_info->nTLayerType == MV_GENTL_XOF_DEVICE)
        {
            st_info = get_xof_camera_info(device_info);
        }
        if (st_info != nullptr)
        {
            device_list.emplace_back(st_info); //添加到设备信息列表中
        }
    }
    return device_list;
}

st_device_info_mvs* device_enum_mvs::get_GigE_camera_info(MV_CC_DEVICE_INFO* device_info)
{
    if (device_info == nullptr || device_info->nTLayerType != MV_GIGE_DEVICE)
    {
        return nullptr;
    }
    st_device_info_mvs* st_info = new st_device_info_mvs(device_info);
    //制造商名称
    std::string manufacturer_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stGigEInfo.chManufacturerName);
    st_info->set_item(QString::fromStdString("制造商"), QString::fromStdString(manufacturer_name));
    //制造商的具体信息
    std::string manufacturer_info = reinterpret_cast<const char*>(device_info->SpecialInfo.stGigEInfo.chManufacturerSpecificInfo);
    st_info->set_item(QString::fromStdString("制造商信息"), QString::fromStdString(manufacturer_info));
    //型号名称
    std::string model_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stGigEInfo.chModelName);
    st_info->set_item(QString::fromStdString("设备型号"), QString::fromStdString(model_name));
    //设备版本
    std::string device_version = reinterpret_cast<const char*>(device_info->SpecialInfo.stGigEInfo.chDeviceVersion);
    st_info->set_item(QString::fromStdString("设备版本"), QString::fromStdString(device_version));
    //序列号
    std::string serial_number = reinterpret_cast<const char*>(device_info->SpecialInfo.stGigEInfo.chSerialNumber);
    st_info->set_item(QString::fromStdString("序列号"), QString::fromStdString(serial_number));
    //用户自定义名称
    std::string user_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stGigEInfo.chUserDefinedName);
    st_info->set_item(QString::fromStdString("设备名称"), QString::fromStdString(user_name));
    //获取 ip 地址
    int nIp1 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentIp) & 0xff000000) >> 24);
    int nIp2 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentIp) & 0x00ff0000) >> 16);
    int nIp3 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentIp) & 0x0000ff00) >> 8);
    int nIp4 = (static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentIp) & 0x000000ff);
    std::string ip_address = std::to_string(nIp1) + "." + std::to_string(nIp2) + "." + std::to_string(nIp3) + "." + std::to_string(nIp4);
    st_info->set_item(QString::fromStdString("IP地址"), QString::fromStdString(ip_address));
    //获取子网掩码
    int nMask1 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentSubNetMask) & 0xff000000) >> 24);
    int nMask2 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentSubNetMask) & 0x00ff0000) >> 16);
    int nMask3 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentSubNetMask) & 0x0000ff00) >> 8);
    int nMask4 = (static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nCurrentSubNetMask) & 0x000000ff);
    std::string str_sub_mask = std::to_string(nMask1) + "." + std::to_string(nMask2) + "." + std::to_string(nMask3) + "." + std::to_string(nMask4);
    st_info->set_item(QString::fromStdString("子网掩码"), QString::fromStdString(str_sub_mask));
    //默认网关
    int nGate1 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nDefultGateWay) & 0xff000000) >> 24);
    int nGate2 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nDefultGateWay) & 0x00ff0000) >> 16);
    int nGate3 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nDefultGateWay) & 0x0000ff00) >> 8);
    int nGate4 = (static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nDefultGateWay) & 0x000000ff);
    std::string str_default_gate = std::to_string(nGate1) + "." + std::to_string(nGate2) + "." + std::to_string(nGate3) + "." + std::to_string(nGate4);
    st_info->set_item(QString::fromStdString("默认网关"), QString::fromStdString(str_default_gate));
    //网口IP地址
    int nPort1 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nNetExport) & 0xff000000) >> 24);
    int nPort2 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nNetExport) & 0x00ff0000) >> 16);
    int nPort3 = ((static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nNetExport) & 0x0000ff00) >> 8);
    int nPort4 = (static_cast<unsigned int>(device_info->SpecialInfo.stGigEInfo.nNetExport) & 0x000000ff);
    std::string str_port = std::to_string(nPort1) + "." + std::to_string(nPort2) + "." + std::to_string(nPort3) + "." + std::to_string(nPort4);
    st_info->set_item(QString::fromStdString("网口IP地址"), QString::fromStdString(str_port));
    //唯一标识符
    st_info->m_unique_id = QString::fromStdString(serial_number + "(" + model_name + ")");
    return st_info;
}

st_device_info_mvs* device_enum_mvs::get_usb_camera_info(MV_CC_DEVICE_INFO* device_info)
{
    if (device_info == nullptr || device_info->nTLayerType != MV_USB_DEVICE)
    {
        return nullptr;
    }
    st_device_info_mvs* st_info = new st_device_info_mvs(device_info);
    //制造商名称，供应商名称，设备型号，设备版本, 序列号,支持的USB协议,设备名称, 设备地址
    std::string manufacturer_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stUsb3VInfo.chManufacturerName);
    st_info->set_item("制造商", manufacturer_name);
    std::string vendor_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stUsb3VInfo.chVendorName);
    st_info->set_item("供应商", vendor_name);
    std::string model_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stUsb3VInfo.chModelName);
    st_info->set_item("设备型号", model_name);
    std::string device_version = reinterpret_cast<const char*>(device_info->SpecialInfo.stUsb3VInfo.chDeviceVersion);
    st_info->set_item("设备版本", device_version);
    std::string serial_number = reinterpret_cast<const char*>(device_info->SpecialInfo.stUsb3VInfo.chSerialNumber);
    st_info->set_item("序列号", serial_number);
    std::string user_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stUsb3VInfo.chUserDefinedName);
    st_info->set_item("设备名称", user_name);
    std::string usb_protocols = std::to_string(device_info->SpecialInfo.stUsb3VInfo.nbcdUSB);
    st_info->set_item("支持的USB协议", usb_protocols);
    int nDevice1 = ((static_cast<unsigned int>(device_info->SpecialInfo.stUsb3VInfo.nDeviceAddress) & 0xff000000) >> 24);
    int nDevice2 = ((static_cast<unsigned int>(device_info->SpecialInfo.stUsb3VInfo.nDeviceAddress) & 0x00ff0000) >> 16);
    int nDevice3 = ((static_cast<unsigned int>(device_info->SpecialInfo.stUsb3VInfo.nDeviceAddress) & 0x0000ff00) >> 8);
    int nDevice4 = (static_cast<unsigned int>(device_info->SpecialInfo.stUsb3VInfo.nDeviceAddress) & 0x000000ff);
    std::string device_address = std::to_string(nDevice1) + "." + std::to_string(nDevice2) + "." + std::to_string(nDevice3) + "." + std::to_string(nDevice4);
    st_info->set_item("设备地址", user_name);
    //唯一标识符
    st_info->m_unique_id = QString::fromStdString(serial_number + "(" + model_name + ")");
    return st_info;
}

st_device_info_mvs* device_enum_mvs::get_camlink_camera_info(MV_CC_DEVICE_INFO* device_info)
{
    if (device_info == nullptr || device_info->nTLayerType != MV_GENTL_CAMERALINK_DEVICE)
    {
        return nullptr;
    }
    st_device_info_mvs* st_info = new st_device_info_mvs(device_info);
    std::string manufacturer_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chManufacturerInfo);
    st_info->set_item("制造商", manufacturer_name);
    std::string vendor_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chVendorName);
    st_info->set_item("供应商", vendor_name);
    std::string model_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chModelName);
    st_info->set_item("设备型号", model_name);
    std::string device_version = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chDeviceVersion);
    st_info->set_item("设备版本", device_version);
    std::string serial_number = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chSerialNumber);
    st_info->set_item("序列号", serial_number);
    std::string user_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chUserDefinedName);
    st_info->set_item(std::string("设备名称"), user_name);
    std::string device_id = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chDeviceID);
    st_info->set_item("设备ID", device_id);
    std::string interface_id = reinterpret_cast<const char*>(device_info->SpecialInfo.stCMLInfo.chInterfaceID);
    st_info->set_item("采集卡ID", interface_id);
    //唯一标识符
    st_info->m_unique_id = QString::fromStdString(serial_number + "(" + model_name + ")");
    return st_info;
}

st_device_info_mvs* device_enum_mvs::get_cxp_camera_info(MV_CC_DEVICE_INFO* device_info)
{
    if (device_info == nullptr || device_info->nTLayerType != MV_GENTL_CXP_DEVICE)
    {
        return nullptr;
    }
    st_device_info_mvs* st_info = new st_device_info_mvs(device_info);
    std::string manufacturer_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chManufacturerInfo);
    st_info->set_item("制造商", manufacturer_name);
    std::string vendor_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chVendorName);
    st_info->set_item("供应商", vendor_name);
    std::string model_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chModelName);
    st_info->set_item("设备型号", model_name);
    std::string device_version = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chDeviceVersion);
    st_info->set_item("设备版本", device_version);
    std::string serial_number = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chSerialNumber);
    st_info->set_item("序列号", serial_number);
    std::string user_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chUserDefinedName);
    st_info->set_item("设备名称", user_name);
    std::string device_id = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chDeviceID);
    st_info->set_item("设备ID", device_id);
    std::string interface_id = reinterpret_cast<const char*>(device_info->SpecialInfo.stCXPInfo.chInterfaceID);
    st_info->set_item("采集卡ID", interface_id);
    //唯一标识符
    st_info->m_unique_id = QString::fromStdString(serial_number + "(" + model_name + ")");
    return st_info;
}

st_device_info_mvs* device_enum_mvs::get_xof_camera_info(MV_CC_DEVICE_INFO* device_info)
{
    if (device_info == nullptr || device_info->nTLayerType != MV_GENTL_XOF_DEVICE)
    {
        return nullptr;
    }
    st_device_info_mvs* st_info = new st_device_info_mvs(device_info);
    std::string manufacturer_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chManufacturerInfo);
    st_info->set_item("制造商", manufacturer_name);
    std::string vendor_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chVendorName);
    st_info->set_item("供应商", vendor_name);
    std::string model_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chModelName);
    st_info->set_item("设备型号", model_name);
    std::string device_version = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chDeviceVersion);
    st_info->set_item("设备版本", device_version);
    std::string serial_number = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chSerialNumber);
    st_info->set_item("序列号", serial_number);
    std::string user_name = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chUserDefinedName);
    st_info->set_item("设备名称", user_name);
    std::string device_id = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chDeviceID);
    st_info->set_item("设备ID", device_id);
    std::string interface_id = reinterpret_cast<const char*>(device_info->SpecialInfo.stXoFInfo.chInterfaceID);
    st_info->set_item("采集卡ID", interface_id);
    //唯一标识符
    st_info->m_unique_id = QString::fromStdString(serial_number + "(" + model_name + ")");
    return st_info;
}
