
#define NON_LINEAR_PARAMETERIZATION 1
#define SafetyHeightMargin 16.0
#define HeightPower 0.5
#define ViewZenithPower 0.2
#define SunViewPower 1.5


// Returns the cosine of the horizon angle for the given altitude
// (height above the sphere surface) and sphere radius
//      
//                    |
//           _ _ _ _ _| Horizon angle  
//            A       |'. 
//  fAltitude |       |  '.
//           _V_ _ _ _|    '.
//              .  '  '  '  .'.
//           .'               '.
//          '                   '
float GetCosHorizonAngle(float fAltitude, float fSphereRadius)
{
    fAltitude = max(fAltitude, 0.0);
    return -sqrt(fAltitude * (2.0 * fSphereRadius + fAltitude)) / (fSphereRadius + fAltitude);
}


// Returns the lookup table zenith angle coordinate
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
float ZenithAngle2TexCoord(float fCosZenithAngle,       // Cosine of the zenith angle
                           float fAltitude,             // Altitude (height above the sea level)
                           float fEarthRadius,          // Earth radius at sea level
                           float fAtmBottomAltitude,    // Altitude of the bottom atmosphere boundary (wrt sea level)
                           float fTexDim,               // Look-up texture dimension
                           float power,                 // Non-linear transform power
                           float fPrevTexCoord          // Previous look-up texture coordinate
                          )
{
    float fCosHorizonAngle = GetCosHorizonAngle(fAltitude - fAtmBottomAltitude, fEarthRadius + fAtmBottomAltitude);

    // When performing look-ups into the scattering texture, it is very important that all the look-ups are consistent
    // wrt to the horizon. This means that if the first look-up is above (below) horizon, then the second look-up
    // should also be above (below) horizon. 
    // We use previous texture coordinate, if it is provided, to find out if previous look-up was above or below
    // horizon. If texture coordinate is negative, then this is the first look-up
    bool bIsAboveHorizon = fPrevTexCoord >= 0.5;
    bool bIsBelowHorizon = 0.0 <= fPrevTexCoord && fPrevTexCoord < 0.5;

    float fTexCoord;
    if ( bIsAboveHorizon || 
        !bIsBelowHorizon && (fCosZenithAngle > fCosHorizonAngle))
    {
        // Scale to [0,1]
        fTexCoord = saturate( (fCosZenithAngle - fCosHorizonAngle) / (1.0 - fCosHorizonAngle) );
        fTexCoord = pow(fTexCoord, power);
        // Now remap texture coordinate to the upper half of the texture.
        // To avoid filtering across discontinuity at 0.5, we must map
        // the texture coordinate to [0.5 + 0.5/fTexDim, 1 - 0.5/fTexDim]
        //
        //      0.5   1.5               D/2+0.5        D-0.5  texture coordinate x dimension
        //       |     |                   |            |
        //    |  X  |  X  | .... |  X  ||  X  | .... |  X  |  
        //       0     1          D/2-1   D/2          D-1    texel index
        //
        fTexCoord = 0.5 + 0.5 / fTexDim + fTexCoord * (fTexDim/2.0 - 1.0) / fTexDim;
    }
    else
    {
        fTexCoord = saturate( (fCosHorizonAngle - fCosZenithAngle) / (fCosHorizonAngle - (-1.0)) );
        fTexCoord = pow(fTexCoord, power);
        // Now remap texture coordinate to the lower half of the texture.
        // To avoid filtering across discontinuity at 0.5, we must map
        // the texture coordinate to [0.5/fTexDim, 0.5 - 0.5/fTexDim]
        //
        //      0.5   1.5        D/2-0.5             texture coordinate x dimension
        //       |     |            |       
        //    |  X  |  X  | .... |  X  ||  X  | .... 
        //       0     1          D/2-1   D/2        texel index
        //
        fTexCoord = 0.5f / fTexDim + fTexCoord * (fTexDim/2.0 - 1.0) / fTexDim;
    }    

    return fTexCoord;
}


