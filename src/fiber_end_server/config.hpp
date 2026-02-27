/***********************************************
 * 加载端面检测配置文件，以设置相关参数.
 * 目前包括 Y轴位置列表，每张影像端面数量， 影像保存路径
 ***********************************************/

#pragma once
#include <vector>
#include <string>
#include <pugixml.hpp>
#include <QJsonObject>
#include <QJsonArray>

#include "../common/common.h"

 // 端面检测配置参数
struct st_config_data
{
	int m_light_brightness{ 80 };						//光源亮度
	int m_move_speed{ 1000 };							//移动速度
	int m_max_x{ 10000 }, m_max_y{ 10000 };				//运动范围
	int m_position_x{ 0 }, m_position_y{ 0 };			//相机当前位置，这里不会保存到文件，考虑到重启时恢复位置可能会导致设备损坏，这里由设备自行设置
														//初始化加载文件时该值默认为0, 在启动运动控制模块之后获取设备位置并更新该值
	int m_move_step_x{ 1000 }, m_move_step_y{ 1000 };	//上下调整时的移动步长
	std::vector<st_position> m_photo_location_list;		//拍照位置列表，复位之后的位置。运行状态下，会依次在此位置自动对焦-检测
														//自动对焦时需要一个较好的初始位置，以提高自动对焦的速度和效果
	int m_fiber_end_count{ 4 };							//每张影像上的端面数量. 自动对焦以及后续检测需要使用的参数
	int m_fiber_end_pixel_size{ 1 };					//影像上每个端面的像素直径，用于创建形状匹配模型
	double m_fiber_end_physical_size{ 100.0 };			//端面物理直径尺寸，单位微米. 该参数用于计算像素尺寸以及在前端缩略图中显示端面的像素尺寸
	double m_field_of_view{ 1.0 };						//每张检测结果的视野大小，默认与精定位结果一致，可以调整外扩系数
	int m_auto_detect{ 1 };								//对焦完成之后是否自动执行检测 0 -- 否    1 -- 是
	std::string m_save_path{ "./saveimages" };		//指定保存拍照图像的路径

	std::string m_config_file_path{ "./config.xml" };		//配置文件路径,服务刚启动之后会加载配置文件，只在调用 load_from_file 时初始化一次

	//服务刚启动的时候加载配置文件，保存文件路径以及数据
	bool load_from_file(const std::string& config_file_path)
	{
		m_config_file_path = config_file_path;
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(config_file_path.c_str());
		if (!result)
		{
			write_log(("Failed to load config file: " + config_file_path).c_str());
			return false;
		}
		pugi::xml_node root = doc.document_element();
		load_from_node(root);
		return true;
	}

	bool load_from_node(const pugi::xml_node& node)
	{
		if (auto n = node.child("light_brightness"))
			m_light_brightness = n.text().as_int(m_light_brightness);
		if (auto n = node.child("move_speed"))
			m_move_speed = n.text().as_int(m_move_speed);
		if (auto n = node.child("max_x"))
			m_max_x = n.text().as_int(m_max_x);
		if (auto n = node.child("max_y"))
			m_max_y = n.text().as_int(m_max_y);
		if (auto n = node.child("move_step_x"))
			m_move_step_x = n.text().as_int(m_move_step_x);
		if (auto n = node.child("move_step_y"))
			m_move_step_y = n.text().as_int(m_move_step_y);

		// 拍照位置列表
		m_photo_location_list.clear();
		for (pugi::xml_node pos : node.children("position"))
		{
			int x = pos.attribute("x").as_int(0);
			int y = pos.attribute("y").as_int(0);
			m_photo_location_list.emplace_back(x, y);
		}

		if (auto n = node.child("fiber_end_count"))
			m_fiber_end_count = n.text().as_int(m_fiber_end_count);
		if (auto n = node.child("fiber_end_pixel_size"))
			m_fiber_end_pixel_size = n.text().as_int(m_fiber_end_pixel_size);
		if (auto n = node.child("fiber_end_physical_size"))
			m_fiber_end_physical_size = n.text().as_double(m_fiber_end_physical_size);
		if (auto n = node.child("field_of_view"))
			m_field_of_view = n.text().as_double(m_field_of_view);
		if (auto n = node.child("auto_detect"))
			m_auto_detect = n.text().as_int(m_auto_detect);
		if (auto n = node.child("save_path"))
			m_save_path = n.text().as_string(m_save_path.c_str());
		return true;
	}

