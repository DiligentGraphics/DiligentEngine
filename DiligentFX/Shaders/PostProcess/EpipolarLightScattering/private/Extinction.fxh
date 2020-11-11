
// This function for analytical evaluation of particle density integral is 
// provided by Eric Bruneton
// http://www-evasion.inrialpes.fr/Membres/Eric.Bruneton/
//
// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
float2 GetDensityIntegralAnalytic(float r, float mu, float d, float EarthRadius, float4 ParticleScaleHeight) 
{
    float2 f2A     = sqrt(ParticleScaleHeight.zw * 0.5 * r);
    float4 f4A01   = f2A.xxyy * float2(mu, mu + d / r).xyxy;
    float4 f4A01s  = sign(f4A01);
    float4 f4A01sq = f4A01 * f4A01;
    
    float2 f2X;
    f2X.x = f4A01s.y > f4A01s.x ? exp(f4A01sq.x) : 0.0;
    f2X.y = f4A01s.w > f4A01s.z ? exp(f4A01sq.z) : 0.0;
    
    float4 f4Y =
        f4A01s 
        / (2.3193 * abs(f4A01) + sqrt(1.52 * f4A01sq + 4.0)) 
        * float3(1.0, exp(-ParticleScaleHeight.zw * d * (d / (2.0 * r) + mu)) ).xyxz;

    return 
        sqrt((6.2831 * ParticleScaleHeight.xy) * r) 
        * exp((EarthRadius - r) * ParticleScaleHeight.zw) 
        * (f2X + float2(f4Y.x - f4Y.y, f4Y.z - f4Y.w));
}


float3 GetExtinctionUnverified(float3 f3StartPos,
                               float3 f3EndPos,
                               float3 f3EyeDir,
                               float3 f3EarthCentre,
                               float  fEarthRadius,
                               float4 f4ParticleScaleHeight)
{
#if 0
    float2 f2ParticleDensity = IntegrateParticleDensity(f3StartPos, f3EndPos, f3EarthCentre, 20);
#else
    float r = length(f3StartPos-f3EarthCentre);
    float fCosZenithAngle = dot(f3StartPos-f3EarthCentre, f3EyeDir) / r;
    float2 f2ParticleDensity = GetDensityIntegralAnalytic(r, fCosZenithAngle, length(f3StartPos - f3EndPos), fEarthRadius, f4ParticleScaleHeight);
#endif

    // Get optical depth
    float3 f3TotalRlghOpticalDepth = g_MediaParams.f4RayleighExtinctionCoeff.rgb * f2ParticleDensity.x;
    float3 f3TotalMieOpticalDepth  = g_MediaParams.f4MieExtinctionCoeff.rgb      * f2ParticleDensity.y;
        
    // Compute extinction
    float3 f3Extinction = exp( -(f3TotalRlghOpticalDepth + f3TotalMieOpticalDepth) );
    return f3Extinction;
}

float3 GetExtinction(float3 f3StartPos,
                     float3 f3EndPos,
                     float3 f3EarthCentre,
                     float  fEarthRadius,
                     float  fAtmTopRadius,
                     float4 f4ParticleScaleHeight)
{
    float3 f3EyeDir = f3EndPos - f3StartPos;
    float fRayLength = length(f3EyeDir);
    f3EyeDir /= fRayLength;

    // Compute intersections of the view ray with the atmosphere and the Earth
    float4 f4Isecs;
    GetRaySphereIntersection2( f3StartPos, f3EyeDir, f3EarthCentre,
                               float2(fEarthRadius, fAtmTopRadius), 
                               f4Isecs);
    float2 f2RayEarthIsecs  = f4Isecs.xy;
    float2 f2RayAtmTopIsecs = f4Isecs.zw;

    // If the ray misses the atmosphere, there is no extinction
    if (f2RayAtmTopIsecs.y < 0.0)
    {
        return float3(1.0, 1.0, 1.0);
    }

    // Limit the ray length by the distance to the top of the atmosphere
    fRayLength = min(fRayLength, f2RayAtmTopIsecs.y);

    if (f2RayEarthIsecs.x > 0.0)
    {
        // The ray hits the Earth surface
        fRayLength = min(fRayLength, f2RayEarthIsecs.x);
    }

    // Update the end point
    f3EndPos = f3StartPos + f3EyeDir * fRayLength;

    // If the ray starts outside of the atmosphere, move the starting point to the intersection
    f3StartPos += f3EyeDir * max(f2RayAtmTopIsecs.x, 0.0);

    return GetExtinctionUnverified(f3StartPos, f3EndPos, f3EyeDir, f3EarthCentre, fEarthRadius, f4ParticleScaleHeight);
}
