#include "thread_calc_image_clarity.h"
#include "../basic_algorithm/common_api.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>

#include "../common/common.h"


////////////////////////////////////////////////////////////////////////////////////////////////
thread_calc_image_clarity::thread_calc_image_clarity(const QString& name, QObject* /*parent*/)
    : m_name(name), m_running(false)
{
}

thread_calc_image_clarity::~thread_calc_image_clarity()
{
    stop();
    if (m_thread.joinable())
        m_thread.join();
}

void thread_calc_image_clarity::start()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = true;
    }
    m_thread = std::thread([this] { run(); });
}

void thread_calc_image_clarity::add_image(const QString& camera_id, const QImage& image_data)
{
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_task_images.push(st_task_image_data(camera_id, image_data));
    }
    m_wait_condition.notify_one();
}

int thread_calc_image_clarity::task_image_count()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return static_cast<int>(m_task_images.size());
}

void thread_calc_image_clarity::stop()
{
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_running = false;
    }
    m_wait_condition.notify_all();
}

void thread_calc_image_clarity::run()
{
    while (true)
    {
        st_task_image_data task_image("", QImage());
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            if (!m_running && m_task_images.empty())
                break;

            if (m_task_images.empty())
                m_wait_condition.wait(locker);

            if (!m_task_images.empty())
            {
                m_calculate_finish.store(false);
                task_image = m_task_images.front();
                m_task_images.pop();
            }
            else
            {
                continue;
            }
        }
        process_task(task_image);
    }
}

bool thread_calc_image_clarity::reset_calculate_image_clarity(const std::vector<QString>& camera_ids)
{
    m_task_type = TASK_CALCULATE_IMAGE_CLARITY;
    m_camera_ids = camera_ids;
    m_processed_max_frame_count.store(false);
    m_calc_frame_count.clear();
    m_calc_frame_max_clarity.clear();
    m_calc_frame_attenuation_times.clear();
    for (size_t i = 0; i < camera_ids.size(); i++)
    {
        m_calc_frame_count.insert(camera_ids[i], 0);
        m_calc_frame_max_clarity.insert(camera_ids[i], 0.0);
        m_calc_frame_attenuation_times.insert(camera_ids[i], 0);
    }
    m_clarity_thresh.clear();
    for (int i = 0; i < camera_ids.size(); i++)
    {
        m_clarity_thresh.insert(camera_ids[i], 0.0);
    }

    {
        std::lock_guard<std::mutex> locker(m_mutex);
        while (!m_task_images.empty()) m_task_images.pop();
    }
    return true;
}

void thread_calc_image_clarity::process_task(const st_task_image_data& image_data)
{
    if (m_task_type == TASK_AUTO_FOCUS)
    {
        process_auto_focus_task(image_data);
    }
    else if (m_task_type == TASK_CALCULATE_IMAGE_CLARITY)
    {
        process_calc_image_clarity_task(image_data);
    }
    else if (m_task_type == TASK_CLARITY_CALIBRATION)
    {
        process_clarity_calibration_task(image_data);
    }
    else if (m_task_type == TASK_PIXEL_ADJUSTMENT)
    {
        process_pixel_adjustment_task(image_data);
    }
}

