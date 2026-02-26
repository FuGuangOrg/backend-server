#include "common.h"

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

common::common()
{
}

QString L(const char* szText)
{
    return QString::fromStdString(std::string(szText));
}

std::string l(const QString& qstr)
{
    return qstr.toStdString();
}

void write_log(const char* szInfo)
{
    if (!OUTPUT_LOG)
    {
        return;
    }
    QString current_directory = QCoreApplication::applicationDirPath();
    QString log_directory = current_directory + "./log";
    QDir dir(log_directory);
    if (!dir.exists())
    {
        dir.mkpath(log_directory);
    }
    QDateTime datetime = QDateTime::currentDateTime();
    QString qDate = datetime.toString("yyyy-MM-dd");
    std::string sPath = l(log_directory) + "/" + l(qDate) + ".txt";
    std::ofstream ofile(sPath.c_str(), std::ios::app);
    QString qCurrentTime = datetime.toString("yyyy-MM-dd HH:mm:ss");
    std::string sHeader = "[" + l(qCurrentTime) + "]";
    ofile << sHeader << szInfo << std::endl;
}

bool make_path(const QString& dir)
{
    QDir directory(dir);
    if (!directory.exists())
    {
        return directory.mkpath(".");
    }
    return true;
}

bool clean_path(const QString& dir, int max_count)
{
    QDir directory(dir);
	if (!directory.exists()) 
    {
        return false;
    }
	QFileInfoList sub_dirs = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    // 如果子目录数量不超过 max_count，直接返回
    if (sub_dirs.size() <= max_count) 
    {
        return true;
    }
    // 排序子目录，按最后修改时间升序（最早修改的排在前面）
    std::sort(sub_dirs.begin(), sub_dirs.end(), 
        [](const QFileInfo& a, const QFileInfo& b) {return a.lastModified() < b.lastModified();});

    int remove_count = static_cast<int>(sub_dirs.size()) - max_count;
    for (int i = 0; i < remove_count; i++)
    {
        QString sub_dir_path = sub_dirs[i].absoluteFilePath();
        if (!QDir(sub_dir_path).removeRecursively())
        {
            write_log(l(QString("failed to delete folder: %1").arg(sub_dir_path)).c_str());
        }
    }
    return true;
}

bool remove_files(const QString& dir, const QString& suffix)
{
    QDir directory(dir);
    if (!directory.exists())
        return false;
    QString filter = suffix;
    QFileInfoList files = directory.entryInfoList({ filter }, QDir::Files);
    bool ok = true;
    for (const QFileInfo& info : files) 
    {
        if (!QFile::remove(info.absoluteFilePath())) 
        {
            ok = false;
        }
    }
    return ok;
}

std::string utf8_to_ansi(const std::string& str)
{
    if (!is_utf8(str.c_str(), str.length()))
    {
        return str;
    }
    QString qData(str.c_str());
    return std::string(qData.toLocal8Bit());
    
}

bool is_utf8(const char* str, int len)
{
    //UTF-8编码  1-3字节
    //1字节与ASCII相同
    //2字节编码范围为00000080-000007FF(128-->7*256+15*16+16=2047)
    //三字节的编码范围为00000800-0000FFFF
    if (check_utf8_encode(str, len) == UTF8)	//ANSI有可能被误检为UTF-8,中文UTF-8都是3-4个字节，因此第一个字符的编码一定不小于11100000
    {
        int p = 0;
        uint8_t ch;
        while (true)
        {
            ch = str[p];
            if (ch < 128)
            {
                p += 1;
                if (p >= len)
                {
                    break;
                }
            }
            else if (ch < 224)	//中文小于224，一定不是UTF8
            {
                p += 2;
                if (p >= len)
                {
                    break;
                }
            }
            else   //中文超过224，可能是UTF8占三位或四位，也有可能是ANSI占两位，此时再检查后面第三位和第四位,UTF8的情况前面已经判断过了，这里不再判定
            {
                return true;
            }
        }
    }
    return false;
}

ENCODE_TYPE check_utf8_encode(const char* data, size_t size)
{
    bool bAnsi = true;
    uint8_t ch = 0x00;
    int32_t nBytes = 0;
    for (size_t i = 0; i < size; i++)
    {
        ch = *(data + i);
        if ((ch & 0x80) != 0x00)
        {
            bAnsi = false;
        }
        if (nBytes == 0)		//
        {
            if (ch >= 0x80)
            {
                if (ch >= 0xFC && ch <= 0xFD)
                {
                    nBytes = 6;
                }
                else if (ch >= 0xF8)
                {
                    nBytes = 5;
                }
                else if (ch >= 0xF0)
                {
                    nBytes = 4;
                }
                else if (ch >= 0xE0)
                {
                    nBytes = 3;
                }
                else if (ch >= 0xC0)
                {
                    nBytes = 2;
                }
                else
                {
                    return ENCODE_TYPE::ANSI;
                }
                nBytes--;
            }
        }
        else
        {
            if ((ch & 0xC0) != 0x80)
            {
                return ENCODE_TYPE::ANSI;
            }
            nBytes--;
        }
    }
    if (nBytes > 0 || bAnsi)
    {
        return ENCODE_TYPE::ANSI;
    }
    return ENCODE_TYPE::UTF8;
}




