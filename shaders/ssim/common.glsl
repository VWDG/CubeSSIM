#version 460

#extension GL_NV_shader_thread_shuffle : enable
#extension GL_NV_shader_thread_group : enable

#define K1 0.01f
#define K2 0.03f
#define L 1.0f
#define WarpSize 32

layout(binding = 0) uniform sampler2D Kernel;

layout(std430, binding = 0) buffer MSSIMBuffer
{
    vec4 SSIMSum;
};

int GetKernelHalf()
{
    return (textureSize(Kernel, 0).x / 2);
}
