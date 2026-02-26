#pragma once

#include <QImage>

#include "device_camera_global.h"
#include <string>
#include <QMap>
#include <QObject>

struct st_range
{
    double min{ 0 };
    double max{ 0 };
    st_range(double x0 = 0.0,double x1 = 0.0):min(x0),max(x1){ }
};

//相机操作返回值
enum CAMERA_RETURN_STATUS
{
	STATUS_SUCCESS = 0,         //操作成功
    STATUS_ERROR_HANDLE = 1,    //无效句柄
    STATUS_ERROR_SUPPORT = 2,    //不支持的功能
    STATUS_ERROR_PARAMETER = 3,     //错误参数

    STATUS_ERROR_UNKNOWN        //未知错误
};

//设置 IP 类型
enum IP_CONFIG
{
    IP_CONFIG_STATIC = 0,
	IP_CONFIG_DHCP = 1,
    IP_CONFIG_LLA = 2
};

enum TRIGGER_MODE
{
	TRIGGER_MODE_CONTINUOUS = 0,        //连续触发
    TRIGGER_MODE_ONCE = 1               //触发一次
};

//自动曝光模式
const QString global_auto_exposure_closed = QString::fromStdString("关闭");
const QString global_auto_exposure_once = QString::fromStdString("自动曝光一次");
const QString global_auto_exposure_continuous = QString::fromStdString("持续自动曝光");

//自动增益模式
const QString global_auto_gain_closed = QString::fromStdString("关闭");
const QString global_auto_gain_once = QString::fromStdString("自动一次");
const QString global_auto_gain_continuous = QString::fromStdString("持续自动");

//触发模式
const QString global_trigger_mode_once = QString::fromStdString("触发一次");
const QString global_trigger_mode_continuous = QString::fromStdString("连续触发");

//触发源
const QString global_trigger_source_software = QString::fromStdString("软触发");
const QString global_trigger_source_line1 = QString::fromStdString("线路1");
const QString global_trigger_source_line2 = QString::fromStdString("线路2");
const QString global_trigger_source_line3 = QString::fromStdString("线路3");
const QString global_trigger_source_line4 = QString::fromStdString("线路4");
const QString global_trigger_source_line5 = QString::fromStdString("线路5");
const QString global_trigger_source_line6 = QString::fromStdString("线路6");
const QString global_trigger_source_line7 = QString::fromStdString("线路7");
const QString global_trigger_source_line8 = QString::fromStdString("线路8");

//相机参数配置，相机设备参数<--->st_camera_config <--->XML文件
struct st_camera_config
{
    QString unique_id{ "" };						//相机唯一 ID
    double frame_rate{ 30.0 };						//采集帧率
    int start_x{ 0 };								//起点 X 坐标
    int start_y{ 0 };								//起点 Y 坐标
    int width{ 1920 };								//宽度
    int height{ 1080 };								//高度
    QString pixel_format{ "" };					//像素格式
    QString auto_exposure_mode{ "" };				//自动曝光模式
    double auto_exposure_time_floor{ 1.0 };			//自动增益下限
    double auto_exposure_time_upper{ 10000000.0 };	//自动增益上限
    double exposure_time{ 20000.0 };				//曝光时间，单位微秒(us)
    QString auto_gain_mode{ "" };					//自动增益模式
    double auto_gain_floor{ 0.0 };					//自动增益下限
    double auto_gain_upper{ 12.0 };					//自动增益上限
    double gain{ 0.0 };								//增益
    //const QString trigger_mode{ global_trigger_mode_continuous };		//触发模式,固定参数
    //const QString trigger_source{ global_trigger_source_software };		//触发源，固定参数
    st_camera_config() {}
};


class DEVICE_CAMERA_EXPORT interface_camera : public QObject
{
    Q_OBJECT
public:
    virtual ~interface_camera() = default;
    virtual int create_device_handle() = 0;
    virtual int open() = 0;         //打开相机
    virtual int close() = 0;        //关闭相机
    /***********************************参数设置*******************************/
    //不需要打开相机
    virtual int set_ip_address(unsigned int ip, unsigned int subnet_mask, unsigned int default_gateway) = 0;//设置静态 ip 地址
    virtual int set_ip_config(unsigned int type) = 0;               //设置动态 ip 地址

