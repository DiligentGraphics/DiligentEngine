#ifndef _EPIPOLAR_LIGHT_SCATTERING_STRCUTURES_FXH_
#define _EPIPOLAR_LIGHT_SCATTERING_STRCUTURES_FXH_

#ifdef __cplusplus

#   ifndef BOOL
#      define BOOL int32_t // Do not use bool, because sizeof(bool)==1 !
#   endif

#   ifndef TRUE
#      define TRUE 1
#   endif

#   ifndef FALSE
#      define FALSE 0
#   endif

#   ifndef CHECK_STRUCT_ALIGNMENT
        // Note that defining empty macros causes GL shader compilation error on Mac, because
        // it does not allow standalone semicolons outside of main.
        // On the other hand, adding semicolon at the end of the macro definition causes gcc error.
#       define CHECK_STRUCT_ALIGNMENT(s) static_assert( sizeof(s) % 16 == 0, "sizeof(" #s ") is not multiple of 16" )
#   endif

#   ifndef DEFAULT_VALUE
#       define DEFAULT_VALUE(x) =x
#   endif

#else

#   ifndef BOOL
#       define BOOL bool
#   endif

#   ifndef DEFAULT_VALUE
#       define DEFAULT_VALUE(x)
#   endif

#endif


// Epipolar light scattering
#define LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING  0
// High-quality brute-force ray marching for every pixel without any optimizations
#define LIGHT_SCTR_TECHNIQUE_BRUTE_FORCE        1


// Shadow map cascade processing mode

// Process all shadow map cascades in a single pass
#define CASCADE_PROCESSING_MODE_SINGLE_PASS     0
// Process every shadow map cascade in a separate pass
#define CASCADE_PROCESSING_MODE_MULTI_PASS      1
// Process every shadow map cascade in a separate pass, but use single instanced draw command
#define CASCADE_PROCESSING_MODE_MULTI_PASS_INST 2


// Epipolar sampling refinement criterion

// Use depth difference to refine epipolar sampling
#define REFINEMENT_CRITERION_DEPTH_DIFF  0
// Use inscattering difference to refine epipolar sampling
#define REFINEMENT_CRITERION_INSCTR_DIFF 1


// Extinction evaluation mode used when attenuating background

// Evaluate extinction for each pixel using analytic formula by Eric Bruneton
#define EXTINCTION_EVAL_MODE_PER_PIXEL 0 
                                        
// Render extinction in epipolar space and perform bilateral filtering 
// in the same manner as for inscattering
#define EXTINCTION_EVAL_MODE_EPIPOLAR  1 
                                         
                                         
// Single scattering evaluation mode

// No single scattering
#define SINGLE_SCTR_MODE_NONE        0
// Use numerical intergarion
#define SINGLE_SCTR_MODE_INTEGRATION 1
// Use single scattering look-up-table
#define SINGLE_SCTR_MODE_LUT         2

// No higher-order scattering
#define MULTIPLE_SCTR_MODE_NONE       0
// Use unoccluded (unshadowed) scattering
#define MULTIPLE_SCTR_MODE_UNOCCLUDED 1
// Use occluded (shadowed) scattering
#define MULTIPLE_SCTR_MODE_OCCLUDED   2

#ifndef EARTH_RADIUS
    // Average Earth radius at sea level
#   define EARTH_RADIUS 6371000.0
#endif

struct EpipolarLightScatteringAttribs
{
    // Total number of epipolar slices (or lines). For high quality effect,
    // set this value to (Screen Width + Screen Height)/2
    uint uiNumEpipolarSlices                DEFAULT_VALUE(512);
    // Maximum number of samples on a single epipolar line.
    // For high quality effect, set this value to max(Screen Width, Screen Height)/2. 
    uint uiMaxSamplesInSlice                DEFAULT_VALUE(256);
    // Initial ray marching sample spacing on an epipolar line. 
    // Additional samples are added at discontinuities.
    uint uiInitialSampleStepInSlice         DEFAULT_VALUE(16);
    // Sample density scale near the epipole where inscattering changes rapidly.
    // Note that sampling near the epipole is very cheap since only a few steps
    // are required to perform ray marching.
    uint uiEpipoleSamplingDensityFactor     DEFAULT_VALUE(2);

