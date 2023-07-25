layout(binding = 0, rgba32f) uniform image2D Mu12Image;
layout(binding = 1, rgba32f) uniform image2D Sigma12Image;
layout(binding = 2, rgba32f) uniform image2D Mu1SqImage;
layout(binding = 3, rgba32f) uniform image2D Mu2SqImage;
layout(binding = 4, rgba32f) uniform image2D Sigma1SqImage;
layout(binding = 5, rgba32f) uniform image2D Sigma2SqImage;
layout(binding = 6, rgba32f) uniform image2D ResultImage;
layout(binding = 7, rgba32f) uniform image2D InputImage;

void main()
{
    // ssim_map = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))./((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2));

    ivec2 UV = ivec2(gl_GlobalInvocationID.xy);

    vec4 Input = imageLoad(InputImage, UV);

    if (Input.a < 1.0f)
    {
        imageStore(ResultImage, UV, vec4(0.0f));
        return;
    }

    float C1 = (K1 * L) * (K1 * L);
    float C2 = (K2 * L) * (K2 * L);

    vec4 Mu12 = imageLoad(Mu12Image, UV);
    vec4 Sigma12 = imageLoad(Sigma12Image, UV);
    vec4 Mu1Sq = imageLoad(Mu1SqImage, UV);
    vec4 Mu2Sq = imageLoad(Mu2SqImage, UV);
    vec4 Sigma1Sq = imageLoad(Sigma1SqImage, UV);
    vec4 Sigma2Sq = imageLoad(Sigma2SqImage, UV);

    vec4 Result = ((2.0f * Mu12 + C1) * (2.0f * Sigma12 + C2)) / ((Mu1Sq + Mu2Sq + C1) * (Sigma1Sq + Sigma2Sq + C2));

    imageStore(ResultImage, UV, vec4(Result.rgb, 1.0f));
}
