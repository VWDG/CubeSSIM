#version 460

#extension GL_NV_shader_thread_shuffle : enable
#extension GL_NV_shader_thread_group : enable

#define L 1.0f
#define WarpSize 32
#define Sigma 1.5f
#define MinWeight 1e-4f

layout(std430, binding = 0) buffer IMEDBuffer
{
    vec4 IMEDSum;
};

int GetKernelHalf()
{
    float D2 = log(MinWeight / (1.0f / (2.0f * 3.14159265359f* Sigma * Sigma))) * (2.0f * Sigma * Sigma);
    float D = sqrt(abs(D2));
    return int(D + 0.5f);
}

float GetGaussian(float D)
{
    return (1.0f / (2.0f * 3.14159265359f * Sigma * Sigma)) * exp(-(D * D) / (2.0f * Sigma * Sigma));
};

float GetG(vec2 Point1, vec2 Point2)
{
    //*
    float I_Diff = Point1.x - Point2.x;
    float J_Diff = Point1.y - Point2.y;

    return GetGaussian(sqrt(I_Diff * I_Diff + J_Diff * J_Diff));
    /*/
    return (Point1.x == Point2.x && Point1.y == Point2.y) ? 1.0f : 0.0f;
    //*/
};