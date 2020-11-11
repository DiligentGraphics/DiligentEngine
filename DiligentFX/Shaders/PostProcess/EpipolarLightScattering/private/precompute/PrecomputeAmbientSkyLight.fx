#include "AtmosphereShadersCommon.fxh"

cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

#include "LookUpTables.fxh"

Texture2D<float3> g_tex2DSphereRandomSampling;

Texture3D<float3> g_tex3DMultipleSctrLUT;
SamplerState g_tex3DMultipleSctrLUT_sampler;


void PrecomputeAmbientSkyLightPS(FullScreenTriangleVSOutput VSOut,
                                 // IMPORTANT: non-system generated pixel shader input
                                 // arguments must have the exact same name as vertex shader 
                                 // outputs and must go in the same order.

                                 out float4 f4SkyLight : SV_Target)
{
    float fU = NormalizedDeviceXYToTexUV(VSOut.f2NormalizedXY).x;
    float3 f3RayStart     = float3(0.0, 20.0, 0.0);
    float3 f3EarthCentre  = float3(0.0, -g_MediaParams.fEarthRadius, 0.0);
    float fCosZenithAngle = clamp(fU * 2.0 - 1.0, -1.0, +1.0);
    float3 f3DirOnLight   = float3(sqrt(saturate(1.0 - fCosZenithAngle*fCosZenithAngle)), fCosZenithAngle, 0.0);
    f4SkyLight = float4(0.0, 0.0, 0.0, 0.0);
    // Go through a number of random directions on the sphere
    for(int iSample = 0; iSample < NUM_RANDOM_SPHERE_SAMPLES; ++iSample)
    {
        // Get random direction
        float3 f3RandomDir = normalize( g_tex2DSphereRandomSampling.Load(int3(iSample,0,0)) );
        // Reflect directions from the lower hemisphere
        f3RandomDir.y = abs(f3RandomDir.y);
        // Get multiple scattered light radiance when looking in direction f3RandomDir (the light thus goes in direction -f3RandomDir)
        float4 f4UVWQ = float4(-1.0, -1.0, -1.0, -1.0);
        float3 f3Sctr = LookUpPrecomputedScattering(
            f3RayStart,
            f3RandomDir,
            f3EarthCentre,
            g_MediaParams.fEarthRadius,
            f3DirOnLight.xyz,
            g_MediaParams.fAtmBottomAltitude,
            g_MediaParams.fAtmTopAltitude,
            g_tex3DMultipleSctrLUT,
            g_tex3DMultipleSctrLUT_sampler,
            f4UVWQ); 
        // Accumulate ambient irradiance through the horizontal plane
        f4SkyLight.rgb += f3Sctr * dot(f3RandomDir, float3(0.0, 1.0, 0.0));
    }
    // Each sample covers 2 * PI / NUM_RANDOM_SPHERE_SAMPLES solid angle (integration is performed over
    // upper hemisphere)
    f4SkyLight.rgb *= 2.0 * PI / float(NUM_RANDOM_SPHERE_SAMPLES);
}
