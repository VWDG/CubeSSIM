layout(binding = 1) uniform samplerCube Image;
layout(binding = 1, rgba32f) uniform image2DArray Filtered;

void main()
{
    int KernelHalf = GetKernelHalf();
    int TextureSize = textureSize(Image, 0).x;

    ivec3 UVW = ivec3(gl_GlobalInvocationID.xyz);

    vec4 Result = vec4(0.0f);
    
    for (int x = -KernelHalf; x <= KernelHalf; ++ x)
    {
        for (int y = -KernelHalf; y <= KernelHalf; ++ y)
        {
            ivec2 SampleUV = UVW.xy + ivec2(x, y);

            if (IsSampleValid(SampleUV, TextureSize))
            {
                ivec3 UVWOffset = UVW;
                UVWOffset.xy += ivec2(x, y);
                vec3 Direction = IDToVector(UVWOffset, TextureSize);
                vec4 Pixel = textureLod(Image, Direction, 0);
                float Weight = texelFetch(Kernel, ivec2(x, y) + KernelHalf, 0).r;
                Result += Weight * Pixel;
            }
        }
    }

    imageStore(Filtered, UVW, Result);
}