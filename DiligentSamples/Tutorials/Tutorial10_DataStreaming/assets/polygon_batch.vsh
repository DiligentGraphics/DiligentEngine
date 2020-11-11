
struct VSInput
{
    float2 PolygonXY            : ATTRIB0;
    float4 QuadRotationAndScale : ATTRIB1;
    float2 QuadCenter           : ATTRIB2;
    float  TexArrInd            : ATTRIB3;
};

struct PSInput 
{ 
    float4 Pos     : SV_POSITION; 
    float2 UV      : TEX_COORD;
    float TexIndex : TEX_ARRAY_INDEX;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    float2 pos = VSIn.PolygonXY.xy;
    float2x2 mat = MatrixFromRows(VSIn.QuadRotationAndScale.xy, VSIn.QuadRotationAndScale.zw);
    pos = mul(pos, mat);
    pos += VSIn.QuadCenter.xy;
    PSIn.Pos = float4(pos, 0.0, 1.0);
    const float sqrt2 = 1.414213562373095;
    PSIn.UV = VSIn.PolygonXY * sqrt2 * 0.5 + 0.5;
    PSIn.TexIndex = VSIn.TexArrInd;
}
