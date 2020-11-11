
void ClampParticlePosition(inout float2 f2Pos,
                           inout float2 f2Speed,
                           in    float  fSize,
                           in    float2 f2Scale)
{
    if (f2Pos.x + fSize * f2Scale.x > 1.0)
    {
        f2Pos.x -= f2Pos.x + fSize * f2Scale.x - 1.0;
        f2Speed.x *= -1.0;
    }

    if (f2Pos.x - fSize * f2Scale.x < -1.0)
    {
        f2Pos.x += -1.0 - (f2Pos.x - fSize * f2Scale.x);
        f2Speed.x *= -1.0;
    }

    if (f2Pos.y + fSize * f2Scale.y > 1.0)
    {
        f2Pos.y -= f2Pos.y + fSize * f2Scale.y - 1.0;
        f2Speed.y *= -1.0;
    }

    if (f2Pos.y - fSize * f2Scale.y < -1.0)
    {
        f2Pos.y += -1.0 - (f2Pos.y - fSize * f2Scale.y);
        f2Speed.y *= -1.0;
    }
}

int3 GetGridLocation(float2 f2Pos, int2 i2ParticleGridSize)
{
    int3 i3GridPos;
    i3GridPos.x = clamp(int((f2Pos.x + 1.0) * 0.5 * float(i2ParticleGridSize.x)), 0, i2ParticleGridSize.x - 1);
    i3GridPos.y = clamp(int((f2Pos.y + 1.0) * 0.5 * float(i2ParticleGridSize.y)), 0, i2ParticleGridSize.y - 1);
    i3GridPos.z = i3GridPos.x + i3GridPos.y * i2ParticleGridSize.x;
    return i3GridPos;
}
