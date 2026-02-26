#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include "thread_algorithm.h"
#include "thread_motion_control.h"
#include "thread_device_enum.h"
#include "thread_misc.h"
#include "config.hpp"

class fiber_end_server : public QTcpServer
{
    Q_OBJECT
public:
    explicit fiber_end_server(QString ip = "127.0.0.1", quint16 port = 5555, QObject* parent = nullptr);
    void set_server_config(const QString& ip, quint16 port);
	bool start();
    void stop();

    bool load_config_file(const std::string& config_file_path);         //加载配置文件，如果没有则使用默认值
	
	void process_request(const QJsonObject& obj);                       //处理外部请求
    void send_process_result(const QVariant& result_data,bool task_finished = true);              //向客户端发送消息

    static int get_task_type(const QJsonObject& obj);                   //后端执行完毕之后根据任务类型设置状态

    static QJsonObject server_parameter_to_json(const st_config_data& config_data);
protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onDisconnected();

    void on_algorithm_task_finished(const QVariant& task_data);   //子线程任务执行完毕之后向主线程发送消息，然后主线程通过TCP/IP转发
    void on_device_enum_task_finished(const QVariant& task_data);
    void on_misc_task_finished(const QVariant& task_data);
    void on_motion_control_task_finished(const QVariant& task_data);

private:
	QString m_server_ip{ "127.0.0.1" };                         //服务器 IP 地址
	quint16 m_server_port{ 5555 };                              //服务器端口号
    std::atomic<bool> m_stop_server{ false };                   //前端发送的终止服务请求，该值置为True，然后退出所有子线程
    QList<QTcpSocket*> m_clients;                               //连接的客户端
	QMap<QString, QTcpSocket*> m_map_request_id_to_socket;      //请求 id 和对应的客户端socket映射，在连接多个客户端时确保不会回复错误
	device_manager m_device_manager;                            //设备管理器，用于存储和管理设备信息
    st_config_data m_config_data;                               //端面检测参数
	thread_algorithm* m_thread_algorithm{ nullptr };            //四个子线程，分别处理四类任务
    thread_motion_control* m_thread_motion_control{ nullptr };  
    thread_device_enum* m_thread_device_enum{ nullptr };
    thread_misc* m_thread_misc{ nullptr };
    //interface_camera* m_camera{ nullptr };			            //相机对象
    /***************************线程执行状态变量，防止命令冲突*************************/
    std::atomic<bool> m_is_triggering{ false };       //后端线程是否正在采图

};
