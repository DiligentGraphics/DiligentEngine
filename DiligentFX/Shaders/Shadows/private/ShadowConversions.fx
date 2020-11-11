
#include "FullScreenTriangleVSOutput.fxh"
#include "BasicStructures.fxh"
#include "Shadows.fxh"

struct ConversionAttribs
{
    int iCascade;
    float fHorzFilterRadius;
    float fVertFilterRadius;
    float fEVSMPositiveExponent;

    float fEVSMNegativeExponent;
    bool Is32BitEVSM;
};

cbuffer cbConversionAttribs
{
    ConversionAttribs g_Attribs;
}

Texture2DArray g_tex2DShadowMap;

float GetSampleWeight(int x, float FilterRadius)
{
    float fTexelMin = max(float(x),       min(0.5 - FilterRadius, 0.0));
    float fTexelMax = min(float(x) + 1.0, max(0.5 + FilterRadius, 1.0));
    return fTexelMax - fTexelMin;
}

float4 VSMHorzPS(FullScreenTriangleVSOutput VSOut) : SV_Target
{
    float2 f2Moments = float2(0.0, 0.0);
    int range = int(floor(g_Attribs.fHorzFilterRadius + 0.5));
    float fTotalWeight = 0.0;
    for (int i = -range; i <= +range; ++i)
    {
        float fWeight = GetSampleWeight(i, g_Attribs.fHorzFilterRadius);
        float fDepth = g_tex2DShadowMap.Load( int4( int(VSOut.f4PixelPos.x) + i, int(VSOut.f4PixelPos.y), g_Attribs.iCascade, 0) ).r;
        f2Moments += float2(fDepth, fDepth*fDepth) * fWeight;
        fTotalWeight += fWeight;
    }
    return float4(f2Moments / fTotalWeight, 0.0, 0.0);
}

float4 EVSMHorzPS(FullScreenTriangleVSOutput VSOut) : SV_Target
{
    float2 f2Exponents = GetEVSMExponents(g_Attribs.fEVSMPositiveExponent, g_Attribs.fEVSMNegativeExponent, g_Attribs.Is32BitEVSM);

    float4 f4Moments = float4(0.0, 0.0, 0.0, 0.0);
    int range = int(floor(g_Attribs.fHorzFilterRadius + 0.5));
    float fTotalWeight = 0.0;
    for (int i = -range; i <= +range; ++i)
    {
        float fWeight = GetSampleWeight(i, g_Attribs.fHorzFilterRadius);
        float fDepth = g_tex2DShadowMap.Load( int4( int(VSOut.f4PixelPos.x) + i, int(VSOut.f4PixelPos.y), g_Attribs.iCascade, 0) ).r;
        float2 f2EVSMDepth = WarpDepthEVSM(fDepth, f2Exponents);
        f4Moments += float4(f2EVSMDepth.x, f2EVSMDepth.x*f2EVSMDepth.x, f2EVSMDepth.y, f2EVSMDepth.y*f2EVSMDepth.y) * fWeight;
        fTotalWeight += fWeight;
    }
    return f4Moments / fTotalWeight;
}

float4 VertBlurPS(FullScreenTriangleVSOutput VSOut) : SV_Target
{
    float4 f4Moments = float4(0.0, 0.0, 0.0, 0.0);
    int range = int(floor(g_Attribs.fVertFilterRadius + 0.5));
    float fTotalWeight = 0.0;
    for (int i = -range; i <= +range; ++i)
    {
        float fWeight = GetSampleWeight(i, g_Attribs.fVertFilterRadius);
        f4Moments += g_tex2DShadowMap.Load( int4( int(VSOut.f4PixelPos.x), int(VSOut.f4PixelPos.y) + i, 0, 0) ) * fWeight;
        fTotalWeight += fWeight;
    }
    return f4Moments / fTotalWeight;
}