    // Refinement threshold controls detection of discontinuities. Smaller values
    // produce more samples and higher quality, but at a higher performance cost.
    float fRefinementThreshold              DEFAULT_VALUE(0.03f);
    // Whether to show epipolar sampling.
    // Do not use bool, because sizeof(bool)==1 and as a result bool variables
    // will be incorrectly mapped on GPU constant buffer.
    BOOL bShowSampling                      DEFAULT_VALUE(FALSE); 
    // Whether to correct inscattering at depth discontinuities. Improves quality
    // for additional cost.
    BOOL bCorrectScatteringAtDepthBreaks    DEFAULT_VALUE(FALSE);
    // Whether to display pixels which are classified as depth discontinuities and which
    // will be corrected. Only has effect when bCorrectScatteringAtDepthBreaks is TRUE.
    BOOL bShowDepthBreaks                   DEFAULT_VALUE(FALSE); 

    // Whether to show lighting only
    BOOL bShowLightingOnly                  DEFAULT_VALUE(FALSE);
    // Optimize sample locations to avoid oversampling. This should generally be TRUE.
    BOOL bOptimizeSampleLocations           DEFAULT_VALUE(TRUE);
    // Wether to enable light shafts or render unshadowed inscattering.
    // Setting this to FALSE increases performance, but reduces visual quality.
    BOOL bEnableLightShafts                 DEFAULT_VALUE(TRUE);
    // Number of inscattering integral steps taken when computing unshadowed inscattering (default is OK).
    uint uiInstrIntegralSteps               DEFAULT_VALUE(30);
    
    // Size of the shadowmap texel (1/width, 1/height)
    float2 f2ShadowMapTexelSize             DEFAULT_VALUE(float2(0,0));
    // Maximum number of ray marching samples on a single ray. Typically this value should match the maximum 
    // shadow map cascade resolution. Using lower value will improve performance but may result
    // in moire patterns. Note that in most cases singificantly less samples are actually taken.
    uint uiMaxSamplesOnTheRay               DEFAULT_VALUE(512);
    // The number of ray marching samples on a ray when running scattering correction pass.
    // This value should typically be much lower than the maximum number of samples on a single ray
    // because 1D min-max optimization is not available during the correction pass.
    uint uiNumSamplesOnTheRayAtDepthBreak   DEFAULT_VALUE(32);

    // This defines the number of samples at the lowest level of min-max binary tree
    // and should match the maximum cascade shadow map resolution
    uint uiMinMaxShadowMapResolution        DEFAULT_VALUE(0);
    // Number of shadow map cascades
    int iNumCascades                        DEFAULT_VALUE(0);
    // First cascade to use for ray marching. Usually first few cascades are small, and ray
    // marching them is inefficient.
    int iFirstCascadeToRayMarch             DEFAULT_VALUE(2);
    // Cap on the maximum shadow map step in texels. Can be increased for higher shadow map
    // resolutions.
    float fMaxShadowMapStep                 DEFAULT_VALUE(16.f);

    // Whether to use 1D min/max binary tree optimization. This improves
    // performance for higher shadow map resolution. Test it.
    BOOL bUse1DMinMaxTree                   DEFAULT_VALUE(TRUE);
    // Whether to use 32-bit float or 16-bit UNORM min-max binary tree.
    BOOL bIs32BitMinMaxMipMap               DEFAULT_VALUE(FALSE);
    // Technique used to evaluate light scattering.
    int  iLightSctrTechnique                DEFAULT_VALUE(LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING);
    // Shadow map cascades processing mode.
    int  iCascadeProcessingMode             DEFAULT_VALUE(CASCADE_PROCESSING_MODE_SINGLE_PASS);

    // Epipolar sampling refinement criterion.
    int  iRefinementCriterion               DEFAULT_VALUE(REFINEMENT_CRITERION_INSCTR_DIFF);
    // Single scattering evaluation mode.
    int  iSingleScatteringMode              DEFAULT_VALUE(SINGLE_SCTR_MODE_INTEGRATION);
    // Higher-order scattering evaluation mode.
    int  iMultipleScatteringMode            DEFAULT_VALUE(MULTIPLE_SCTR_MODE_UNOCCLUDED);
    // Atmospheric extinction evaluation mode.
    int  iExtinctionEvalMode                DEFAULT_VALUE(EXTINCTION_EVAL_MODE_EPIPOLAR);

    // Whether to use Ozone approximation (ignored when custom scattering coefficients are used).
    BOOL bUseOzoneApproximation             DEFAULT_VALUE(TRUE);
    // Whether to use custom scattering coefficients.
    BOOL bUseCustomSctrCoeffs               DEFAULT_VALUE(FALSE);
    // Aerosol density scale to use for scattering coefficient computation.
    float fAerosolDensityScale              DEFAULT_VALUE(1.f);
    // Aerosol absorbtion scale to use for scattering coefficient computation.
    float fAerosolAbsorbtionScale           DEFAULT_VALUE(0.1f);

