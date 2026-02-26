#include "image_shared_memory.h"
#include <QBuffer>
#include <QDebug>

#include "../common/common.h"

image_shared_memory::image_shared_memory(const QString & key)
    : m_key_prefix(key)
{

}

void image_shared_memory::detach()
{
    
}

bool image_shared_memory::write_image(const QImage& img, st_image_meta& meta)
{
    int width = img.width();
    int height = img.height();
    int channels(0);
    if (img.format() == QImage::Format_Grayscale8)
    {
        channels = 1;
    }
    else if(img.format() == QImage::Format_RGB888)
    {
        channels = 3;
    }
    if(channels != 1 && channels != 3)
    {
        std::string info = std::string("image format error: only support 1 channel or 3 channel!");
        write_log(info.c_str());
        return false;
    }
    quint64 data_size = static_cast<quint64>(width * height * channels);
    int write_index = m_index;
    m_index = (m_index + 1) % buffer_size;      // 下一次写入的缓冲区索引
    //生成 key,根据 key 得到可供写入的共享内存
    QString key = QString("%1_buffer%2_%3x%4x%5").arg(m_key_prefix).arg(write_index).arg(width).arg(height).arg(channels);
    QSharedMemory* shared_memory{ nullptr };
    QMap<QString, QSharedMemory*>::iterator iter = m_shared_memories.find(key);
    if(iter == m_shared_memories.end())
    {
	    //创建共享内存
        shared_memory = new QSharedMemory(key);
        if(!shared_memory->create(static_cast<qsizetype>(data_size)))
        {
            std::string info = std::string("Shared memory create failed: ") + l(shared_memory->errorString());
            write_log(info.c_str());
            delete shared_memory;
            shared_memory = nullptr;
        }
        else
        {
            m_shared_memories.insert(key, shared_memory);
        }
    }
    else
    {
        shared_memory = iter.value();
    }
    if(shared_memory == nullptr)
    {
	    return false;
    }
    // 上一次写入的缓冲区索引是 m_index，这里切换缓冲区写入
    if (!shared_memory->lock())
    {
        std::string info = std::string("Shared memory lock failed: ") + l(shared_memory->errorString());
        write_log(info.c_str());
        //qDebug() << "Shared memory lock failed:" << m_buffers[write_index].errorString();
        return false;
    }
    memcpy(shared_memory->data(), img.constBits(), static_cast<size_t>(data_size));
    shared_memory->unlock();
    // 更新元数据
    m_frame_counter++;
    meta.width = width;
    meta.height = height;
    meta.channels = channels;
    meta.index = write_index;       //当前写入的缓冲区索引，读的时候使用该索引对应的缓冲区
    meta.shared_memory_key = shared_memory->key();
    meta.frame_id = m_frame_counter;

    return true;
}

QImage image_shared_memory::read_image(const st_image_meta& meta)
{
    QString key = meta.shared_memory_key; // 读取的缓冲区标识
    QSharedMemory* shared_memory{ nullptr };    //绑定到 key 的共享内存
    QMap<QString, QSharedMemory*>::iterator iter = m_shared_memories.find(key);
    if(iter == m_shared_memories.end())
    {
        shared_memory = new QSharedMemory(key);
        //如果绑定失败，释放内存并移除
        if (!shared_memory->attach(QSharedMemory::ReadOnly))
        {
            std::string info = std::string("Shared memory attach failed:") + l(shared_memory->errorString());
            write_log(info.c_str());
            delete shared_memory;
            shared_memory = nullptr;
            return QImage();
        }
        else
        {
            m_shared_memories.insert(key, shared_memory);
        }
    }
    else
    {
        shared_memory = iter.value();
    }
    int width = meta.width;
    int height = meta.height;
    int channels = meta.channels;
    // 只支持单通道和三通道
    if (channels != 1 && channels != 3)
    {
        std::string info = std::string("image format error: only support 1 channel or 3 channel!");
        write_log(info.c_str());
        return QImage();
    }
    // 如果宽高数据异常，直接返回空图像
    if (width == 0 || height == 0)
    {
        write_log("Shared memory info error...");
        return QImage();
    }
    if (!shared_memory->lock())
    {
        std::string info = std::string("Shared memory lock failed:") + l(shared_memory->errorString());
        write_log(info.c_str());
        return QImage();
    }
    QImage::Format format = QImage::Format_Grayscale8;
    if(channels == 3)
    {
        format = QImage::Format_RGB888;
    }
    QImage img(width, height, format);
    memcpy(img.bits(), shared_memory->constData(), static_cast<size_t>(width * height * channels));
    shared_memory->unlock();
    return img;
}

QJsonObject image_shared_memory::meta_to_json(const st_image_meta& meta)
{
    QJsonObject json;
    json["shm_key"] = meta.shared_memory_key;
    json["width"] = meta.width;
    json["height"] = meta.height;
    json["channels"] = meta.channels;
    json["index"] = meta.index;
    json["is_new_memory"] = meta.is_new_memory;
    json["frame_id"] = static_cast<long long>(meta.frame_id);
    return json;
}

st_image_meta image_shared_memory::json_to_meta(const QJsonObject& json)
{
    st_image_meta meta;
    meta.shared_memory_key = json["shm_key"].toString();
    meta.width = json["width"].toInt();
    meta.height = json["height"].toInt();
    meta.channels = json["channels"].toInt();
    meta.index = json["index"].toInt();
    meta.is_new_memory = static_cast<quint64>(json["is_new_memory"].toBool());
    meta.frame_id = static_cast<quint64>(json["frame_id"].toDouble());
    return meta;
}