#include "util.h"

#include <fstream>
#include <streambuf>
#define _USE_MATH_DEFINES
#include <math.h>

cv::Mat ConvertImage(const cv::Mat& Image)
{
    cv::Mat ConvertedImage;
    Image.convertTo(ConvertedImage, CV_32FC3);

    switch (g_ImageFormat)
    {
    case RGBA:
        cv::cvtColor(ConvertedImage, ConvertedImage, cv::COLOR_BGR2RGB);
        break;
    case YCbCrA:
        cv::cvtColor(ConvertedImage, ConvertedImage, cv::COLOR_BGR2YCrCb);
        break;
    case MONOCHROME:
        cv::cvtColor(ConvertedImage, ConvertedImage, cv::COLOR_BGR2GRAY);
        cv::cvtColor(ConvertedImage, ConvertedImage, cv::COLOR_GRAY2BGR);
        break;
    }

    cv::Mat ResultImage(Image.rows, Image.cols, CV_32FC4);

    for (int i = 0; i < ConvertedImage.cols * ConvertedImage.rows; ++i)
    {
        auto& Pixel = ConvertedImage.at<cv::Vec3f>(i);
        auto& OrigPixel = Image.at<cv::Vec3b>(i);

        cv::Vec4f ResultPixel(Pixel[0], Pixel[1], Pixel[2], 255.0f);
        ResultPixel = ResultPixel / 255.0f;
        ResultImage.at<cv::Vec4f>(i) = ResultPixel;
    }

    return ResultImage;
}

int DivUp(int TotalShaderCount, int WorkGroupSize)
{
    return (TotalShaderCount + WorkGroupSize - 1) / WorkGroupSize;
}

float GetGaussian(float D, float Sigma)
{
    return (1.0f / (2.0f * static_cast<float>(M_PI) * Sigma * Sigma)) * expf(-(D * D) / (2.0f * Sigma * Sigma));
};

float GetGaussian(int u, int v, float Sigma)
{
    float D = sqrtf(static_cast<float>(u) * u + v * v);
    return GetGaussian(D, Sigma);
};

int GetKernelSize(float Sigma, float MinWeight)
{
    float D2 = logf(MinWeight / (1.0f / (2.0f * static_cast<float>(M_PI)* Sigma * Sigma))) * (2.0f * Sigma * Sigma);
    float D = sqrt(abs(D2));
    return static_cast<int>(D + 0.5f);
}

bool IsExtensionAvailable(const std::string& Extension)
{
    GLint ExtensionCount;
    glGetIntegerv(GL_NUM_EXTENSIONS, &ExtensionCount);
    for (GLint i = 0; i < ExtensionCount; ++i) {
        std::string CurrentExtension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));;
        if (CurrentExtension == Extension)
        {
            return true;
        }
    }
    return false;
}

