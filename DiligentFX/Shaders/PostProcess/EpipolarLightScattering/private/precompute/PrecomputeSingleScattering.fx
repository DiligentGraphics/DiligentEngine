#include "AtmosphereShadersCommon.fxh"

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 16
#endif

cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

Texture2D<float2> g_tex2DOccludedNetDensityToAtmTop;
SamplerState g_tex2DOccludedNetDensityToAtmTop_sampler;

#include "LookUpTables.fxh"
#include "ScatteringIntegrals.fxh"
#include "PrecomputeCommon.fxh"

RWTexture3D</*format = rgba16f*/float3> g_rwtex3DSingleScattering;


// This shader pre-computes the radiance of single scattering at a given point in given
// direction.
[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void PrecomputeSingleScatteringCS(uint3 ThreadId  : SV_DispatchThreadID)
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
                                fCosSunViewAngle );
    float3 f3EarthCentre = float3(0.0, -g_MediaParams.fEarthRadius, 0.0);
    float3 f3RayStart    = float3(0.0, fAltitude, 0.0);
    float3 f3ViewDir     = ComputeViewDir(fCosViewZenithAngle);
    float3 f3DirOnLight  = ComputeLightDir(f3ViewDir, fCosSunZenithAngle, fCosSunViewAngle);
  
    // Intersect view ray with the atmosphere boundaries
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
        g_rwtex3DSingleScattering[ThreadId] = float3(0.0, 0.0, 0.0);
        return;
    }

    // Set the ray length to the distance to the top of the atmosphere
    float fRayLength = f2RayAtmTopIsecs.y;
    // If ray hits Earth, limit the length by the distance to the surface
    if(f2RayEarthIsecs.x > 0.0)
        fRayLength = min(fRayLength, f2RayEarthIsecs.x);
    
    float3 f3RayEnd = f3RayStart + f3ViewDir * fRayLength;

    // Integrate single-scattering
    float3 f3Extinction, f3Inscattering;
    IntegrateUnshadowedInscattering(f3RayStart, 
                                    f3RayEnd,
                                    f3ViewDir,
                                    f3EarthCentre,
                                    g_MediaParams.fEarthRadius,
                                    g_MediaParams.fAtmBottomAltitude,
                                    g_MediaParams.fAtmAltitudeRangeInv,
                                    g_MediaParams.f4ParticleScaleHeight,
                                    f3DirOnLight.xyz,
                                    100u,
                                    f3Inscattering,
                                    f3Extinction);

    g_rwtex3DSingleScattering[ThreadId] = f3Inscattering;
}
