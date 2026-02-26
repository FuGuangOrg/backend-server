#pragma once
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QImage>

// TODO(fuguang-server): QMap → std::unordered_map, QQueue → moodycamel::ReaderWriterQueue
// 当前保留 Qt 容器，因为 auto_focus 层位于 fuguang-server（Qt 允许）
// 下一步迁移：m_camera_id: QString→std::string, m_image: QImage→cv::Mat
#include <QMap>
#include <QQueue>

#include "../common/common_api.h"
#include "../basic_algorithm/object_detector.h"

#include "auto_focus_global.h"

struct st_task_image_data
{
    QString m_camera_id{ "" };
    QImage m_image;

    st_task_image_data(const QString& camera_id, QImage image) :m_camera_id(camera_id), m_image(image)
    {}
};

//记录对焦结果，考虑到结果存储需要保存较大的范围，m_focus_image 记录的是粗定位结果外扩之后的数据
//实际对焦区域是 m_focus_image 中 m_focus_box 记录的部分
struct st_focus_image
{
    cv::Mat m_focus_image;
    st_detect_box m_focus_box;

    st_focus_image(){}
};

//缓存影像.由于端面和背景可能不在一个平面，因此最清晰的端面和最清晰的背景可能不在同一帧影像中
//每个端面找到最清晰的局部影像的过程中，缓存足够数量的影像用于背景分离，以找到最清晰的背景和最清晰的端面
//缓存最清晰的局部影响前后 m_cache_size 帧，共 2*m_cache_size+1帧.
//每次对焦时首先初始化为总端面数量，m_images存储每个端面的缓存影像队列，m_finished_count记录每个端面找到最清晰影像之后已缓存的帧数
struct st_cache_image
{
	int m_cache_size{ 5 };                  //缓存帧数为 m_cache_size*2 + 1
    std::vector<int> m_finished_count;      //找到最清晰影像之后缓存的帧数
	std::vector<int> m_focus_index;          //最清晰影像在缓存队列中的位置
    std::vector<std::deque<cv::Mat>> m_images;

    void add_cache_image(int index, const cv::Mat& image, bool has_finished = false)
    {
        if (m_finished_count[index] >= m_cache_size)
        {
            return;
        }
        if (has_finished)
        {
            m_finished_count[index]++;
        }
        if (m_images[index].size() >= 2 * m_cache_size + 1)
        {
            m_images[index].pop_front();
        }
        m_images[index].push_back(image);
    }

	void set_finished_count(int index, int count)
    {
        m_finished_count[index] = count;
		m_focus_index[index] = m_images[index].size();    //最清晰影像在缓存队列中的位置
	}

    void initialize(int total_count,int cache_size = 5)
    {
		m_cache_size = cache_size;
        m_focus_index.assign(total_count, 0);
        m_finished_count.assign(total_count, 0);
		m_images.assign(total_count, std::deque<cv::Mat>());
    }

    bool cache_finished() const
    {
	    for (int i = 0;i < m_finished_count.size();i++)
	    {
		    if(m_finished_count[i] < m_cache_size)
            {
                return false;
			}
	    }
        return true;
    }
	
};

enum thread_task_type
{
	TASK_AUTO_FOCUS = 0,                    //自动对焦
    TASK_CALCULATE_IMAGE_CLARITY = 1,       //得到整幅影像最大清晰度
	TASK_CLARITY_CALIBRATION = 2,           //清晰度标定
	TASK_PIXEL_ADJUSTMENT = 3,               //位置调整
};

/**************************
 * 线程类,监视影像队列，执行清晰度计算
 *****************************/
class AUTO_FOCUS_EXPORT thread_calc_image_clarity : public QThread
{
    Q_OBJECT
public:
    thread_calc_image_clarity(const QString& name, QObject* parent = nullptr);
    virtual ~thread_calc_image_clarity() override;

    void add_image(const QString& camera_id, const QImage& image_data);
    int task_image_count();
    void set_max_frame_count(int frame_count) { m_max_frame_count = frame_count; }
    void stop();


