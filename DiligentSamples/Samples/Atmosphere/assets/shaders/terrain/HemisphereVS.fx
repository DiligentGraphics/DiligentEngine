
#include "HostSharedTerrainStructs.fxh"
#include "ToneMappingStructures.fxh"
#include "EpipolarLightScatteringStructures.fxh"
#include "EpipolarLightScatteringFunctions.fxh"
#include "TerrainShadersCommon.fxh"

cbuffer cbTerrainAttribs
{
    TerrainAttribs g_TerrainAttribs;
};

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

cbuffer cbLightAttribs
{
    LightAttribs g_LightAttribs;
};

cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

Texture2D< float2 > g_tex2DOccludedNetDensityToAtmTop;
SamplerState        g_tex2DOccludedNetDensityToAtmTop_sampler;

Texture2D< float3 > g_tex2DAmbientSkylight;
SamplerState        g_tex2DAmbientSkylight_sampler;

void HemisphereVS(in float3 f3PosWS : ATTRIB0,
                  in float2 f2MaskUV0 : ATTRIB1,
                  out float4 f4PosPS : SV_Position,
                  out HemisphereVSOutput VSOut
                  // IMPORTANT: non-system generated pixel shader input
                  // arguments must have the exact same name as vertex shader 
                  // outputs and must go in the same order.
                 )
{
    VSOut.TileTexUV = f3PosWS.xz;

    f4PosPS = mul( float4(f3PosWS,1.0), g_CameraAttribs.mViewProj);
    
    float4 ShadowMapSpacePos = mul( float4(f3PosWS,1.0), g_LightAttribs.ShadowAttribs.mWorldToLightView);
    VSOut.f3PosInLightViewSpace = ShadowMapSpacePos.xyz / ShadowMapSpacePos.w;
    VSOut.f2MaskUV0 = f2MaskUV0;
    float3 f3Normal = normalize(f3PosWS - float3(0.0, -g_TerrainAttribs.m_fEarthRadius, 0.0));
    VSOut.f3Normal = f3Normal;
    VSOut.f3Tangent = normalize( cross(f3Normal, float3(0.0,0.0,1.0)) );
    VSOut.f3Bitangent = normalize( cross(VSOut.f3Tangent, f3Normal) );

    GetSunLightExtinctionAndSkyLight(f3PosWS,
        float3(0.0, -g_MediaParams.fEarthRadius, 0.0),
        g_LightAttribs.f4Direction.xyz,
        g_MediaParams,
        g_tex2DOccludedNetDensityToAtmTop,
        g_tex2DOccludedNetDensityToAtmTop_sampler,
        g_tex2DAmbientSkylight,
        g_tex2DAmbientSkylight_sampler,
        VSOut.f3SunLightExtinction,
        VSOut.f3AmbientSkyLight);
}
