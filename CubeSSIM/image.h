#pragma once

#include <GL/glew.h>
#include <opencv2/opencv.hpp>

struct SImage
{
    bool IsCubemap = false;
    cv::Mat CPU;
    std::shared_ptr<GLuint> GPU;
    cv::Mat FacesCPU[6];
    int Width, Height;
};