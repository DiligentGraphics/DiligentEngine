#include "AtmosphereShadersCommon.fxh"

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 16
#endif

cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

Texture3D<float3> g_tex3DPointwiseSctrRadiance;
SamplerState g_tex3DPointwiseSctrRadiance_sampler;

#include "LookUpTables.fxh"
#include "PrecomputeCommon.fxh"

RWTexture3D</*format = rgba32f*/float3> g_rwtex3DInsctrOrder;

// This shader computes in-scattering order for a given point and direction. It performs integration of the 
// light scattered at particular point along the ray, see eq. (11) in [Bruneton and Neyret 08].
[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void ComputeScatteringOrderCS(uint3 ThreadId  : SV_DispatchThreadID)
{
    // Get attributes for the current point
    float4 f4LUTCoords = LUTCoordsFromThreadID(ThreadId);
    float fAltitude, fCosViewZenithAngle, fCosSunZenithAngle, fCosSunViewAngle;
    InsctrLUTCoords2WorldParams(f4LUTCoords,
                                g_MediaParams.fEarthRadius,
                                g_MediaParams.fAtmBottomAltitude,
                                g_MediaParams.fAtmTopAltitude,
                                fAltitude,
                                fCosViewZenithAngle,
                                fCosSunZenithAngle,
                                fCosSunViewAngle);
    float3 f3EarthCentre = float3(0.0, -g_MediaParams.fEarthRadius, 0.0);
    float3 f3RayStart    = float3(0.0, fAltitude, 0.0);
    float3 f3ViewDir     = ComputeViewDir(fCosViewZenithAngle);
    float3 f3DirOnLight  = ComputeLightDir(f3ViewDir, fCosSunZenithAngle, fCosSunViewAngle);
    
    // Intersect the ray with the atmosphere boundaries
    float4 f4Isecs;
    GetRaySphereIntersection2( f3RayStart, f3ViewDir, f3EarthCentre, 
                               float2(g_MediaParams.fAtmBottomRadius, g_MediaParams.fAtmTopRadius), 
                               f4Isecs);
    float2 f2RayEarthIsecs  = f4Isecs.xy;
    float2 f2RayAtmTopIsecs = f4Isecs.zw;

    if(f2RayAtmTopIsecs.y <= 0.0)
    {
        // This is just a sanity check and should never happen
        // as the start point is always under the top of the 
        // atmosphere (look at InsctrLUTCoords2WorldParams())
        g_rwtex3DInsctrOrder[ThreadId] = float3(0.0, 0.0, 0.0); 
        return;
    }

    float fRayLength = f2RayAtmTopIsecs.y;
    if(f2RayEarthIsecs.x > 0.0)
        fRayLength = min(fRayLength, f2RayEarthIsecs.x);
    
    float3 f3RayEnd = f3RayStart + f3ViewDir * fRayLength;

    const int iNumSamples = 64;

    float4 f4UVWQ = float4(-1.0, -1.0, -1.0, -1.0);
    float3 f3PrevSctrRadiance = LookUpPrecomputedScattering(
        f3RayStart,
        f3ViewDir,
        f3EarthCentre,
        g_MediaParams.fEarthRadius,
        f3DirOnLight.xyz,
        g_MediaParams.fAtmBottomAltitude,
        g_MediaParams.fAtmTopAltitude,
        g_tex3DPointwiseSctrRadiance,
        g_tex3DPointwiseSctrRadiance_sampler,
        f4UVWQ); 
    float2 f2PrevParticleDensity = exp( -float2(fAltitude, fAltitude) * g_MediaParams.f4ParticleScaleHeight.zw );

    float2 f2NetParticleDensityFromCam = float2(0.0, 0.0);
    float3 f3Inscattering = float3(0.0, 0.0, 0.0);

    // We want to place more samples when the starting point is close to the surface,
    // but for high altitudes linear distribution works better.
    float fStartAltitude = length(f3RayStart - f3EarthCentre) - g_MediaParams.fEarthRadius;
    float pwr = lerp(2.0, 1.0, saturate((fStartAltitude - g_MediaParams.fAtmBottomAltitude) * g_MediaParams.fAtmAltitudeRangeInv));

    float fPrevSampleDist = 0.0;
    for (int iSample = 1; iSample <= iNumSamples; ++iSample)
    {
        // We need to place more samples closer to the start point and fewer samples farther away.
        // The ad-hoc power function does the job well.
        float r = pow(float(iSample) / float(iNumSamples), pwr);
        float3 f3Pos = lerp(f3RayStart, f3RayEnd, r);
        float fCurrSampleDist = fRayLength * r;
        float fStepLen = fCurrSampleDist - fPrevSampleDist;
        fPrevSampleDist = fCurrSampleDist;
        
        float fCurrHeight = length(f3Pos - f3EarthCentre) - g_MediaParams.fEarthRadius;
        float2 f2ParticleDensity = exp( -float2(fCurrHeight, fCurrHeight) * g_MediaParams.f4ParticleScaleHeight.zw );

        f2NetParticleDensityFromCam += (f2PrevParticleDensity + f2ParticleDensity) * (fStepLen / 2.0);
        f2PrevParticleDensity = f2ParticleDensity;
        
        // Get optical depth
        float3 f3RlghOpticalDepth = g_MediaParams.f4RayleighExtinctionCoeff.rgb * f2NetParticleDensityFromCam.x;
        float3 f3MieOpticalDepth  = g_MediaParams.f4MieExtinctionCoeff.rgb      * f2NetParticleDensityFromCam.y;
        
        // Compute extinction from the camera for the current integration point:
        float3 f3ExtinctionFromCam = exp( -(f3RlghOpticalDepth + f3MieOpticalDepth) );

        // Get attenuated scattered light radiance in the current point
        float4 f4UVWQ = float4(-1.0, -1.0, -1.0, -1.0);
        float3 f3SctrRadiance = f3ExtinctionFromCam *
            LookUpPrecomputedScattering(
                f3Pos,
                f3ViewDir,
                f3EarthCentre,
                g_MediaParams.fEarthRadius,
                f3DirOnLight.xyz,
                g_MediaParams.fAtmBottomAltitude,
                g_MediaParams.fAtmTopAltitude,
                g_tex3DPointwiseSctrRadiance,
                g_tex3DPointwiseSctrRadiance_sampler,
                f4UVWQ); 
        // Update in-scattering integral
        f3Inscattering += (f3SctrRadiance +  f3PrevSctrRadiance) * (fStepLen/2.0);
        f3PrevSctrRadiance = f3SctrRadiance;
    }

    g_rwtex3DInsctrOrder[ThreadId] = f3Inscattering;
}
