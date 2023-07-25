layout(binding = 0, rgba32f) uniform image2D SSIMMap;
layout(binding = 1, rgba32f) uniform image2D SSIMSumMap;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

shared vec4 SharedData[gl_WorkGroupSize.x * gl_WorkGroupSize.y / WarpSize];

void main()
{
    ivec2 UV = ivec2(gl_GlobalInvocationID.xy);
    vec4 SSIM = imageLoad(SSIMMap, UV);

    for (uint i = WarpSize / 2; i >= 1; i /= 2)
    {
        SSIM += shuffleDownNV(SSIM, i, WarpSize);
    }

    if (gl_LocalInvocationIndex % WarpSize == 0)
    {
        SharedData[gl_LocalInvocationIndex / WarpSize] = SSIM;
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
