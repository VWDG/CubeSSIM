layout(binding = 0, rgba32f) uniform image2D Image1;
layout(binding = 1, rgba32f) uniform image2D Image2;
layout(binding = 2, rgba32f) uniform image2D SumMap;

shared vec4 SharedData[gl_WorkGroupSize.x * gl_WorkGroupSize.y / WarpSize];

void main()
{
    int KernelHalf = GetKernelHalf();
    ivec2 ImageSize = imageSize(Image1);

    ivec2 UV = ivec2(gl_GlobalInvocationID.xy);
    int Col1 = UV.x;
    int Row1 = UV.y;

    vec3 ED = vec3(0.0f);
    float WeightSum = 0.0f;
    vec4 SumData = vec4(0.0f);

    if (Col1 < ImageSize.x && Row1 < ImageSize.y)
    {
        for (int Col2 = max(Col1 - KernelHalf, 0); Col2 < min(Col1 + KernelHalf, ImageSize.x); ++Col2)
        {
            for (int Row2 = max(Row1 - KernelHalf, 0); Row2 < min(Row1 + KernelHalf, ImageSize.y); ++Row2)
            {
                vec4 Pix1i = imageLoad(Image1, ivec2(Col1, Row1));
                vec4 Pix1j = imageLoad(Image1, ivec2(Col2, Row2));

                if (Pix1i.a > 0.0f && Pix1j.a > 0.0f)
                {
                    vec4 Pix2i = imageLoad(Image2, ivec2(Col1, Row1));
                    vec4 Pix2j = imageLoad(Image2, ivec2(Col2, Row2));

                    vec3 Diffi = Pix1i.rgb - Pix2i.rgb;
                    vec3 Diffj = Pix1j.rgb - Pix2j.rgb;
                    vec3 Diff = Diffi * Diffj;

                    float G = GetG(vec2(Col1, Row1), vec2(Col2, Row2));
                    ED += G * Diff;
                    WeightSum += G;
                }
            }
        }

        if (WeightSum > 0.0f)
        {
            SumData = vec4(ED / WeightSum, 1.0f);
        }
    }

    for (uint i = WarpSize / 2; i >= 1; i /= 2)
    {
        SumData += shuffleDownNV(SumData, i, WarpSize);
    }

    if (gl_LocalInvocationIndex % WarpSize == 0)
    {
        SharedData[gl_LocalInvocationIndex / WarpSize] = SumData;
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
            imageStore(SumMap, ivec2(gl_WorkGroupID), Sum);
        }
    }
}
