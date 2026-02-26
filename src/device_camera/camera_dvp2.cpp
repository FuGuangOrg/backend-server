#include "camera_dvp2.h"

#include <QFile>
#include <QtXml/QDomDocument>

#include "dvpParam.h"

#include "../common/common.h"

dvp2_camera::dvp2_camera(const QString& user_name, const QString& friendly_name, const QString& unique_id)
			:m_user_name(user_name),m_friendly_name(friendly_name)
{
	m_unique_id = unique_id;
    m_sdk_type = SDK_DVP2;
    m_map_ret_status.insert(DVP_STATUS_OK, STATUS_SUCCESS);
    m_map_ret_status.insert(DVP_STATUS_INVALID_HANDLE, STATUS_ERROR_HANDLE);
    m_map_ret_status.insert(DVP_STATUS_NOT_SUPPORTED, STATUS_ERROR_SUPPORT);
    m_map_ret_status.insert(DVP_STATUS_PARAMETER_INVALID, STATUS_ERROR_PARAMETER);

    m_global_map_auto_exposure_mode.insert(QString::fromStdString("Disable"), global_auto_exposure_closed);
    m_global_map_auto_exposure_mode.insert(QString::fromStdString("Once"), global_auto_exposure_once);
    m_global_map_auto_exposure_mode.insert(QString::fromStdString("Continuous"), global_auto_exposure_continuous);
    m_global_map_auto_exposure_mode.insert(QString::fromStdString("AE_OP_OFF"), global_auto_exposure_closed);
    m_global_map_auto_exposure_mode.insert(QString::fromStdString("AE_OP_ONCE"), global_auto_exposure_once);
    m_global_map_auto_exposure_mode.insert(QString::fromStdString("AE_OP_CONTINUOUS"), global_auto_exposure_continuous);

    m_global_map_auto_gain_mode.insert(QString::fromStdString("Disable"), global_auto_gain_closed);
    m_global_map_auto_gain_mode.insert(QString::fromStdString("Once"), global_auto_gain_once);
    m_global_map_auto_gain_mode.insert(QString::fromStdString("Continuous"), global_auto_gain_continuous);
}

dvp2_camera::~dvp2_camera()
{
    
}

bool dvp2_camera::is_device_accessible(unsigned int nAccessMode) const
{
    return true;
}

int dvp2_camera::create_device_handle()
{
    return 0;
}

int dvp2_camera::open()
{
    dvpStatus status = dvpOpenByUserId(m_user_name.toStdString().c_str(), OPEN_NORMAL, &m_device_handle);
    if(status != DVP_STATUS_OK)
    {
        status = dvpOpenByName(m_friendly_name.toStdString().c_str(), OPEN_NORMAL, &m_device_handle);
        if(status != DVP_STATUS_OK)
        {
            return map_ret_status(status);
		}
    }
    //注册回调函数
    status = dvpRegisterStreamCallback(m_device_handle, stream_callback, STREAM_EVENT_FRAME_THREAD, this);
    m_is_opened = true;
    return map_ret_status(status);
}

int dvp2_camera::close()
{
    if(m_device_handle != 0)
    {
        dvpStreamState state;
        dvpGetStreamState(m_device_handle, &state);
        if (state == STATE_STARTED)
        {
            dvpStatus status = dvpStop(m_device_handle);
            if(status != DVP_STATUS_OK)
            {
                return map_ret_status(status);
            }
        }

        dvpStatus status = dvpClose(m_device_handle);
        if (status != DVP_STATUS_OK)
        {
            return map_ret_status(status);
        }
		m_is_opened = false;
        m_device_handle = 0;
    }
    return STATUS_SUCCESS;
}

bool dvp2_camera::is_device_connected()
{
    return true;
}

/****************************一些基础参数获取******************************/
double dvp2_camera::get_frame_rate()
{
    if(0)
    {
        dvpFrameCount frame_count;
        dvpStatus status = dvpGetFrameCount(m_device_handle, &frame_count);
        if (status != DVP_STATUS_OK)
        {
            return -1.0;
        }
        return static_cast<double>(frame_count.fFrameRate);
    }
    else
    {
        return static_cast<double>(m_trigger_fps);
    }
    
}

st_range dvp2_camera::get_frame_rate_range()
{
    return st_range(m_fps_min,m_fps_max);
}