GLuint LoadShader(const std::initializer_list<std::string>& Paths)
{
    const std::string ShaderDirectory = "../shaders/";

    std::string ShaderSource;

    for (auto Path : Paths)
    {
        std::ifstream File(ShaderDirectory + Path);
        if (!File.is_open())
        {
            std::string Error = "Shader file " + Path + " not found";
            throw std::exception(Error.c_str());
        }
        std::string FileContent((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
        ShaderSource += FileContent;
    }

    const GLchar* pContent = ShaderSource.c_str();

    GLuint Shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(Shader, 1, &pContent, NULL);
    glCompileShader(Shader);

    GLint Success;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Success);
    if (!Success)
    {
        GLchar InfoLog[2048];
        glGetShaderInfoLog(Shader, 2048, NULL, InfoLog);
        std::cout << "Error when compiling shader " << Paths.end() << ":\n" << InfoLog << std::endl;
        glDeleteShader(Shader);
        throw std::exception("Shader compilation error");
    };

    // shader Program
    GLuint Program = glCreateProgram();
    glAttachShader(Program, Shader);
    glLinkProgram(Program);

    glGetProgramiv(Program, GL_LINK_STATUS, &Success);
    if (!Success)
    {
        GLchar InfoLog[2048];
        glGetProgramInfoLog(Program, 2048, NULL, InfoLog);
        std::cout << "Error when linking shader program" << Paths.end() << ":\n" << InfoLog << std::endl;
        glDeleteShader(Shader);
        glDeleteProgram(Program);
        throw std::exception("Program linking error");
    }

    glDeleteShader(Shader);

    return Program;
}

SImage LoadImage(const std::string& Path)
{
    cv::Mat File = cv::imread(Path);

    if (!File.data)
    {
        throw std::exception("File could not be loaded!");
    }

    cv::Mat CPUImage = ConvertImage(File);

    SImage Image;
    Image.CPU = CPUImage;
    Image.IsCubemap = false;

    std::shared_ptr<GLuint> TexturePtr(new GLuint, [](GLuint* tex) {glDeleteTextures(1, tex); });

    glCreateTextures(GL_TEXTURE_2D, 1, TexturePtr.get());
    glTextureStorage2D(*TexturePtr, 1, GL_RGBA32F, File.cols, File.rows);
    glTextureSubImage2D(*TexturePtr, 0, 0, 0, File.cols, File.rows, GL_RGBA, GL_FLOAT, CPUImage.data);
    glTextureParameteri(*TexturePtr, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(*TexturePtr, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    Image.GPU = TexturePtr;

    Image.Width = File.cols;
    Image.Height = File.rows;

    return Image;
}

SImage LoadCubemap(const std::string& Path)
{
    // Load the file of the horizontal cube map and split it into its faces

    cv::Mat HorizontalCubemap = cv::imread(Path);

    if (HorizontalCubemap.data == nullptr)
    {
        throw std::exception("Could not load cube map file.");
    }

    std::vector<cv::Mat> CubemapFaces;

    auto Width = HorizontalCubemap.cols;
    auto Height = HorizontalCubemap.rows;
    auto CubemapSize = Height;

    if (Width / 6 != Height)
    {
        throw std::exception("Wrong cubemap format.");
    }

    int Offset = CubemapSize * 0;
    auto ROI = cv::Rect(0, 0, CubemapSize, CubemapSize);
    auto ZP = HorizontalCubemap(ROI);
    cv::flip(ZP, ZP, 0);

    Offset = CubemapSize * 1;
    ROI = cv::Rect(Offset, 0, CubemapSize, CubemapSize);
    auto XP = HorizontalCubemap(ROI);
    cv::flip(XP, XP, 0);

    Offset = CubemapSize * 2;
    ROI = cv::Rect(Offset, 0, CubemapSize, CubemapSize);
    auto ZN = HorizontalCubemap(ROI);
    cv::flip(ZN, ZN, 0);

    Offset = CubemapSize * 3;
    ROI = cv::Rect(Offset, 0, CubemapSize, CubemapSize);
    auto XN = HorizontalCubemap(ROI);
    cv::flip(XN, XN, 0);

    Offset = CubemapSize * 4;
    ROI = cv::Rect(Offset, 0, CubemapSize, CubemapSize);
    auto YP = HorizontalCubemap(ROI);
    cv::flip(YP, YP, 0);

    Offset = CubemapSize * 5;
    ROI = cv::Rect(Offset, 0, CubemapSize, CubemapSize);
    auto YN = HorizontalCubemap(ROI);
    cv::flip(YN, YN, 0);

    CubemapFaces.push_back(XP);
    CubemapFaces.push_back(XN);
    CubemapFaces.push_back(YP);
    CubemapFaces.push_back(YN);
    CubemapFaces.push_back(ZP);
    CubemapFaces.push_back(ZN);

    // Store the cube map as a OpenGL texture

    SImage Image;

    std::shared_ptr<GLuint> TexturePtr(new GLuint, [](GLuint* tex) {glDeleteTextures(1, tex); });

    glGenTextures(1, TexturePtr.get());
    glBindTexture(GL_TEXTURE_CUBE_MAP, *TexturePtr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA32F, XP.cols, XP.rows);

    for (int i = 0; i < g_FaceCount; ++i)
    {
        cv::Mat Face = CubemapFaces[i];

        if (Face.cols != Face.rows)
        {
            throw std::exception("Cubemap faces have to be square");
        }

        cv::Mat CPUImage = ConvertImage(Face);

        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, Face.cols, Face.rows, GL_RGBA, GL_FLOAT, CPUImage.data);

        Image.FacesCPU[i] = CPUImage;
        Image.Width = Face.cols;
        Image.Height = Face.rows;
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    Image.IsCubemap = true;
    Image.GPU = TexturePtr;

    return Image;
}