
struct VSInput
{
    uint   VertID               : SV_VertexID;
    float4 QuadRotationAndScale : ATTRIB0;
    float2 QuadCenter           : ATTRIB1;
    float  TexArrInd            : ATTRIB2;
};

struct PSInput 
{ 
    float4 Pos     : SV_POSITION; 
    float2 uv      : TEX_COORD;
    float TexIndex : TEX_ARRAY_INDEX;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    float4 pos_uv[4];
    pos_uv[0] = float4(-1.0,+1.0, 0.0,0.0);
    pos_uv[1] = float4(-1.0,-1.0, 0.0,1.0);
    pos_uv[2] = float4(+1.0,+1.0, 1.0,0.0);
    pos_uv[3] = float4(+1.0,-1.0, 1.0,1.0);

    float2 pos = pos_uv[VSIn.VertID].xy;
    float2x2 mat = MatrixFromRows(VSIn.QuadRotationAndScale.xy, VSIn.QuadRotationAndScale.zw);
    pos = mul(pos, mat);
    pos += VSIn.QuadCenter.xy;
    PSIn.Pos = float4(pos, 0.0, 1.0);
    PSIn.uv = pos_uv[VSIn.VertID].zw;
    PSIn.TexIndex = VSIn.TexArrInd;
}