    // Custom Rayleigh coefficients.
    float4 f4CustomRlghBeta                 DEFAULT_VALUE(float4(5.8e-6f, 13.5e-6f, 33.1e-6f, 0.f));
    // Custom Mie coefficients.
    float4 f4CustomMieBeta                  DEFAULT_VALUE(float4(2.e-5f, 2.e-5f, 2.e-5f, 0.f));
    // Custom Ozone absorption coefficient.
    float4 f4CustomOzoneAbsorption          DEFAULT_VALUE(float4(0.650f, 1.881f, 0.085f, 0.f) * 1e-6f);

    float4 f4EarthCenter                    DEFAULT_VALUE(float4(0.f, -static_cast<float>(EARTH_RADIUS), 0.f, 0.f));

    // ToneMappingStructures.fxh must be included before EpipolarLightScatteringStructures.fxh
    ToneMappingAttribs  ToneMapping;

    // Members below are automatically set by the effect. User-provided values are ignored.
    float4 f4ScreenResolution               DEFAULT_VALUE(float4(0,0,0,0));
    float4 f4LightScreenPos                 DEFAULT_VALUE(float4(0,0,0,0));
    
    BOOL   bIsLightOnScreen                 DEFAULT_VALUE(FALSE);
    float  fNumCascades                     DEFAULT_VALUE(0);
    float  fFirstCascadeToRayMarch          DEFAULT_VALUE(0);
    int    Padding0;
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(EpipolarLightScatteringAttribs);
#endif

struct AirScatteringAttribs
{
    // Angular Rayleigh scattering coefficient contains all the terms exepting 1 + cos^2(Theta):
    // Pi^2 * (n^2-1)^2 / (2*N) * (6+3*Pn)/(6-7*Pn)
    float4 f4AngularRayleighSctrCoeff;
    // Total Rayleigh scattering coefficient is the integral of angular scattering coefficient in all directions
    // and is the following:
    // 8 * Pi^3 * (n^2-1)^2 / (3*N) * (6+3*Pn)/(6-7*Pn)
    float4 f4TotalRayleighSctrCoeff;
    float4 f4RayleighExtinctionCoeff;

    // Note that angular scattering coefficient is essentially a phase function multiplied by the
    // total scattering coefficient
    float4 f4AngularMieSctrCoeff;
    float4 f4TotalMieSctrCoeff;
    float4 f4MieExtinctionCoeff;

    float4 f4TotalExtinctionCoeff;
    // Cornette-Shanks phase function (see Nishita et al. 93) normalized to unity has the following form:
    // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)
    float4 f4CS_g; // x == 3*(1-g^2) / (2*(2+g^2))
                   // y == 1 + g^2
                   // z == -2*g

    // Earth parameters can't be chnaged at run time
    float fEarthRadius              DEFAULT_VALUE(static_cast<float>(EARTH_RADIUS));
    float fAtmBottomAltitude        DEFAULT_VALUE(0.f);     // Altitude of the bottom atmosphere boundary (sea level by default)
    float fAtmTopAltitude           DEFAULT_VALUE(80000.f); // Altitude of the top atmosphere boundary, 80 km by default
    float fTurbidity                DEFAULT_VALUE(1.02f);

    float fAtmBottomRadius          DEFAULT_VALUE(fEarthRadius + fAtmBottomAltitude);
    float fAtmTopRadius             DEFAULT_VALUE(fEarthRadius + fAtmTopAltitude);
    float fAtmAltitudeRangeInv      DEFAULT_VALUE(1.f / (fAtmTopAltitude - fAtmBottomAltitude));
    float fAerosolPhaseFuncG        DEFAULT_VALUE(0.76f);

    float4 f4ParticleScaleHeight    DEFAULT_VALUE(float4(7994.f, 1200.f, 1.f/7994.f, 1.f/1200.f));
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(AirScatteringAttribs);
#endif

// Internal structure used by the effect
struct MiscDynamicParams
{
    float fMaxStepsAlongRay;   // Maximum number of steps during ray tracing
    float fCascadeInd;
    float fElapsedTime;
    float fDummy;

#ifdef __cplusplus
    uint ui4SrcMinMaxLevelXOffset;
    uint ui4SrcMinMaxLevelYOffset;
    uint ui4DstMinMaxLevelXOffset;
    uint ui4DstMinMaxLevelYOffset;
#else
    uint4 ui4SrcDstMinMaxLevelOffset;
#endif
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(MiscDynamicParams);
#endif

#endif //_EPIPOLAR_LIGHT_SCATTERING_STRCUTURES_FXH_