int dvp2_camera::set_frame_rate(double frame_rate)
{
    dvpStatus status(DVP_STATUS_OK);
    if(0)
    {
        QString str = QString("%1").arg(frame_rate);
    	status = dvpSetConfigString(m_device_handle, "AcquisitionFrameRateValue", str.toStdString().c_str());
        
    }
    else
    {
        m_trigger_fps = static_cast<int>(frame_rate);
        double time_us = 1000000.0 / m_trigger_fps;
        status = dvpSetSoftTriggerLoop(m_device_handle, time_us);
		dvpGetSoftTriggerLoop(m_device_handle,&time_us);
        m_trigger_fps = static_cast<int>(1000000.0 / time_us + 0.5);
    }
    return map_ret_status(status);
}

int dvp2_camera::get_maximum_width()
{
    dvpSensorInfo sensor_info;
    dvpStatus status = dvpGetSensorInfo(m_device_handle, &sensor_info);
    if(status != DVP_STATUS_OK)
    {
        return -1;
    }
    return sensor_info.region.iMaxW;
    
}

int dvp2_camera::get_maximum_height()
{
    dvpSensorInfo sensor_info;
    dvpStatus status = dvpGetSensorInfo(m_device_handle, &sensor_info);
    if (status != DVP_STATUS_OK)
    {
        return -1;
    }
    return sensor_info.region.iMaxH;
}

int dvp2_camera::get_start_x()
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        return -1;
    }
    return region.X;
}

st_range dvp2_camera::get_start_x_range()
{
    return st_range(0, get_maximum_width() - get_width());
}

int dvp2_camera::set_start_x(int start_x)
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        if (!m_map_ret_status.contains(status))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[status];
    }
    region.X = start_x;
	status = dvpSetRoi(m_device_handle, region);
    return map_ret_status(status);
}

int dvp2_camera::get_start_y()
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        return -1;
    }
    return region.Y;
}

st_range dvp2_camera::get_start_y_range()
{
    return st_range(0, get_maximum_height() - get_height());
}

int dvp2_camera::set_start_y(int start_y)
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        if (!m_map_ret_status.contains(status))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[status];
    }
    region.Y = start_y;
    status = dvpSetRoi(m_device_handle, region);
    return map_ret_status(status);
}

int dvp2_camera::get_width()
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        return -1;
    }
    return region.W;
}

st_range dvp2_camera::get_width_range()
{
    return st_range(0, get_maximum_width() - get_start_x());
}

int dvp2_camera::set_width(int width)
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        if (!m_map_ret_status.contains(status))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[status];
    }
    region.W = width;
    status = dvpSetRoi(m_device_handle, region);
    return map_ret_status(status);
}

int dvp2_camera::get_height()
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        return -1;
    }
    return region.H;
}

st_range dvp2_camera::get_height_range()
{
    return st_range(0, get_maximum_height() - get_start_y());
}

int dvp2_camera::set_height(int height)
{
    dvpRegion region;
    dvpStatus status = dvpGetRoi(m_device_handle, &region);
    if (status != DVP_STATUS_OK)
    {
        if (!m_map_ret_status.contains(status))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[status];
    }
    region.H = height;
    status = dvpSetRoi(m_device_handle, region);
    return map_ret_status(status);
}

const QMap<QString, unsigned int>& dvp2_camera::enum_pixel_format()
{
    if(m_supported_formats.isEmpty())
    {
        dvpSelectionDescr des;
        dvpStatus status = dvpGetSourceFormatSelDescr(m_device_handle, &des);
        for (int i = 0;i < des.uCount;i++)
        {
            dvpFormatSelection fs;
            status = dvpGetSourceFormatSelDetail(m_device_handle, i, &fs);
            if(status == DVP_STATUS_OK)
            {
                m_supported_formats.insert(QString::fromStdString(fs.selection.string), fs.selection.iIndex);
            }
        }
    }
    return m_supported_formats;
}

QString dvp2_camera::get_pixel_format()
{
    dvpUint32 index(0);
	dvpStatus status = dvpGetSourceFormatSel(m_device_handle, &index);
    dvpFormatSelection fs;
    status = dvpGetSourceFormatSelDetail(m_device_handle, index, &fs);
    return L(fs.selection.string);
}

int dvp2_camera::set_pixel_format(unsigned int format)
{
    dvpStatus status = dvpSetSourceFormatSel(m_device_handle, format);
    return map_ret_status(status);
}

int dvp2_camera::set_pixel_format(const QString& sFormat)
{
    if (m_supported_formats.find(sFormat) == m_supported_formats.end())
    {
        return m_map_ret_status[STATUS_ERROR_PARAMETER];
    }
    return set_pixel_format(m_supported_formats[sFormat]);
}