// Transforms zenith angle look-up coordinate into the cosine of the zenith angle
float TexCoord2ZenithAngle(float fTexCoord,             // Texture coordinate
                           float fAltitde,              // Altitude (height above the sea level)
                           float fEarthRadius,          // Earth radius at sea level
                           float fAtmBottomAltitude,    // Altitude of the bottom atmosphere boundary (wrt sea level)
                           float fTexDim,               // Look-up texture dimension
                           float power                  // Non-linear transform power
                          )
{
    float fCosZenithAngle;

    float fCosHorzAngle = GetCosHorizonAngle(fAltitde - fAtmBottomAltitude, fEarthRadius + fAtmBottomAltitude);
    if( fTexCoord > 0.5 )
    {
        // Remap to [0,1] from the upper half of the texture [0.5 + 0.5/fTexDim, 1 - 0.5/fTexDim]
        fTexCoord = saturate( (fTexCoord - (0.5 + 0.5 / fTexDim)) * fTexDim / (fTexDim/2.0 - 1.0) );
        fTexCoord = pow(fTexCoord, 1.0/power);
        // Assure that the ray does NOT hit Earth
        fCosZenithAngle = max( (fCosHorzAngle + fTexCoord * (1.0 - fCosHorzAngle)), fCosHorzAngle + 1e-4);
    }
    else
    {
        // Remap to [0,1] from the lower half of the texture [0.5/fTexDim, 0.5 - 0.5/fTexDim]
        fTexCoord = saturate((fTexCoord - 0.5 / fTexDim) * fTexDim / (fTexDim/2.0 - 1.0));
        fTexCoord = pow(fTexCoord, 1.0/power);
        // Assure that the ray DOES hit Earth
        fCosZenithAngle = min( (fCosHorzAngle - fTexCoord * (fCosHorzAngle - (-1.0))), fCosHorzAngle - 1e-4);
    }
    return fCosZenithAngle;
}

