layout(binding = 0) uniform samplerCube Image1;
layout(binding = 1) uniform samplerCube Image2;
layout(binding = 2, rgba32f) uniform image2D SumMap;

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

shared vec4 SharedData[gl_WorkGroupSize.x * gl_WorkGroupSize.y / WarpSize];

void main()
{
    int KernelHalf = GetKernelHalf();
    int TextureSize = textureSize(Image1, 0).x;

    ivec2 UV = ivec2(gl_GlobalInvocationID.xy);
    int Col1 = UV.x;
    int Row1 = UV.y;

    vec3 ED = vec3(0.0f);
    float WeightSum = 0.0f;
    vec4 SumData = vec4(0.0f);

    if (Col1 < TextureSize && Row1 < TextureSize)
    {
        for (int Face = 0; Face < 6; ++ Face)
        {
            for (int Col2 = Col1 - KernelHalf; Col2 < Col1 + KernelHalf; ++Col2)
            {
                for (int Row2 = Row1 - KernelHalf; Row2 < Row1 + KernelHalf; ++Row2)
                {
                    if (IsSampleValid(ivec2(Col2, Row2), TextureSize))
                    {
                        vec3 Direction;

                        Direction = IDToVector(ivec3(Col1, Row1, Face), TextureSize);
                        vec4 Pix1i = textureLod(Image1, Direction, 0);
                        Direction = IDToVector(ivec3(Col2, Row2, Face), TextureSize);
                        vec4 Pix1j = textureLod(Image1, Direction, 0);

                        Direction = IDToVector(ivec3(Col1, Row1, Face), TextureSize);
                        vec4 Pix2i = textureLod(Image2, Direction, 0);
                        Direction = IDToVector(ivec3(Col2, Row2, Face), TextureSize);
                        vec4 Pix2j = textureLod(Image2, Direction, 0);

                        vec3 Diffi = Pix1i.rgb - Pix2i.rgb;
                        vec3 Diffj = Pix1j.rgb - Pix2j.rgb;
                        vec3 Diff = Diffi * Diffj;

                        float G = GetG(vec2(Col1, Row1), vec2(Col2, Row2));
                        ED += G * Diff;
                        WeightSum += G;
                    }
                }
            }
        }

        if (WeightSum > 0.0f)
        {
            SumData = vec4(ED / WeightSum, 1.0f);
        }
    }

    SumData *= TexelCoordSolidAngle(vec2(Col1, Row1), TextureSize) * 6;

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
