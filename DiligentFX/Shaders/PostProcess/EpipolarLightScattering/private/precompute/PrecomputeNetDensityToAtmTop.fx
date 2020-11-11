#include "AtmosphereShadersCommon.fxh"


cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

float2 IntegrateParticleDensity(float3 f3Start, 
                                float3 f3End,
                                float3 f3EarthCentre,
                                int    iNumSteps )
{
    float3 f3Step = (f3End - f3Start) / float(iNumSteps);
    float fStepLen = length(f3Step);
        
    float fStartHeightAboveSurface = abs( length(f3Start - f3EarthCentre) - g_MediaParams.fEarthRadius );
    float2 f2PrevParticleDensity = exp( -fStartHeightAboveSurface * g_MediaParams.f4ParticleScaleHeight.zw );

    float2 f2ParticleNetDensity = float2(0.0, 0.0);
    for (int iStepNum = 1; iStepNum <= iNumSteps; ++iStepNum)
    {
        float3 f3CurrPos = f3Start + f3Step * float(iStepNum);
        float fHeightAboveSurface = abs( length(f3CurrPos - f3EarthCentre) - g_MediaParams.fEarthRadius );
        float2 f2ParticleDensity = exp( -fHeightAboveSurface * g_MediaParams.f4ParticleScaleHeight.zw );
        f2ParticleNetDensity += (f2ParticleDensity + f2PrevParticleDensity) * fStepLen / 2.0;
        f2PrevParticleDensity = f2ParticleDensity;
    }
    return f2ParticleNetDensity;
}

float2 IntegrateParticleDensityAlongRay(float3     f3Pos, 
                                        float3     f3RayDir,
                                        float3     f3EarthCentre, 
                                        const int  iNumSteps,
                                        const bool bOccludeByEarth)
{
    if( bOccludeByEarth )
    {
        // If the ray hits the bottom atmosphere boundary, return huge optical depth
        float2 f2RayEarthIsecs; 
        GetRaySphereIntersection(f3Pos, f3RayDir, f3EarthCentre, g_MediaParams.fAtmBottomRadius, f2RayEarthIsecs);
        if( f2RayEarthIsecs.x > 0.0 )
            return float2(1e+20, 1e+20);
    }

    // Get intersection with the top of the atmosphere (the start point must always be under the top of it)
    //      
    //                     /
    //                .   /  . 
    //      .  '         /\         '  .
    //                  /  f2RayAtmTopIsecs.y > 0
    //                 *
    //                   f2RayAtmTopIsecs.x < 0
    //                  /
    //      
    float2 f2RayAtmTopIsecs;
    GetRaySphereIntersection(f3Pos, f3RayDir, f3EarthCentre, g_MediaParams.fAtmTopRadius, f2RayAtmTopIsecs);
    float fIntegrationDist = f2RayAtmTopIsecs.y;

    float3 f3RayEnd = f3Pos + f3RayDir * fIntegrationDist;

    return IntegrateParticleDensity(f3Pos, f3RayEnd, f3EarthCentre, iNumSteps);
}

void PrecomputeNetDensityToAtmTopPS( FullScreenTriangleVSOutput VSOut,
                                     out float2 f2Density : SV_Target0 )
{
    float2 f2UV = NormalizedDeviceXYToTexUV(VSOut.f2NormalizedXY);
    // Do not allow start point be at the Earth surface and on the top of the atmosphere
    float fStartHeight = clamp( lerp(g_MediaParams.fAtmBottomAltitude, g_MediaParams.fAtmTopAltitude, f2UV.x), 10.0, g_MediaParams.fAtmTopAltitude-10.0 );

    float fCosTheta = f2UV.y * 2.0 - 1.0;
    float fSinTheta = sqrt( saturate(1.0 - fCosTheta*fCosTheta) );
    float3 f3RayStart = float3(0.0, 0.0, fStartHeight);
    float3 f3RayDir = float3(fSinTheta, 0.0, fCosTheta);
    
    float3 f3EarthCentre = float3(0.0, 0.0, -g_MediaParams.fEarthRadius);

    const int iNumSteps = 200;
    f2Density = IntegrateParticleDensityAlongRay(f3RayStart, f3RayDir, f3EarthCentre, iNumSteps, true);
}
