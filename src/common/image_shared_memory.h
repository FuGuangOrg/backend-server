#pragma once
#include "common_global.h"
#include <QSharedMemory>
#include <QImage>
#include <QJsonObject>

constexpr int buffer_size = 3;      //三缓冲

// 影像元数据结构
struct st_image_meta
{
    QString shared_memory_key{ "" };
    int width{ 0 };
    int height{ 0 };
    int channels{ 0 };
    int index{ 0 };
    bool is_new_memory{ false }; // 是否是新创建的共享内存
    quint64 frame_id{ 0 };
};

class COMMON_EXPORT image_shared_memory
{
public:
    explicit image_shared_memory(const QString& key);

    // 写入图像，如果宽高/bytesPerLine变化会自动重建
    bool write_image(const QImage& img, st_image_meta& meta);

    // 读端：读取图像，失败时返回空 QImage
    QImage read_image(const st_image_meta& meta);

    // 将元数据打包成 JSON（用于 TCP 发送）
    static QJsonObject meta_to_json(const st_image_meta& meta);

    // 从 JSON 解析元数据
    static st_image_meta json_to_meta(const QJsonObject& json);

    void detach();
private:
    QString m_key_prefix{ "" };             //key前缀,不同的模块使用不同的共享内存，通过key区分。这里只需要为不同的模块指定不同的前缀即可
    QMap<QString, QSharedMemory*> m_shared_memories;
    int m_width{ 0 };
    int m_height{ 0 };
    int m_channels{ 0 };
    quint64 m_frame_counter{ 0 };
    int m_index{ 0 };  // 0 --> buffer_size-1
};