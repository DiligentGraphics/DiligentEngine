#ifndef _BASIC_STRUCTURES_FXH_
#define _BASIC_STRUCTURES_FXH_


#ifdef __cplusplus

#   ifndef BOOL
#      define BOOL int32_t // Do not use bool, because sizeof(bool)==1 !
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


struct CascadeAttribs
{
	float4 f4LightSpaceScale;
	float4 f4LightSpaceScaledBias;
    float4 f4StartEndZ;

    // Cascade margin in light projection space ([-1, +1] x [-1, +1] x [-1(GL) or 0, +1])
    float4 f4MarginProjSpace;
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(CascadeAttribs);
#endif

#define SHADOW_MODE_PCF 1
#define SHADOW_MODE_VSM 2
#define SHADOW_MODE_EVSM2 3
#define SHADOW_MODE_EVSM4 4
#ifndef SHADOW_MODE
#   define SHADOW_MODE SHADOW_MODE_PCF
#endif

#define MAX_CASCADES 8
struct ShadowMapAttribs
{
    // 0
#ifdef __cplusplus
    float4x4 mWorldToLightViewT; // Matrices in HLSL are COLUMN-major while float4x4 is ROW major
#else
    matrix mWorldToLightView;  // Transform from view space to light projection space
#endif
    // 16
    CascadeAttribs Cascades[MAX_CASCADES];

#ifdef __cplusplus
    float4x4 mWorldToShadowMapUVDepthT[MAX_CASCADES];
    float fCascadeCamSpaceZEnd[MAX_CASCADES];
#else
    matrix mWorldToShadowMapUVDepth[MAX_CASCADES];
    float4 f4CascadeCamSpaceZEnd[MAX_CASCADES/4];
#endif

    float4 f4ShadowMapDim;    // Width, Height, 1/Width, 1/Height

    // Number of shadow cascades
    int   iNumCascades                  DEFAULT_VALUE(0);
    float fNumCascades                  DEFAULT_VALUE(0);
    // Do not use bool, because sizeof(bool)==1 !
	BOOL  bVisualizeCascades            DEFAULT_VALUE(0);
    BOOL  bVisualizeShadowing           DEFAULT_VALUE(0);

    float fReceiverPlaneDepthBiasClamp  DEFAULT_VALUE(10);
    float fFixedDepthBias               DEFAULT_VALUE(1e-5f);
    float fCascadeTransitionRegion      DEFAULT_VALUE(0.1f);
    int   iMaxAnisotropy                DEFAULT_VALUE(4);

    float fVSMBias                      DEFAULT_VALUE(1e-4f);
    float fVSMLightBleedingReduction    DEFAULT_VALUE(0);
    float fEVSMPositiveExponent         DEFAULT_VALUE(40);
    float fEVSMNegativeExponent         DEFAULT_VALUE(5);

    BOOL  bIs32BitEVSM                  DEFAULT_VALUE(1);
    int   iFixedFilterSize              DEFAULT_VALUE(3); // 3x3 filter
    float fFilterWorldSize              DEFAULT_VALUE(0);
    bool  fDummy;
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(ShadowMapAttribs);
#endif

struct LightAttribs
{
    float4 f4Direction      DEFAULT_VALUE(float4(0, 0,-1, 0));
    float4 f4AmbientLight   DEFAULT_VALUE(float4(0, 0, 0, 0));
    float4 f4Intensity      DEFAULT_VALUE(float4(1, 1, 1, 1));

    ShadowMapAttribs ShadowAttribs;
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(LightAttribs);
#endif

struct CameraAttribs
{
    float4 f4Position;     // Camera world position
    float4 f4ViewportSize; // (width, height, 1/width, 1/height)

    float2 f2ViewportOrigin; // (min x, min y)
    float fNearPlaneZ; 
    float fFarPlaneZ; // fNearPlaneZ < fFarPlaneZ

#ifdef __cplusplus
    float4x4 mViewT;
    float4x4 mProjT;
    float4x4 mViewProjT;
    float4x4 mViewInvT;
    float4x4 mProjInvT;
    float4x4 mViewProjInvT;
#else
    matrix mView;
    matrix mProj;
    matrix mViewProj;
    matrix mViewInv;
    matrix mProjInv;
    matrix mViewProjInv;
#endif

    float4 f4ExtraData[5]; // Any appliation-specific data
    // Sizeof(CameraAttribs) == 256*2
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(CameraAttribs);
#endif

#endif //_BASIC_STRUCTURES_FXH_
