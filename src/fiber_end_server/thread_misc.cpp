#include "thread_misc.h"
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QBuffer>
#include <QDir>
#include <QImage>

#include "../common/common_api.h"
#include "../basic_algorithm/common_api.h"

thread_misc::thread_misc(QString name, QObject* parent)
    :thread_base(name, parent)
{

}

thread_misc::~thread_misc()
{
	if (m_camera != nullptr)
	{
		m_camera->close();
		delete m_camera;
		m_camera = nullptr;
	}
    if(m_motion_control != nullptr)
    {
        delete m_motion_control;
        m_motion_control = nullptr;
    }
    if(m_auto_focus != nullptr)
    {
        delete m_auto_focus;
        m_auto_focus = nullptr;
    }
    if (m_fiber_end_detector != nullptr)
    {
        delete m_fiber_end_detector;
        m_fiber_end_detector = nullptr;
    }
}

bool thread_misc::initialize(st_config_data* config_data)
{
    setup_camera_config_mgr();
	if(!setup_motion_control(config_data))
	{
        write_log("setup_motion_control fail!");
	}
    //初始化算法检测模块。在打开相机之后需要支持算法参数设置，因此需要预先初始化
    if(!setup_fiber_end_detector())
    {
        write_log("setup_fiber_end_detector fail!");
    }
    return true;
}

void thread_misc::setup_camera_config_mgr()
{
    QString current_directory = QCoreApplication::applicationDirPath();
    QString camera_config_path = current_directory + L("/camera_configs.xml");
    m_camera_config_mgr.load_from_file(camera_config_path);
}

bool thread_misc::setup_motion_control(st_config_data* config_data)
{
    m_config_data = config_data;
    motion_parameter* parameter(nullptr);
#ifdef MOTION_CONTROL_PLC
    //ip+端口连接运控，串口连接光源
    std::string ip_address = "192.168.6.6";         // ip 地址
	int port = 502;                                 // 端口号
    std::string port_name = "COM1";                 // 串口号
    unsigned int baud_rate = 115200;                // 波特率
    m_motion_control = new motion_control_plc();
    parameter = new motion_parameter_plc(ip_address, port,port_name, baud_rate);
#else
    std::string port_name = "COM1";  // 串口号
    unsigned int baud_rate = 115200;
    m_motion_control = new motion_control_port();
    parameter = new motion_parameter_port(port_name, baud_rate);
#endif
    if (!m_motion_control->initialize(parameter))
    {
        return false;
    }
    if(!m_motion_control->open())
    {
	    return false;
    }
    connect(m_motion_control, &motion_control::post_device_request_start_process, this, &thread_misc::on_device_request_start_process);
	m_motion_control->reset(0);
    m_motion_control->reset(1);
    m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
    m_motion_control->get_position(m_config_data->m_position_z, m_config_data.m_position_x, m_config_data.m_position_y);

    m_motion_control->set_light_source_param(1000000, m_config_data->m_light_brightness, 1);
	return true;
}

bool thread_misc::setup_fiber_end_detector()
{
    m_fiber_end_detector = new fiber_end_algorithm;
    QString current_directory = QCoreApplication::applicationDirPath();
    std::string shape_model_path = (current_directory + "/shape_model/model.bin").toStdString();
    std::string algorithm_parameter_path = (current_directory + "/parameter.xml").toStdString();
    if (!m_fiber_end_detector->initialize(m_config_data->m_fiber_end_pixel_size, algorithm_parameter_path))
    {
        delete m_fiber_end_detector;
        m_fiber_end_detector = nullptr;
        return false;
    }
    return true;
}

bool thread_misc::load_user_config_file(const QString& file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        write_log(QString("Failed to open file for reading: %1").arg(file_path).toStdString().c_str());
        return false;
    }
    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        file.close();
        write_log(QString("Failed to parse XML: %1").arg(file_path).toStdString().c_str());
        return false;
    }
    file.close();
    QDomElement root = doc.documentElement();
    if (root.tagName() != L("user_config"))
    {
	    return false;
    }
    QDomElement camera_node = root.firstChildElement(L("Camera"));
    if(camera_node.isNull())
    {
        write_log("Load camera node failed....");
        return false;
	}
    m_camera->import_config(st_camera_config_mgr::load_camera_config_from_node(camera_node));
	//加载完成之后更新对应文件
	m_camera_config_mgr.update_camera_config(m_camera->export_config());
    m_camera_config_mgr.save_to_file();
	QDomElement algorithm_node = root.firstChildElement(L("algorithm_parameter"));
    if (algorithm_node.isNull())
    {
        write_log("Load algorithm node failed....");
        return false;
    }
	m_fiber_end_detector->algorithm_parameter()->load_from_node(algorithm_node);
    //加载完成之后更新对应文件
    m_fiber_end_detector->algorithm_parameter()->save_to_xml();

	QDomElement server_node = root.firstChildElement(L("server_parameter"));
    if (server_node.isNull())
    {
        write_log("Load server node failed....");
        return false;
    }
	m_config_data->load_from_node(server_node);
    //通知自动对焦模块
    if(m_auto_focus != nullptr)
    {
        m_auto_focus->set_fiber_end_count(m_config_data->m_fiber_end_count);
    }
    //加载完成之后更新对应文件
    m_config_data->save();

	return true;
}

bool thread_misc::save_user_config_file(const QString& file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        write_log("Failed to open config file for writing");
        return false;
    }
    QDomDocument doc;
    // 加 XML 声明
    QDomProcessingInstruction instr = doc.createProcessingInstruction(
        "xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instr);
    //相机参数、算法参数、端面检测参数保存到 XML
    QDomElement root = doc.createElement(L("user_config"));
    doc.appendChild(root);
    QDomElement camera_node = st_camera_config_mgr::save_camera_config_to_node(doc, m_camera->export_config());
    root.appendChild(camera_node);
    QDomElement algorithm_node = m_fiber_end_detector->algorithm_parameter()->save_to_node(doc,L("algorithm_parameter"));
    root.appendChild(algorithm_node);
    QDomElement server_node = m_config_data->save_to_node(doc,L("server_parameter"));
    root.appendChild(server_node);
    // 写入文件
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    doc.save(out, 4);  // 保存格式化的 XML,第二个参数表示缩进的空格数，用于控制 QDomDocument::save() 输出 XML 时的缩进格式
    file.close();
	return true;
}

