#include "AtmosphereShadersCommon.fxh"

cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

Texture2D<float2> g_tex2DOccludedNetDensityToAtmTop;
SamplerState g_tex2DOccludedNetDensityToAtmTop_sampler;

#include "LookUpTables.fxh"
#include "PrecomputeCommon.fxh"
#include "ScatteringIntegrals.fxh"

Texture3D<float3> g_tex3DPreviousSctrOrder;
SamplerState g_tex3DPreviousSctrOrder_sampler;

Texture2D<float3> g_tex2DSphereRandomSampling;

RWTexture3D</*format = rgba32f*/float3> g_rwtex3DSctrRadiance;

// This shader pre-computes the radiance of light scattered at a given point in given
// direction. It multiplies the previous order in-scattered light with the phase function 
// for each type of particles and integrates the result over the whole set of directions,
// see eq. (7) in [Bruneton and Neyret 08].
[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void ComputeSctrRadianceCS(uint3 ThreadId  : SV_DispatchThreadID)
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
    
    // Compute particle density scale factor
    float2 f2ParticleDensity = exp( -float2(fAltitude, fAltitude) * g_MediaParams.f4ParticleScaleHeight.zw );
    
    float3 f3SctrRadiance = float3(0.0, 0.0, 0.0);
    // Go through a number of samples randomly distributed over the sphere
    for(int iSample = 0; iSample < NUM_RANDOM_SPHERE_SAMPLES; ++iSample)
    {
        // Get random direction
        float3 f3RandomDir = normalize( g_tex2DSphereRandomSampling.Load(int3(iSample,0,0)) );
        // Get the previous order in-scattered light when looking in direction f3RandomDir (the light thus goes in direction -f3RandomDir)
        float4 f4UVWQ = float4(-1.0, -1.0, -1.0, -1.0);
        float3 f3PrevOrderSctr = LookUpPrecomputedScattering(
            f3RayStart,
            f3RandomDir,
            f3EarthCentre,
            g_MediaParams.fEarthRadius,
            f3DirOnLight.xyz,
            g_MediaParams.fAtmBottomAltitude,
            g_MediaParams.fAtmTopAltitude,
            g_tex3DPreviousSctrOrder,
            g_tex3DPreviousSctrOrder_sampler,
            f4UVWQ); 
        
        // Apply phase functions for each type of particles
        // Note that total scattering coefficients are baked into the angular scattering coeffs
        float3 f3DRlghInsctr = f2ParticleDensity.x * f3PrevOrderSctr;
        float3 f3DMieInsctr  = f2ParticleDensity.y * f3PrevOrderSctr;
        float fCosTheta = dot(f3ViewDir, f3RandomDir);
        ApplyPhaseFunctions(f3DRlghInsctr, f3DMieInsctr, fCosTheta);

        f3SctrRadiance += f3DRlghInsctr + f3DMieInsctr;
    }
    // Since we tested N random samples, each sample covered 4*Pi / N solid angle
    // Note that our phase function is normalized to 1 over the sphere. For instance,
    // uniform phase function would be p(theta) = 1 / (4*Pi).
    // Notice that for uniform intensity I if we get N samples, we must obtain exactly I after
    // numeric integration
    f3SctrRadiance *= 4.0*PI / float(NUM_RANDOM_SPHERE_SAMPLES);

    g_rwtex3DSctrRadiance[ThreadId] = f3SctrRadiance;
}

