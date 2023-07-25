#include "ssim_gpu.h"

#include "settings.h"
#include "util.h"

#define M_PI 3.14159265358979323846f

ScoreType CSSIMGPU::ComputeSSIMImage(GLuint Image1, GLuint Image2)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, KernelTexture);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, MSSIMBuffer);

    ExecuteShader2Image(FilterShader[CurMode], Image1, Textures[CurMode][Mu1]);             // mu1   = filter2(window, img1, 'valid');
    ExecuteShader2Image(FilterShader[CurMode], Image2, Textures[CurMode][Mu2]);             // mu2   = filter2(window, img2, 'valid');
    ExecuteShader3Image(MulShader[CurMode], Textures[CurMode][Mu1], Textures[CurMode][Mu1], Textures[CurMode][Mu1Sq]); // mu1_sq = mu1.*mu1;
    ExecuteShader3Image(MulShader[CurMode], Textures[CurMode][Mu2], Textures[CurMode][Mu2], Textures[CurMode][Mu2Sq]); // mu2_sq = mu2.*mu2;
    ExecuteShader3Image(MulShader[CurMode], Textures[CurMode][Mu1], Textures[CurMode][Mu2], Textures[CurMode][Mu12]);  // mu1_mu2 = mu1.*mu2;

    // sigma1_sq = filter2(window, img1.*img1, 'valid') - mu1_sq;
    // sigma2_sq = filter2(window, img2.*img2, 'valid') - mu2_sq;
    // sigma12 = filter2(window, img1.*img2, 'valid') - mu1_mu2;
    ExecuteShader3Image(MulShader[CurMode], Image1, Image1, Textures[CurMode][Im1Sq]);
    ExecuteShader3Image(MulShader[CurMode], Image2, Image2, Textures[CurMode][Im2Sq]);
    ExecuteShader3Image(MulShader[CurMode], Image1, Image2, Textures[CurMode][Im12]);
    ExecuteShader3Image(FilterMinusShader[CurMode], Textures[CurMode][Im1Sq], Textures[CurMode][Mu1Sq], Textures[CurMode][Sigma1Sq]);
    ExecuteShader3Image(FilterMinusShader[CurMode], Textures[CurMode][Im2Sq], Textures[CurMode][Mu2Sq], Textures[CurMode][Sigma2Sq]);
    ExecuteShader3Image(FilterMinusShader[CurMode], Textures[CurMode][Im12], Textures[CurMode][Mu12], Textures[CurMode][Sigma12]);

    // ssim_map = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))./((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2));
    CalculateSSIMMap(Image1);
    ScoreType SSIM = CalculateSSIM();
    return SSIM;
}

ScoreType CSSIMGPU::ComputeSSIMCubemap(GLuint Image1, GLuint Image2)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, KernelTexture);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, MSSIMBuffer);

    ExecuteShader2Cubemap(FilterShader[CurMode], Image1, Textures[CurMode][Mu1]);             // mu1   = filter2(window, img1, 'valid');
    ExecuteShader2Cubemap(FilterShader[CurMode], Image2, Textures[CurMode][Mu2]);             // mu2   = filter2(window, img2, 'valid');
    MulCubemap(MulShader[CurMode], Textures[CurMode][Mu1], Textures[CurMode][Mu1], Textures[CurMode][Mu1Sq]); // mu1_sq = mu1.*mu1;
    MulCubemap(MulShader[CurMode], Textures[CurMode][Mu2], Textures[CurMode][Mu2], Textures[CurMode][Mu2Sq]); // mu2_sq = mu2.*mu2;
    MulCubemap(MulShader[CurMode], Textures[CurMode][Mu1], Textures[CurMode][Mu2], Textures[CurMode][Mu12]);  // mu1_mu2 = mu1.*mu2;
    
    // sigma1_sq = filter2(window, img1.*img1, 'valid') - mu1_sq;
    // sigma2_sq = filter2(window, img2.*img2, 'valid') - mu2_sq;
    // sigma12 = filter2(window, img1.*img2, 'valid') - mu1_mu2;
    MulCubemap(MulShader[CurMode], Image1, Image1, Textures[CurMode][Im1Sq]);
    MulCubemap(MulShader[CurMode], Image2, Image2, Textures[CurMode][Im2Sq]);
    MulCubemap(MulShader[CurMode], Image1, Image2, Textures[CurMode][Im12]);
    FilterMinusCubemap(FilterMinusShader[CurMode], Textures[CurMode][Im1Sq], Textures[CurMode][Mu1Sq], Textures[CurMode][Sigma1Sq]);
    FilterMinusCubemap(FilterMinusShader[CurMode], Textures[CurMode][Im2Sq], Textures[CurMode][Mu2Sq], Textures[CurMode][Sigma2Sq]);
    FilterMinusCubemap(FilterMinusShader[CurMode], Textures[CurMode][Im12], Textures[CurMode][Mu12], Textures[CurMode][Sigma12]);

    // ssim_map = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))./((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2));
    CalculateSSIMMap(Image1);
    ScoreType SSIM = CalculateSSIM();
    return SSIM;
}