const QMap<QString, unsigned int>& dvp2_camera::enum_supported_auto_exposure_mode()
{
    if (m_supported_auto_exposure_mode.isEmpty())
    {
        int cur_mode = 0;
        unsigned int supported_count = 0;
        int supported_values[64] = { 0 };
        dvpStatus  status = dvpGetEnumValue(m_device_handle, V_EXPOSURE_AUTO_E, &cur_mode, supported_values, &supported_count);
        for (int i = 0; i < (int)supported_count; i++)
        {
            dvpEnumDescr stEnumDescr;
            dvpGetEnumDescr(m_device_handle, V_EXPOSURE_AUTO_E, i, &stEnumDescr);
            QString name = QString::fromStdString(stEnumDescr.szEnumName);
            QMap<QString, QString>::iterator iter = m_global_map_auto_exposure_mode.find(name);
            if (iter != m_global_map_auto_exposure_mode.end())
            {
                m_map_auto_exposure_mode.insert(iter.key(), iter.value());
                m_supported_auto_exposure_mode.insert(m_map_auto_exposure_mode[name], supported_values[i]);
            }
            else
            {
                m_supported_auto_exposure_mode.insert(QString::fromStdString("error_mode"), supported_values[i]);
            }
        }
        if (m_supported_auto_exposure_mode.isEmpty())
        {
            m_supported_auto_exposure_mode.insert(global_auto_exposure_closed, cur_mode);
        }
    }
    return m_supported_auto_exposure_mode;
}

QString dvp2_camera::get_auto_exposure_mode()
{
    char sz_text[64] = { 0 };
    dvpStatus status = dvpGetEnumValueByString(m_device_handle, V_EXPOSURE_AUTO_E, sz_text);
    if(status != DVP_STATUS_OK)
    {
        return QString("error_mode");
    }
    QString mode = QString::fromStdString(sz_text);
    if(m_map_auto_exposure_mode.contains(mode))
    {
        return m_map_auto_exposure_mode[mode];
    }
    return QString("error_mode");
}

int dvp2_camera::set_auto_exposure_mode(QString auto_exposure_mode)
{
    //这里根据字符串设置
    QString key("");
    for (QMap<QString, QString>::iterator iter = m_map_auto_exposure_mode.begin(); iter != m_map_auto_exposure_mode.end();++iter)
    {
	    if(iter.value() == auto_exposure_mode)
	    {
            key = iter.key();
            break;
	    }
    }
    if(key.isEmpty())
    {
        return STATUS_ERROR_PARAMETER;
    }
    dvpStatus status = dvpSetEnumValueByString(m_device_handle, V_EXPOSURE_AUTO_E, key.toStdString().c_str());
    return map_ret_status(status);
}

bool dvp2_camera::is_auto_exposure_closed()
{
    return get_auto_exposure_mode() == global_auto_exposure_closed;
}

double dvp2_camera::get_auto_exposure_time_floor()
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if(status != DVP_STATUS_OK)
    {
        return -1.0;
    }
    return config.fExposureMin;
}

st_range dvp2_camera::get_auto_exposure_time_floor_range()
{
    st_range range;
    float exposure_time(0.0);
    dvpFloatDescr exposure_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_EXPOSURE_TIME_F, &exposure_time, &exposure_descr);
    if (status != DVP_STATUS_OK)
    {
        return range;
    }
    range.min = static_cast<double>(exposure_descr.fMin);
    range.max = get_auto_exposure_time_upper();
    return range;
}

int dvp2_camera::set_auto_exposure_time_floor(double exposure_time)
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if(status != DVP_STATUS_OK)
    {
        if (!m_map_ret_status.contains(status))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[status];
    }
    config.fExposureMin = exposure_time;
    status = dvpSetAeConfig(m_device_handle, config);
    return map_ret_status(status);
}

double dvp2_camera::get_auto_exposure_time_upper()
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if (status != DVP_STATUS_OK)
    {
        return -1.0;
    }
    return config.fExposureMax;
}

st_range dvp2_camera::get_auto_exposure_time_upper_range()
{
    st_range range;
    float exposure_time(0.0);
    dvpFloatDescr exposure_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_EXPOSURE_TIME_F, &exposure_time, &exposure_descr);
    if (status != DVP_STATUS_OK)
    {
        return range;
    }
    range.min = get_auto_exposure_time_floor();
    range.max = static_cast<double>(exposure_descr.fMax);
    return range;
}

