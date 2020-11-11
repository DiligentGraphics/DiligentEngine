#include "BasicStructures.fxh"
#include "AtmosphereShadersCommon.fxh"

Texture2D<float4> g_tex2DSliceEndPoints;

cbuffer cbLightParams
{
    LightAttribs g_LightAttribs;
}

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
};

#define f4IncorrectSliceUVDirAndStart float4(-10000, -10000, 0, 0)
void RenderSliceUVDirInShadowMapTexturePS(in FullScreenTriangleVSOutput VSOut,
                                          // IMPORTANT: non-system generated pixel shader input
                                          // arguments must have the exact same name as vertex shader 
                                          // outputs and must go in the same order.
                                          // Moreover, even if the shader is not using the argument,
                                          // it still must be declared.

                                          out float4 f4SliceUVDirAndStart : SV_Target )
{
    int iSliceInd = int(VSOut.f4PixelPos.x);
    // Load epipolar slice endpoints
    float4 f4SliceEndpoints = g_tex2DSliceEndPoints.Load(  int3(iSliceInd,0,0) );
    // All correct entry points are completely inside the [-1+1/W,1-1/W]x[-1+1/H,1-1/H] area
    if( !IsValidScreenLocation(f4SliceEndpoints.xy, g_PPAttribs.f4ScreenResolution) )
    {
        f4SliceUVDirAndStart = f4IncorrectSliceUVDirAndStart;
        return;
    }

    int iCascadeInd = int(VSOut.f4PixelPos.y);
    matrix mWorldToShadowMapUVDepth = g_LightAttribs.ShadowAttribs.mWorldToShadowMapUVDepth[iCascadeInd];

    // Reconstruct slice exit point position in world space
    float3 f3SliceExitWS = ProjSpaceXYZToWorldSpace( float3(f4SliceEndpoints.zw, g_LightAttribs.ShadowAttribs.Cascades[iCascadeInd].f4StartEndZ.y), g_CameraAttribs.mProj, g_CameraAttribs.mViewProjInv );
    // Transform it to the shadow map UV
    float2 f2SliceExitUV = WorldSpaceToShadowMapUV(f3SliceExitWS, mWorldToShadowMapUVDepth).xy;
    
    // Compute camera position in shadow map UV space
    float2 f2SliceOriginUV = WorldSpaceToShadowMapUV(g_CameraAttribs.f4Position.xyz, mWorldToShadowMapUVDepth).xy;

    // Compute slice direction in shadow map UV space
    float2 f2SliceDir = f2SliceExitUV - f2SliceOriginUV;
    f2SliceDir /= max(abs(f2SliceDir.x), abs(f2SliceDir.y));
    
    float4 f4BoundaryMinMaxXYXY = float4(0.0, 0.0, 1.0, 1.0) + float4(0.5, 0.5, -0.5, -0.5)*g_PPAttribs.f2ShadowMapTexelSize.xyxy;
    if( any( Less( (f2SliceOriginUV.xyxy - f4BoundaryMinMaxXYXY) * float4( 1.0, 1.0, -1.0, -1.0), float4(0.0, 0.0, 0.0, 0.0) ) ) )
    {
        // If slice origin in UV coordinates falls beyond [0,1]x[0,1] region, we have
        // to continue the ray and intersect it with this rectangle
        //                  
        //    f2SliceOriginUV
        //       *
        //        \
        //         \  New f2SliceOriginUV
        //    1   __\/___
        //       |       |
        //       |       |
        //    0  |_______|
        //       0       1
        //           
        
        // First, compute signed distances from the slice origin to all four boundaries
        bool4 b4IsValidIsecFlag = Greater(abs(f2SliceDir.xyxy), 1e-6 * float4(1.0, 1.0, 1.0, 1.0));
        float4 f4DistToBoundaries = (f4BoundaryMinMaxXYXY - f2SliceOriginUV.xyxy) / (f2SliceDir.xyxy + BoolToFloat( Not(b4IsValidIsecFlag) ) );

        //We consider only intersections in the direction of the ray
        b4IsValidIsecFlag = And( b4IsValidIsecFlag, Greater(f4DistToBoundaries, float4(0.0, 0.0, 0.0, 0.0)) );
        // Compute the second intersection coordinate
        float4 f4IsecYXYX = f2SliceOriginUV.yxyx + f4DistToBoundaries * f2SliceDir.yxyx;
        
        // Select only these coordinates that fall onto the boundary
        b4IsValidIsecFlag = And( b4IsValidIsecFlag, GreaterEqual(f4IsecYXYX, f4BoundaryMinMaxXYXY.yxyx));
        b4IsValidIsecFlag = And( b4IsValidIsecFlag, LessEqual(f4IsecYXYX, f4BoundaryMinMaxXYXY.wzwz) );
        // Replace distances to all incorrect boundaries with the large value
        f4DistToBoundaries = BoolToFloat(b4IsValidIsecFlag) * f4DistToBoundaries + 
                             // It is important to make sure compiler does not use mad here,
                             // otherwise operations with FLT_MAX will lose all precision
                             BoolToFloat( Not(b4IsValidIsecFlag) ) * float4(+FLT_MAX, +FLT_MAX, +FLT_MAX, +FLT_MAX);
        // Select the closest valid intersection
        float2 f2MinDist = min(f4DistToBoundaries.xy, f4DistToBoundaries.zw);
        float fMinDist = min(f2MinDist.x, f2MinDist.y);
        
        // Update origin
        f2SliceOriginUV = f2SliceOriginUV + fMinDist * f2SliceDir;
    }
    
    f2SliceDir *= g_PPAttribs.f2ShadowMapTexelSize;

    f4SliceUVDirAndStart = float4(f2SliceDir, f2SliceOriginUV);
}
