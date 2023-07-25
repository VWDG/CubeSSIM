#include "imed_gpu.h"

#include "settings.h"
#include "util.h"

#define M_PI 3.14159265358979323846f

ScoreType CIMEDGPU::ComputeIMEDImage(GLuint Image1, GLuint Image2)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, IMEDBuffer);

    if (CurMode == ImageMode)
    {
        // Compute IMED per pixel and do first pass of summation
        glUseProgram(IMEDShader[CurMode]);
        glBindImageTexture(0, Image1, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(1, Image2, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(2, IMEDSumMap[CurMode], 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
        glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, Image1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, Image2);

        // Compute IMED per pixel and do first pass of summation
        glUseProgram(IMEDShader[CurMode]);
        glBindImageTexture(2, IMEDSumMap[CurMode], 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
        glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    }

    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Now do the final summation and read back data to the CPU
    glUseProgram(SumShader[CurMode]);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    float* IMEDSum = static_cast<float*>(glMapNamedBuffer(IMEDBuffer, GL_READ_ONLY));
    glUnmapNamedBuffer(IMEDBuffer);

    if (CurMode == CubemapMode)
    {
        for (int i = 0; i < 3; ++i)
        {
            IMEDSum[i] = sqrtf(IMEDSum[i] / (4.0f * M_PI));
        }
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            IMEDSum[i] = sqrtf(IMEDSum[i] / IMEDSum[3]);
        }
    }

    return ScoreType(IMEDSum[0], IMEDSum[1], IMEDSum[2]);
}

ScoreType CIMEDGPU::ComputeIMED(const SImage& Image1, const SImage& Image2)
{
    CheckImages(Image1, Image2);
    
    if (Image1.IsCubemap)
    {
        CurMode = CubemapMode;

        if (CubemapSize != Image1.Width)
        {
            CubemapSize = Image1.Width;

            WorkGroupCount[CurMode][0] = DivUp(CubemapSize, WorkGroupSize[CurMode][0]);
            WorkGroupCount[CurMode][1] = DivUp(CubemapSize, WorkGroupSize[CurMode][1]);
            WorkGroupCount[CurMode][2] = 1;

            glCreateTextures(GL_TEXTURE_2D, 1, &IMEDSumMap[CurMode]);
            glTextureStorage2D(IMEDSumMap[CurMode], 1, GL_RGBA32F, WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1]);
            glTextureParameteri(IMEDSumMap[CurMode], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(IMEDSumMap[CurMode], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
    }
    else
    {
        CurMode = ImageMode;

        if (ImageWidth != Image1.CPU.cols || ImageHeight != Image1.CPU.rows)
        {
            ImageWidth = Image1.CPU.cols;
            ImageHeight = Image1.CPU.rows;

            WorkGroupCount[CurMode][0] = DivUp(ImageWidth, WorkGroupSize[CurMode][0]);
            WorkGroupCount[CurMode][1] = DivUp(ImageHeight, WorkGroupSize[CurMode][1]);
            WorkGroupCount[CurMode][2] = 1;

            glCreateTextures(GL_TEXTURE_2D, 1, &IMEDSumMap[CurMode]);
            glTextureStorage2D(IMEDSumMap[CurMode], 1, GL_RGBA32F, WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1]);
            glTextureParameteri(IMEDSumMap[CurMode], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(IMEDSumMap[CurMode], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
    }

    return ComputeIMEDImage(*Image1.GPU, *Image2.GPU);
}

void CIMEDGPU::CheckImages(const SImage& Image1, const SImage& Image2)
{
    if (Image1.IsCubemap != Image2.IsCubemap)
    {
        throw std::exception("Different image types");
    }

    if (Image1.IsCubemap)
    {
        if (Image1.FacesCPU[0].cols == Image2.FacesCPU[0].cols ==
            Image1.FacesCPU[0].rows == Image2.FacesCPU[0].rows)
        {
            throw std::exception("Different image sizes");
        }
    }
    else
    {
        if (Image1.CPU.cols != Image2.CPU.cols ||
            Image1.CPU.rows != Image2.CPU.rows)
        {
            throw std::exception("Different image sizes");
        }
    }
}

CIMEDGPU::CIMEDGPU()
    : ImageWidth(0)
    , ImageHeight(0)
    , CubemapSize(0)
    , IMEDBuffer(0)
    , IMEDSumMap{ 0, 0 }
{
    const std::string Common = "imed/common.glsl";
    const std::string CommonMode[ModeCount] = { "imed/image/common.glsl", "imed/cubemap/common.glsl" };
    const std::string Path[ModeCount] = { "imed/image/", "imed/cubemap/" };

    for (int i = 0; i < ModeCount; ++ i)
    {
        IMEDShader[i] = LoadShader({ Common, CommonMode[i], Path[i] + "imed.glsl" });
        SumShader[i] = LoadShader({ Common, Path[i] + "sum.glsl" });

        glGetProgramiv(IMEDShader[i], GL_COMPUTE_WORK_GROUP_SIZE, WorkGroupSize[i]);
    }

    glCreateBuffers(1, &IMEDBuffer);
    glNamedBufferData(IMEDBuffer, sizeof(float) * 4, nullptr, GL_DYNAMIC_READ);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

CIMEDGPU::~CIMEDGPU()
{
    for (int i = 0; i < ModeCount; ++i)
    {
        glDeleteProgram(IMEDShader[i]);
        glDeleteProgram(SumShader[i]);
    }
    glDeleteTextures(ModeCount, IMEDSumMap);
    glDeleteBuffers(1, &IMEDBuffer);
}