void thread_calc_image_clarity::process_calc_image_clarity_task(const st_task_image_data& image_data)
{
    if (m_processed_max_frame_count.load())
    {
        m_calculate_finish.store(true);
        return;
    }
    int camera_index = get_camera_index(image_data.m_camera_id);
    if (camera_index == -1)
    {
        m_calculate_finish.store(true);
        return;
    }
    if (image_data.m_image.isNull())
    {
        m_calculate_finish.store(true);
        return;
    }
    if (m_calc_frame_count.contains(image_data.m_camera_id))
    {
        m_calc_frame_count[image_data.m_camera_id]++;
    }
    {
        bool finish(true);
        for (QMap<QString, int>::iterator iter = m_calc_frame_count.begin(); iter != m_calc_frame_count.end(); ++iter)
        {
            if (iter.value() < m_max_frame_count)
            {
                finish = false;
                break;
            }
        }
        if (finish)
        {
            m_processed_max_frame_count.store(true);
            m_calculate_finish.store(true);
            return;
        }
    }
    cv::Mat image = convert_qimage_to_cvmat(image_data.m_image, 1);
    cv::Mat image_ovr;
    cv::resize(image, image_ovr, cv::Size(), m_frame_scale_size, m_frame_scale_size, cv::INTER_AREA);
    double clarity = calc_image_clarity_multiscale(image_ovr);
    if(clarity > m_calc_frame_max_clarity[image_data.m_camera_id])
    {
        m_calc_frame_max_clarity[image_data.m_camera_id] = clarity;
        m_calc_frame_attenuation_times[image_data.m_camera_id] = 0;
    }
    else
    {
        m_calc_frame_attenuation_times[image_data.m_camera_id]++;
    }
    int attenuation_time(m_attenuation_time_thresh + 1);
    for (QMap<QString, int>::iterator iter = m_calc_frame_attenuation_times.begin(); iter != m_calc_frame_attenuation_times.end(); ++iter)
    {
        attenuation_time = std::min(attenuation_time, iter.value());
    }
    if(attenuation_time > m_attenuation_time_thresh)
    {
        m_processed_max_frame_count.store(true);
        m_calculate_finish.store(true);
        return;
    }
    if (clarity > m_clarity_thresh[image_data.m_camera_id])
    {
        m_clarity_thresh[image_data.m_camera_id] = m_calc_frame_max_clarity[image_data.m_camera_id] * 0.5;
    }
    m_calculate_finish.store(true);
}

