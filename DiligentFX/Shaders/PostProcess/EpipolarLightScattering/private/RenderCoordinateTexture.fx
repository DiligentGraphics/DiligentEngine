#include "AtmosphereShadersCommon.fxh"

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
};

Texture2D<float> g_tex2DCamSpaceZ;
SamplerState     g_tex2DCamSpaceZ_sampler;

Texture2D<float4> g_tex2DSliceEndPoints;

void GenerateCoordinateTexturePS(FullScreenTriangleVSOutput VSOut, 
                                 // IMPORTANT: non-system generated pixel shader input
                                 // arguments must have the exact same name as vertex shader 
                                 // outputs and must go in the same order.

                                 out float2 f2XY      : SV_Target0,
                                 out float fCamSpaceZ : SV_Target1)

{
    float4 f4SliceEndPoints = g_tex2DSliceEndPoints.Load( int3(VSOut.f4PixelPos.y,0,0) );
    
    // If slice entry point is outside [-1,1]x[-1,1] area, the slice is completely invisible
    // and we can skip it from further processing.
    // Note that slice exit point can lie outside the screen, if sample locations are optimized
    if (!IsValidScreenLocation(f4SliceEndPoints.xy, g_PPAttribs.f4ScreenResolution))
    {
        // Discard invalid slices
        // Such slices will not be marked in the stencil and as a result will always be skipped
        discard;
    }

    float2 f2UV = NormalizedDeviceXYToTexUV(VSOut.f2NormalizedXY);

    // Note that due to the rasterization rules, UV coordinates are biased by 0.5 texel size.
    //
    //      0.5     1.5     2.5     3.5
    //   |   X   |   X   |   X   |   X   |     ....       
    //   0       1       2       3       4   f2UV * f2TexDim
    //   X - locations where rasterization happens
    //
    // We need remove this offset:
    float fSamplePosOnEpipolarLine = f2UV.x - 0.5 / float(g_PPAttribs.uiMaxSamplesInSlice);
    // fSamplePosOnEpipolarLine is now in the range [0, 1 - 1/MAX_SAMPLES_IN_SLICE]
    // We need to rescale it to be in [0, 1]
    fSamplePosOnEpipolarLine *= float(g_PPAttribs.uiMaxSamplesInSlice) / (float(g_PPAttribs.uiMaxSamplesInSlice)-1.0);
    fSamplePosOnEpipolarLine = saturate(fSamplePosOnEpipolarLine);

    // Compute interpolated position between entry and exit points:
    f2XY = lerp(f4SliceEndPoints.xy, f4SliceEndPoints.zw, fSamplePosOnEpipolarLine);
    if (!IsValidScreenLocation(f2XY, g_PPAttribs.f4ScreenResolution))
    {
        // Discard pixels that fall behind the screen
        // This can happen if slice exit point was optimized
        discard;
    }

    // Compute camera space z for current location
    float2 f2ScreenSpaceUV = NormalizedDeviceXYToTexUV( f2XY );
    fCamSpaceZ = g_tex2DCamSpaceZ.SampleLevel(g_tex2DCamSpaceZ_sampler, f2ScreenSpaceUV, 0);
}
