#include "camera_mvs.h"

#include <QThread>

mvs_camera::mvs_camera(MV_CC_DEVICE_INFO* device_info, const QString& unique_id)
	:m_device_info(device_info)
{
	m_unique_id = unique_id;
    m_sdk_type = SDK_MVS;
    m_map_ret_status.insert(MV_OK, STATUS_SUCCESS);
    m_map_ret_status.insert(MV_E_HANDLE, STATUS_ERROR_HANDLE);
    m_map_ret_status.insert(MV_E_SUPPORT, STATUS_ERROR_SUPPORT);
    m_map_ret_status.insert(MV_E_PARAMETER, STATUS_ERROR_PARAMETER);

    // 不同的相机对于同一设置可能会有所不同，因此这里可能会有多个不同的键(各个相机的操作)对应相同的值(映射到同一操作)
    m_map_auto_exposure_mode.insert(QString::fromStdString("Disable"), global_auto_exposure_closed);
    m_map_auto_exposure_mode.insert(QString::fromStdString("Off"), global_auto_exposure_closed);
    m_map_auto_exposure_mode.insert(QString::fromStdString("Once"), global_auto_exposure_once);
    m_map_auto_exposure_mode.insert(QString::fromStdString("Continuous"), global_auto_exposure_continuous);

    m_map_auto_gain_mode.insert(QString::fromStdString("Disable"), global_auto_gain_closed);
    m_map_auto_gain_mode.insert(QString::fromStdString("Off"), global_auto_gain_closed);
    m_map_auto_gain_mode.insert(QString::fromStdString("Once"), global_auto_gain_once);
    m_map_auto_gain_mode.insert(QString::fromStdString("Continuous"), global_auto_gain_continuous);

    m_map_trigger_mode.insert(QString::fromStdString("Off"), global_trigger_mode_continuous);
    m_map_trigger_mode.insert(QString::fromStdString("On"), global_trigger_mode_once);

    m_map_trigger_source.insert(QString::fromStdString("Software"), global_trigger_source_software);
}

mvs_camera::~mvs_camera()
{
    if (m_device_handle)
    {
        MV_CC_DestroyHandle(m_device_handle);
        m_device_handle = nullptr;
    }
    m_device_info = nullptr;
}

bool mvs_camera::is_device_accessible(unsigned int nAccessMode) const
{
	if (m_device_info == nullptr)
	{
		return false;
	}
    return  MV_CC_IsDeviceAccessible(m_device_info, nAccessMode);
    
}

