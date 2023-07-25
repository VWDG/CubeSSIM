layout(binding = 0, rgba32f) uniform image2DArray Mu12Image;
layout(binding = 1, rgba32f) uniform image2DArray Sigma12Image;
layout(binding = 2, rgba32f) uniform image2DArray Mu1SqImage;
layout(binding = 3, rgba32f) uniform image2DArray Mu2SqImage;
layout(binding = 4, rgba32f) uniform image2DArray Sigma1SqImage;
layout(binding = 5, rgba32f) uniform image2DArray Sigma2SqImage;
layout(binding = 6, rgba32f) uniform image2DArray ResultImage;

void main()
{
    // ssim_map = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))./((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2));

    float C1 = (K1 * L) * (K1 * L);
    float C2 = (K2 * L) * (K2 * L);

    ivec3 UVW = ivec3(gl_GlobalInvocationID.xyz);

    vec4 Mu12 = imageLoad(Mu12Image, UVW);
    vec4 Sigma12 = imageLoad(Sigma12Image, UVW);
    vec4 Mu1Sq = imageLoad(Mu1SqImage, UVW);
    vec4 Mu2Sq = imageLoad(Mu2SqImage, UVW);
    vec4 Sigma1Sq = imageLoad(Sigma1SqImage, UVW);
    vec4 Sigma2Sq = imageLoad(Sigma2SqImage, UVW);

    vec4 Result = ((2.0f * Mu12 + C1) * (2.0f * Sigma12 + C2)) / ((Mu1Sq + Mu2Sq + C1) * (Sigma1Sq + Sigma2Sq + C2));

    imageStore(ResultImage, UVW, Result);
}
