/********************
 * 设备枚举线程
 ********************/
#pragma once
#include "work_threads.h"
#include "device_manager.hpp"
#include "../device_enum/device_enum_factory.h"


class thread_device_enum : public thread_base
{
    Q_OBJECT
public:
    thread_device_enum(QString name, QObject* parent = nullptr);
	virtual ~thread_device_enum() override;
	void set_device_manager(device_manager* manager) { m_device_manager = manager; }

	//将枚举的设备列表转换为 JSON 对象，发送给前端
    static QJsonObject device_list_to_json(const std::vector<st_device_info*>& device_list);
protected:
    void process_task(const QVariant& task_data) override;

private:
	device_manager* m_device_manager{ nullptr }; //设备管理器，用于存储和管理设备信息
};