#include "thread_device_enum.h"

thread_device_enum::thread_device_enum(QString name, QObject* parent)
    :thread_base(name , parent)
{

}

thread_device_enum::~thread_device_enum()
{
    // 释放资源
    device_enum_factory::release_device_enums();
}

void thread_device_enum::process_task(const QVariant& task_data)
{
    QJsonObject obj = task_data.toJsonObject();
    QString command = obj["command"].toString();
    if (command == "client_request_camera_list")
    {
        interface_device_enum* device_enum = device_enum_factory::create_device_enum(m_device_manager->sdk_type());
        std::vector<st_device_info*> device_info_list = device_enum->enumerate_devices();   //枚举的设备信息由m_device_manager管理
		m_device_manager->set_device_list(device_info_list);                                // 更新设备管理器中的设备列表
        // 转换成 QJsonObject 对象，然后发送给前端
		QJsonObject result_obj = device_list_to_json(device_info_list);
		result_obj["command"] = "server_camera_list";
        result_obj["request_id"] = obj["request_id"];
		emit post_task_finished(QVariant::fromValue(result_obj));
    }
    else if(command == "client_request_server_parameter")
    {
        interface_device_enum* device_enum = device_enum_factory::create_device_enum(m_device_manager->sdk_type());
        std::vector<st_device_info*> device_info_list = device_enum->enumerate_devices();   //枚举的设备信息由m_device_manager管理
        m_device_manager->set_device_list(device_info_list);                                // 更新设备管理器中的设备列表
        // 转换成 QJsonObject 对象，然后发送给前端
        QJsonObject result_obj = device_list_to_json(device_info_list);
        result_obj["command"] = command;                    //枚举之后需要继续检查相机状态，这里不修改命令
        result_obj["request_id"] = obj["request_id"];
        emit post_task_finished(QVariant::fromValue(result_obj));
    }
}

QJsonObject thread_device_enum::device_list_to_json(const std::vector<st_device_info*>& device_list)
{
    QJsonObject root;
    QJsonArray devices_array;

    for (const auto& device : device_list) 
    {
        QJsonObject device_obj;
        device_obj["unique_id"] = device->m_unique_id;

        QJsonArray cam_items_array;
        for (const auto& cam_item : device->m_cam_items) {
            QJsonObject cam_obj;
            cam_obj["key"] = cam_item.m_key;
            cam_obj["value"] = cam_item.m_value;
            cam_items_array.append(cam_obj);
        }

        device_obj["cam_items"] = cam_items_array;
        devices_array.append(device_obj);
    }
    root["device_list"] = devices_array;
    return root;
}