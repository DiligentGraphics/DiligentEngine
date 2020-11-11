#include "AtmosphereShadersCommon.fxh"

#if LIGHT_ADAPTATION
cbuffer cbMiscDynamicParams
{
    MiscDynamicParams g_MiscParams;
}
#endif

Texture2D<float2>  g_tex2DLowResLuminance;

void UpdateAverageLuminancePS( FullScreenTriangleVSOutput VSOut, 
                               // We must declare vertex shader output even though we 
                               // do not use it, because otherwise the shader will not
                               // run on NVidia GLES
                               out float4 f4Luminance : SV_Target)
{
#if LIGHT_ADAPTATION
    const float fAdaptationRate = 1.0;
    float fNewLuminanceWeight = 1.0 - exp( - fAdaptationRate * g_MiscParams.fElapsedTime );
#else
    float fNewLuminanceWeight = 1.0;
#endif
    float2 LogLum_W = g_tex2DLowResLuminance.Load( int3(0,0,LOW_RES_LUMINANCE_MIPS-1) );
    float LogLum = LogLum_W.x / max(LogLum_W.y, 1e-6);
    fNewLuminanceWeight *= saturate(LogLum_W.y / 1e-3);

    f4Luminance = float4( exp(LogLum), 0.0, 0.0, fNewLuminanceWeight );
}