void thread_misc::process_task(const QVariant& task_data)
{
    QJsonObject obj = task_data.toJsonObject();
    QString command = obj["command"].toString();
	QJsonObject result_obj;     //返回的消息对象
    result_obj["request_id"] = obj["request_id"];
    if (command == "client_request_open_camera")
    {
		QString unique_id = obj["param"].toString();
		st_device_info* device_info = m_device_manager->get_device_info(unique_id);
        if(device_info == nullptr)
        {
            result_obj["command"] = "server_report_info";
            result_obj["param"] = QString("未找到相机设备: %1").arg(unique_id);
		}
        else
        {
            m_camera = camera_factory::create_camera(device_info);
            if(m_camera == nullptr)
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("无法创建相机设备: %1").arg(unique_id);
			}
            else
            {
                if(m_camera->open() != STATUS_SUCCESS)
                {
                    result_obj["command"] = "server_report_info";
                    result_obj["param"] = QString("无法打开相机设备: %1").arg(unique_id);
                }
                else
                {
                    //关联信号槽，获取相机取图成功的数据
                    connect(m_camera, &interface_camera::post_stream_image_ready, this, &thread_misc::on_stream_image_ready);
                    //根据 unique 获取相机参数，然后设置到相机
                    st_camera_config camera_config = m_camera_config_mgr.get_camera_config(m_camera->m_unique_id);
                    m_camera->import_config(camera_config);
                	//无论是否设置成功，都需要设置成连续触发和软触发，并且在此之后开始采集
                    {
                        m_camera->set_trigger_source(global_trigger_source_software);
						//一部分相机存在bug，需要先将循环触发关闭再打开才有效
                        {
                            m_camera->set_trigger_mode(global_trigger_mode_continuous);
                            m_camera->set_trigger_mode(global_trigger_mode_once);
                        }
						m_camera->set_trigger_mode(global_trigger_mode_continuous);
						m_camera->start_grab();
                    }
                    result_obj["command"] = "server_camera_opened_success";
                    QJsonObject camera_obj = camera_parameter_to_json(m_camera);
                    result_obj["camera"] = camera_obj;          // 将相机参数转换为 JSON 对象
					result_obj["unique_id"] = unique_id;        // 返回相机的唯一标识符
                }
            }
        }
        emit post_task_finished(QVariant::fromValue(result_obj));
    }
    else if(command == "client_request_close_camera")
    {
        if(m_camera != nullptr)
        {
            m_camera->close();
        	disconnect(m_camera, &interface_camera::post_stream_image_ready, this, &thread_misc::on_stream_image_ready);
        }
        result_obj["command"] = "server_camera_closed_success";
        emit post_task_finished(QVariant::fromValue(result_obj));
    }
    else if(command == "client_request_change_camera_parameter")
    {
        if (m_camera != nullptr)
        {
            QJsonObject param_obj = obj["param"].toObject();
            QString name = param_obj["name"].toString();
            result_obj["name"] = name;
			int ret = STATUS_SUCCESS;
            if (name == "fps")
            {
                ret = m_camera->set_frame_rate(param_obj["value"].toDouble());
                result_obj["value"] = m_camera->get_frame_rate();
				result_obj["range"] = range_to_json(m_camera->get_frame_rate_range());
            }
            else if (name == "start_x")
            {
                ret = m_camera->set_start_x(param_obj["value"].toInt());
                result_obj["value"] = m_camera->get_start_x();
                result_obj["range"] = range_to_json(m_camera->get_width_range());
            }
            else if (name == "start_y")
            {
                ret = m_camera->set_start_y(param_obj["value"].toInt());
                result_obj["value"] = m_camera->get_start_y();
                result_obj["range"] = range_to_json(m_camera->get_height_range());
            }
            else if (name == "width")
            {
                ret = m_camera->set_width(param_obj["value"].toInt());
                result_obj["value"] = m_camera->get_width();
                result_obj["range"] = range_to_json(m_camera->get_start_x_range());
            }
            else if (name == "height")
            {
                ret = m_camera->set_height(param_obj["value"].toInt());
                result_obj["value"] = m_camera->get_height();
                result_obj["range"] = range_to_json(m_camera->get_start_y_range());
            }
            else if (name == "pixel_format")
            {
                ret = m_camera->set_pixel_format(param_obj["value"].toString());
                result_obj["value"] = m_camera->get_pixel_format();
            }
            else if (name == "auto_exposure_mode")
            {
                ret = m_camera->set_auto_exposure_mode(param_obj["value"].toString());
                result_obj["value"] = m_camera->get_auto_exposure_mode();
            }
            else if (name == "auto_exposure_time_floor")
            {
                ret = m_camera->set_auto_exposure_time_floor(param_obj["value"].toDouble());
                result_obj["value"] = m_camera->get_auto_exposure_time_floor();
                result_obj["range"] = range_to_json(m_camera->get_auto_exposure_time_upper_range());
            }
            else if (name == "auto_exposure_time_upper")
            {
                ret = m_camera->set_auto_exposure_time_upper(param_obj["value"].toDouble());
                result_obj["value"] = m_camera->get_auto_exposure_time_upper();
                result_obj["range"] = range_to_json(m_camera->get_auto_exposure_time_floor_range());
            }
            else if (name == "exposure_time")
            {
                ret = m_camera->set_exposure_time(param_obj["value"].toDouble());
                result_obj["value"] = m_camera->get_exposure_time();
                result_obj["range"] = range_to_json(m_camera->get_exposure_time_range());
            }
            else if (name == "auto_gain_mode")
            {
                ret = m_camera->set_auto_gain_mode(param_obj["value"].toString());
                result_obj["value"] = m_camera->get_auto_gain_mode();
            }
            else if(name == "auto_gain_floor")
            {
                ret = m_camera->set_auto_gain_floor(param_obj["value"].toDouble());
                result_obj["value"] = m_camera->get_auto_gain_floor();
                result_obj["range"] = range_to_json(m_camera->get_auto_gain_upper_range());
            }
            else if (name == "auto_gain_upper")
            {
                ret = m_camera->set_auto_gain_upper(param_obj["value"].toDouble());
                result_obj["value"] = m_camera->get_auto_gain_upper();
                result_obj["range"] = range_to_json(m_camera->get_auto_gain_floor_range());
            }
            else if (name == "gain")
            {
                ret = m_camera->set_gain(param_obj["value"].toDouble());
                result_obj["value"] = m_camera->get_gain();
                result_obj["range"] = range_to_json(m_camera->get_gain_range());
            }
            else if (name == "trigger_mode")
            {
                ret = m_camera->set_trigger_mode(param_obj["value"].toString());
                result_obj["value"] = m_camera->get_trigger_mode();
            }
            else if (name == "trigger_source")
            {
                ret = m_camera->set_trigger_source(param_obj["value"].toString());
                result_obj["value"] = m_camera->get_trigger_source();
            }
            if(ret != STATUS_SUCCESS)
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("设置相机参数失败: %1").arg(m_camera->map_ret_status(ret));
                emit post_task_finished(QVariant::fromValue(result_obj));
            }
            else
            {
                if(name != "trigger_mode" && name != "trigger_source")
                {
                    //对于非触发模式和触发源的参数修改，需要保存到相机配置文件中
                    st_camera_config camera_config = m_camera->export_config();
                    m_camera_config_mgr.update_camera_config(camera_config);
                    m_camera_config_mgr.save_to_file();
				}
                // 成功后返回修改的参数
                result_obj["command"] = "server_camera_parameter_changed_success";
            	emit post_task_finished(QVariant::fromValue(result_obj));
			}
        }
        else
        {
            result_obj["command"] = "server_report_info";
            result_obj["param"] = QString("相机对象无效！");
            result_obj["request_id"] = obj["request_id"];
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
    }
    else if(command == "client_request_start_grab")
    {
        if(m_camera != nullptr)
        {
			bool start = obj["param"].toBool();
            int ret = STATUS_SUCCESS;
            QString report_info("");
            if(start)
            {
                ret = m_camera->start_grab();
                if(ret != STATUS_SUCCESS)
                {
                    result_obj["command"] = "server_report_info";
                    report_info = QString("开始采集失败: %1").arg(m_camera->map_ret_status(ret));
                    result_obj["param"] = report_info;
                    emit post_task_finished(QVariant::fromValue(result_obj));
                    return;
				}
                //如果是连续模式，记录 request_id
                if(m_camera->get_trigger_mode() == global_trigger_mode_continuous)
                {
                    m_stream_request_id = obj["request_id"].toString();
                    result_obj["task_finish"] = false;
                }
            }
            else
            {
				ret = m_camera->stop_grab();
                if (ret != STATUS_SUCCESS)
                {
                    result_obj["command"] = "server_report_info";
                    report_info = QString("停止采集失败: %1").arg(m_camera->map_ret_status(ret));
                    result_obj["param"] = report_info;
                    emit post_task_finished(QVariant::fromValue(result_obj));
                    return;
                }
            }
            result_obj["command"] = "server_camera_grab_set_success";
            result_obj["param"] = start;
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
        else
        {
            result_obj["command"] = "server_report_info";
            result_obj["param"] = QString("相机对象无效！");
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
        
    }
    else if(command == "client_request_trigger_once")
    {
        // 软触发模式下使用
        if (m_camera != nullptr)
        {
            QImage img = m_camera->trigger_once();
            if(0 && !img.isNull())
            {
                QString path = QString("C:/Temp/server.png");
                img.save(path);  // 保存为 PNG 文件
            }
            //触发失败,返回消息. 如果成功这里不处理，在相机发送信号的槽函数中处理
            if (img.isNull())
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("触发采图失败");
            }
            else
            {
                st_image_meta meta;
                if (!m_shared_memory_trigger_image.write_image(img, meta))
                {
                    result_obj["command"] = "server_report_info";
                    result_obj["param"] = QString("写入图片数据失败");
                }
                else
                {
                    result_obj["command"] = "server_camera_trigger_once_success";
                    QJsonObject json = image_shared_memory::meta_to_json(meta);
                    result_obj["param"] = json;
                }
            }
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
	}
    else if(command == "client_request_change_algorithm_parameter")
    {
	    if(m_fiber_end_detector != nullptr)
	    {
            QJsonObject param_obj = obj["param"].toObject();
            QString key = param_obj["key"].toString();
            double value = param_obj["value"].toDouble();
            m_fiber_end_detector->set_algorithm_parameter(key, value);
            //回复消息
            QJsonObject ret_obj = m_fiber_end_detector->algorithm_parameter()->save_to_json(key);
            result_obj["command"] = "server_algorithm_parameter_changed_success";
            result_obj["param"] = ret_obj;
            emit post_task_finished(QVariant::fromValue(result_obj));
	    }
        else
        {
            result_obj["command"] = "server_report_info";
            result_obj["param"] = QString("检测算法对象无效！");
            result_obj["request_id"] = obj["request_id"];
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
    }
    else if (command == "client_request_user_config_set")
    {
        QJsonObject param = obj["param"].toObject();
		bool load_flag = param["load"].toBool();
		QString file_path = param["path"].toString();
		//load_flag: 加载(true)或保存(false)标识; file_path: 配置文件路径
        if(load_flag)
        {
	        if(load_user_config_file(file_path))
	        {
		        //加载成功,回复所有参数
				result_obj["command"] = "server_load_user_config_success";
				result_obj["camera"] = camera_parameter_to_json(m_camera);
				result_obj["algorithm_parameter"] = m_fiber_end_detector->algorithm_parameter()->save_to_json();
				result_obj["fiber_end_parameter"] = m_config_data->save_to_json();
	        }
	        else
	        {
		        //加载失败,回复错误消息
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("加载配置文件失败: %1").arg(file_path);
	        }
        }
        else
        {
            //保存成功或失败均回复提示信息
	        if(save_user_config_file(file_path))
            {
                //保存成功,回复所有参数
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("保存成功!");
            }
            else
            {
                //保存失败,回复错误消息
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("保存配置文件失败: %1").arg(file_path);
            }
        }
        emit post_task_finished(QVariant::fromValue(result_obj));
    }
    else if(command == "client_request_move_camera")
    {
        result_obj["command"] = "server_move_camera_success";   //使用统一回复命令，涉及到移动+取图两个步骤，在result_obj["image"]中存储取图状态
        QJsonObject param = obj["param"].toObject();
        int pos_x(0), pos_y(0);
        if (param["name"] == "move_to_position")
        {
            //m_calc_image_clarity = true;
            //m_clarity_images.clear();
            if (m_motion_control != nullptr)
            {
                pos_x = param["x"].toInt();
                pos_y = param["y"].toInt();
                m_motion_control->move_position(0, pos_y, m_config_data->m_move_speed);
                m_motion_control->move_position(1, pos_x, m_config_data->m_move_speed);
                m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
            }
            //m_calc_image_clarity = false;
            //std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            //for (size_t i = 0;i < m_clarity_images.size();i++)
            //{
            //    QString path = QString("C:/Temp/frame_images_%1.png").arg(i, 2, 10, QChar('0'));
            //    m_clarity_images[i].save(path);
            //}
            
        }
	    else if (param["name"] == "move_forward_y")
	    {
            //限制位判断，超出范围时返回提示信息
            if(m_config_data->m_position_y + m_config_data->m_move_step_y > m_config_data->m_max_y)
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("Y 轴移动超出范围: 当前位置 %1, 最大位置 %2, 移动步长 %3")
                                        .arg(m_config_data->m_position_y).arg(m_config_data->m_max_y).arg(m_config_data->m_move_step_y);
                emit post_task_finished(QVariant::fromValue(result_obj));
                return;
			}
            if(m_motion_control != nullptr)
            {
                m_motion_control->move_distance(0, m_config_data->m_move_step_y, m_config_data->m_move_speed);
                m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
            }
	    }
        else if (param["name"] == "move_back_x")
        {
        	if (m_motion_control != nullptr)
            {
                m_motion_control->move_distance(1, -m_config_data->m_move_step_x, m_config_data->m_move_speed);
                m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
            }
        }
        else if (param["name"] == "move_forward_x")
        {
			//限制位判断，超出范围时返回提示信息
            if (m_config_data->m_position_x + m_config_data->m_move_step_x > m_config_data->m_max_x)
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("X 轴移动超出范围: 当前位置 %1, 最大位置 %2, 移动步长 %3")
                    .arg(m_config_data->m_position_x).arg(m_config_data->m_max_x).arg(m_config_data->m_move_step_x);
                emit post_task_finished(QVariant::fromValue(result_obj));
                return;
            }
            if (m_motion_control != nullptr)
            {
                m_motion_control->move_distance(1, m_config_data->m_move_step_x, m_config_data->m_move_speed);
                m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
            }
        }
        else if (param["name"] == "move_back_y")
        {
            if (m_motion_control != nullptr)
            {
                m_motion_control->move_distance(0, -m_config_data->m_move_step_y, m_config_data->m_move_speed);
                m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
            }
        }
        result_obj["x"] = m_config_data->m_position_x;
        result_obj["y"] = m_config_data->m_position_y;
        //移动相机之后采图,触发模式下需要采图，连续模式下会自动采图
        if (m_camera != nullptr)
        {
            m_camera->start_grab();     //可能没有开始采集
            if(m_camera->get_trigger_mode() == global_trigger_mode_once)
            {
                QImage img = m_camera->trigger_once();
                if (img.isNull())
                {
                    result_obj["image"] = "trigger error";
                }
                else
                {
                    st_image_meta meta;
                    if (!m_shared_memory_trigger_image.write_image(img, meta))
                    {
                        result_obj["image"] = "write image error";
                    }
                    else
                    {
                        result_obj["image"] = "success";
                        QJsonObject json = image_shared_memory::meta_to_json(meta);
                        result_obj["image_data"] = json;
                    }
                }
            }
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
        else
        {
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
    }
    else if(command == "client_request_move_camera_by_index")
    {
        int index = obj["param"].toInt();
        if (index >= m_config_data->m_photo_location_list.size())
        {
            result_obj["command"] = "server_report_info";
            result_obj["param"] = QString("找不到目标位置: 共有 %1 个拍照位，您选中第 %2 个!")
        								.arg(m_config_data->m_photo_location_list.size()).arg(index + 1);
            emit post_task_finished(QVariant::fromValue(result_obj));
        }
        else
        {
            int pos_x = m_config_data->m_photo_location_list[index].m_x;
            int pos_y = m_config_data->m_photo_location_list[index].m_y;
            move_to_position(pos_x, pos_y, obj["request_id"].toString(), true);
        }
    	
    }
    else if(command == "client_request_set_motion_parameter")
    {
        QJsonObject param = obj["param"].toObject();
        
    	if(param["name"] == "set_light_brightness")
        {
            int light_brightness = param["value"].toInt();
            bool ret = m_motion_control->set_light_source_param(1000000, light_brightness, 1);
            if(ret)
            {
                m_config_data->m_light_brightness = light_brightness;
                m_config_data->save();
            }
            else
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("设置光源亮度失败!");
            }
        }
        else if(param["name"] == "set_move_speed")
        {
            int move_speed = param["value"].toInt();
            m_config_data->m_move_speed = move_speed;
            m_config_data->save();
        }
        else if (param["name"] == "set_max_x")
        {
            int max_x = param["value"].toInt();
            m_config_data->m_max_x = max_x;
            m_config_data->save();
        }
        else if (param["name"] == "set_max_y")
        {
            int max_y = param["value"].toInt();
            m_config_data->m_max_y = max_y;
            m_config_data->save();
        }
        else if (param["name"] == "set_move_step_x")
        {
            int move_step_x = param["value"].toInt();
            m_config_data->m_move_step_x = move_step_x;
            m_config_data->save();
        }
        else if (param["name"] == "set_move_step_y")
        {
            int move_step_y = param["value"].toInt();
            m_config_data->m_move_step_y = move_step_y;
            m_config_data->save();
        }
        else if (param["name"] == "set_zero")
        {
            bool ret = m_motion_control->set_current_position_zero(0);
            if(ret)
            {
                ret = m_motion_control->set_current_position_zero(1);
                if (ret)
                {
                    m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
                    result_obj["command"] = "server_set_motion_parameter_success";
                    result_obj["name"] = param["name"];
                    result_obj["x"] = m_config_data->m_position_x;
                    result_obj["y"] = m_config_data->m_position_y;
                }
                else
                {
                    result_obj["command"] = "server_report_info";
                    result_obj["param"] = QString("设置零点失败!");
                }
            }
            else
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("设置零点失败!");
            }
        }
        else if (param["name"] == "reset_position")
        {
            bool ret = m_motion_control->reset(0);
            if (ret)
            {
                ret = m_motion_control->reset(1);
                if (ret)
                {
                    m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
                    result_obj["command"] = "server_set_motion_parameter_success";
                    result_obj["name"] = param["name"];
                    result_obj["x"] = m_config_data->m_position_x;
                    result_obj["y"] = m_config_data->m_position_y;
                }
                else
                {
                    result_obj["command"] = "server_report_info";
                    result_obj["param"] = QString("复位失败!");
                }
            }
            else
            {
                result_obj["command"] = "server_report_info";
                result_obj["param"] = QString("复位失败!");
            }
        }
        emit post_task_finished(QVariant::fromValue(result_obj));
    }
    else if(command == "client_request_auto_focus")
    {
        if(m_auto_focus == nullptr)
        {
            QString current_directory = QCoreApplication::applicationDirPath();
            std::string calibration_file_path = (current_directory + "/calibration.bin").toStdString();
            m_auto_focus = new auto_focus(calibration_file_path, m_motion_control, m_camera, m_config_data->m_fiber_end_count);
        }
        std::vector<cv::Mat> images = m_auto_focus->get_focus_images(m_config_data->m_position_y);
        if(1)
        {
            for (int i = 0; i < images.size();i++)
            {
                char szPath[256] = { 0 };
                sprintf_s(szPath, "C:/Temp/focus_%d.png", i);
                //QString file_path = QString("D:/Temp/focus_%1.png").arg(i);
                cv::imwrite(szPath,images[i]);
            }
        }
    }
    else if(command == "client_request_anomaly_detection")
    {
        anomaly_detection(result_obj["request_id"].toString(), 0, true);
    }
    else if(command == "client_request_auto_calibration")
    {
        result_obj["command"] = "server_auto_calibration_status";
        m_is_processing.store(true);
        //自动标定,切换到触发模式
        if(m_camera->get_trigger_mode() != global_trigger_mode_once)
        {
            m_camera->stop_grab();
            m_camera->set_trigger_mode(global_trigger_mode_once);
            m_camera->set_trigger_source(global_trigger_source_software);
        }
        m_camera->start_grab();
        /*********************** 1.根据像素直径创建精匹配模型文件 **********************/
        //返回消息-正在初始化匹配参数
        result_obj["task_finish"] = false;
        result_obj["process"] = 0;
        result_obj["status"] = L("正在初始化匹配参数...");
        emit post_task_finished(QVariant::fromValue(result_obj));
        double diameter = obj["diameter"].toDouble();
        cv::Mat mask(static_cast<int>(diameter), static_cast<int>(diameter), CV_8UC1, cv::Scalar(0)); // 单通道、全黑
        int radius = diameter / 2;
        cv::Point center(diameter / 2, diameter / 2);
        cv::circle(mask, center, radius, cv::Scalar(255), cv::FILLED, cv::LINE_AA); // 填充白色
        QString current_directory = QCoreApplication::applicationDirPath();
        QString shape_model_dir = current_directory + L("/shape_model");
        make_path(shape_model_dir);
    	std::string shape_model_path = (shape_model_dir + "/model.bin").toStdString();
        m_fiber_end_detector->create_shape_model(mask, 0.0, 0.0, 1.0, 0.96, 1.04, 0.02,
            0, 400, cv::Mat(), shape_model_path);
        /*********************** 2.精匹配得到像素物理尺寸(um) **********************/
        //返回消息-正在计算像素尺寸
        result_obj["process"] = 33;
        result_obj["status"] = L("正在计算像素尺寸...");
        emit post_task_finished(QVariant::fromValue(result_obj));
        double center_distance = obj["center_distance"].toDouble();
        
        QImage img = m_camera->trigger_once();
        cv::Mat image_gray = convert_qimage_to_cvmat(img, 1);
        int step_width = image_gray.cols / m_config_data->m_fiber_end_count;
        int start_x(0);
        cv::Rect roi;
        cv::Rect empty_rect = cv::Rect(0, 0, 0, 0);
        std::vector<st_position> centers;
        centers.resize(m_config_data->m_fiber_end_count);
        for (int i = 0;i < m_config_data->m_fiber_end_count;i++)
        {
            start_x = i * step_width;
            cv::Mat roi_image = get_roi_image(image_gray, st_detect_box(0.0, start_x, 0, start_x + step_width, image_gray.rows), 
								0, 1, roi);
            st_detect_box box = m_fiber_end_detector->get_shape_match_result(roi_image);
            if(box.is_valid())
            {
                centers[i] = st_position((box.m_x0 + box.m_x1) / 2 + start_x, (box.m_y0 + box.m_y1) / 2);
            }
        }
        //计算平均距离
        double dist = 0.0;
        for (int i = 0; i < m_config_data->m_fiber_end_count - 1; i++)
        {
            int dx = centers[i + 1].m_x - centers[i].m_x;
            int dy = centers[i + 1].m_y - centers[i].m_y;
            dist += sqrt(dx * dx + dy * dy);
        }
        dist = dist / (m_config_data->m_fiber_end_count - 1) + 1e-9;
        m_fiber_end_detector->set_algorithm_parameter("pixel_physical_size", center_distance / dist);
        {
	        //这里也需要回复消息,更新界面参数
            QJsonObject param_obj = m_fiber_end_detector->algorithm_parameter()->save_to_json("pixel_physical_size");
            QJsonObject ret_obj;
            ret_obj["command"] = "server_algorithm_parameter_changed_success";
            ret_obj["request_id"] = obj["request_id"];
            ret_obj["task_finish"] = false;
            ret_obj["param"] = param_obj;
            emit post_task_finished(QVariant::fromValue(ret_obj));
        }
        /*********************** 3.清晰度标定 **********************/
        //返回消息-正在进行清晰度标定
        result_obj["process"] = 66;
        result_obj["status"] = L("正在进行清晰度标定...");
        emit post_task_finished(QVariant::fromValue(result_obj));
        if (m_auto_focus == nullptr)
        {
            std::string calibration_file_path = (current_directory + "/calibration.bin").toStdString();
            m_auto_focus = new auto_focus(calibration_file_path, m_motion_control, m_camera, m_config_data->m_fiber_end_count);
        }
        m_auto_focus->calibrate();
        std::string calibration_file_path = (current_directory + "/calibration.bin").toStdString();
        m_auto_focus->save(calibration_file_path);
        m_is_processing.store(false);
        //移动回当前位置
    	move_to_position(m_config_data->m_position_x, m_config_data->m_position_y , result_obj["request_id"].toString());
        //处理完毕，切换到连续模式
        m_camera->stop_grab();
        m_camera->set_trigger_mode(global_trigger_mode_continuous);
        m_camera->set_trigger_source(global_trigger_source_software);
        m_camera->start_grab();

        //返回消息-处理完毕
        result_obj["task_finish"] = true;
        result_obj["process"] = 100;
        result_obj["status"] = L("处理完毕!");
        emit post_task_finished(QVariant::fromValue(result_obj));
    }
    else if(command == "client_request_update_server_parameter")
    {
        QJsonObject param = obj["param"].toObject();
        if (param["name"] == "update_photo_location_list")
        {
            std::vector<st_position> positions;
            QJsonArray posArray = param["photo_location_list"].toArray();
            for (int i = 0; i < posArray.size(); i++)
            {
                QJsonObject obj = posArray[i].toObject();
                int  x = obj["x"].toInt();
                int  y = obj["y"].toInt();
                positions.emplace_back(st_position(x, y));
            }
            if(m_config_data->position_list_changed(positions))
            {
                m_config_data->m_photo_location_list = positions;
                m_config_data->save();
            }
        }
        else if (param["name"] == "update_fiber_end_count")
        {
            int fiber_end_count = param["fiber_end_count"].toInt();
            if (m_config_data->m_fiber_end_count != fiber_end_count)
            {
                m_config_data->m_fiber_end_count = fiber_end_count;
                m_config_data->save();
            }
            //通知自动对焦模块
            if(m_auto_focus != nullptr)
            {
                m_auto_focus->set_fiber_end_count(fiber_end_count);
            }
        }
        else if (param["name"] == "update_fiber_end_physical_size")
        {
            double fiber_end_physical_size = param["fiber_end_physical_size"].toDouble();
            if (fabs(m_config_data->m_fiber_end_physical_size - fiber_end_physical_size) > 0.0001)
            {
                m_config_data->m_fiber_end_physical_size = fiber_end_physical_size;
                m_config_data->save();
            }
        }
        else if (param["name"] == "update_field_of_view")
        {
            double field_of_view = param["field_of_view"].toDouble();
            if (fabs(m_config_data->m_field_of_view - field_of_view) > 0.0001)
            {
                m_config_data->m_field_of_view = field_of_view;
                m_config_data->save();
            }
        }
    	else if (param["name"] == "update_auto_detect")
        {
            int auto_detect = param["auto_detect"].toInt();
            if (m_config_data->m_auto_detect != auto_detect)
            {
                m_config_data->m_auto_detect = auto_detect;
                m_config_data->save();
            }
        }
        else if (param["name"] == "update_save_path")
        {
            QString save_path = param["save_path"].toString();
            if (m_config_data->m_save_path != save_path.toStdString())
            {
                m_config_data->m_save_path = save_path.toStdString();
                m_config_data->save();
            }
        }
    }
    else if(command == "client_request_start_process")
    {
    	start_process(obj["request_id"].toString());
    }
    else if(command == "device_request_start_process")
    {
        start_process(obj["request_id"].toString());
    }
}