    //需要打开相机
    virtual double get_frame_rate() = 0;                                //获取采集帧率
    virtual st_range get_frame_rate_range() = 0;                        //获取可设置的采集帧率范围
    virtual int set_frame_rate(double frame_rate) = 0;                  //设置采集帧率

    virtual int get_maximum_width() = 0;                                //获取最大宽度
    virtual int get_maximum_height() = 0;                               //获取最大高度

    virtual int get_start_x() = 0;                                      //获取起点 X 坐标
    virtual st_range get_start_x_range() = 0;                           //获取可设置的 X 坐标范围
    virtual int set_start_x(int start_x) = 0;                           //设置起点 X 坐标

    virtual int get_start_y() = 0;                                      //获取起点 Y 坐标
    virtual st_range get_start_y_range() = 0;                           //获取可设置的 Y 坐标范围
    virtual int set_start_y(int start_y) = 0;                           //设置起点 Y 坐标

    virtual int get_width() = 0;                                        //获取宽度
    virtual st_range get_width_range() = 0;                             //获取可设置的宽度范围
    virtual int set_width(int width) = 0;                               //设置宽度
    
    virtual int get_height() = 0;                                       //获取高度
    virtual st_range get_height_range() = 0;                            //获取可设置的高度范围
    virtual int set_height(int height) = 0;                             //设置高度
    
    virtual const QMap<QString, unsigned int>& enum_pixel_format() = 0;     //枚举所有支持的像素格式,只需要枚举一次
    virtual QString get_pixel_format() = 0;                                 //获取像素格式
    virtual int set_pixel_format(unsigned int nValue) = 0;                  //设置像素格式
    virtual int set_pixel_format(const QString& sFormat) = 0;               //设置像素格式

    virtual const QMap<QString, unsigned int>& enum_supported_auto_exposure_mode() = 0;         //枚举所有支持的自动曝光模式,只需要枚举一次
    virtual QString get_auto_exposure_mode() = 0;                                               //获取自动曝光模式
    virtual int set_auto_exposure_mode(QString auto_exposure_mode) = 0;                    //设置自动曝光模式
    virtual bool is_auto_exposure_closed() = 0;                                                 //自动曝光模式是否关闭

    virtual double get_auto_exposure_time_floor() = 0;                  //获取自动曝光时间下限
    virtual st_range get_auto_exposure_time_floor_range() = 0;          //获取可设置的自动曝光时间下限范围
    virtual int set_auto_exposure_time_floor(double exposure_time) = 0; //设置自动曝光时间下限，开启自动曝光时可用

    virtual double get_auto_exposure_time_upper() = 0;                  //获取自动曝光时间上限
    virtual st_range get_auto_exposure_time_upper_range() = 0;          //获取可设置的自动曝光时间上限范围
    virtual int set_auto_exposure_time_upper(double exposure_time) = 0; //设置自动曝光时间上限，开启自动曝光时可用

    virtual double get_exposure_time() = 0;                             //获取曝光时间
    virtual st_range get_exposure_time_range() = 0;                     //获取可设置的曝光时间范围
    virtual int set_exposure_time(double exposure_time) = 0;            //设置曝光时间，关闭自动曝光时可用

    virtual const QMap<QString, unsigned int>& enum_supported_auto_gain_mode() = 0;     //枚举所有支持的自动增益模式,只需要枚举一次
    virtual QString get_auto_gain_mode() = 0;                                    //获取自动增益模式
    virtual int set_auto_gain_mode(QString auto_gain_mode) = 0;              //设置自动增益模式
    virtual bool is_auto_gain_closed() = 0;                                  //自动增益模式是否关闭

    virtual double get_auto_gain_floor() = 0;                           //获取自动增益下限
    virtual st_range get_auto_gain_floor_range() = 0;                   //获取可设置的自动增益下限范围
    virtual int set_auto_gain_floor(double gain) = 0;                   //设置自动增益下限，开启自动增益时可用

	virtual double get_auto_gain_upper() = 0;                           //获取自动增益上限
    virtual st_range get_auto_gain_upper_range() = 0;                   //获取可设置的自动增益上限范围
    virtual int set_auto_gain_upper(double gain) = 0;                   //设置自动增益上限，开启自动增益时可用

