bool IsSampleValid(ivec2 UV, int Size)
{
    bool IsUValid = (UV.x >= 0 && UV.x < Size);
    bool IsVValid = (UV.y >= 0 && UV.y < Size);
    return IsUValid || IsVValid;
}

vec3 IDToVector(ivec3 ID, int Resolution)
{
    vec3 Direction = vec3(0.0f, 0.0f, 0.0f);
    vec2 uvVector = (vec2(ID.xy) / Resolution) * 2 - 1.0f;
    uvVector += (1.0f / Resolution);

    uint Face = ID.z;

    switch (Face)
    {
        case 0:
            Direction.x = 1;
            Direction.yz = -uvVector.yx;
            break;
        case 1:
            Direction.x = -1;
            Direction.yz = vec2(-uvVector.y, uvVector.x);
            break;
        case 2:
            Direction.y = 1;
            Direction.xz = uvVector.xy;
            break;
        case 3:
            Direction.y = -1;
            Direction.xz = vec2(uvVector.x, -uvVector.y);
            break;
        case 4:
            Direction.z = 1;
            Direction.xy = vec2(uvVector.x, -uvVector.y);
            break;
        case 5:
            Direction.z = -1;
            Direction.xy = -uvVector.xy;
            break;
    }

    return normalize(Direction);
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
