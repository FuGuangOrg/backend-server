/************************************************************************/
/* 以C++接口为基础，对常用函数进行二次封装，方便用户使用                */
/************************************************************************/
#pragma once

#include "camera_factory.h"
#include "grab_worker.h"


#include "MvCameraControl.h"
#include <string>

#include <QMap>
#include <QObject>

class DEVICE_CAMERA_EXPORT mvs_camera : public interface_camera
{
    Q_OBJECT
    mvs_camera(MV_CC_DEVICE_INFO* device_info, const QString& unique_id);
    virtual ~mvs_camera() override;
public:
    /******************************
     * 设备的关闭与打开操作依赖于设备句柄，但创建句柄和开关设备是两个独立的操作
     ******************************/
	virtual int create_device_handle() override;            //创建设备句柄
    virtual int open() override;                            //打开设备
    virtual int close() override;                           //关闭设备

    int set_ip_address(unsigned int ip, unsigned int subnet_mask, unsigned int default_gateway) override;//设置静态 ip
    int set_ip_config(unsigned int type) override;          //配置IP方式，同时包含设置动态IP的功能

    double get_frame_rate() override;                       //获取采集帧率
	st_range get_frame_rate_range() override;               //获取可设置的采集帧率范围
    int set_frame_rate(double frame_rate) override;         //设置采集帧率


    int get_maximum_width() override;                               //获取最大宽度
    int get_maximum_height() override;                              //获取最大高度

    int get_start_x() override;                                      //获取起点 X 坐标
    st_range get_start_x_range() override;                           //获取可设置的 X 坐标范围
    int set_start_x(int start_x) override;                           //设置起点 X 坐标

    int get_start_y() override;                                      //获取起点 Y 坐标
    st_range get_start_y_range() override;                           //获取可设置的 Y 坐标范围
    int set_start_y(int start_y) override;                           //设置起点 Y 坐标

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

    const QMap<QString, unsigned int>& enum_supported_auto_exposure_mode() override;            //枚举所有支持的自动曝光模式,只需要枚举一次
    QString get_auto_exposure_mode() override;                                                  //获取自动曝光模式
    int set_auto_exposure_mode(QString auto_exposure_mode) override;                           //设置自动曝光模式
    bool is_auto_exposure_closed() override;                                                    //自动曝光模式是否关闭

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
    QString get_auto_gain_mode() override;                                   //获取是否开启了自动增益  0--关闭 2--连续
    int set_auto_gain_mode(QString auto_gain_mode) override;             //设置是否开启自动增益 只能传0(关闭)或2(连续)
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


    virtual int start_grab() override;                                              //开始采集
    virtual int stop_grab() override;                                               //停止采集
	virtual bool is_grab_running() override;                                        //是否正在采集
    virtual QImage get_image(int millisecond) override;                             //采图函数,线程中执行

    virtual QImage trigger_once() override;                                            //触发一次

    //判断设备是否可达
    bool is_device_accessible(unsigned int nAccessMode) const;

    //判断相机是否处于连接状态
    bool is_device_connected();

    //注册图像数据回调
    int register_image_callback(void(__stdcall* cbOutput)(unsigned char* pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser), void* pUser) const;

    //释放图像缓存
    int free_image_buffer(MV_FRAME_OUT* lp_frame) const;

    //设置SDK内部图像缓存节点个数
    int set_image_node_num(unsigned int num) const;

    //获取设备信息,取流之前调用
    int get_device_info(MV_CC_DEVICE_INFO* device_info);

    //获取相机在网络层面的匹配信息，如数据包丢失率、网络延迟、丢帧情况等.
    //仅支持 GigE 相机和 USB 相机
    int get_all_match_info(void* info) const;

    //获取 GEV（GigE Vision）相机在网络层面的匹配信息，如数据包丢失率、网络延迟、丢帧情况等
    int get_gige_all_match_info(MV_MATCH_INFO_NET_DETECT* match_info_net_detect);

    //获取 U3V (USB3 Vision)相机在网络层面的匹配信息，如数据包丢失率、网络延迟、丢帧情况等
    int get_usb_all_match_info(MV_MATCH_INFO_USB_DETECT* match_info_usb_detect);






    //执行一次Command型命令，如 UserSetSave
    int execute_command(IN const char* command) const;

    //探测网络最佳包大小(只对GigE相机有效)
    int get_optimal_packet_size(unsigned int* optimal_packet_size) const;

    //注册消息异常回调
    int register_exception_callback(void(__stdcall* exception)(unsigned int msg_type, void* user), void* user) const;

    //注册单个事件回调
    int register_event_callback(const char* event_name, void(__stdcall* event)(MV_EVENT_OUT_INFO * event_info, void* user), void* user) const;

    //设置网络传输模式
    int set_net_transfer_mode(unsigned int mode) const;

    //像素格式转换
    int convert_pixel_format(MV_CC_PIXEL_CONVERT_PARAM_EX* convert_param) const;

    //保存图片
    int save_image(MV_SAVE_IMAGE_PARAM_EX3* param) const;

    //保存图片到文件
	int save_image_to_file(MV_CC_IMAGE* image, MV_CC_SAVE_IMAGE_PARAM* param, const char* file_path) const;

    //绘制圆形辅助线
    int draw_circle(MVCC_CIRCLE_INFO* circle_info) const;

    //绘制线形辅助线
    int draw_lines(MVCC_LINES_INFO* lines_info) const;

private:
    friend class camera_factory;

	void* m_device_handle{ nullptr };                           //设备句柄，通过设备句柄操作设备，例如打开或者关闭设备
    MV_CC_DEVICE_INFO* m_device_info{ nullptr };                //设备指针，外部传入。本类只负责相关操作，不负责资源释放

    QThread* m_grab_thread{ nullptr };                          //取图子线程
    grab_worker* m_grab_worker{ nullptr };                      //取图任务类对象
};
