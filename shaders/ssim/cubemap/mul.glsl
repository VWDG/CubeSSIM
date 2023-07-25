layout(binding = 0, rgba32f) uniform image2DArray Image1;
layout(binding = 1, rgba32f) uniform image2DArray Image2;
layout(binding = 2, rgba32f) uniform image2DArray ResultImage;

void main()
{
    ivec3 UVW = ivec3(gl_GlobalInvocationID.xyz);

    vec4 Result = imageLoad(Image1, UVW) * imageLoad(Image2, UVW);

    imageStore(ResultImage, UVW, Result);
}
