// RefineSampleLocations.fx
// Refines sample locations using inscattering difference or z difference

#include "BasicStructures.fxh"
#include "AtmosphereShadersCommon.fxh"

// In fact we only need RG16U texture to store
// interpolation source indices. However, NVidia GLES does
// not supported imge load/store operations on this format, 
// so we have to resort to RGBA32U.
RWTexture2D<uint2/*format = rgba32ui*/> g_rwtex2DInterpolationSource;

Texture2D<float2> g_tex2DCoordinates;
Texture2D<float>  g_tex2DEpipolarCamSpaceZ;
Texture2D<float3> g_tex2DScatteredColor;
Texture2D<float>  g_tex2DAverageLuminance;

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
};

#include "ToneMapping.fxh"

#ifndef INITIAL_SAMPLE_STEP
#   define INITIAL_SAMPLE_STEP 128
#endif

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE max(INITIAL_SAMPLE_STEP, 32)
#endif

#ifndef REFINEMENT_CRITERION
#   define REFINEMENT_CRITERION REFINEMENT_CRITERION_INSCTR_DIFF
#endif

// In my first implementation I used group shared memory to store camera space z
// values. This was a very low-performing method
// After that I tried using arrays of bool flags instead, but this did not help very much
// since memory bandwidth was almost the same (on GPU each bool consumes 4 bytes)
// Finally, I came up with packing 32 flags into single uint value.
// This not only enables using 32x times less memory, but also enables very efficient
// test if depth break is present in the section
#define NUM_PACKED_FLAGS (THREAD_GROUP_SIZE/32)
groupshared uint g_uiPackedCamSpaceDiffFlags[ NUM_PACKED_FLAGS ];
#if REFINEMENT_CRITERION == REFINEMENT_CRITERION_INSCTR_DIFF
groupshared float3 g_f3Inscattering[THREAD_GROUP_SIZE+1];
#endif

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void RefineSampleLocationsCS(uint3 Gid  : SV_GroupID, 
                             uint3 GTid : SV_GroupThreadID)
{
    // Each thread group processes one slice
    uint uiSliceInd = Gid.y;
    // Compute global index of the first sample in the thread group
    // Each group processes THREAD_GROUP_SIZE samples in the slice
    uint uiGroupStartGlobalInd = Gid.x * uint(THREAD_GROUP_SIZE);
    uint uiSampleInd = GTid.x; // Sample index in the group
    // Compute global index of this sample which is required to fetch the sample's coordinates
    uint uiGlobalSampleInd = uiGroupStartGlobalInd + uiSampleInd;
    // Load location of the current sample using global sample index
    float2 f2SampleLocationPS = g_tex2DCoordinates.Load( int3(uiGlobalSampleInd, uiSliceInd, 0) );
    
    bool bIsValidThread = all( Less( abs(f2SampleLocationPS), (1.0 + 1e-4)*float2(1.0,1.0) ) );

    // Initialize flags with zeroes
    if( GTid.x < uint(NUM_PACKED_FLAGS) )
        g_uiPackedCamSpaceDiffFlags[GTid.x] = 0u;
    
    GroupMemoryBarrierWithGroupSync();

    // Let each thread in the group compute its own flag
    // Note that if the sample is located behind the screen, its flag will be set to zero
    // Besides, since g_tex2DEpipolarCamSpaceZ is cleared with invalid coordinates, the difference
    // flag between valid and invalid locations will also be zero. Thus the sample next to invalid will always
    // be marked as ray marching sample
    [branch]
    if( bIsValidThread )
    {
#if REFINEMENT_CRITERION == REFINEMENT_CRITERION_DEPTH_DIFF
        // Load camera space Z for this sample and for its right neighbour (remeber to use global sample index)
        float fCamSpaceZ =            g_tex2DEpipolarCamSpaceZ.Load( int3(uiGlobalSampleInd,         uiSliceInd, 0) );
        float fRightNeighbCamSpaceZ = g_tex2DEpipolarCamSpaceZ.Load( int3( int(uiGlobalSampleInd)+1, uiSliceInd, 0) );
        float fMaxZ = max(fCamSpaceZ, fRightNeighbCamSpaceZ);
        fMaxZ = max(fMaxZ, 1.0);
        // Compare the difference with the threshold
        bool bFlag = abs(fCamSpaceZ - fRightNeighbCamSpaceZ)/fMaxZ < 0.2*g_PPAttribs.fRefinementThreshold;
#elif REFINEMENT_CRITERION == REFINEMENT_CRITERION_INSCTR_DIFF
        // Load inscattering for this sample and for its right neighbour
        float3 f3Insctr0 = g_tex2DScatteredColor.Load( int3(uiGlobalSampleInd,         uiSliceInd, 0) );
        float3 f3Insctr1 = g_tex2DScatteredColor.Load( int3( int(uiGlobalSampleInd)+1, uiSliceInd, 0) );
        float3 f3MaxInsctr = max(f3Insctr0, f3Insctr1);
        
        // Compute minimum inscattering threshold based on the average scene luminance
        float fAverageLum = GetAverageSceneLuminance(g_tex2DAverageLuminance);
        // Inscattering threshold should be proportional to the average scene luminance and
        // inversely proportional to the middle gray level (the higher middle gray, the briter the scene,
        // thus the less the theshold)
        // It should also account for the fact that rgb channels contribute differently
        // to the percieved brightness. For r channel the threshold should be smallest, 
        // for b channel - the largest
        float3 f3MinInsctrThreshold = (0.02 * fAverageLum / RGB_TO_LUMINANCE.xyz) / g_PPAttribs.ToneMapping.fMiddleGray;

        f3MaxInsctr = max(f3MaxInsctr, f3MinInsctrThreshold);
        // Compare the difference with the threshold. If the neighbour sample is invalid, its inscattering
        // is large negative value and the difference is guaranteed to be larger than the threshold
        bool bFlag = all( Less(abs(f3Insctr0 - f3Insctr1)/f3MaxInsctr, g_PPAttribs.fRefinementThreshold*float3(1.0,1.0,1.0) ) );
#endif
        // Set appropriate flag using INTERLOCKED Or:
        uint uiBit = bFlag ? (1u << (uiSampleInd % 32u)) : 0u;
        InterlockedOr( g_uiPackedCamSpaceDiffFlags[int(uiSampleInd)/32], uiBit );
    }

    // Synchronize threads in the group
    GroupMemoryBarrierWithGroupSync();
    
    // Skip invalid threads. This can be done only after the synchronization
    if( !bIsValidThread )
        return;

    //                                 uiInitialSampleStep
    //       uiSampleInd             |<--------->|
    //          |                    |           |
    //       X  *  *  *  X  *  *  *  X  *  *  *  X           X - locations of initial samples
    //       |           |
    //       |           uiInitialSample1Ind
    //      uiInitialSample0Ind
    //
    // Find two closest initial ray marching samples
    uint uiInitialSampleStep = uint(INITIAL_SAMPLE_STEP);
    uint uiInitialSample0Ind = (uiSampleInd / uiInitialSampleStep) * uiInitialSampleStep;
    // Use denser sampling near the epipole to account for high variation
    // Note that sampling near the epipole is very cheap since only a few steps
    // are required to perform ray marching
    uint uiInitialSample0GlobalInd = uiInitialSample0Ind + uiGroupStartGlobalInd;
    float2 f2InitialSample0Coords = g_tex2DCoordinates.Load( int3(uiInitialSample0GlobalInd, uiSliceInd, 0) );
    if( float(uiInitialSample0GlobalInd)/float(g_PPAttribs.uiMaxSamplesInSlice) < 0.05 && 
        length(f2InitialSample0Coords - g_PPAttribs.f4LightScreenPos.xy) < 0.1 )
    {
        uiInitialSampleStep = max( uint(INITIAL_SAMPLE_STEP) / g_PPAttribs.uiEpipoleSamplingDensityFactor, 1u );
        uiInitialSample0Ind = (uiSampleInd / uiInitialSampleStep) * uiInitialSampleStep;
    }
    uint uiInitialSample1Ind = uiInitialSample0Ind + uiInitialSampleStep;

    // Remeber that the last sample in each epipolar slice must be ray marching one
    uint uiInterpolationTexWidth, uiInterpolationTexHeight;
    g_rwtex2DInterpolationSource.GetDimensions(uiInterpolationTexWidth, uiInterpolationTexHeight);
    if( Gid.x == uiInterpolationTexWidth/uint(THREAD_GROUP_SIZE) - 1u )
        uiInitialSample1Ind = min(uiInitialSample1Ind, uint(THREAD_GROUP_SIZE-1) );

    uint uiLeftSrcSampleInd  = uiSampleInd;
    uint uiRightSrcSampleInd = uiSampleInd;

    // Do nothing if sample is one of initial samples. In this case the sample will be 
    // interpolated from itself
    if( uiSampleInd > uiInitialSample0Ind && uiSampleInd < uiInitialSample1Ind )
    {
        // Load group shared memory to the thread local memory
        uint uiPackedCamSpaceDiffFlags[ NUM_PACKED_FLAGS ];
        for(int i=0; i < NUM_PACKED_FLAGS; ++i)
            uiPackedCamSpaceDiffFlags[i] = g_uiPackedCamSpaceDiffFlags[i];
    
        // Check if there are no depth breaks in the whole section
        // In such case all the flags are set
        bool bNoDepthBreaks = true;
#if INITIAL_SAMPLE_STEP < 32
        {
            // Check if all uiInitialSampleStep flags starting from
            // position uiInitialSample0Ind are set:
            int iFlagPackOrder = int(uiInitialSample0Ind / 32u);
            int iFlagOrderInPack = int(uiInitialSample0Ind % 32u);
            uint uiFlagPack = uiPackedCamSpaceDiffFlags[iFlagPackOrder];
            uint uiAllFlagsMask = ((1u<<uiInitialSampleStep) - 1u);
            if( ((uiFlagPack >> uint(iFlagOrderInPack)) & uiAllFlagsMask) != uiAllFlagsMask )
                bNoDepthBreaks = false;
        }
#else
        {
            for(int i=0; i < NUM_PACKED_FLAGS; ++i)
                if( uiPackedCamSpaceDiffFlags[i] != 0xFFFFFFFFU )
                    // If at least one flag is not set, there is a depth break on this section
                    bNoDepthBreaks = false;
        }
#endif

        if( bNoDepthBreaks )
        {
            // If there are no depth breaks, we can skip all calculations
            // and use initial sample locations as interpolation sources:
            uiLeftSrcSampleInd = uiInitialSample0Ind;
            uiRightSrcSampleInd = uiInitialSample1Ind;
        }
        else
        {
            // Find left interpolation source
            {
                // Note that i-th flag reflects the difference between i-th and (i+1)-th samples:
                // Flag[i] = abs(fCamSpaceZ[i] - fCamSpaceZ[i+1]) < g_PPAttribs.fRefinementThreshold;
                // We need to find first depth break starting from iFirstDepthBreakToTheLeftInd sample
                // and going to the left up to uiInitialSample0Ind
                int iFirstDepthBreakToTheLeftInd = int(uiSampleInd)-1;
                //                                                              iFirstDepthBreakToTheLeftInd
                //                                                                     |
                //                                                                     V
                //      0  1  2  3                       30 31   32 33     ....   i-1  i  i+1 ....  63   64
                //   |                                         |                           1  1  1  1  |
                //          uiPackedCamSpaceDiffFlags[0]             uiPackedCamSpaceDiffFlags[1]
                //
                //   iFlagOrderInPack == i % 32

                int iFlagPackOrder = int( uint(iFirstDepthBreakToTheLeftInd) / 32u );
                int iFlagOrderInPack = int( uint(iFirstDepthBreakToTheLeftInd) % 32u );
                uint uiFlagPack = uiPackedCamSpaceDiffFlags[iFlagPackOrder];
                // To test if there is a depth break in the current flag pack,
                // we must check all flags starting from the iFlagOrderInPack
                // downward to 0 position. We must skip all flags from iFlagOrderInPack+1 to 31
                if( iFlagOrderInPack < 31 )
                {
                    // Set all higher flags to 1, so that they will be skipped
                    // Note that if iFlagOrderInPack == 31, there are no flags to skip
                    // Note also that (U << 32) != 0 as it can be expected. (U << 32) == U instead
                    uiFlagPack |= ( uint(0x0FFFFFFFFU) << uint(iFlagOrderInPack+1) );
                }
                // Find first zero flag starting from iFlagOrderInPack position. Since all
                // higher bits are set, they will be effectivelly skipped
                int iFirstUnsetFlagPos = firstbithigh( uint(~uiFlagPack) );
                // firstbithigh(0) == +INT_MAX
                if( !(0 <= iFirstUnsetFlagPos && iFirstUnsetFlagPos < 32) )
                    // There are no set flags => proceed to the next uint flag pack
                    iFirstUnsetFlagPos = -1;
                iFirstDepthBreakToTheLeftInd -= iFlagOrderInPack - iFirstUnsetFlagPos;

#if INITIAL_SAMPLE_STEP > 32
                // Check the remaining full flag packs
                iFlagPackOrder--;
                while( iFlagPackOrder >= 0 && iFirstUnsetFlagPos == -1 )
                {
                    uiFlagPack = uiPackedCamSpaceDiffFlags[iFlagPackOrder];
                    iFirstUnsetFlagPos = firstbithigh( uint(~uiFlagPack) );
                    if( !(0 <= iFirstUnsetFlagPos && iFirstUnsetFlagPos < 32) )
                        iFirstUnsetFlagPos = -1;
                    iFirstDepthBreakToTheLeftInd -= 31 - iFirstUnsetFlagPos;
                    iFlagPackOrder--;
                }
#endif
                // Ray marching sample is located next to the identified depth break:
                uiLeftSrcSampleInd = max( uint(iFirstDepthBreakToTheLeftInd + 1), uiInitialSample0Ind );
            }

            // Find right interpolation source using symmetric method
            {
                // We need to find first depth break starting from iRightSrcSampleInd and
                // going to the right up to the uiInitialSample1Ind
                uiRightSrcSampleInd = uiSampleInd;
                int iFlagPackOrder = int(uiRightSrcSampleInd / 32u );
                int iFlagOrderInPack = int(uiRightSrcSampleInd % 32u);
                uint uiFlagPack = uiPackedCamSpaceDiffFlags[iFlagPackOrder];
                // We need to find first unset flag in the current flag pack
                // starting from iFlagOrderInPack position and up to the 31st bit
                // Set all lower order bits to 1 so that they are skipped during
                // the test:
                if( iFlagOrderInPack > 0 )
                    uiFlagPack |= uint( (1 << iFlagOrderInPack)-1 );
                // Find first zero flag:
                int iFirstUnsetFlagPos = firstbitlow( uint(~uiFlagPack) );
                if( !(0 <= iFirstUnsetFlagPos && iFirstUnsetFlagPos < 32) )
                    iFirstUnsetFlagPos = 32;
                uiRightSrcSampleInd += uint(iFirstUnsetFlagPos - iFlagOrderInPack);

#if INITIAL_SAMPLE_STEP > 32
                // Check the remaining full flag packs
                iFlagPackOrder++;
                while( iFlagPackOrder < int(NUM_PACKED_FLAGS) && iFirstUnsetFlagPos == 32 )
                {
                    uiFlagPack = uiPackedCamSpaceDiffFlags[iFlagPackOrder];
                    iFirstUnsetFlagPos = firstbitlow( uint(~uiFlagPack) );
                    if( !(0 <= iFirstUnsetFlagPos && iFirstUnsetFlagPos < 32) )
                        iFirstUnsetFlagPos = 32;
                    uiRightSrcSampleInd += uint(iFirstUnsetFlagPos);
                    iFlagPackOrder++;
                }
#endif
                uiRightSrcSampleInd = min(uiRightSrcSampleInd, uiInitialSample1Ind);
            }
        }

        // If at least one interpolation source is the same as the sample itself, the
        // sample is ray marching sample and is interpolated from itself:
        if(uiLeftSrcSampleInd == uiSampleInd || uiRightSrcSampleInd == uiSampleInd )
            uiLeftSrcSampleInd = uiRightSrcSampleInd = uiSampleInd;
    }

    g_rwtex2DInterpolationSource[ int2(uiGlobalSampleInd, uiSliceInd) ] = uint2(uiGroupStartGlobalInd + uiLeftSrcSampleInd, uiGroupStartGlobalInd + uiRightSrcSampleInd);
}