int mvs_camera::create_device_handle()
{
    if (m_device_handle != nullptr)
    {
        return m_map_ret_status[MV_OK];
    }
    int ret = MV_CC_CreateHandle(&m_device_handle, m_device_info);
    if (MV_OK != ret)
    {
        if(m_device_handle != nullptr)
        {
            MV_CC_DestroyHandle(m_device_handle);
            m_device_handle = nullptr;
        }
    }
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::open()
{
    if (m_device_info == nullptr)
    {
        return m_map_ret_status[MV_E_PARAMETER];
    }
    if(create_device_handle() != MV_OK)
    {
        return m_map_ret_status[MV_E_HANDLE];
    }
    //根据设备句柄打开设备
    int ret = MV_CC_OpenDevice(m_device_handle);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::close()
{
    if (nullptr == m_device_handle)
    {
        return m_map_ret_status[MV_E_HANDLE];
    }
    //停止采集
    stop_grab();
	// 停止子线程
    if(m_grab_worker!= nullptr)
    {
        m_grab_worker->stop();
        while (!m_grab_worker->is_finished())
        {
			QThread::msleep(10);    // 等待子线程函数结束
        }
		m_grab_thread->wait();      // 等待子线程结束
		delete m_grab_worker;
        m_grab_worker = nullptr;
		delete m_grab_thread;
        m_grab_thread = nullptr;
    }
    int ret = MV_CC_CloseDevice(m_device_handle);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

bool mvs_camera::is_device_connected()
{
    if(create_device_handle() != MV_OK)
    {
        return m_map_ret_status[MV_E_HANDLE];
    }
    return MV_CC_IsDeviceConnected(m_device_handle);
}

int mvs_camera::register_image_callback(void(__stdcall* cbOutput)(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser), void* pUser) const
{
    int ret = MV_CC_RegisterImageCallBackEx(m_device_handle, cbOutput, pUser);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::free_image_buffer(MV_FRAME_OUT* lp_frame) const
{
    int ret = MV_CC_FreeImageBuffer(m_device_handle, lp_frame);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::set_image_node_num(unsigned int num) const
{
    int ret = MV_CC_SetImageNodeNum(m_device_handle, num);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_device_info(MV_CC_DEVICE_INFO* device_info)
{
    if (create_device_handle() != MV_OK)
    {
        return m_map_ret_status[MV_E_HANDLE];
    }
    int ret = MV_CC_GetDeviceInfo(m_device_handle, device_info);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_all_match_info(void* info) const
{
    if(info == nullptr)
    {
        return m_map_ret_status[MV_E_PARAMETER];
    }
    MV_ALL_MATCH_INFO match_info = { 0,nullptr,0 };
    if (m_device_info->nTLayerType == MV_GIGE_DEVICE)
    {
        match_info.nType = MV_MATCH_TYPE_NET_DETECT;
        match_info.pInfo = info;
        match_info.nInfoSize = sizeof(MV_MATCH_INFO_NET_DETECT);
        memset(match_info.pInfo, 0, sizeof(MV_MATCH_INFO_NET_DETECT));
    }
    else if(m_device_info->nTLayerType == MV_USB_DEVICE)
    {
        match_info.nType = MV_MATCH_TYPE_USB_DETECT;
        match_info.pInfo = info;
        match_info.nInfoSize = sizeof(MV_MATCH_INFO_USB_DETECT);
        memset(match_info.pInfo, 0, sizeof(MV_MATCH_INFO_USB_DETECT));
    }
    int ret = MV_CC_GetAllMatchInfo(m_device_handle, &match_info);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_gige_all_match_info(MV_MATCH_INFO_NET_DETECT* match_info_net_detect)
{
    if (match_info_net_detect == nullptr)
    {
        return m_map_ret_status[MV_E_PARAMETER];
    }
    MV_CC_DEVICE_INFO device_info = {0};
    get_device_info(&device_info);
    if (device_info.nTLayerType != MV_GIGE_DEVICE)
    {
        return MV_E_SUPPORT;
    }
    MV_ALL_MATCH_INFO match_info = {0};
    match_info.nType = MV_MATCH_TYPE_NET_DETECT;
    match_info.pInfo = match_info_net_detect;
    match_info.nInfoSize = sizeof(MV_MATCH_INFO_NET_DETECT);
    memset(match_info.pInfo, 0, sizeof(MV_MATCH_INFO_NET_DETECT));

    int ret = MV_CC_GetAllMatchInfo(m_device_handle, &match_info);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_usb_all_match_info(MV_MATCH_INFO_USB_DETECT* match_info_usb_detect)
{
	if (match_info_usb_detect == nullptr)
	{
	    return m_map_ret_status[MV_E_PARAMETER];
	}
	MV_CC_DEVICE_INFO device_info = {0};
	get_device_info(&device_info);
	if (device_info.nTLayerType != MV_USB_DEVICE)
	{
	    return m_map_ret_status[MV_E_SUPPORT];
	}
	MV_ALL_MATCH_INFO match_info = {0};
	match_info.nType = MV_MATCH_TYPE_USB_DETECT;
	match_info.pInfo = match_info_usb_detect;
	match_info.nInfoSize = sizeof(MV_MATCH_INFO_USB_DETECT);
	memset(match_info.pInfo, 0, sizeof(MV_MATCH_INFO_USB_DETECT));

	int ret = MV_CC_GetAllMatchInfo(m_device_handle, &match_info);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

/****************************一些基础参数获取******************************/
double mvs_camera::get_frame_rate()
{
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AcquisitionFrameRate", &value);
    if (MV_OK != nRet)
    {
        return -1.0;
    }
    return static_cast<double>(value.fCurValue);
}

st_range mvs_camera::get_frame_rate_range()
{
    st_range range;
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AcquisitionFrameRate", &value);
    if (MV_OK != nRet)
    {
        return range;
    }
    range.min = static_cast<double>(value.fMin);
    range.max = static_cast<double>(value.fMax);
    return range;
}

int mvs_camera::set_frame_rate(double frame_rate)
{
    int ret = MV_CC_SetFloatValue(m_device_handle, "AcquisitionFrameRate", static_cast<float>(frame_rate));
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_maximum_width()
{
    MVCC_INTVALUE_EX value = { 0 };
    const int ret = MV_CC_GetIntValueEx(m_device_handle, "WidthMax", &value);
    if (MV_OK != ret)
    {
        return -1;
    }
    return static_cast<int>(value.nCurValue);
}

int mvs_camera::get_maximum_height()
{
    MVCC_INTVALUE_EX value = { 0 };
    const int nRet = MV_CC_GetIntValueEx(m_device_handle, "HeightMax", &value);
    if (MV_OK != nRet)
    {
        return -1;
    }
    return static_cast<int>(value.nCurValue);
}

int mvs_camera::get_start_x()
{
    MVCC_INTVALUE_EX value = { 0 };
    const int ret = MV_CC_GetIntValueEx(m_device_handle, "OffsetX", &value);
    if (MV_OK != ret)
    {
        return -1;
    }
    return static_cast<int>(value.nCurValue);
}

st_range mvs_camera::get_start_x_range()
{
    return st_range(0, get_maximum_width() - get_width());
}

int mvs_camera::set_start_x(int start_x)
{
    int ret = MV_CC_SetIntValueEx(m_device_handle, "OffsetX", start_x);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_start_y()
{
    MVCC_INTVALUE_EX value = { 0 };
    const int ret = MV_CC_GetIntValueEx(m_device_handle, "OffsetY", &value);
    if (MV_OK != ret)
    {
        return -1;
    }
    return static_cast<int>(value.nCurValue);
}

st_range mvs_camera::get_start_y_range()
{
    return st_range(0, get_maximum_height() - get_height());
}

int mvs_camera::set_start_y(int start_y)
{
    int ret = MV_CC_SetIntValueEx(m_device_handle, "OffsetY", start_y);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_width()
{
    MVCC_INTVALUE_EX value = { 0 };
    const int nRet = MV_CC_GetIntValueEx(m_device_handle, "Width", &value);
    if (MV_OK != nRet)
    {
        return -1;
    }
    return static_cast<int>(value.nCurValue);
}

st_range mvs_camera::get_width_range()
{
    return st_range(0, get_maximum_width() - get_start_x());
}

int mvs_camera::set_width(int width)
{
    int ret = MV_CC_SetIntValueEx(m_device_handle, "Width", width);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::get_height()
{
    MVCC_INTVALUE_EX value = { 0 };
    const int nRet = MV_CC_GetIntValueEx(m_device_handle, "Height", &value);
    if (MV_OK != nRet)
    {
        return -1;
    }
    return static_cast<int>(value.nCurValue);
}

st_range mvs_camera::get_height_range()
{
    return st_range(0, get_maximum_height() - get_start_y());
}

int mvs_camera::set_height(int height)
{
    int ret = MV_CC_SetIntValueEx(m_device_handle, "Height", height);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

const QMap<QString, unsigned int>& mvs_camera::enum_pixel_format()
{
    if(m_supported_formats.isEmpty())
    {
        MVCC_ENUMVALUE value = { 0 };
        int nRet = MV_CC_GetEnumValue(m_device_handle, "PixelFormat", &value);
        if (MV_OK != nRet)
        {
            return m_supported_formats;
        }
        MVCC_ENUMENTRY pixel_format_info = { 0 };
        for (unsigned int i = 0; i < value.nSupportedNum; i++)
        {
            pixel_format_info.nValue = value.nSupportValue[i];
            if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "PixelFormat", &pixel_format_info))
            {
                continue;
            }
            m_supported_formats.insert(QString::fromStdString(pixel_format_info.chSymbolic), pixel_format_info.nValue);
        }
    }
    return m_supported_formats;
}

QString mvs_camera::get_pixel_format()
{
    MVCC_ENUMVALUE value = { 0 };
    MVCC_ENUMENTRY pixel_format_info = { 0 };
    int nRet = MV_CC_GetEnumValue(m_device_handle, "PixelFormat", &value);
    if (MV_OK != nRet)
    {
        return QString::fromStdString("error_pixel_format");
    }
    pixel_format_info.nValue = value.nCurValue;
    nRet = MV_CC_GetEnumEntrySymbolic(m_device_handle, "PixelFormat", &pixel_format_info);
    if (MV_OK != nRet)
    {
        return QString::fromStdString("error_pixel_format");
    }
    return QString::fromStdString(pixel_format_info.chSymbolic);
}

int mvs_camera::set_pixel_format(unsigned int format)
{
    int ret = MV_CC_SetEnumValue(m_device_handle, "PixelFormat", format);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::set_pixel_format(const QString& sFormat)
{
	if(m_supported_formats.find(sFormat) == m_supported_formats.end())
	{
        return m_map_ret_status[MV_E_PARAMETER];
	}
    return set_pixel_format(m_supported_formats[sFormat]);
}

const QMap<QString, unsigned int>& mvs_camera::enum_supported_auto_exposure_mode()
{
    if (m_supported_auto_exposure_mode.isEmpty())
    {
        MVCC_ENUMVALUE value = { 0 };
        int nRet = MV_CC_GetEnumValue(m_device_handle, "ExposureAuto", &value);
        if (MV_OK != nRet)
        {
            m_supported_auto_exposure_mode.insert(QString::fromStdString("关闭"), value.nCurValue);
            return m_supported_auto_exposure_mode;
        }
        MVCC_ENUMENTRY auto_exposure_info = { 0 };
        for (unsigned int i = 0; i < value.nSupportedNum; i++)
        {
            auto_exposure_info.nValue = value.nSupportValue[i];
            if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "ExposureAuto", &auto_exposure_info))
            {
                continue;
            }
            QString name = QString::fromStdString(auto_exposure_info.chSymbolic);
            if (m_map_auto_exposure_mode.contains(name))
            {
                m_supported_auto_exposure_mode.insert(m_map_auto_exposure_mode[name], auto_exposure_info.nValue);
            }
            else
            {
                m_supported_auto_exposure_mode.insert(QString::fromStdString("error_mode"), auto_exposure_info.nValue);
            }
        }
        if(m_supported_auto_exposure_mode.isEmpty())
        {
            m_supported_auto_exposure_mode.insert(global_auto_exposure_closed, value.nCurValue);
        }
    }
    return m_supported_auto_exposure_mode;
}

QString mvs_camera::get_auto_exposure_mode()
{
    MVCC_ENUMVALUE value = { 0 };
    const int ret = MV_CC_GetEnumValue(m_device_handle, "ExposureAuto", &value);
    if (MV_OK != ret)
    {
        return QString::fromStdString("关闭");
    }
    MVCC_ENUMENTRY auto_exposure_info = { 0 };
    auto_exposure_info.nValue = value.nCurValue;
    if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "ExposureAuto", &auto_exposure_info))
    {
        return QString::fromStdString("关闭");
    }
    QString mode = QString::fromStdString(auto_exposure_info.chSymbolic);
    if (m_map_auto_exposure_mode.contains(mode))
    {
        return m_map_auto_exposure_mode[mode];
    }
    return QString("关闭");
}

int mvs_camera::set_auto_exposure_mode(QString auto_exposure_mode)
{
    if(!m_supported_auto_exposure_mode.contains(auto_exposure_mode))
    {
        return STATUS_ERROR_PARAMETER;
    }
    int ret = MV_CC_SetEnumValue(m_device_handle, "ExposureAuto", m_supported_auto_exposure_mode[auto_exposure_mode]);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

bool mvs_camera::is_auto_exposure_closed()
{
    return get_auto_exposure_mode() == global_auto_exposure_closed;
}

double mvs_camera::get_auto_exposure_time_floor()
{
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoExposureTimeLowerLimit", &value);
    if (MV_OK != nRet)
    {
        return -1.0;
    }
    return static_cast<double>(value.fCurValue);
}

st_range mvs_camera::get_auto_exposure_time_floor_range()
{
    st_range range;
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoExposureTimeLowerLimit", &value);
    if (MV_OK != nRet)
    {
        return range;
    }
    range.min = static_cast<double>(value.fMin);
    range.max = static_cast<double>(value.fMax);
    return range;
}

int mvs_camera::set_auto_exposure_time_floor(double exposure_time)
{
    int ret = MV_CC_SetFloatValue(m_device_handle, "AutoExposureTimeLowerLimit", static_cast<float>(exposure_time));
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

double mvs_camera::get_auto_exposure_time_upper()
{
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoExposureTimeUpperLimit", &value);
    if (MV_OK != nRet)
    {
        return -1.0;
    }
    return static_cast<double>(value.fCurValue);
}

st_range mvs_camera::get_auto_exposure_time_upper_range()
{
    st_range range;
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoExposureTimeUpperLimit", &value);
    if (MV_OK != nRet)
    {
        return range;
    }
    range.min = static_cast<double>(value.fMin);
    range.max = static_cast<double>(value.fMax);
    return range;
}

int mvs_camera::set_auto_exposure_time_upper(double exposure_time)
{
    int ret = MV_CC_SetFloatValue(m_device_handle, "AutoExposureTimeUpperLimit", static_cast<float>(exposure_time));
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

double mvs_camera::get_exposure_time()
{
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "ExposureTime", &value);
    if (MV_OK != nRet)
    {
        return -1.0;
    }
    return static_cast<double>(value.fCurValue);
}

st_range mvs_camera::get_exposure_time_range()
{
    st_range range;
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "ExposureTime", &value);
    if (MV_OK != nRet)
    {
        return range;
    }
    range.min = static_cast<double>(value.fMin);
    range.max = static_cast<double>(value.fMax);
    return range;
}

int mvs_camera::set_exposure_time(double exposure_time)
{
    int ret = MV_CC_SetFloatValue(m_device_handle, "ExposureTime", static_cast<float>(exposure_time));
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

const QMap<QString, unsigned int>& mvs_camera::enum_supported_auto_gain_mode()
{
    if (m_supported_auto_gain_mode.isEmpty())
    {
        MVCC_ENUMVALUE value = { 0 };
        int nRet = MV_CC_GetEnumValue(m_device_handle, "GainAuto", &value);
        if (MV_OK != nRet)
        {
            return m_supported_auto_gain_mode;
        }
        MVCC_ENUMENTRY auto_gain_info = { 0 };
        for (unsigned int i = 0; i < value.nSupportedNum; i++)
        {
            auto_gain_info.nValue = value.nSupportValue[i];
            if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "GainAuto", &auto_gain_info))
            {
                continue;
            }
            QString name = QString::fromStdString(auto_gain_info.chSymbolic);
            if (m_map_auto_gain_mode.contains(name))
            {
                m_supported_auto_gain_mode.insert(m_map_auto_gain_mode[name], auto_gain_info.nValue);
            }
            else
            {
                m_supported_auto_gain_mode.insert(QString::fromStdString("error_mode"), auto_gain_info.nValue);
            }
        }
        if (m_supported_auto_gain_mode.isEmpty())
        {
            m_supported_auto_gain_mode.insert(global_auto_gain_closed, value.nCurValue);
        }
    }
    return m_supported_auto_gain_mode;

}

QString mvs_camera::get_auto_gain_mode()
{
    MVCC_ENUMVALUE value = { 0 };
    const int nRet = MV_CC_GetEnumValue(m_device_handle, "GainAuto", &value);
    if (MV_OK != nRet)
    {
        return QString::fromStdString("error_mode");
    }
    MVCC_ENUMENTRY auto_gain_info = { 0 };
    auto_gain_info.nValue = value.nCurValue;
    if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "GainAuto", &auto_gain_info))
    {
        return QString::fromStdString("error_mode");
    }
    QString mode = QString::fromStdString(auto_gain_info.chSymbolic);
    if (m_map_auto_gain_mode.contains(mode))
    {
        return m_map_auto_gain_mode[mode];
    }
    return QString("error_mode");
}

int mvs_camera::set_auto_gain_mode(QString auto_gain_mode)
{
    if (!m_supported_auto_gain_mode.contains(auto_gain_mode))
    {
        return STATUS_ERROR_PARAMETER;
    }
    int ret = MV_CC_SetEnumValue(m_device_handle, "GainAuto", m_supported_auto_gain_mode[auto_gain_mode]);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

bool mvs_camera::is_auto_gain_closed()
{
    return get_auto_gain_mode() == global_auto_gain_closed;
}

double mvs_camera::get_auto_gain_floor()
{
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoGainLowerLimit", &value);
    if (MV_OK != nRet)
    {
        return -1.0;
    }
    return static_cast<double>(value.fCurValue);
}

st_range mvs_camera::get_auto_gain_floor_range()
{
    st_range range;
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoGainLowerLimit", &value);
    if (MV_OK != nRet)
    {
        return range;
    }
    range.min = static_cast<double>(value.fMin);
    range.max = static_cast<double>(value.fMax);
    return range;
}

int mvs_camera::set_auto_gain_floor(double gain)
{
    int ret = MV_CC_SetFloatValue(m_device_handle, "AutoGainLowerLimit", static_cast<float>(gain));
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

double mvs_camera::get_auto_gain_upper()
{
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoGainUpperLimit", &value);
    if (MV_OK != nRet)
    {
        return -1.0;
    }
    return static_cast<double>(value.fCurValue);
}

st_range mvs_camera::get_auto_gain_upper_range()
{
    st_range range;
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "AutoGainUpperLimit", &value);
    if (MV_OK != nRet)
    {
        return range;
    }
    range.min = static_cast<double>(value.fMin);
    range.max = static_cast<double>(value.fMax);
    return range;
}

int mvs_camera::set_auto_gain_upper(double gain)
{
    int ret = MV_CC_SetFloatValue(m_device_handle, "AutoGainUpperLimit", static_cast<float>(gain));
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

double mvs_camera::get_gain()
{
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "Gain", &value);
    if (MV_OK != nRet)
    {
        return -1.0;
    }
    return static_cast<double>(value.fCurValue);
}

st_range mvs_camera::get_gain_range()
{
    st_range range;
    MVCC_FLOATVALUE value = { 0 };
    const int nRet = MV_CC_GetFloatValue(m_device_handle, "Gain", &value);
    if (MV_OK != nRet)
    {
        return range;
    }
    range.min = static_cast<double>(value.fMin);
    range.max = static_cast<double>(value.fMax);
    return range;
}

int mvs_camera::set_gain(double gain)
{
    int ret = MV_CC_SetFloatValue(m_device_handle, "Gain", static_cast<float>(gain));
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}


/****************************采集控制******************************/
const QMap<QString, unsigned int>& mvs_camera::enum_supported_trigger_mode()
{
    if(m_supported_trigger_mode.isEmpty())
    {
        MVCC_ENUMVALUE value = { 0 };
        int ret = MV_CC_GetEnumValue(m_device_handle, "TriggerMode", &value);
        if (MV_OK != ret)
        {
            return m_supported_trigger_mode;
        }
        for (unsigned int i = 0; i < value.nSupportedNum; i++)
        {
            MVCC_ENUMENTRY trigger_mode_info = { 0 };
            trigger_mode_info.nValue = value.nSupportValue[i];
            if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "TriggerMode", &trigger_mode_info))
            {
                continue;
            }
            QString name = QString::fromStdString(trigger_mode_info.chSymbolic);
            if (m_map_trigger_mode.contains(name))
            {
                m_supported_trigger_mode.insert(m_map_trigger_mode[name], trigger_mode_info.nValue);
            }
            else
            {
                m_supported_trigger_mode.insert(QString::fromStdString("error_mode"), trigger_mode_info.nValue);
            }
        }
    }
    return m_supported_trigger_mode;
}

QString mvs_camera::get_trigger_mode()
{
    MVCC_ENUMVALUE value = { 0 };
    int ret = MV_CC_GetEnumValue(m_device_handle, "TriggerMode", &value);
    if(ret != MV_OK)
    {
        return QString::fromStdString("error_mode");
    }
    MVCC_ENUMENTRY trigger_mode_info = { 0 };
    trigger_mode_info.nValue = value.nCurValue;
    if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "TriggerMode", &trigger_mode_info))
    {
        return QString::fromStdString("error_mode");
    }
    QString mode = QString::fromStdString(trigger_mode_info.chSymbolic);
    if (m_map_trigger_mode.contains(mode))
    {
        return m_map_trigger_mode[mode];
    }
    return QString("error_mode");
}

int mvs_camera::set_trigger_mode(QString trigger_mode)
{
    if (!m_supported_trigger_mode.contains(trigger_mode))
    {
        return STATUS_ERROR_PARAMETER;
    }
    int ret = MV_CC_SetEnumValue(m_device_handle, "TriggerMode", m_supported_trigger_mode[trigger_mode]);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

const QMap<QString, unsigned int>& mvs_camera::enum_supported_trigger_source()
{
    if (m_supported_trigger_source.isEmpty())
    {
        MVCC_ENUMVALUE value = { 0 };
        int ret = MV_CC_GetEnumValue(m_device_handle, "TriggerSource", &value);
        if (MV_OK != ret)
        {
            return m_supported_trigger_source;
        }
        for (unsigned int i = 0; i < value.nSupportedNum; i++)
        {
            MVCC_ENUMENTRY trigger_source_info = { 0 };
            trigger_source_info.nValue = value.nSupportValue[i];
            if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "TriggerSource", &trigger_source_info))
            {
                continue;
            }
            QString name = QString::fromStdString(trigger_source_info.chSymbolic);
            if (m_map_trigger_source.contains(name))
            {
                m_supported_trigger_source.insert(m_map_trigger_source[name], trigger_source_info.nValue);
            }
            else
            {
                m_supported_trigger_source.insert(name, trigger_source_info.nValue);
            }
        }
    }
    return m_supported_trigger_source;

}

QString mvs_camera::get_trigger_source()
{
    MVCC_ENUMVALUE value = { 0 };
    int ret = MV_CC_GetEnumValue(m_device_handle, "TriggerSource", &value);
    if (ret != MV_OK)
    {
        return QString::fromStdString("error_source");
    }
    MVCC_ENUMENTRY trigger_source_info = { 0 };
    trigger_source_info.nValue = value.nCurValue;
    if (MV_OK != MV_CC_GetEnumEntrySymbolic(m_device_handle, "TriggerSource", &trigger_source_info))
    {
        return QString::fromStdString("error_source");
    }
    QString mode = QString::fromStdString(trigger_source_info.chSymbolic);
    if (m_map_trigger_source.contains(mode))
    {
        return m_map_trigger_source[mode];
    }
    else if (m_supported_trigger_source.contains(mode))
    {
        return mode;
    }
    return QString("error_source");
}

int mvs_camera::set_trigger_source(QString trigger_source)
{
    if (!m_supported_trigger_source.contains(trigger_source))
    {
        return STATUS_ERROR_PARAMETER;
    }
    int ret = MV_CC_SetEnumValue(m_device_handle, "TriggerSource", m_supported_trigger_source[trigger_source]);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::start_grab()
{
    if (m_grab_thread == nullptr)
    {
        m_grab_thread = new QThread;
        m_grab_worker = new grab_worker(this);
        m_grab_worker->moveToThread(m_grab_thread);
        connect(m_grab_thread, &QThread::started, m_grab_worker, &grab_worker::process);
        m_grab_thread->start();         // 启动线程
    }
    m_grab_worker->resume();
    while (m_grab_worker->is_paused())
    {
		QThread::msleep(10);
        continue;
    }
    int ret = MV_CC_StartGrabbing(m_device_handle);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::stop_grab()
{
    if (m_grab_worker == nullptr)
    {
        return STATUS_SUCCESS;
    }
    m_grab_worker->pause();
    while (!m_grab_worker->is_paused())
    {
        QThread::msleep(10);
        continue;
    }
    int ret = MV_CC_StopGrabbing(m_device_handle);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

bool mvs_camera::is_grab_running()
{
	if(m_grab_worker != nullptr)
    {
        if(!m_grab_worker->is_finished() && !m_grab_worker->is_paused())
        {
	        return true;
        }
    }
    return false;
}

QImage mvs_camera::get_image(int millisecond)
{
    MV_FRAME_OUT mv_frame{ 0 };
    int ret = MV_CC_GetImageBuffer(m_device_handle, &mv_frame, millisecond);
    if (ret != MV_OK)
    {
        return QImage();
    }
    // mv_frame 转 QImage
    // .....
    return QImage();
}

QImage mvs_camera::trigger_once()
{
    int ret = MV_CC_SetCommandValue(m_device_handle, "TriggerSoftware");
    //...
    return QImage();
}



int mvs_camera::execute_command(IN const char* command) const
{
    return MV_CC_SetCommandValue(m_device_handle, command);
}

int mvs_camera::get_optimal_packet_size(unsigned int* optimal_packet_size) const
{
	if (m_device_info->nTLayerType != MV_GIGE_DEVICE)
	{
        return m_map_ret_status[MV_E_SUPPORT];
	}
    if (optimal_packet_size == nullptr)
    {
        return m_map_ret_status[MV_E_PARAMETER];
    }
    int nRet = MV_CC_GetOptimalPacketSize(m_device_handle);
    if (nRet < MV_OK)
    {
        return nRet;
    }
    *optimal_packet_size = static_cast<unsigned int>(nRet);
    return m_map_ret_status[MV_OK];
}

int mvs_camera::register_exception_callback(void(__stdcall* exception)(unsigned int msg_type, void* user), void* user) const
{
    int ret = MV_CC_RegisterExceptionCallBack(m_device_handle, exception, user);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::register_event_callback(const char* event_name, void(__stdcall* event)(MV_EVENT_OUT_INFO* event_info, void* user), void* user) const
{
    int ret = MV_CC_RegisterEventCallBackEx(m_device_handle, event_name, event, user);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::set_ip_address(unsigned int ip, unsigned int subnet_mask, unsigned int default_gateway)
{
    int ret = create_device_handle();
    if(ret != MV_OK)
    {
        if (!m_map_ret_status.contains(ret))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[ret];
    }
    if(is_device_accessible(MV_ACCESS_Exclusive))
    {
        ret = MV_GIGE_SetIpConfig(m_device_handle, MV_IP_CFG_STATIC);
        if (MV_OK != ret)
        {
            if (!m_map_ret_status.contains(ret))
            {
                return STATUS_ERROR_UNKNOWN;
            }
            return m_map_ret_status[ret];
        }
        //设置强制IP
        ret = MV_GIGE_ForceIpEx(m_device_handle, ip, subnet_mask, default_gateway);
        if (MV_OK != ret)
        {
            if (!m_map_ret_status.contains(ret))
            {
                return STATUS_ERROR_UNKNOWN;
            }
            return m_map_ret_status[ret];
        }
        //需要重新创建句柄
        MV_CC_DestroyHandle(m_device_handle);
        m_device_handle = nullptr;
        ret = create_device_handle();
    }
    else
    {
        //设置强制IP
        ret = MV_GIGE_ForceIpEx(m_device_handle, ip, subnet_mask, default_gateway);
        if (MV_OK != ret)
        {
            if (!m_map_ret_status.contains(ret))
            {
                return STATUS_ERROR_UNKNOWN;
            }
            return m_map_ret_status[ret];
        }
        m_device_info->SpecialInfo.stGigEInfo.nCurrentIp = ip;
        m_device_info->SpecialInfo.stGigEInfo.nCurrentSubNetMask = subnet_mask;
        m_device_info->SpecialInfo.stGigEInfo.nDefultGateWay = default_gateway;
        //需要重新创建句柄，设置为静态IP方式进行保存
        MV_CC_DestroyHandle(m_device_handle);
        m_device_handle = nullptr;
        ret = create_device_handle();
        if(ret != MV_OK)
        {
            if (!m_map_ret_status.contains(ret))
            {
                return STATUS_ERROR_UNKNOWN;
            }
            return m_map_ret_status[ret];
        }
        //设置IP配置选项
        ret = MV_GIGE_SetIpConfig(m_device_handle, MV_IP_CFG_STATIC);
    }
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::set_ip_config(unsigned int type)
{
    int ret = create_device_handle();
    if (ret != MV_OK)
    {
        if (!m_map_ret_status.contains(ret))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[ret];
    }
    if(type == IP_CONFIG_DHCP)
    {
        ret = MV_GIGE_SetIpConfig(m_device_handle, MV_IP_CFG_DHCP);
    }
    else if(type == IP_CONFIG_LLA)
    {
        ret = MV_GIGE_SetIpConfig(m_device_handle, MV_IP_CFG_LLA);
    }
    else
    {
        ret = MV_E_PARAMETER;
    }
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::set_net_transfer_mode(unsigned int mode) const
{
    int ret = MV_GIGE_SetNetTransMode(m_device_handle, mode);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::convert_pixel_format(MV_CC_PIXEL_CONVERT_PARAM_EX* convert_param) const
{
    int ret = MV_CC_ConvertPixelTypeEx(m_device_handle, convert_param);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::save_image(MV_SAVE_IMAGE_PARAM_EX3* param) const
{
    int ret = MV_CC_SaveImageEx3(m_device_handle, param);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::save_image_to_file(MV_CC_IMAGE* image, MV_CC_SAVE_IMAGE_PARAM* param, const char* file_path) const
{
	int ret = MV_CC_SaveImageToFileEx2(m_device_handle, image, param, file_path);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::draw_circle(MVCC_CIRCLE_INFO* circle_info) const
{
    int ret = MV_CC_DrawCircle(m_device_handle, circle_info);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}

int mvs_camera::draw_lines(MVCC_LINES_INFO* lines_info) const
{
    int ret = MV_CC_DrawLines(m_device_handle, lines_info);
    if (!m_map_ret_status.contains(ret))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[ret];
}