void thread_calc_image_clarity::process_clarity_calibration_task(const st_task_image_data& image_data)
{
    if (m_processed_max_frame_count.load())
    {
        m_calculate_finish.store(true);
        return;
    }
    int camera_index = get_camera_index(image_data.m_camera_id);
    if (camera_index == -1)
    {
        m_calculate_finish.store(true);
        return;
    }
    if (image_data.m_image.isNull())
    {
        m_calculate_finish.store(true);
        return;
    }
    if (m_calc_frame_count.contains(image_data.m_camera_id))
    {
        m_calc_frame_count[image_data.m_camera_id]++;
    }
    {
        bool finish(true);
        for (QMap<QString, int>::iterator iter = m_calc_frame_count.begin(); iter != m_calc_frame_count.end(); ++iter)
        {
            if (iter.value() < m_max_frame_count)
            {
                finish = false;
                break;
            }
        }
        if (finish)
        {
            m_processed_max_frame_count.store(true);
            m_calculate_finish.store(true);
            return;
        }
    }
    cv::Mat image = convert_qimage_to_cvmat(image_data.m_image, 1);
    cv::Mat image_ovr;
    cv::resize(image, image_ovr, cv::Size(), m_frame_scale_size, m_frame_scale_size, cv::INTER_AREA);
    double clarity = calc_image_clarity_multiscale(image_ovr);
    if (clarity >= m_clarity_thresh[image_data.m_camera_id] &&
        m_camera_fiber_end[image_data.m_camera_id].size() < 1)
    {
        cv::Mat image_rgb = convert_qimage_to_cvmat(image_data.m_image, 3);
        m_camera_fiber_end[image_data.m_camera_id] = m_object_detector->detect_objects(image_rgb);
        if (m_camera_fiber_end[image_data.m_camera_id].size() < 1)
        {
            m_calculate_finish.store(true);
            return;
        }
        else
        {
            std::vector<st_detect_box>& fiber_ends = m_camera_fiber_end[image_data.m_camera_id];
            std::sort(fiber_ends.begin(), fiber_ends.end(), st_detect_box::sort_by_x0);
            for (int i = fiber_ends.size() - 1; i >= 0; i--)
            {
                double box_width = fiber_ends[i].m_x1 - fiber_ends[i].m_x0;
                double box_height = fiber_ends[i].m_y1 - fiber_ends[i].m_y0;
                if (box_width < m_object_detector->target_size_x() - 5.0 || box_height < m_object_detector->target_size_y() - 5.0)
                {
                    fiber_ends.erase(fiber_ends.begin() + i);
                    continue;
                }
                if (fiber_ends[i].m_x0 < 1.0 || fiber_ends[i].m_y0 < 1.0 ||
                    fiber_ends[i].m_x1 > image.cols - 2 || fiber_ends[i].m_y1 > image.rows - 2)
                {
                    fiber_ends.erase(fiber_ends.begin() + i);
                    continue;
                }
            }
            for (size_t i = 0; i < fiber_ends.size(); i++)
            {
                fiber_ends[i].m_x0 = std::max(0.0, fiber_ends[i].m_x0 - m_object_expand);
                fiber_ends[i].m_y0 = std::max(0.0, fiber_ends[i].m_y0 - m_object_expand);
                fiber_ends[i].m_x1 = std::min(static_cast<double>(image.cols - 1), fiber_ends[i].m_x1 + m_object_expand);
                fiber_ends[i].m_y1 = std::min(static_cast<double>(image.rows - 1), fiber_ends[i].m_y1 + m_object_expand);
            }
        }
    }
    if (m_total_fiber_end_count == 0)
    {
        for (QMap<QString, std::vector<st_detect_box>>::iterator iter = m_camera_fiber_end.begin(); iter != m_camera_fiber_end.end(); ++iter)
        {
            if (iter.value().size() < 1)
            {
                m_total_fiber_end_count = 0;
                m_calculate_finish.store(true);
                return;
            }
            m_total_fiber_end_count += iter.value().size();
        }
        m_focus_image_claritys.assign(m_total_fiber_end_count, 0.0);
        m_attenuation_times.assign(m_total_fiber_end_count, 0);
    }
    std::vector<st_detect_box>& fiber_ends = m_camera_fiber_end[image_data.m_camera_id];
    int start_index = get_start_index(image_data.m_camera_id);
    for (size_t i = 0; i < fiber_ends.size(); i++)
    {
        int x0 = static_cast<int>(fiber_ends[i].m_x0);
        int y0 = static_cast<int>(fiber_ends[i].m_y0);
        int x1 = static_cast<int>(fiber_ends[i].m_x1);
        int y1 = static_cast<int>(fiber_ends[i].m_y1);
        cv::Rect roi(x0, y0, x1 - x0, y1 - y0);
        cv::Mat sub_img = image(roi);
        cv::resize(sub_img, sub_img, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
        double clarity_value = calc_image_clarity_multiscale(sub_img);
        m_focus_image_claritys[start_index + i] = std::max(m_focus_image_claritys[start_index + i], clarity_value);
    }
    m_calculate_finish.store(true);
}

void thread_calc_image_clarity::process_auto_focus_task(const st_task_image_data& image_data)
{
    if (m_finished.load() || m_processed_max_frame_count.load())
    {
        m_calculate_finish.store(true);
        return;
    }
    if(m_object_detect_fail.load())
    {
        m_calculate_finish.store(true);
        return;
    }
    int camera_index = get_camera_index(image_data.m_camera_id);
    if (camera_index == -1)
    {
        m_calculate_finish.store(true);
        return;
    }
    if (image_data.m_image.isNull())
    {
        m_calculate_finish.store(true);
        return;
    }
    if (m_save_cache)
    {
        QString current_directory = QCoreApplication::applicationDirPath();
        QString dir = current_directory + L("/Temp");
        make_path(dir);
        static std::vector<int> image_indexs(m_camera_ids.size(), 0);
        cv::Mat image0 = convert_qimage_to_cvmat(image_data.m_image, 1);
        int pos = get_camera_index(image_data.m_camera_id);
        if (pos != -1)
        {
            std::string path = l(dir) + l(QString("/%1_%2.png").arg(image_data.m_camera_id).arg(image_indexs[pos], 4, 10, QChar('0')));
            cv::imwrite(utf8_to_ansi(path), image0);
            image_indexs[pos]++;
        }
    }

    cv::Mat image = convert_qimage_to_cvmat(image_data.m_image, 1);
    cv::Rect rect;
    rect.x = image.cols * 0.375;
    rect.y = image.cols * 0;
    rect.width = image.cols * 0.25;
    rect.height = image.rows;
    cv::Mat image_roi = image(rect);
    if (m_camera_fiber_end.contains(image_data.m_camera_id) && m_camera_fiber_end[image_data.m_camera_id].size() < 1)
    {
        if (m_camera_position_fail[image_data.m_camera_id])
        {
            m_calculate_finish.store(true);
            return;
        }
        double clarity = calc_image_clarity(image);
        if (clarity > m_calc_frame_max_clarity[image_data.m_camera_id] * 0.8)
        {
            cv::Mat image_rgb = convert_qimage_to_cvmat(image_data.m_image, 3);
            m_camera_fiber_end[image_data.m_camera_id] = m_object_detector->detect_objects(image_rgb);
            if (m_camera_fiber_end[image_data.m_camera_id].size() < 1)
            {
                m_camera_position_fail[image_data.m_camera_id] = true;
                bool object_detect_fail(true);
                for (QMap<QString, bool>::iterator iter = m_camera_position_fail.begin(); iter != m_camera_position_fail.end(); ++iter)
                {
                    if (!iter.value())
                    {
                        object_detect_fail = false;
                        break;
                    }
                }
                if (object_detect_fail)
                {
                    m_object_detect_fail.store(true);
                    m_calculate_finish.store(true);
                    return;
                }
            }
            else
            {
                std::vector<st_detect_box>& fiber_ends = m_camera_fiber_end[image_data.m_camera_id];
                std::sort(fiber_ends.begin(), fiber_ends.end(), st_detect_box::sort_by_x0);
                for (int i = 0; i < fiber_ends.size(); i++)
                {
                    fiber_ends[i].m_x0 = std::max(0.0, fiber_ends[i].m_x0 - m_object_expand);
                    fiber_ends[i].m_y0 = std::max(0.0, fiber_ends[i].m_y0 - m_object_expand);
                    fiber_ends[i].m_x1 = std::min(static_cast<double>(image.cols - 1), fiber_ends[i].m_x1 + m_object_expand);
                    fiber_ends[i].m_y1 = std::min(static_cast<double>(image.rows - 1), fiber_ends[i].m_y1 + m_object_expand);
                }
                for (int i = fiber_ends.size() - 1; i >= 0; i--)
                {
                    double box_width = fiber_ends[i].m_x1 - fiber_ends[i].m_x0;
                    double box_height = fiber_ends[i].m_y1 - fiber_ends[i].m_y0;
                    if (box_width < m_object_detector->target_size_x() - 5.0 || box_height < m_object_detector->target_size_y() - 5.0)
                    {
                        fiber_ends.erase(fiber_ends.begin() + i);
                        continue;
                    }
                    if (fiber_ends[i].m_x0 < 1.0 || fiber_ends[i].m_y0 < 1.0 ||
                        fiber_ends[i].m_x1 > image.cols - 2 || fiber_ends[i].m_y1 > image.rows - 2)
                    {
                        fiber_ends.erase(fiber_ends.begin() + i);
                        continue;
                    }
                    if (fiber_ends[i].m_x0 < m_adjustment_x_min - box_width * 0.8)
                    {
                        fiber_ends.erase(fiber_ends.begin() + i);
                        continue;
                    }
                    if (fiber_ends[i].m_x1 > m_adjustment_x_max + box_width * 0.8)
                    {
                        fiber_ends.erase(fiber_ends.begin() + i);
                        continue;
                    }
                }
                predict_missing(fiber_ends);
                if (fiber_ends.size() > 0 && image_data.m_camera_id == m_camera_ids[0])
                {
                    m_max_position = static_cast<int>(fiber_ends.back().m_x1);
                }
            }
        }
        else
        {
            m_calculate_finish.store(true);
            return;
        }
    }
    if (m_total_fiber_end_count == 0)
    {
        for (QMap<QString, std::vector<st_detect_box>>::iterator iter = m_camera_fiber_end.begin(); iter != m_camera_fiber_end.end(); ++iter)
        {
            if (iter.value().size() < 1)
            {
                m_total_fiber_end_count = 0;
                m_calculate_finish.store(true);
                return;
            }
            m_total_fiber_end_count += iter.value().size();
        }
        m_focus_images.assign(m_total_fiber_end_count, st_focus_image());
        m_finished_flags.assign(m_total_fiber_end_count, false);
        m_focus_image_claritys.assign(m_total_fiber_end_count, 0.0);
        m_attenuation_times.assign(m_total_fiber_end_count, 0);
        m_cache_images.initialize(m_total_fiber_end_count, 15);
    }
    std::vector<st_detect_box>& fiber_ends = m_camera_fiber_end[image_data.m_camera_id];
    std::vector<double> clarity_values(fiber_ends.size(), 0.0);
    int start_index = get_start_index(image_data.m_camera_id);
#pragma omp parallel for num_threads(8)
    for (int i = 0; i < fiber_ends.size(); i++)
    {
        if (m_finished_flags[start_index + i])
        {
            continue;
        }
        int x0 = static_cast<int>(fiber_ends[i].m_x0);
        int y0 = static_cast<int>(fiber_ends[i].m_y0);
        int x1 = static_cast<int>(fiber_ends[i].m_x1);
        int y1 = static_cast<int>(fiber_ends[i].m_y1);
        cv::Rect roi(x0, y0, x1 - x0, y1 - y0);
        cv::Mat sub_img = image(roi);
        cv::resize(sub_img, sub_img, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
        clarity_values[i] = calc_image_clarity_bandpass(sub_img, cv::Mat(), 0.3, 0.4, 2.0);
    }
    QString save_path = QString("%1/%2_%3.png").arg(m_save_dir).arg(image_data.m_camera_id).arg(m_save_index);
    if (!m_save_images.contains(save_path))
    {
        m_save_images.insert(save_path, cv::Mat());
    }
    for (int i = 0; i < fiber_ends.size(); i++)
    {
        if (m_finished_flags[start_index + i])
        {
            if(m_cache_images.m_finished_count[start_index+i] < m_cache_images.m_cache_size)
            {
                st_focus_image focus_image = generate_focus_image_from_detect_box(image, fiber_ends[i]);
                m_cache_images.add_cache_image(start_index + i, focus_image.m_focus_image, true);
            }
            continue;
        }
        st_focus_image focus_image = generate_focus_image_from_detect_box(image, fiber_ends[i]);
        double value = clarity_values[i];
        if (value > m_focus_image_claritys[start_index + i])
        {
            m_focus_image_claritys[start_index + i] = value;
            m_attenuation_times[start_index + i] = 0;
            m_focus_images[start_index + i] = focus_image;
            m_cache_images.add_cache_image(start_index + i, focus_image.m_focus_image, false);
            m_cache_images.set_finished_count(start_index + i, 0);
            if (i == 0)
            {
                m_save_images[save_path] = image.clone();
            }
        }
        else
        {
            m_cache_images.add_cache_image(start_index + i, focus_image.m_focus_image, true);
            m_attenuation_times[start_index + i] += 1;
            if (value < m_focus_image_claritys[start_index + i] - m_clarity_diff_thresh ||
                m_attenuation_times[start_index + i] >= m_attenuation_time_thresh)
            {
                m_finished_flags[start_index + i] = true;
                continue;
            }
        }
    }
    if(m_cache_images.cache_finished())
    {
        m_finished.store(true);
    }
    m_calculate_finish.store(true);
}

void thread_calc_image_clarity::process_pixel_adjustment_task(const st_task_image_data& image_data)
{
    if (image_data.m_camera_id != m_camera_ids[0])
    {
        m_calculate_finish.store(true);
        return;
    }
    if (image_data.m_image.isNull())
    {
        m_calculate_finish.store(true);
        return;
    }
    cv::Mat image = convert_qimage_to_cvmat(image_data.m_image, 1);
    double clarity = calc_image_clarity(image);
    if (clarity > m_calc_frame_max_clarity[image_data.m_camera_id])
    {
        m_calc_frame_max_clarity[image_data.m_camera_id] = clarity;
        m_max_clarity_frame_index = m_frame_index;
        m_max_clarity_frame = image;
    }
    m_frame_index++;
    if(m_save_cache)
    {
        QString current_directory = QCoreApplication::applicationDirPath();
        QString dir = current_directory + L("/Temp");
        make_path(dir);
        static std::vector<int> image_indexs2(m_camera_ids.size(), 0);
        int pos = get_camera_index(image_data.m_camera_id);
        if (pos != -1)
        {
            std::string path = l(dir) + l(QString("/adjustment_%1_%2.png").arg(image_data.m_camera_id).arg(image_indexs2[pos], 4, 10, QChar('0')));
            cv::imwrite(utf8_to_ansi(path), image);
            image_indexs2[pos]++;
        }
    }
    frame_count++;
    m_calculate_finish.store(true);
}

bool thread_calc_image_clarity::all_finished()
{
    if(m_total_fiber_end_count < 1)
    {
        return false;
    }
    for (int i = 0; i < m_total_fiber_end_count; i++)
    {
        if (!m_finished_flags[i])
        {
            return false;
        }
    }
    return true;
}

int thread_calc_image_clarity::get_camera_index(const QString& camera_id)
{
    for (int i = 0;i < m_camera_ids.size();i++)
    {
        if(m_camera_ids[i] == camera_id)
        {
            return i;
        }
    }
    return -1;
}

void thread_calc_image_clarity::save_cache_images(const QString& dst_dir, QString& sub_dir)
{
    if (!make_path(dst_dir))
    {
        return;
    }
    sub_dir = dst_dir + L("/cache-") + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
    if (!make_path(sub_dir))
    {
        return;
    }
    for (int i = 0;i < m_cache_images.m_images.size();i++)
    {
        QString file_dir = sub_dir + QString("/%1").arg(i + 1);
        if (!make_path(file_dir))
        {
            continue;
        }
        for (int j = 0;j < m_cache_images.m_images[i].size();j++)
        {
            cv::Mat image = m_cache_images.m_images[i][j];
            QString file_path = file_dir + QString("/%1.png").arg(j + 1, 4, 10, QChar('0'));
            cv::imwrite(utf8_to_ansi(l(file_path)), image);
        }
    }
}

bool thread_calc_image_clarity::reset_clarity_calibration(const std::vector<QString>& camera_ids)
{
    m_task_type = TASK_CLARITY_CALIBRATION;
    m_camera_ids = camera_ids;
    m_processed_max_frame_count.store(false);
    m_calc_frame_count.clear();
    for (int i = 0; i < camera_ids.size(); i++)
    {
        m_calc_frame_count.insert(camera_ids[i], 0);
    }
    m_total_fiber_end_count = 0;
    m_camera_fiber_end.clear();
    for (int i = 0; i < camera_ids.size(); i++)
    {
        m_camera_fiber_end.insert(camera_ids[i], std::vector<st_detect_box>());
    }
    m_calculate_finish.store(true);
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        while (!m_task_images.empty()) m_task_images.pop();
    }
    return true;
}

bool thread_calc_image_clarity::reset_auto_focus(const std::vector<QString>& camera_ids, int fiber_end_count, const QString& save_dir, int index, bool save_cache)
{
    m_task_type = TASK_AUTO_FOCUS;
    m_camera_ids = camera_ids;
    m_save_dir = save_dir;
    m_save_index = index;
    m_total_fiber_end_count = 0;
    m_fiber_end_count = fiber_end_count;
    m_max_position = 0;
    m_camera_fiber_end.clear();
    m_calc_frame_count.clear();
    m_camera_position_fail.clear();
    m_save_cache = save_cache;
    frame_id = 0;
    for (size_t i = 0; i < camera_ids.size(); i++)
    {
        m_camera_fiber_end.insert(camera_ids[i], std::vector<st_detect_box>());
        m_calc_frame_count.insert(camera_ids[i], 0);
        m_camera_position_fail.insert(camera_ids[i], false);
    }
    m_focus_images.clear();
    m_focus_image_claritys.clear();
    m_attenuation_times.clear();
    m_finished_flags.clear();
    m_save_images.clear();
    m_finished.store(false);
    m_object_detect_fail.store(false);
    m_processed_max_frame_count.store(false);
    m_calculate_finish.store(true);
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        while (!m_task_images.empty()) m_task_images.pop();
    }
    return true;
}

