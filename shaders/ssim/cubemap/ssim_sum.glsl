
float AreaElement(float x, float y)
{
    return atan(sqrt(x * x + y * y + 1.0f), x * y);
}

float TexelCoordSolidAngle(vec2 UV, int Resolution)
{
    UV = (UV / Resolution) * 2.0f - 1.0f;

    float InvResolution = 1.0f / Resolution;
    float x0 = UV.x - InvResolution;
    float y0 = UV.y - InvResolution;
    float x1 = UV.x + InvResolution;
    float y1 = UV.y + InvResolution;
    float SolidAngle = AreaElement(x0, y0) - AreaElement(x0, y1) - AreaElement(x1, y0) + AreaElement(x1, y1);

    return abs(SolidAngle);
}

layout(binding = 0, rgba32f) uniform image2DArray SSIMMap;
layout(binding = 1, rgba32f) uniform image2D SSIMSumMap;

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

shared vec4 SharedData[gl_WorkGroupSize.x * gl_WorkGroupSize.y / WarpSize];

void main()
{
    ivec2 UV = ivec2(gl_GlobalInvocationID.xy);
    vec4 SSIMSum = vec4(0.0f);

    for (int i = 0; i < 6; ++ i)
    {
        SSIMSum += imageLoad(SSIMMap, ivec3(UV, i));
    }

    SSIMSum *= TexelCoordSolidAngle(UV, imageSize(SSIMMap).x);

    for (uint i = WarpSize / 2; i >= 1; i /= 2)
    {
        SSIMSum += shuffleDownNV(SSIMSum, i, WarpSize);
    }

    if (gl_LocalInvocationIndex % WarpSize == 0)
    {
        SharedData[gl_LocalInvocationIndex / WarpSize] = SSIMSum;
    }

    barrier();

    uint Iterations = gl_WorkGroupSize.x * gl_WorkGroupSize.y / WarpSize;
    if (gl_LocalInvocationIndex < Iterations)
    {
        vec4 Sum = SharedData[gl_LocalInvocationIndex];
        for (uint i = Iterations; i >= 1; i /= 2)
        {
            Sum += shuffleDownNV(Sum, i, WarpSize);
        }
        if (gl_LocalInvocationIndex == 0)
        {
            imageStore(SSIMSumMap, ivec2(gl_WorkGroupID), Sum);
        }
    }
}