	//计算清晰度步骤 1:计算所有大影像清晰度，得到 粗定位清晰度阈值(ACCE)/跳帧清晰度阈值(YSOD)
    bool reset_calculate_image_clarity(const std::vector<QString>& camera_ids);

    //获取相机在列表中的位置，如果没有返回-1
    int get_camera_index(const QString& camera_id);
    double clarity_diff_thresh() const { return m_clarity_diff_thresh; }
    void set_clarity_diff_thresh(double clarity_diff_thresh) { m_clarity_diff_thresh = clarity_diff_thresh; }
    void set_adjustment_x_range(int x_min, int x_max) { m_adjustment_x_min = x_min; m_adjustment_x_max = x_max; }
    std::vector<double> focus_image_claritys() const { return m_focus_image_claritys; }
    std::vector<int> calc_frame_count() const
    {
        std::vector<int> frame_counts;
        for (QMap<QString, int>::const_iterator iter = m_calc_frame_count.begin(); iter != m_calc_frame_count.end(); ++iter)
        {
            frame_counts.emplace_back(iter.value());
        }
        return frame_counts;
    }

    std::vector<std::deque<cv::Mat>> get_cache_images() { return m_cache_images.m_images; }
    std::vector<int> get_focus_indexes() { return m_cache_images.m_focus_index; }
	//保存缓存影像
    void save_cache_images(const QString& dst_dir, QString& sub_dir);
    
	int max_clarity_frame_index() const { return m_max_clarity_frame_index; }
    cv::Mat max_clarity_frame() const { return m_max_clarity_frame; }

    bool reset_clarity_calibration(const std::vector<QString>& camera_ids);
    bool reset_auto_focus(const std::vector<QString>& camera_ids, int fiber_end_count, const QString& save_dir, int index, bool save_cache = false);
    //计算像素偏移
    bool reset_pixel_adjustment(const std::vector<QString>& camera_ids, int fiber_end_count, bool save_cache = false);
	std::vector<st_focus_image> focus_images() { return m_focus_images; }
    void set_object_detector(object_detector* detector) { m_object_detector = detector; }
    object_detector* object_detector_ptr() const { return m_object_detector; }
    double object_expand() const { return  m_object_expand; }
    int get_start_index(const QString& camera_id);      //获取相机拍摄影像的第一个端面在当前对焦的所有端面中的位置，如果没有返回-1
    const QMap<QString, double>& clarity_thresh() const { return m_clarity_thresh; }
    void set_clarity_thresh(const QMap<QString, double>& camera_claritys) { m_clarity_thresh = camera_claritys; }
    int max_position() const { return m_max_position; }
    
    //保存大图
    void save_images();
    //从粗定位结果得到局部影像,用于计算清晰度或者创建缓存帧
    st_focus_image generate_focus_image_from_detect_box(const cv::Mat& image, const st_detect_box& box);

    std::atomic<bool> m_object_detect_fail{ false };            //粗定位失败结束
    
    int max_frame_count() { return m_max_frame_count; }
    bool all_finished();                                               //是否所有区域计算完成
	std::atomic<bool> m_finished{ false };                      //正常处理完毕结束
    std::atomic<bool>  m_processed_max_frame_count{ false };    //处理到最大帧数时结束
    std::atomic<bool> m_calculate_finish{ true };               //单次计算是否执行完成
    std::atomic<bool> m_pixel_adjustment_finished{ false };     //像素偏移计算完成
signals:
    void post_task_finished(const QVariant& task_data);

protected:
    void run() override;
	void process_task(const st_task_image_data& image_data);
    void process_calc_image_clarity_task(const st_task_image_data& image_data);
    void process_clarity_calibration_task(const st_task_image_data& image_data);
    void process_auto_focus_task(const st_task_image_data& image_data);
    void process_pixel_adjustment_task(const st_task_image_data& image_data);

private:
    /******************清晰度计算参数*******************/
    QMap<QString, double> m_clarity_thresh;
    QMap<QString, std::vector<st_detect_box>> m_camera_fiber_end;           //每个相机对应的端面,粗定位得到
    int m_total_fiber_end_count{ 0 };                   //所有相机的端面总数
    object_detector* m_object_detector{ nullptr };      //目标检测器，用于定位端面位置，替代分割方案,不负责资源释放
    double m_object_expand{ 80.0 };                     //目标外扩像素，确保粗定位结果包含完整端面
    QString m_save_dir{ "" };                        //大图保存路径
	int m_save_index{ 0 };                              //一个产品可能存在多次运动对焦，当前运动次序，用于保存大图时区分不同次运动
    QMap<QString, cv::Mat> m_save_images;               //每个相机拍摄的足够清晰的大图
    int m_max_position{ 0 };                            //第一个相机端面中最大的定位位置，用于控制后续移动
	
