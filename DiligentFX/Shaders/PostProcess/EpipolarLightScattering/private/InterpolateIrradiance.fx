// InterpolateIrradiance.fx
// Interpolates irradiance from ray marching samples to all epipolar samples

#include "AtmosphereShadersCommon.fxh"

Texture2D<uint2>  g_tex2DInterpolationSource;
Texture2D<float3> g_tex2DInitialInsctrIrradiance;

void InterpolateIrradiancePS(FullScreenTriangleVSOutput VSOut,
                             // IMPORTANT: non-system generated pixel shader input
                             // arguments must have the exact same name as vertex shader 
                             // outputs and must go in the same order.
                             // Moreover, even if the shader is not using the argument,
                             // it still must be declared.

                             out float4 f4InterpolatedIrradiance : SV_Target)
{
    int iSampleInd = int(VSOut.f4PixelPos.x);
    int iSliceInd = int(VSOut.f4PixelPos.y);
    // Get interpolation sources
    uint2 ui2InterpolationSources = g_tex2DInterpolationSource.Load( int3(iSampleInd, iSliceInd, 0) );
    float fInterpolationPos = float(iSampleInd - int(ui2InterpolationSources.x)) / float( max(ui2InterpolationSources.y - ui2InterpolationSources.x, 1u) );

    float3 f3SrcInsctr0 = g_tex2DInitialInsctrIrradiance.Load( int3(ui2InterpolationSources.x, iSliceInd, 0) );
    float3 f3SrcInsctr1 = g_tex2DInitialInsctrIrradiance.Load( int3(ui2InterpolationSources.y, iSliceInd, 0));

    // Ray marching samples are interpolated from themselves
    f4InterpolatedIrradiance.rgb = lerp(f3SrcInsctr0, f3SrcInsctr1, fInterpolationPos);
    f4InterpolatedIrradiance.a = 1.0;
}
