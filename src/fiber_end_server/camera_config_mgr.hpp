/****************************************************************
 * 相机参数管理, 存储/加载所有相机参数，所有的相机以一个文件管理，不同的相机以 unique_id 区分
 * 每个相机参数对应一个 XML 节点，使用结构体存储
 * 使用方法:
 * (1) 启动后端时构建对象并加载该文件
 * (2) 打开相机时根据 unique_id 查找对应参数，如果没有找到则使用相机默认参数，否则使用查找结果设置相机参数
 * (3) 用户修改相机参数时，首先将相机参数保存到结构体，然后更新或添加到管理对象，最后保存到文件
 ****************************************************************/
#pragma once

#include <QMap>
#include <QString>
#include <pugixml.hpp>

#include "../device_camera/interface_camera.h"

struct st_camera_config_mgr
{
	bool load_from_file(const QString& file_path)
	{
		m_config_file_path = file_path;
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(file_path.toStdString().c_str());
		if (!result)
			return false;
		pugi::xml_node root = doc.document_element();
		if (std::string(root.name()) != "CameraConfigs")
			return false;
		m_map_camera_config.clear();
		for (pugi::xml_node elem : root.children("Camera"))
		{
			st_camera_config camera_config = load_camera_config_from_node(elem);
			if (!camera_config.unique_id.isEmpty())
			{
				m_map_camera_config.insert(camera_config.unique_id, camera_config);
			}
		}
		return true;
	}

	void save_to_file()
	{
		pugi::xml_document doc;
		pugi::xml_node root = doc.append_child("CameraConfigs");
		for (auto it = m_map_camera_config.begin(); it != m_map_camera_config.end(); ++it)
		{
			save_camera_config_to_node(root, it.value());
		}
		doc.save_file(m_config_file_path.toStdString().c_str(), "    ");
	}

	// 将相机配置写入 parent，返回新建的 Camera 子节点
	static pugi::xml_node save_camera_config_to_node(pugi::xml_node& parent, const st_camera_config& camera_config)
	{
		pugi::xml_node element = parent.append_child("Camera");
		element.append_attribute("unique_id")                  = camera_config.unique_id.toStdString().c_str();
		element.append_attribute("frame_rate")                 = camera_config.frame_rate;
		element.append_attribute("start_x")                    = camera_config.start_x;
		element.append_attribute("start_y")                    = camera_config.start_y;
		element.append_attribute("width")                      = camera_config.width;
		element.append_attribute("height")                     = camera_config.height;
		element.append_attribute("pixel_format")               = camera_config.pixel_format.toStdString().c_str();
		element.append_attribute("auto_exposure_mode")         = camera_config.auto_exposure_mode.toStdString().c_str();
		element.append_attribute("auto_exposure_time_floor")   = camera_config.auto_exposure_time_floor;
		element.append_attribute("auto_exposure_time_upper")   = camera_config.auto_exposure_time_upper;
		element.append_attribute("exposure_time")              = camera_config.exposure_time;
		element.append_attribute("auto_gain_mode")             = camera_config.auto_gain_mode.toStdString().c_str();
		element.append_attribute("auto_gain_floor")            = camera_config.auto_gain_floor;
		element.append_attribute("auto_gain_upper")            = camera_config.auto_gain_upper;
		element.append_attribute("gain")                       = camera_config.gain;
		return element;
	}

	static st_camera_config load_camera_config_from_node(const pugi::xml_node& elem)
	{
		st_camera_config camera_config;
		camera_config.unique_id               = QString::fromStdString(elem.attribute("unique_id").as_string(""));
		camera_config.frame_rate              = elem.attribute("frame_rate").as_double(30.0);
		camera_config.start_x                 = elem.attribute("start_x").as_int(0);
		camera_config.start_y                 = elem.attribute("start_y").as_int(0);
		camera_config.width                   = elem.attribute("width").as_int(1920);
		camera_config.height                  = elem.attribute("height").as_int(1080);
		camera_config.pixel_format            = QString::fromStdString(elem.attribute("pixel_format").as_string(""));
		camera_config.auto_exposure_mode      = QString::fromStdString(elem.attribute("auto_exposure_mode").as_string(""));
		camera_config.auto_exposure_time_floor = elem.attribute("auto_exposure_time_floor").as_double(1.0);
		camera_config.auto_exposure_time_upper = elem.attribute("auto_exposure_time_upper").as_double(10000000.0);
		camera_config.exposure_time           = elem.attribute("exposure_time").as_double(20000.0);
		camera_config.auto_gain_mode          = QString::fromStdString(elem.attribute("auto_gain_mode").as_string(""));
		camera_config.auto_gain_floor         = elem.attribute("auto_gain_floor").as_double(0.0);
		camera_config.auto_gain_upper         = elem.attribute("auto_gain_upper").as_double(12.0);
		camera_config.gain                    = elem.attribute("gain").as_double(0.0);
		return camera_config;
	}

	void update_camera_config(const st_camera_config& config)
	{
		if (config.unique_id.isEmpty())
			return;
		QMap<QString, st_camera_config>::iterator iter = m_map_camera_config.find(config.unique_id);
		if (iter != m_map_camera_config.end())
		{
			iter.value() = config;
		}
		else
		{
			m_map_camera_config.insert(config.unique_id, config);
		}
	}

	st_camera_config get_camera_config(const QString& unique_id) const
	{
		if (unique_id.isEmpty())
		{
			return st_camera_config();
		}
		QMap<QString, st_camera_config>::const_iterator iter = m_map_camera_config.find(unique_id);
		if (iter != m_map_camera_config.end())
		{
			return iter.value();
		}
		return st_camera_config();
	}

	QMap<QString, st_camera_config> m_map_camera_config;	//相机参数映射，key 为相机 unique_id
	QString m_config_file_path{ "" };
};
