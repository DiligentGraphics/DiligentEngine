#ifndef _ATMOSPHERE_SHADERS_COMMON_FXH_
#define _ATMOSPHERE_SHADERS_COMMON_FXH_

#include "FullScreenTriangleVSOutput.fxh"
#include "ToneMappingStructures.fxh"
#include "EpipolarLightScatteringStructures.fxh"
#include "ShaderUtilities.fxh"

#define PI      3.1415928
#define FLT_MAX 3.402823466e+38

#ifndef OPTIMIZE_SAMPLE_LOCATIONS
#   define OPTIMIZE_SAMPLE_LOCATIONS 1
#endif

#ifndef CORRECT_INSCATTERING_AT_DEPTH_BREAKS
#   define CORRECT_INSCATTERING_AT_DEPTH_BREAKS 0
#endif

//#define SHADOW_MAP_DEPTH_BIAS 1e-4

#ifndef ENABLE_LIGHT_SHAFTS
#   define ENABLE_LIGHT_SHAFTS 1
#endif

#ifndef USE_1D_MIN_MAX_TREE
#   define USE_1D_MIN_MAX_TREE 1
#endif

#ifndef IS_32BIT_MIN_MAX_MAP
#   define IS_32BIT_MIN_MAX_MAP 0
#endif

#ifndef SINGLE_SCATTERING_MODE
#   define SINGLE_SCATTERING_MODE SINGLE_SCTR_MODE_INTEGRATION
#endif

#ifndef MULTIPLE_SCATTERING_MODE
#   define MULTIPLE_SCATTERING_MODE MULTIPLE_SCTR_MODE_NONE
#endif

#ifndef PRECOMPUTED_SCTR_LUT_DIM
#   define PRECOMPUTED_SCTR_LUT_DIM float4(32.0, 128.0, 32.0, 16.0)
#endif

#ifndef NUM_RANDOM_SPHERE_SAMPLES
#   define NUM_RANDOM_SPHERE_SAMPLES 128
#endif

#ifndef PERFORM_TONE_MAPPING
#   define PERFORM_TONE_MAPPING 1
#endif

#ifndef LOW_RES_LUMINANCE_MIPS
#   define LOW_RES_LUMINANCE_MIPS 7
#endif

#ifndef TONE_MAPPING_MODE
#   define TONE_MAPPING_MODE TONE_MAPPING_MODE_REINHARD_MOD
#endif

#ifndef LIGHT_ADAPTATION
#   define LIGHT_ADAPTATION 1
#endif

#ifndef CASCADE_PROCESSING_MODE
#   define CASCADE_PROCESSING_MODE CASCADE_PROCESSING_MODE_SINGLE_PASS
#endif

#ifndef USE_COMBINED_MIN_MAX_TEXTURE
#   define USE_COMBINED_MIN_MAX_TEXTURE 1
#endif

#ifndef EXTINCTION_EVAL_MODE
#   define EXTINCTION_EVAL_MODE EXTINCTION_EVAL_MODE_EPIPOLAR
#endif

#ifndef AUTO_EXPOSURE
#   define AUTO_EXPOSURE 1
#endif

#define INVALID_EPIPOLAR_LINE float4(-1000.0, -1000.0, -100.0, -100.0)

#define RGB_TO_LUMINANCE float3(0.212671, 0.715160, 0.072169)

// GLSL compiler is so bad that it cannot properly handle matrices passed as structure members!
float3 ProjSpaceXYZToWorldSpace(in float3 f3PosPS, in float4x4 mProj, in float4x4 mViewProjInv /*CameraAttribs CamAttribs <- DO NOT DO THIS*/)
{
    // We need to compute normalized device z before applying view-proj inverse matrix

    // It does not matter if we are in HLSL or GLSL. The way normalized device
    // coordinates are computed is the same in both APIs - simply transform by
    // matrix and then divide by w. Consequently, the inverse transform is also 
    // the same.
    // What differs is that in GL, NDC z is transformed from [-1,+1] to [0,1]
    // before storing in the depth buffer, which we will have to inverse.
    float fNDC_Z = CameraZToNormalizedDeviceZ(f3PosPS.z, mProj);
    float4 ReconstructedPosWS = mul( float4(f3PosPS.xy, fNDC_Z, 1.0), mViewProjInv );
    ReconstructedPosWS /= ReconstructedPosWS.w;
    return ReconstructedPosWS.xyz;
}

