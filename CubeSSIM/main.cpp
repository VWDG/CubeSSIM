
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/core/utils/logger.hpp>

#include <iostream>
#include <map>
#include <string>

#include "imed_gpu.h"
#include "ssim_gpu.h"
#include "util.h"

using namespace std::string_literals;

void GLAPIENTRY MessageCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*);

int main()
{
    try
    {
        cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        GLFWwindow* Window = glfwCreateWindow(1200, 600, "Cube-SSIM", NULL, NULL);
        if (Window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(Window);

        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
            glfwTerminate();
            return -1;
        }

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(MessageCallback, 0);

        if (!IsExtensionAvailable("GL_NV_shader_thread_shuffle"))
        {
            std::cout << "GL_NV_shader_thread_shuffle is not available\n";
            std::cout << "The current implementation does not work without it\n";
            glfwTerminate();
            return -1;
        }

        auto ImagePathGT = "../images/image_true_cube_horizon.png"s;
        auto ImagePathInpainted = "../images/image_false_cube_horizon.png"s;

        std::cout << "Images:\n" << ImagePathGT << '\n' << ImagePathInpainted << "\n\n";

        // Load cube maps as 2D images
        auto Image1 = LoadImage(ImagePathGT);
        auto Image2 = LoadImage(ImagePathInpainted);

        // Load cube maps as actual cube maps
        auto Cubemap1 = LoadCubemap(ImagePathGT);
        auto Cubemap2 = LoadCubemap(ImagePathInpainted);

        // Compute SSIM and IMED scores
        auto SSIMImage = CSSIMGPU::GetInstance().ComputeSSIM(Cubemap1, Cubemap2);
        auto IMEDImage = CIMEDGPU::GetInstance().ComputeIMED(Cubemap1, Cubemap2);

        auto SSIMCubemap = CSSIMGPU::GetInstance().ComputeSSIM(Image1, Image2);
        auto IMEDCubemap = CIMEDGPU::GetInstance().ComputeIMED(Image1, Image2);

        // Use 622 weights
        const float YWeight = 0.6f;
        const float CbWeight = 0.2f;
        const float CrWeight = 0.2f;

        // Output results
        std::cout << "Weights:\n" << "Y:  " << YWeight << "\nCb: " << CbWeight << "\nCr: " << CrWeight << "\n\n\n";

        std::cout << "SSIM and IMED results for 2D images:\n\n";

        std::cout << "SSIM score (YCbCr): " << SSIMImage << '\n';
        std::cout << "SSIM score (total): " << SSIMImage[0] * YWeight + SSIMImage[1] * CbWeight + SSIMImage[2] * CrWeight << "\n\n";
        std::cout << "IMED score (YCbCr): " << IMEDImage << '\n';
        std::cout << "IMED score (total): " << IMEDImage[0] * YWeight + IMEDImage[1] * CbWeight + IMEDImage[2] * CrWeight << "\n\n\n";

        std::cout << "SSIM and IMED results for cube maps:\n\n";

        std::cout << "SSIM score (YCbCr): " << SSIMCubemap << '\n';
        std::cout << "SSIM score (total): " << SSIMCubemap[0] * YWeight + SSIMCubemap[1] * CbWeight + SSIMCubemap[2] * CrWeight << "\n\n";
        std::cout << "IMED score (YCbCr): " << IMEDCubemap << '\n';
        std::cout << "IMED score (total): " << IMEDCubemap[0] * YWeight + IMEDCubemap[1] * CbWeight + IMEDCubemap[2] * CrWeight << "\n\n\n";

        glfwTerminate();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }
}

void GLAPIENTRY
MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity > GL_DEBUG_SEVERITY_LOW)
    {
        std::cout << message << std::endl;
    }
}