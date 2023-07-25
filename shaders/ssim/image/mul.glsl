layout(binding = 0, rgba32f) uniform image2D Image1;
layout(binding = 1, rgba32f) uniform image2D Image2;
layout(binding = 2, rgba32f) uniform image2D ResultImage;

void main()
{
    ivec2 UV = ivec2(gl_GlobalInvocationID.xy);

    vec4 Result = imageLoad(Image1, UV) * imageLoad(Image2, UV);

    imageStore(ResultImage, UV, Result);
}
