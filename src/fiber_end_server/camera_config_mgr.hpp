/****************************************************************
 * 相机参数管理, 存储/加载所有相机参数，所有的相机以一个文件管理，不同的相机以 unique_id 区分
 * 每个相机参数对应一个 XML 节点，使用结构体存储
 * 使用方法:
 * (1) 启动后端时构建对象并加载该文件
 * (2) 打开相机时根据 unique_id 查找对应参数，如果没有找到则使用相机默认参数，否则使用查找结果设置相机参数
 * (3) 用户修改相机参数时，首先将相机参数保存到结构体，然后更新或添加到管理对象，最后保存到文件
 ****************************************************************/
#pragma once

#include <QFile>
#include <QMap>
#include <QString>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QTextStream>

#include "../device_camera/interface_camera.h"

struct st_camera_config_mgr
{
	bool load_from_file(const QString& file_path)
	{
		m_config_file_path = file_path;
		QFile file(file_path);
		if (!file.exists())
			return false;
		if (!file.open(QIODevice::ReadOnly))
			return false;
		QDomDocument doc;
		if (!doc.setContent(&file))
		{
			file.close();
			return false;
		}
		file.close();
		QDomElement root = doc.documentElement();
		if (root.tagName() != "CameraConfigs")
			return false;
		m_map_camera_config.clear();
		QDomNodeList list = root.elementsByTagName("Camera");
		for (int i = 0; i < list.count(); i++)
		{
			QDomElement elem = list.at(i).toElement();
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
		QDomDocument doc;
		QDomElement root = doc.createElement("CameraConfigs");
		doc.appendChild(root);
		for (auto it = m_map_camera_config.begin(); it != m_map_camera_config.end(); ++it)
		{
			QDomElement elem = save_camera_config_to_node(doc, it.value());
			root.appendChild(elem);
		}
		QFile file(m_config_file_path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
			return;
		QTextStream out(&file);
		doc.save(out, 4);
		file.close();
	}

	static QDomElement save_camera_config_to_node(QDomDocument& doc, const st_camera_config& camera_config)
	{
		QDomElement element = doc.createElement("Camera");
		element.setAttribute("unique_id", camera_config.unique_id);
		element.setAttribute("frame_rate", camera_config.frame_rate);
		element.setAttribute("start_x", camera_config.start_x);
		element.setAttribute("start_y", camera_config.start_y);
		element.setAttribute("width", camera_config.width);
		element.setAttribute("height", camera_config.height);
		element.setAttribute("pixel_format", camera_config.pixel_format);
		element.setAttribute("auto_exposure_mode", camera_config.auto_exposure_mode);
		element.setAttribute("auto_exposure_time_floor", camera_config.auto_exposure_time_floor);
		element.setAttribute("auto_exposure_time_upper", camera_config.auto_exposure_time_upper);
		element.setAttribute("exposure_time", camera_config.exposure_time);
		element.setAttribute("auto_gain_mode", camera_config.auto_gain_mode);
		element.setAttribute("auto_gain_floor", camera_config.auto_gain_floor);
		element.setAttribute("auto_gain_upper", camera_config.auto_gain_upper);
		element.setAttribute("gain", camera_config.gain);
		//固定值不保存到文件
		//element.setAttribute("trigger_mode", trigger_mode);
		//element.setAttribute("trigger_source", trigger_source);
		return element;
	}

	static st_camera_config load_camera_config_from_node(const QDomElement& elem)
	{
		st_camera_config camera_config;
		camera_config.unique_id = elem.attribute("unique_id", "");
		camera_config.frame_rate = elem.attribute("frame_rate", "30").toDouble();
		camera_config.start_x = elem.attribute("start_x", "0").toInt();
		camera_config.start_y = elem.attribute("start_y", "0").toInt();
		camera_config.width = elem.attribute("width", "1920").toInt();
		camera_config.height = elem.attribute("height", "1080").toInt();
		camera_config.pixel_format = elem.attribute("pixel_format", "");
		camera_config.auto_exposure_mode = elem.attribute("auto_exposure_mode", "");
		camera_config.auto_exposure_time_floor = elem.attribute("auto_exposure_time_floor", "1").toDouble();
		camera_config.auto_exposure_time_upper = elem.attribute("auto_exposure_time_upper", "10000000").toDouble();
		camera_config.exposure_time = elem.attribute("exposure_time", "20000").toDouble();
		camera_config.auto_gain_mode = elem.attribute("auto_gain_mode", "");
		camera_config.auto_gain_floor = elem.attribute("auto_gain_floor", "0").toDouble();
		camera_config.auto_gain_upper = elem.attribute("auto_gain_upper", "12").toDouble();
		camera_config.gain = elem.attribute("gain", "0").toDouble();
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