int dvp2_camera::set_auto_exposure_time_upper(double exposure_time)
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if (status != DVP_STATUS_OK)
    {
        if (!m_map_ret_status.contains(status))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[status];
    }
    config.fExposureMax = exposure_time;
    status = dvpSetAeConfig(m_device_handle, config);
    return map_ret_status(status);
}

double dvp2_camera::get_exposure_time()
{
    float exposure_time(0.0);
    dvpFloatDescr exposure_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_EXPOSURE_TIME_F, &exposure_time, &exposure_descr);
    if (status != DVP_STATUS_OK)
    {
        return -1.0;
    }
    return static_cast<double>(exposure_time);
}

st_range dvp2_camera::get_exposure_time_range()
{
    st_range range;
    float exposure_time(0.0);
    dvpFloatDescr exposure_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_EXPOSURE_TIME_F, &exposure_time, &exposure_descr);
    if (status != DVP_STATUS_OK)
    {
        return range;
    }
    range.min = static_cast<double>(exposure_descr.fMin);
    range.max = static_cast<double>(exposure_descr.fMax);
    return range;
}

int dvp2_camera::set_exposure_time(double exposure_time)
{
    dvpStatus status = dvpSetFloatValue(m_device_handle, V_EXPOSURE_TIME_F, static_cast<float>(exposure_time));
    return map_ret_status(status);
}

const QMap<QString, unsigned int>& dvp2_camera::enum_supported_auto_gain_mode()
{
    if (m_supported_auto_gain_mode.isEmpty())
    {
        int cur_mode = 0;
        unsigned int supported_count = 0;
        int supported_values[64] = { 0 };
        dvpStatus  status = dvpGetEnumValue(m_device_handle, V_GAIN_AUTO_E, &cur_mode, supported_values, &supported_count);

        for (int i = 0; i < (int)supported_count; i++)
        {
            dvpEnumDescr stEnumDescr;
            dvpGetEnumDescr(m_device_handle, V_GAIN_AUTO_E, i, &stEnumDescr);
            QString name = QString::fromStdString(stEnumDescr.szEnumName);
            QMap<QString, QString>::iterator iter = m_global_map_auto_gain_mode.find(name);
            if (iter != m_global_map_auto_gain_mode.end())
            {
                m_map_auto_gain_mode.insert(iter.key(), iter.value());
                m_supported_auto_gain_mode.insert(m_map_auto_gain_mode[name], supported_values[i]);
            }
            else
            {
                m_supported_auto_gain_mode.insert(QString::fromStdString("error_mode"), supported_values[i]);
            }

        }
        if (m_supported_auto_gain_mode.isEmpty())
        {
            m_supported_auto_gain_mode.insert(global_auto_gain_closed, cur_mode);
        }
    }
    return m_supported_auto_gain_mode;
}

QString dvp2_camera::get_auto_gain_mode()
{
    char sz_text[64] = { 0 };
    dvpStatus status = dvpGetEnumValueByString(m_device_handle, V_GAIN_AUTO_E, sz_text);
    if (status != DVP_STATUS_OK)
    {
        return QString::fromStdString("关闭");
    }
    QString mode = QString::fromStdString(sz_text);
    if (m_map_auto_gain_mode.contains(mode))
    {
        return m_map_auto_gain_mode[mode];
    }
    return QString::fromStdString("关闭");
}

int dvp2_camera::set_auto_gain_mode(QString auto_gain_mode)
{
    QString key("");
    for (QMap<QString, QString>::iterator iter = m_map_auto_gain_mode.begin(); iter != m_map_auto_gain_mode.end(); ++iter)
    {
        if (iter.value() == auto_gain_mode)
        {
            key = iter.key();
            break;
        }
    }
    if (key.isEmpty())
    {
        return STATUS_ERROR_PARAMETER;
    }
    dvpStatus status = dvpSetEnumValueByString(m_device_handle, V_GAIN_AUTO_E, key.toStdString().c_str());
    return map_ret_status(status);
}

bool dvp2_camera::is_auto_gain_closed()
{
    return get_auto_gain_mode() == global_auto_gain_closed;
}

double dvp2_camera::get_auto_gain_floor()
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if (status != DVP_STATUS_OK)
    {
        return -1.0;
    }
    return static_cast<double>(config.fGainMin);
}

st_range dvp2_camera::get_auto_gain_floor_range()
{
    st_range range;
    float gain(0.0);
    dvpFloatDescr gain_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_GAIN_F, &gain, &gain_descr);
    if (status != DVP_STATUS_OK)
    {
        return range;
    }
    range.min = static_cast<double>(gain_descr.fMin);
    range.max = get_auto_gain_upper();
    return range;
}