float3 WorldSpaceToShadowMapUV(in float3 f3PosWS, in matrix mWorldToShadowMapUVDepth)
{
    float4 f4ShadowMapUVDepth = mul( float4(f3PosWS, 1), mWorldToShadowMapUVDepth );
    // Shadow map projection matrix is orthographic, so we do not need to divide by w
    //f4ShadowMapUVDepth.xyz /= f4ShadowMapUVDepth.w;
    
    // Applying depth bias results in light leaking through the opaque objects when looking directly
    // at the light source
    return f4ShadowMapUVDepth.xyz;
}


void GetRaySphereIntersection(in  float3 f3RayOrigin,
                              in  float3 f3RayDirection,
                              in  float3 f3SphereCenter,
                              in  float  fSphereRadius,
                              out float2 f2Intersections)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    f3RayOrigin -= f3SphereCenter;
    float A = dot(f3RayDirection, f3RayDirection);
    float B = 2.0 * dot(f3RayOrigin, f3RayDirection);
    float C = dot(f3RayOrigin,f3RayOrigin) - fSphereRadius*fSphereRadius;
    float D = B*B - 4.0*A*C;
    // If discriminant is negative, there are no real roots hence the ray misses the
    // sphere
    if (D < 0.0)
    {
        f2Intersections = float2(-1.0, -1.0);
    }
    else
    {
        D = sqrt(D);
        f2Intersections = float2(-B - D, -B + D) / (2.0*A); // A must be positive here!!
    }
}

void GetRaySphereIntersection2(in  float3 f3RayOrigin,
                               in  float3 f3RayDirection,
                               in  float3 f3SphereCenter,
                               in  float2 f2SphereRadius,
                               out float4 f4Intersections)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    f3RayOrigin -= f3SphereCenter;
    float A = dot(f3RayDirection, f3RayDirection);
    float B = 2.0 * dot(f3RayOrigin, f3RayDirection);
    float2 C = dot(f3RayOrigin,f3RayOrigin) - f2SphereRadius*f2SphereRadius;
    float2 D = B*B - 4.0*A*C;
    // If discriminant is negative, there are no real roots hence the ray misses the
    // sphere
    float2 f2RealRootMask = float2(D.x >= 0.0 ? 1.0 : 0.0, D.y >= 0.0 ? 1.0 : 0.0);
    D = sqrt( max(D,0.0) );
    f4Intersections =   f2RealRootMask.xxyy * float4(-B - D.x, -B + D.x, -B - D.y, -B + D.y) / (2.0*A) + 
                      (float4(1.0, 1.0, 1.0, 1.0) - f2RealRootMask.xxyy) * float4(-1.0, -1.0, -1.0, -1.0);
}


float4 GetOutermostScreenPixelCoords(float4 ScreenResolution)
{
    // The outermost visible screen pixels centers do not lie exactly on the boundary (+1 or -1), but are biased by
    // 0.5 screen pixel size inwards
    //
    //                                        2.0
    //    |<---------------------------------------------------------------------->|
    //
    //       2.0/Res
    //    |<--------->|
    //    |     X     |      X     |     X     |    ...    |     X     |     X     |
    //   -1     |                                                            |    +1
    //          |                                                            |
    //          |                                                            |
    //      -1 + 1.0/Res                                                  +1 - 1.0/Res
    return float4(-1.0, -1.0, 1.0, 1.0) + float4(1.0, 1.0, -1.0, -1.0) * ScreenResolution.zwzw;
}


// When checking if a point is inside the screen, we must test against 
// the biased screen boundaries 
bool IsValidScreenLocation(in float2 f2XY, float4 ScreenResolution)
{
    const float2 SAFETY_EPSILON = float2(0.2, 0.2);
    return all( LessEqual( abs(f2XY), float2(1.0, 1.0) - (float2(1.0, 1.0) - SAFETY_EPSILON) * ScreenResolution.zw ) );
}

float GetAverageSceneLuminance(in Texture2D<float> tex2DAverageLuminance)
{
#if AUTO_EXPOSURE
    float fAveLogLum = tex2DAverageLuminance.Load( int3(0,0,0) );
#else
    float fAveLogLum =  0.1;
#endif
    fAveLogLum = max(0.05, fAveLogLum); // Average luminance is an approximation to the key of the scene
    return fAveLogLum;
}

float2 GetWeightedLogLum(float3 Color, float MinLuminance)
{
    float Luminance = dot(Color, RGB_TO_LUMINANCE);
    float LumWeight = saturate((Luminance - MinLuminance) / MinLuminance);
    float LogLum = log(max(Luminance, 1e-5));
    return float2(LogLum * LumWeight, LumWeight);
}

#endif //_ATMOSPHERE_SHADERS_COMMON_FXH_
