# CubeSSIM

This project implements the and Structural Similarity Index Measure (SSIM) and Image Euclidian Distance (IMED) for images as well as cube maps. It is implemented in C++ using OpenGL and GLSL.

## Usage
The *bin* folder already contains a compiled version called *CubeSSIMr.exe* which just loads two images and shows the SSIM and IMED values for 2D images as well as cube maps. It needs to be run from the *bin* folder. The application depends on *OpenGL 4.3* or later and the extension *GL_NV_shader_thread_shuffle*. It was tested on Windows 10 and Nvidia hardware.
## Building the C++ Project
Visual Studio 2022 is required for building the project.
The project depends on the external libraries *glm*, *glfw3*, and *opencv*. All external libaries are available via *vcpkg*. Here is a simple explanation on how to set up vcpkg: https://vcpkg.io/en/getting-started.html
The dependencies are listed in the *vcpkg_deps.txt* file. After setting up *vcpkg*, you can install all dependencies using the following command:

**vcpkg --triplet x64-windows install "@vcpkg_deps.txt"**

Lastly, you can simply open the solution in Visual Studio 2022 and compile the project. The main.cpp file contains a simple example that is also available as a binary in the *bin* folder.