int dvp2_camera::set_auto_gain_floor(double gain)
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if (status != DVP_STATUS_OK)
    {
        return map_ret_status(status);
    }
    config.fGainMin = gain;
    status = dvpSetAeConfig(m_device_handle, config);
    //dvpAeConfig cfg;
	//status = dvpGetAeConfig(m_device_handle, &cfg);      //测试设置是否生效,这里测试有效，但是关闭相机之后再打开，读取的下限被重置成1.0
    return map_ret_status(status);
}

double dvp2_camera::get_auto_gain_upper()
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if (status != DVP_STATUS_OK)
    {
        return -1.0;
    }
    return static_cast<double>(config.fGainMax);
}

st_range dvp2_camera::get_auto_gain_upper_range()
{
    st_range range;
    float gain(0.0);
    dvpFloatDescr gain_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_GAIN_F, &gain, &gain_descr);
    if (status != DVP_STATUS_OK)
    {
        return range;
    }
    range.min = get_auto_gain_floor();
    range.max = static_cast<double>(gain_descr.fMax);
    return range;
}

int dvp2_camera::set_auto_gain_upper(double gain)
{
    dvpAeConfig config;
    dvpStatus status = dvpGetAeConfig(m_device_handle, &config);
    if (status != DVP_STATUS_OK)
    {
        if (!m_map_ret_status.contains(status))
        {
            return STATUS_ERROR_UNKNOWN;
        }
        return m_map_ret_status[status];
    }
    config.fGainMax = gain;
    status = dvpSetAeConfig(m_device_handle, config);
    if (!m_map_ret_status.contains(status))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[status];
}

double dvp2_camera::get_gain()
{
    float gain(0.0);
    dvpFloatDescr gain_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_GAIN_F, &gain, &gain_descr);
    if (status != DVP_STATUS_OK)
    {
        return -1.0;
    }
    return static_cast<double>(gain);
}

st_range dvp2_camera::get_gain_range()
{
    st_range range;
    float gain(0.0);
    dvpFloatDescr gain_descr;
    dvpStatus status = dvpGetFloatValue(m_device_handle, V_GAIN_F, &gain, &gain_descr);
    if (status != DVP_STATUS_OK)
    {
        return range;
    }
    range.min = static_cast<double>(gain_descr.fMin);
    range.max = static_cast<double>(gain_descr.fMax);
    return range;
}

int dvp2_camera::set_gain(double gain)
{
    dvpStatus status = dvpSetFloatValue(m_device_handle, V_GAIN_F, static_cast<float>(gain));
    if (!m_map_ret_status.contains(status))
    {
        return STATUS_ERROR_UNKNOWN;
    }
    return m_map_ret_status[status];
}

/****************************采集控制******************************/
const QMap<QString, unsigned int>& dvp2_camera::enum_supported_trigger_mode()
{
    /*********************************
     * DVP2 的 SDK 通过 dvpGetTriggerState 来获取触发模式，该接口只能获取一个 bool 变量表示 连续模式(false) 和 触发模式(true)
     * 这里支持的触发模式固定为连续触发和触发一次
     *********************************/
    if (m_supported_trigger_mode.isEmpty())
    {
        m_supported_trigger_mode.insert(global_trigger_mode_continuous, 0);     //连续触发
        m_supported_trigger_mode.insert(global_trigger_mode_once, 1);           //触发一次
    }
    return m_supported_trigger_mode;
}

QString dvp2_camera::get_trigger_mode()
{
    if(0)
    {
        bool trigger_state = false;
        dvpStatus ret = dvpGetTriggerState(m_device_handle, &trigger_state);
        if (ret != DVP_STATUS_OK)
        {
            return global_trigger_mode_continuous;      //默认连续模式
        }
        if (trigger_state)
        {
            return global_trigger_mode_once;
        }
    }
    else
    {
	    //修改原来逻辑，根据循环触发是否开启判定是连续模式还是触发模式
        bool trigger_loop_state = false;
        dvpStatus ret = dvpGetSoftTriggerLoopState(m_device_handle, &trigger_loop_state);
        if (ret != DVP_STATUS_OK)
        {
            return global_trigger_mode_continuous;      //默认连续模式
        }
        if (!trigger_loop_state)
        {
            return global_trigger_mode_once;
        }
    }
    return global_trigger_mode_continuous;
}

