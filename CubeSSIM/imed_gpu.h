#pragma once

#include "image.h"
#include "util.h"

#include <GL/glew.h>
#include <opencv2/opencv.hpp>

class CIMEDGPU
{
public:
    static CIMEDGPU& GetInstance()
    {
        static CIMEDGPU Instance;
        return Instance;
    }

    ScoreType ComputeIMED(const SImage& Image1, const SImage& Image2);

private:
    CIMEDGPU();
    ~CIMEDGPU();

    enum EMode
    {
        ImageMode,
        CubemapMode,
        ModeCount
    };

    ScoreType ComputeIMEDImage(GLuint Image1, GLuint Image2);
    static void CheckImages(const SImage& Image1, const SImage& Image2);

    int ImageWidth, ImageHeight;
    int CubemapSize;

    GLuint IMEDBuffer;

    GLuint IMEDShader[ModeCount];
    int WorkGroupSize[ModeCount][3];
    int WorkGroupCount[ModeCount][3];

    GLuint IMEDSumMap[ModeCount];
    GLuint SumShader[ModeCount];

    EMode CurMode;
};