    QMap<QString, bool> m_camera_position_fail;         //每个相机粗定位是否成功，都失败时直接返回
    std::vector<st_focus_image> m_focus_images;         //检测结果，存储最清晰的局部影像

    std::vector<QString> m_camera_ids;                  //所有的相机
	int m_fiber_end_count{ 0 };                         //每个相机拍摄的端面数量
    std::vector<double> m_focus_image_claritys;         //最清晰的局部影像的清晰度，与 m_focus_images 一一对应
    std::vector<bool> m_finished_flags;                 //每个区域的完成标识，如果计算完成后续不再计算
    int m_max_frame_count{ 120 };                       //每个端面参与计算的最大帧数，包含跳过帧，防止极端情况下陷入无限循环
    QMap<QString, int> m_calc_frame_count;              //每个相机拍摄的影像参与计算的帧数，最大不超过 m_max_frame_count 

    QMap<QString, double> m_calc_frame_max_clarity;     //清晰度标定时使用，记录每个相机拍摄的整张影像最大清晰度
    QMap<QString, int> m_calc_frame_attenuation_times;  //清晰度标定时使用，记录整张影像清晰度无增长次数
    double m_frame_scale_size{ 0.2 };                   //清晰度标定时使用，计算整张影像清晰度时的缩放系数
	//new:清晰度无增长次数阈值，如果某个区域清晰度已经有 m_attenuation_time_thresh 次没有超过其当前最大清晰度，表示后续不可能再增加，已经找到最清晰的影像
    int m_attenuation_time_thresh{ 20 };
    std::vector<int> m_attenuation_times;               //每个区域的清晰度无增长次数，达到 m_attenuation_time_thresh 次时不再计算后续影像的清晰度
    //清晰度衰减阈值，如果 cur_clarity < last_clarity + m_attenuation_clarity_thresh, 表示清晰度没有明显增长(>0)或者持续衰减(<0)
	double m_clarity_diff_thresh{ 50.0 };               //如果当前清晰度小于最大清晰度一个较大的差值，认为已经找到了最清晰影像

    int m_frame_index{ 0 };
	int m_max_clarity_frame_index{ 0 };			        //最清晰的影像在队列中的索引
    cv::Mat m_max_clarity_frame;
	//缓存帧，用于背景分离，得到最清晰的背景影像和最清晰的端面影像
	st_cache_image m_cache_images;
	bool m_save_cache{ false };                                     //是否保存缓存影像(大图)
    int m_adjustment_x_min{ 0 }, m_adjustment_x_max{ 10000 };       //居中端面在影像上的允许位置范围
    /******************任务相关参数********************/
    QQueue<st_task_image_data> m_task_images;           //任务影像，每张影像需要拆分计算清晰度
    QMutex m_mutex;
    QWaitCondition m_wait_condition;
    bool m_running;                                     //运行标识
    QString m_name;                                     //线程名称
	thread_task_type m_task_type{ TASK_AUTO_FOCUS };    //当前任务类型
    int m_index{ 0 };                                   //调式参数，测试任务线程能否正常接收到影像
public:
    int frame_count{ 0 };
    int frame_id{ 0 };
};