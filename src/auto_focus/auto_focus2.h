/*********************************************************
 * 自动对焦模块 V2.0
 * 自动对焦时的移动始终只沿一个轴（主光轴）进行
 *（1）参数设置，只执行一次。用户设置一个范围 X0-->X1,使所有端面在该范围内由模糊-->清晰-->模糊变化，范围越小自动对焦速度越快
 * (2) 连续采集模式下缓慢移动相机.移动相机的同时进行拍照，需要连接信号槽，得到拍照结果
 * (3) 将每一张影像 frame 加入任务队列 task_queue,子线程从中获取数据，并使用 openmp 并行计算每张影像各个子区域的清晰度
 * (4) 每个目标对应一个局部区域.得到清晰度最大的局部影像作为每个目标的对焦结果，各个局部影像单独处理(不考虑约束)，或者依次处理(考虑约束) 
 * (5) 对于某个局部区域，如果其清晰度连续若干次下降(例如3或5)，或者遍历到列表末尾，表示已经找到最清晰的影像
 *********************************************************/
#pragma once
#include <string>
#include <vector>

#include "thread_calc_image_clarity.h"
#include "../device_camera/interface_camera.h"
#include "../motion_control/motion_control.h"
#include "../basic_algorithm/object_detector.h"

class AUTO_FOCUS_EXPORT auto_focus2: public QObject
{
	Q_OBJECT
public:
	//执行自动对焦,需要设置起始位置和忽略标记，某些端面可能无效，并不需要检测
	std::vector<st_focus_image> get_focus_images(const QString& save_dir, int index, int fiber_end_count, bool save_cache = false);
	//执行位置调整，使端面居中，获取使端面居中时需要偏移的像素
	void get_pixel_adjustment(int fiber_end_count, int position, int search_range, int move_speed, int move_step, 
		int& pixel_adjustment_x, int& pixel_adjustment_y, int& precision_position, bool save_cache = false);
	//std::vector<std::deque<st_focus_image>> get_cache_images() { return work_thread->get_cache_images(); }
	//清晰度标定
	void clarity_calibration();
	QMap<QString, double> clarity_thresh() { return work_thread->clarity_thresh(); }
	void set_clarity_thresh(const QMap<QString,double>& camera_claritys) { work_thread->set_clarity_thresh(camera_claritys); }
	void set_object_detector(object_detector* detector) { work_thread->set_object_detector(detector); }
	int max_position() const { return work_thread->max_position(); }
	int object_offset() const { return m_object_offset; }
	auto_focus2(motion_control* motion_control = nullptr, const std::vector<interface_camera*>& cameras = std::vector<interface_camera*>());

	void set_motion_control(motion_control* motion_control) { m_motion_control = motion_control; }

	void add_image(const QString& camera_id, const QImage& image);		//相机子线程-->主线程添加到任务队列

	void set_motion_parameters(int search_distance,int move_speed, int move_step);			//设置对焦参数，搜索距离，移动速度，和帧率

	void set_process_position(int position);

	double clarity_diff_thresh() { return work_thread->clarity_diff_thresh(); }
	void set_clarity_diff_thresh(double clarity_diff_thresh) { work_thread->set_clarity_diff_thresh(clarity_diff_thresh); }

	void save_cache_images(const QString& dst_dir,QString& sub_dir) { work_thread->save_cache_images(dst_dir, sub_dir); }
	std::vector<std::deque<cv::Mat>> get_cache_images() { return work_thread->get_cache_images(); }
	std::vector<int> get_focus_indexes() { return work_thread->get_focus_indexes(); }
	
	
private:
	motion_control* m_motion_control{ nullptr };		//运动控制,不负责资源管理
	//std::vector<QString> m_camera_ids;				//相机 id 列表，可以根据 id 区分影像属于哪个相机
	std::vector<interface_camera*> m_cameras;			//CPU/GPU 很弱时高帧率会影响界面响应速度，这里直接持有相机指针在对焦时设置帧率
	std::atomic<bool> m_capture{ false };
	thread_calc_image_clarity* work_thread{ nullptr };
	int m_object_offset{ 0 };                           //第一个相机中相邻端面之间的像素偏移，用于控制后续移动
	int m_start_position{ 0 };
	int m_end_position{ 0 };

	// 对焦参数
	int m_search_distance{ 330 };
	int m_move_speed{ 300 };
	int m_move_step{ 5 };
};
