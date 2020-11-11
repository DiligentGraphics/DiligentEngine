// UnwarpEpipolarScattering.fx
// Transforms scattering and extinction from epipolar space to camera space and combines with the
// back buffer

#include "BasicStructures.fxh"
#include "AtmosphereShadersCommon.fxh"

cbuffer cbParticipatingMediaScatteringParams
{
    AirScatteringAttribs g_MediaParams;
}

cbuffer cbLightParams
{
    LightAttribs g_LightAttribs;
}

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
}

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

Texture2D<float4> g_tex2DSliceEndPoints;
SamplerState      g_tex2DSliceEndPoints_sampler; // Linear clamp

Texture2D<float>  g_tex2DEpipolarCamSpaceZ;
SamplerState      g_tex2DEpipolarCamSpaceZ_sampler; // Linear clamp

Texture2D<float3> g_tex2DScatteredColor;
SamplerState      g_tex2DScatteredColor_sampler; // Linear clamp

Texture2D<float>  g_tex2DCamSpaceZ;
SamplerState      g_tex2DCamSpaceZ_sampler; // Linear clamp

Texture2D<float4> g_tex2DColorBuffer;
SamplerState      g_tex2DColorBuffer_sampler; // Point clamp

Texture2D<float>  g_tex2DAverageLuminance;

#if EXTINCTION_EVAL_MODE == EXTINCTION_EVAL_MODE_EPIPOLAR
    Texture2D<float3> g_tex2DEpipolarExtinction;
    SamplerState      g_tex2DEpipolarExtinction_sampler; // Linear clamp
#endif

#include "Extinction.fxh"
#include "ToneMapping.fxh"

