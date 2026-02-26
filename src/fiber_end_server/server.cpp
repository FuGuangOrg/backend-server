#include "server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QCoreApplication>

fiber_end_server::fiber_end_server(QString ip, quint16 port, QObject* parent)
	: QTcpServer(parent),m_server_ip(ip),m_server_port(port)
{
	m_thread_algorithm = new thread_algorithm(QString::fromStdString("算法处理子线程"), this);
    connect(m_thread_algorithm, &thread_base::post_task_finished, this, &fiber_end_server::on_algorithm_task_finished, Qt::QueuedConnection);
	m_thread_device_enum = new thread_device_enum(QString::fromStdString("设备枚举子线程"),this);
    m_thread_device_enum->set_device_manager(&m_device_manager);
    connect(m_thread_device_enum, &thread_base::post_task_finished, this, &fiber_end_server::on_device_enum_task_finished, Qt::QueuedConnection);
	m_thread_misc = new thread_misc(QString::fromStdString("其他任务子线程"), this);
	m_thread_misc->set_device_manager(&m_device_manager);
    connect(m_thread_misc, &thread_base::post_task_finished, this, &fiber_end_server::on_misc_task_finished, Qt::QueuedConnection);
    m_thread_motion_control = new thread_motion_control(QString::fromStdString("运动控制子线程"), this);
    connect(m_thread_motion_control, &thread_base::post_task_finished, this, &fiber_end_server::on_motion_control_task_finished, Qt::QueuedConnection);
}

void fiber_end_server::set_server_config(const QString& ip, quint16 port)
{
    m_server_ip = ip;
    m_server_port = port;
}

bool fiber_end_server::start()
{
    if (!listen(QHostAddress(m_server_ip), m_server_port))
    {
        qWarning() << QString::fromStdString("后端启动失败:") << errorString();
        return false;
    }
    //服务器启动时，首先 1.加载配置文件 2.启动运控模块 3.设置光源亮度
    QString current_directory = QCoreApplication::applicationDirPath();
    std::string config_file_path = (current_directory + "/config.xml").toStdString();
    load_config_file(config_file_path);         //加载配置文件，如果没有则使用默认值
    //子线程初始化，包括运控模块和算法检测模块
    if(!m_thread_misc->initialize(&m_config_data))
    {
        return false;
    }
    qDebug() << QString::fromStdString("后端已启动，监听端口:") << m_server_port;
	m_thread_algorithm->start();
    m_thread_motion_control->start();
    m_thread_device_enum->start();
    m_thread_misc->start();

    //启动线程之后
    return true;
}

bool fiber_end_server::load_config_file(const std::string& config_file_path)
{
    return m_config_data.load_from_file(config_file_path);
}

void fiber_end_server::stop()
{
    delete m_thread_algorithm;
    m_thread_algorithm = nullptr;
    delete m_thread_motion_control;
    m_thread_motion_control = nullptr;
    delete m_thread_device_enum;
    m_thread_device_enum = nullptr;
    delete m_thread_misc;
    m_thread_misc = nullptr;
    for (QTcpSocket* client : m_clients)
    {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
    m_map_request_id_to_socket.clear();
}

void fiber_end_server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* client = new QTcpSocket(this);
    client->setSocketDescriptor(socketDescriptor);
    connect(client, &QTcpSocket::readyRead, this, &fiber_end_server::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &fiber_end_server::onDisconnected);
    m_clients << client;
    qDebug() << QString::fromStdString("新前端已连接");
}

void fiber_end_server::onReadyRead()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    QByteArray data = client->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QString request_id = obj["request_id"].toString();
	m_map_request_id_to_socket[request_id] = client; // 保存请求 ID 和对应的客户端
    process_request(obj);
}

void fiber_end_server::onDisconnected()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    for (QMap<QString, QTcpSocket*>::iterator iter = m_map_request_id_to_socket.begin();
        iter != m_map_request_id_to_socket.end(); )
    {
	    if (iter.value() == client)
	    {
            iter = m_map_request_id_to_socket.erase(iter);
	    }
	    else
	    {
            ++iter;
	    }
    }
    m_clients.removeAll(client);
    client->deleteLater();
}

void fiber_end_server::process_request(const QJsonObject& obj)
{
    if(m_stop_server.load())
    {
	    return;
    }
    if(m_thread_misc->m_is_processing.load())
    {
        //如果任务正在处理，主线程直接响应并返回.
        QJsonObject result_obj;     //返回的消息对象
        result_obj["request_id"] = obj["request_id"];
        result_obj["command"] = "server_report_info";
        result_obj["param"] = QString("后端正在进行检测任务, 请待任务执行完毕之后再进行操作!");
        send_process_result(QVariant::fromValue(result_obj));
        return;
    }
    QString command = obj["command"].toString();
    qDebug() << L("收到消息:") << command;
    
    if (command == "client_request_server_parameter")
    {
        m_thread_device_enum->add_task(obj);
    }
    else if (command == "client_request_camera_list")
    {
        m_thread_device_enum->add_task(obj);
    }
    else if (command == "client_request_open_camera")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_close_camera")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_change_camera_parameter")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_start_grab")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_trigger_once")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_change_algorithm_parameter")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_user_config_set")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_move_camera")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_move_camera_by_index")
    {
        m_thread_misc->add_task(obj);
    }
    else if(command == "client_request_set_motion_parameter")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_auto_focus")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_anomaly_detection")
    {
        m_thread_misc->add_task(obj);
    }
    else if (command == "client_request_auto_calibration")
    {
        m_thread_misc->add_task(obj);
    }
    else if(command == "client_request_update_server_parameter")
    {
        m_thread_misc->add_task(obj);
    }
    else if(command == "client_request_start_process")
    {
        bool start = obj["param"].toBool();
        if(start)
        {
            m_thread_misc->add_task(obj);
        }
        else if(m_thread_misc->m_is_processing.load())
        {
            m_thread_misc->m_terminate.store(true);
        }
    }
    else if(command == "client_request_stop_server")
    {
        if(m_stop_server.load())
        {
	        return;
        }
        m_stop_server.store(true);
        stop();
        //停止线程并释放资源之后退出程序
        QCoreApplication::exit();
    }
}

