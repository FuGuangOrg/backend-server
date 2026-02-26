#include "auto_focus2.h"

auto_focus2::auto_focus2(motion_control* motion_control, const std::vector<interface_camera*>& cameras) :
	m_motion_control(motion_control), m_cameras(cameras)
{
	work_thread = new thread_calc_image_clarity(L("calc image clarity thread"), this);
	work_thread->start();
}

std::vector<st_focus_image> auto_focus2::get_focus_images(const QString& save_dir, int index, int fiber_end_count, bool save_cache)
{
	std::vector<st_focus_image> ret_images;
	std::vector<QString> camera_ids;
	for (size_t i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		camera_ids.emplace_back(m_cameras[i]->m_unique_id);
	}
	if (!work_thread->reset_auto_focus(camera_ids, fiber_end_count, save_dir, index, save_cache))
	{
		return ret_images;
	}
	write_log(l(QString("auto focus start_position_z = %1, end_position_z = %2").arg(m_start_position).arg(m_end_position)).c_str());
	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	m_motion_control->move_position(0, m_start_position, 5000);
	std::chrono::steady_clock::time_point end = std::chrono::high_resolution_clock::now();
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	write_log(l(QString("preparing use time %1  ms").arg(duration_ms.count())).c_str());
	
	start = std::chrono::high_resolution_clock::now();
	//设置硬触发
	for (int i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		m_cameras[i]->set_trigger_mode(global_trigger_mode_once);
		m_cameras[i]->set_trigger_source(global_trigger_source_line1);
	}
	m_capture.store(true);
	end = std::chrono::high_resolution_clock::now();
	duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	write_log(l(QString("set camera use time %1  ms").arg(duration_ms.count())).c_str());

	start = std::chrono::high_resolution_clock::now();
	m_motion_control->move_position(0, m_end_position, m_move_speed, m_move_step);
	end = std::chrono::high_resolution_clock::now();
	duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	write_log(l(QString("move_position use time %1  ms").arg(duration_ms.count())).c_str());
	start = std::chrono::high_resolution_clock::now();
	bool is_finish = work_thread->m_finished.load();
	bool is_calculating_finished = work_thread->m_calculate_finish.load();
	int task_image_count = work_thread->task_image_count();
	bool is_detect_fail = work_thread->m_object_detect_fail.load();
	while (true)
	{
		is_finish = work_thread->m_finished.load();
		is_calculating_finished = work_thread->m_calculate_finish.load();
		task_image_count = work_thread->task_image_count();
		is_detect_fail = work_thread->m_object_detect_fail.load();
		if (is_calculating_finished && (task_image_count < 1 || is_detect_fail))
		{
			break;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			continue;
		}
	}
	end = std::chrono::high_resolution_clock::now();
	duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	write_log(l(QString("wait for result use time %1  ms").arg(duration_ms.count())).c_str());
	QString info = QString("parameter status:   is_finish: %1 -- is_calculating_finished: %2 -- task_image_count: %3 -- is_detect_fail: %4")
		.arg(is_finish).arg(is_calculating_finished).arg(task_image_count).arg(is_detect_fail);
	write_log(l(info).c_str());
	m_capture.store(false);
	//恢复软触发
	start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		m_cameras[i]->set_trigger_mode(global_trigger_mode_continuous);
		m_cameras[i]->set_trigger_source(global_trigger_source_software);
	}
	end = std::chrono::high_resolution_clock::now();
	duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	write_log(l(QString("reset camera use time %1  ms").arg(duration_ms.count())).c_str());
	work_thread->save_images();
	if (1)			// 调试功能，打印对焦结果清晰度，同时便于检测队列中影像是否处理完毕
	{
		// 每个端面的最清晰值
		std::string title = "claritys : ";
		std::ostringstream oss;
		for (int i = 0; i < work_thread->focus_image_claritys().size(); i++)
		{
			oss << work_thread->focus_image_claritys()[i] << " ";
		}
		std::string result = title + oss.str();
		write_log(result.c_str());
		//每个端面参与计算的帧数
		//QString str = QString("calc_frame_count: (max_frame_count: %1)").arg(work_thread->max_frame_count());
		//title = l(str);
		//std::ostringstream oss2;
		//for (int i = 0; i < work_thread->calc_frame_count().size(); i++)
		//{
		//	oss2 << work_thread->calc_frame_count()[i] - 1 << " ";
		//}
		//result = title + oss2.str();
		//write_log(result.c_str());
	}
	ret_images = work_thread->focus_images();
	return ret_images;
}

