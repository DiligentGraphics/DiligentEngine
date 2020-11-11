
#include "HostSharedTerrainStructs.fxh"
#include "TerrainShadersCommon.fxh"

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

void HemisphereZOnlyVS(in float3 f3PosWS : ATTRIB0,
                       out float4 f4PosPS : SV_Position)
{
    f4PosPS = mul( float4(f3PosWS,1.0), g_CameraAttribs.mViewProj);
}
