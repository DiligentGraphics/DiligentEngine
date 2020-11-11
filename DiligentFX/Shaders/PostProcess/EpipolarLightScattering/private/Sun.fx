#include "BasicStructures.fxh"
#include "AtmosphereShadersCommon.fxh"

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
}

#define fSunAngularRadius (32.0/2.0 / 60.0 * ((2.0 * PI)/180.0)) // Sun angular DIAMETER is 32 arc minutes
#define fTanSunAngularRadius tan(fSunAngularRadius)

struct SunVSOutput
{
    float2 f2NormalizedXY : NORMALIZED_XY; // Normalized device XY coordinates [-1,1]x[-1,1]
};

void SunVS(in uint VertexId : SV_VertexID,
           out SunVSOutput VSOut, 
           // IMPORTANT: non-system generated pixel shader input
           // arguments must have the exact same name as vertex shader 
           // outputs and must go in the same order.
           // Moreover, even if the shader is not using the argument,
           // it still must be declared.

           out float4 f4Pos : SV_Position)
{
    float2 fCotanHalfFOV = float2( MATRIX_ELEMENT(g_CameraAttribs.mProj, 0, 0), MATRIX_ELEMENT(g_CameraAttribs.mProj, 1, 1) );
    float2 f2SunScreenPos = g_PPAttribs.f4LightScreenPos.xy;
    float2 f2SunScreenSize = fTanSunAngularRadius * fCotanHalfFOV;
    float4 MinMaxUV = f2SunScreenPos.xyxy + float4(-1.0, -1.0, 1.0, 1.0) * f2SunScreenSize.xyxy;
 
    float2 Verts[4];
    Verts[0] = MinMaxUV.xy;
    Verts[1] = MinMaxUV.xw;
    Verts[2] = MinMaxUV.zy;
    Verts[3] = MinMaxUV.zw;

    VSOut.f2NormalizedXY = Verts[VertexId];
    f4Pos = float4(Verts[VertexId], 1.0, 1.0);
}

void SunPS(SunVSOutput VSOut,
           out float4 f4Color : SV_Target)
{
    float2 fCotanHalfFOV = float2( MATRIX_ELEMENT(g_CameraAttribs.mProj, 0, 0), MATRIX_ELEMENT(g_CameraAttribs.mProj, 1, 1) );
    float2 f2SunScreenSize = fTanSunAngularRadius * fCotanHalfFOV;
    float2 f2dXY = (VSOut.f2NormalizedXY - g_PPAttribs.f4LightScreenPos.xy) / f2SunScreenSize;
    f4Color.rgb = sqrt(saturate(1.0 - dot(f2dXY, f2dXY))) * float3(1.0, 1.0, 1.0);
    f4Color.a = 1.0;
}