int dvp2_camera::set_trigger_mode(QString trigger_mode)
{
    //这里根据 dvpGetSoftTriggerLoopState 判断是否是连续模式，而设备可能处于真正的连续模式(dvpGetTriggerState返回false)下
	//为了确保设置生效，需要确保 dvpGetTriggerState 返回 true
	//1.设置 TriggerState 为 true
    bool trigger_state = false;
    dvpStatus ret = dvpGetTriggerState(m_device_handle, &trigger_state);
    if (ret != DVP_STATUS_OK)
    {
		return map_ret_status(ret);
    }
    if (!trigger_state)
    {
        ret = dvpSetTriggerState(m_device_handle, true);
        if (ret != DVP_STATUS_OK)
        {
            return map_ret_status(ret);
        }
    }
    //2. 根据传入参数设置 SoftTriggerLoopState
    if (get_trigger_mode() != trigger_mode)
    {
        if (trigger_mode == global_trigger_mode_once)
        {
            dvpSetSoftTriggerLoopState(m_device_handle, false);
        }
        else
        {
            m_trigger_fps = std::max(m_trigger_fps, m_fps_min);
            m_trigger_fps = std::min(m_trigger_fps, m_fps_max);
            double time_us = 1000000.0 / m_trigger_fps;
            dvpStatus ret = dvpSetSoftTriggerLoop(m_device_handle, time_us);
            if (ret != DVP_STATUS_OK)
            {
                return map_ret_status(ret);
            }
            ret = dvpSetSoftTriggerLoopState(m_device_handle, true);
            return map_ret_status(ret);
        }
    }
    
    return STATUS_SUCCESS;
    /*
    if(get_trigger_mode() != trigger_mode)
    {
        if(0)
        {
            bool trigger_state(false);
            if (trigger_mode == global_trigger_mode_once)
            {
                trigger_state = true;
            }
            //触发一次时关闭循环触发，连续触发时打开循环触发
            dvpStatus ret = dvpSetSoftTriggerLoopState(m_device_handle, !trigger_state);
            if (ret != DVP_STATUS_OK)
            {
                return map_ret_status(ret);
            }
            ret = dvpSetTriggerState(m_device_handle, trigger_state);
            return map_ret_status(ret);
        }
        else
        {
            //修改逻辑，内部均设置为触发模式，界面上设置为触发模式时，禁用循环触发;界面上设置为连续模式时，开启循环触发，且根据帧率调整时间
            
            bool trigger_state = trigger_mode == global_trigger_mode_once;
            if(!trigger_state)
            {
                
            }
            ret = dvpSetSoftTriggerLoopState(m_device_handle, !trigger_state);
        	return map_ret_status(ret);
        }
    }
    */
}

const QMap<QString, unsigned int>& dvp2_camera::enum_supported_trigger_source()
{
    /*********************************
     * DVP2 的 SDK 通过 dvpGetTriggerSource 来获取触发源，该接口只能获取一个枚举变量表示当前触发源
     * 这里支持的触发源设置为所有的枚举值
     *********************************/
    if (m_supported_trigger_source.isEmpty())
    {
        m_supported_trigger_source.insert(global_trigger_source_software, TRIGGER_SOURCE_SOFTWARE);
        m_supported_trigger_source.insert(global_trigger_source_line1, TRIGGER_SOURCE_LINE1);
        m_supported_trigger_source.insert(global_trigger_source_line2, TRIGGER_SOURCE_LINE2);
        m_supported_trigger_source.insert(global_trigger_source_line3, TRIGGER_SOURCE_LINE3);
        m_supported_trigger_source.insert(global_trigger_source_line4, TRIGGER_SOURCE_LINE4);
        m_supported_trigger_source.insert(global_trigger_source_line5, TRIGGER_SOURCE_LINE5);
        m_supported_trigger_source.insert(global_trigger_source_line6, TRIGGER_SOURCE_LINE6);
        m_supported_trigger_source.insert(global_trigger_source_line7, TRIGGER_SOURCE_LINE7);
        m_supported_trigger_source.insert(global_trigger_source_line8, TRIGGER_SOURCE_LINE8);
    }
    return m_supported_trigger_source;
}