// Transforms inscattering look-up table coordinates into world parameters
void InsctrLUTCoords2WorldParams(in  float4 f4UVWQ,
                                 in  float  fEarthRadius,
                                 in  float  fAtmBottomAltitude,
                                 in  float  fAtmTopAltitude,
                                 out float  fAltitude,
                                 out float  fCosViewZenithAngle,
                                 out float  fCosSunZenithAngle,
                                 out float  fCosSunViewAngle)
{
#if NON_LINEAR_PARAMETERIZATION
    // Rescale to exactly 0,1 range
    f4UVWQ.xzw = saturate(( f4UVWQ * PRECOMPUTED_SCTR_LUT_DIM - float4(0.5,0.5,0.5,0.5) ) / ( PRECOMPUTED_SCTR_LUT_DIM - float4(1.0,1.0,1.0,1.0) )).xzw;

    f4UVWQ.x = pow( f4UVWQ.x, 1.0/HeightPower );
    // Allowable altitude range is limited to [fAtmBottomAltitude + SafetyHeightMargin, fAtmTopAltitude - SafetyHeightMargin] to
    // avoid numeric issues at the atmosphere boundaries
    fAltitude = f4UVWQ.x * ((fAtmTopAltitude - fAtmBottomAltitude) - 2.0 * SafetyHeightMargin) + (fAtmBottomAltitude + SafetyHeightMargin);

    fCosViewZenithAngle = TexCoord2ZenithAngle(f4UVWQ.y, fAltitude, fEarthRadius, fAtmBottomAltitude, PRECOMPUTED_SCTR_LUT_DIM.y, ViewZenithPower);
    
    // Use Eric Bruneton's formula for cosine of the sun-zenith angle
    fCosSunZenithAngle = tan((2.0 * f4UVWQ.z - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);

    f4UVWQ.w = sign(f4UVWQ.w - 0.5) * pow( abs((f4UVWQ.w - 0.5)*2.0), 1.0/SunViewPower)/2.0 + 0.5;
    fCosSunViewAngle = cos(f4UVWQ.w*PI);
#else
    // Rescale to exactly 0,1 range
    f4UVWQ = (f4UVWQ * PRECOMPUTED_SCTR_LUT_DIM - float4(0.5,0.5,0.5,0.5)) / (PRECOMPUTED_SCTR_LUT_DIM-float4(1.0,1.0,1.0,1.0));

    // Allowable altitude range is limited to [fAtmBottomAltitude + SafetyHeightMargin, fAtmTopAltitude - SafetyHeightMargin] to
    // avoid numeric issues at the atmosphere boundaries
    fAltitude = f4UVWQ.x * ((fAtmTopAltitude - fAtmBottomAltitude) - 2.0 * SafetyHeightMargin) + (fAtmBottomAltitude + SafetyHeightMargin);

    fCosViewZenithAngle = f4UVWQ.y * 2.0 - 1.0;
    fCosSunZenithAngle  = f4UVWQ.z * 2.0 - 1.0;
    fCosSunViewAngle    = f4UVWQ.w * 2.0 - 1.0;
#endif

    fCosViewZenithAngle = clamp(fCosViewZenithAngle, -1.0, +1.0);
    fCosSunZenithAngle  = clamp(fCosSunZenithAngle,  -1.0, +1.0);
    // Compute allowable range for the cosine of the sun view angle for the given
    // view zenith and sun zenith angles
    float D = (1.0 - fCosViewZenithAngle * fCosViewZenithAngle) * (1.0 - fCosSunZenithAngle  * fCosSunZenithAngle);
    
    // !!!!  IMPORTANT NOTE regarding NVIDIA hardware !!!!

    // There is a very weird issue on NVIDIA hardware with clamp(), saturate() and min()/max() 
    // functions. No matter what function is used, fCosViewZenithAngle and fCosSunZenithAngle
    // can slightly fall outside [-1,+1] range causing D to be negative
    // Using saturate(D), max(D, 0) and even D>0?D:0 does not work!
    // The only way to avoid taking the square root of negative value and obtaining NaN is 
    // to use max() with small positive value:
    D = sqrt( max(D, 1e-20) );
    
    // The issue was reproduceable on NV GTX 680, driver version 9.18.13.2723 (9/12/2013).
    // The problem does not arise on Intel hardware

    float2 f2MinMaxCosSunViewAngle = fCosViewZenithAngle*fCosSunZenithAngle + float2(-D, +D);
    // Clamp to allowable range
    fCosSunViewAngle    = clamp(fCosSunViewAngle, f2MinMaxCosSunViewAngle.x, f2MinMaxCosSunViewAngle.y);
}

float4 WorldParams2InsctrLUTCoords(float  fAltitude,
                                   float  fCosViewZenithAngle,
                                   float  fCosSunZenithAngle,
                                   float  fCosSunViewAngle,
                                   float  fEarthRadius,
                                   float  fAtmBottomAltitude,
                                   float  fAtmTopAltitude,
                                   float4 f4RefUVWQ)
{
    float4 f4UVWQ;

    // Limit allowable altitude range to [fAtmBottomAltitude + SafetyHeightMargin, AtmTopAltitude - SafetyHeightMargin] to
    // avoid numeric issues at the atmosphere boundaries.
    // (ray/sphere intersection tests are unstable when fAltitude == fAtmBottomAltitude and fAltitude == AtmTopAltitude)
    fAltitude = clamp(fAltitude, fAtmBottomAltitude + SafetyHeightMargin, fAtmTopAltitude - SafetyHeightMargin);
    f4UVWQ.x = saturate( (fAltitude - (fAtmBottomAltitude + SafetyHeightMargin)) / ((fAtmTopAltitude - fAtmBottomAltitude) - 2.0*SafetyHeightMargin) );

#if NON_LINEAR_PARAMETERIZATION
    f4UVWQ.x = pow(f4UVWQ.x, HeightPower);

    f4UVWQ.y = ZenithAngle2TexCoord(fCosViewZenithAngle, fAltitude, fEarthRadius, fAtmBottomAltitude, PRECOMPUTED_SCTR_LUT_DIM.y, ViewZenithPower, f4RefUVWQ.y);
    
    // Use Eric Bruneton's formula for cosine of the sun-zenith angle
    f4UVWQ.z = (atan(max(fCosSunZenithAngle, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5;

    fCosSunViewAngle = clamp(fCosSunViewAngle, -1.0, +1.0);
    f4UVWQ.w = acos(fCosSunViewAngle) / PI;
    f4UVWQ.w = sign(f4UVWQ.w - 0.5) * pow( abs((f4UVWQ.w - 0.5)/0.5), SunViewPower)/2.0 + 0.5;
    
    f4UVWQ.xzw = ((f4UVWQ * (PRECOMPUTED_SCTR_LUT_DIM - float4(1.0, 1.0, 1.0, 1.0)) + float4(0.5, 0.5, 0.5, 0.5)) / PRECOMPUTED_SCTR_LUT_DIM).xzw;
#else
    f4UVWQ.y = (fCosViewZenithAngle+1.f) / 2.f;
    f4UVWQ.z = (fCosSunZenithAngle +1.f) / 2.f;
    f4UVWQ.w = (fCosSunViewAngle   +1.f) / 2.f;

    f4UVWQ = (f4UVWQ * (PRECOMPUTED_SCTR_LUT_DIM - float4(1.0,1.0,1.0,1.0)) + float4(0.5,0.5,0.5,0.5)) / PRECOMPUTED_SCTR_LUT_DIM;
#endif

    return f4UVWQ;
}

float3 ComputeViewDir(in float fCosViewZenithAngle)
{
    return float3(sqrt(saturate(1.0 - fCosViewZenithAngle*fCosViewZenithAngle)), fCosViewZenithAngle, 0.0);
}

float3 ComputeLightDir(in float3 f3ViewDir, in float fCosSunZenithAngle, in float fCosSunViewAngle)
{
    float3 f3DirOnLight;
    f3DirOnLight.x = (f3ViewDir.x > 0.0) ? (fCosSunViewAngle - fCosSunZenithAngle * f3ViewDir.y) / f3ViewDir.x : 0.0;
    f3DirOnLight.y = fCosSunZenithAngle;
    f3DirOnLight.z = sqrt( saturate(1.0 - dot(f3DirOnLight.xy, f3DirOnLight.xy)) );
    // Do not normalize f3DirOnLight! Even if its length is not exactly 1 (which can 
    // happen because of fp precision issues), all the dot products will still be as 
    // specified, which is essentially important. If we normalize the vector, all the 
    // dot products will deviate, resulting in wrong pre-computation.
    // Since fCosSunViewAngle is clamped to allowable range, f3DirOnLight should always
    // be normalized. However, due to some issues on NVidia hardware sometimes
    // it may not be as that (see IMPORTANT NOTE regarding NVIDIA hardware)
    //f3DirOnLight = normalize(f3DirOnLight);
    return f3DirOnLight;
}


float3 LookUpPrecomputedScattering(in float3 f3StartPoint, 
                                   in float3 f3ViewDir, 
                                   in float3 f3EarthCentre,
                                   in float  fEarthRadius,
                                   in float3 f3DirOnLight,
                                   in float  fAtmBottomAltitude,
                                   in float  fAtmTopAltitude,
                                   in Texture3D<float3> tex3DScatteringLUT,
                                   in SamplerState      tex3DScatteringLUT_sampler,
                                   inout float4 f4UVWQ)
{
    float3 f3EarthCentreToPointDir = f3StartPoint - f3EarthCentre;
    float fDistToEarthCentre = length(f3EarthCentreToPointDir);
    f3EarthCentreToPointDir /= fDistToEarthCentre;
    float fAltitude          = fDistToEarthCentre - fEarthRadius; // Height above sea level
    float fCosViewZenithAngle = dot( f3EarthCentreToPointDir, f3ViewDir    );
    float fCosSunZenithAngle  = dot( f3EarthCentreToPointDir, f3DirOnLight );
    float fCosSunViewAngle    = dot( f3ViewDir,               f3DirOnLight );

    // Provide previous look-up coordinates
    f4UVWQ = WorldParams2InsctrLUTCoords(fAltitude,
                                         fCosViewZenithAngle,
                                         fCosSunZenithAngle, 
                                         fCosSunViewAngle, 
                                         fEarthRadius,
                                         fAtmBottomAltitude,
                                         fAtmTopAltitude,
                                         f4UVWQ);

    float3 f3UVW0; 
    f3UVW0.xy = f4UVWQ.xy;
    float fQ0Slice = floor(f4UVWQ.w * PRECOMPUTED_SCTR_LUT_DIM.w - 0.5);
    fQ0Slice = clamp(fQ0Slice, 0.0, PRECOMPUTED_SCTR_LUT_DIM.w-1.0);
    float fQWeight = (f4UVWQ.w * PRECOMPUTED_SCTR_LUT_DIM.w - 0.5) - fQ0Slice;
    fQWeight = max(fQWeight, 0.0);
    float2 f2SliceMinMaxZ = float2(fQ0Slice, fQ0Slice+1.0)/PRECOMPUTED_SCTR_LUT_DIM.w + float2(0.5,-0.5) / (PRECOMPUTED_SCTR_LUT_DIM.z*PRECOMPUTED_SCTR_LUT_DIM.w);
    f3UVW0.z =  (fQ0Slice + f4UVWQ.z) / PRECOMPUTED_SCTR_LUT_DIM.w;
    f3UVW0.z = clamp(f3UVW0.z, f2SliceMinMaxZ.x, f2SliceMinMaxZ.y);
    
    float fQ1Slice = min(fQ0Slice+1.0, PRECOMPUTED_SCTR_LUT_DIM.w-1.0);
    float fNextSliceOffset = (fQ1Slice - fQ0Slice) / PRECOMPUTED_SCTR_LUT_DIM.w;
    float3 f3UVW1 = f3UVW0 + float3(0.0, 0.0, fNextSliceOffset);
    float3 f3Insctr0 = tex3DScatteringLUT.SampleLevel(tex3DScatteringLUT_sampler/*LinearClamp*/, f3UVW0, 0.0);
    float3 f3Insctr1 = tex3DScatteringLUT.SampleLevel(tex3DScatteringLUT_sampler/*LinearClamp*/, f3UVW1, 0.0);
    float3 f3Inscattering = lerp(f3Insctr0, f3Insctr1, fQWeight);

    return f3Inscattering;
}
