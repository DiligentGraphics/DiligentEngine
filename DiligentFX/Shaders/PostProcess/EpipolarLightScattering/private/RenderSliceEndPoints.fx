
#include "AtmosphereShadersCommon.fxh"

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
}

// This function computes entry point of the epipolar line given its exit point
//                  
//    g_PPAttribs.f4LightScreenPos
//       *
//        \
//         \  f2EntryPoint
//        __\/___
//       |   \   |
//       |    \  |
//       |_____\_|
//           | |
//           | f2ExitPoint
//           |
//        Exit boundary
float2 GetEpipolarLineEntryPoint(float2 f2ExitPoint)
{
    float2 f2EntryPoint;

    //if( IsValidScreenLocation(g_PPAttribs.f4LightScreenPos.xy) )
    if (g_PPAttribs.bIsLightOnScreen)
    {
        // If light source is on the screen, its location is entry point for each epipolar line
        f2EntryPoint = g_PPAttribs.f4LightScreenPos.xy;
    }
    else
    {
        // If light source is outside the screen, we need to compute intersection of the ray with
        // the screen boundaries
        
        // Compute direction from the light source to the exit point
        // Note that exit point must be located on shrinked screen boundary
        float2 f2RayDir = f2ExitPoint.xy - g_PPAttribs.f4LightScreenPos.xy;
        float fDistToExitBoundary = length(f2RayDir);
        f2RayDir /= fDistToExitBoundary;
        // Compute signed distances along the ray from the light position to all four boundaries
        // The distances are computed as follows using vector instructions:
        // float fDistToLeftBoundary   = abs(f2RayDir.x) > 1e-5 ? (-1 - g_PPAttribs.f4LightScreenPos.x) / f2RayDir.x : -FLT_MAX;
        // float fDistToBottomBoundary = abs(f2RayDir.y) > 1e-5 ? (-1 - g_PPAttribs.f4LightScreenPos.y) / f2RayDir.y : -FLT_MAX;
        // float fDistToRightBoundary  = abs(f2RayDir.x) > 1e-5 ? ( 1 - g_PPAttribs.f4LightScreenPos.x) / f2RayDir.x : -FLT_MAX;
        // float fDistToTopBoundary    = abs(f2RayDir.y) > 1e-5 ? ( 1 - g_PPAttribs.f4LightScreenPos.y) / f2RayDir.y : -FLT_MAX;
        
        // Note that in fact the outermost visible screen pixels do not lie exactly on the boundary (+1 or -1), but are biased by
        // 0.5 screen pixel size inwards. Using these adjusted boundaries improves precision and results in
        // smaller number of pixels which require inscattering correction
        float4 f4Boundaries = GetOutermostScreenPixelCoords(g_PPAttribs.f4ScreenResolution);
        bool4 b4IsCorrectIntersectionFlag = Greater( abs(f2RayDir.xyxy), 1e-5 * float4(1.0, 1.0, 1.0, 1.0) );
        float4 f4DistToBoundaries = (f4Boundaries - g_PPAttribs.f4LightScreenPos.xyxy) / (f2RayDir.xyxy + BoolToFloat( Not(b4IsCorrectIntersectionFlag) ) );
        // Addition of !b4IsCorrectIntersectionFlag is required to prevent divison by zero
        // Note that such incorrect lanes will be masked out anyway

        // We now need to find first intersection BEFORE the intersection with the exit boundary
        // This means that we need to find maximum intersection distance which is less than fDistToBoundary
        // We thus need to skip all boundaries, distance to which is greater than the distance to exit boundary
        // Using -FLT_MAX as the distance to these boundaries will result in skipping them:
        b4IsCorrectIntersectionFlag = And( b4IsCorrectIntersectionFlag, Less( f4DistToBoundaries, (fDistToExitBoundary - 1e-4) * float4(1.0, 1.0, 1.0, 1.0) ) );
        float4 f4CorrectDist   = BoolToFloat( b4IsCorrectIntersectionFlag ) * f4DistToBoundaries;
        // When working with FLT_MAX, we must make sure that the compiler does not use mad instruction, 
        // which will screw things up due to precision issues. DO NOT use 1.0-Flag
        float4 f4IncorrectDist = BoolToFloat( Not(b4IsCorrectIntersectionFlag) ) * float4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);
        f4DistToBoundaries = f4CorrectDist + f4IncorrectDist;
        
        float fFirstIntersecDist = 0.0;
        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.x);
        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.y);
        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.z);
        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.w);
        
        // The code above is equivalent to the following lines:
        // fFirstIntersecDist = fDistToLeftBoundary   < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToLeftBoundary)   : fFirstIntersecDist;
        // fFirstIntersecDist = fDistToBottomBoundary < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToBottomBoundary) : fFirstIntersecDist;
        // fFirstIntersecDist = fDistToRightBoundary  < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToRightBoundary)  : fFirstIntersecDist;
        // fFirstIntersecDist = fDistToTopBoundary    < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToTopBoundary)    : fFirstIntersecDist;

        // Now we can compute entry point:
        f2EntryPoint = g_PPAttribs.f4LightScreenPos.xy + f2RayDir * fFirstIntersecDist;

        // For invalid rays, coordinates are outside [-1,1]x[-1,1] area
        // and such rays will be discarded
        //
        //       g_PPAttribs.f4LightScreenPos
        //             *
        //              \|
        //               \-f2EntryPoint
        //               |\
        //               | \  f2ExitPoint 
        //               |__\/___
        //               |       |
        //               |       |
        //               |_______|
        //
    }

    return f2EntryPoint;
}