bool thread_calc_image_clarity::reset_pixel_adjustment(const std::vector<QString>& camera_ids, int fiber_end_count, bool save_cache)
{
    m_task_type = TASK_PIXEL_ADJUSTMENT;
    m_camera_ids = camera_ids;
    m_save_cache = save_cache;
    m_processed_max_frame_count.store(false);
    m_pixel_adjustment_finished.store(false);
    m_calc_frame_count.clear();
    m_calc_frame_max_clarity.clear();
    for (size_t i = 0; i < camera_ids.size(); i++)
    {
        m_calc_frame_count.insert(camera_ids[i], 0);
        m_calc_frame_max_clarity.insert(camera_ids[i], 0.0);
    }
    m_total_fiber_end_count = fiber_end_count;
    m_max_clarity_frame_index = m_frame_index = 0;
    frame_count = 0;
    m_calculate_finish.store(true);
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        while (!m_task_images.empty()) m_task_images.pop();
    }
    return true;
}

int thread_calc_image_clarity::get_start_index(const QString& camera_id)
{
    int camera_index = get_camera_index(camera_id);
    if (camera_index == -1)
    {
        return -1;
    }
    int ret(0);
    for (int i = 0; i < m_camera_ids.size(); i++)
    {
        if (m_camera_ids[i] < camera_id)
        {
            ret += m_camera_fiber_end[m_camera_ids[i]].size();
        }
    }
    return ret;
}

