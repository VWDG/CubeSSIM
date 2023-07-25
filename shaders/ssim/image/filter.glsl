layout(binding = 0, rgba32f) uniform image2D Image;
layout(binding = 1, rgba32f) uniform image2D Filtered;

void main()
{
    int KernelHalf = GetKernelHalf();
    ivec2 UV = ivec2(gl_GlobalInvocationID.xy);
    ivec2 ImageSize = imageSize(Image); 

    vec4 Pixel = imageLoad(Image, UV);

    if (Pixel.a == 0.0f)
    {
        imageStore(Filtered, UV, vec4(0.0f));
        return;
    }

    vec4 Result = vec4(0.0f);
    vec4 TotalWeight = vec4(0.0f);
    for (int x = -KernelHalf; x <= KernelHalf; ++ x)
    {
        for (int y = -KernelHalf; y <= KernelHalf; ++ y)
        {
            ivec2 Sample = UV + ivec2(x, y);
            if (Sample.x >= 0 && Sample.y >= 0 && Sample.x < ImageSize.x && Sample.y < ImageSize.y)
            {
                vec4 Pixel = imageLoad(Image, Sample);
                if (Pixel.a > 0)
                {
                    float Weight = texelFetch(Kernel, ivec2(x, y) + KernelHalf, 0).r;
                    TotalWeight += Weight;
                    Result += Weight * Pixel;
                }
            }
        }
    }

    imageStore(Filtered, UV, Result / TotalWeight);
}
