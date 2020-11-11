
// Returns net particle density to the atmosphere top
//
//                    | Zenith   .'
//                    | angle  .'
//                    |      .'
//                    |    .'
//                    |  .'
//           _ _ _ _ _|.'
//            A       |
//  fAltitude |       |
//           _V_ _ _ _|
//              .  '  '  '  .
//           .'               '.
//          '                   '
float2 GetNetParticleDensity(in float fAltitude,            // Altitude (height above the sea level) of the starting point
                             in float fCosZenithAngle,      // Cosine of the zenith angle
                             in float fAtmBottomAltitude,   // Altitude of the bottom atmosphere boundary
                             in float fAtmAltitudeRangeInv  // 1 / (fAtmTopAltitude - fAtmBottomAltitude)
                             )
{
    float fNormalizedAltitude = (fAltitude - fAtmBottomAltitude) * fAtmAltitudeRangeInv;
    return g_tex2DOccludedNetDensityToAtmTop.SampleLevel(g_tex2DOccludedNetDensityToAtmTop_sampler, float2(fNormalizedAltitude, fCosZenithAngle*0.5+0.5), 0);
}

void ApplyPhaseFunctions(inout float3 f3RayleighInscattering,
                         inout float3 f3MieInscattering,
                         in float cosTheta)
{
    f3RayleighInscattering *= g_MediaParams.f4AngularRayleighSctrCoeff.rgb * (1.0 + cosTheta*cosTheta);
    
    // Apply Cornette-Shanks phase function (see Nishita et al. 93):
    // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)
    // f4CS_g = ( 3*(1-g^2) / (2*(2+g^2)), 1+g^2, -2g, 1 )
    float fDenom = rsqrt( dot(g_MediaParams.f4CS_g.yz, float2(1.0, cosTheta)) ); // 1 / (1 + g^2 - 2g*cos(theta))^(1/2)
    float fCornettePhaseFunc = g_MediaParams.f4CS_g.x * (fDenom*fDenom*fDenom) * (1.0 + cosTheta*cosTheta);
    f3MieInscattering *= g_MediaParams.f4AngularMieSctrCoeff.rgb * fCornettePhaseFunc;
}

// This function computes atmospheric properties in the given point
void GetAtmosphereProperties(in float3  f3Pos,
                             in float3  f3EarthCentre,
                             in float   fEarthRadius,
                             in float   fAtmBottomAltitude,
                             in float   fAtmAltitudeRangeInv,
							 in float4  f4ParticleScaleHeight,
                             in float3  f3DirOnLight,
                             out float2 f2ParticleDensity,
                             out float2 f2NetParticleDensityToAtmTop)
{
    // Calculate the point height above the SPHERICAL Earth surface:
    float3 f3EarthCentreToPointDir = f3Pos - f3EarthCentre;
    float fDistToEarthCentre = length(f3EarthCentreToPointDir);
    f3EarthCentreToPointDir /= fDistToEarthCentre;
    float fHeightAboveSurface = fDistToEarthCentre - fEarthRadius;

    f2ParticleDensity = exp( -fHeightAboveSurface * f4ParticleScaleHeight.zw );

    // Get net particle density from the integration point to the top of the atmosphere:
    float fCosSunZenithAngleForCurrPoint = dot( f3EarthCentreToPointDir, f3DirOnLight );
    f2NetParticleDensityToAtmTop = GetNetParticleDensity(fHeightAboveSurface, fCosSunZenithAngleForCurrPoint, fAtmBottomAltitude, fAtmAltitudeRangeInv);
}