void thread_calc_image_clarity::save_images()
{
    for (QMap<QString, cv::Mat>::iterator iter = m_save_images.begin(); iter != m_save_images.end(); ++iter)
    {
        cv::imwrite(utf8_to_ansi(l(iter.key())), iter.value());
    }
}

st_focus_image thread_calc_image_clarity::generate_focus_image_from_detect_box(const cv::Mat& image, const st_detect_box& box)
{
    st_detect_box new_box = box.buffer(2.5);
    new_box.m_x0 = std::max(0.0, new_box.m_x0);
    new_box.m_y0 = std::max(0.0, new_box.m_y0);
    new_box.m_x1 = std::min(double(image.cols - 1), new_box.m_x1);
    new_box.m_y1 = std::min(double(image.rows - 1), new_box.m_y1);
    int x0 = static_cast<int>(new_box.m_x0);
    int y0 = static_cast<int>(new_box.m_y0);
    int x1 = static_cast<int>(new_box.m_x1);
    int y1 = static_cast<int>(new_box.m_y1);
    cv::Rect roi(x0, y0, x1 - x0, y1 - y0);
    double offset_x0 = box.m_x0 - new_box.m_x0;
    double offset_y0 = box.m_y0 - new_box.m_y0;
    double offset_x1 = box.m_x1 - new_box.m_x0;
    double offset_y1 = box.m_y1 - new_box.m_y0;
    st_focus_image focus_image = st_focus_image();
    focus_image.m_focus_image = image(roi).clone();
    focus_image.m_focus_box =
        st_detect_box(0.0, offset_x0, offset_y0, offset_x1, offset_y1);
    return focus_image;
}
