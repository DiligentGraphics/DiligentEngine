

void ComputeUnshadowedInscattering(float2     f2SampleLocation, 
                                   float      fCamSpaceZ,
                                   uint       uiNumSteps,
                                   float3     f3EarthCentre,
                                   out float3 f3Inscattering,
                                   out float3 f3Extinction)
{
    f3Inscattering = float3(0.0, 0.0, 0.0);
    f3Extinction = float3(1.0, 1.0, 1.0);
    float3 f3RayTermination = ProjSpaceXYZToWorldSpace( float3(f2SampleLocation, fCamSpaceZ), g_CameraAttribs.mProj, g_CameraAttribs.mViewProjInv );
    float3 f3CameraPos = g_CameraAttribs.f4Position.xyz;
    float3 f3ViewDir = f3RayTermination - f3CameraPos;
    float fRayLength = length(f3ViewDir);
    f3ViewDir /= fRayLength;

    float4 f4Isecs;
    GetRaySphereIntersection2(f3CameraPos, f3ViewDir, f3EarthCentre, 
                              float2(g_MediaParams.fAtmTopRadius, g_MediaParams.fAtmBottomRadius), f4Isecs);
    float2 f2RayAtmTopIsecs = f4Isecs.xy; 
    float2 f2RayEarthIsecs  = f4Isecs.zw;

    if( f2RayAtmTopIsecs.y <= 0.0 )
    {
        //                                                          view dir
        //                                                        /
        //             d<0                                       /
        //               *--------->                            *
        //            .      .                             .   /  . 
        //  .  '                    '  .         .  '         /\         '  .
        //                                                   /  f2rayatmtopisecs.y < 0
        //
        // the camera is outside the atmosphere and the ray either does not intersect the
        // top of it or the intersection point is behind the camera. In either
        // case there is no inscattering
        return;
    }

    float3 f3RayStart = f3CameraPos + f3ViewDir * max(0.0, f2RayAtmTopIsecs.x);
    if( fCamSpaceZ > g_CameraAttribs.fFarPlaneZ ) // fFarPlaneZ is pre-multiplied with 0.999999f
        fRayLength = +FLT_MAX;
    fRayLength = min(fRayLength, f2RayAtmTopIsecs.y);
    // If there is an intersection with the Earth surface, limit the tracing distance to the intersection
    if( f2RayEarthIsecs.x > 0.0 )
    {
        fRayLength = min(fRayLength, f2RayEarthIsecs.x);
    }    
    float3 f3RayEnd = f3CameraPos + f3ViewDir * fRayLength;

#if SINGLE_SCATTERING_MODE == SINGLE_SCTR_MODE_INTEGRATION
    IntegrateUnshadowedInscattering(f3RayStart, 
                                    f3RayEnd,
                                    f3ViewDir,
                                    f3EarthCentre,
                                    g_MediaParams.fEarthRadius,
                                    g_MediaParams.fAtmBottomAltitude,
                                    g_MediaParams.fAtmAltitudeRangeInv,
                                    g_MediaParams.f4ParticleScaleHeight,
                                    -g_LightAttribs.f4Direction.xyz,
                                    uiNumSteps,
                                    f3Inscattering,
                                    f3Extinction);
#endif

#if SINGLE_SCATTERING_MODE == SINGLE_SCTR_MODE_LUT || MULTIPLE_SCATTERING_MODE > MULTIPLE_SCTR_MODE_NONE

#if MULTIPLE_SCATTERING_MODE > MULTIPLE_SCTR_MODE_NONE
    #if SINGLE_SCATTERING_MODE == SINGLE_SCTR_MODE_LUT
        #define tex3DSctrLUT         g_tex3DMultipleSctrLUT
        #define tex3DSctrLUT_sampler g_tex3DMultipleSctrLUT_sampler
    #elif SINGLE_SCATTERING_MODE == SINGLE_SCTR_MODE_NONE || SINGLE_SCATTERING_MODE == SINGLE_SCTR_MODE_INTEGRATION
        #define tex3DSctrLUT         g_tex3DHighOrderSctrLUT
        #define tex3DSctrLUT_sampler g_tex3DHighOrderSctrLUT_sampler
    #endif
#else
    #define tex3DSctrLUT         g_tex3DSingleSctrLUT
    #define tex3DSctrLUT_sampler g_tex3DSingleSctrLUT_sampler
#endif

    f3Extinction = GetExtinctionUnverified(f3RayStart, f3RayEnd, f3ViewDir, f3EarthCentre, g_MediaParams.fEarthRadius, g_MediaParams.f4ParticleScaleHeight);

    // To avoid artifacts, we must be consistent when performing look-ups into the scattering texture, i.e.
    // we must assure that if the first look-up is above (below) horizon, then the second look-up
    // is also above (below) horizon. 
    float4 f4UVWQ = float4(-1.0, -1.0, -1.0, -1.0);
    f3Inscattering += 
        LookUpPrecomputedScattering(
            f3RayStart,
            f3ViewDir,
            f3EarthCentre,
            g_MediaParams.fEarthRadius,
            -g_LightAttribs.f4Direction.xyz,
            g_MediaParams.fAtmBottomAltitude,
            g_MediaParams.fAtmTopAltitude,
            tex3DSctrLUT,
            tex3DSctrLUT_sampler,
            f4UVWQ); 

    // Provide previous look-up coordinates to the function to assure that look-ups are consistent
    f3Inscattering -= f3Extinction *
        LookUpPrecomputedScattering(
            f3RayEnd,
            f3ViewDir,
            f3EarthCentre,
            g_MediaParams.fEarthRadius,
            -g_LightAttribs.f4Direction.xyz,
            g_MediaParams.fAtmBottomAltitude,
            g_MediaParams.fAtmTopAltitude,
            tex3DSctrLUT,
            tex3DSctrLUT_sampler,
            f4UVWQ);

    #undef tex3DSctrLUT
    #undef tex3DSctrLUT_sampler
#endif

}