void thread_misc::on_stream_image_ready(const QString& camera_id, const QImage& img)
{
	//向前端发消息
    if(!img.isNull())
    {
        QJsonObject result_obj;     //返回的消息对象
        result_obj["request_id"] = m_stream_request_id;
        result_obj["task_finish"] = false;
        st_image_meta meta;
        if (!m_shared_memory_trigger_image.write_image(img, meta))
        {
            result_obj["command"] = "server_report_info";
            result_obj["param"] = QString("写入图片数据失败");
        }
        else
        {
            //if(m_calc_image_clarity)
            //{
            //    m_clarity_images.emplace_back(img);
            //}
            result_obj["command"] = "server_camera_stream_image_ready";
            QJsonObject json = image_shared_memory::meta_to_json(meta);
            result_obj["param"] = json;
        }
        emit post_task_finished(QVariant::fromValue(result_obj));
    }
}

void thread_misc::on_device_request_start_process()
{
    QJsonObject obj;
	obj["command"] = "device_request_start_process";
	obj["request_id"] = "";     //所有界面响应
    add_task(obj);
    //start_process(L(""));
}

int thread_misc::start_process(const QString& request_id)
{
	if(m_is_processing.load())      //防止重复下发
	{
        return 1;
	}
    std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
    m_is_processing.store(true);
	{
        //首先删除内部目录中的数据，自动切换到触发模式，然后向客户端回复消息，通知正在运行，禁用界面上相关按钮
        QString current_directory = QCoreApplication::applicationDirPath();
        QString image_dir = current_directory + "/detect_images";
        make_path(image_dir);
        remove_files(image_dir, L("*.png"));        //移除目录中所有文件
		if (m_camera->get_trigger_mode() != global_trigger_mode_once)
        {
            m_camera->stop_grab();
			m_camera->set_trigger_mode(global_trigger_mode_once);
			m_camera->set_trigger_source(global_trigger_source_software);
        }
        m_camera->start_grab();

        QJsonObject ret_obj;     //返回的消息对象
        ret_obj["request_id"] = request_id;
        ret_obj["command"] = "server_process_status";
        ret_obj["task_finish"] = false;
        ret_obj["param"] = -1;
        emit post_task_finished(QVariant::fromValue(ret_obj));
	}
	for (int i = 0;i < m_config_data->m_photo_location_list.size();i++)
    {
        if(m_terminate.load())
        {
	        break;
        }
	    /******************1.运动到拍照位*******************/
        int pos_x = m_config_data->m_photo_location_list[i].m_x;
        int pos_y = m_config_data->m_photo_location_list[i].m_y;
        if(!move_to_position(pos_x,pos_y, request_id))
        {
	        continue;
        }
        if (m_terminate.load())
        {
            break;
        }
        /******************2.异常检测*******************/
        //检测失败的时候直接退出，不再进行下一步处理
        if (!anomaly_detection(request_id, i, false))
        {
	        break;
        }
    }
    std::chrono::steady_clock::time_point end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double use_time = duration_ms.count() / 1000.0;
    //检测完毕，回到第一个拍照位置
    if(m_config_data->m_photo_location_list.size() > 0)
    {
        move_to_position(m_config_data->m_photo_location_list[0].m_x, m_config_data->m_photo_location_list[0].m_y, request_id);
    }
    //切换回连续模式
    m_camera->stop_grab();
    m_camera->set_trigger_mode(global_trigger_mode_continuous);
    m_camera->set_trigger_source(global_trigger_source_software);
	m_camera->start_grab();

    m_is_processing.store(false);
	{
        int ret = m_terminate.load();
        //回复消息，通知运行完毕，恢复界面上相关按钮状态
        QJsonObject ret_obj;                                        // 返回的消息对象
        ret_obj["request_id"] = request_id;
        ret_obj["command"] = "server_process_status";
        ret_obj["task_finish"] = true;
        ret_obj["param"] = ret;                             // 返回运行状态，如果是执行中断，界面上需要有提示
        ret_obj["use_time"] = use_time;                     // 如果是正常执行完毕，显示算法运行时间
        emit post_task_finished(QVariant::fromValue(ret_obj));
	}
    m_terminate.store(false);
    return 0;
}

