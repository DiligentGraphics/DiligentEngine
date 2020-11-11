#include "FullScreenTriangleVSOutput.fxh"

void FullScreenTriangleVS(in uint VertexId : SV_VertexID,
                          in uint InstID   : SV_InstanceID,
                          out FullScreenTriangleVSOutput VSOut)
{
    float2 PosXY[3];
    PosXY[0] = float2(-1.0, -1.0);
    PosXY[1] = float2(-1.0, +3.0);
    PosXY[2] = float2(+3.0, -1.0);

    float2 f2XY = PosXY[VertexId];
    VSOut.f2NormalizedXY = f2XY;
    VSOut.fInstID = float( InstID );

    // Write 0 to the depth buffer
    // NDC_MIN_Z ==  0 in DX
    // NDC_MIN_Z == -1 in GL
    float z = NDC_MIN_Z;
    VSOut.f4PixelPos = float4(f2XY, z, 1.0);
}