float4 GenerateSliceEndpointsPS(FullScreenTriangleVSOutput VSOut
                                // IMPORTANT: non-system generated pixel shader input
                                // arguments must go in the exact same order as VS outputs.
                                // Moreover, even if the shader is not using the argument,
                                // it still must be declared
                                ) : SV_Target
{
    float2 f2UV = NormalizedDeviceXYToTexUV(VSOut.f2NormalizedXY);

    // Note that due to the rasterization rules, UV coordinates are biased by 0.5 texel size.
    //
    //      0.5     1.5     2.5     3.5
    //   |   X   |   X   |   X   |   X   |     ....       
    //   0       1       2       3       4   f2UV * TexDim
    //   X - locations where rasterization happens
    //
    // We need to remove this offset. Also clamp to [0,1] to fix fp32 precision issues
    float fEpipolarSlice = saturate(f2UV.x - 0.5 / float(g_PPAttribs.uiNumEpipolarSlices) );

    // fEpipolarSlice now lies in the range [0, 1 - 1/NUM_EPIPOLAR_SLICES]
    // 0 defines location in exacatly left top corner, 1 - 1/NUM_EPIPOLAR_SLICES defines
    // position on the top boundary next to the top left corner
    uint uiBoundary = uint( clamp(floor( fEpipolarSlice * 4.0 ), 0.0, 3.0) );
    float fPosOnBoundary = frac( fEpipolarSlice * 4.0 );

    bool4 b4BoundaryFlags = bool4( 
            uiBoundary == 0u,
            uiBoundary == 1u,
            uiBoundary == 2u,
            uiBoundary == 3u
        );

    // Note that in fact the outermost visible screen pixels do not lie exactly on the boundary (+1 or -1), but are biased by
    // 0.5 screen pixel size inwards. Using these adjusted boundaries improves precision and results in
    // samller number of pixels which require inscattering correction
    float4 f4OutermostScreenPixelCoords = GetOutermostScreenPixelCoords(g_PPAttribs.f4ScreenResolution);// xyzw = (left, bottom, right, top)

    // Check if there can definitely be no correct intersection with the boundary:
    //  
    //  Light.x <= LeftBnd    Light.y <= BottomBnd     Light.x >= RightBnd     Light.y >= TopBnd    
    //                                                                                 *             
    //          ____                 ____                    ____                   __/_             
    //        .|    |               |    |                  |    |  .*             |    |            
    //      .' |____|               |____|                  |____|.'               |____|            
    //     *                           \                                                               
    //                                  *                                                  
    //     Left Boundary       Bottom Boundary           Right Boundary          Top Boundary 
    //
    bool4 b4IsInvalidBoundary = LessEqual( (g_PPAttribs.f4LightScreenPos.xyxy - f4OutermostScreenPixelCoords.xyzw) * float4(1.0, 1.0, -1.0, -1.0),  float4(0.0, 0.0, 0.0, 0.0) );
    if( dot( BoolToFloat(b4IsInvalidBoundary), BoolToFloat(b4BoundaryFlags) ) != 0.0 )
    {
        return INVALID_EPIPOLAR_LINE;
    }
    // Additinal check above is required to eliminate false epipolar lines which can appear is shown below.
    // The reason is that we have to use some safety delta when performing check in IsValidScreenLocation() 
    // function. If we do not do this, we will miss valid entry points due to precision issues.
    // As a result there could appear false entry points which fall into the safety region, but in fact lie
    // outside the screen boundary:
    //
    //   LeftBnd-Delta LeftBnd           
    //                      false epipolar line
    //          |        |  /
    //          |        | /          
    //          |        |/         X - false entry point
    //          |        *
    //          |       /|
    //          |------X-|-----------  BottomBnd
    //          |     /  |
    //          |    /   |
    //          |___/____|___________ BottomBnd-Delta
    //          
    //          


    //             <------
    //   +1   0,1___________0.75
    //          |     3     |
    //        | |           | A
    //        | |0         2| |
    //        V |           | |
    //   -1     |_____1_____|
    //       0.25  ------>  0.5
    //
    //         -1          +1
    //

    //                                   Left             Bottom           Right              Top   
    float4 f4BoundaryXPos = float4(               0.0, fPosOnBoundary,                1.0, 1.0-fPosOnBoundary);
    float4 f4BoundaryYPos = float4( 1.0-fPosOnBoundary,              0.0,  fPosOnBoundary,                1.0);
    // Select the right coordinates for the boundary
    float2 f2ExitPointPosOnBnd = float2( dot(f4BoundaryXPos, BoolToFloat(b4BoundaryFlags)), dot(f4BoundaryYPos, BoolToFloat(b4BoundaryFlags)) );
    float2 f2ExitPoint = lerp(f4OutermostScreenPixelCoords.xy, f4OutermostScreenPixelCoords.zw, f2ExitPointPosOnBnd);
    // GetEpipolarLineEntryPoint() gets exit point on SHRINKED boundary
    float2 f2EntryPoint = GetEpipolarLineEntryPoint(f2ExitPoint);
    
#if OPTIMIZE_SAMPLE_LOCATIONS
    // If epipolar slice is not invisible, advance its exit point if necessary
    if( IsValidScreenLocation(f2EntryPoint, g_PPAttribs.f4ScreenResolution) )
    {
        // Compute length of the epipolar line in screen pixels:
        float fEpipolarSliceScreenLen = length( (f2ExitPoint - f2EntryPoint) * g_PPAttribs.f4ScreenResolution.xy / 2.0 );
        // If epipolar line is too short, update epipolar line exit point to provide 1:1 texel to screen pixel correspondence:
        f2ExitPoint = f2EntryPoint + (f2ExitPoint - f2EntryPoint) * max(float(g_PPAttribs.uiMaxSamplesInSlice) / fEpipolarSliceScreenLen, 1.0);
    }
#endif

    return float4(f2EntryPoint, f2ExitPoint);
}
