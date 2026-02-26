/************************************************************************/
/* 以C++接口为基础，对常用函数进行二次封装，方便用户使用                */
/************************************************************************/
#pragma once
#include <mutex>

#include "DVPCamera.h"
#include "camera_factory.h"

#include <string>
#include <QMap>
#include <QObject>

class DEVICE_CAMERA_EXPORT dvp2_camera : public interface_camera
{
    dvp2_camera(const QString& user_name, const QString& friendly_name, const QString& unique_id);
    virtual ~dvp2_camera() override;
public:
    /******************************
     * 设备的关闭与打开操作依赖于设备句柄，但创建句柄和开关设备是两个独立的操作
     ******************************/
    virtual int create_device_handle() override;            //创建设备句柄
    virtual int open() override;                            //打开设备,通过枚举信息的"设备名称"和"友好设备名称"打开设备
    virtual int close() override;                           //关闭设备

    int set_ip_address(unsigned int ip, unsigned int subnet_mask, unsigned int default_gateway) override;//设置静态 ip
    int set_ip_config(unsigned int type) override;          //配置IP方式，同时包含设置动态IP的功能

    double get_frame_rate() override;                       //获取采集帧率
    st_range get_frame_rate_range() override;               //获取可设置的采集帧率范围
    int set_frame_rate(double frame_rate) override;         //设置采集帧率


    int get_maximum_width() override;                       //获取最大宽度
    int get_maximum_height() override;                      //获取最大高度

    int get_start_x() override;                             //获取起点 X 坐标
    st_range get_start_x_range() override;                  //获取可设置的 X 坐标范围
    int set_start_x(int start_x) override;                  //设置起点 X 坐标

    int get_start_y() override;                             //获取起点 Y 坐标
    st_range get_start_y_range() override;                  //获取可设置的 Y 坐标范围
    int set_start_y(int start_y) override;                  //设置起点 Y 坐标

    int get_width() override;                               //获取宽度
    st_range get_width_range() override;                    //获取可设置的宽度范围
    int set_width(int width) override;                      //设置宽度

    int get_height() override;                              //获取高度
    st_range get_height_range() override;                   //获取可设置的高度范围
    int set_height(int height) override;                    //设置高度

    const QMap<QString, unsigned int>& enum_pixel_format() override;     //枚举所有支持的像素格式
    QString get_pixel_format() override;                         //获取像素格式
    int set_pixel_format(unsigned int format) override;         //设置像素格式
    int set_pixel_format(const QString& sFormat) override;

	const QMap<QString, unsigned int>& enum_supported_auto_exposure_mode() override;          //枚举所有支持的自动曝光模式,只需要枚举一次
    QString get_auto_exposure_mode() override;                                               //获取自动曝光模式
    int set_auto_exposure_mode(QString auto_exposure_mode) override;                         //设置自动曝光模式
	bool is_auto_exposure_closed() override;                                                 //自动曝光模式是否关闭

    double get_auto_exposure_time_floor() override;                 //获取自动曝光时间下限
    st_range get_auto_exposure_time_floor_range() override;         //获取可设置的自动曝光时间下限范围
    int set_auto_exposure_time_floor(double exposure_time) override;     //设置自动曝光时间下限，开启自动曝光时可用

    double get_auto_exposure_time_upper() override;                     //获取自动曝光时间上限
    st_range get_auto_exposure_time_upper_range() override;             //获取可设置的自动曝光时间上限范围
    int set_auto_exposure_time_upper(double exposure_time) override;    //设置自动曝光时间上限，开启自动曝光时可用

    double get_exposure_time() override;                                //获取曝光时间
    st_range get_exposure_time_range() override;                        //获取可设置的曝光时间范围
    int set_exposure_time(double exposure_time) override;                    //设置曝光时间，关闭自动曝光时可用

    const QMap<QString, unsigned int>& enum_supported_auto_gain_mode() override;     //枚举所有支持的自动增益模式,只需要枚举一次
    QString get_auto_gain_mode() override;                                          //获取增益模式
    int set_auto_gain_mode(QString auto_gain_mode) override;                             //设置增益模式
    bool is_auto_gain_closed() override;

    double get_auto_gain_floor() override;                          //获取自动增益下限
    st_range get_auto_gain_floor_range() override;                  //获取可设置的自动增益下限范围
    int set_auto_gain_floor(double gain) override;                  //设置自动增益下限，开启自动增益时可用

    double get_auto_gain_upper() override;                          //获取自动增益上限
    st_range get_auto_gain_upper_range() override;                  //获取可设置的自动增益上限范围
    int set_auto_gain_upper(double gain) override;                  //设置自动增益上限，开启自动增益时可用

    double get_gain() override;                                     //获取增益
    st_range get_gain_range() override;                             //获取可设置的增益范围
    int set_gain(double gain) override;                             //设置增益，关闭自动增益时可用

    /*******************************采集控制**********************************/
    virtual const QMap<QString, unsigned int>& enum_supported_trigger_mode() override;          //枚举所有支持的触发模式
    virtual QString get_trigger_mode() override;                                                //获取触发模式
    virtual int set_trigger_mode(QString trigger_mode) override;                                //设置触发模式

    virtual const QMap<QString, unsigned int>& enum_supported_trigger_source() override;        //枚举所有支持的触发源
    virtual QString get_trigger_source() override;                                              //获取触发源
    virtual int set_trigger_source(QString trigger_source) override;                            //设置触发源

    virtual int start_grab() override;                                                          // 开始采集
    virtual int stop_grab() override;                                                           // 停止采集
    virtual bool is_grab_running() override;                                                    // 是否正在采集

    virtual QImage get_image(int millisecond) override;                             // 采图函数
    bool is_frame_ready() { return m_frame_ready.load(); }                          // 触发模式下采图时判断是否采图完毕
    void set_frame_ready(bool is_ready);
    QImage current_image() { return m_image; }
    void set_current_image(const QImage& img) { m_image = img; }
	// 回调函数，在其他线程中执行(注册时指定类型为 STREAM_EVENT_FRAME_THREAD ). 获取视频流
	static int stream_callback(dvpHandle handle, dvpStreamEvent event, void* lp_context, dvpFrame* lp_frame, void* lp_buf);

    virtual QImage trigger_once() override;                                            //触发一次

    //判断设备是否可达
    bool is_device_accessible(unsigned int nAccessMode) const;

    //判断相机是否处于连接状态
    bool is_device_connected();

    virtual bool import_config(const st_camera_config& camera_config) override;
    virtual st_camera_config export_config() override; 

private:
    friend class camera_factory;
	QString m_user_name{ "" }, m_friendly_name{ "" };    //设备名称和友好设备名称，SDK优先通过设备名称打开相机，其次通过友好设备名称打开相机
    dvpHandle m_device_handle{ 0 };                             //设备句柄，通过设备句柄操作设备，例如打开或者关闭设备
    std::mutex m_frame_ready_mutex;                             
    std::condition_variable m_condition_variable;               //条件变量，触发模式取图时阻塞当前线程
    std::atomic<bool> m_frame_ready{ true };             //触发模式下的取图标识
    int m_trigger_fps{ 10 };                                    //连续模式下取图的帧率
    int m_fps_min{ 1 }, m_fps_max{ 60 };                        //连续模式下取图的帧率范围(CPU性能不足时不能使用太高的帧率，否则容易卡死)
    QImage m_image;                                             //触发模式取图的影像
};
