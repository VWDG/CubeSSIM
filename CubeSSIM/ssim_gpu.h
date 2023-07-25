#pragma once

#include "image.h"
#include "util.h"

#include <GL/glew.h>
#include <opencv2/opencv.hpp>

class CSSIMGPU
{
public:
    static CSSIMGPU& GetInstance()
    {
        static CSSIMGPU Instance;
        return Instance;
    }

    ScoreType ComputeSSIM(const SImage& Image1, const SImage& Image2);
    void SetImageResolution(int x, int y);
    void SetCubemapResolution(int x);

private:
    CSSIMGPU();
    ~CSSIMGPU();

    enum EMode
    {
        ImageMode,
        CubemapMode,
        ModeCount
    };

    ScoreType ComputeSSIMImage(GLuint Image1, GLuint Image2);
    ScoreType ComputeSSIMCubemap(GLuint Image1, GLuint Image2);
    static void CheckImages(const SImage& Image1, const SImage& Image2);

    void ExecuteShader2Image(GLuint Shader, GLuint Image, GLuint Result);
    void ExecuteShader3Image(GLuint Shader, GLuint Image1, GLuint Image2, GLuint Result);
    void ExecuteShader2Cubemap(GLuint Shader, GLuint Image, GLuint Result);
    void MulCubemap(GLuint Shader, GLuint Image1, GLuint Image2, GLuint Result);
    void FilterMinusCubemap(GLuint Shader, GLuint Image1, GLuint Image2, GLuint Result);

    void CalculateSSIMMap(GLuint Image);
    ScoreType CalculateSSIM();

    GLuint CreateImage();
    GLuint CreateCubemap();
    void DeleteImages(EMode Mode);

    void CreateKernelTexture();

    int ImageWidth, ImageHeight;
    int CubemapSize;

    GLuint KernelTexture;
    GLuint MSSIMBuffer;

    GLuint FilterShader[ModeCount];
    GLuint MulShader[ModeCount];
    GLuint FilterMinusShader[ModeCount];
    GLuint SSIMMapShader[ModeCount];
    GLuint SSIMSumShader[ModeCount];
    GLuint SSIMSumFinalShader[ModeCount];
    int WorkGroupSize[ModeCount][3];
    int WorkGroupCount[ModeCount][3];

    enum ENames
    {
        Mu1, Mu2, Mu1Sq, Mu2Sq, Sigma1Sq, Sigma2Sq, Mu12, Sigma12, Im1Sq, Im2Sq, Im12, SSSIMMap, ImageCount
    };

    std::vector<GLuint> Textures[ModeCount];
    GLuint SSIMSumMap[ModeCount];

    EMode CurMode;
};