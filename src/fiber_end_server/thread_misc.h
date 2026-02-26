/********************
 * 其他任务线程,打开相机、设置相机参数、对焦、拍照等
 ********************/

#pragma once

#include "work_threads.h"
#include "device_manager.hpp"
#include "config.hpp"
#include "../device_camera/camera_factory.h"
#include "camera_config_mgr.hpp"
#include "../common/image_shared_memory.h"
#include "../auto_focus/auto_focus.h"
#include "../basic_algorithm/fiber_end_algorithm.h"

#ifdef MOTION_CONTROL_PLC
#include "../motion_control/motion_control_plc.h"
#else
#include "../motion_control/motion_control_port.h"
#endif
class thread_misc : public thread_base
{
	Q_OBJECT
public:
    thread_misc(QString name, QObject* parent = nullptr);
	virtual ~thread_misc() override;
	void set_device_manager(device_manager* manager) { m_device_manager = manager; }

	interface_camera* camera() const { return m_camera; }						//获取相机对象)
	st_config_data* config_data() const { return m_config_data; }				//获取配置参数
	motion_control* get_motion_control() const { return m_motion_control; }		//获取运控模块
	st_basic_algorithm_parameter* algorithm_parameter() const { return m_fiber_end_detector->algorithm_parameter(); }

	/***********************
	 * 初始化功能，包括初始化运控模块、自动对焦对象模块和自动检测模块
	 ***********************/
	bool initialize(st_config_data* config_data);

	void setup_camera_config_mgr();									//初始化相机参数管理器
	bool setup_motion_control(st_config_data* config_data);			//初始化运控模块
	bool setup_fiber_end_detector();								//初始化算法检测模块

	bool load_user_config_file(const QString& file_path);			//加载用户配置文件
	bool save_user_config_file(const QString& file_path);			//保存用户配置文件

	//将相机参数转换为 JSON 对象，发送给前端
	static QJsonObject camera_parameter_to_json(interface_camera* camera);
	static QJsonObject range_to_json(const st_range& range);

	/**************************************
	 * 开始运行. 相机依次移动到位置列表，执行拍照-自动对焦-异常检测任务.（在此之前需要执行一次复位操作）
	 * 返回值: 0 -- 执行完毕 1 -- 重复下发返回值
	 * const QString& request_id -- 客户端发送的消息唯一标识符, 这里需要多次阶段性回复消息，因此需要依据该变量回复消息
	 **************************************/
	int start_process(const QString& request_id);
	bool move_to_position(int pos_x, int pos_y, const QString& request_id,bool task_finish = false);//移动相机位置，拍照并回复消息
	bool anomaly_detection(const QString& request_id, int index, bool is_task_finish = true);		//异常检测，多次回复(每一张影像检测完毕的)消息
	std::atomic<bool> m_is_processing{ false };		//运行标识，正在运行时为 true
	std::atomic<bool> m_terminate{ false };			//中断标识,用户中断任务时该值置为true,任务函数(start_process)不断监视该变量以响应中断操作
protected:
    void process_task(const QVariant& task_data) override;

public slots:
	void on_stream_image_ready(const QString& camera_id, const QImage& img);	//连续模式下取图成功，写入共享内存然后发送给前端
	void on_device_request_start_process();										//接收设备开关发送的信号，开始检测任务
private:
	st_camera_config_mgr m_camera_config_mgr;				//相机配置管理器，用于保存和加载相机参数
	interface_camera* m_camera{ nullptr };					//相机对象，用于执行打开相机、设置参数等操作
	motion_control* m_motion_control{ nullptr };			//运控对象，用于移动相机
	auto_focus* m_auto_focus{ nullptr };					//自动对焦模块
	device_manager* m_device_manager{ nullptr };			//设备管理器，用于存储和管理设备信息
	st_config_data* m_config_data{ nullptr };				//服务配置参数,存储一些配置信息，例如拍照位置，保存路径，每张影像上的端面数量等
	fiber_end_algorithm* m_fiber_end_detector{ nullptr };	//端面检测器，指定影像数据，输出检测结果
	
	image_shared_memory m_shared_memory_trigger_image{ "trigger_image" };	// 共享内存对象，用于传输拍照得到的图像数据
	image_shared_memory m_shared_memory_detect_image{ "detect_image" };		// 共享内存对象，用于传输检测的图像数据

	int save_focus_image{ 0 };					//保存自动对焦的影像
	//bool m_calc_image_clarity{ false };			//调试参数，在移动相机的同时计算影像清晰度
	//double m_max_clarity{ 0.0 };				//调试参数，保存移动相机时清晰度最高的影像
	//std::vector<QImage> m_clarity_images;		//调试参数，保存移动相机时拍摄的影像
	QString m_stream_request_id{ "" };		//连续模式下采图之后需要持续发送给指定客户端，记录下发开始采集命令的客户端请求id

};