void UnwarpEpipolarInsctrImage( in float2 f2PosPS, 
                                in float fCamSpaceZ,
                                out float3 f3Inscattering,
                                out float3 f3Extinction )
{
    // Compute direction of the ray going from the light through the pixel
    float2 f2RayDir = normalize( f2PosPS - g_PPAttribs.f4LightScreenPos.xy );

    // Find, which boundary the ray intersects. For this, we will 
    // find which two of four half spaces the f2RayDir belongs to
    // Each of four half spaces is produced by the line connecting one of four
    // screen corners and the current pixel:
    //    ________________        _______'________           ________________           
    //   |'            . '|      |      '         |         |                |          
    //   | '       . '    |      |     '          |      .  |                |          
    //   |  '  . '        |      |    '           |        '|.        hs1    |          
    //   |   *.           |      |   *     hs0    |         |  '*.           |          
    //   |  '   ' .       |      |  '             |         |      ' .       |          
    //   | '        ' .   |      | '              |         |          ' .   |          
    //   |'____________ '_|      |'_______________|         | ____________ '_.          
    //                           '                                             '
    //                           ________________  .        '________________  
    //                           |             . '|         |'               | 
    //                           |   hs2   . '    |         | '              | 
    //                           |     . '        |         |  '             | 
    //                           | . *            |         |   *            | 
    //                         . '                |         |    '           | 
    //                           |                |         | hs3 '          | 
    //                           |________________|         |______'_________| 
    //                                                              '
    // The equations for the half spaces are the following:
    //bool hs0 = (f2PosPS.x - (-1)) * f2RayDir.y < f2RayDir.x * (f2PosPS.y - (-1));
    //bool hs1 = (f2PosPS.x -  (1)) * f2RayDir.y < f2RayDir.x * (f2PosPS.y - (-1));
    //bool hs2 = (f2PosPS.x -  (1)) * f2RayDir.y < f2RayDir.x * (f2PosPS.y -  (1));
    //bool hs3 = (f2PosPS.x - (-1)) * f2RayDir.y < f2RayDir.x * (f2PosPS.y -  (1));
    // Note that in fact the outermost visible screen pixels do not lie exactly on the boundary (+1 or -1), but are biased by
    // 0.5 screen pixel size inwards. Using these adjusted boundaries improves precision and results in
    // smaller number of pixels which require inscattering correction
    float4 f4Boundaries = GetOutermostScreenPixelCoords(g_PPAttribs.f4ScreenResolution);//left, bottom, right, top
    float4 f4HalfSpaceEquationTerms = (f2PosPS.xxyy - f4Boundaries.xzyw/*float4(-1,1,-1,1)*/) * f2RayDir.yyxx;
    bool4 b4HalfSpaceFlags = Less( f4HalfSpaceEquationTerms.xyyx, f4HalfSpaceEquationTerms.zzww );

    // Now compute mask indicating which of four sectors the f2RayDir belongs to and consiquently
    // which border the ray intersects:
    //    ________________ 
    //   |'            . '|         0 : hs3 && !hs0
    //   | '   3   . '    |         1 : hs0 && !hs1
    //   |  '  . '        |         2 : hs1 && !hs2
    //   |0  *.       2   |         3 : hs2 && !hs3
    //   |  '   ' .       |
    //   | '   1    ' .   |
    //   |'____________ '_|
    //
    bool4 b4SectorFlags = And( b4HalfSpaceFlags.wxyz, Not(b4HalfSpaceFlags.xyzw) );
    // Note that b4SectorFlags now contains true (1) for the exit boundary and false (0) for 3 other

    // Compute distances to boundaries according to following lines:
    //float fDistToLeftBoundary   = abs(f2RayDir.x) > 1e-5 ? ( -1 - g_PPAttribs.f4LightScreenPos.x) / f2RayDir.x : -FLT_MAX;
    //float fDistToBottomBoundary = abs(f2RayDir.y) > 1e-5 ? ( -1 - g_PPAttribs.f4LightScreenPos.y) / f2RayDir.y : -FLT_MAX;
    //float fDistToRightBoundary  = abs(f2RayDir.x) > 1e-5 ? (  1 - g_PPAttribs.f4LightScreenPos.x) / f2RayDir.x : -FLT_MAX;
    //float fDistToTopBoundary    = abs(f2RayDir.y) > 1e-5 ? (  1 - g_PPAttribs.f4LightScreenPos.y) / f2RayDir.y : -FLT_MAX;
    float4 f4DistToBoundaries = ( f4Boundaries - g_PPAttribs.f4LightScreenPos.xyxy ) / (f2RayDir.xyxy + BoolToFloat( Less( abs(f2RayDir.xyxy), 1e-6 * float4(1.0, 1.0, 1.0, 1.0)) ) );
    // Select distance to the exit boundary:
    float fDistToExitBoundary = dot( BoolToFloat( b4SectorFlags ), f4DistToBoundaries );
    // Compute exit point on the boundary:
    float2 f2ExitPoint = g_PPAttribs.f4LightScreenPos.xy + f2RayDir * fDistToExitBoundary;

    // Compute epipolar slice for each boundary:
    //if( LeftBoundary )
    //    fEpipolarSlice = 0.0  - (LeftBoudaryIntersecPoint.y   -   1 )/2 /4;
    //else if( BottomBoundary )
    //    fEpipolarSlice = 0.25 + (BottomBoudaryIntersecPoint.x - (-1))/2 /4;
    //else if( RightBoundary )
    //    fEpipolarSlice = 0.5  + (RightBoudaryIntersecPoint.y  - (-1))/2 /4;
    //else if( TopBoundary )
    //    fEpipolarSlice = 0.75 - (TopBoudaryIntersecPoint.x      - 1 )/2 /4;
    float4 f4EpipolarSlice = float4(0, 0.25, 0.5, 0.75) + 
        saturate( (f2ExitPoint.yxyx - f4Boundaries.wxyz)*float4(-1.0, +1.0, +1.0, -1.0) / (f4Boundaries.wzwz - f4Boundaries.yxyx) ) / 4.0;
    // Select the right value:
    float fEpipolarSlice = dot( BoolToFloat(b4SectorFlags), f4EpipolarSlice);

    // Now find two closest epipolar slices, from which we will interpolate
    // First, find index of the slice which precedes our slice
    // Note that 0 <= fEpipolarSlice <= 1, and both 0 and 1 refer to the first slice
    float fPrecedingSliceInd = min( floor(fEpipolarSlice * float(g_PPAttribs.uiNumEpipolarSlices)), float(g_PPAttribs.uiNumEpipolarSlices-1u) );

    // Compute EXACT texture coordinates of preceding and succeeding slices and their weights
    // Note that slice 0 is stored in the first texel which has exact texture coordinate 0.5/NUM_EPIPOLAR_SLICES
    // (search for "fEpipolarSlice = saturate(f2UV.x - 0.5f / (float)NUM_EPIPOLAR_SLICES)"):
    float fSrcSliceV[2];
    // Compute V coordinate to refer exactly the center of the slice row
    fSrcSliceV[0] = fPrecedingSliceInd/float(g_PPAttribs.uiNumEpipolarSlices) + 0.5/float(g_PPAttribs.uiNumEpipolarSlices);
    // Use frac() to wrap around to the first slice from the next-to-last slice:
    fSrcSliceV[1] = frac( fSrcSliceV[0] + 1.0/float(g_PPAttribs.uiNumEpipolarSlices) );
        
    // Compute slice weights
    float fSliceWeights[2];
    fSliceWeights[1] = (fEpipolarSlice*float(g_PPAttribs.uiNumEpipolarSlices)) - fPrecedingSliceInd;
    fSliceWeights[0] = 1.0 - fSliceWeights[1];

    f3Inscattering = float3(0.0, 0.0, 0.0);
    f3Extinction   = float3(0.0, 0.0, 0.0);
    float fTotalWeight = 0.0;
    [unroll]
    for(int i=0; i<2; ++i)
    {
        // Load epipolar line endpoints
        float4 f4SliceEndpoints = g_tex2DSliceEndPoints.SampleLevel( g_tex2DSliceEndPoints_sampler, float2(fSrcSliceV[i], 0.5), 0 );

        // Compute line direction on the screen
        float2 f2SliceDir = f4SliceEndpoints.zw - f4SliceEndpoints.xy;
        float fSliceLenSqr = dot(f2SliceDir, f2SliceDir);
        
        // Project current pixel onto the epipolar line
        float fSamplePosOnLine = dot((f2PosPS - f4SliceEndpoints.xy), f2SliceDir) / max(fSliceLenSqr, 1e-8);
        // Compute index of the slice on the line
        // Note that the first sample on the line (fSamplePosOnLine==0) is exactly the Entry Point, while 
        // the last sample (fSamplePosOnLine==1) is exactly the Exit Point
        // (search for "fSamplePosOnEpipolarLine *= (float)MAX_SAMPLES_IN_SLICE / ((float)MAX_SAMPLES_IN_SLICE-1.f)")
        float fSampleInd = fSamplePosOnLine * float(g_PPAttribs.uiMaxSamplesInSlice-1u);
       
        // We have to manually perform bilateral filtering of the scattered radiance texture to
        // eliminate artifacts at depth discontinuities

        float fPrecedingSampleInd = floor(fSampleInd);
        // Get bilinear filtering weight
        float fUWeight = fSampleInd - fPrecedingSampleInd;
        // Get texture coordinate of the left source texel. Again, offset by 0.5 is essential
        // to align with the texel center
        float fPrecedingSampleU = (fPrecedingSampleInd + 0.5) / float(g_PPAttribs.uiMaxSamplesInSlice);
    
        float2 f2SctrColorUV = float2(fPrecedingSampleU, fSrcSliceV[i]);

        // Gather 4 camera space z values
        // Note that we need to bias f2SctrColorUV by 0.5 texel size to refer the location between all four texels and
        // get the required values for sure
        // The values in float4, which Gather() returns are arranged as follows:
        //   _______ _______
        //  |       |       |
        //  |   x   |   y   |
        //  |_______o_______|  o gather location
        //  |       |       |
        //  |   *w  |   z   |  * f2SctrColorUV
        //  |_______|_______|
        //  |<----->|
        //     1/f2ScatteredColorTexDim.x
        
        // x == g_tex2DEpipolarCamSpaceZ.SampleLevel(samPointClamp, f2SctrColorUV, 0, int2(0,1))
        // y == g_tex2DEpipolarCamSpaceZ.SampleLevel(samPointClamp, f2SctrColorUV, 0, int2(1,1))
        // z == g_tex2DEpipolarCamSpaceZ.SampleLevel(samPointClamp, f2SctrColorUV, 0, int2(1,0))
        // w == g_tex2DEpipolarCamSpaceZ.SampleLevel(samPointClamp, f2SctrColorUV, 0, int2(0,0))

        float2 f2ScatteredColorTexDim = float2(g_PPAttribs.uiMaxSamplesInSlice, g_PPAttribs.uiNumEpipolarSlices);
        float2 f2SrcLocationsCamSpaceZ = g_tex2DEpipolarCamSpaceZ.Gather(g_tex2DEpipolarCamSpaceZ_sampler, f2SctrColorUV + float2(0.5, 0.5) / f2ScatteredColorTexDim.xy).wz;
        
        // Compute depth weights in a way that if the difference is less than the threshold, the weight is 1 and
        // the weights fade out to 0 as the difference becomes larger than the threshold:
        float2 f2MaxZ = max( f2SrcLocationsCamSpaceZ, max(fCamSpaceZ,1.0) );
        float2 f2DepthWeights = saturate( g_PPAttribs.fRefinementThreshold / max( abs(fCamSpaceZ-f2SrcLocationsCamSpaceZ)/f2MaxZ, g_PPAttribs.fRefinementThreshold ) );
        // Note that if the sample is located outside the [-1,1]x[-1,1] area, the sample is invalid and fCurrCamSpaceZ == fInvalidCoordinate
        // Depth weight computed for such sample will be zero
        f2DepthWeights = pow(f2DepthWeights, float2(4.0, 4.0));

        // Multiply bilinear weights with the depth weights:
        float2 f2BilateralUWeights = float2(1.0-fUWeight, fUWeight) * f2DepthWeights * fSliceWeights[i];
        // If the sample projection is behind [0,1], we have to discard this slice
        // We however must take into account the fact that if at least one sample from the two 
        // bilinear sources is correct, the sample can still be properly computed
        //        
        //            -1       0       1                  N-2     N-1      N              Sample index
        // |   X   |   X   |   X   |   X   |  ......   |   X   |   X   |   X   |   X   |
        //         1-1/(N-1)   0    1/(N-1)                        1   1+1/(N-1)          fSamplePosOnLine   
        //             |                                                   |
        //             |<-------------------Clamp range------------------->|                   
        //
        f2BilateralUWeights *= (abs(fSamplePosOnLine - 0.5) < 0.5 + 1.0 / float(g_PPAttribs.uiMaxSamplesInSlice-1u)) ? 1.0 : 0.0;
        // We now need to compute the following weighted summ:
        //f3FilteredSliceCol = 
        //    f2BilateralUWeights.x * g_tex2DScatteredColor.SampleLevel(samPoint, f2SctrColorUV, 0, int2(0,0)) +
        //    f2BilateralUWeights.y * g_tex2DScatteredColor.SampleLevel(samPoint, f2SctrColorUV, 0, int2(1,0));

        // We will use hardware to perform bilinear filtering and get this value using single bilinear fetch:

        // Offset:                  (x=1,y=0)                (x=1,y=0)               (x=0,y=0)
        float fSubpixelUOffset = f2BilateralUWeights.y / max(f2BilateralUWeights.x + f2BilateralUWeights.y, 0.001);
        fSubpixelUOffset /= f2ScatteredColorTexDim.x;
        
        float3 f3FilteredSliceInsctr = 
            (f2BilateralUWeights.x + f2BilateralUWeights.y) * 
                g_tex2DScatteredColor.SampleLevel(g_tex2DScatteredColor_sampler, f2SctrColorUV + float2(fSubpixelUOffset, 0), 0);
        f3Inscattering += f3FilteredSliceInsctr;

#if EXTINCTION_EVAL_MODE == EXTINCTION_EVAL_MODE_EPIPOLAR
        float3 f3FilteredSliceExtinction = 
            (f2BilateralUWeights.x + f2BilateralUWeights.y) * 
                g_tex2DEpipolarExtinction.SampleLevel(g_tex2DEpipolarExtinction_sampler, f2SctrColorUV + float2(fSubpixelUOffset, 0), 0);
        f3Extinction += f3FilteredSliceExtinction;
#endif

        // Update total weight
        fTotalWeight += dot(f2BilateralUWeights, float2(1.0, 1.0));
    }

#if CORRECT_INSCATTERING_AT_DEPTH_BREAKS
    if( fTotalWeight < 1e-2 )
    {
        // Discarded pixels will keep 0 value in stencil and will be later
        // processed to correct scattering
        discard;
    }
#endif
    
    f3Inscattering /= fTotalWeight;
    f3Extinction /= fTotalWeight;
}