// This function computes differential inscattering for the given particle densities 
// (without applying phase functions)
void ComputePointDiffInsctr(in float2 f2ParticleDensityInCurrPoint,
                            in float2 f2NetParticleDensityFromCam,
                            in float2 f2NetParticleDensityToAtmTop,
                            out float3 f3DRlghInsctr,
                            out float3 f3DMieInsctr)
{
    // Compute total particle density from the top of the atmosphere through the integraion point to camera
    float2 f2TotalParticleDensity = f2NetParticleDensityFromCam + f2NetParticleDensityToAtmTop;
        
    // Get optical depth
    float3 f3TotalRlghOpticalDepth = g_MediaParams.f4RayleighExtinctionCoeff.rgb * f2TotalParticleDensity.x;
    float3 f3TotalMieOpticalDepth  = g_MediaParams.f4MieExtinctionCoeff.rgb      * f2TotalParticleDensity.y;
        
    // And total extinction for the current integration point:
    float3 f3TotalExtinction = exp( -(f3TotalRlghOpticalDepth + f3TotalMieOpticalDepth) );

    f3DRlghInsctr = f2ParticleDensityInCurrPoint.x * f3TotalExtinction;
    f3DMieInsctr  = f2ParticleDensityInCurrPoint.y * f3TotalExtinction; 
}

void ComputeInsctrIntegral(in float3    f3RayStart,
                           in float3    f3RayEnd,
                           in float3    f3EarthCentre,
                           in float     fEarthRadius,
                           in float     fAtmBottomAltitude,
                           in float     fAtmAltitudeRangeInv,
						   in float4    f4ParticleScaleHeight,
                           in float3    f3DirOnLight,
                           in uint      uiNumSteps,
                           inout float2 f2NetParticleDensityFromCam,
                           inout float3 f3RayleighInscattering,
                           inout float3 f3MieInscattering)
{
    // Evaluate the integrand at the starting point
    float2 f2PrevParticleDensity = float2(0.0, 0.0);
    float2 f2NetParticleDensityToAtmTop;
    GetAtmosphereProperties(f3RayStart,
                            f3EarthCentre,
                            fEarthRadius,
                            fAtmBottomAltitude,
                            fAtmAltitudeRangeInv,
                            f4ParticleScaleHeight,
                            f3DirOnLight,
                            f2PrevParticleDensity,
                            f2NetParticleDensityToAtmTop);

    float3 f3PrevDiffRInsctr = float3(0.0, 0.0, 0.0);
    float3 f3PrevDiffMInsctr = float3(0.0, 0.0, 0.0);
    ComputePointDiffInsctr(f2PrevParticleDensity, f2NetParticleDensityFromCam, f2NetParticleDensityToAtmTop, f3PrevDiffRInsctr, f3PrevDiffMInsctr);

    float fRayLen = length(f3RayEnd - f3RayStart);

    // We want to place more samples when the starting point is close to the surface,
    // but for high altitudes linear distribution works better.
    float fStartAltitude = length(f3RayStart - f3EarthCentre) - fEarthRadius;
    float pwr = lerp(2.0, 1.0, saturate((fStartAltitude - fAtmBottomAltitude) * fAtmAltitudeRangeInv));

    float fPrevSampleDist = 0.0;
    for (uint uiSampleNum = 1u; uiSampleNum <= uiNumSteps; ++uiSampleNum)
    {
        // Evaluate the function at the end of each section and compute the area of a trapezoid
        
        // We need to place more samples closer to the start point and fewer samples farther away.
        // I tried to use more scientific approach (see the bottom of the file), however
        // it did not work out because uniform media assumption is inapplicable.
        // So instead we will use ad-hoc approach (power function) that works quite well.
        float r = pow(float(uiSampleNum) / float(uiNumSteps), pwr);
        float3 f3CurrPos = lerp(f3RayStart, f3RayEnd, r);
        float fCurrSampleDist = fRayLen * r;
        float fStepLen = fCurrSampleDist - fPrevSampleDist;
        fPrevSampleDist = fCurrSampleDist;

        float2 f2ParticleDensity;
        GetAtmosphereProperties(f3CurrPos,
                                f3EarthCentre,
                                fEarthRadius,
                                fAtmBottomAltitude,
                                fAtmAltitudeRangeInv,
                                f4ParticleScaleHeight,
                                f3DirOnLight,
                                f2ParticleDensity,
                                f2NetParticleDensityToAtmTop);

        // Accumulate net particle density from the camera to the integration point:
        f2NetParticleDensityFromCam += (f2PrevParticleDensity + f2ParticleDensity) * (fStepLen / 2.0);
        f2PrevParticleDensity = f2ParticleDensity;

        float3 f3DRlghInsctr, f3DMieInsctr;
        ComputePointDiffInsctr(f2ParticleDensity, f2NetParticleDensityFromCam, f2NetParticleDensityToAtmTop, f3DRlghInsctr, f3DMieInsctr);

        f3RayleighInscattering += (f3DRlghInsctr + f3PrevDiffRInsctr) * (fStepLen / 2.0);
        f3MieInscattering      += (f3DMieInsctr  + f3PrevDiffMInsctr) * (fStepLen / 2.0);

        f3PrevDiffRInsctr = f3DRlghInsctr;
        f3PrevDiffMInsctr = f3DMieInsctr;
    }
}


