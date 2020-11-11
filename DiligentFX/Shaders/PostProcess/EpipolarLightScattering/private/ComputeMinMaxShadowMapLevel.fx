#include "AtmosphereShadersCommon.fxh"

Texture2D<float2> g_tex2DMinMaxLightSpaceDepth;

cbuffer cbMiscDynamicParams
{
    MiscDynamicParams g_MiscParams;
}

// 1D min max mip map is arranged as follows:
//
//    g_MiscParams.ui4SrcDstMinMaxLevelOffset.x
//     |
//     |      g_MiscParams.ui4SrcDstMinMaxLevelOffset.z
//     |_______|____ __
//     |       |    |  |
//     |       |    |  |
//     |       |    |  |
//     |       |    |  |
//     |_______|____|__|
//     |<----->|<-->|
//         |     |
//         |    uiMinMaxShadowMapResolution/
//      uiMinMaxShadowMapResolution/2
//                         
void ComputeMinMaxShadowMapLevelPS(in FullScreenTriangleVSOutput VSOut,
                                   out float2 f2MinMaxDepth : SV_Target)
{
    uint2 uiDstSampleInd = uint2(VSOut.f4PixelPos.xy);
    uint2 uiSrcSample0Ind = uint2(g_MiscParams.ui4SrcDstMinMaxLevelOffset.x + (uiDstSampleInd.x - g_MiscParams.ui4SrcDstMinMaxLevelOffset.z)*2u, uiDstSampleInd.y);
    uint2 uiSrcSample1Ind = uiSrcSample0Ind + uint2(1,0);
    float2 f2MinMaxDepth0 = g_tex2DMinMaxLightSpaceDepth.Load( int3(uiSrcSample0Ind,0) ).xy;
    float2 f2MinMaxDepth1 = g_tex2DMinMaxLightSpaceDepth.Load( int3(uiSrcSample1Ind,0) ).xy;

    f2MinMaxDepth.x = min(f2MinMaxDepth0.x, f2MinMaxDepth1.x);
    f2MinMaxDepth.y = max(f2MinMaxDepth0.y, f2MinMaxDepth1.y);
}