bool thread_misc::move_to_position(int pos_x, int pos_y, const QString& request_id, bool task_finish)
{
    if (m_motion_control == nullptr)
    {
        return false;
    }
    m_motion_control->move_position(0, pos_y, m_config_data->m_move_speed);
    m_motion_control->move_position(1, pos_x, m_config_data->m_move_speed);
    m_motion_control->get_position(m_config_data->m_position_y, m_config_data->m_position_x);
    bool ret(false);
    QJsonObject ret_obj;     //返回的消息对象
    ret_obj["request_id"] = request_id;
    ret_obj["command"] = "server_move_camera_success";      //回复消息，移动相机成功
    ret_obj["task_finish"] = task_finish;                         //要添加此标识，表示任务没有执行完
    ret_obj["x"] = m_config_data->m_position_x;
    ret_obj["y"] = m_config_data->m_position_y;
    //移动相机之后采图,触发模式下需要采图，连续模式下会自动采图
    //该接口会在两种场景下使用: (1) 用户双击列表项时(不一定设置) (2) 用户开始检测时(已在外部设置触发模式且处于采集状态)
    if (m_camera != nullptr)
    {
        m_camera->start_grab();     //可能没有开始采集
        if(m_camera->get_trigger_mode() == global_trigger_mode_once)
        {
            QImage img = m_camera->trigger_once();
            if (img.isNull())
            {
                ret_obj["image"] = "trigger error";
            }
            else
            {
                st_image_meta meta;
                if (!m_shared_memory_trigger_image.write_image(img, meta))
                {
                    ret_obj["image"] = "write image error";
                }
                else
                {
                    ret_obj["image"] = "success";
                    QJsonObject json = image_shared_memory::meta_to_json(meta);
                    ret_obj["image_data"] = json;
                    ret = true;
                }
            }
            emit post_task_finished(QVariant::fromValue(ret_obj));
        }
    }
    return ret;
}