void fiber_end_server::send_process_result(const QVariant& result_data, bool task_finished)
{
    QJsonObject obj = result_data.toJsonObject();
    QString request_id = obj["request_id"].toString();
    if(request_id == "")
    {
	    //通知所有客户端
        for (QTcpSocket* client : m_clients)
        {
            if (client != nullptr)
            {
                // 序列化 JSON
                QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);

                // 构造长度前缀
                qint32 size = payload.size();
                QByteArray block;
                QDataStream out(&block, QIODevice::WriteOnly);
                out.setByteOrder(QDataStream::BigEndian); // 网络字节序
                out << size;
                block.append(payload);
                //写入 socket
                client->write(block);
                client->flush();
            }
        }
    }
    else
    {
        auto iter = m_map_request_id_to_socket.find(request_id);
        if (iter != m_map_request_id_to_socket.end())
        {
            QTcpSocket* client = iter.value();
            if (task_finished)
            {
                m_map_request_id_to_socket.erase(iter);
            }
            if (client != nullptr)
            {
                // 序列化 JSON
                QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);

                // 构造长度前缀
                qint32 size = payload.size();
                QByteArray block;
                QDataStream out(&block, QIODevice::WriteOnly);
                out.setByteOrder(QDataStream::BigEndian); // 网络字节序
                out << size;
                block.append(payload);
                //写入 socket
                client->write(block);
                client->flush();
            }
        }
    }
}

int fiber_end_server::get_task_type(const QJsonObject& obj)
{
    if (obj.contains(QString("task_type")))
    {
        return obj["task_type"].toInt();
    }
    return TASK_TYPE_ANY; //默认返回任意任务类型
}

QJsonObject fiber_end_server::server_parameter_to_json(const st_config_data& config_data)
{
    return config_data.save_to_json();
}

void fiber_end_server::on_algorithm_task_finished(const QVariant& task_data)
{
	
}

void fiber_end_server::on_device_enum_task_finished(const QVariant& task_data)
{
	QJsonObject obj = task_data.toJsonObject();
    /*********************************
	 * 枚举线程在执行 client_request_server_parameter 任务的时候返回的结果为
	 * obj["command"] = "client_request_server_parameter";
	 * obj["device_list"] = devices_array;  //设备列表
	 * obj["request_id"]    //原始请求 ID，不需要改变
	 * 这里再追加一个["camera"]和["parameter"]字段，将打开的相机信息和服务器的端面检测参数传递给前端
     *********************************/
    if(obj["command"].toString() == QString("client_request_server_parameter"))
    {
        // 修改命令为 server_camera_parameter. 客户端接收到该消息后，需要:
		// （1）初始化相机列表
		// （2）更新相机视图图标
		// （3）打开控制面板，显示相机参数、算法参数以及端面检测相关设置参数
		obj["command"] = "server_all_parameters";
	    if(m_thread_misc->camera() != nullptr && m_thread_misc->camera()->m_is_opened)
        {
            QJsonObject camera_obj = thread_misc::camera_parameter_to_json(m_thread_misc->camera());
            obj["camera"] = camera_obj; // 将相机参数添加到返回结果中
        }
        else
        {
            obj["camera"] = QJsonObject(); // 如果没有打开相机，则返回空对象
        }
        obj["algorithm_parameter"] = m_thread_misc->algorithm_parameter()->save_to_json();
        obj["fiber_end_parameter"] = m_config_data.save_to_json();  // 将端面检测参数添加到返回结果中
		send_process_result(QVariant::fromValue(obj));                      // 将任务结果发送给客户端
    }
    else
    {
        send_process_result(task_data);                         // 将任务结果发送给客户端
    }
}

void fiber_end_server::on_misc_task_finished(const QVariant& task_data)
{
    QJsonObject obj = task_data.toJsonObject();
    if (obj["command"].toString() == QString("server_camera_opened_success"))
    {
        obj["algorithm_parameter"] = m_thread_misc->algorithm_parameter()->save_to_json();
        obj["fiber_end_parameter"] = m_config_data.save_to_json();  // 将端面检测参数添加到返回结果中
        send_process_result(QVariant::fromValue(obj));
    }
    else
    {
        if(obj.contains("task_finish"))
        {
            bool finish = obj["task_finish"].toBool();
            send_process_result(task_data, finish);
        }
        else
        {
            send_process_result(task_data);                 // 将任务结果发送给客户端
        }
        
    }
}

void fiber_end_server::on_motion_control_task_finished(const QVariant& task_data)
{
	
}