	//界面上修改相关配置之后更新数据，然后保存到文件
	void save() const
	{
		pugi::xml_document doc;
		pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";
		pugi::xml_node root = doc.append_child("config");
		save_to_node(root);
		if (!doc.save_file(m_config_file_path.c_str(), "    ", pugi::format_default, pugi::encoding_utf8))
		{
			write_log("Failed to save config file");
		}
	}

	// 将参数写入已存在的 node
	void save_to_node(pugi::xml_node& node) const
	{
		auto append_int = [&](const char* tag, int value) {
			node.append_child(tag).text().set(value);
		};
		auto append_double = [&](const char* tag, double value) {
			node.append_child(tag).text().set(value);
		};
		auto append_str = [&](const char* tag, const char* value) {
			node.append_child(tag).text().set(value);
		};

		append_int("light_brightness", m_light_brightness);
		append_int("move_speed", m_move_speed);
		append_int("max_x", m_max_x);
		append_int("max_y", m_max_y);
		append_int("move_step_x", m_move_step_x);
		append_int("move_step_y", m_move_step_y);

		for (const auto& pos : m_photo_location_list)
		{
			pugi::xml_node pos_node = node.append_child("position");
			pos_node.append_attribute("x") = pos.m_x;
			pos_node.append_attribute("y") = pos.m_y;
		}

		append_int("fiber_end_count", m_fiber_end_count);
		append_int("fiber_end_pixel_size", m_fiber_end_pixel_size);
		append_double("fiber_end_physical_size", m_fiber_end_physical_size);
		append_double("field_of_view", m_field_of_view);
		append_int("auto_detect", m_auto_detect);
		append_str("save_path", m_save_path.c_str());
	}

	// 创建命名子节点并写入，返回该节点（供 thread_misc 组合用户配置文件时使用）
	pugi::xml_node save_to_node(pugi::xml_node& parent, const std::string& node_name) const
	{
		pugi::xml_node node = parent.append_child(node_name.c_str());
		save_to_node(node);
		return node;
	}

	QJsonObject save_to_json() const
	{
		QJsonObject root;
		root["light_brightness"] = m_light_brightness;
		root["move_speed"] = m_move_speed;
		root["max_x"] = m_max_x;
		root["max_y"] = m_max_y;
		root["position_x"] = m_position_x;
		root["position_y"] = m_position_y;
		root["move_step_x"] = m_move_step_x;
		root["move_step_y"] = m_move_step_y;
		QJsonArray positions_array;
		for (const auto& pos : m_photo_location_list)
		{
			QJsonObject obj;
			obj["x"] = pos.m_x;
			obj["y"] = pos.m_y;
			positions_array.append(obj);
		}
		root["photo_location_list"] = positions_array;
		root["fiber_end_count"] = m_fiber_end_count;
		root["fiber_end_pixel_size"] = m_fiber_end_pixel_size;
		root["fiber_end_physical_size"] = m_fiber_end_physical_size;
		root["field_of_view"] = m_field_of_view;
		root["auto_detect"] = m_auto_detect;
		root["save_path"] = QString::fromStdString(m_save_path);

		return root;
	}

	bool position_list_changed(const std::vector<st_position>& positions) const
	{
		if(m_photo_location_list.size() != positions.size())
		{
			return true;
		}
		for (size_t i = 0; i < positions.size(); i++)
		{
			if (m_photo_location_list[i].m_x != positions[i].m_x ||
				m_photo_location_list[i].m_y != positions[i].m_y)
			{
				return true;
			}
		}
		return false;
	}
};