bool thread_misc::anomaly_detection(const QString& request_id, int index, bool is_task_finish)
{
    QJsonObject result_obj;     //返回的消息对象
    result_obj["request_id"] = request_id;
    result_obj["task_finish"] = is_task_finish;
    result_obj["start_index"] = index * m_config_data->m_fiber_end_count;
    result_obj["fiber_end_count"] = m_config_data->m_fiber_end_count;
    QString current_directory = QCoreApplication::applicationDirPath();
    if (m_fiber_end_detector == nullptr)
    {
        m_fiber_end_detector = new fiber_end_algorithm;
        std::string shape_model_path = (current_directory + "/shape_model/model.bin").toStdString();
        std::string algorithm_parameter_path = (current_directory + "/parameter.xml").toStdString();
        if (!m_fiber_end_detector->initialize(m_config_data->m_fiber_end_pixel_size, algorithm_parameter_path))
        {
            delete m_fiber_end_detector;
            m_fiber_end_detector = nullptr;
            std::cerr << "fiber_end_algorithm initialize error!" << std::endl;
            result_obj["command"] = "server_anomaly_detection_finish";
            result_obj["param"] = QString::fromStdString("算法初始化失败!");
            emit post_task_finished(QVariant::fromValue(result_obj));
            return false;
        }
    }
    if (m_auto_focus == nullptr)
    {
        std::string calibration_file_path = (current_directory + "/calibration.bin").toStdString();
        m_auto_focus = new auto_focus(calibration_file_path, m_motion_control, m_camera, m_config_data->m_fiber_end_count);
    }
    QDateTime datetime = QDateTime::currentDateTime();      //时间戳, 用于临时文件命名
    QString qCurrentTime = datetime.toString("yyyy-MM-dd-HH-mm-ss");
    /*********************1.得到若干清晰的单通道端面影像，每张影像上包含一个端面********************/
    std::vector<cv::Mat> images = m_auto_focus->get_focus_images(m_config_data->m_photo_location_list[index].m_y);
    if (images.size() == 0)
    {
        result_obj["command"] = "server_anomaly_detection_finish";
        result_obj["param"] = L("自动对焦失败,请检查影像端面数量并进行自动标定!");
        emit post_task_finished(QVariant::fromValue(result_obj));
        return false;
    }
    if(save_focus_image)            //将自动对焦结果保存在指定位置，内部调试使用
    {
        QString focus_dir = current_directory + "/focus_images";        //内部自动对焦结果目录
        make_path(focus_dir);
        for (int i = 0; i < images.size(); i++)
        {
            QString path = QString("%1/%2_%3.jpg").arg(focus_dir).arg(qCurrentTime).arg(index * m_config_data->m_fiber_end_count + i);
            cv::imwrite(l(path).c_str(), images[i]);
        }
    }
    /*********************2.精定位，然后将结果显示在界面上********************/
    QString image_dir = current_directory + "/detect_images";        //前后端影像传输目录,包含两张影像，index_shape.png:精定位结果 index_det.png:检测结果
    make_path(image_dir);
    QString user_dir = L(m_config_data->m_save_path.c_str());  //外部指定的保存目录
    make_path(user_dir);
    /******************************
     * 执行流程:
     * (1) 精定位得到结果 box
     * (2) 定位失败，输出定位结果(原始数据)
     * (3) 定位成功
     *      3.1 如果开启了自动检测,自动检测，外扩之后输出定位结果 + 检测结果
     *      3.2 如果没有开启自动检测，外扩之后输出定位结果
     ******************************/
    for (int i = 0; i < images.size(); i++)
    {
        // 精定位
        st_detect_box box = m_fiber_end_detector->get_shape_match_result(images[i]);
        cv::Mat shape_image = images[i];    //原始数据
        if(!box.is_valid())
        {
	        // 定位失败，输出原始数据
            QString dst_image_path = QString("%1/%2_%3_shape.png").arg(user_dir).arg(qCurrentTime).arg(index * m_config_data->m_fiber_end_count + i);
            cv::imwrite(dst_image_path.toStdString(), shape_image);
            dst_image_path = QString("%1/%2_shape.png").arg(image_dir).arg(index * m_config_data->m_fiber_end_count + i);
        	cv::imwrite(dst_image_path.toStdString(), shape_image);
        }
        else
        {
            //定位成功，首先保存外扩之后的定位结果
            cv::Rect buffer_roi;
            st_detect_box buffer_box = box.buffer(m_config_data->m_field_of_view);  //首先进行外扩
            cv::Mat buffer_shape_image = get_roi_image(images[i], buffer_box, 0, 1, buffer_roi);    //得到外扩影像及其在原始影像上的区域
            //cv::Mat buffer_shape_image = get_roi_image_padding(images[i], buffer_box, 1);    //得到外扩影像及其在原始影像上的区域
        	QString dst_image_path = QString("%1/%2_%3_shape.png").arg(user_dir).arg(qCurrentTime).arg(index * m_config_data->m_fiber_end_count + i);
            cv::imwrite(dst_image_path.toStdString(), buffer_shape_image);
            dst_image_path = QString("%1/%2_shape.png").arg(image_dir).arg(index * m_config_data->m_fiber_end_count + i);
            cv::Mat enhance_shape_image = unsharp_masking(buffer_shape_image, 1.0, 7);
        	cv::imwrite(dst_image_path.toStdString(), enhance_shape_image);
            //如果开启了自动检测, 在原始定位结果的基础上执行自动检测，然后外扩保存
            //if(m_config_data->m_auto_detect)
			if (0)          //这里禁用自动检测功能
            {
                cv::Rect roi;
                shape_image = get_roi_image(images[i], box, 0, 1, roi);
                st_detect_result detect_result = m_fiber_end_detector->get_detect_result(shape_image);
                //这里检测结果的坐标是相对于roi区域的，但我们保存的影像数据是buffer_roi区域，外扩之后需要对检测结果坐标进行偏移
                double dx = roi.x - buffer_roi.x;
                double dy = roi.y - buffer_roi.y;
                detect_result = detect_result.translate(dx, dy);
                //保存检测结果
                QString dst_detect_path = QString("%1/%2_%3.det").arg(user_dir).arg(qCurrentTime).arg(index * m_config_data->m_fiber_end_count + i);
                detect_result.save_to_file(dst_detect_path.toStdString());
                //绘制结果并保存
                std::vector<cv::Mat> channels(3, buffer_shape_image);
                cv::Mat detect_image;
                cv::merge(channels, detect_image);  // 三个通道都指向同一个数据
                detect_image = detect_result.draw_to_image(detect_image);
            	dst_image_path = QString("%1/%2_%3_det.png").arg(user_dir).arg(qCurrentTime).arg(index * m_config_data->m_fiber_end_count + i);
                cv::imwrite(dst_image_path.toStdString(), detect_image);
                dst_image_path = QString("%1/%2_det.png").arg(image_dir).arg(index * m_config_data->m_fiber_end_count + i);
                cv::Mat enhance_detect_image = unsharp_masking(detect_image, 1.0, 7);
            	cv::imwrite(dst_image_path.toStdString(), enhance_detect_image);
            }
        }
    }
    result_obj["command"] = "server_anomaly_detection_finish";
    result_obj["param"] = "success";
    emit post_task_finished(QVariant::fromValue(result_obj));
    
    return true;
}