    virtual double get_gain() = 0;                                      //获取增益
    virtual st_range get_gain_range() = 0;                              //获取可设置的增益范围
    virtual int set_gain(double gain) = 0;                              //设置增益，关闭自动增益时可用

    /***********************************采集控制*************************************/
    virtual const QMap<QString, unsigned int>& enum_supported_trigger_mode() = 0;       //枚举所有支持的触发模式
    virtual QString get_trigger_mode() = 0;                                             //获取触发模式
    virtual int set_trigger_mode(QString trigger_mode) = 0;                             //设置触发模式

    virtual const QMap<QString, unsigned int>& enum_supported_trigger_source() = 0;     //枚举所有支持的触发源
    virtual QString get_trigger_source() = 0;                                           //获取触发源
    virtual int set_trigger_source(QString trigger_source) = 0;                         //设置触发源

    virtual int start_grab() = 0;                                           //开始采集
    virtual int stop_grab() = 0;                                            //停止采集
	virtual bool is_grab_running() = 0;                                     //是否正在采集
    virtual QImage get_image(int millisecond) = 0;                          //采图函数

    virtual QImage trigger_once() = 0;                                        //触发一次,返回采集的图像

    virtual bool import_config(const st_camera_config& camera_config) = 0;  //导入参数并设置到相机
    virtual st_camera_config export_config() = 0;                           //导出相机参数
signals:
    void post_stream_image_ready(const QString& camera_id, const QImage& image);
public:
    QMap<int, int> m_map_ret_status;                                //返回值映射，不同的相机对于某一状态的返回值很可能不一样，因此需要对所有相机进行统一映射
    int map_ret_status(int ret) const
    {
        if (m_map_ret_status.contains(ret))
        {
            return m_map_ret_status[ret];
        }
        return STATUS_ERROR_UNKNOWN;
	}
	QMap<QString, unsigned int> m_supported_formats;                //支持的像素格式，该内容只需要获取一次

    /************************************
     *  自动曝光模式字符串映射,不同的相机对应的字符串可能不同。例如:相机 A 关闭字符串为 OFF，而相机 B 为 Disable
     *  为了将所有字符串统一映射到 "关闭"/"自动曝光一次"/"持续自动曝光"，这里先注册全局映射，里面包含所有相机可能出现的字符串
     *  然后针对每个相机，再创建一个独立的映射，例如全局映射包含"OFF"-->"关闭", "Disable"-->"关闭"，
     *  在界面上针对某个相机设置"关闭"时，匹配的字符串可能是"OFF"/"Disable",这会导致设置失败。因此每个相机还需要
     *  一个特有的映射关系。增益模式同理
     ***********************************/
    QMap<QString, QString> m_global_map_auto_exposure_mode;         //全局映射，对所有相机进行统一映射。初始化时创建
    QMap<QString, QString> m_map_auto_exposure_mode;                //当前相机映射，打开相机之后枚举曝光模式时才能创建
    QMap<QString, unsigned int> m_supported_auto_exposure_mode;     //支持的自动曝光模式，该内容只需要获取一次，记录统一映射后的字符串

    QMap<QString, QString> m_global_map_auto_gain_mode;             //全局映射，对所有相机进行统一映射。初始化时创建
    QMap<QString, QString> m_map_auto_gain_mode;                    //当前相机映射，打开相机之后枚举曝光模式时才能创建
    QMap<QString, unsigned int> m_supported_auto_gain_mode;         //支持的自动增益模式，该内容只需要获取一次，记录统一映射后的字符串

    QMap<QString, QString> m_map_trigger_mode;                      //触发模式字符串映射，设置不同的触发模式会影响到其它控件状态，因此需要对所有相机进行统一映射
    QMap<QString, unsigned int> m_supported_trigger_mode;           //支持的触发模式

    QMap<QString, QString> m_map_trigger_source;                    //触发源字符串映射，只对所有相机的软触发源进行统一映射
    QMap<QString, unsigned int> m_supported_trigger_source;         //支持的触发源

    int m_sdk_type{ 0 };                                                    // SDK 类型
	QString m_unique_id{ "" };                                           // 相机唯一 ID
	bool m_is_opened{ false };                                              // 相机是否已打开
    std::atomic<bool> m_is_grab_running{ false };                    // 是否正在采集

};
