// This file is derived from the open source project provided by Intel Corportaion that
// requires the following notice to be kept:
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

#ifndef _TERRAIN_STRCUTS_FXH_
#define _TERRAIN_STRCUTS_FXH_

#include "BasicStructures.fxh"

#define PI 3.1415928f
#define HEIGHT_MAP_SCALE 65535.f

#ifdef __cplusplus
#   ifndef CHECK_STRUCT_ALIGNMENT
        // Note that defining empty macros causes GL shader compilation error on Mac, because
        // it does not allow standalone semicolons outside of main.
        // On the other hand, adding semicolon at the end of the macro definition causes gcc error.
#       define CHECK_STRUCT_ALIGNMENT(s) static_assert( sizeof(s) % 16 == 0, "sizeof(" #s ") is not multiple of 16" )
#   endif
#endif

struct TerrainAttribs
{
    float m_fElevationScale;
    float m_fElevationSamplingInterval;
    float m_fEarthRadius;
    float m_fBaseMtrlTilingScale;
    float4 m_f4TilingScale;

	float4 f4CascadeColors[MAX_CASCADES];

#ifdef __cplusplus
    TerrainAttribs() : 
        m_fElevationScale(0.1f),
		m_fElevationSamplingInterval(32.f),
        m_fEarthRadius(6371000.f),
        m_fBaseMtrlTilingScale(200.f),
        m_f4TilingScale(500.f, 800.f, 80.f, 80.f)
    {
        f4CascadeColors[0] = float4(0,1,0,1);
		f4CascadeColors[1] = float4(0,0,1,1);
		f4CascadeColors[2] = float4(1,1,0,1);
		f4CascadeColors[3] = float4(0,1,1,1);
		f4CascadeColors[4] = float4(1,0,1,1);
		f4CascadeColors[5] = float4(0.3f, 1, 0.7f,1);
		f4CascadeColors[6] = float4(0.7f, 0.3f,1,1);
		f4CascadeColors[7] = float4(1, 0.7f, 0.3f, 1);
    }
#endif
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(TerrainAttribs);
#endif

struct NMGenerationAttribs
{
    float m_fSampleSpacingInterval;
    int   m_iMIPLevel;
    float m_fElevationScale;
    float m_fDummy;
};
#ifdef CHECK_STRUCT_ALIGNMENT
    CHECK_STRUCT_ALIGNMENT(NMGenerationAttribs);
#endif


#endif //_TERRAIN_STRCUTS_FXH_
