#pragma once
#include <QMutex>
#include <QMap>
#include <QString>
#include <vector>

#include "../device_enum/device_info.hpp" 

class device_manager
{
public:
    device_manager() = default;
    ~device_manager() { release_device_list(); }
public:
    void set_device_list(const std::vector<st_device_info*>& device_list)
	{
        QMutexLocker locker(&m_mutex);
        release_device_list();
        for (const auto& device : device_list)
        {
            m_device_list.insert(device->m_unique_id, device);
        }
    }

	void release_device_list()
    {
        for (auto device_info : m_device_list)
        {
            if (device_info != nullptr)
            {
                delete device_info;     //释放旧的设备信息
                device_info = nullptr;
            }
        }
        m_device_list.clear();
    }

    st_device_info* get_device_info(const QString& unique_id)
	{
        QMutexLocker locker(&m_mutex);
        if (m_device_list.contains(unique_id))
        {
            return m_device_list[unique_id];
        }
        return nullptr;
    }

    std::vector<st_device_info*> get_all_devices()
	{
        QMutexLocker locker(&m_mutex);
        std::vector<st_device_info*> result;
        for (auto& val : m_device_list)
        {
            result.push_back(val);
        }
        return result;
    }

    TYPE_SDK sdk_type() { return m_sdk_type; }
private:
	TYPE_SDK m_sdk_type{ SDK_DVP2 }; //使用哪个 SDK 操作相机.设备操作线程和枚举线程需要使用相同的 SDK
    QMap<QString, st_device_info*> m_device_list;
    QMutex m_mutex;
};
