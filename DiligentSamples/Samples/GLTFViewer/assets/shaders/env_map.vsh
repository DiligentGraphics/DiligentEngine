
void main(in  uint   VertexId : SV_VertexID,
          out float4 Pos      : SV_Position,
          out float4 ClipPos  : CLIP_POS)
{
    float2 PosXY[3];
    PosXY[0] = float2(-1.0, -1.0);
    PosXY[1] = float2(-1.0, +3.0);
    PosXY[2] = float2(+3.0, -1.0);

    float2 f2XY = PosXY[VertexId];
    Pos = float4(f2XY, 1.0, 1.0);
    ClipPos = Pos;
}
