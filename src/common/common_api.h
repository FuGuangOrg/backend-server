#pragma once

#include <QImage>
#include <opencv2/opencv.hpp>
#include "common_global.h"

#include "common.h"

//将cv::Mat 转换成QImage
QImage COMMON_EXPORT convert_cvmat_to_qimage(const cv::Mat& image, int output_channels);

//将QImage转换成cv::Mat
cv::Mat COMMON_EXPORT convert_qimage_to_cvmat(const QImage& image, int output_channels);

