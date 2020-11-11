#include "BasicStructures.fxh"

#ifndef SHADOW_PASS
#   define SHADOW_PASS 0
#endif

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
};

#if !SHADOW_PASS
cbuffer cbLightAttribs
{
    LightAttribs g_LightAttribs;
};
#endif

struct VSInput
{
    float3 Position  : ATTRIB0;
    float3 Normal    : ATTRIB1;
    float2 TexCoord  : ATTRIB2;
};

struct VSOutput
{
    float4 PositionPS 	        : SV_Position;
    float3 PosInLightViewSpace 	: LIGHT_SPACE_POS;
    float3 NormalWS 	        : NORMALWS;
    float2 TexCoord 	        : TEXCOORD;
};

void MeshVS(in  VSInput  VSIn,
            out VSOutput VSOut)
{
#if !SHADOW_PASS
    float4 LightSpacePos = mul( float4(VSIn.Position, 1.0), g_LightAttribs.ShadowAttribs.mWorldToLightView);
    VSOut.PosInLightViewSpace = LightSpacePos.xyz / LightSpacePos.w;
#else
    VSOut.PosInLightViewSpace = float3(0.0, 0.0, 0.0);
#endif

    VSOut.PositionPS   = mul(float4(VSIn.Position, 1.0), g_CameraAttribs.mViewProj);
    VSOut.NormalWS     = VSIn.Normal;
    VSOut.TexCoord     = VSIn.TexCoord;
}
