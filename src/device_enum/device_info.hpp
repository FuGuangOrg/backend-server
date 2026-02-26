#pragma once

#include "device_enum_global.h"

#include <QString>
#include <string>
#include <vector>
//using namespace std;

enum TYPE_SDK
{
	SDK_MVS = 0,				//海康威视的SDK
	SDK_DVP2 = 1				//度申科技的SDK
};

enum TYPE_DEVICE
{
	TYPE_DEVICE_UNKNOW = -1,
	TYPE_DEVICE_GIGE = 0,
	TYPE_DEVICE_USB = 1,
	TYPE_DEVICE_GENTL_CAMERALINK = 2,
	TYPE_DEVICE_GENTL_CXP = 3,
	TYPE_DEVICE_GENTL_XOF = 4
};

//不同的相机有不同的参数项，使用 key-value 形式存储
struct DEVICE_ENUM_EXPORT st_cam_item
{
	QString m_key{ "" };
	QString m_value{ "" };

	st_cam_item(const QString& key = "", const QString& value = "")
		:m_key(key), m_value(value)
	{
	}
};



struct DEVICE_ENUM_EXPORT st_device_info
{
	struct st_device_info_ptr_less		//如果是指针数组，需要使用排序结构体进行排序
	{
		bool operator()(const st_device_info* a, const st_device_info* b) const
		{
			return a->m_unique_id < b->m_unique_id;
		}
	};
	virtual ~st_device_info() = default;
	virtual int get_device_type() const  = 0;	//相机类型

	TYPE_SDK m_type_sdk{ SDK_DVP2 };				//SDK 类型，创建相机时需要根据该参数决定使用哪个SDK
	std::vector<st_cam_item> m_cam_items;			//相机参数项
	QString m_unique_id{ "" };				//唯一标识符,由序列号_型号组成

	//设置属性-值，如果没有则添加
	void set_item(const QString& key, const QString& value)
	{
		bool is_exist = false;
		for (size_t i = 0; i < m_cam_items.size(); i++)
		{
			if (m_cam_items[i].m_key == key)
			{
				m_cam_items[i].m_value = value;
				is_exist = true;
				break;
			}
		}
		if (!is_exist)
		{
			m_cam_items.emplace_back(st_cam_item(key, value));
		}
	}

	void set_item(const std::string& key, const std::string& value)
	{
		return set_item(QString::fromStdString(key), QString::fromStdString(value));
	}

	QString get_item(const QString& key) const
	{
		for (size_t i = 0; i < m_cam_items.size(); i++)
		{
			if (m_cam_items[i].m_key == key)
			{
				return m_cam_items[i].m_value;
			}
		}
		return QString::fromStdString("error");
	}

	QString get_item(const std::string& key) const
	{
		return get_item(QString::fromStdString(key));
	}

	bool operator <(const st_device_info& other) const
	{
		return m_unique_id < other.m_unique_id;
	}
};