void IntegrateUnshadowedInscattering(in float3   f3RayStart, 
                                     in float3   f3RayEnd,
                                     in float3   f3ViewDir,
                                     in float3   f3EarthCentre,
                                     in float    fEarthRadius,
                                     in float    fAtmBottomAltitude,
                                     in float    fAtmAltitudeRangeInv,
							         in float4   f4ParticleScaleHeight,
                                     in float3   f3DirOnLight,
                                     in uint     uiNumSteps,
                                     out float3  f3Inscattering,
                                     out float3  f3Extinction)
{
    float2 f2NetParticleDensityFromCam = float2(0.0, 0.0);
    float3 f3RayleighInscattering = float3(0.0, 0.0, 0.0);
    float3 f3MieInscattering = float3(0.0, 0.0, 0.0);
    ComputeInsctrIntegral( f3RayStart,
                           f3RayEnd,
                           f3EarthCentre,
                           fEarthRadius,
                           fAtmBottomAltitude,
                           fAtmAltitudeRangeInv,
                           f4ParticleScaleHeight,
                           f3DirOnLight,
                           uiNumSteps,
                           f2NetParticleDensityFromCam,
                           f3RayleighInscattering,
                           f3MieInscattering);

    float3 f3TotalRlghOpticalDepth = g_MediaParams.f4RayleighExtinctionCoeff.rgb * f2NetParticleDensityFromCam.x;
    float3 f3TotalMieOpticalDepth  = g_MediaParams.f4MieExtinctionCoeff.rgb      * f2NetParticleDensityFromCam.y;
    f3Extinction = exp( -(f3TotalRlghOpticalDepth + f3TotalMieOpticalDepth) );

    // Apply phase function
    // Note that cosTheta = dot(DirOnCamera, LightDir) = dot(ViewDir, DirOnLight) because
    // DirOnCamera = -ViewDir and LightDir = -DirOnLight
    float cosTheta = dot(f3ViewDir, f3DirOnLight);
    ApplyPhaseFunctions(f3RayleighInscattering, f3MieInscattering, cosTheta);

    f3Inscattering = f3RayleighInscattering + f3MieInscattering;
}


// BACKUP
//
// To better distribute samples for numerical integration I tried used the following simplifying assumptions:
// - Uniform media density
// - No light extinction
// Under these assumptions the total amount of light inscattered from 0 to distance t will be computed by
//
//      I(t) = Integral(exp(-a*x), 0 -> t) = 1/a * (1 - exp(-a*t))
//
// We want to find sampling that produces even distribution for function I(t):
//
//      I(tn) = I(D) * n / N, where D is the total distance and N is the total sample count.
//
// From here:
//
//      1/a * (1 - exp(-a*tn)) = I(D) * n / N
//      1 - exp(-a*tn) = a * I(D) * n / N
//      exp(-a*tn) = 1 - a * I(D) * n / N
//      tn = -1/a * log(1 - a * I(D) * n / N) = -1/a * log(1 - n/N * (1 - exp(-a*D)))
//
// This unfortunately did not work because homogeneous media assumption is inappropriate:
// -  when a = 1e-5, the distribution is basically linear
// -  when a = 1e-4, the first N-1 samples cover first 50% distance, while the last one covers the remaining 50% distance
