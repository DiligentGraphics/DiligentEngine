//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------

#   ifndef _STRCUTURES_FXH_
#   define _STRCUTURES_FXH_

#define PI (3.1415928f)

#ifdef __cplusplus

#ifndef BOOL
#   define BOOL int32_t // Do not use bool, because sizeof(bool)==1 !
#endif

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#endif

#else

#   define BOOL bool

#endif

#ifdef __cplusplus
#   define CHECK_STRUCT_ALIGNMENT(s) static_assert( sizeof(s) % 16 == 0, "structure size is not multiple of 16" );
#else
#   define CHECK_STRUCT_ALIGNMENT(s)
#endif



#define MAX_CASCADES 8
struct CascadeAttribs
{
	float4 f4LightSpaceScale;
	float4 f4LightSpaceScaledBias;
    float4 f4StartEndZ;
};
#ifdef __cplusplus
static_assert( (sizeof(CascadeAttribs) % 16) == 0, "sizeof(CascadeAttribs) is not multiple of 16" );
#endif

struct ShadowMapAttribs
{
    // 0
#ifdef __cplusplus
    float4x4 mWorldToLightView; // Matrices in HLSL are COLUMN-major while float4x4 is ROW major
#else
    matrix mWorldToLightView;  // Transform from view space to light projection space
#endif
    // 16
    CascadeAttribs Cascades[MAX_CASCADES];

#ifdef __cplusplus
    float fCascadeCamSpaceZEnd[MAX_CASCADES];
    float4x4 mWorldToShadowMapUVDepth[MAX_CASCADES];
#else
	float4 f4CascadeCamSpaceZEnd[MAX_CASCADES/4];
    matrix mWorldToShadowMapUVDepth[MAX_CASCADES];
#endif

    // Do not use bool, because sizeof(bool)==1 !
	BOOL bVisualizeCascades;

    float3 f3Padding;
};
#ifdef __cplusplus
static_assert( (sizeof(ShadowMapAttribs) % 16) == 0, "sizeof(ShadowMapAttribs) is not multiple of 16" );
#endif


struct LightAttribs
{
    float4 f4DirOnLight;
    float4 f4AmbientLight;
    float4 f4LightScreenPos;
    float4 f4ExtraterrestrialSunColor;

    BOOL bIsLightOnScreen;
    float3 f3Dummy;

    ShadowMapAttribs ShadowAttribs;
};
CHECK_STRUCT_ALIGNMENT(LightAttribs)

struct CameraAttribs
{
    float4 f4CameraPos;            ///< Camera world position
    float fNearPlaneZ; 
    float fFarPlaneZ; // fNearPlaneZ < fFarPlaneZ
    float2 f2Dummy;

#ifdef __cplusplus
    float4x4 WorldViewProj;
    //float4x4 mView;
    float4x4 mProj;
    float4x4 mViewProjInv;
#else
    matrix WorldViewProj;
    //matrix mView;
    matrix mProj;
    matrix mViewProjInv;
#endif
};
CHECK_STRUCT_ALIGNMENT(CameraAttribs)

#define LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING 0
#define LIGHT_SCTR_TECHNIQUE_BRUTE_FORCE 1

#define CASCADE_PROCESSING_MODE_SINGLE_PASS 0
#define CASCADE_PROCESSING_MODE_MULTI_PASS 1
#define CASCADE_PROCESSING_MODE_MULTI_PASS_INST 2

#define REFINEMENT_CRITERION_DEPTH_DIFF 0
#define REFINEMENT_CRITERION_INSCTR_DIFF 1

// Extinction evaluation mode used when attenuating background
#define EXTINCTION_EVAL_MODE_PER_PIXEL 0// Evaluate extinction for each pixel using analytic formula 
                                        // by Eric Bruneton
#define EXTINCTION_EVAL_MODE_EPIPOLAR 1 // Render extinction in epipolar space and perform
                                        // bilateral filtering in the same manner as for
                                        // inscattering

#define SINGLE_SCTR_MODE_NONE 0
#define SINGLE_SCTR_MODE_INTEGRATION 1
#define SINGLE_SCTR_MODE_LUT 2

#define MULTIPLE_SCTR_MODE_NONE 0
#define MULTIPLE_SCTR_MODE_UNOCCLUDED 1
#define MULTIPLE_SCTR_MODE_OCCLUDED 2

#define TONE_MAPPING_MODE_EXP 0
#define TONE_MAPPING_MODE_REINHARD 1
#define TONE_MAPPING_MODE_REINHARD_MOD 2
#define TONE_MAPPING_MODE_UNCHARTED2 3
#define TONE_MAPPING_FILMIC_ALU 4
#define TONE_MAPPING_LOGARITHMIC 5
#define TONE_MAPPING_ADAPTIVE_LOG 6


struct PostProcessingAttribs
{
    uint m_uiNumEpipolarSlices;
    uint m_uiMaxSamplesInSlice;
    uint m_uiInitialSampleStepInSlice;
    uint m_uiEpipoleSamplingDensityFactor;

    float m_fRefinementThreshold;
    // do not use bool, because sizeof(bool)==1 and as a result bool variables
    // will be incorrectly mapped on GPU constant buffer
    BOOL m_bShowSampling; 
    BOOL m_bCorrectScatteringAtDepthBreaks; 
    BOOL m_bShowDepthBreaks; 

    BOOL m_bShowLightingOnly;
    BOOL m_bOptimizeSampleLocations;
    BOOL m_bEnableLightShafts;
    uint m_uiInstrIntegralSteps;
    
