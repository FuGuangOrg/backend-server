#include "common_api.h"

#include <QDir>


QImage convert_cvmat_to_qimage(const cv::Mat& image, int output_channels)
{
    if (image.empty() || (output_channels != 1 && output_channels != 3))
    {
        return QImage();
    }
    // 处理输入通道数和输出通道数的关系
    if (image.type() == CV_8UC1)        // 输入单通道
    {
        if (output_channels == 1)       // 输出也是单通道
        {
            return QImage(image.data, image.cols, image.rows, image.step, QImage::Format_Grayscale8).copy();
        }
        else            // 输出三通道
        {
            cv::Mat converted;
            cv::cvtColor(image, converted, cv::COLOR_GRAY2RGB);
            return QImage(converted.data, converted.cols, converted.rows, converted.step, QImage::Format_RGB888).copy();
        }
    }
    else if (image.type() == CV_8UC3)       // 输入三通道
    {
        if (output_channels == 1)         // 输出单通道
        {
            cv::Mat converted;
            cv::cvtColor(image, converted, cv::COLOR_BGR2GRAY); // 或 COLOR_RGB2GRAY
            return QImage(converted.data, converted.cols, converted.rows, converted.step, QImage::Format_Grayscale8).copy();
        }
        else             // 输出也是三通道
        {
            // OpenCV 默认是 BGR，需要转换成 RGB
            cv::Mat converted;
            cv::cvtColor(image, converted, cv::COLOR_BGR2RGB);
            return QImage(converted.data, converted.cols, converted.rows, converted.step, QImage::Format_RGB888).copy();
        }
    }
    return QImage();
}


cv::Mat convert_qimage_to_cvmat(const QImage& image, int output_channels)
{
    if (image.isNull() || (output_channels != 1 && output_channels != 3))
    {
        return cv::Mat();
    }
    switch (image.format())
    {
    case QImage::Format_Grayscale8:   // 单通道灰度
    {
        cv::Mat mat(image.height(), image.width(), CV_8UC1,const_cast<uchar*>(image.bits()), image.bytesPerLine());
        if (output_channels == 1)
        {
            return mat.clone();  // 单通道直接返回
        }
        else
        {
            cv::Mat converted;
            cv::cvtColor(mat, converted, cv::COLOR_GRAY2BGR);
            return converted;
        }
    }
    case QImage::Format_RGB888:   // 3 通道 RGB
    {
        cv::Mat mat(image.height(), image.width(), CV_8UC3,const_cast<uchar*>(image.bits()), image.bytesPerLine());
        if (output_channels == 1)
        {
            cv::Mat gray;
            cv::cvtColor(mat, gray, cv::COLOR_RGB2GRAY);
            return gray;
        }
        else
        {
            cv::Mat bgr;
            cv::cvtColor(mat, bgr, cv::COLOR_RGB2BGR);  // 转成 OpenCV 默认的 BGR
            return bgr;
        }
    }

    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:    // 带 Alpha 或 32bit RGB
    {
        cv::Mat mat(image.height(), image.width(), CV_8UC4,const_cast<uchar*>(image.bits()), image.bytesPerLine());
        if (output_channels == 1)
        {
            cv::Mat gray;
            cv::cvtColor(mat, gray, cv::COLOR_BGRA2GRAY);
            return gray;
        }
        else
        {
            cv::Mat bgr;
            cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);
            return bgr;
        }
    }

    default:
    {
        // 不支持的格式，先转成一个常见格式再处理
        QImage converted = image.convertToFormat(QImage::Format_RGB888);
        return convert_qimage_to_cvmat(converted, output_channels);
    }
    }
}