QJsonObject thread_misc::camera_parameter_to_json(interface_camera* camera)
{
    QJsonObject root;
	//唯一标识符
    root["unique_id"] = camera->m_unique_id;
	// 基本采集参数
	double fps = camera->get_frame_rate();
    root["frame_rate"] = fps;
    root["frame_rate_range"] = range_to_json(camera->get_frame_rate_range());
	int max_width = camera->get_maximum_width();
    root["maximum_width"] = max_width;
    root["maximum_height"] = camera->get_maximum_height();
    root["start_x"] = camera->get_start_x();
    root["start_x_range"] = range_to_json(camera->get_start_x_range());
    root["start_y"] = camera->get_start_y();
    root["start_y_range"] = range_to_json(camera->get_start_y_range());
    root["width"] = camera->get_width();
    root["width_range"] = range_to_json(camera->get_width_range());
    root["height"] = camera->get_height();
    root["height_range"] = range_to_json(camera->get_height_range());

    // 像素格式
    {
        QJsonArray fmtArray;
        for (auto it = camera->enum_pixel_format().begin(); it != camera->enum_pixel_format().end(); ++it)
        {
            QJsonObject fmt;
            fmt["name"] = it.key();
            fmt["value"] = static_cast<int>(it.value());
            fmtArray.append(fmt);
        }
        root["pixel_formats"] = fmtArray;
        root["current_pixel_format"] = camera->get_pixel_format();
    }

    // 自动曝光
    {
        QJsonArray modes;
        for (auto it = camera->enum_supported_auto_exposure_mode().begin();
            it != camera->enum_supported_auto_exposure_mode().end(); ++it)
        {
            QJsonObject mode;
            mode["name"] = it.key();
            mode["value"] = static_cast<int>(it.value());
            modes.append(mode);
        }
        root["auto_exposure_modes"] = modes;
        root["current_auto_exposure_mode"] = camera->get_auto_exposure_mode();
        root["is_auto_exposure_closed"] = camera->is_auto_exposure_closed();
        root["auto_exposure_time_floor"] = camera->get_auto_exposure_time_floor();
        root["auto_exposure_time_floor_range"] = range_to_json(camera->get_auto_exposure_time_floor_range());
        root["auto_exposure_time_upper"] = camera->get_auto_exposure_time_upper();
        root["auto_exposure_time_upper_range"] = range_to_json(camera->get_auto_exposure_time_upper_range());
        root["exposure_time"] = camera->get_exposure_time();
        root["exposure_time_range"] = range_to_json(camera->get_exposure_time_range());
    }

    // 自动增益
    {
        QJsonArray modes;
        for (auto it = camera->enum_supported_auto_gain_mode().begin();
            it != camera->enum_supported_auto_gain_mode().end(); ++it)
        {
            QJsonObject mode;
            mode["name"] = it.key();
            mode["value"] = static_cast<int>(it.value());
            modes.append(mode);
        }
        root["auto_gain_modes"] = modes;
        root["current_auto_gain_mode"] = camera->get_auto_gain_mode();
        root["is_auto_gain_closed"] = camera->is_auto_gain_closed();
        root["auto_gain_floor"] = camera->get_auto_gain_floor();
        root["auto_gain_floor_range"] = range_to_json(camera->get_auto_gain_floor_range());
        root["auto_gain_upper"] = camera->get_auto_gain_upper();
        root["auto_gain_upper_range"] = range_to_json(camera->get_auto_gain_upper_range());
        root["gain"] = camera->get_gain();
        root["gain_range"] = range_to_json(camera->get_gain_range());
    }

    // 触发模式
    {
        QJsonArray triggerModes;
        for (auto it = camera->enum_supported_trigger_mode().begin();
            it != camera->enum_supported_trigger_mode().end(); ++it)
        {
            QJsonObject tm;
            tm["name"] = it.key();
            tm["value"] = static_cast<int>(it.value());
            triggerModes.append(tm);
        }
        root["trigger_modes"] = triggerModes;
        root["current_trigger_mode"] = camera->get_trigger_mode();
    }

    // 触发源
    {
        QJsonArray triggerSources;
        for (auto it = camera->enum_supported_trigger_source().begin();
            it != camera->enum_supported_trigger_source().end(); ++it)
        {
            QJsonObject ts;
            ts["name"] = it.key();
            ts["value"] = static_cast<int>(it.value());
            triggerSources.append(ts);
        }
        root["trigger_sources"] = triggerSources;
        root["current_trigger_source"] = camera->get_trigger_source();
    }

	root["is_grab_running"] = camera->is_grab_running();    //是否正在采集

    return root;
}

QJsonObject thread_misc::range_to_json(const st_range& range)
{
    QJsonObject obj;
    obj["min"] = range.min;
    obj["max"] = range.max;
    return obj;
}