    float2 m_f2ShadowMapTexelSize;
    uint m_uiShadowMapResolution;
    uint m_uiMinMaxShadowMapResolution;

    BOOL m_bUse1DMinMaxTree;
    float m_fMaxShadowMapStep;
    float m_fMiddleGray;
    uint m_uiLightSctrTechnique;

    int m_iNumCascades;
    int m_iFirstCascade;
    float m_fNumCascades;
    float m_fFirstCascade;

    uint m_uiCascadeProcessingMode;
    uint m_uiRefinementCriterion;
    BOOL m_bIs32BitMinMaxMipMap;
    uint m_uiMultipleScatteringMode;

    uint m_uiSingleScatteringMode;
    BOOL m_bAutoExposure;
    uint m_uiToneMappingMode;
    BOOL m_bLightAdaptation;
    
    float m_fWhitePoint;
    float m_fLuminanceSaturation;
    float2 f2Dummy;
    
    uint m_uiExtinctionEvalMode;
    BOOL m_bUseCustomSctrCoeffs;
    float m_fAerosolDensityScale;
    float m_fAerosolAbsorbtionScale;

    float4 m_f4CustomRlghBeta;
    float4 m_f4CustomMieBeta;

#ifdef __cplusplus
    PostProcessingAttribs() : 
        m_uiNumEpipolarSlices(512),
        m_uiMaxSamplesInSlice(256),
        m_uiInitialSampleStepInSlice(16),
        // Note that sampling near the epipole is very cheap since only a few steps
        // required to perform ray marching
        m_uiEpipoleSamplingDensityFactor(2),
        m_fRefinementThreshold(0.03f),
        m_bShowSampling(FALSE),
        m_bCorrectScatteringAtDepthBreaks(FALSE),
        m_bShowDepthBreaks(FALSE),
        m_bShowLightingOnly(FALSE),
        m_bOptimizeSampleLocations(TRUE),
        m_bEnableLightShafts(TRUE),
        m_uiInstrIntegralSteps(30),
        m_bUse1DMinMaxTree(TRUE),
        m_fMaxShadowMapStep(16.f),
        m_f2ShadowMapTexelSize(0,0),
        m_uiMinMaxShadowMapResolution(0),
        m_fMiddleGray(.18f),
        m_uiLightSctrTechnique(LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING),
        m_iNumCascades(0),
        m_iFirstCascade(1),
        m_fNumCascades(0),
        m_fFirstCascade(1),
        m_uiCascadeProcessingMode(CASCADE_PROCESSING_MODE_SINGLE_PASS),
        m_uiRefinementCriterion(REFINEMENT_CRITERION_INSCTR_DIFF),
        m_bIs32BitMinMaxMipMap(FALSE),
        m_uiMultipleScatteringMode(MULTIPLE_SCTR_MODE_UNOCCLUDED),
        m_uiSingleScatteringMode(SINGLE_SCTR_MODE_LUT),
        m_bAutoExposure(TRUE),
        m_uiToneMappingMode(TONE_MAPPING_MODE_UNCHARTED2),
        m_bLightAdaptation(TRUE),
        m_fWhitePoint(3e+0f),
        m_fLuminanceSaturation(1e+0),
        m_uiExtinctionEvalMode(EXTINCTION_EVAL_MODE_EPIPOLAR),
        m_bUseCustomSctrCoeffs(FALSE),
        m_fAerosolDensityScale(1.f),
        m_fAerosolAbsorbtionScale(0.1f),
        m_f4CustomRlghBeta( 5.8e-6f, 13.5e-6f, 33.1e-6f, 0.f ),
        m_f4CustomMieBeta(2.e-5f, 2.e-5f, 2.e-5f, 0.f)
        {}
#endif
};
CHECK_STRUCT_ALIGNMENT(PostProcessingAttribs)

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

    float fEarthRadius;
    float fAtmTopHeight;
    float2 f2ParticleScaleHeight;
    
    float fTurbidity;
    float fAtmTopRadius;
    float m_fAerosolPhaseFuncG;
    float m_fDummy;


#ifdef __cplusplus
    AirScatteringAttribs():        
        f2ParticleScaleHeight(7994.f, 1200.f),
        // Air molecules and aerosols are assumed to be distributed
        // between 6360 km and 6420 km
        fEarthRadius(6360000.f),
        fAtmTopHeight(80000.f),
        fTurbidity(1.02f),
        m_fAerosolPhaseFuncG(0.76f)
    {
        fAtmTopRadius = fEarthRadius + fAtmTopHeight;
    }
#endif
};

CHECK_STRUCT_ALIGNMENT(AirScatteringAttribs)

struct MiscDynamicParams
{
    float fMaxStepsAlongRay;   // Maximum number of steps during ray tracing
    float fCascadeInd;
    float2 f2WQ; // Used when pre-computing inscattering look-up table

    uint uiDepthSlice;
    float fElapsedTime;
    float2 f2Dummy;

#ifdef __cplusplus
    uint ui4SrcMinMaxLevelXOffset;
    uint ui4SrcMinMaxLevelYOffset;
    uint ui4DstMinMaxLevelXOffset;
    uint ui4DstMinMaxLevelYOffset;
#else
    uint4 ui4SrcDstMinMaxLevelOffset;
#endif
};
CHECK_STRUCT_ALIGNMENT(MiscDynamicParams)

#endif //_STRCUTURES_FXH_
