/***************************************************
 * 公共接口类，包含一些常用的功能，例如共享内存、数据转换等
 ***************************************************/

#pragma once

#include "common_global.h"

#include <string>
#include <fstream>
const int OUTPUT_LOG = 1;

enum ENCODE_TYPE  //字符串编码类型
{
    ANSI,
    UTF8,
    UTF16_LE,
    UTF16_BE,
    UTF8_BOM
};

struct COMMON_EXPORT st_position
{
    int m_x{ 0 }, m_y{ 0 }, m_z{ 0 };
	st_position(int x = 0, int y = 0,int z = 0) :m_x(x), m_y(y),m_z(z) {}

    //支持按照 X 升序排序
    bool operator<(const st_position& other) const
    {
        return m_x < other.m_x;
    }
};


class COMMON_EXPORT common
{
public:
    common();
};

//String-->QString
QString COMMON_EXPORT L(const char* szText);

//QString-->String
std::string COMMON_EXPORT l(const QString& qstr);

void COMMON_EXPORT write_log(const char* szInfo);

bool COMMON_EXPORT make_path(const QString& dir);

//目录清理,对于指定目录dir，根据子目录最后修改日期保留最近的 max_count 个子目录，只处理一级子目录
bool COMMON_EXPORT clean_path(const QString& dir, int max_count);

bool COMMON_EXPORT remove_files(const QString& dir, const QString& suffix);

//UTF8转ANSI
std::string COMMON_EXPORT utf8_to_ansi(const std::string& str);

// 判断是否为 UTF-8 编码
bool COMMON_EXPORT is_utf8(const char* str, int len);

ENCODE_TYPE COMMON_EXPORT check_utf8_encode(const char* data, size_t size);