ScoreType CSSIMGPU::ComputeSSIM(const SImage& Image1, const SImage& Image2)
{
    CheckImages(Image1, Image2);
    
    if (Image1.IsCubemap)
    {
        CurMode = CubemapMode;
        SetCubemapResolution(Image1.Width);
        return ComputeSSIMCubemap(*Image1.GPU, *Image2.GPU);
    }
    else
    {
        CurMode = ImageMode;
        SetImageResolution(Image1.Width, Image2.Height);
        return ComputeSSIMImage(*Image1.GPU, *Image2.GPU);
    }
}

void CSSIMGPU::ExecuteShader2Image(GLuint Shader, GLuint Image, GLuint Result)
{
    glUseProgram(Shader);
    glBindImageTexture(0, Image, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, Result, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void CSSIMGPU::ExecuteShader3Image(GLuint Shader, GLuint Image1, GLuint Image2, GLuint Result)
{
    glUseProgram(Shader);
    glBindImageTexture(0, Image1, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, Image2, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(2, Result, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void CSSIMGPU::ExecuteShader2Cubemap(GLuint Shader, GLuint Image, GLuint Result)
{
    glUseProgram(Shader);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Image);
    glBindImageTexture(1, Result, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void CSSIMGPU::MulCubemap(GLuint Shader, GLuint Image1, GLuint Image2, GLuint Result)
{
    glUseProgram(Shader);
    glBindImageTexture(0, Image1, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, Image2, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(2, Result, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void CSSIMGPU::FilterMinusCubemap(GLuint Shader, GLuint Image, GLuint Subtrahend, GLuint Result)
{
    glUseProgram(Shader);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Image);
    glBindImageTexture(1, Subtrahend, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(2, Result, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void CSSIMGPU::CalculateSSIMMap(GLuint Image)
{
    auto IsLayered = CurMode == CubemapMode ? GL_TRUE : GL_FALSE;
    glUseProgram(SSIMMapShader[CurMode]);
    glBindImageTexture(0, Textures[CurMode][Mu12], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, Textures[CurMode][Sigma12], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(2, Textures[CurMode][Mu1Sq], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(3, Textures[CurMode][Mu2Sq], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(4, Textures[CurMode][Sigma1Sq], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(5, Textures[CurMode][Sigma2Sq], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(6, Textures[CurMode][SSSIMMap], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(7, Image, 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], WorkGroupCount[CurMode][2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

ScoreType CSSIMGPU::CalculateSSIM()
{
    auto IsLayered = CurMode == CubemapMode ? GL_TRUE : GL_FALSE;
    glBindImageTexture(0, Textures[CurMode][SSSIMMap], 0, IsLayered, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, SSIMSumMap[CurMode], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glUseProgram(SSIMSumShader[CurMode]);
    glDispatchCompute(WorkGroupCount[CurMode][0], WorkGroupCount[CurMode][1], 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    
    glUseProgram(SSIMSumFinalShader[CurMode]);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    ScoreType Score;

    if (CurMode == ImageMode)
    {
        float* SSIMSum = static_cast<float*>(glMapNamedBuffer(MSSIMBuffer, GL_READ_ONLY));
        glUnmapNamedBuffer(MSSIMBuffer);

        for (int i = 0; i < 3; ++i)
        {
            Score[i] = SSIMSum[i] / SSIMSum[3];
        }
    }
    else
    {
        ScoreType SSIMSum = *static_cast<ScoreType*>(glMapNamedBuffer(MSSIMBuffer, GL_READ_ONLY));
        glUnmapNamedBuffer(MSSIMBuffer);

        for (int i = 0; i < 3; ++i)
        {
            Score[i] = SSIMSum[i] /= 4.0f * M_PI; // Surface area of the unit sphere
        }
    }
    
    return Score;
}

void CSSIMGPU::CheckImages(const SImage& Image1, const SImage& Image2)
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

void CSSIMGPU::SetCubemapResolution(int x)
{
    if (CubemapSize == x)
    {
        return;
    }

    WorkGroupCount[CubemapMode][0] = DivUp(x, WorkGroupSize[CubemapMode][0]);
    WorkGroupCount[CubemapMode][1] = DivUp(x, WorkGroupSize[CubemapMode][1]);
    WorkGroupCount[CubemapMode][2] = 1;

    DeleteImages(CubemapMode);

    CubemapSize = x;

    for (int i = 0; i < ImageCount; ++i)
    {
        Textures[CubemapMode].push_back(CreateCubemap());
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &SSIMSumMap[CubemapMode]);
    glTextureStorage2D(SSIMSumMap[CubemapMode], 1, GL_RGBA32F, WorkGroupCount[CubemapMode][0], WorkGroupCount[CubemapMode][1]);
    glTextureParameteri(SSIMSumMap[CubemapMode], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(SSIMSumMap[CubemapMode], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void CSSIMGPU::SetImageResolution(int x, int y)
{
    if (ImageWidth == x && ImageHeight == y)
    {
        return;
    }

    WorkGroupCount[ImageMode][0] = DivUp(x, WorkGroupSize[ImageMode][0]);
    WorkGroupCount[ImageMode][1] = DivUp(y, WorkGroupSize[ImageMode][1]);
    WorkGroupCount[ImageMode][2] = 1;

    DeleteImages(ImageMode);

    ImageWidth = x;
    ImageHeight = y;

    for (int i = 0; i < ImageCount; ++ i)
    {
        Textures[ImageMode].push_back(CreateImage());
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &SSIMSumMap[ImageMode]);
    glTextureStorage2D(SSIMSumMap[ImageMode], 1, GL_RGBA32F, WorkGroupCount[ImageMode][0], WorkGroupCount[ImageMode][1]);
    glTextureParameteri(SSIMSumMap[ImageMode], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(SSIMSumMap[ImageMode], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

GLuint CSSIMGPU::CreateImage()
{
    GLuint Image;
    glCreateTextures(GL_TEXTURE_2D, 1, &Image);
    glTextureStorage2D(Image, 1, GL_RGBA32F, ImageWidth, ImageHeight);
    glTextureParameteri(Image, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(Image, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return Image;
}

GLuint CSSIMGPU::CreateCubemap()
{
    GLuint Image;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &Image);
    glTextureStorage2D(Image, 1, GL_RGBA32F, CubemapSize, CubemapSize);
    glTextureParameteri(Image, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(Image, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return Image;
}

void CSSIMGPU::DeleteImages(EMode Mode)
{
    auto Count = static_cast<GLsizei>(Textures[Mode].size());
    glDeleteTextures(Count, Textures[Mode].data());
    Textures[Mode].clear();
}

void CSSIMGPU::CreateKernelTexture()
{
    float Kernel[g_SSIMKernelSize * g_SSIMKernelSize];
    float Sum = 0.0f;
    for (int i = 0; i < g_SSIMKernelSize; ++i)
    {
        for (int j = 0; j < g_SSIMKernelSize; ++j)
        {
            int u = i - g_SSIMKernelSize / 2;
            int v = j - g_SSIMKernelSize / 2;

            Kernel[i + g_SSIMKernelSize * j] = GetGaussian(u, v, g_SSIMSigma);
            Sum += Kernel[i + g_SSIMKernelSize * j];
        }
    }

    for (int i = 0; i < g_SSIMKernelSize; ++i)
    {
        for (int j = 0; j < g_SSIMKernelSize; ++j)
        {
            Kernel[i + g_SSIMKernelSize * j] /= Sum;
        }
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &KernelTexture);
    glTextureStorage2D(KernelTexture, 1, GL_R32F, g_SSIMKernelSize, g_SSIMKernelSize);
    glTextureSubImage2D(KernelTexture, 0, 0, 0, g_SSIMKernelSize, g_SSIMKernelSize, GL_RED, GL_FLOAT, Kernel);
    glTextureParameteri(KernelTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(KernelTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

CSSIMGPU::CSSIMGPU()
    : ImageWidth(0)
    , ImageHeight(0)
    , CubemapSize(0)
    , KernelTexture(0)
    , MSSIMBuffer(0)
    , FilterShader{ 0, 0 }
    , MulShader{ 0, 0 }
    , FilterMinusShader{ 0, 0 }
    , SSIMMapShader{ 0, 0 }
    , SSIMSumShader{ 0, 0 }
    , SSIMSumMap{ 0, 0 }
{
    const std::string Common = "ssim/common.glsl";
    const std::string CommonMode[ModeCount] = { "ssim/image/common.glsl", "ssim/cubemap/common.glsl" };
    const std::string Path[ModeCount] = { "ssim/image/", "ssim/cubemap/" };

    for (int i = 0; i < ModeCount; ++ i)
    {
        FilterShader[i] = LoadShader({ Common, CommonMode[i], Path[i] + "filter.glsl" });
        MulShader[i] = LoadShader({ Common, CommonMode[i], Path[i] + "mul.glsl" });
        FilterMinusShader[i] = LoadShader({ Common, CommonMode[i], Path[i] + "filter_minus.glsl" });
        SSIMMapShader[i] = LoadShader({ Common, CommonMode[i], Path[i] + "ssim_map.glsl" });
        SSIMSumFinalShader[i] = LoadShader({ Common, Path[i] + "ssim_sum_final.glsl" });

        glGetProgramiv(FilterShader[i], GL_COMPUTE_WORK_GROUP_SIZE, WorkGroupSize[i]);
    }

    SSIMSumShader[ImageMode] = LoadShader({ Common, CommonMode[ImageMode], Path[ImageMode] + "ssim_sum.glsl" });
    SSIMSumShader[CubemapMode] = LoadShader({ Common, Path[CubemapMode] + "ssim_sum.glsl" });
    
    CreateKernelTexture();

    glCreateBuffers(1, &MSSIMBuffer);
    glNamedBufferData(MSSIMBuffer, sizeof(float) * 4, nullptr, GL_DYNAMIC_READ);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

CSSIMGPU::~CSSIMGPU()
{
    for (int i = 0; i < ModeCount; ++i)
    {
        glDeleteProgram(FilterShader[i]);
        glDeleteProgram(MulShader[i]);
        glDeleteProgram(FilterMinusShader[i]);
        glDeleteProgram(SSIMMapShader[i]);
        glDeleteProgram(SSIMSumShader[i]);
        DeleteImages(static_cast<EMode>(i));
    }
    glDeleteTextures(ModeCount, SSIMSumMap);
    glDeleteTextures(1, &KernelTexture);
    glDeleteBuffers(1, &MSSIMBuffer);
}