
cbuffer PolygonAttribs
{
    float4 g_QuadRotationAndScale; // float2x2 doesn't work
    float4 g_QuadCenter;
};

struct VSInput
{
    float2 PolygonXY : ATTRIB0;
};

struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float2 UV  : TEX_COORD;
};

void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    float2 pos = VSIn.PolygonXY.xy;
    float2x2 mat = MatrixFromRows(g_QuadRotationAndScale.xy, g_QuadRotationAndScale.zw);
    pos = mul(pos, mat);
    pos += g_QuadCenter.xy;
    PSIn.Pos = float4(pos, 0.0, 1.0);
    const float sqrt2 = 1.414213562373095;
    PSIn.UV = VSIn.PolygonXY * sqrt2 * 0.5 + 0.5;
}