void ApplyInscatteredRadiancePS(FullScreenTriangleVSOutput VSOut,
                                // IMPORTANT: non-system generated pixel shader input
                                // arguments must have the exact same name as vertex shader 
                                // outputs and must go in the same order.
                                // Moreover, even if the shader is not using the argument,
                                // it still must be declared.

                                out float4 f4Color : SV_Target)
{
    float2 f2UV = NormalizedDeviceXYToTexUV(VSOut.f2NormalizedXY);
    float fCamSpaceZ = g_tex2DCamSpaceZ.SampleLevel(g_tex2DCamSpaceZ_sampler, f2UV, 0);
    
    float3 f3Inscttering, f3Extinction;
    UnwarpEpipolarInsctrImage(VSOut.f2NormalizedXY, fCamSpaceZ, f3Inscttering, f3Extinction);

    float3 f3BackgroundColor = float3(0.0, 0.0, 0.0);
    [branch]
    if( !g_PPAttribs.bShowLightingOnly )
    {
        f3BackgroundColor = g_tex2DColorBuffer.SampleLevel( g_tex2DColorBuffer_sampler, f2UV, 0).rgb;
        // fFarPlaneZ is pre-multiplied with 0.999999f
        f3BackgroundColor *= (fCamSpaceZ > g_CameraAttribs.fFarPlaneZ) ? g_LightAttribs.f4Intensity.rgb : float3(1.0, 1.0, 1.0);

#if EXTINCTION_EVAL_MODE == EXTINCTION_EVAL_MODE_PER_PIXEL
        float3 f3ReconstructedPosWS = ProjSpaceXYZToWorldSpace(float3(VSOut.f2NormalizedXY.xy, fCamSpaceZ), g_CameraAttribs.mProj, g_CameraAttribs.mViewProjInv);
        f3Extinction = GetExtinction(g_CameraAttribs.f4Position.xyz, f3ReconstructedPosWS, g_PPAttribs.f4EarthCenter.xyz,
                                     g_MediaParams.fAtmBottomRadius, g_MediaParams.fAtmTopRadius, g_MediaParams.f4ParticleScaleHeight);
#endif
        f3BackgroundColor *= f3Extinction;
    }

#if PERFORM_TONE_MAPPING
    float fAveLogLum = GetAverageSceneLuminance(g_tex2DAverageLuminance);
    f4Color.rgb = ToneMap(f3BackgroundColor + f3Inscttering, g_PPAttribs.ToneMapping, fAveLogLum);
#else
    const float MinLumn = 0.01;
    float2 LogLum_W = GetWeightedLogLum(f3BackgroundColor + f3Inscttering, MinLumn);
    f4Color.rgb = float3(LogLum_W.x, LogLum_W.y, 0.0);
#endif
    f4Color.a = 1.0;
}