QString dvp2_camera::get_trigger_source()
{
    enum_supported_trigger_source();        //可能在初始化界面之前会先设置触发源，这里需要确保相关数组初始化
    dvpTriggerSource trigger_source;
	dvpStatus ret = dvpGetTriggerSource(m_device_handle, &trigger_source);
    if(ret != DVP_STATUS_OK)
    {
        return global_trigger_source_software;      //默认返回软触发
    }
    for (QMap<QString, unsigned int>::const_iterator iter = m_supported_trigger_source.cbegin();
        iter != m_supported_trigger_source.cend(); ++iter) 
    {
        if(iter.value() == trigger_source)
        {
            return iter.key();      //返回当前触发源
		}
    }
    return global_trigger_source_software;      //默认返回软触发
}

int dvp2_camera::set_trigger_source(QString trigger_source)
{
    if(get_trigger_source() != trigger_source)
    {
        if (!m_supported_trigger_source.contains(trigger_source))
        {
            return STATUS_ERROR_PARAMETER; // 如果不匹配任何触发源，返回参数错误 
        }
        dvpTriggerSource value = static_cast<dvpTriggerSource>(m_supported_trigger_source[trigger_source]);
        dvpStatus ret = dvpSetTriggerSource(m_device_handle, value);
        return map_ret_status(ret);
    }
    return STATUS_SUCCESS;
}

int dvp2_camera::start_grab()
{
    if(!m_is_grab_running.load())
    {
        dvpStatus ret = dvpStart(m_device_handle);
        if (ret != DVP_STATUS_OK)
        {
            return map_ret_status(ret);
        }
        m_is_grab_running.store(true);
    }
	return STATUS_SUCCESS;
}

int dvp2_camera::stop_grab()
{
    if(m_is_grab_running.load())
    {
        dvpStatus ret = dvpStop(m_device_handle);
        if (ret != DVP_STATUS_OK)
        {
            return map_ret_status(ret);
        }
        m_is_grab_running.store(false);
    }
    return STATUS_SUCCESS;
}

bool dvp2_camera::is_grab_running()
{
    return m_is_grab_running.load();
}

QImage dvp2_camera::get_image(int millisecond)
{
    dvpFrame frame;
    void* p(nullptr);       //帧数据首地址，不需要手动释放内存
    dvpStatus ret = dvpGetFrame(m_device_handle, &frame, &p, millisecond);
    if (ret != DVP_STATUS_OK)
    {
        return QImage();
    }
    if(0)   //测试保存到图像
    {
        dvpSavePicture(&frame, p, "D:/Temp/test.png", 100);
    }
    //将数据保存到 QImage
    QImage img;
    int stride(0);
    switch (frame.format)
    {
    case FORMAT_MONO:
    {
        stride = frame.iWidth; // 灰度图像的 stride 等于宽度
        img = QImage(static_cast<const uchar*>(p), frame.iWidth, frame.iHeight, stride, QImage::Format_Grayscale8).convertToFormat(QImage::Format_RGB888);
        if(0)   //将数据保存到图像
        {
            QString path = QString("D:/Temp/%1.png").arg(frame.uTimestamp);
            img.save(path);  // 保存为 PNG 文件
        }
        break;
    }
    case FORMAT_BGR24:
    {
        stride = frame.iWidth * 3; // 三通道图像的 stride 等于 宽度*3
        img = QImage(static_cast<uchar*>(p), frame.iWidth, frame.iHeight, stride, QImage::Format_BGR888).convertToFormat(QImage::Format_RGB888);
        if (0)   //将数据保存到图像
        {
            QString path = QString("D:/Temp/%1.png").arg(frame.uTimestamp);
            img.save(path);  // 保存为 PNG 文件
        }
        break;
    }
    default:
        // 不支持的格式
        qWarning("Unsupported image format: %d", frame.format);
        break;
    }
    return img;
}

QImage dvp2_camera::trigger_once()
{
    m_frame_ready.store(false);
    dvpStatus status = dvpTriggerFire(m_device_handle);
    if(status != DVP_STATUS_OK)
    {
        m_frame_ready.store(true);
        return QImage();
    }
    std::unique_lock<std::mutex> lock(m_frame_ready_mutex);
    if (!m_condition_variable.wait_for(
        lock, std::chrono::milliseconds(2000),      //超时2秒返回空图
        [&] { return m_frame_ready.load(); }))
    {
        m_frame_ready.store(true);
        return QImage();
    }
	return m_image;           // 获取最新帧
}

void dvp2_camera::set_frame_ready(bool is_ready)
{
    m_frame_ready.store(is_ready);
    if(is_ready)
    {
        m_condition_variable.notify_one();
    }
}

