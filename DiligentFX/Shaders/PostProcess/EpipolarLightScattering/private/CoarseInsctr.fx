// CoarseInsctr.fx:
// Renders coarse unshadowed inscattering for EVERY epipolar sample and computes extinction.
// Coarse inscattering is used to refine sampling, while extinction is then transformed to 
// screen space, if extinction evaluation mode is EXTINCTION_EVAL_MODE_EPIPOLAR

#include "BasicStructures.fxh"
#include "AtmosphereShadersCommon.fxh"

cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

cbuffer cbLightParams
{
    LightAttribs g_LightAttribs;
}

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
};

Texture2D<float2> g_tex2DOccludedNetDensityToAtmTop;
SamplerState      g_tex2DOccludedNetDensityToAtmTop_sampler;

Texture2D<float>  g_tex2DEpipolarCamSpaceZ;

Texture2D<float2> g_tex2DCoordinates;

Texture3D<float3> g_tex3DSingleSctrLUT;
SamplerState      g_tex3DSingleSctrLUT_sampler;

Texture3D<float3> g_tex3DHighOrderSctrLUT;
SamplerState      g_tex3DHighOrderSctrLUT_sampler;

Texture3D<float3> g_tex3DMultipleSctrLUT;
SamplerState      g_tex3DMultipleSctrLUT_sampler;

#include "LookUpTables.fxh"
#include "ScatteringIntegrals.fxh"
#include "Extinction.fxh"
#include "UnshadowedScattering.fxh"

void ShaderFunctionInternal(in float4  f4Pos,
                            out float3 f3Inscattering, 
                            out float3 f3Extinction)
{
    // Compute unshadowed inscattering from the camera to the ray end point using few steps
    float fCamSpaceZ =  g_tex2DEpipolarCamSpaceZ.Load( uint3(f4Pos.xy, 0) );
    float2 f2SampleLocation = g_tex2DCoordinates.Load( uint3(f4Pos.xy, 0) );

    ComputeUnshadowedInscattering(f2SampleLocation, fCamSpaceZ, 
                                  7u, // Use hard-coded constant here so that compiler can optimize the code
                                      // more efficiently
                                  g_PPAttribs.f4EarthCenter.xyz,
                                  f3Inscattering, f3Extinction);
    f3Inscattering *= g_LightAttribs.f4Intensity.rgb;
}

// Render inscattering only
void RenderCoarseUnshadowedInsctrPS(FullScreenTriangleVSOutput VSOut, 
                                    // IMPORTANT: non-system generated pixel shader input
                                    // arguments must have the exact same name as vertex shader 
                                    // outputs and must go in the same order.
                                    // Moreover, even if the shader is not using the argument,
                                    // it still must be declared.

                                    out float4 f4Inscattering : SV_Target0) 
{
    float3 f3Extinction = float3(1.0, 1.0, 1.0);
    ShaderFunctionInternal(VSOut.f4PixelPos, f4Inscattering.rgb, f3Extinction );
    f4Inscattering.a = 1.0;
}

// Render inscattering and extinction
void RenderCoarseUnshadowedInsctrAndExtinctionPS(FullScreenTriangleVSOutput VSOut,
                                                 out float4 f4Inscattering : SV_Target0,
                                                 out float4 f4Extinction   : SV_Target1) 
{
    ShaderFunctionInternal(VSOut.f4PixelPos, f4Inscattering.rgb, f4Extinction.rgb );
    f4Inscattering.a = 0.0;
    f4Extinction.a = 0.0;
}
