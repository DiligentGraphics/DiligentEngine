#ifndef _EPIPOLAR_LIGHT_SCATTERING_FUNCTIONS_FXH_
#define _EPIPOLAR_LIGHT_SCATTERING_FUNCTIONS_FXH_

void GetSunLightExtinctionAndSkyLight(in float3               f3PosWS,
                                      in float3               f3EarthCentre,
                                      in float3               f3LightDirection,
                                      in AirScatteringAttribs MediaAttribs,

                                      in Texture2D<float2>    tex2DOccludedNetDensityToAtmTop,
                                      in SamplerState         tex2DOccludedNetDensityToAtmTop_sampler,
                                      in Texture2D< float3 >  tex2DAmbientSkylight,
                                      in SamplerState         tex2DAmbientSkylight_sampler,

                                      out float3              f3Extinction,
                                      out float3              f3AmbientSkyLight)
{
    float3 f3DirFromEarthCentre = f3PosWS - f3EarthCentre;
    float fDistToCentre = length(f3DirFromEarthCentre);
    f3DirFromEarthCentre /= fDistToCentre;
    float fAltitude = fDistToCentre - MediaAttribs.fEarthRadius;
    float fCosZenithAngle = dot(f3DirFromEarthCentre, -f3LightDirection);

    float fNormalizedAltitude = (fAltitude - MediaAttribs.fAtmBottomAltitude) * MediaAttribs.fAtmAltitudeRangeInv;
    float2 f2ParticleDensityToAtmTop = tex2DOccludedNetDensityToAtmTop.SampleLevel( tex2DOccludedNetDensityToAtmTop_sampler, float2(fNormalizedAltitude, fCosZenithAngle*0.5 + 0.5), 0 );
    
    float3 f3RlghOpticalDepth = MediaAttribs.f4RayleighExtinctionCoeff.rgb * f2ParticleDensityToAtmTop.x;
    float3 f3MieOpticalDepth  = MediaAttribs.f4MieExtinctionCoeff.rgb      * f2ParticleDensityToAtmTop.y;
        
    f3Extinction = exp( -(f3RlghOpticalDepth + f3MieOpticalDepth) );
    
    f3AmbientSkyLight = tex2DAmbientSkylight.SampleLevel( tex2DAmbientSkylight_sampler, float2(fCosZenithAngle*0.5 + 0.5, 0.5), 0 );
}

#endif //_EPIPOLAR_LIGHT_SCATTERING_FUNCTIONS_FXH_