int dvp2_camera::stream_callback(dvpHandle handle, dvpStreamEvent event, void* lp_context, dvpFrame* lp_frame, void* lp_buf)
{
    dvp2_camera* lp_camera = static_cast<dvp2_camera*>(lp_context);
    if(lp_camera == nullptr || !lp_camera->m_is_grab_running.load())
    {
        return 0;   //返回 0 表示影像数据从设备中移除
	}
    QImage img;
    if(lp_frame != nullptr && lp_buf != nullptr)
    {
        int stride(0);
        switch (lp_frame->format)
        {
        case FORMAT_MONO:
        {
            stride = lp_frame->iWidth; // 灰度图像的 stride 等于宽度
            img = QImage(static_cast<const uchar*>(lp_buf), lp_frame->iWidth, lp_frame->iHeight, stride, QImage::Format_Grayscale8).copy();
        	if (0)   //将数据保存到图像
            {
                QString path = QString("C:/Temp/%1.png").arg(lp_frame->uTimestamp);
                img.save(path);  // 保存为 PNG 文件
            }
            break;
        }
        case FORMAT_BGR24:
        {
            stride = lp_frame->iWidth * 3; // 三通道图像的 stride 等于 宽度*3
            img = QImage(static_cast<uchar*>(lp_buf), lp_frame->iWidth, lp_frame->iHeight, stride, QImage::Format_BGR888).convertToFormat(QImage::Format_RGB888);
            //img = QImage(static_cast<uchar*>(lp_buf), lp_frame->iWidth, lp_frame->iHeight, stride, QImage::Format_BGR888).copy();
            if (0)   //将数据保存到图像
            {
                QString path = QString("D:/Temp/%1.png").arg(lp_frame->uTimestamp);
                img.save(path);  // 保存为 PNG 文件
            }
            break;
        }
        default:
            // 不支持的格式
            qWarning("Unsupported image format: %d", lp_frame->format);
            break;
        }
    }
    
    if(lp_camera != nullptr)
    {
        //触发模式下发送采集命令之前会置为 false，这里取图成功之后会置为 true
        if(!lp_camera->is_frame_ready())
        {
            lp_camera->set_current_image(img);
            lp_camera->set_frame_ready(true);
        }
        //连续模式下直接传输给外部线程
        else
        {
            //write_log("lp_camera->post_stream_image_ready......");
            emit lp_camera->post_stream_image_ready(lp_camera->m_unique_id,img);
        }
    }
    return 0;   //返回 0 表示影像数据从设备中移除
}

int dvp2_camera::set_ip_address(unsigned int ip, unsigned int subnet_mask, unsigned int default_gateway)
{
    return 0;
}

int dvp2_camera::set_ip_config(unsigned int type)
{
    return 0;
}

bool dvp2_camera::import_config(const st_camera_config& camera_config)
{
    if(m_unique_id != camera_config.unique_id)
    {
        return false;
	}
	set_frame_rate(camera_config.frame_rate);
    set_start_x(camera_config.start_x);
    set_start_y(camera_config.start_y);
	set_width(camera_config.width);
    set_height(camera_config.height);
	set_pixel_format(camera_config.pixel_format);
	set_auto_exposure_mode(camera_config.auto_exposure_mode);
	set_auto_exposure_time_floor(camera_config.auto_exposure_time_floor);
	set_auto_exposure_time_upper(camera_config.auto_exposure_time_upper);
	set_exposure_time(camera_config.exposure_time);
	set_auto_gain_mode(camera_config.auto_gain_mode);
	set_auto_gain_floor(camera_config.auto_gain_floor);
	set_auto_gain_upper(camera_config.auto_gain_upper);
	set_gain(camera_config.gain);
	return true;
}

st_camera_config dvp2_camera::export_config()
{
    st_camera_config camera_config;
	camera_config.unique_id = m_unique_id;
	camera_config.frame_rate = get_frame_rate();
	camera_config.start_x = get_start_x();
	camera_config.start_y = get_start_y();
	camera_config.width = get_width();
	camera_config.height = get_height();
	camera_config.pixel_format = get_pixel_format();
	camera_config.auto_exposure_mode = get_auto_exposure_mode();
	camera_config.auto_exposure_time_floor = get_auto_exposure_time_floor();
	camera_config.auto_exposure_time_upper = get_auto_exposure_time_upper();
	camera_config.exposure_time = get_exposure_time();
	camera_config.auto_gain_mode = get_auto_gain_mode();
	camera_config.auto_gain_floor = get_auto_gain_floor();
	camera_config.auto_gain_upper = get_auto_gain_upper();
	camera_config.gain = get_gain();
	return camera_config;
}