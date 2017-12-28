cbuffer Constants
{
    float4x4 g_WorldViewProj;
};

struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float2 UV : TEX_COORD; 
};

PSInput main(uint VertId : SV_VertexID) 
{
    float4 Pos[] =
    {
        float4(-0.5, +0.5, 0.0, 1.0),
        float4(-0.5, -0.5, 0.0, 1.0),
        float4(+0.5, +0.5, 0.0, 1.0),
        float4(+0.5, -0.5, 0.0, 1.0)
    };
    float2 UV[] =
    {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 1.0),
        float2(1.0, 0.0)
    };

    PSInput ps; 
    ps.Pos = mul( Pos[VertId], g_WorldViewProj);
    ps.UV = UV[VertId];
    return ps;
}
