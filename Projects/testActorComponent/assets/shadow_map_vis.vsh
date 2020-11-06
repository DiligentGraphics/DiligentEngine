#include "structures.fxh"

void main(in  uint    VertId : SV_VertexID,
          out ShadowMapVisPSInput PSIn)
{
    float2 Pos[4];
    Pos[0] = float2(-1.0, -1.0);
    Pos[1] = float2(-1.0, +1.0);
    Pos[2] = float2(+1.0, -1.0);
    Pos[3] = float2(+1.0, +1.0);

    float2 Center = float2(-0.6, -0.6);
    float2 Size   = float2(0.35, 0.35);

    PSIn.Pos = float4(Center + Size * Pos[VertId], 0.0, 1.0);
    PSIn.UV  = Pos[VertId].xy * F3NDC_XYZ_TO_UVD_SCALE.xy + float2(0.5, 0.5);
}
