layout(binding = 1, rgba32f) uniform image2D SSIMSumMap;

ivec2 IndexToUV(uint Index, uint Width)
{
    uvec2 UV;
    UV.x = Index % Width;
    UV.y = Index / Width;
    return ivec2(UV);
}

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

shared vec4 SharedData[gl_WorkGroupSize.x];

void main()
{
    ivec2 ImageSize = imageSize(SSIMSumMap);
    uint PixelCount = ImageSize.x * ImageSize.y;
    uint Iterations = (PixelCount + gl_WorkGroupSize.x - 1) / gl_WorkGroupSize.x;

    vec4 Sum = vec4(0.0f);
    for (uint i = 0; i < Iterations; ++ i)
    {
        uint Index = gl_LocalInvocationIndex + (gl_WorkGroupSize.x * i);
        ivec2 UV = IndexToUV(Index, ImageSize.x);
        if (UV.x < ImageSize.x && UV.y < ImageSize.y)
        {
            Sum += imageLoad(SSIMSumMap, UV);
        }
    }

    SharedData[gl_LocalInvocationIndex] = Sum;
    barrier();

    for (uint i = gl_WorkGroupSize.x / 2; i >= 1; i /= 2)
    {
        if (gl_LocalInvocationIndex < i)
        {
            SharedData[gl_LocalInvocationIndex] += SharedData[gl_LocalInvocationIndex + i];
        }
        barrier();
    }

    if (gl_LocalInvocationIndex == 0)
    {
        SSIMSum = SharedData[gl_LocalInvocationIndex];
    }
}
