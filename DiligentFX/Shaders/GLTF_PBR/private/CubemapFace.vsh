cbuffer cbTransform
{
    float4x4 g_Rotation;
}

void main(in uint VertexId     : SV_VertexID,
          out float4 Pos       : SV_Position,
          out float3 WorldPos  : WORLD_POS)
{
    float2 PosXY[4];
    PosXY[0] = float2(-1.0, -1.0);
    PosXY[1] = float2(-1.0, +1.0);
    PosXY[2] = float2(+1.0, -1.0);
    PosXY[3] = float2(+1.0, +1.0);
    Pos = float4(PosXY[VertexId], 1.0, 1.0);
    float4 f4WorldPos = mul(g_Rotation, Pos);
    WorldPos = f4WorldPos.xyz / f4WorldPos.w;
#if (defined(GLSL) || defined(GL_ES)) && !defined(VULKAN)
    Pos.y *= -1.0;
#endif
}