void auto_focus2::clarity_calibration()
{
	//设置硬触发
	for (size_t i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		m_cameras[i]->set_trigger_mode(global_trigger_mode_once);
		m_cameras[i]->set_trigger_source(global_trigger_source_line1);
	}
	std::vector<QString> camera_ids;
	for (size_t i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		camera_ids.emplace_back(m_cameras[i]->m_unique_id);
	}
	/****************
	 * 第一次运动，计算大影像清晰度，
	 * 这里分为两种情况:(1) 对于 X 轴存在误差的设备，需要进行粗定位获取端面才能进行后续处理，
	 *						此时根据大影清晰度得到粗定位清晰度阈值
	 *					(2) 对于 X 轴能精细控制的设备，使用固定的分割策略而非粗定位,
	 *						此时根据大影清晰度得到跳过帧率的清晰度阈值
	 * 目前 YSOD 使用的产品属于情况(2),  ACCE 使用的产品属于情况(1)
	 ***********************/
	{
		if (!work_thread->reset_calculate_image_clarity(camera_ids))
		{
			return;
		}
		m_motion_control->move_position(0, m_start_position, 5000);
		m_capture.store(true);
		m_motion_control->move_position(0, m_end_position, m_move_speed, m_move_step);
		while (work_thread->task_image_count() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		m_capture.store(false);
		//根据产品类型决定设置的参数
		//验证结果
		if (0)
		{
			const QMap<QString, double>& clarity_thresh = work_thread->clarity_thresh();
			for (QMap<QString, double>::const_iterator iter = clarity_thresh.begin(); iter != clarity_thresh.end(); ++iter)
			{
				QString info = QString("camera id: %1 -- positioning clarity threshold: %2")
					.arg(iter.key()).arg(iter.value());
				write_log(l(info).c_str());
			}
		}

	}
	/****************2.第二次运动，粗定位，然后得到每个端面最大清晰度**********************/
	{
		if (!work_thread->reset_clarity_calibration(camera_ids))
		{
			return;
		}
		m_motion_control->move_position(0, m_start_position, 5000);
		m_capture.store(true);
		m_motion_control->move_position(0, m_end_position, m_move_speed, m_move_step);
		while (work_thread->task_image_count() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		m_capture.store(false);
		//得到最大清晰度最小值，取其一半作为清晰度差值阈值
		double clarity_diff_thresh(DBL_MAX);
		for (size_t i = 0; i < work_thread->focus_image_claritys().size(); i++)
		{
			clarity_diff_thresh = std::min(clarity_diff_thresh, work_thread->focus_image_claritys()[i]);
		}
		work_thread->set_clarity_diff_thresh(0.5 * clarity_diff_thresh);
		if (0)
		{
			QString info = QString("clarity_diff_thresh : %1").arg(work_thread->clarity_diff_thresh());
			write_log(l(info).c_str());
		}
	}

	//恢复软触发
	for (size_t i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		m_cameras[i]->set_trigger_mode(global_trigger_mode_continuous);
		m_cameras[i]->set_trigger_source(global_trigger_source_software);
	}
}

void auto_focus2::get_pixel_adjustment(int fiber_end_count, int position, int search_range, int move_speed, int move_step,
	int& pixel_adjustment_x, int& pixel_adjustment_y, int& precision_position,bool save_cache)
{
	m_object_offset = 0;
	std::vector<QString> camera_ids;
	for (size_t i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		camera_ids.emplace_back(m_cameras[i]->m_unique_id);
	}
	for (int i = 0; i < m_cameras.size(); i++)
	{
		if (m_cameras[i] == nullptr)
		{
			continue;
		}
		m_cameras[i]->set_trigger_mode(global_trigger_mode_once);
		m_cameras[i]->set_trigger_source(global_trigger_source_line1);
	}
	if (!work_thread->reset_pixel_adjustment(camera_ids,fiber_end_count, save_cache))
	{
		return;
	}
	int start_position = position - search_range;
	int end_position = position + search_range;
	m_motion_control->move_position(0, start_position, 5000);
	//write_log(l(QString("start_position = %1").arg(start_position)).c_str());
	m_capture.store(true);
	m_motion_control->move_position(0, end_position, move_speed, move_step);
	//write_log(l(QString("end_position = %1,move_speed = %2").arg(end_position).arg(move_speed)).c_str());
	while (work_thread->task_image_count() > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	m_capture.store(false);
	precision_position = start_position + move_step * work_thread->max_clarity_frame_index();
	//得到最清晰影像，进行粗定位并计算x-y方向偏移
	cv::Mat frame_data = work_thread->max_clarity_frame();
	//在当前影像上进行粗定位-纠偏，最终结果来自于清晰度最大的影像
	cv::Mat frame_rgb;
	cv::cvtColor(frame_data, frame_rgb, cv::COLOR_GRAY2RGB);
	std::vector<st_detect_box> boxes = work_thread->object_detector_ptr()->detect_objects(frame_rgb);
	if (boxes.size() < 1)       //粗定位失败
	{
		pixel_adjustment_x = pixel_adjustment_y = 0;
		return;
	}
	else
	{
		//当前影像的端面位置记录于 boxes, 按照从左到右排序并外扩 80 像素
		std::sort(boxes.begin(), boxes.end(), st_detect_box::sort_by_x0);
		//对粗定位结果外扩，确保包含完整端面
		for (size_t i = 0; i < boxes.size(); i++)
		{
			boxes[i].m_x0 = std::max(0.0, boxes[i].m_x0 - work_thread->object_expand());
			boxes[i].m_y0 = std::max(0.0, boxes[i].m_y0 - work_thread->object_expand());
			boxes[i].m_x1 = std::min(static_cast<double>(frame_rgb.cols - 1), boxes[i].m_x1 + work_thread->object_expand());
			boxes[i].m_y1 = std::min(static_cast<double>(frame_rgb.rows - 1), boxes[i].m_y1 + work_thread->object_expand());
		}
		predict_missing(boxes);
		//尺寸约束:如果粗定位结果不是预期尺寸，移除该结果
		for (int i = boxes.size() - 1; i >= 0; i--)
		{
			double box_width = boxes[i].m_x1 - boxes[i].m_x0;
			double box_height = boxes[i].m_y1 - boxes[i].m_y0;
			if (box_width < work_thread->object_detector_ptr()->target_size_x() - 5.0 || 
				box_height < work_thread->object_detector_ptr()->target_size_y() - 5.0)
			{
				boxes.erase(boxes.begin() + i);
				continue;
			}
		}
	}
	if (boxes.size() < 1)       //粗定位失败
	{
		pixel_adjustment_x = pixel_adjustment_y = 0;
		return;
	}
	/**************************************************
	 * 现在所有的端面区域记录于 boxes，计算像素偏移
	 * 已知端面数量 num，设定端面数量为 N，第一个端面与最后一个端面在影像上的像素坐标分别为 x0 和 x1(包围盒右侧坐标)
	 * 两个端面之间的平均像素间距为 D = (x1 - x0) / (num - 1)
	 * 需要补充的端面数量为 M = N - num，因此补充之后端面在影像上的坐标跨度为L = x1 - x0 + D*M +width(第一个包围盒宽度)
	 * 影像尺寸为 W，因此第一个端面包围盒左部在影像中的像素坐标为 X = (W - L) / 2，得偏移量 offset_x = X - (x0-width)
	 * 计算所有包围盒中心 Y 方向平均值 y，影像高度为 H， 得Y方向偏移量 offset_y = (H/2) - y
	 **************************************************/
	int num = static_cast<int>(boxes.size());
	double x_start = boxes.front().m_x0;
	double x_end_1 = boxes.front().m_x1;
	double x_end_2 = boxes.back().m_x1;
	if (num > 1)
	{
		m_object_offset = static_cast<int>((x_end_2 - x_end_1) / (num - 1));
	}
	double new_width = x_end_2 - x_start + m_object_offset * (fiber_end_count - num);
	int adjustment_x_min = static_cast<int>((frame_rgb.cols - new_width) / 2.0);
	int adjustment_x_max = adjustment_x_min + static_cast<int>(new_width);
	work_thread->set_adjustment_x_range(adjustment_x_min, adjustment_x_max);
	pixel_adjustment_x = adjustment_x_min - x_start;
	double y_sum = 0.0;
	for (int i = 0; i < num; i++)
	{
		y_sum += (boxes[i].m_y0 + boxes[i].m_y1) / 2.0;
	}
	y_sum /= num;
	pixel_adjustment_y = (frame_rgb.rows / 2.0) - y_sum;
	write_log(l(QString("pixel adjustment frame count:%1" )
		.arg(work_thread->frame_count)).c_str());
}

void auto_focus2::add_image(const QString& camera_id, const QImage& image)
{
	if(m_capture.load())
	{
		work_thread->add_image(camera_id,image);
	}
}

void auto_focus2::set_motion_parameters(int search_distance, int move_speed, int move_step)
{
	m_search_distance = search_distance;
	m_move_speed = move_speed;
	m_move_step = move_step;
}

void auto_focus2::set_process_position(int position)
{
	m_start_position = position - m_search_distance;
	m_end_position = position + m_search_distance;
}
