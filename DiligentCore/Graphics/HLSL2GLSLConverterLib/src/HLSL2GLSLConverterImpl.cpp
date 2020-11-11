/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

// Converter limitation:
// * Using Texture* keywords in macros is not supported. The following lines will not work:
//   -  #define TEXTURE2D Texture2D
//      TEXTURE2D MacroTex2D;
//
//
// List of supported HLSL Objects and methods:
//
// * Texture1D:
//  -  void GetDimensions (out {int, uint, float} Width);
//  -  void GetDimensions (in uint MipLevel, out {int, uint, float} Width, out {int, uint, float} NumberOfLevels);
//  -  ret Sample( sampler_state S, float Location [, int Offset] );
//  -  ret SampleBias( sampler_state S, float Location, float Bias [, int Offset] );
//  -  ret SampleLevel( sampler_state S, float Location, float LOD [, int Offset] )
//  -  ret SampleGrad( sampler_state S, float Location, float DDX, float DDY [, int Offset] );
//  - float SampleCmp( SamplerComparisonState S, float Location, float CompareValue, [int Offset] );
//  - float SampleCmpLevelZero( SamplerComparisonState S, float Location, float CompareValue, [int Offset] );
//  -  ret Load( int Location, [int Offset ] );
//
//
// * Texture1DArray:
//  - void GetDimensions( out {int, uint, float} Width, out {int, uint, float} Elements );
//  - void GetDimensions( in uint MipLevel, out {int, uint, float} Width, out {int, uint, float} Elements, out {int, uint, float} NumberOfLevels );
//  -  ret Sample( sampler_state S, float2 Location [, int Offset] );
//  -  ret SampleBias( sampler_state S, float2 Location, float Bias [, int Offset] );
//  -  ret SampleLevel( sampler_state S, float2 Location, float LOD [, int Offset] )
//  -  ret SampleGrad( sampler_state S, float2 Location, float DDX, float DDY [, int Offset] );
//  - float SampleCmp( SamplerComparisonState S, float2 Location, float CompareValue, [int Offset] );
//  - float SampleCmpLevelZero( SamplerComparisonState S, float2 Location, float CompareValue, [int Offset] );
//  -  ret Load( int2 Location, [int Offset ] );
// Remarks:
//  - Array index goes in Location.y

//
// * Texture2D:
//  - void GetDimensions( out {int, uint, float} Width, out {int, uint, float} Height );
//  - void GetDimensions( in uint MipLevel, out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} NumberOfLevels );
//  -  ret Sample( sampler_state S, float2 Location [, int2 Offset] );
//  -  ret SampleBias( sampler_state S, float2 Location, float Bias [, int2 Offset] );
//  -  ret SampleLevel( sampler_state S, float2 Location, float LOD [, int2 Offset] )
//  -  ret SampleGrad( sampler_state S, float2 Location, float2 DDX, float2 DDY [, int2 Offset] );
//  - float SampleCmp( SamplerComparisonState S, float2 Location, float CompareValue, [int2 Offset] );
//  - float SampleCmpLevelZero( SamplerComparisonState S, float2 Location, float CompareValue [, int2 Offset] );
//  -  ret Load( int2 Location, [int2 Offset ] );
//  -  ret Gather( sampler_state S, float2 Location [, int2 Offset] );
//  -  float4 GatherCmp( SamplerComparisonState S, float2 Location, float CompareValue [, int2 Offset] );
//
// * Texture2DArray:
//  - void GetDimensions( out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Elements );
//  - void GetDimensions( in uint MipLevel, out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Elements, out {int, uint, float} NumberOfLevels );
//  -  ret Sample( sampler_state S, float3 Location [, int2 Offset] );
//  -  ret SampleBias( sampler_state S, float3 Location, float Bias [, int2 Offset] );
//  -  ret SampleLevel( sampler_state S, float3 Location, float LOD [, int2 Offset] )
//  -  ret SampleGrad( sampler_state S, float3 Location, float2 DDX, float2 DDY [, int2 Offset] );
//  - float SampleCmp( SamplerComparisonState S, float2 Location, float CompareValue [, int2 Offset] );
//  -  ret Load( int Location3 [, int2 Offset ] );
//  -  ret Gather( sampler_state S, float3 Location [, int2 Offset] );
//  -  float4 GatherCmp( SamplerComparisonState S, float3 Location, float CompareValue [, int2 Offset] );
//  Remarks:
//  - Array index goes in Location.z
//  - SampleCmpLevelZero() is not supported as there is no corresponding OpenGL instruction. The
//    instruction will always return 0.
//
// * Texture3D:
//  - void GetDimensions( out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Depth );
//  - void GetDimensions( in uint MipLevel, out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Depth, out {int, uint, float} NumberOfLevels );
//  -  ret Sample( sampler_state S, float3 Location [, int3 Offset] );
//  -  ret SampleBias( sampler_state S, float3 Location, float Bias [, int3 Offset] );
//  -  ret SampleLevel( sampler_state S, float3 Location, float LOD [, int3 Offset] )
//  -  ret SampleGrad( sampler_state S, float3 Location, float3 DDX, float3 DDY [, int3 Offset] );
//  -  ret Load( int3 Location [, int3 Offset ] );
//
// * TextureCube:
//  - void GetDimensions( out {int, uint, float} Width, out {int, uint, float} Height );
//  - void GetDimensions( in uint MipLevel, out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} NumberOfLevels );
//  -  ret Sample( sampler_state S, float3 Location );
//  -  ret SampleBias( sampler_state S, float3 Location, float Bias );
//  -  ret SampleLevel( sampler_state S, float3 Location, float LOD ) - NO offset version
//  -  ret SampleGrad( sampler_state S, float3 Location, float3 DDX, float3 DDY );
//  - float SampleCmp( SamplerComparisonState S, float3 Location, float CompareValue );
//  -  ret Gather( sampler_state S, float3 Location );
//  -  float4 GatherCmp( SamplerComparisonState S, float3 Location, float CompareValue );
//  Remarks:
//  - SampleCmpLevelZero() is not supported as there is no corresponding OpenGL instruction. The
//    instruction will always return 0.

// * TextureCubeArray:
//  - void GetDimensions( out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Elements );
//  - void GetDimensions( in uint MipLevel, out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Elements, out {int, uint, float} NumberOfLevels );
//  -  ret Sample( sampler_state S, float4 Location );
//  -  ret SampleBias( sampler_state S, float4 Location, float Bias );
//  -  ret SampleLevel( sampler_state S, float4 Location, float LOD ) - NO offset version
//  -  ret SampleGrad( sampler_state S, float4 Location, float3 DDX, float3 DDY );
//  - float SampleCmp( SamplerComparisonState S, float4 Location, float CompareValue );
//  -  ret Gather( sampler_state S, float4 Location );
//  -  float4 GatherCmp( SamplerComparisonState S, float4 Location, float CompareValue );
//  Remarks:
//  - SampleCmpLevelZero() is not supported as there is no corresponding OpenGL instruction. The
//    instruction will always return 0.
//  - Array index goes in Location.w
//
// * Texture2DMS:
//  - void GetDimensions(out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} NumberOfSamples);
//  -  ret Load( int2 Location, int Sample, [int2 Offset ] );
//
// * Texture2DMSArray:
//  - void GetDimensions( out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Elements, out {int, uint, float} NumberOfSamples );
//  -  ret Load( int3 Location, int Sample, [int2 Offset ] );
//
//
// * RWTexture1D:
//  - void GetDimensions(out {int, uint, float} Width);
//
// * RWTexture1DArray:
//  - void GetDimensions(out {int, uint, float} Width, out {int, uint, float} Elements);
//
// * RWTexture2D:
//  - void GetDimensions(out {int, uint, float} Width, out {int, uint, float} Height);
//
// * RWTexture2DArray:
//  - void GetDimensions(out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Elements);
//
// * RWTexture3D:
//  - void GetDimensions(out {int, uint, float} Width, out {int, uint, float} Height, out {int, uint, float} Depth);
//

// \remarks
//   All GetDimensions() functions return valid value in NumberOfLevels only on Desktop GL 4.3+
//   For multisampled textures, GetDimensions() always returns 0 in NumberOfSamples.


// Support for HLSL intrinsics:

// [V] abs( {int, int2, int3, int4, float, float2, float3, float4} )
// [V] acos( {float, float2, float3, float4} )
//   (-) acos( {matrix types} )
// [V] all( {bool2, bool3, bool4})
//   (-) all( {bool, int, int2, int3, int4, float, float2, float3, float4, matrix types} )
// [V] any( {bool2, bool3, bool4})
//   (-) any( {bool, int, int2, int3, int4, float, float2, float3, float4, matrix types} )
// [V] asdouble( {uint} )
// [V] asfloat( {int, int2, int3, int4, uint, uint2, uint3, uint4, float, float2, float3, float4} )
//   (-) asfloat( {matrix types} )
// [V] asint( {int, int2, int3, int4, uint, uint2, uint3, uint4, float, float2, float3, float4} )
//   (-) asint( {matrix types} )
// [V] asuint( {int, int2, int3, int4, uint, uint2, uint3, uint4, float, float2, float3, float4} )
//   (-) asuint( {matrix types} )
// [V] asin( {float, float2, float3, float4} )
//   (-) asin( {matrix types} )
// [V] atan( {float, float2, float3, float4} )
//   (-) atan( {matrix types} )
// [V] atan2( {float, float2, float3, float4} )
//   (-) atan2( {matrix types} )
// [V] ceil( {float, float2, float3, float4} )
//   (-) ceil( {matrix types} )
// [V] clamp( {int, int2, int3, int4, uint, uint2, uint3, uint4, float, float2, float3, float4} )
//   (-) clamp( {matrix types} )
// [V] cos( {float, float2, float3, float4} )
//   (-) cos( {matrix types} )
// [V] cosh( {float, float2, float3, float4} )
//   (-) cosh( {matrix types} )
// [V] countbits( {int, int2, int3, int4, uint, uint2, uint3, uint4} )
// [V] cross(float3)
// [V] ddx
// [V] ddx_coarse - defined as ddx
// [V] ddx_fine - defined as ddx
// [V] ddy
// [V] ddy_coarse - defined as ddy
// [V] ddy_fine - defined as ddy
// [V] degrees( {float, float2, float3, float4} )
//   (-) degrees( {matrix types} )
// [V] determinant
// [V] distance( {float, float2, float3, float4} )
// [V] dot( {float, float2, float3, float4} )
//   (-) dot( {int, int2, int3, int4} )
// [V] dst - defined as distance
// [V] exp( {float, float2, float3, float4} )
//   (-) exp( {matrix types} )
// [V] exp2( {float, float2, float3, float4} )
//   (-) exp2( {matrix types} )
// [V] f16tof32( {int, int2, int3, int4, uint, uint2, uint3, uint4} )
// [V] f32tof16( {float, float2, float3, float4} ) -> {uint, uint2, uint3, uint4}
// [V] faceforward( {float, float2, float3, float4} )
// [V] firstbithigh( {int, int2, int3, int4, uint, uint2, uint3, uint4} )
// [V] firstbitlow( {int, int2, int3, int4, uint, uint2, uint3, uint4} )
// [V] floor( {float, float2, float3, float4} )
//   (-) floor( {matrix types} )
// [V] fma( {double, double2, double3, double4} )
// [V] fmod( {float, float2, float3, float4} )
//   (-) fmod( {matrix types} )
// [V] frac( {float, float2, float3, float4} )
//   (-) frac( {matrix types} )
// [V] frexp( {float, float2, float3, float4}, {float, float2, float3, float4} )
// [V] fwidth( {float, float2, float3, float4} )
//   (-) fwidth( {matrix types} )
// [V] isfinite( {float, float2, float3, float4} ) - implemented as (!isinf(x) && !isnan(x))
// [V] isinf( {float, float2, float3, float4} )
//   (-) isinf( {matrix types} )
// [V] isnan( {float, float2, float3, float4} )
//   (-) isnan( {matrix types} )
// [V] ldexp( {float, float2, float3, float4}, {int, int2, int3, int4} )
// [V] length( {float, float2, float3, float4} )
// [V] lerp( {float, float2, float3, float4} )
//   (-) lerp( {matrix types} )
// [V] log( {float, float2, float3, float4} )
//   (-) log( {matrix types} )
// [V] log2( {float, float2, float3, float4} )
//   (-) log2( {matrix types} )
// [V] log10( {float, float2, float3, float4} )
//   (-) log10( {matrix types} )
// [V] mad( {float, float2, float3, float4} )
//   (-) mad( {matrix types} )
// [V] max( {int, int2, int3, int4, uint, uint2, uint3, uint4, float, float2, float3, float4} )
//   (-) max( {matrix types} )
// [V] min( {int, int2, int3, int4, uint, uint2, uint3, uint4, float, float2, float3, float4} )
//   (-) min( {matrix types} )
// [V] modf( {float, float2, float3, float4} )
//   (-) modf( {int, int2, int3, int4, matrix types} )
// [V] mul - defined as a*b
// [V] noise( {float, float2, float3, float4} )
// [V] normalize( {float, float2, float3, float4} )
// [V] pow( {float, float2, float3, float4} )
//   (-) pow( {matrix types} )
// [V] radians( {float, float2, float3, float4} )
//   (-) radians( {matrix types} )
// [V] rcp( {float, float2, float3, float4} ) - defined as 1.0/(x)
// [V] reflect( {float, float2, float3, float4} )
// [V] refract( {float, float2, float3, float4} )
// [V] reversebits( {int, int2, int3, int4, uint, uint2, uint3, uint4} )
// [V] round( {float, float2, float3, float4} )
//   (-) round( {matrix types} )
// [V] rsqrt( {float, float2, float3, float4} )
//   (-) rsqrt( {matrix types} )
// [V] saturate( {float, float2, float3, float4} )
// [V] sign( {float, float2, float3, float4, int, int2, int3, int4} )
//   (-) sign( {matrix types} )
// [V] sin( {float, float2, float3, float4} )
//   (-) sin( {matrix types} )
// [V] sinh( {float, float2, float3, float4} )
//   (-) sinh( {matrix types} )
// [V] sincos( {float, float2, float3, float4} )
// [V] smoothstep( {float, float2, float3, float4} )
//   (-) smoothstep( {matrix types} )
// [V] sqrt( {float, float2, float3, float4} )
//   (-) sqrt( {matrix types} )
// [V] step( {float, float2, float3, float4} )
//   (-) step( {matrix types} )
// [V] tan( {float, float2, float3, float4} )
//   (-) tan( {matrix types} )
// [V] tanh( {float, float2, float3, float4} )
//   (-) tanh( {matrix types} )
// [V] transpose
// [V] trunc( {float, float2, float3, float4} )
//   (-) trunc( {matrix types} )

// [V] AllMemoryBarrier - calls all memory barrier functions in gl
// [V] AllMemoryBarrierWithGroupSync
// [V] DeviceMemoryBarrier - calls image, atomic counter & buffer memory barriers
// [V] DeviceMemoryBarrierWithGroupSync
// [V] GroupMemoryBarrier - calls group memory & shared memory barriers
// [V] GroupMemoryBarrierWithGroupSync

// [V] InterlockedAdd( {int, uint} )
// [V] InterlockedAnd( {int, uint} )
// [V] InterlockedCompareExchange( {int, uint} )
// [V] InterlockedCompareStore( {int, uint} )
// [V] InterlockedExchange( {int, uint} )
// [V] InterlockedMax( {int, uint} )
// [V] InterlockedMin( {int, uint} )
// [V] InterlockedOr( {int, uint} )
// [V] InterlockedXor( {int, uint} )

// [ ] Process2DQuadTessFactorsAvg
// [ ] Process2DQuadTessFactorsMax
// [ ] Process2DQuadTessFactorsMin
// [ ] ProcessIsolineTessFactors
// [ ] ProcessQuadTessFactorsAvg
// [ ] ProcessQuadTessFactorsMax
// [ ] ProcessQuadTessFactorsMin
// [ ] ProcessTriTessFactorsAvg
// [ ] ProcessTriTessFactorsMax
// [ ] ProcessTriTessFactorsMin

// [ ] CheckAccessFullyMapped

// [ ] GetRenderTargetSampleCount
// [ ] GetRenderTargetSamplePosition

// [ ] EvaluateAttributeAtCentroid
// [ ] EvaluateAttributeAtSample
// [ ] EvaluateAttributeSnapped

// [ ] abort
// [ ] errorf
// [ ] printf
// [ ] clip
// [ ] msad4
// [ ] lit

// [ ] D3DCOLORtoUBYTE4

// Legacy not supported functions:
// [ ] tex1D
// [ ] tex1D
// [ ] tex1Dbias
// [ ] tex1Dgrad
// [ ] tex1Dlod
// [ ] tex1Dproj
// [ ] tex2D
// [ ] tex2D
// [ ] tex2Dbias
// [ ] tex2Dgrad
// [ ] tex2Dlod
// [ ] tex2Dproj
// [ ] tex3D
// [ ] tex3D
// [ ] tex3Dbias
// [ ] tex3Dgrad
// [ ] tex3Dlod
// [ ] tex3Dproj
// [ ] texCUBE
// [ ] texCUBE
// [ ] texCUBEbias
// [ ] texCUBEgrad
// [ ] texCUBElod
// [ ] texCUBEproj


#include "pch.h"
#include <unordered_set>
#include <string>

#include "HLSL2GLSLConverterImpl.hpp"
#include "GraphicsAccessories.hpp"
#include "DataBlobImpl.hpp"
#include "StringDataBlobImpl.hpp"
#include "StringTools.hpp"
#include "EngineMemory.h"

using namespace std;

namespace Diligent
{

static const Char* g_GLSLDefinitions =
    {
#include "GLSLDefinitions_inc.h"
};

inline bool IsWhitespace(Char Symbol)
{
    return Symbol == ' ' || Symbol == '\t';
}

inline bool IsNewLine(Char Symbol)
{
    return Symbol == '\r' || Symbol == '\n';
}

inline bool IsDelimiter(Char Symbol)
{
    static const Char* Delimeters = " \t\r\n";
    return strchr(Delimeters, Symbol) != nullptr;
}

inline bool IsStatementSeparator(Char Symbol)
{
    static const Char* StatementSeparator = ";}";
    return strchr(StatementSeparator, Symbol) != nullptr;
}


// IteratorType may be String::iterator or String::const_iterator.
// While iterator is convertible to const_iterator,
// iterator& cannot be converted to const_iterator& (Microsoft compiler allows
// such conversion, while gcc does not)
template <typename InteratorType>
static bool SkipComment(const String& Input, InteratorType& Pos)
{
    // // Comment     /* Comment
    // ^              ^
    if (Pos == Input.end() || *Pos != '/')
        return false;

    auto NextPos = Pos + 1;
    // // Comment     /* Comment
    //  ^              ^
    if (NextPos == Input.end())
        return false;

    if (*NextPos == '/')
    {
        // Skip // comment
        Pos = NextPos + 1;
        // // Comment
        //   ^
        for (; Pos != Input.end() && !IsNewLine(*Pos); ++Pos)
            ;
        return true;
    }
    else if (*NextPos == '*')
    {
        // Skip /* comment */
        Pos = NextPos + 1;
        // /* Comment
        //   ^
        while (Pos != Input.end())
        {
            if (*Pos == '*')
            {
                // /* Comment */
                //            ^
                ++Pos;
                // /* Comment */
                //             ^
                if (Pos == Input.end())
                    break;
                if (*Pos == '/')
                {
                    ++Pos;
                    // /* Comment */
                    //              ^
                    break;
                }
            }
            else
            {
                // Must handle /* **/ properly
                ++Pos;
            }
        }
        return true;
    }

    return false;
}

inline bool SkipDelimeters(const String& Input, String::const_iterator& SrcChar)
{
    for (; SrcChar != Input.end() && IsDelimiter(*SrcChar); ++SrcChar)
        ;
    return SrcChar == Input.end();
}

// IteratorType may be String::iterator or String::const_iterator.
// While iterator is convertible to const_iterator,
// iterator& cannot be converted to const_iterator& (Microsoft compiler allows
// such conversion, while gcc does not)
template <typename IteratorType>
inline bool SkipDelimetersAndComments(const String& Input, IteratorType& SrcChar)
{
    bool DelimiterFound = false;
    bool CommentFound   = false;
    do
    {
        DelimiterFound = false;
        for (; SrcChar != Input.end() && IsDelimiter(*SrcChar); ++SrcChar)
            DelimiterFound = true;

        CommentFound = SkipComment(Input, SrcChar);
    } while (SrcChar != Input.end() && (DelimiterFound || CommentFound));

    return SrcChar == Input.end();
}

inline bool SkipIdentifier(const String& Input, String::const_iterator& SrcChar)
{
    if (SrcChar == Input.end())
        return true;

    if (isalpha(*SrcChar) || *SrcChar == '_')
    {
        ++SrcChar;
        if (SrcChar == Input.end())
            return true;
    }
    else
        return false;

    for (; SrcChar != Input.end() && (isalnum(*SrcChar) || *SrcChar == '_'); ++SrcChar)
        ;

    return SrcChar == Input.end();
}


const HLSL2GLSLConverterImpl& HLSL2GLSLConverterImpl::GetInstance()
{
    static HLSL2GLSLConverterImpl Converter;
    return Converter;
}

HLSL2GLSLConverterImpl::HLSL2GLSLConverterImpl()
{
    // Populate HLSL keywords hash map
#define DEFINE_KEYWORD(keyword) m_HLSLKeywords.insert(std::make_pair(#keyword, TokenInfo(TokenType::kw_##keyword, #keyword)));
    ITERATE_KEYWORDS(DEFINE_KEYWORD)
#undef DEFINE_KEYWORD

    // Prepare texture function stubs
    //                          sampler  usampler  isampler sampler*Shadow
    const String Prefixes[] = {"", "u", "i", ""};
    const String Suffixes[] = {"", "", "", "Shadow"};
    for (int i = 0; i < _countof(Prefixes); ++i)
    {
        const auto& Pref = Prefixes[i];
        const auto& Suff = Suffixes[i];
        // GetDimensions() does not return anything, so swizzle should be empty
#define DEFINE_GET_DIM_STUB(Name, Obj, NumArgs) m_GLSLStubs.emplace(make_pair(FunctionStubHashKey(Pref + Obj + Suff, "GetDimensions", NumArgs), GLSLStubInfo(Name, "")))

        DEFINE_GET_DIM_STUB("GetTex1DDimensions_1", "sampler1D", 1); // GetDimensions( Width )
        DEFINE_GET_DIM_STUB("GetTex1DDimensions_3", "sampler1D", 3); // GetDimensions( Mip, Width, NumberOfMips )

        DEFINE_GET_DIM_STUB("GetTex1DArrDimensions_2", "sampler1DArray", 2); // GetDimensions( Width, ArrElems )
        DEFINE_GET_DIM_STUB("GetTex1DArrDimensions_4", "sampler1DArray", 4); // GetDimensions( Mip, Width, ArrElems, NumberOfMips )

        DEFINE_GET_DIM_STUB("GetTex2DDimensions_2", "sampler2D", 2); // GetDimensions( Width, Height )
        DEFINE_GET_DIM_STUB("GetTex2DDimensions_4", "sampler2D", 4); // GetDimensions( Mip, Width, Height, NumberOfMips );

        DEFINE_GET_DIM_STUB("GetTex2DArrDimensions_3", "sampler2DArray", 3); // GetDimensions( Width, Height, ArrElems )
        DEFINE_GET_DIM_STUB("GetTex2DArrDimensions_5", "sampler2DArray", 5); // GetDimensions( Mip, Width, Height, ArrElems, NumberOfMips )

        DEFINE_GET_DIM_STUB("GetTex2DDimensions_2", "samplerCube", 2); // GetDimensions( Width, Height )
        DEFINE_GET_DIM_STUB("GetTex2DDimensions_4", "samplerCube", 4); // GetDimensions( Mip, Width, Height, NumberOfMips )

        DEFINE_GET_DIM_STUB("GetTex2DArrDimensions_3", "samplerCubeArray", 3); // GetDimensions( Width, Height, ArrElems )
        DEFINE_GET_DIM_STUB("GetTex2DArrDimensions_5", "samplerCubeArray", 5); // GetDimensions( Mip, Width, Height, ArrElems, NumberOfMips )

        DEFINE_GET_DIM_STUB("GetTexBufferDimensions_1", "samplerBuffer", 1); // GetDimensions( Width )

        if (Suff == "")
        {
            // No shadow samplers for Tex3D, Tex2DMS and Tex2DMSArr
            DEFINE_GET_DIM_STUB("GetTex3DDimensions_3", "sampler3D", 3); // GetDimensions( Width, Height, Depth )
            DEFINE_GET_DIM_STUB("GetTex3DDimensions_5", "sampler3D", 5); // GetDimensions( Mip, Width, Height, Depth, NumberOfMips )


            // clang-format off
            DEFINE_GET_DIM_STUB("GetTex2DMSDimensions_3",    "sampler2DMS",      3); // GetDimensions( Width, Height, NumSamples )
            DEFINE_GET_DIM_STUB("GetTex2DMSArrDimensions_4", "sampler2DMSArray", 4); // GetDimensions( Width, Height, ArrElems, NumSamples )

            // Images
            DEFINE_GET_DIM_STUB("GetRWTex1DDimensions_1",     "image1D",      1); // GetDimensions( Width )
            DEFINE_GET_DIM_STUB("GetRWTex1DArrDimensions_2",  "image1DArray", 2); // GetDimensions( Width, ArrElems )
            DEFINE_GET_DIM_STUB("GetRWTex2DDimensions_2",     "image2D",      2); // GetDimensions( Width, Height )
            DEFINE_GET_DIM_STUB("GetRWTex2DArrDimensions_3",  "image2DArray", 3); // GetDimensions( Width, Height, ArrElems )
            DEFINE_GET_DIM_STUB("GetRWTex3DDimensions_3",     "image3D",      3); // GetDimensions( Width, Height, Depth )
            DEFINE_GET_DIM_STUB("GetRWTexBufferDimensions_1", "imageBuffer",  1); // GetDimensions( Width )
            // clang-format on

            m_ImageTypes.insert(HashMapStringKey(Pref + "image1D"));
            m_ImageTypes.insert(HashMapStringKey(Pref + "image1DArray"));
            m_ImageTypes.insert(HashMapStringKey(Pref + "image2D"));
            m_ImageTypes.insert(HashMapStringKey(Pref + "image2DArray"));
            m_ImageTypes.insert(HashMapStringKey(Pref + "image3D"));
            m_ImageTypes.insert(HashMapStringKey(Pref + "imageBuffer"));
        }
#undef DEFINE_GET_DIM_STUB
    }

    String Dimensions[] = {"1D", "1DArray", "2D", "2DArray", "3D", "Cube", "CubeArray"};
    for (int d = 0; d < _countof(Dimensions); ++d)
    {
        String Dim = Dimensions[d];
        for (int i = 0; i < 3; ++i)
        {
            auto GLSLSampler = Prefixes[i] + "sampler" + Dim;

            // Use default swizzle to return the same number of components as specified in the texture declaration
            // Converter will insert _SWIZZLEn, where n is the number of components, after the function stub.
            // Example:
            // Texture2D<float3> Tex2D;
            // ...
            // Tex2D.Sample(Tex2D_sampler, f2UV) -> Sample_2(Tex2D, Tex2D_sampler, f2UV)_SWIZZLE3
            const Char* Swizzle = "_SWIZZLE";

#define DEFINE_STUB(Name, Obj, Func, NumArgs) m_GLSLStubs.emplace(make_pair(FunctionStubHashKey(Obj, Func, NumArgs), GLSLStubInfo(Name, Swizzle)))

            // clang-format off
            DEFINE_STUB("Sample_2",      GLSLSampler, "Sample",      2); // Sample     ( Sampler, Location )
            DEFINE_STUB("SampleBias_3",  GLSLSampler, "SampleBias",  3); // SampleBias ( Sampler, Location, Bias )
            DEFINE_STUB("SampleLevel_3", GLSLSampler, "SampleLevel", 3); // SampleLevel( Sampler, Location, LOD )
            DEFINE_STUB("SampleGrad_4",  GLSLSampler, "SampleGrad",  4); // SampleGrad ( Sampler, Location, DDX, DDY )
            if (Dim != "Cube" && Dim != "CubeArray")
            {
                // No offset versions for cube & cube array
                DEFINE_STUB("Sample_3",      GLSLSampler, "Sample",      3); // Sample     ( Sampler, Location, Offset )
                DEFINE_STUB("SampleBias_4",  GLSLSampler, "SampleBias",  4); // SampleBias ( Sampler, Location, Bias, Offset )
                DEFINE_STUB("SampleLevel_4", GLSLSampler, "SampleLevel", 4); // SampleLevel( Sampler, Location, LOD, Offset )
                DEFINE_STUB("SampleGrad_5",  GLSLSampler, "SampleGrad",  5); // SampleGrad ( Sampler, Location, DDX, DDY, Offset )
            }
            // clang-format on
            if (Dim != "1D" && Dim != "1DArray" && Dim != "3D")
            {
                // Gather always returns float4 independent of the number of components, so no swizzling
                Swizzle = "";
                DEFINE_STUB("Gather_2", GLSLSampler, "Gather", 2); // Gather( SamplerState, Location )
                DEFINE_STUB("Gather_3", GLSLSampler, "Gather", 3); // Gather( SamplerState, Location, Offset )
            }
        }
    }

    // Gather always returns float4 independent of the number of components, so no swizzling
    const Char* Swizzle = "";
    DEFINE_STUB("GatherCmp_3", "sampler2DShadow", "GatherCmp", 3);        // GatherCmp( SmplerCmp, Location, CompareValue )
    DEFINE_STUB("GatherCmp_4", "sampler2DShadow", "GatherCmp", 4);        // GatherCmp( SmplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("GatherCmp_3", "sampler2DArrayShadow", "GatherCmp", 3);   // GatherCmp( SmplerCmp, Location, CompareValue )
    DEFINE_STUB("GatherCmp_4", "sampler2DArrayShadow", "GatherCmp", 4);   // GatherCmp( SmplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("GatherCmp_3", "samplerCubeShadow", "GatherCmp", 3);      // GatherCmp( SmplerCmp, Location, CompareValue )
    DEFINE_STUB("GatherCmp_3", "samplerCubeArrayShadow", "GatherCmp", 3); // GatherCmp( SmplerCmp, Location, CompareValue )

    // All load operations should return the same number of components as specified
    // in texture declaraion, so use swizzling. Example:
    // Texture3D<int2> Tex3D;
    // ...
    // Tex3D.Load(i4Location) -> LoadTex3D_1(Tex3D, i4Location)_SWIZZLE2
    Swizzle = "_SWIZZLE";
    for (int i = 0; i < 3; ++i)
    {
        auto Pref = Prefixes[i];
        // clang-format off
        DEFINE_STUB("LoadTex1D_1",      Pref + "sampler1D",        "Load", 1); // Load( Location )
        DEFINE_STUB("LoadTex1DArr_1",   Pref + "sampler1DArray",   "Load", 1); // Load( Location )
        DEFINE_STUB("LoadTex2D_1",      Pref + "sampler2D",        "Load", 1); // Load( Location )
        DEFINE_STUB("LoadTex2DArr_1",   Pref + "sampler2DArray",   "Load", 1); // Load( Location )
        DEFINE_STUB("LoadTex3D_1",      Pref + "sampler3D",        "Load", 1); // Load( Location )
        DEFINE_STUB("LoadTex2DMS_2",    Pref + "sampler2DMS",      "Load", 2); // Load( Location, Sample )
        DEFINE_STUB("LoadTex2DMSArr_2", Pref + "sampler2DMSArray", "Load", 2); // Load( Location, Sample )

        DEFINE_STUB("LoadTex1D_2",      Pref + "sampler1D",       "Load", 2);  // Load( Location, Offset )
        DEFINE_STUB("LoadTex1DArr_2",   Pref + "sampler1DArray",  "Load", 2);  // Load( Location, Offset )
        DEFINE_STUB("LoadTex2D_2",      Pref + "sampler2D",       "Load", 2);  // Load( Location, Offset )
        DEFINE_STUB("LoadTex2DArr_2",   Pref + "sampler2DArray",  "Load", 2);  // Load( Location, Offset )
        DEFINE_STUB("LoadTex3D_2",      Pref + "sampler3D",       "Load", 2);  // Load( Location, Offset )
        DEFINE_STUB("LoadTex2DMS_3",    Pref + "sampler2DMS",     "Load", 3);  // Load( Location, Sample, Offset )
        DEFINE_STUB("LoadTex2DMSArr_3", Pref + "sampler2DMSArray", "Load", 3); // Load( Location, Sample, Offset )

        DEFINE_STUB("LoadTexBuffer_1", Pref + "samplerBuffer", "Load", 1); // Load( Location )

        DEFINE_STUB("LoadRWTex1D_1",     Pref + "image1D",      "Load", 1); // Load( Location )
        DEFINE_STUB("LoadRWTex1DArr_1",  Pref + "image1DArray", "Load", 1); // Load( Location )
        DEFINE_STUB("LoadRWTex2D_1",     Pref + "image2D",      "Load", 1); // Load( Location )
        DEFINE_STUB("LoadRWTex2DArr_1",  Pref + "image2DArray", "Load", 1); // Load( Location )
        DEFINE_STUB("LoadRWTex3D_1",     Pref + "image3D",      "Load", 1); // Load( Location )
        DEFINE_STUB("LoadRWTexBuffer_1", Pref + "imageBuffer",  "Load", 1); // Load( Location )
        // clang-format on
    }

    // SampleCmp() returns float independent of the number of components, so
    // use no swizzling
    Swizzle = "";

    DEFINE_STUB("SampleCmpTex1D_3", "sampler1DShadow", "SampleCmp", 3);             // SampleCmp( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpTex1DArr_3", "sampler1DArrayShadow", "SampleCmp", 3);     // SampleCmp( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpTex2D_3", "sampler2DShadow", "SampleCmp", 3);             // SampleCmp( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpTex2DArr_3", "sampler2DArrayShadow", "SampleCmp", 3);     // SampleCmp( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpTexCube_3", "samplerCubeShadow", "SampleCmp", 3);         // SampleCmp( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpTexCubeArr_3", "samplerCubeArrayShadow", "SampleCmp", 3); // SampleCmp( SamplerCmp, Location, CompareValue )

    DEFINE_STUB("SampleCmpTex1D_4", "sampler1DShadow", "SampleCmp", 4);         // SampleCmp( SamplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("SampleCmpTex1DArr_4", "sampler1DArrayShadow", "SampleCmp", 4); // SampleCmp( SamplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("SampleCmpTex2D_4", "sampler2DShadow", "SampleCmp", 4);         // SampleCmp( SamplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("SampleCmpTex2DArr_4", "sampler2DArrayShadow", "SampleCmp", 4); // SampleCmp( SamplerCmp, Location, CompareValue, Offset )


    DEFINE_STUB("SampleCmpLevel0Tex1D_3", "sampler1DShadow", "SampleCmpLevelZero", 3);             // SampleCmpLevelZero( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpLevel0Tex1DArr_3", "sampler1DArrayShadow", "SampleCmpLevelZero", 3);     // SampleCmpLevelZero( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpLevel0Tex2D_3", "sampler2DShadow", "SampleCmpLevelZero", 3);             // SampleCmpLevelZero( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpLevel0Tex2DArr_3", "sampler2DArrayShadow", "SampleCmpLevelZero", 3);     // SampleCmpLevelZero( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpLevel0TexCube_3", "samplerCubeShadow", "SampleCmpLevelZero", 3);         // SampleCmpLevelZero( SamplerCmp, Location, CompareValue )
    DEFINE_STUB("SampleCmpLevel0TexCubeArr_3", "samplerCubeArrayShadow", "SampleCmpLevelZero", 3); // SampleCmpLevelZero( SamplerCmp, Location, CompareValue )

    DEFINE_STUB("SampleCmpLevel0Tex1D_4", "sampler1DShadow", "SampleCmpLevelZero", 4);         // SampleCmpLevelZero( SamplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("SampleCmpLevel0Tex1DArr_4", "sampler1DArrayShadow", "SampleCmpLevelZero", 4); // SampleCmpLevelZero( SamplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("SampleCmpLevel0Tex2D_4", "sampler2DShadow", "SampleCmpLevelZero", 4);         // SampleCmpLevelZero( SamplerCmp, Location, CompareValue, Offset )
    DEFINE_STUB("SampleCmpLevel0Tex2DArr_4", "sampler2DArrayShadow", "SampleCmpLevelZero", 4); // SampleCmpLevelZero( SamplerCmp, Location, CompareValue, Offset )


    // InterlockedOp( dest, val )
    // InterlockedOp( dest, val, original_val )
#define DEFINE_ATOMIC_OP_STUBS(Op)                                                  \
    DEFINE_STUB("Interlocked" Op "SharedVar_2", "shared_var", "Interlocked" Op, 2); \
    DEFINE_STUB("Interlocked" Op "SharedVar_3", "shared_var", "Interlocked" Op, 3); \
    DEFINE_STUB("Interlocked" Op "Image_2", "image", "Interlocked" Op, 2);          \
    DEFINE_STUB("Interlocked" Op "Image_3", "image", "Interlocked" Op, 3);          \
    m_AtomicOperations.insert(HashMapStringKey("Interlocked" Op));


    DEFINE_ATOMIC_OP_STUBS("Add");
    DEFINE_ATOMIC_OP_STUBS("And");
    DEFINE_ATOMIC_OP_STUBS("Exchange");
    DEFINE_ATOMIC_OP_STUBS("Max");
    DEFINE_ATOMIC_OP_STUBS("Min");
    DEFINE_ATOMIC_OP_STUBS("Or");
    DEFINE_ATOMIC_OP_STUBS("Xor");

    // InterlockedCompareExchange( dest, compare_value, value, original_value )
    DEFINE_STUB("InterlockedCompareExchangeSharedVar_4", "shared_var", "InterlockedCompareExchange", 4);
    DEFINE_STUB("InterlockedCompareExchangeImage_4", "image", "InterlockedCompareExchange", 4);
    m_AtomicOperations.insert(HashMapStringKey("InterlockedCompareExchange"));

    // InterlockedCompareStore( dest, compare_value, value )
    DEFINE_STUB("InterlockedCompareStoreSharedVar_3", "shared_var", "InterlockedCompareStore", 3);
    DEFINE_STUB("InterlockedCompareStoreImage_3", "image", "InterlockedCompareStore", 3);
    m_AtomicOperations.insert(HashMapStringKey("InterlockedCompareStore"));

#undef DEFINE_STUB

#define DEFINE_VARIABLE(ShaderInd, IsOut, Semantic, Variable) m_HLSLSemanticToGLSLVar[ShaderInd][IsOut].emplace(make_pair(HashMapStringKey(Semantic), Variable))
    DEFINE_VARIABLE(VSInd, InVar, "sv_vertexid", "_GET_GL_VERTEX_ID");
    DEFINE_VARIABLE(VSInd, InVar, "sv_instanceid", "_GET_GL_INSTANCE_ID");
    DEFINE_VARIABLE(VSInd, OutVar, "sv_position", "_SET_GL_POSITION");

    DEFINE_VARIABLE(GSInd, InVar, "sv_position", "_GET_GL_POSITION");
    DEFINE_VARIABLE(GSInd, InVar, "sv_primitiveid", "_GET_GL_PRIMITIVE_ID");
    DEFINE_VARIABLE(GSInd, OutVar, "sv_position", "_SET_GL_POSITION");
    DEFINE_VARIABLE(GSInd, OutVar, "sv_rendertargetarrayindex", "_SET_GL_LAYER");

    DEFINE_VARIABLE(HSInd, InVar, "sv_outputcontrolpointid", "_GET_GL_INVOCATION_ID");
    DEFINE_VARIABLE(HSInd, InVar, "sv_primitiveid", "_GET_GL_PRIMITIVE_ID");
    DEFINE_VARIABLE(HSInd, InVar, "sv_position", "_GET_GL_POSITION");
    DEFINE_VARIABLE(HSInd, OutVar, "sv_position", "_SET_GL_POSITION");
    DEFINE_VARIABLE(HSInd, OutVar, "sv_tessfactor", "_SetGLTessLevelOuter");
    DEFINE_VARIABLE(HSInd, OutVar, "sv_insidetessfactor", "_SetGLTessLevelInner");

    DEFINE_VARIABLE(DSInd, InVar, "sv_position", "_GET_GL_POSITION");
    DEFINE_VARIABLE(DSInd, InVar, "sv_tessfactor", "_GetGLTessLevelOuter");
    DEFINE_VARIABLE(DSInd, InVar, "sv_insidetessfactor", "_GetGLTessLevelInner");
    DEFINE_VARIABLE(DSInd, InVar, "sv_domainlocation", "_GET_GL_TESS_COORD");
    DEFINE_VARIABLE(DSInd, InVar, "sv_primitiveid", "_GET_GL_PRIMITIVE_ID");
    DEFINE_VARIABLE(DSInd, OutVar, "sv_position", "_SET_GL_POSITION");

    DEFINE_VARIABLE(PSInd, InVar, "sv_position", "_GET_GL_FRAG_COORD");
    DEFINE_VARIABLE(PSInd, InVar, "sv_isfrontface", "_GET_GL_FRONT_FACING");
    DEFINE_VARIABLE(PSInd, OutVar, "sv_depth", "_SET_GL_FRAG_DEPTH");

    DEFINE_VARIABLE(CSInd, InVar, "sv_dispatchthreadid", "_GET_GL_GLOBAL_INVOCATION_ID");
    DEFINE_VARIABLE(CSInd, InVar, "sv_groupid", "_GET_GL_WORK_GROUP_ID");
    DEFINE_VARIABLE(CSInd, InVar, "sv_groupthreadid", "_GET_GL_LOCAL_INVOCATION_ID");
    DEFINE_VARIABLE(CSInd, InVar, "sv_groupindex", "_GET_GL_LOCAL_INVOCATION_INDEX");
#undef DEFINE_VARIABLE
}

String CompressNewLines(const String& Str)
{
    String Out;
    auto   Char = Str.begin();
    while (Char != Str.end())
    {
        if (*Char == '\r')
        {
            ++Char;
            // Replace \r\n with \n
            if (Char != Str.end() && *Char == '\n')
            {
                Out.push_back('\n');
                ++Char;
            }
            else
                Out.push_back('\r');
        }
        else
        {
            Out.push_back(*(Char++));
        }
    }
    return Out;
}

static Int32 CountNewLines(const String& Str)
{
    Int32 NumNewLines = 0;
    auto  Char        = Str.begin();
    while (Char != Str.end())
    {
        if (*Char == '\r')
        {
            ++NumNewLines;
            ++Char;
            // \r\n should be counted as one newline
            if (Char != Str.end() && *Char == '\n')
                ++Char;
        }
        else
        {
            if (*Char == '\n')
                ++NumNewLines;
            ++Char;
        }
    }
    return NumNewLines;
}


// IteratorType may be String::iterator or String::const_iterator.
// While iterator is convertible to const_iterator,
// iterator& cannot be converted to const_iterator& (Microsoft compiler allows
// such conversion, while gcc does not)
template <typename IteratorType>
String HLSL2GLSLConverterImpl::ConversionStream::PrintTokenContext(IteratorType& TargetToken, Int32 NumAdjacentLines)
{
    if (TargetToken == m_Tokens.end())
        --TargetToken;

    //\n  ++ x ;
    //\n  ++ y ;
    //\n  if ( x != 0 )
    //         ^
    //\n      x += y ;
    //\n
    //\n  if ( y != 0 )
    //\n      x += 2 ;

    const int NumSepChars = 20;
    String    Ctx(">");
    for (int i = 0; i < NumSepChars; ++i) Ctx.append("  >");
    Ctx.push_back('\n');

    // Find first token in the current line
    auto  CurrLineStartToken = TargetToken;
    Int32 NumLinesAbove      = 0;
    while (CurrLineStartToken != m_Tokens.begin())
    {
        NumLinesAbove += CountNewLines(CurrLineStartToken->Delimiter);
        if (NumLinesAbove > 0)
            break;
        --CurrLineStartToken;
    }
    //\n  if( x != 0 )
    //    ^

    // Find first token in the line NumAdjacentLines above
    auto TopLineStart = CurrLineStartToken;
    while (TopLineStart != m_Tokens.begin() && NumLinesAbove <= NumAdjacentLines)
    {
        --TopLineStart;
        NumLinesAbove += CountNewLines(TopLineStart->Delimiter);
    }
    //\n  ++ x ;
    //    ^
    //\n  ++ y ;
    //\n  if ( x != 0 )

    // Write everything from the top line up to the current line start
    auto Token = TopLineStart;
    for (; Token != CurrLineStartToken; ++Token)
    {
        Ctx.append(CompressNewLines(Token->Delimiter));
        Ctx.append(Token->Literal);
    }

    //\n  if ( x != 0 )
    //    ^

    Int32  NumLinesBelow = 0;
    String Spaces; // Accumulate whitespaces preceding current token
    bool   AccumWhiteSpaces = true;
    while (Token != m_Tokens.end() && NumLinesBelow == 0)
    {
        if (AccumWhiteSpaces)
        {
            for (const auto& Char : Token->Delimiter)
            {
                if (IsNewLine(Char))
                    Spaces.clear();
                else if (Char == '\t')
                    Spaces.push_back(Char);
                else
                    Spaces.push_back(' ');
            }
        }

        // Acumulate spaces until we encounter current token
        if (Token == TargetToken)
            AccumWhiteSpaces = false;

        if (AccumWhiteSpaces)
            Spaces.append(Token->Literal.length(), ' ');

        Ctx.append(CompressNewLines(Token->Delimiter));
        Ctx.append(Token->Literal);
        ++Token;

        if (Token == m_Tokens.end())
            break;

        NumLinesBelow += CountNewLines(Token->Delimiter);
    }

    // Write ^ on the line below
    Ctx.push_back('\n');
    Ctx.append(Spaces);
    Ctx.push_back('^');

    // Write NumAdjacentLines lines below current line
    while (Token != m_Tokens.end() && NumLinesBelow <= NumAdjacentLines)
    {
        Ctx.append(CompressNewLines(Token->Delimiter));
        Ctx.append(Token->Literal);
        ++Token;

        if (Token == m_Tokens.end())
            break;

        NumLinesBelow += CountNewLines(Token->Delimiter);
    }

    Ctx.append("\n<");
    for (int i = 0; i < NumSepChars; ++i) Ctx.append("  <");
    Ctx.push_back('\n');

    return Ctx;
}


#define VERIFY_PARSER_STATE(Token, Condition, ...)                       \
    do                                                                   \
    {                                                                    \
        if (!(Condition))                                                \
        {                                                                \
            auto err = FormatString(__VA_ARGS__);                        \
            LOG_ERROR_AND_THROW(err, "\n", PrintTokenContext(Token, 4)); \
        }                                                                \
    } while (false)

template <typename IterType>
bool SkipPrefix(const Char* RefStr, IterType& begin, IterType end)
{
    auto pos = begin;
    while (*RefStr && pos != end)
    {
        if (*(RefStr++) != *(pos++))
            return false;
    }
    if (*RefStr == 0)
    {
        begin = pos;
        return true;
    }

    return false;
}

// The method scans the source code and replaces
// all #include directives with the contents of the
// file. It maintains a set of already parsed includes
// to avoid double inclusion
void HLSL2GLSLConverterImpl::ConversionStream::InsertIncludes(String& GLSLSource, IShaderSourceInputStreamFactory* pSourceStreamFactory)
{
    // Put all the includes into the set to avoid multiple inclusion
    std::unordered_set<String> ProcessedIncludes;

    do
    {
        // Find the first #include statement
        auto Pos             = GLSLSource.begin();
        auto IncludeStartPos = GLSLSource.end();
        while (Pos != GLSLSource.end())
        {
            // #   include "TestFile.fxh"
            if (SkipDelimetersAndComments(GLSLSource, Pos))
                break;
            if (*Pos == '#')
            {
                IncludeStartPos = Pos;
                // #   include "TestFile.fxh"
                // ^
                ++Pos;
                // #   include "TestFile.fxh"
                //  ^
                if (SkipDelimetersAndComments(GLSLSource, Pos))
                {
                    // End of the file reached - break
                    break;
                }
                // #   include "TestFile.fxh"
                //     ^
                if (SkipPrefix("include", Pos, GLSLSource.end()))
                {
                    // #   include "TestFile.fxh"
                    //            ^
                    break;
                }
                else
                {
                    // This is not an #include directive:
                    // #define MACRO
                    // Continue search through the file
                }
            }
            else
                ++Pos;
        }

        // No more #include found
        if (Pos == GLSLSource.end())
            break;

        // Find open quotes
        if (SkipDelimetersAndComments(GLSLSource, Pos))
            LOG_ERROR_AND_THROW("Unexpected EOF after #include directive");
        // #   include "TestFile.fxh"
        //             ^
        if (*Pos != '\"' && *Pos != '<')
            LOG_ERROR_AND_THROW("Missing open quotes or \'<\' after #include directive");
        ++Pos;
        // #   include "TestFile.fxh"
        //              ^
        auto IncludeNameStartPos = Pos;
        // Find closing quotes
        while (Pos != GLSLSource.end() && *Pos != '\"' && *Pos != '>') ++Pos;
        // #   include "TestFile.fxh"
        //                          ^
        if (Pos == GLSLSource.end())
            LOG_ERROR_AND_THROW("Missing closing quotes or \'>\' after #include directive");

        // Get the name of the include file
        auto IncludeName = String(IncludeNameStartPos, Pos);
        ++Pos;
        // #   include "TestFile.fxh"
        // ^                         ^
        // IncludeStartPos           Pos
        GLSLSource.erase(IncludeStartPos, Pos);

        // Convert the name to lower case
        String IncludeFileLowercase = StrToLower(IncludeName);
        // Insert the lower-case name into the set
        auto It = ProcessedIncludes.insert(IncludeFileLowercase);
        // If the name was actually inserted, which means the include encountered for the first time,
        // replace the text with the file content
        if (It.second)
        {
            RefCntAutoPtr<IFileStream> pIncludeDataStream;
            pSourceStreamFactory->CreateInputStream(IncludeName.c_str(), &pIncludeDataStream);
            if (!pIncludeDataStream)
                LOG_ERROR_AND_THROW("Failed to open include file ", IncludeName);
            RefCntAutoPtr<IDataBlob> pIncludeData(MakeNewRCObj<DataBlobImpl>()(0));
            pIncludeDataStream->ReadBlob(pIncludeData);

            // Get include text
            auto   IncludeText = reinterpret_cast<const Char*>(pIncludeData->GetDataPtr());
            size_t NumSymbols  = pIncludeData->GetSize();

            // Insert the text into source
            GLSLSource.insert(IncludeStartPos - GLSLSource.begin(), IncludeText, NumSymbols);
        }
    } while (true);
}


void ReadNumericConstant(const String& Source, String::const_iterator& Pos, String& Output)
{
#define COPY_SYMBOL()                    \
    {                                    \
        Output.push_back(*(Pos++));      \
        if (Pos == Source.end()) return; \
    }

    while (Pos != Source.end() && *Pos >= '0' && *Pos <= '9')
        COPY_SYMBOL()

    if (*Pos == '.')
    {
        COPY_SYMBOL()
        // Copy all numbers
        while (Pos != Source.end() && *Pos >= '0' && *Pos <= '9')
            COPY_SYMBOL()
    }

    // Scientific notation
    // e+1242, E-234
    if (*Pos == 'e' || *Pos == 'E')
    {
        COPY_SYMBOL()

        if (*Pos == '+' || *Pos == '-')
            COPY_SYMBOL()

        // Skip all numbers
        while (Pos != Source.end() && *Pos >= '0' && *Pos <= '9')
            COPY_SYMBOL()
    }

    if (*Pos == 'f' || *Pos == 'F')
        COPY_SYMBOL()
#undef COPY_SYMBOL
}


// The function convertes source code into a token list
void HLSL2GLSLConverterImpl::ConversionStream::Tokenize(const String& Source)
{
#define CHECK_END(...)                      \
    do                                      \
    {                                       \
        if (SrcPos == Source.end())         \
        {                                   \
            LOG_ERROR_MESSAGE(__VA_ARGS__); \
            break;                          \
        }                                   \
    } while (false)

    int OpenBracketCount = 0;
    int OpenBraceCount   = 0;
    int OpenStapleCount  = 0;

    // Push empty node in the beginning of the list to facilitate
    // backwards searching
    m_Tokens.push_back(TokenInfo());

    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb509638(v=vs.85).aspx

    // Notes:
    // * Operators +, - are not detected
    //   * This might be a + b, -a or -10
    // * Operator ?: is not detected
    auto SrcPos = Source.begin();
    while (SrcPos != Source.end())
    {
        TokenInfo NewToken;
        auto      DelimStart = SrcPos;
        SkipDelimetersAndComments(Source, SrcPos);
        if (DelimStart != SrcPos)
        {
            auto DelimSize = SrcPos - DelimStart;
            NewToken.Delimiter.reserve(DelimSize);
            NewToken.Delimiter.append(DelimStart, SrcPos);
        }
        if (SrcPos == Source.end())
            break;

        switch (*SrcPos)
        {
            case '#':
            {
                NewToken.Type       = TokenType::PreprocessorDirective;
                auto DirectiveStart = SrcPos;
                ++SrcPos;
                SkipDelimetersAndComments(Source, SrcPos);
                CHECK_END("Missing preprocessor directive");
                SkipIdentifier(Source, SrcPos);
                auto DirectiveSize = SrcPos - DirectiveStart;
                NewToken.Literal.reserve(DirectiveSize);
                NewToken.Literal.append(DirectiveStart, SrcPos);
            }
            break;

            case ';':
                NewToken.Type = TokenType::Semicolon;
                NewToken.Literal.push_back(*(SrcPos++));
                break;

            case '=':
                if (m_Tokens.size() > 0 && NewToken.Delimiter == "")
                {
                    auto& LastToken = m_Tokens.back();
                    // +=, -=, *=, /=, %=, <<=, >>=, &=, |=, ^=
                    if (LastToken.Literal == "+" ||
                        LastToken.Literal == "-" ||
                        LastToken.Literal == "*" ||
                        LastToken.Literal == "/" ||
                        LastToken.Literal == "%" ||
                        LastToken.Literal == "<<" ||
                        LastToken.Literal == ">>" ||
                        LastToken.Literal == "&" ||
                        LastToken.Literal == "|" ||
                        LastToken.Literal == "^")
                    {
                        LastToken.Type = TokenType::Assignment;
                        LastToken.Literal.push_back(*(SrcPos++));
                        continue;
                    }
                    else if (LastToken.Literal == "<" ||
                             LastToken.Literal == ">" ||
                             LastToken.Literal == "=" ||
                             LastToken.Literal == "!")
                    {
                        LastToken.Type = TokenType::ComparisonOp;
                        LastToken.Literal.push_back(*(SrcPos++));
                        continue;
                    }
                }

                NewToken.Type = TokenType::Assignment;
                NewToken.Literal.push_back(*(SrcPos++));
                break;

            case '|':
            case '&':
                if (m_Tokens.size() > 0 && NewToken.Delimiter == "" &&
                    m_Tokens.back().Literal.length() == 1 && m_Tokens.back().Literal[0] == *SrcPos)
                {
                    m_Tokens.back().Type = TokenType::BooleanOp;
                    m_Tokens.back().Literal.push_back(*(SrcPos++));
                    continue;
                }
                else
                {
                    NewToken.Type = TokenType::BitwiseOp;
                    NewToken.Literal.push_back(*(SrcPos++));
                }
                break;

            case '<':
            case '>':
                if (m_Tokens.size() > 0 && NewToken.Delimiter == "" &&
                    m_Tokens.back().Literal.length() == 1 && m_Tokens.back().Literal[0] == *SrcPos)
                {
                    m_Tokens.back().Type = TokenType::BitwiseOp;
                    m_Tokens.back().Literal.push_back(*(SrcPos++));
                    continue;
                }
                else
                {
                    // Note: we do not distinguish between comparison operators
                    // and template arguments like in Texture2D<float> at this
                    // point. This will be clarified when textures are processed.
                    NewToken.Type = TokenType::ComparisonOp;
                    NewToken.Literal.push_back(*(SrcPos++));
                }
                break;

            case '+':
            case '-':
                if (m_Tokens.size() > 0 && NewToken.Delimiter == "" &&
                    m_Tokens.back().Literal.length() == 1 && m_Tokens.back().Literal[0] == *SrcPos)
                {
                    m_Tokens.back().Type = TokenType::IncDecOp;
                    m_Tokens.back().Literal.push_back(*(SrcPos++));
                    continue;
                }
                else
                {
                    // We do not currently distinguish between math operator a + b,
                    // unary operator -a and numerical constant -1:
                    NewToken.Literal.push_back(*(SrcPos++));
                }
                break;

            case '~':
            case '^':
                NewToken.Type = TokenType::BitwiseOp;
                NewToken.Literal.push_back(*(SrcPos++));
                break;

            case '*':
            case '/':
            case '%':
                NewToken.Type = TokenType::MathOp;
                NewToken.Literal.push_back(*(SrcPos++));
                break;

            case '!':
                NewToken.Type = TokenType::BooleanOp;
                NewToken.Literal.push_back(*(SrcPos++));
                break;

            case ',':
                NewToken.Type = TokenType::Comma;
                NewToken.Literal.push_back(*(SrcPos++));
                break;

            case '"':
                //[domain("quad")]
                //        ^
                NewToken.Type = TokenType::SrtingConstant;
                ++SrcPos;
                //[domain("quad")]
                //         ^
                while (SrcPos != Source.end() && *SrcPos != '"')
                    NewToken.Literal.push_back(*(SrcPos++));
                //[domain("quad")]
                //             ^
                if (SrcPos != Source.end())
                    ++SrcPos;
                //[domain("quad")]
                //              ^
                break;

#define BRACKET_CASE(Symbol, TokenType, Action)  \
    case Symbol:                                 \
        NewToken.Type = TokenType;               \
        NewToken.Literal.push_back(*(SrcPos++)); \
        Action;                                  \
        break;

                BRACKET_CASE('(', TokenType::OpenBracket, ++OpenBracketCount);
                BRACKET_CASE(')', TokenType::ClosingBracket, --OpenBracketCount);
                BRACKET_CASE('{', TokenType::OpenBrace, ++OpenBraceCount);
                BRACKET_CASE('}', TokenType::ClosingBrace, --OpenBraceCount);
                BRACKET_CASE('[', TokenType::OpenStaple, ++OpenStapleCount);
                BRACKET_CASE(']', TokenType::ClosingStaple, --OpenStapleCount);

#undef BRACKET_CASE


            default:
            {
                auto IdentifierStartPos = SrcPos;
                SkipIdentifier(Source, SrcPos);
                if (IdentifierStartPos != SrcPos)
                {
                    auto IDSize = SrcPos - IdentifierStartPos;
                    NewToken.Literal.reserve(IDSize);
                    NewToken.Literal.append(IdentifierStartPos, SrcPos);
                    auto KeywordIt = m_Converter.m_HLSLKeywords.find(NewToken.Literal.c_str());
                    if (KeywordIt != m_Converter.m_HLSLKeywords.end())
                    {
                        NewToken.Type = KeywordIt->second.Type;
                        VERIFY(NewToken.Literal == KeywordIt->second.Literal, "Inconsistent literal");
                    }
                    else
                    {
                        NewToken.Type = TokenType::Identifier;
                    }
                }

                if (NewToken.Type == TokenType::Undefined)
                {
                    bool bIsNumericalCostant = *SrcPos >= '0' && *SrcPos <= '9';
                    if (!bIsNumericalCostant && *SrcPos == '.')
                    {
                        auto NextPos        = SrcPos + 1;
                        bIsNumericalCostant = NextPos != Source.end() && *NextPos >= '0' && *NextPos <= '9';
                    }
                    if (bIsNumericalCostant)
                    {
                        ReadNumericConstant(Source, SrcPos, NewToken.Literal);
                        NewToken.Type = TokenType::NumericConstant;
                    }
                }

                if (NewToken.Type == TokenType::Undefined)
                {
                    NewToken.Literal.push_back(*(SrcPos++));
                }
                // Operators
                // https://msdn.microsoft.com/en-us/library/windows/desktop/bb509631(v=vs.85).aspx
            }
        }

        m_Tokens.push_back(NewToken);
    }
#undef CHECK_END
}


void HLSL2GLSLConverterImpl::ConversionStream::FindClosingBracket(TokenListType::iterator&       Token,
                                                                  const TokenListType::iterator& ScopeEnd,
                                                                  TokenType                      OpenBracketType,
                                                                  TokenType                      ClosingBracketType)
{
    VERIFY_EXPR(Token->Type == OpenBracketType);
    ++Token; // Skip open bracket
    int BracketCount = 1;
    // Find matching closing bracket
    while (Token != ScopeEnd)
    {
        if (Token->Type == OpenBracketType)
            ++BracketCount;
        else if (Token->Type == ClosingBracketType)
        {
            --BracketCount;
            if (BracketCount == 0)
                break;
        }
        ++Token;
    }
    VERIFY_PARSER_STATE(Token, BracketCount == 0, "No matching closing bracket found in the scope");
}

// The function replaces cbuffer with uniform and adds semicolon if it is missing after the closing brace:
// cbuffer
// {
//    ...
// }; <- Semicolon must be here
//
void HLSL2GLSLConverterImpl::ConversionStream::ProcessConstantBuffer(TokenListType::iterator& Token)
{
    VERIFY_EXPR(Token->Type == TokenType::kw_cbuffer);

    // Replace "cbuffer" with "uniform"
    Token->Literal = "uniform";
    ++Token;
    // cbuffer CBufferName
    //         ^

    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF after \"cbuffer\" keyword");
    VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Identifier, "Identifier expected after \"cbuffer\" keyword");
    const auto& CBufferName = Token->Literal;

    ++Token;
    // cbuffer CBufferName
    //                    ^
    if (Token->Literal == ":")
    {
        // cbuffer CBufferName : register(b0) {
        //                     ^

        // Remove register
        while (Token != m_Tokens.end() && Token->Type != TokenType::OpenBrace)
        {
            auto CurrToken = Token;
            ++Token;
            m_Tokens.erase(CurrToken);
        }
        // cbuffer CBufferName {
        //                     ^
    }

    while (Token != m_Tokens.end() && Token->Type != TokenType::OpenBrace)
        ++Token;
    // cbuffer CBufferName
    // {
    // ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Missing open brace in the definition of cbuffer ", CBufferName);

    // Find closing brace
    FindClosingBracket(Token, m_Tokens.end(), TokenType::OpenBrace, TokenType::ClosingBrace);

    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "No matching closing brace found in the definition of cbuffer ", CBufferName);
    ++Token; // Skip closing brace
    // cbuffer CBufferName
    // {
    //    ...
    // }
    // int a
    // ^

    if (Token == m_Tokens.end() || Token->Type != TokenType::Semicolon)
    {
        m_Tokens.insert(Token, TokenInfo(TokenType::Semicolon, ";"));
        // cbuffer CBufferName
        // {
        //    ...
        // };
        // int a;
        // ^
    }
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessStructuredBuffer(TokenListType::iterator& Token, Uint32& ShaderStorageBlockBinding)
{
    // StructuredBuffer<DataType> g_Data;
    // ^
    VERIFY_EXPR(Token->Type == TokenType::kw_StructuredBuffer || Token->Type == TokenType::kw_RWStructuredBuffer);
    if (Token->Type == TokenType::kw_RWStructuredBuffer)
    {
        std::stringstream ss;
        ss << "layout(std140, binding=" << ShaderStorageBlockBinding << ") buffer";
        Token->Literal = ss.str();
        ++ShaderStorageBlockBinding;
    }
    else
        Token->Literal = "layout(std140) readonly buffer";
    // buffer<DataType> g_Data;
    // ^

    ++Token;
    // buffer<DataType> g_Data;
    //       ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF after \"StructuredBuffer\" keyword");
    VERIFY_PARSER_STATE(Token, Token->Literal == "<", "\'<\' expected after \"StructuredBuffer\" keyword");
    Token->Literal = "{";
    Token->Type    = TokenType::OpenBrace;
    // buffer{DataType> g_Data;
    //       ^
    auto OpenBraceToken = Token;

    ++Token;
    // buffer{DataType> g_Data;
    //        ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF after");
    VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Identifier, "Identifier expected in Structured Buffer definition");

    ++Token;
    // buffer{DataType> g_Data;
    //                ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF in Structured Buffer definition");
    VERIFY_PARSER_STATE(Token, Token->Literal == ">", "\'>\' expected after type definition");
    auto ClosingAngleBracketTkn = Token;
    ++Token;
    m_Tokens.erase(ClosingAngleBracketTkn);
    // buffer{DataType g_Data;
    //                 ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF after");
    VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Identifier, "Identifier expected in Structured Buffer definition");
    if (Token->Delimiter.empty())
        Token->Delimiter = " ";

    m_Tokens.insert(OpenBraceToken, TokenInfo(TokenType::Identifier, Token->Literal.c_str(), " "));
    //          OpenBraceToken
    //              V
    // buffer g_Data{DataType g_Data;
    //                        ^
    auto GlobalVarNameToken = Token;

    ++Token;
    // buffer g_Data{DataType g_Data;
    //                              ^

    if (Token->Literal == ":")
    {
        // buffer g_Data{DataType g_Data : register(t0);
        //                               ^

        // Remove register
        while (Token != m_Tokens.end() && Token->Type != TokenType::Semicolon)
        {
            auto CurrToken = Token;
            ++Token;
            m_Tokens.erase(CurrToken);
        }

        // buffer g_Data{DataType g_Data ;
        //                               ^
    }
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF after");
    VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Semicolon, "\';\' expected");

    m_Tokens.insert(Token, TokenInfo(TokenType::OpenStaple, "["));
    m_Tokens.insert(Token, TokenInfo(TokenType::ClosingStaple, "]"));
    m_Tokens.insert(Token, TokenInfo(TokenType::Semicolon, ";"));
    m_Tokens.insert(Token, TokenInfo(TokenType::ClosingBrace, "}"));
    // buffer g_Data{DataType g_Data[]};
    //                                 ^
    ++Token;
    String NameRedefine("#define ");
    NameRedefine += GlobalVarNameToken->Literal + ' ' + GlobalVarNameToken->Literal + "_data\r\n";
    m_Tokens.insert(Token, TokenInfo(TokenType::TextBlock, NameRedefine.c_str(), "\r\n"));
    GlobalVarNameToken->Literal.append("_data");
    // buffer g_Data{DataType g_Data_data[]};
    // #define g_Data g_Data_data
    //                           ^
}

void HLSL2GLSLConverterImpl::ConversionStream::RegisterStruct(TokenListType::iterator& Token)
{
    // struct VSOutput
    // ^
    VERIFY_EXPR(Token->Type == TokenType::kw_struct && Token->Literal == "struct");

    ++Token;
    // struct VSOutput
    //        ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && Token->Type == TokenType::Identifier, "Identifier expected");
    auto& StructName = Token->Literal;
    m_StructDefinitions.insert(std::make_pair(StructName.c_str(), Token));

    ++Token;
    // struct VSOutput
    // {
    // ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && Token->Type == TokenType::OpenBrace, "Open brace expected");

    // Find closing brace
    FindClosingBracket(Token, m_Tokens.end(), TokenType::OpenBrace, TokenType::ClosingBrace);
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Missing closing brace for structure \"", StructName, "\"");
    // }
    // ^
    ++Token;
}


// The function finds all sampler states in the current scope ONLY, and puts them into the
// hash table. The hash table indicates if the sampler is comparison or not. It is required to
// match HLSL texture declaration to sampler* or sampler*Shadow.
//
// GLSL only allows samplers as uniform variables and function agruments. It does not allow
// local variables of sampler type. So the two possible scopes the function can process are
// global scope and the function argument list.
//
// Only samplers in the current scope are processed, all samplers in nested scopes are ignored
//
// After the function returns, Token points to the end of the scope (m_Tokens.end() for global scope,
// or closing bracket for the function argument list)
//
// Example 1:
//
//   Token
//   |
//    SamplerState g_Sampler;
//    SamplerComparsionState g_CmpSampler;
//    void Function(in SamplerState in_Sampler)
//    {
//    }
//
// SamplersHash = { {g_Sampler, "false"}, { {g_CmpSampler, "true"} }
//
// Example 2:
//
//    SamplerState g_Sampler;
//    SamplerComparsionState g_CmpSampler;
//                 Token
//                 |
//    void Function(in SamplerState in_Sampler)
//    {
//    }
//
// SamplersHash = { {in_Sampler, "false"} }
//
void HLSL2GLSLConverterImpl::ConversionStream::ParseSamplers(TokenListType::iterator& Token, SamplerHashType& SamplersHash)
{
    VERIFY_EXPR(Token->Type == TokenType::OpenBracket || Token->Type == TokenType::OpenBrace || Token == m_Tokens.begin());
    Uint32 ScopeDepth             = 1;
    bool   IsFunctionArgumentList = Token->Type == TokenType::OpenBracket;

    // Skip scope start symbol, which is either open bracket or m_Tokens.begin()
    ++Token;
    while (Token != m_Tokens.end() && ScopeDepth > 0)
    {
        if (Token->Type == TokenType::OpenBracket ||
            Token->Type == TokenType::OpenBrace)
        {
            // Increase scope depth
            ++ScopeDepth;
            ++Token;
        }
        else if (Token->Type == TokenType::ClosingBracket ||
                 Token->Type == TokenType::ClosingBrace)
        {
            // Decrease scope depth
            --ScopeDepth;
            if (ScopeDepth == 0)
                break;
            ++Token;
        }
        else if ((Token->Type == TokenType::kw_SamplerState ||
                  Token->Type == TokenType::kw_SamplerComparisonState) &&
                 // ONLY parse sampler states in the current scope, skip
                 // all nested scopes
                 ScopeDepth == 1)
        {
            const auto& SamplerType   = Token->Literal;
            bool        bIsComparison = Token->Type == TokenType::kw_SamplerComparisonState;
            // SamplerState LinearClamp;
            // ^
            ++Token;

            // There may be a number of samplers declared after single
            // Sampler[Comparison]State keyword:
            // SamplerState Tex2D1_sampler, Tex2D2_sampler;
            do
            {
                // SamplerState LinearClamp;
                //              ^
                VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF in ", SamplerType, " declaration");
                VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Identifier, "Missing identifier in ", SamplerType, " declaration");
                const auto& SamplerName = Token->Literal;

                // Add sampler state into the hash map
                SamplersHash.insert(std::make_pair(SamplerName, bIsComparison));

                ++Token;
                // SamplerState LinearClamp ;
                //                          ^

                // We cannot just remove sampler declarations, because samplers can
                // be passed to functions as arguments.
                // SamplerState and SamplerComparisonState are #defined as int, so all
                // sampler variables will just be unused global variables or function parameters.
                // Hopefully GLSL compiler will be able to optimize them out.

                if (IsFunctionArgumentList)
                {
                    // In function argument list, every arument
                    // has its own type declaration
                    break;
                }

                // Go to the next sampler declaraion or statement end
                while (Token != m_Tokens.end() && Token->Type != TokenType::Comma && Token->Type != TokenType::Semicolon)
                    ++Token;
                VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while parsing ", SamplerType, " declaration");

                if (Token->Type == TokenType::Comma)
                {
                    // SamplerState Tex2D1_sampler, Tex2D2_sampler ;
                    //                            ^
                    ++Token;
                    // SamplerState Tex2D1_sampler, Tex2D2_sampler ;
                    //                              ^
                }
                else
                {
                    // SamplerState Tex2D1_sampler, Tex2D2_sampler ;
                    //                                             ^
                    break;
                }
            } while (Token != m_Tokens.end());
        }
        else
            ++Token;
    }
    VERIFY_PARSER_STATE(Token, ScopeDepth == 1 && Token == m_Tokens.end() || ScopeDepth == 0, "Error parsing scope");
}

void ParseImageFormat(const String& Comment, String& ImageFormat)
{
    //    /* format = r32f */
    // ^
    auto Pos = Comment.begin();
    if (SkipDelimeters(Comment, Pos))
        return;
    //    /* format = r32f */
    //    ^
    if (*Pos != '/')
        return;
    ++Pos;
    //    /* format = r32f */
    //     ^
    //    // format = r32f
    //     ^
    if (Pos == Comment.end() || (*Pos != '/' && *Pos != '*'))
        return;
    ++Pos;
    //    /* format = r32f */
    //      ^
    if (SkipDelimeters(Comment, Pos))
        return;
    //    /* format = r32f */
    //       ^
    if (!SkipPrefix("format", Pos, Comment.end()))
        return;
    //    /* format = r32f */
    //             ^
    if (SkipDelimeters(Comment, Pos))
        return;
    //    /* format = r32f */
    //              ^
    if (*Pos != '=')
        return;
    ++Pos;
    //    /* format = r32f */
    //               ^
    if (SkipDelimeters(Comment, Pos))
        return;
    //    /* format = r32f */
    //                ^

    auto ImgFmtStartPos = Pos;
    SkipIdentifier(Comment, Pos);

    ImageFormat = String(ImgFmtStartPos, Pos);
}


// The function processes texture declaration that is indicated by Token, converts it to
// corresponding GLSL sampler type and adds the new sampler into Objects hash map.
//
// Samplers is the stack of sampler states found in all nested scopes.
// GLSL only supports samplers as global uniform variables or function arguments.
// Consequently, there are two possible levels in Samplers stack:
// level 0 - global sampler states (always present)
// level 1 - samplers declared as function arguments (only present when parsing function body)
//
// The function uses the following rules to convert HLSL texture declaration into GLSL sampler:
// - HLSL texture dimension defines GLSL sampler dimension:
//      - Texture2D   -> sampler2D
//      - TextureCube -> samplerCube
// - HLSL texture component type defines GLSL sampler type. If no type is specified, float4 is assumed:
//      - Texture2D<float>     -> sampler2D
//      - Texture3D<uint4>     -> usampler3D
//      - Texture2DArray<int2> -> isampler2DArray
//      - Texture2D            -> sampler2D
// - To distinguish if sampler should be shadow or not, the function tries to find <Texture Name>_sampler
//   in the provided sampler state stack. If the sampler type is comparison, the texture is converted
//   to shadow sampler. If sampler state is either not comparison or not found, regular sampler is used
//   Examples:
//      - Texture2D g_ShadowMap;                        -> sampler2DShadow
//        SamplerComparisonState g_ShadowMap_sampler;
//      - Texture2D g_Tex2D;                            -> sampler2D g_Tex2D;
//        SamplerState g_Tex2D_sampler;
//        Texture3D g_Tex3D;                            -> sampler3D g_Tex3D;
//
void HLSL2GLSLConverterImpl::ConversionStream::ProcessTextureDeclaration(TokenListType::iterator&            Token,
                                                                         const std::vector<SamplerHashType>& Samplers,
                                                                         ObjectsTypeHashType&                Objects,
                                                                         const char*                         SamplerSuffix,
                                                                         Uint32&                             ImageBinding)
{
    auto TexDeclToken = Token;
    auto TextureDim   = TexDeclToken->Type;
    // Texture2D < float > ... ;
    // ^
    bool IsRWTexture =
        TextureDim == TokenType::kw_RWTexture1D ||
        TextureDim == TokenType::kw_RWTexture1DArray ||
        TextureDim == TokenType::kw_RWTexture2D ||
        TextureDim == TokenType::kw_RWTexture2DArray ||
        TextureDim == TokenType::kw_RWTexture3D ||
        TextureDim == TokenType::kw_RWBuffer;
    String ImgFormat;

    ++Token;
    // Texture2D < float > ... ;
    //           ^
#define CHECK_EOF() VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF in ", TexDeclToken->Literal, " declaration")
    CHECK_EOF();

    auto   TypeDefinitionStart = Token;
    String GLSLSampler;
    String LayoutQualifier;
    Uint32 NumComponents = 0;
    if (Token->Literal == "<")
    {
        // Fix token type
        VERIFY_EXPR(Token->Type == TokenType::ComparisonOp);
        Token->Type = TokenType::OpenAngleBracket;

        ++Token;
        CHECK_EOF();
        // Texture2D < float > ... ;
        //             ^
        auto TexFmtToken = Token;
        VERIFY_PARSER_STATE(Token, Token->IsBuiltInType(), "Texture format type must be built-in type");
        if (Token->Type >= TokenType::kw_float && Token->Type <= TokenType::kw_float4)
        {
            if (Token->Type == TokenType::kw_float)
                NumComponents = 1;
            else
                NumComponents = static_cast<int>(Token->Type) - static_cast<int>(TokenType::kw_float);
        }
        else if (Token->Type >= TokenType::kw_int && Token->Type <= TokenType::kw_int4)
        {
            GLSLSampler.push_back('i');
            if (Token->Type == TokenType::kw_int)
                NumComponents = 1;
            else
                NumComponents = static_cast<int>(Token->Type) - static_cast<int>(TokenType::kw_int);
        }
        else if (Token->Type >= TokenType::kw_uint && Token->Type <= TokenType::kw_uint4)
        {
            GLSLSampler.push_back('u');
            if (Token->Type == TokenType::kw_uint)
                NumComponents = 1;
            else
                NumComponents = static_cast<int>(Token->Type) - static_cast<int>(TokenType::kw_uint);
        }
        else
        {
            VERIFY_PARSER_STATE(Token, false, Token->Literal, " is not valid texture component type\n"
                                                              "Only the following texture element types are supported: float[1,2,3,4], int[1,2,3,4], uint[1,2,3,4]");
        }
        VERIFY_PARSER_STATE(Token, NumComponents >= 1 && NumComponents <= 4, "Between 1 and 4 components expected, ", NumComponents, " deduced");

        ++Token;
        CHECK_EOF();
        // Texture2D < float > ... ;
        //                   ^
        if ((TextureDim == TokenType::kw_Texture2DMS ||
             TextureDim == TokenType::kw_Texture2DMSArray) &&
            Token->Literal == ",")
        {
            // Texture2DMS < float, 4 > ... ;
            //                    ^
            ++Token;
            CHECK_EOF();
            // Texture2DMS < float, 4 > ... ;
            //                      ^
            // Texture2DMS < float, SAMPLE_COUNT > ... ;
            //                      ^
            VERIFY_PARSER_STATE(Token, Token->Type == TokenType::NumericConstant || Token->Type == TokenType::Identifier,
                                "Number of samples is expected in ", TexDeclToken->Literal, " declaration");

            // We do not really need the number of samples, so just skip it
            ++Token;
            CHECK_EOF();
            // Texture2DMS < float, 4 > ... ;
            //                        ^
        }
        VERIFY_PARSER_STATE(Token, Token->Literal == ">", "Missing \">\" in ", TexDeclToken->Literal, " declaration");
        // Fix token type
        VERIFY_EXPR(Token->Type == TokenType::ComparisonOp);
        Token->Type = TokenType::ClosingAngleBracket;

        if (IsRWTexture)
        {
            // RWTexture2D<float /* format = r32f */ >
            //                                       ^
            ParseImageFormat(Token->Delimiter, ImgFormat);
            if (ImgFormat.length() == 0)
            {
                // RWTexture2D</* format = r32f */ float >
                //                                 ^
                //                            TexFmtToken
                ParseImageFormat(TexFmtToken->Delimiter, ImgFormat);
            }

            if (ImgFormat.length() != 0)
            {
                std::stringstream ss;
                ss << "layout(" << ImgFormat << ", binding=" << ImageBinding++ << ")";
                LayoutQualifier = ss.str();
            }
        }

        ++Token;
        // Texture2D < float > TexName ;
        //                     ^
        CHECK_EOF();
    }

    if (IsRWTexture)
        GLSLSampler.append("image");
    else
        GLSLSampler.append("sampler");

    switch (TextureDim)
    {
        // clang-format off
        case TokenType::kw_RWTexture1D:
        case TokenType::kw_Texture1D:          GLSLSampler += "1D";        break;

        case TokenType::kw_RWTexture1DArray:
        case TokenType::kw_Texture1DArray:     GLSLSampler += "1DArray";   break;

        case TokenType::kw_RWTexture2D:
        case TokenType::kw_Texture2D:          GLSLSampler += "2D";        break;

        case TokenType::kw_RWTexture2DArray:
        case TokenType::kw_Texture2DArray:     GLSLSampler += "2DArray";   break;

        case TokenType::kw_RWTexture3D:
        case TokenType::kw_Texture3D:          GLSLSampler += "3D";        break;

        case TokenType::kw_TextureCube:        GLSLSampler += "Cube";      break;
        case TokenType::kw_TextureCubeArray:   GLSLSampler += "CubeArray"; break;
        case TokenType::kw_Texture2DMS:        GLSLSampler += "2DMS";      break;
        case TokenType::kw_Texture2DMSArray:   GLSLSampler += "2DMSArray"; break;

        case TokenType::kw_RWBuffer:
        case TokenType::kw_Buffer:             GLSLSampler += "Buffer";    break;
        // clang-format on
        default: UNEXPECTED("Unexpected texture type");
    }

    //   TypeDefinitionStart
    //           |
    // Texture2D < float > TexName ;
    //                     ^
    m_Tokens.erase(TypeDefinitionStart, Token);
    // Texture2D TexName ;
    //           ^

    bool IsGlobalScope = Samplers.size() == 1;

    // There may be more than one texture variable declared in the same
    // statement:
    // Texture2D<float> g_Tex2D1, g_Tex2D1;
    do
    {
        // Texture2D TexName ;
        //           ^
        VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Identifier, "Identifier expected in ", TexDeclToken->Literal, " declaration");

        // Make sure there is a delimiter between sampler keyword and the
        // identifier. In cases like this
        // Texture2D<float>Name;
        // There will be no whitespace
        if (Token->Delimiter == "")
            Token->Delimiter = " ";

        // Texture2D TexName ;
        //           ^
        const auto& TextureName = Token->Literal;

        auto CompleteGLSLSampler = GLSLSampler;
        if (!IsRWTexture)
        {
            // Try to find matching sampler
            auto SamplerName = TextureName + SamplerSuffix;
            // Search all scopes starting with the innermost
            for (auto ScopeIt = Samplers.rbegin(); ScopeIt != Samplers.rend(); ++ScopeIt)
            {
                auto SamplerIt = ScopeIt->find(SamplerName);
                if (SamplerIt != ScopeIt->end())
                {
                    if (SamplerIt->second)
                        CompleteGLSLSampler.append("Shadow");
                    break;
                }
            }
        }

        // TexDeclToken
        // |
        // Texture2D TexName ;
        //           ^
        TexDeclToken->Literal = "";
        if (IsGlobalScope)
        {
            // Use layout qualifier for global variables only, not for function arguments
            TexDeclToken->Literal.append(LayoutQualifier);
            // Samplers and images in global scope must be declared uniform.
            // Function arguments must not be declared uniform
            TexDeclToken->Literal.append("uniform ");
            // From GLES 3.1 spec:
            //    Except for image variables qualified with the format qualifiers r32f, r32i, and r32ui,
            //    image variables must specify either memory qualifier readonly or the memory qualifier writeonly.
            // So on GLES we have to assume that an image is a writeonly variable
            if (IsRWTexture && ImgFormat != "r32f" && ImgFormat != "r32i" && ImgFormat != "r32ui")
                TexDeclToken->Literal.append("IMAGE_WRITEONLY "); // defined as 'writeonly' on GLES and as '' on desktop in GLSLDefinitions.h
        }
        TexDeclToken->Literal.append(CompleteGLSLSampler);
        Objects.m.insert(std::make_pair(HashMapStringKey(TextureName), HLSLObjectInfo(CompleteGLSLSampler, NumComponents)));

        // In global scope, multiple variables can be declared in the same statement
        if (IsGlobalScope)
        {
            // Texture2D TexName, TexName2 ;
            //           ^

            // Go to the next texture in the declaration or to the statement end, removing register declarations
            while (Token != m_Tokens.end() && Token->Type != TokenType::Comma && Token->Type != TokenType::Semicolon)
            {
                if (Token->Literal == ":")
                {
                    // Texture2D TexName : register(t0);
                    // Texture2D TexName : register(t0),
                    //                   ^

                    // Remove register
                    while (Token != m_Tokens.end() && Token->Type != TokenType::Comma && Token->Type != TokenType::Semicolon)
                    {
                        auto CurrToken = Token;
                        ++Token;
                        m_Tokens.erase(CurrToken);
                    }

                    // Texture2D TexName ,
                    //                   ^
                }
                else
                {
                    ++Token;
                }
            }

            if (Token != m_Tokens.end())
            {
                if (Token->Type == TokenType::Comma)
                {
                    // Texture2D TexName, TexName2 ;
                    //                  ^
                    Token->Type    = TokenType::Semicolon;
                    Token->Literal = ";";
                    // Texture2D TexName; TexName2 ;
                    //                  ^

                    ++Token;
                    // Texture2D TexName; TexName2 ;
                    //                    ^

                    // Insert empty token that will contain next sampler/image declaration
                    TexDeclToken = m_Tokens.insert(Token, TokenInfo(TextureDim, "", "\n"));
                    // Texture2D TexName;
                    // <Texture Declaration TBD> TexName2 ;
                    // ^                         ^
                    // TexDeclToken              Token
                }
                else
                {
                    // Texture2D TexName, TexName2 ;
                    //                             ^
                    ++Token;
                    break;
                }
            }
        }
    } while (IsGlobalScope && Token != m_Tokens.end());

#undef SKIP_DELIMITER
#undef CHECK_EOF
}


// Finds an HLSL object with the given name in object stack
const HLSL2GLSLConverterImpl::HLSLObjectInfo* HLSL2GLSLConverterImpl::ConversionStream::FindHLSLObject(const String& Name)
{
    for (auto ScopeIt = m_Objects.rbegin(); ScopeIt != m_Objects.rend(); ++ScopeIt)
    {
        auto It = ScopeIt->m.find(Name.c_str());
        if (It != ScopeIt->m.end())
            return &It->second;
    }
    return nullptr;
}

Uint32 HLSL2GLSLConverterImpl::ConversionStream::CountFunctionArguments(TokenListType::iterator& Token, const TokenListType::iterator& ScopeEnd)
{
    // TestText.Sample( TestText_sampler, float2(0.0, 1.0)  );
    //                ^
    VERIFY_EXPR(Token->Type == TokenType::OpenBracket);
    Uint32 NumArguments = 0;
    ProcessScope(
        Token, ScopeEnd, TokenType::OpenBracket, TokenType::ClosingBracket,
        [&](TokenListType::iterator& tkn, int ScopeDepth) {
            // Argument list is not empty, so there is at least one arument.
            if (NumArguments == 0)
                NumArguments = 1;
            // Number of additional arguments equals the number of commas
            // in scope depth 1

            // Do not count arguments of nested functions:
            // TestText.Sample( TestText_sampler, float2(0.0, 1.0)  );
            //                                          ^
            //                                        ScopeDepth == 2
            if (ScopeDepth == 1 && tkn->Literal == ",")
                ++NumArguments;
            ++tkn;
        } //
    );
    // TestText.Sample( TestText_sampler, float2(0.0, 1.0)  );
    //                                                      ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while processing argument list");
    VERIFY_EXPR(Token->Type == TokenType::ClosingBracket);
    ++Token;
    // TestText.Sample( TestText_sampler, float2(0.0, 1.0)  );
    //                                                       ^
    return NumArguments;
}

// The function processes HLSL object method in current scope and replaces it
// with the corresponding GLSL function stub
// Example:
// Texture2D<float2> Tex2D;
// ...
// Tex2D.Sample(Tex2D_sampler, f2UV) -> Sample_2(Tex2D, Tex2D_sampler, f2UV)_SWIZZLE2
bool HLSL2GLSLConverterImpl::ConversionStream::ProcessObjectMethod(TokenListType::iterator&       Token,
                                                                   const TokenListType::iterator& ScopeStart,
                                                                   const TokenListType::iterator& ScopeEnd)
{
    // TestText.Sample( ...
    //         ^
    //      DotToken
    auto DotToken = Token;
    VERIFY_EXPR(DotToken != ScopeEnd && Token->Literal == ".");
    auto MethodToken = DotToken;
    ++MethodToken;
    VERIFY_EXPR(MethodToken != ScopeEnd && MethodToken->Type == TokenType::Identifier);
    // TestText.Sample( ...
    //          ^
    //     MethodToken
    auto IdentifierToken = DotToken;
    // m_Tokens contains dummy node at the beginning, so we can
    // check for ScopeStart to break the loop
    while (IdentifierToken != ScopeStart && IdentifierToken->Type != TokenType::Identifier)
        --IdentifierToken;
    if (IdentifierToken == ScopeStart)
        return false;
    // TestTextArr[2].Sample( ...
    // ^
    // IdentifierToken

    // Try to find identifier
    const auto* pObjectInfo = FindHLSLObject(IdentifierToken->Literal);
    if (pObjectInfo == nullptr)
    {
        return false;
    }
    const auto& ObjectType = pObjectInfo->GLSLType;

    auto ArgsListStartToken = MethodToken;
    ++ArgsListStartToken;

    // TestText.Sample( ...
    //                ^
    //     ArgsListStartToken

    if (ArgsListStartToken == ScopeEnd || ArgsListStartToken->Type != TokenType::OpenBracket)
        return false;
    auto   ArgsListEndToken = ArgsListStartToken;
    Uint32 NumArguments     = CountFunctionArguments(ArgsListEndToken, ScopeEnd);

    if (ArgsListEndToken == ScopeEnd)
        return false;
    // TestText.Sample( TestText_sampler, float2(0.0, 1.0)  );
    //                                                       ^
    //                                               ArgsListEndToken
    auto StubIt = m_Converter.m_GLSLStubs.find(FunctionStubHashKey(ObjectType, MethodToken->Literal.c_str(), NumArguments));
    if (StubIt == m_Converter.m_GLSLStubs.end())
    {
        LOG_ERROR_MESSAGE("Unable to find function stub for ", IdentifierToken->Literal, ".", MethodToken->Literal, "(", NumArguments, " args). GLSL object type: ", ObjectType);
        return false;
    }

    //            DotToken
    //               V
    // TestTextArr[2].Sample( TestTextArr_sampler, ...
    // ^                    ^
    // IdentifierToken      ArgsListStartToken

    *ArgsListStartToken = TokenInfo(TokenType::Comma, ",");
    // TestTextArr[2].Sample, TestTextArr_sampler, ...
    //               ^      ^
    //           DotToken  ArgsListStartToken

    m_Tokens.erase(DotToken, ArgsListStartToken);
    // TestTextArr[2], TestTextArr_sampler, ...
    // ^
    // IdentifierToken

    m_Tokens.insert(IdentifierToken, TokenInfo(TokenType::Identifier, StubIt->second.Name.c_str(), IdentifierToken->Delimiter.c_str()));
    IdentifierToken->Delimiter = " ";
    // FunctionStub TestTextArr[2], TestTextArr_sampler, ...
    //              ^
    //              IdentifierToken


    m_Tokens.insert(IdentifierToken, TokenInfo(TokenType::OpenBracket, "("));
    // FunctionStub( TestTextArr[2], TestTextArr_sampler, ...
    //               ^
    //               IdentifierToken

    Token = ArgsListStartToken;
    // FunctionStub( TestTextArr[2], TestTextArr_sampler, ...
    //                             ^
    //                           Token

    // Nested function calls will be automatically processed:
    // FunctionStub( TestTextArr[2], TestTextArr_sampler, TestTex.Sample(...
    //                             ^
    //                           Token


    // Add swizzling if there is any
    if (StubIt->second.Swizzle.length() > 0)
    {
        // FunctionStub( TestTextArr[2], TestTextArr_sampler, ...    );
        //                                                            ^
        //                                                     ArgsListEndToken

        auto SwizzleToken = m_Tokens.insert(ArgsListEndToken, TokenInfo(TokenType::TextBlock, StubIt->second.Swizzle.c_str(), ""));
        SwizzleToken->Literal.push_back(static_cast<Char>('0' + pObjectInfo->NumComponents));
        // FunctionStub( TestTextArr[2], TestTextArr_sampler, ...    )_SWIZZLE4;
        //                                                                     ^
        //                                                            ArgsListEndToken
    }
    return true;
}

void HLSL2GLSLConverterImpl::ConversionStream::RemoveFlowControlAttribute(TokenListType::iterator& Token)
{
    VERIFY_EXPR(Token->IsFlowControl());
    // [ branch ] if ( ...
    //            ^
    auto PrevToken = Token;
    --PrevToken;
    // [ branch ] if ( ...
    //          ^
    // Note that dummy empty token is inserted into the beginning of the list
    if (PrevToken == m_Tokens.begin() || PrevToken->Type != TokenType::ClosingStaple)
        return;

    --PrevToken;
    // [ branch ] if ( ...
    //   ^
    if (PrevToken == m_Tokens.begin() || PrevToken->Type != TokenType::Identifier)
        return;

    --PrevToken;
    // [ branch ] if ( ...
    // ^
    if (PrevToken == m_Tokens.begin() || PrevToken->Type != TokenType::OpenStaple)
        return;

    //  [ branch ] if ( ...
    //  ^          ^
    // PrevToken   Token
    Token->Delimiter = PrevToken->Delimiter;
    m_Tokens.erase(PrevToken, Token);
}

void HLSL2GLSLConverterImpl::ConversionStream::RemoveSamplerRegister(TokenListType::iterator& Token)
{
    // SamplerState Tex2D_sampler;
    // ^
    VERIFY_EXPR(Token->Type == TokenType::kw_SamplerState || Token->Type == TokenType::kw_SamplerComparisonState);

    ++Token;
    // SamplerState Tex2D_sampler;
    //              ^

    bool DeclarationEnded = false;
    while (Token != m_Tokens.end() && Token->Type == TokenType::Identifier && !DeclarationEnded)
    {
        ++Token;
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while processing sampler declaration");

        // Skip to one of the following:
        //
        // SamplerState Tex2D_sampler;
        //                           ^
        // SamplerState Tex2D_sampler,
        //                           ^
        // SamplerState Tex2D_sampler:
        //                           ^
        while (Token != m_Tokens.end() && Token->Type != TokenType::Comma && Token->Type != TokenType::Semicolon && Token->Literal != ":")
            ++Token;
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while processing sampler declaration");

        if (Token->Literal == ":")
        {
            // SamplerState Tex2D_sampler : register(s0),
            //                            ^

            // Remove register
            while (Token != m_Tokens.end() && Token->Type != TokenType::Comma && Token->Type != TokenType::Semicolon)
            {
                auto CurrToken = Token;
                ++Token;
                m_Tokens.erase(CurrToken);
            }
            // SamplerState Tex2D_sampler ,
            //                            ^
        }
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while processing sampler declaration");
        VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Comma || Token->Type == TokenType::Semicolon,
                            "Unexpected symbol while processing sampler declaration: expected ',' or ';'");

        // Go to the next sampler declaration or next statement

        // SamplerState Tex2D_sampler ;
        //                            ^
        // SamplerState Tex2D_sampler ,
        //                            ^
        DeclarationEnded = Token->Type == TokenType::Semicolon;
        ++Token;
    }
}

// The function finds all HLSL object methods in the current scope and calls ProcessObjectMethod()
// that replaces them with the corresponding GLSL function stub.
void HLSL2GLSLConverterImpl::ConversionStream::ProcessObjectMethods(const TokenListType::iterator& ScopeStart,
                                                                    const TokenListType::iterator& ScopeEnd)
{
    auto Token = ScopeStart;
    while (Token != ScopeEnd)
    {
        // Search for .identifier pattern

        if (Token->Literal == ".")
        {
            auto DotToken = Token;
            ++Token;
            if (Token == ScopeEnd)
                break;
            if (Token->Type == TokenType::Identifier)
            {
                if (ProcessObjectMethod(DotToken, ScopeStart, ScopeEnd))
                    Token = DotToken;
            }
            else
            {
                ++Token;
                continue;
            }
        }
        else
            ++Token;
    }
}

// The function processes HLSL RW texture operator [] and replaces it with
// corresponding imageStore GLSL function.
// Example:
// RWTex[Location] = f3Value -> imageStore( RWTex,Location, _ExpandVector(f3Value))
// _ExpandVector() function expands any input vector to 4-component vector
bool HLSL2GLSLConverterImpl::ConversionStream::ProcessRWTextureStore(TokenListType::iterator&       Token,
                                                                     const TokenListType::iterator& ScopeEnd)
{
    // RWTex[Location.x] = float4(0.0, 0.0, 0.0, 1.0);
    // ^
    auto AssignmentToken = Token;
    while (AssignmentToken != ScopeEnd &&
           !(AssignmentToken->Type == TokenType::Assignment || AssignmentToken->Type == TokenType::Semicolon))
        ++AssignmentToken;

    // The function is called for ALL RW texture objects found, so this may not be
    // the store operation, but something else (for instance:
    // InterlockedExchange(Tex2D_I1[GTid.xy], 1, iOldVal) )
    if (AssignmentToken == ScopeEnd || AssignmentToken->Type != TokenType::Assignment)
        return false;
    // RWTex[Location.x] = float4(0.0, 0.0, 0.0, 1.0);
    //                   ^
    //            AssignmentToken
    auto ClosingStaplePos = AssignmentToken;
    while (ClosingStaplePos != Token && ClosingStaplePos->Type != TokenType::ClosingStaple)
        --ClosingStaplePos;
    if (ClosingStaplePos == Token)
        return false;
    // RWTex[Location.x] = float4(0.0, 0.0, 0.0, 1.0);
    //                 ^
    //          ClosingStaplePos

    auto OpenStaplePos = ClosingStaplePos;
    while (OpenStaplePos != Token && OpenStaplePos->Type != TokenType::OpenStaple)
        --OpenStaplePos;
    if (OpenStaplePos == Token)
        return false;
    // RWTex[Location.x] = float4(0.0, 0.0, 0.0, 1.0);
    //      ^
    //  OpenStaplePos

    auto SemicolonToken = AssignmentToken;
    while (SemicolonToken != ScopeEnd && SemicolonToken->Type != TokenType::Semicolon)
        ++SemicolonToken;
    if (SemicolonToken == ScopeEnd)
        return false;
    // RWTex[Location.x] = float4(0.0, 0.0, 0.0, 1.0);
    // ^                                             ^
    // Token                                    SemicolonToken

    m_Tokens.insert(Token, TokenInfo(TokenType::Identifier, "imageStore", Token->Delimiter.c_str()));
    m_Tokens.insert(Token, TokenInfo(TokenType::OpenBracket, "(", ""));
    Token->Delimiter = " ";
    // imageStore( RWTex[Location.x] = float4(0.0, 0.0, 0.0, 1.0);

    OpenStaplePos->Delimiter = "";
    OpenStaplePos->Type      = TokenType::Comma;
    OpenStaplePos->Literal   = ",";
    // imageStore( RWTex,Location.x] = float4(0.0, 0.0, 0.0, 1.0);
    //                             ^
    //                         ClosingStaplePos

    auto LocationToken = OpenStaplePos;
    ++LocationToken;
    m_Tokens.insert(LocationToken, TokenInfo(TokenType::Identifier, "_ToIvec", " "));
    m_Tokens.insert(LocationToken, TokenInfo(TokenType::OpenBracket, "(", ""));
    // imageStore( RWTex, _ToIvec(Location.x] = float4(0.0, 0.0, 0.0, 1.0);
    //                                      ^
    //                               ClosingStaplePos

    m_Tokens.insert(ClosingStaplePos, TokenInfo(TokenType::ClosingBracket, ")", ""));
    // imageStore( RWTex, _ToIvec(Location.x)] = float4(0.0, 0.0, 0.0, 1.0);
    //                                       ^
    //                                ClosingStaplePos

    ClosingStaplePos->Delimiter = "";
    ClosingStaplePos->Type      = TokenType::Comma;
    ClosingStaplePos->Literal   = ",";
    // imageStore( RWTex, _ToIvec(Location.x), = float4(0.0, 0.0, 0.0, 1.0);
    //                                         ^
    //                                   AssignmentToken

    AssignmentToken->Delimiter = "";
    AssignmentToken->Type      = TokenType::OpenBracket;
    AssignmentToken->Literal   = "(";
    // imageStore( RWTex, _ToIvec(Location.x),( float4(0.0, 0.0, 0.0, 1.0);
    //                                        ^

    m_Tokens.insert(AssignmentToken, TokenInfo(TokenType::Identifier, "_ExpandVector", " "));
    // imageStore( RWTex, _ToIvec(Location.x), _ExpandVector( float4(0.0, 0.0, 0.0, 1.0);
    //                                                      ^

    // Insert closing bracket for _ExpandVector
    m_Tokens.insert(SemicolonToken, TokenInfo(TokenType::ClosingBracket, ")", ""));
    // imageStore( RWTex,  _ToIvec(Location.x), _ExpandVector( float4(0.0, 0.0, 0.0, 1.0));

    // Insert closing bracket for imageStore
    m_Tokens.insert(SemicolonToken, TokenInfo(TokenType::ClosingBracket, ")", ""));
    // imageStore( RWTex,  _ToIvec(Location.x), _ExpandVector( float4(0.0, 0.0, 0.0, 1.0)));

    return false;
}

// Function finds all RW textures in current scope and calls ProcessRWTextureStore()
// that detects if this is store operation and converts it to imageStore()
void HLSL2GLSLConverterImpl::ConversionStream::ProcessRWTextures(const TokenListType::iterator& ScopeStart,
                                                                 const TokenListType::iterator& ScopeEnd)
{
    auto Token = ScopeStart;
    while (Token != ScopeEnd)
    {
        if (Token->Type == TokenType::Identifier)
        {
            // Try to find the object in all scopes
            const auto* pObjectInfo = FindHLSLObject(Token->Literal);
            if (pObjectInfo == nullptr)
            {
                ++Token;
                continue;
            }

            // Check if the object is image type
            auto ImgTypeIt = m_Converter.m_ImageTypes.find(pObjectInfo->GLSLType.c_str());
            if (ImgTypeIt == m_Converter.m_ImageTypes.end())
            {
                ++Token;
                continue;
            }

            // Handle store. If this is not store operation,
            // ProcessRWTextureStore() returns false.
            auto TmpToken = Token;
            if (ProcessRWTextureStore(TmpToken, ScopeEnd))
                Token = TmpToken;
            else
                ++Token;
        }
        else
            ++Token;
    }
}

// The function processes all atomic operations in current scope and replaces them with
// corresponding GLSL function
void HLSL2GLSLConverterImpl::ConversionStream::ProcessAtomics(const TokenListType::iterator& ScopeStart,
                                                              const TokenListType::iterator& ScopeEnd)
{
    auto Token = ScopeStart;
    while (Token != ScopeEnd)
    {
        if (Token->Type == TokenType::Identifier)
        {
            auto AtomicIt = m_Converter.m_AtomicOperations.find(Token->Literal.c_str());
            if (AtomicIt == m_Converter.m_AtomicOperations.end())
            {
                ++Token;
                continue;
            }

            auto OperationToken = Token;
            // InterlockedAdd(g_i4SharedArray[GTid.x].x, 1, iOldVal);
            // ^
            ++Token;
            // InterlockedAdd(g_i4SharedArray[GTid.x].x, 1, iOldVal);
            //               ^
            VERIFY_PARSER_STATE(Token, Token != ScopeEnd, "Unexpected EOF");
            VERIFY_PARSER_STATE(Token, Token->Type == TokenType::OpenBracket, "Open bracket is expected");

            auto ArgsListEndToken = Token;
            auto NumArguments     = CountFunctionArguments(ArgsListEndToken, ScopeEnd);
            // InterlockedAdd(Tex2D[GTid.xy], 1, iOldVal);
            //                                           ^
            //                                       ArgsListEndToken
            VERIFY_PARSER_STATE(ArgsListEndToken, ArgsListEndToken != ScopeEnd, "Unexpected EOF");

            ++Token;
            VERIFY_PARSER_STATE(Token, Token != ScopeEnd, "Unexpected EOF");

            const auto* pObjectInfo = FindHLSLObject(Token->Literal);
            if (pObjectInfo != nullptr)
            {
                // InterlockedAdd(Tex2D[GTid.xy], 1, iOldVal);
                //                ^
                auto StubIt = m_Converter.m_GLSLStubs.find(FunctionStubHashKey("image", OperationToken->Literal.c_str(), NumArguments));
                VERIFY_PARSER_STATE(OperationToken, StubIt != m_Converter.m_GLSLStubs.end(), "Unable to find function stub for funciton ", OperationToken->Literal, " with ", NumArguments, " arguments");

                // Find first comma
                int NumOpenBrackets = 1;
                while (Token != ScopeEnd && NumOpenBrackets != 0)
                {
                    // Do not count arguments of nested functions:
                    if (NumOpenBrackets == 1 && (Token->Type == TokenType::Comma || Token->Type == TokenType::ClosingBracket))
                        break;

                    if (Token->Type == TokenType::OpenBracket)
                        ++NumOpenBrackets;
                    else if (Token->Type == TokenType::ClosingBracket)
                        --NumOpenBrackets;

                    ++Token;
                }
                // InterlockedAdd(Tex2D[GTid.xy], 1, iOldVal);
                //                              ^
                VERIFY_PARSER_STATE(Token, Token != ScopeEnd, "Unexpected EOF");
                VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Comma, "Comma is expected");

                --Token;
                // InterlockedAdd(Tex2D[GTid.xy], 1, iOldVal);
                //                             ^
                VERIFY_PARSER_STATE(Token, Token->Type == TokenType::ClosingStaple, "Expected \']\'");
                auto ClosingBracketToken = Token;
                --Token;
                m_Tokens.erase(ClosingBracketToken);
                // InterlockedAdd(Tex2D[GTid.xy, 1, iOldVal);
                //                           ^
                while (Token != ScopeStart && Token->Type != TokenType::OpenStaple)
                    --Token;
                // InterlockedAdd(Tex2D[GTid.xy, 1, iOldVal);
                //                     ^

                VERIFY_PARSER_STATE(Token, Token != ScopeStart, "Expected \'[\'");
                Token->Type    = TokenType::Comma;
                Token->Literal = ",";
                // InterlockedAdd(Tex2D,GTid.xy, 1, iOldVal);
                //                     ^

                OperationToken->Literal = StubIt->second.Name;
                // InterlockedAddImage_3(Tex2D,GTid.xy, 1, iOldVal);
            }
            else
            {
                // InterlockedAdd(g_i4SharedArray[GTid.x].x, 1, iOldVal);
                //                ^
                auto StubIt = m_Converter.m_GLSLStubs.find(FunctionStubHashKey("shared_var", OperationToken->Literal.c_str(), NumArguments));
                VERIFY_PARSER_STATE(OperationToken, StubIt != m_Converter.m_GLSLStubs.end(), "Unable to find function stub for funciton ", OperationToken->Literal, " with ", NumArguments, " arguments");
                OperationToken->Literal = StubIt->second.Name;
                // InterlockedAddSharedVar_3(g_i4SharedArray[GTid.x].x, 1, iOldVal);
            }
            Token = ArgsListEndToken;
        }
        else
            ++Token;
    }
}


void HLSL2GLSLConverterImpl::ConversionStream::ParseShaderParameter(TokenListType::iterator& Token, ShaderParameterInfo& ParamInfo)
{
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while parsing argument list");
    VERIFY_PARSER_STATE(Token, Token->IsBuiltInType() || Token->Type == TokenType::Identifier,
                        "Missing argument type");
    auto TypeToken = Token;
    ParamInfo.Type = Token->Literal;

    ++Token;
    //          out float4 Color : SV_Target,
    //                     ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while parsing argument list");
    VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Identifier, "Missing argument name after ", ParamInfo.Type);
    ParamInfo.Name = Token->Literal;

    ++Token;
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF");

    if (Token->Type == TokenType::OpenStaple)
    {
        // triangle VSOut In[3]
        //                  ^
        ProcessScope(
            Token, m_Tokens.end(), TokenType::OpenStaple, TokenType::ClosingStaple,
            [&](TokenListType::iterator& tkn, int) {
                ParamInfo.ArraySize.append(tkn->Delimiter);
                ParamInfo.ArraySize.append(tkn->Literal);
                ++tkn;
            } //
        );
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF");
        // triangle VSOut In[3],
        //                    ^
        VERIFY_PARSER_STATE(Token, Token->Type == TokenType::ClosingStaple, "Closing staple expected");

        ++Token;
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF");
        VERIFY_PARSER_STATE(Token, Token->Type != TokenType::OpenStaple, "Multi-dimensional arrays are not supported");
    }


    if (TypeToken->IsBuiltInType())
    {
        //          out float4 Color : SV_Target,
        //                           ^
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected end of file after argument \"", ParamInfo.Name, '\"');
        if (Token->Literal == ":")
        {
            ++Token;
            //          out float4 Color : SV_Target,
            //                             ^
            VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected end of file while looking for semantic for argument \"", ParamInfo.Name, '\"');
            VERIFY_PARSER_STATE(Token, Token->Type == TokenType::Identifier, "Missing semantic for argument \"", ParamInfo.Name, '\"');
            // Transform to lower case -  semantics are case-insensitive
            ParamInfo.Semantic = StrToLower(Token->Literal);

            ++Token;
            //          out float4 Color : SV_Target,
            //                                      ^
        }
    }
    else
    {
        const auto& StructName = TypeToken->Literal;
        auto        it         = m_StructDefinitions.find(StructName.c_str());
        if (it == m_StructDefinitions.end())
            LOG_ERROR_AND_THROW("Unable to find definition for type \'", StructName, "\'");

        TypeToken = it->second;
        // struct VSOutput
        //        ^
        VERIFY_EXPR(TypeToken->Type == TokenType::Identifier && TypeToken->Literal == StructName);

        ++TypeToken;
        // struct VSOutput
        // {
        // ^
        VERIFY_PARSER_STATE(TypeToken, TypeToken != m_Tokens.end() && TypeToken->Type == TokenType::OpenBrace, "Open brace expected");

        ++TypeToken;
        // struct VSOutput
        // {
        //     float4 f4Position;
        //     ^
        while (TypeToken != m_Tokens.end() && TypeToken->Type != TokenType::ClosingBrace)
        {
            ShaderParameterInfo MemberInfo;
            MemberInfo.storageQualifier = ParamInfo.storageQualifier;
            ParseShaderParameter(TypeToken, MemberInfo);
            ParamInfo.members.push_back(MemberInfo);
            // struct VSOutput
            // {
            //     float4 f4Position;
            //                      ^
            VERIFY_PARSER_STATE(TypeToken, Token != m_Tokens.end() && TypeToken->Type == TokenType::Semicolon, "Semicolon expected");
            ++TypeToken;
        }
    }
}

// The function parses shader arguments and puts them into Params array
void HLSL2GLSLConverterImpl::ConversionStream::ProcessFunctionParameters(TokenListType::iterator&          Token,
                                                                         std::vector<ShaderParameterInfo>& Params,
                                                                         bool&                             bIsVoid)
{
    // void TestPS  ( in VSOutput In,
    // ^
    auto TypeToken = Token;

    ++Token;
    // void TestPS  ( in VSOutput In,
    //      ^
    auto FuncNameToken = Token;

    bIsVoid = TypeToken->Type == TokenType::kw_void;
    if (!bIsVoid)
    {
        ShaderParameterInfo RetParam;
        RetParam.Type             = TypeToken->Literal;
        RetParam.Name             = FuncNameToken->Literal;
        RetParam.storageQualifier = ShaderParameterInfo::StorageQualifier::Ret;
        Params.push_back(RetParam);
    }

    ++Token;
    // void TestPS  ( in VSOutput In,
    //              ^
    VERIFY_PARSER_STATE(Token, Token->Type == TokenType::OpenBracket, "Function \"", FuncNameToken->Literal, "\" misses argument list");

    ++Token;
    // void TestPS  ( in VSOutput In,
    //                ^
    auto ArgsListStartToken = Token;
    // Handle empty argument list properly
    // void TestPS  ( )
    //                ^
    if (Token != m_Tokens.end() && Token->Type != TokenType::ClosingBracket)
    {
        while (Token != m_Tokens.end())
        {
            ShaderParameterInfo ParamInfo;

            // Process in/out qualifier
            switch (Token->Type)
            {
                case TokenType::kw_in:
                    VERIFY_EXPR(Token->Literal == "in");
                    //void TestPS  ( in VSOutput In,
                    //               ^
                    ParamInfo.storageQualifier = ShaderParameterInfo::StorageQualifier::In;
                    ++Token;
                    //void TestPS  ( in VSOutput In,
                    //                  ^
                    break;

                case TokenType::kw_out:
                    VERIFY_EXPR(Token->Literal == "out");
                    //          out float4 Color : SV_Target,
                    //          ^
                    ParamInfo.storageQualifier = ShaderParameterInfo::StorageQualifier::Out;
                    ++Token;
                    //          out float4 Color : SV_Target,
                    //              ^
                    break;

                case TokenType::kw_inout:
                    VERIFY_EXPR(Token->Literal == "inout");
                    //          inout TriangleStream<GSOut> triStream
                    //          ^
                    ParamInfo.storageQualifier = ShaderParameterInfo::StorageQualifier::InOut;
                    ++Token;
                    //          inout TriangleStream<GSOut> triStream
                    //                ^
                    break;

                default:
                    ParamInfo.storageQualifier = ShaderParameterInfo::StorageQualifier::In;
                    break;
            }


            // Process different GS/HS/DS attributes
            switch (Token->Type)
            {
                case TokenType::kw_point:
                    // point QuadVSOut In[1]
                    // ^
                    ParamInfo.GSAttribs.PrimType = ShaderParameterInfo::GSAttributes::PrimitiveType::Point;
                    ++Token;
                    break;

                case TokenType::kw_line:
                    // line QuadVSOut In[2]
                    // ^
                    ParamInfo.GSAttribs.PrimType = ShaderParameterInfo::GSAttributes::PrimitiveType::Line;
                    ++Token;
                    break;

                case TokenType::kw_triangle:
                    // triangle QuadVSOut In[3]
                    // ^
                    ParamInfo.GSAttribs.PrimType = ShaderParameterInfo::GSAttributes::PrimitiveType::Triangle;
                    ++Token;
                    break;

                case TokenType::kw_lineadj:
                    // lineadj QuadVSOut In[4]
                    // ^
                    ParamInfo.GSAttribs.PrimType = ShaderParameterInfo::GSAttributes::PrimitiveType::LineAdj;
                    ++Token;
                    break;

                case TokenType::kw_triangleadj:
                    // triangleadj QuadVSOut In[6]
                    // ^
                    ParamInfo.GSAttribs.PrimType = ShaderParameterInfo::GSAttributes::PrimitiveType::TriangleAdj;
                    ++Token;

                case TokenType::kw_TriangleStream:
                case TokenType::kw_PointStream:
                case TokenType::kw_LineStream:
                    switch (Token->Type)
                    {
                        case TokenType::kw_TriangleStream:
                            // inout TriangleStream<GSOut> triStream
                            //       ^
                            ParamInfo.GSAttribs.Stream = ShaderParameterInfo::GSAttributes::StreamType::Triangle;
                            break;

                        case TokenType::kw_PointStream:
                            // inout PointStream<GSOut> ptStream
                            //       ^
                            ParamInfo.GSAttribs.Stream = ShaderParameterInfo::GSAttributes::StreamType::Point;
                            break;

                        case TokenType::kw_LineStream:
                            // inout LineStream<GSOut> lnStream
                            //       ^
                            ParamInfo.GSAttribs.Stream = ShaderParameterInfo::GSAttributes::StreamType::Line;
                            break;

                        default:
                            UNEXPECTED("Unexpected keyword ");
                    }

                    {
                        ++Token;
                        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && Token->Literal == "<", "Angle bracket expected");
                        // inout LineStream<GSOut> lnStream
                        //                 ^
                        auto OpenAngleBarcket = Token++;
                        m_Tokens.erase(OpenAngleBarcket);
                        // inout LineStream GSOut> lnStream
                        //                  ^

                        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF");

                        auto ClosingAngleBarcket = Token;
                        ++ClosingAngleBarcket;
                        VERIFY_PARSER_STATE(ClosingAngleBarcket, ClosingAngleBarcket != m_Tokens.end() && ClosingAngleBarcket->Literal == ">", "Angle bracket expected");
                        m_Tokens.erase(ClosingAngleBarcket);
                        // inout LineStream GSOut lnStream
                        //                  ^
                    }
                    break;

                case TokenType::kw_OutputPatch:
                case TokenType::kw_InputPatch:
                {
                    ParamInfo.HSAttribs.PatchType = Token->Type == TokenType::kw_InputPatch ? ShaderParameterInfo::HSAttributes::InOutPatchType::InputPatch : ShaderParameterInfo::HSAttributes::InOutPatchType::OutputPatch;
                    // HSOutput main(InputPatch<VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
                    //               ^
                    ++Token;
                    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && Token->Literal == "<", "Angle bracket expected");
                    // HSOutput main(InputPatch<VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
                    //                         ^
                    auto OpenAngleBarcket = Token++;
                    m_Tokens.erase(OpenAngleBarcket);
                    // HSOutput main(InputPatch VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
                    //                          ^

                    auto TmpToken = Token;
                    ++TmpToken;
                    // HSOutput main(InputPatch VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
                    //                                  ^
                    VERIFY_PARSER_STATE(TmpToken, TmpToken != m_Tokens.end() && TmpToken->Type == TokenType::Comma, "Comma expected");
                    auto CommaToken = TmpToken;
                    ++TmpToken;
                    m_Tokens.erase(CommaToken);
                    // HSOutput main(InputPatch VSOutput 1> inputPatch, uint uCPID : SV_OutputControlPointID)
                    //                                   ^
                    VERIFY_PARSER_STATE(TmpToken, TmpToken != m_Tokens.end() && TmpToken->Type == TokenType::NumericConstant, "Numeric constant expected");

                    ParamInfo.ArraySize     = TmpToken->Literal;
                    auto NumCtrlPointsToken = TmpToken;
                    ++TmpToken;
                    VERIFY_PARSER_STATE(TmpToken, TmpToken != m_Tokens.end() && TmpToken->Literal == ">", "Angle bracket expected");
                    m_Tokens.erase(NumCtrlPointsToken);
                    // HSOutput main(InputPatch VSOutput > inputPatch, uint uCPID : SV_OutputControlPointID)
                    //                                   ^

                    m_Tokens.erase(TmpToken);
                    // HSOutput main(InputPatch VSOutput inputPatch, uint uCPID : SV_OutputControlPointID)
                    //
                }
                break;

                default: /*do nothing*/ break;
            }

            ParseShaderParameter(Token, ParamInfo);

            VERIFY_PARSER_STATE(Token, Token->Literal == "," || Token->Type == TokenType::ClosingBracket, "\',\' or \')\' is expected after argument \"", ParamInfo.Name, '\"');
            Params.push_back(ParamInfo);
            if (Token->Type == TokenType::ClosingBracket)
                break;
            ++Token;
        }
    }
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF");

    if (!bIsVoid)
    {
        // float4 TestPS  ( in VSOutput In ) : SV_Target
        //                                 ^
        auto ColonToken = Token;
        ++ColonToken;
        VERIFY_PARSER_STATE(ColonToken, ColonToken != m_Tokens.end(), "Unexpected EOF");
        auto& RetParam = Params[0];
        VERIFY_EXPR(RetParam.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret);
        if (ColonToken->Literal == ":")
        {
            // float4 TestPS  ( in VSOutput In ) : SV_Target
            //                                   ^
            //                               ColonToken
            auto SemanticToken = ColonToken;
            ++SemanticToken;
            // float4 TestPS  ( in VSOutput In ) : SV_Target
            //                                     ^
            //                                SemanticToken
            VERIFY_PARSER_STATE(SemanticToken, SemanticToken != m_Tokens.end(), "Unexpected EOF");
            VERIFY_PARSER_STATE(SemanticToken, SemanticToken->Type == TokenType::Identifier, "Exepcted semantic for the return argument ");
            // Transform to lower case -  semantics are case-insensitive
            RetParam.Semantic = StrToLower(SemanticToken->Literal);
            ++SemanticToken;
            // float4 TestPS  ( in VSOutput In ) : SV_Target
            // {
            // ^
            // SemanticToken
            m_Tokens.erase(ColonToken, SemanticToken);
            // float4 TestPS  ( in VSOutput In )
        }
        else
        {
            // VSOut TestVS  ()
            auto TmpTypeToken = TypeToken;
            ParseShaderParameter(TmpTypeToken, RetParam);
        }
        TypeToken->Type    = TokenType::Identifier;
        TypeToken->Literal = "void";
        // void TestPS  ( in VSOutput In )
    }

    //           ArgsListStartToken
    //               V
    //void TestPS  ( in VSOutput In,
    //               out float4 Color : SV_Target,
    //               out float3 Color2 : SV_Target2 )
    //                                              ^
    //                                            Token

    m_Tokens.erase(ArgsListStartToken, Token);
    //void TestPS  ()
}

void InitVariable(const String& Name, const String& InitValue, std::stringstream& OutSS)
{
    OutSS << "    " << Name << " = " << InitValue << ";\n";
}

void DefineInterfaceVar(int location, const char* inout, const String& ParamType, const String& ParamName, std::stringstream& OutSS)
{
    if (location >= 0)
    {
        OutSS << "layout(location = " << location << ") ";
    }
    OutSS << inout << ' ' << ParamType << ' ' << ParamName << ";\n";
}

String HLSL2GLSLConverterImpl::ConversionStream::BuildParameterName(const std::vector<const ShaderParameterInfo*>& MemberStack, Char Separator, const Char* Prefix, const Char* SubstituteInstName, const Char* Index)
{
    String FullName(Prefix);
    auto   it = MemberStack.begin();
    FullName.append(*SubstituteInstName != 0 ? SubstituteInstName : (*it)->Name.c_str());
    FullName.append(Index);
    ++it;
    for (; it != MemberStack.end(); ++it)
    {
        FullName.push_back(Separator);
        FullName.append((*it)->Name);
    }
    return FullName;
}

template <typename TArgHandler>
void HLSL2GLSLConverterImpl::ConversionStream::ProcessShaderArgument(const ShaderParameterInfo& RootParam,
                                                                     int                        ShaderInd,
                                                                     int                        IsOutVar,
                                                                     stringstream&              PrologueSS,
                                                                     TArgHandler                ArgHandler)
{
    std::vector<const ShaderParameterInfo*>                       MemberStack;
    std::vector<std::vector<ShaderParameterInfo>::const_iterator> MemberItStack;
    MemberStack.push_back(&RootParam);
    MemberItStack.push_back(RootParam.members.cbegin());

    // Declare variable
    if (RootParam.storageQualifier != ShaderParameterInfo::StorageQualifier::Ret &&
        RootParam.GSAttribs.PrimType == ShaderParameterInfo::GSAttributes::PrimitiveType::Undefined &&
        RootParam.HSAttribs.PatchType == ShaderParameterInfo::HSAttributes::InOutPatchType::Undefined)
    {
        PrologueSS << "    " << RootParam.Type.c_str() << ' ' << RootParam.Name.c_str();
        if (!RootParam.ArraySize.empty())
            PrologueSS << '[' << RootParam.ArraySize << ']';
        PrologueSS << ";\n";
    }


    while (!MemberStack.empty())
    {
        const auto& CurrParam = *MemberStack.back();
        if (CurrParam.members.empty())
        {
            VERIFY_EXPR(MemberItStack.back() == CurrParam.members.cend());

            if (CurrParam.Semantic.empty())
                LOG_ERROR_AND_THROW("No semantic assigned to parameter \"", CurrParam.Name, "\"");

            String GLSLVariable;
            if (ShaderInd >= 0)
            {
                auto& SemanticToVarMap = m_Converter.m_HLSLSemanticToGLSLVar[ShaderInd][IsOutVar];
                auto  VarIt            = SemanticToVarMap.find(CurrParam.Semantic.c_str());
                if (VarIt != SemanticToVarMap.end())
                    GLSLVariable = VarIt->second;
            }

            ArgHandler(MemberStack, CurrParam, move(GLSLVariable));
            MemberStack.pop_back();
            MemberItStack.pop_back();
        }
        else
        {
            if (!CurrParam.Semantic.empty())
                LOG_ERROR_AND_THROW("Semantic assigned to a structure \"", CurrParam.Name, "\"");
            auto& NextMemberIt = MemberItStack.back();
            if (NextMemberIt == CurrParam.members.cend())
            {
                MemberStack.pop_back();
                MemberItStack.pop_back();
            }
            else
            {
                const ShaderParameterInfo& NextMember = *NextMemberIt;
                ++NextMemberIt;
                MemberStack.push_back(&NextMember);
                MemberItStack.push_back(NextMember.members.cbegin());
            }
        }
    }
}

bool HLSL2GLSLConverterImpl::ConversionStream::RequiresFlatQualifier(const String& Type)
{
    auto keywordIt    = m_Converter.m_HLSLKeywords.find(Type.c_str());
    bool RequiresFlat = false;
    if (keywordIt != m_Converter.m_HLSLKeywords.end())
    {
        VERIFY_EXPR(keywordIt->second.Literal == Type);
        auto kw      = keywordIt->second.Type;
        RequiresFlat = (kw >= TokenType::kw_int && kw <= TokenType::kw_int4x4) ||
            (kw >= TokenType::kw_uint && kw <= TokenType::kw_uint4x4) ||
            (kw >= TokenType::kw_min16int && kw <= TokenType::kw_min16int4x4) ||
            (kw >= TokenType::kw_min12int && kw <= TokenType::kw_min12int4x4) ||
            (kw >= TokenType::kw_min16uint && kw <= TokenType::kw_min16uint4x4);
    }
    return RequiresFlat;
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessFragmentShaderArguments(std::vector<ShaderParameterInfo>& Params,
                                                                              String&                           GlobalVariables,
                                                                              std::stringstream&                ReturnHandlerSS,
                                                                              String&                           Prologue)
{
    stringstream GlobalVarsSS, PrologueSS, InterfaceVarsSS;
    int          InLocation = 0;
    for (const auto& Param : Params)
    {
        if (Param.storageQualifier == ShaderParameterInfo::StorageQualifier::In)
        {
            ProcessShaderArgument(
                Param, PSInd, InVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Getter) //
                {
                    String FullParamName = BuildParameterName(MemberStack, '.');
                    if (!Getter.empty())
                        PrologueSS << "    " << Getter << '(' << FullParamName << ");\n";
                    else
                    {
                        auto InputVarName = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_psin_" : "_");
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? InLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat in" : "in",
                                           Param.Type, InputVarName, InterfaceVarsSS);
                        InitVariable(FullParamName, InputVarName, PrologueSS);
                    }
                } //
            );
        }
        else if (Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Out ||
                 Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret)
        {
            ProcessShaderArgument(
                Param, PSInd, OutVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Setter) //
                {
                    String FullParamName = BuildParameterName(MemberStack, '.', "", Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret ? "_RET_VAL_" : "");
                    if (!Setter.empty())
                        ReturnHandlerSS << Setter << '(' << FullParamName << ");\\\n";
                    else
                    {
                        const auto& Semantic   = Param.Semantic;
                        auto        RTIndexPos = Semantic.begin();
                        int         RTIndex    = -1;
                        if (SkipPrefix("sv_target", RTIndexPos, Semantic.end()))
                        {
                            if (RTIndexPos != Semantic.end())
                            {
                                if (*RTIndexPos >= '0' && *RTIndexPos <= '9')
                                {
                                    RTIndex = *RTIndexPos - '0';
                                    if ((RTIndexPos + 1) != Semantic.end())
                                        RTIndex = -1;
                                }
                            }
                            else
                                RTIndex = 0;
                        }

                        if (RTIndex >= 0 && RTIndex < MAX_RENDER_TARGETS)
                        {
                            // Layout location qualifiers are allowed on FS outputs even in GLES3.0
                            String OutVarName = BuildParameterName(MemberStack, '_', "_psout_");
                            DefineInterfaceVar(RTIndex, "out", Param.Type, OutVarName, GlobalVarsSS);
                            ReturnHandlerSS << OutVarName << " = " << FullParamName << ";\\\n";
                        }
                        else
                        {
                            LOG_ERROR_AND_THROW("Unexpected output semantic \"", Semantic, "\". The only allowed output semantic for fragment shader is SV_Target*");
                        }
                    }
                } //
            );
        }
    }

    GlobalVarsSS << InterfaceVarsSS.str();
    GlobalVariables = GlobalVarsSS.str();
    Prologue        = PrologueSS.str();
}


void HLSL2GLSLConverterImpl::ConversionStream::ProcessVertexShaderArguments(std::vector<ShaderParameterInfo>& Params,
                                                                            String&                           GlobalVariables,
                                                                            std::stringstream&                ReturnHandlerSS,
                                                                            String&                           Prologue)
{
    stringstream GlobalVarsSS, PrologueSS, InterfaceVarsSS;
    int          OutLocation       = 0;
    int          AutoInputLocation = 0; // Automatically assigned input location

    std::unordered_map<int, const Char*> LocationToSemantic;
    for (const auto& Param : Params)
    {
        if (Param.storageQualifier == ShaderParameterInfo::StorageQualifier::In)
        {
            ProcessShaderArgument(
                Param, VSInd, InVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Getter) //
                {
                    String FullParamName = BuildParameterName(MemberStack, '.');
                    if (!Getter.empty())
                        PrologueSS << "    " << Getter << '(' << FullParamName << ");\n";
                    else
                    {
                        int         InputLocation  = AutoInputLocation;
                        const auto& Semantic       = Param.Semantic;
                        auto        SemanticEndPos = Semantic.begin();
                        if (SkipPrefix("attrib", SemanticEndPos, Semantic.end()))
                        {
                            char* EndPtr    = nullptr;
                            auto  AttribInd = static_cast<int>(strtol(&*SemanticEndPos, &EndPtr, 10));
                            if (EndPtr != nullptr && *EndPtr == 0)
                            {
                                InputLocation     = AttribInd;
                                AutoInputLocation = InputLocation;
                            }
                        }
                        auto it = LocationToSemantic.find(InputLocation);
                        if (it != LocationToSemantic.end())
                        {
                            LOG_ERROR_AND_THROW("Location ", InputLocation, " assigned to semantic \"", Semantic, "\" conflicts with previous semantic \"", it->second, "\". Please use ATTRIB* semantic to explicitly specify attribute index");
                        }
                        LocationToSemantic[InputLocation] = Semantic.c_str();
                        auto InputVarName                 = BuildParameterName(MemberStack, '_', "_vsin_");
                        // Layout location qualifiers are allowed on VS inputs even in GLES3.0
                        DefineInterfaceVar(InputLocation, "in", Param.Type, InputVarName, GlobalVarsSS);
                        InitVariable(FullParamName, InputVarName, PrologueSS);
                        AutoInputLocation++;
                    }
                } //
            );
        }
        else if (Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Out ||
                 Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret)
        {
            ProcessShaderArgument(
                Param, VSInd, OutVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Setter) //
                {
                    String FullParamName = BuildParameterName(MemberStack, '.', "", Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret ? "_RET_VAL_" : "");
                    if (!Setter.empty())
                        ReturnHandlerSS << Setter << '(' << FullParamName << ");\\\n";
                    else
                    {
                        auto OutputVarName = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_vsout_" : "_");
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? OutLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat out" : "out",
                                           Param.Type, OutputVarName, InterfaceVarsSS);
                        ReturnHandlerSS << OutputVarName << " = " << FullParamName << ";\\\n";
                    }
                } //
            );
        }
    }

    GlobalVarsSS << InterfaceVarsSS.str();
    GlobalVariables = GlobalVarsSS.str();
    Prologue        = PrologueSS.str();
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessGeometryShaderArguments(TokenListType::iterator&          TypeToken,
                                                                              std::vector<ShaderParameterInfo>& Params,
                                                                              String&                           GlobalVariables,
                                                                              String&                           Prologue)
{
    auto Token = TypeToken;
    // [maxvertexcount(3)]
    // void SelectArraySliceGS(triangle QuadVSOut In[3],
    // ^

    std::unordered_map<HashMapStringKey, String, HashMapStringKey::Hasher> Attributes;
    ProcessShaderAttributes(Token, Attributes);
    auto MaxVertexCountIt = Attributes.find("maxvertexcount");
    if (MaxVertexCountIt == Attributes.end())
        LOG_ERROR_AND_THROW("Geomtry shader \"", Token->Literal, "\" misses \"maxvertexcount\" attribute");
    const Char* MaxVertexCount = MaxVertexCountIt->second.c_str();

    stringstream GlobalVarsSS, PrologueSS, InterfaceVarsInSS, InterfaceVarsOutSS, EmitVertexDefineSS;

    int inLocation = 0, outLocation = 0;
    for (const auto& TopLevelParam : Params)
    {
        // Geometry shader has only one input and one output
        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb509609(v=vs.85).aspx
        // [maxvertexcount(NumVerts)]
        //  void ShaderName ( PrimitiveType DataType Name [ NumElements ],
        //                    inout StreamOutputObject );

        if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::In)
        {
            if (TopLevelParam.GSAttribs.PrimType == ShaderParameterInfo::GSAttributes::PrimitiveType::Undefined)
                LOG_ERROR_AND_THROW("Geometry shader input misses primitive type");

            const Char* GLLayout = nullptr;
            switch (TopLevelParam.GSAttribs.PrimType)
            {
                // clang-format off
                case ShaderParameterInfo::GSAttributes::PrimitiveType::Point:        GLLayout = "points";               break;
                case ShaderParameterInfo::GSAttributes::PrimitiveType::Line:         GLLayout = "lines";                break;
                case ShaderParameterInfo::GSAttributes::PrimitiveType::Triangle:     GLLayout = "triangles";            break;
                case ShaderParameterInfo::GSAttributes::PrimitiveType::LineAdj:      GLLayout = "lines_adjacency";      break;
                case ShaderParameterInfo::GSAttributes::PrimitiveType::TriangleAdj:  GLLayout = "triangles_adjacency";  break;
                default: LOG_ERROR_AND_THROW("Unexpected GS input primitive type");
                // clang-format om
            }
            GlobalVarsSS << "layout (" << GLLayout << ") in;\n";
            PrologueSS << "    const int _NumElements = " << TopLevelParam.ArraySize << ";\n";
            PrologueSS << "    " << TopLevelParam.Type << ' ' << TopLevelParam.Name << "[_NumElements];\n";
            PrologueSS << "    for(int i=0; i < _NumElements; ++i)\n    {\n";

            ProcessShaderArgument(
                TopLevelParam, GSInd, InVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Getter) //
                {
                    String FullIndexedParamName = BuildParameterName(MemberStack, '.', "", "", "[i]");
                    PrologueSS << "    ";
                    if (!Getter.empty())
                        PrologueSS << "    " << Getter << '(' << FullIndexedParamName << ");\n";
                    else
                    {
                        auto VarName      = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_gsin_" : "_");
                        auto InputVarName = VarName + "[i]";
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? inLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat in" : "in",
                                           Param.Type, VarName + "[]", InterfaceVarsInSS);
                        InitVariable(FullIndexedParamName, InputVarName, PrologueSS);
                    }
                } //
            );

            PrologueSS << "    }\n";
        }
        else if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::InOut)
        {
            if (TopLevelParam.GSAttribs.Stream == ShaderParameterInfo::GSAttributes::StreamType::Undefined)
                LOG_ERROR_AND_THROW("Geometry shader output misses stream type");

            const Char* GLStreamType = nullptr;
            switch (TopLevelParam.GSAttribs.Stream)
            {
                // clang-format off
                case ShaderParameterInfo::GSAttributes::StreamType::Point:    GLStreamType = "points";         break;
                case ShaderParameterInfo::GSAttributes::StreamType::Line:     GLStreamType = "line_strip";     break;
                case ShaderParameterInfo::GSAttributes::StreamType::Triangle: GLStreamType = "triangle_strip"; break;
                default: LOG_ERROR_AND_THROW("Unexpected GS output stream type");
                    // clang-format on
            }

            GlobalVarsSS << "layout (" << GLStreamType << ", max_vertices = " << MaxVertexCount << ") out;\n";

            EmitVertexDefineSS << "#define " << TopLevelParam.Name << "_Append(VERTEX){\\\n";

            ProcessShaderArgument(
                TopLevelParam, GSInd, OutVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Setter) //
                {
                    String MacroArgumentName = BuildParameterName(MemberStack, '.', "", "VERTEX");
                    if (!Setter.empty())
                        EmitVertexDefineSS << Setter << '(' << MacroArgumentName << ");\\\n";
                    if (Setter.empty() || Setter == "_SET_GL_LAYER")
                    {
                        // For SV_RenderTargetArrayIndex semantic, we also need to define output
                        // variable that fragment shader will read.
                        // Note that gl_Layer is available in fragment shader, but only starting with GL4.3+
                        auto OutputVarName = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_gsout_" : "_");
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? outLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat out" : "out",
                                           Param.Type, OutputVarName, InterfaceVarsOutSS);
                        EmitVertexDefineSS << OutputVarName << " = " << MacroArgumentName << ";\\\n";
                    }
                } //
            );

            EmitVertexDefineSS << "EmitVertex();}\n\n";
            EmitVertexDefineSS << "#define " << TopLevelParam.Name << "_RestartStrip EndPrimitive\n";
        }
    }

    GlobalVariables = GlobalVarsSS.str() + InterfaceVarsInSS.str() + InterfaceVarsOutSS.str() + EmitVertexDefineSS.str();
    Prologue        = PrologueSS.str();
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessComputeShaderArguments(TokenListType::iterator&          TypeToken,
                                                                             std::vector<ShaderParameterInfo>& Params,
                                                                             String&                           GlobalVariables,
                                                                             String&                           Prologue)
{
    stringstream GlobalVarsSS, PrologueSS;

    auto Token = TypeToken;
    //[numthreads(16,16,1)]
    //void TestCS(uint3 DTid : SV_DispatchThreadID)
    //^
    --Token;
    //[numthreads(16,16,1)]
    //                    ^
    //void TestCS(uint3 DTid : SV_DispatchThreadID)
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.begin() && Token->Type == TokenType::ClosingStaple, "Missing numthreads declaration");

    while (Token != m_Tokens.begin() && Token->Type != TokenType::OpenStaple)
        --Token;
    //[numthreads(16,16,1)]
    //^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.begin(), "Missing numthreads() declaration");
    auto OpenStapleToken = Token;

    ++Token;
    //[numthreads(16,16,1)]
    // ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && Token->Type == TokenType::Identifier && Token->Literal == "numthreads",
                        "Missing numthreads() declaration");

    ++Token;
    //[numthreads(16,16,1)]
    //           ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && Token->Type == TokenType::OpenBracket,
                        "Missing \'(\' after numthreads");

    String             CSGroupSize[3] = {};
    static const Char* DirNames[]     = {"X", "Y", "Z"};
    for (int i = 0; i < 3; ++i)
    {
        ++Token;
        //[numthreads(16,16,1)]
        //            ^
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && (Token->Type == TokenType::NumericConstant || Token->Type == TokenType::Identifier),
                            "Missing group size for ", DirNames[i], " direction");
        CSGroupSize[i] = Token->Literal.c_str();
        ++Token;
        //[numthreads(16,16,1)]
        //              ^    ^
        const Char* ExpectedLiteral = (i < 2) ? "," : ")";
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end() && Token->Literal == ExpectedLiteral,
                            "Missing \'", ExpectedLiteral, "\' after ", DirNames[i], " direction");
    }

    //OpenStapleToken
    //V
    //[numthreads(16,16,1)]
    //void TestCS(uint3 DTid : SV_DispatchThreadID)
    //^
    //TypeToken
    TypeToken->Delimiter = OpenStapleToken->Delimiter;
    m_Tokens.erase(OpenStapleToken, TypeToken);
    //
    // void TestCS(uint3 DTid : SV_DispatchThreadID)

    GlobalVarsSS << "layout ( local_size_x = " << CSGroupSize[0]
                 << ", local_size_y = " << CSGroupSize[1] << ", local_size_z = " << CSGroupSize[2] << " ) in;\n";

    for (const auto& Param : Params)
    {
        if (Param.storageQualifier == ShaderParameterInfo::StorageQualifier::In)
        {
            ProcessShaderArgument(
                Param, CSInd, InVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Getter) //
                {
                    String FullParamName = BuildParameterName(MemberStack, '.');
                    if (Getter.empty())
                    {
                        LOG_ERROR_AND_THROW("Unexpected input semantic \"", Param.Semantic, "\". The only allowed semantics for the compute shader inputs are \"ATTRIB*\", \"SV_VertexID\", and \"SV_InstanceID\".");
                    }
                    PrologueSS << "    " << Getter << '(' << Param.Type << "," << FullParamName << ");\n";
                } //
            );
        }
        else if (Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Out)
        {
            LOG_ERROR_AND_THROW("Output variables are not allowed in compute shaders");
        }
    }

    GlobalVariables = GlobalVarsSS.str();
    Prologue        = PrologueSS.str();
}

template <typename THandler>
void HLSL2GLSLConverterImpl::ConversionStream::ProcessScope(TokenListType::iterator& Token, TokenListType::iterator ScopeEnd, TokenType OpenParenType, TokenType ClosingParenType, THandler Handler)
{
    // The function can handle both global scope as well as local scope
    int StartScopeDepth = 0;
    if (Token->Type == OpenParenType)
    {
        // void TestPS()
        //            ^
        StartScopeDepth = 1;
        ++Token;
    }
    int ScopeDepth = StartScopeDepth;
    while (Token != ScopeEnd)
    {
        if (Token->Type == OpenParenType)
            ++ScopeDepth;
        else if (Token->Type == ClosingParenType)
        {
            --ScopeDepth;
            if (ScopeDepth < StartScopeDepth)
                break;
        }

        Handler(Token, ScopeDepth);
    }
    if (StartScopeDepth == 1)
    {
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while processing scope");
        VERIFY_EXPR(Token->Type == ClosingParenType);
    }
    else
    {
        VERIFY_PARSER_STATE(Token, ScopeDepth == 0, "Unbalanced brackets");
    }
}


void HLSL2GLSLConverterImpl::ConversionStream::ProcessHullShaderConstantFunction(const Char* FuncName, bool& bTakesInputPatch)
{
    // Search for the function in the global scope
    auto EntryPointToken = m_Tokens.end();
    auto Token           = m_Tokens.begin();
    ProcessScope(
        Token, m_Tokens.end(), TokenType::OpenBrace, TokenType::ClosingBrace,
        [&](TokenListType::iterator& tkn, int ScopeDepth) //
        {
            if (ScopeDepth == 0 && tkn->Type == TokenType::Identifier && tkn->Literal == FuncName)
            {
                EntryPointToken = tkn;
                tkn             = m_Tokens.end();
            }
            else
                ++tkn;
        } //
    );
    VERIFY_PARSER_STATE(EntryPointToken, EntryPointToken != m_Tokens.end(), "Unable to find hull shader constant function \"", FuncName, '\"');
    const auto* EntryPoint = EntryPointToken->Literal.c_str();

    auto TypeToken = EntryPointToken;
    --TypeToken;
    // void ConstantHS( InputPatch<VSOutput, 1> p,
    // ^
    // TypeToken
    VERIFY_PARSER_STATE(TypeToken, TypeToken != m_Tokens.begin(), "Function \"", EntryPointToken->Literal, "\" misses return type");

    std::vector<ShaderParameterInfo> Params;

    auto ArgsListEndToken = TypeToken;
    bool bIsVoid          = false;
    ProcessFunctionParameters(ArgsListEndToken, Params, bIsVoid);
    // HS_CONSTANT_DATA_OUTPUT ConstantHS( InputPatch<VSOutput, 1> p,
    //                                     uint BlockID : SV_PrimitiveID)
    //                                                                  ^
    //                                                       ArgsListEndToken

    std::stringstream PrologueSS, ReturnHandlerSS;
    const Char*       ReturnMacroName = "_CONST_FUNC_RETURN_";
    // Some GLES compilers cannot properly handle macros with empty argument lists, such as _CONST_FUNC_RETURN_().
    // Also, some compilers generate an error if there is no whitespace after the macro without arguments: _CONST_FUNC_RETURN_{
    ReturnHandlerSS << "#define " << ReturnMacroName << (bIsVoid ? "" : "(_RET_VAL_)") << " {\\\n";

    bTakesInputPatch = false;
    for (const auto& TopLevelParam : Params)
    {
        if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::In)
        {
            if (TopLevelParam.HSAttribs.PatchType == ShaderParameterInfo::HSAttributes::InOutPatchType::InputPatch)
            {
                bTakesInputPatch = true;
                String Argument(TopLevelParam.Type);
                Argument.push_back(' ');
                Argument.append(TopLevelParam.Name);
                Argument.push_back('[');
                Argument.append(TopLevelParam.ArraySize);
                Argument.push_back(']');
                m_Tokens.insert(ArgsListEndToken, TokenInfo(TokenType::TextBlock, Argument.c_str()));
            }
            else
            {
                ProcessShaderArgument(
                    TopLevelParam, HSInd, InVar, PrologueSS,
                    [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Getter) //
                    {
                        String FullIndexedParamName = BuildParameterName(MemberStack, '.');
                        if (Getter.empty())
                        {
                            LOG_ERROR_AND_THROW("Supported inputs to a hull shader constant function are \"InputPatch<>\" and variables with SV_ semantic.\n"
                                                "Variable \"",
                                                Param.Name, "\" with semantic \"", Param.Semantic, "\" is not supported");
                        }
                        PrologueSS << "    " << Getter << '(' << FullIndexedParamName << ");\n";
                    } //
                );
            }
        }
        else if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::Out ||
                 TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret)
        {
            ProcessShaderArgument(
                TopLevelParam, HSInd, OutVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Setter) //
                {
                    String SrcParamName = BuildParameterName(MemberStack, '.', "", Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret ? "_RET_VAL_" : "");
                    if (Setter.empty())
                    {
                        LOG_ERROR_AND_THROW("Supported output semantics of a hull shader constant function are \"SV_TessFactor\" and \"SV_InsideTessFactor\".\n"
                                            "Variable \"",
                                            Param.Name, "\" with semantic \"", Param.Semantic, "\" is not supported");
                    }

                    // A TCS can only ever write to the per-vertex output variable that corresponds to their invocation,
                    // so writes to per-vertex outputs must be of the form vertexTexCoord[gl_InvocationID]
                    ReturnHandlerSS << Setter << '(' << SrcParamName << ");\\\n";
                } //
            );
        }
    }
    ReturnHandlerSS << "return;}\n";
    m_Tokens.insert(TypeToken, TokenInfo(TokenType::TextBlock, ReturnHandlerSS.str().c_str(), TypeToken->Delimiter.c_str()));
    TypeToken->Delimiter = "\n";

    String Prologue = PrologueSS.str();
    Token           = ArgsListEndToken;
    ++Token;
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected end of file while looking for the body of \"", EntryPoint, "\".");
    VERIFY_PARSER_STATE(Token, Token->Type == TokenType::OpenBrace, "\'{\' expected");

    auto FirstStatementToken = Token;
    ++FirstStatementToken;
    // void main ()
    // {
    //      int a;
    //      ^
    VERIFY_PARSER_STATE(FirstStatementToken, FirstStatementToken != m_Tokens.end(), "Unexpected end of file while looking for the body of \"", EntryPoint, "\".");

    // Insert prologue before the first token
    m_Tokens.insert(FirstStatementToken, TokenInfo(TokenType::TextBlock, Prologue.c_str(), "\n"));

    ProcessReturnStatements(Token, bIsVoid, EntryPoint, ReturnMacroName);
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessShaderAttributes(TokenListType::iterator&                                                Token,
                                                                       std::unordered_map<HashMapStringKey, String, HashMapStringKey::Hasher>& Attributes)
{
    VERIFY_EXPR(Token->IsBuiltInType() || Token->Type == TokenType::Identifier);
    // [patchconstantfunc("ConstantHS")]
    // [maxtessfactor( (float)(32.f+2.f) )]
    // HSOutput main(InputPatch<VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
    // ^
    auto TypeToken = Token;
    --Token;
    while (Token->Type == TokenType::ClosingStaple)
    {
        // [...]
        //     ^

        while (Token != m_Tokens.begin() && Token->Type != TokenType::OpenStaple)
            --Token;
        // [...]
        // ^
        VERIFY_PARSER_STATE(Token, Token != m_Tokens.begin(), "Unable to find matching \'[\'");
        auto OpenStapleToken = Token;

        auto TmpToken = Token;
        ++TmpToken;
        VERIFY_PARSER_STATE(TmpToken, TmpToken != m_Tokens.end() && TmpToken->Type == TokenType::Identifier, "Identifier expected");
        // [domain("quad")]
        //  ^
        auto& Attrib = TmpToken->Literal;
        StrToLowerInPlace(Attrib);

        ++TmpToken;
        VERIFY_PARSER_STATE(TmpToken, TmpToken != m_Tokens.end() && TmpToken->Type == TokenType::OpenBracket, "\'(\' expected");
        String AttribValue;
        ProcessScope(
            TmpToken, m_Tokens.end(), TokenType::OpenBracket, TokenType::ClosingBracket,
            [&](TokenListType::iterator& tkn, int) //
            {
                AttribValue.append(tkn->Delimiter);
                AttribValue.append(tkn->Literal);
                ++tkn;
            } //
        );
        VERIFY_PARSER_STATE(TmpToken, TmpToken != m_Tokens.end() && TmpToken->Type == TokenType::ClosingBracket, "\']\' expected");
        Attributes.emplace(make_pair(HashMapStringKey(move(Attrib)), AttribValue));

        --Token;
        // [patchconstantfunc("ConstantHS")]
        //                                 ^
        // [maxtessfactor( (float)(32.f+2.f) )]

        // OpenStapleTokens
        // V
        // [maxtessfactor( (float)(32.f+2.f) )]
        // HSOutput main(InputPatch<VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
        // ^
        // TypeToken
        TypeToken->Delimiter = OpenStapleToken->Delimiter;
        m_Tokens.erase(OpenStapleToken, TypeToken);
    }
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessHullShaderArguments(TokenListType::iterator&          TypeToken,
                                                                          std::vector<ShaderParameterInfo>& Params,
                                                                          String&                           Globals,
                                                                          std::stringstream&                ReturnHandlerSS,
                                                                          String&                           Prologue)
{
    auto Token = TypeToken;
    // [...]
    // HSOutput main(InputPatch<VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
    // ^
    enum class Domain
    {
        undefined,
        tri,
        quad,
        isoline
    } domain = Domain::undefined;
    enum class Partitioning
    {
        undefined,
        integer,
        fractional_even,
        fractional_odd,
        pow2
    } partitioning = Partitioning::undefined;
    enum class OutputTopology
    {
        undefined,
        point,
        line,
        triangle_cw,
        triangle_ccw
    } topology = OutputTopology::undefined;

    std::unordered_map<HashMapStringKey, String, HashMapStringKey::Hasher> Attributes;
    ProcessShaderAttributes(Token, Attributes);

    auto DomainIt = Attributes.find("domain");
    if (DomainIt != Attributes.end())
    {
        if (DomainIt->second == "tri")
            domain = Domain::tri;
        else if (DomainIt->second == "quad")
            domain = Domain::quad;
        else if (DomainIt->second == "isoline")
            domain = Domain::isoline;
        else
            LOG_ERROR_AND_THROW("Unexpected domain value \"", DomainIt->second, "\". String constant \"tri\", \"quad\" or \"isoline\" expected");
    }

    auto PartitioningIt = Attributes.find("partitioning");
    if (PartitioningIt != Attributes.end())
    {
        if (PartitioningIt->second == "integer")
            partitioning = Partitioning::integer;
        else if (PartitioningIt->second == "fractional_even")
            partitioning = Partitioning::fractional_even;
        else if (PartitioningIt->second == "fractional_odd")
            partitioning = Partitioning::fractional_odd;
        else if (PartitioningIt->second == "pow2")
            partitioning = Partitioning::pow2;
        else
            LOG_ERROR_AND_THROW("Unexpected partitioning \"", PartitioningIt->second, "\". String constant \"integer\", \"fractional_even\", \"fractional_odd\", or \"pow2\" expected");
    }

    auto TopologyIt = Attributes.find("outputtopology");
    if (TopologyIt != Attributes.end())
    {
        if (TopologyIt->second == "point")
            topology = OutputTopology::point;
        else if (TopologyIt->second == "line")
            topology = OutputTopology::line;
        else if (TopologyIt->second == "triangle_cw")
            topology = OutputTopology::triangle_cw;
        else if (TopologyIt->second == "triangle_ccw")
            topology = OutputTopology::triangle_ccw;
        else
            LOG_ERROR_AND_THROW("Unexpected topology \"", TopologyIt->second, "\". String constant \"point\", \"line\", \"triangle_cw\", or \"triangle_ccw\" expected");
    }

    auto ConstFuncIt = Attributes.find("patchconstantfunc");
    if (ConstFuncIt == Attributes.end())
        LOG_ERROR_AND_THROW("Hull shader patch constant function is not specified. Use \"patchconstantfunc\" attribute");

    //auto MaxTessFactorIt = Attributes.find("maxtessfactor");
    auto NumControlPointsIt = Attributes.find("outputcontrolpoints");
    if (NumControlPointsIt == Attributes.end())
        LOG_ERROR_AND_THROW("Number of output control points is not specified. Use \"outputcontrolpoints\" attribute");
    const Char* NumControlPoints = NumControlPointsIt->second.c_str();

    bool        bConstFuncTakesInputPatch = false;
    const Char* ConstantFunc              = ConstFuncIt->second.c_str();
    ProcessHullShaderConstantFunction(ConstantFunc, bConstFuncTakesInputPatch);

    stringstream GlobalsSS;
    // In glsl, domain, partitioning, and topology are properties of tessellation evaluation
    // shader rather than tessellation control shader

    /*GlobalsSS << "layout(";
    switch (domain)
    {
        case Domain::tri:     GlobalsSS << "triangles"; break;
        case Domain::isoline: GlobalsSS << "isolines";  break;
        case Domain::quad:    GlobalsSS << "quads";     break;
        default: LOG_ERROR_AND_THROW( "Hull shader must specify domain" );
    }
    switch (partitioning)
    {
        case Partitioning::integer:         GlobalsSS << ", equal_spacing";           break;
        case Partitioning::fractional_even: GlobalsSS << ", fractional_even_spacing"; break;
        case Partitioning::fractional_odd:  GlobalsSS << ", fractional_odd_spacing";  break;
        case Partitioning::pow2:            
            LOG_WARNING_MESSAGE( "OpenGL does not support pow2 partitioning. Using integer instead" );
            GlobalsSS << ", equal_spacing";
        break;
    }
    switch (topology)
    {
        case OutputTopology::line:  break;
        case OutputTopology::point: break;
        case OutputTopology::triangle_ccw: GlobalsSS << ", ccw"; break;
        case OutputTopology::triangle_cw:  GlobalsSS << ", cw";  break;
    }
    GlobalsSS << ") in;\n";*/
    GlobalsSS << "layout(vertices = " << NumControlPoints << ") out;\n";

    std::stringstream PrologueSS, InterfaceVarsInSS, InterfaceVarsOutSS;
    int               inLocation = 0, outLocation = 0;
    for (const auto& TopLevelParam : Params)
    {
        if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::In)
        {
            bool IsPatch = TopLevelParam.HSAttribs.PatchType == ShaderParameterInfo::HSAttributes::InOutPatchType::InputPatch;
            if (IsPatch)
            {
                PrologueSS << "    const int _NumInputPoints = " << TopLevelParam.ArraySize << ";\n"; // gl_MaxPatchVertices
                PrologueSS << "    " << TopLevelParam.Type << ' ' << TopLevelParam.Name << "[_NumInputPoints];\n";
                // Iterate of the actual number of vertices in the input patch
                PrologueSS << "    for(int i=0; i < gl_PatchVerticesIn; ++i)\n    {\n";
            }

            ProcessShaderArgument(
                TopLevelParam, HSInd, InVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, String Getter) //
                {
                    // All inputs from vertex shaders to the TCS are aggregated into arrays, based on the size of the input patch.
                    // The size of these arrays is the number of input patches provided by the patch primitive.
                    // https://www.khronos.org/opengl/wiki/Tessellation_Control_Shader#Inputs
                    String FullIndexedParamName = BuildParameterName(MemberStack, '.', "", "", IsPatch ? "[i]" : "");
                    if (IsPatch)
                        PrologueSS << "    ";
                    if (!Getter.empty())
                        PrologueSS << "    " << Getter << '(' << FullIndexedParamName << ");\n";
                    else
                    {
                        auto VarName      = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_hsin_" : "_");
                        auto InputVarName = VarName + (IsPatch ? "[i]" : "");
                        // User-defined inputs can be declared as unbounded arrays
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? inLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat in" : "in",
                                           Param.Type, VarName + (IsPatch ? "[]" : ""), InterfaceVarsInSS);
                        InitVariable(FullIndexedParamName, InputVarName, PrologueSS);
                    }
                } //
            );

            if (IsPatch)
            {
                PrologueSS << "    }\n";
                // Add call to the constant function
                // Multiple TCS invocations for the same patch can write to the same tessellation level variable,
                // so long as they are all computing and writing the exact same value.
                // https://www.khronos.org/opengl/wiki/Tessellation_Control_Shader#Outputs
                PrologueSS << "    " << ConstantFunc << '(' << (bConstFuncTakesInputPatch ? TopLevelParam.Name : "") << ");\n";
            }
        }
        else if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::Out ||
                 TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret)
        {
            ProcessShaderArgument(
                TopLevelParam, HSInd, OutVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Setter) //
                {
                    String SrcParamName = BuildParameterName(MemberStack, '.', "", Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret ? "_RET_VAL_" : "");
                    if (!Setter.empty())
                        ReturnHandlerSS << Setter << '(' << SrcParamName << ");\\\n";
                    else
                    {
                        auto OutputVarName = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_hsout_" : "_");
                        // Per-vertex outputs are aggregated into arrays.
                        // https://www.khronos.org/opengl/wiki/Tessellation_Control_Shader#Outputs
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? outLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat out" : "out",
                                           Param.Type, OutputVarName + "[]", InterfaceVarsOutSS);
                        // A TCS can only ever write to the per-vertex output variable that corresponds to their invocation,
                        // so writes to per-vertex outputs must be of the form vertexTexCoord[gl_InvocationID]
                        ReturnHandlerSS << OutputVarName << "[gl_InvocationID] = " << SrcParamName << ";\\\n";
                    }
                } //
            );
        }
    }

    Prologue = PrologueSS.str();
    Globals  = GlobalsSS.str() + InterfaceVarsInSS.str() + InterfaceVarsOutSS.str();
}

void ParseAttributesInComment(const String& Comment, std::unordered_map<HashMapStringKey, String, HashMapStringKey::Hasher>& Attributes)
{
    auto Pos = Comment.begin();
    //    /* partitioning = fractional_even, outputtopology = triangle_cw */
    // ^
    if (SkipDelimeters(Comment, Pos))
        return;
    //    /* partitioning = fractional_even, outputtopology = triangle_cw */
    //    ^
    if (*Pos != '/')
        return;
    ++Pos;
    //    /* partitioning = fractional_even, outputtopology = triangle_cw */
    //     ^
    //    // partitioning = fractional_even, outputtopology = triangle_cw */
    //     ^
    if (Pos == Comment.end() || (*Pos != '/' && *Pos != '*'))
        return;
    ++Pos;
    while (Pos != Comment.end())
    {
        //    /* partitioning = fractional_even, outputtopology = triangle_cw */
        //      ^
        if (SkipDelimeters(Comment, Pos))
            return;
        //    /* partitioning = fractional_even, outputtopology = triangle_cw */
        //       ^
        auto AttribStart = Pos;
        SkipIdentifier(Comment, Pos);
        String Attrib(AttribStart, Pos);
        //    /* partitioning = fractional_even, outputtopology = triangle_cw */
        //                   ^
        if (SkipDelimeters(Comment, Pos))
            return;
        //    /* partitioning = fractional_even, outputtopology = triangle_cw */
        //                    ^
        if (*Pos != '=')
            return;
        ++Pos;
        //    /* partitioning = fractional_even, outputtopology = triangle_cw */
        //                     ^
        if (SkipDelimeters(Comment, Pos))
            return;
        //    /* partitioning = fractional_even, outputtopology = triangle_cw */
        //                     ^
        auto ValueStartPos = Pos;
        SkipIdentifier(Comment, Pos);
        //    /* partitioning = fractional_even , outputtopology = triangle_cw */
        //                                     ^
        String Value(ValueStartPos, Pos);
        Attributes.emplace(make_pair(HashMapStringKey(move(Attrib)), Value));

        if (SkipDelimeters(Comment, Pos))
            return;
        //    /* partitioning = fractional_even , outputtopology = triangle_cw */
        //                                      ^
        if (*Pos != ',' && *Pos != ';')
            return;
        ++Pos;
        //    /* partitioning = fractional_even , outputtopology = triangle_cw */
        //                                       ^
    }
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessDomainShaderArguments(TokenListType::iterator&          TypeToken,
                                                                            std::vector<ShaderParameterInfo>& Params,
                                                                            String&                           Globals,
                                                                            std::stringstream&                ReturnHandlerSS,
                                                                            String&                           Prologue)
{
    auto Token = TypeToken;
    // [domain("quad")]
    // DSOut main( HS_CONSTANT_DATA_OUTPUT input,
    // ^

    std::unordered_map<HashMapStringKey, String, HashMapStringKey::Hasher> Attributes;
    ParseAttributesInComment(TypeToken->Delimiter, Attributes);
    ProcessShaderAttributes(Token, Attributes);

    stringstream GlobalsSS;
    auto         DomainIt = Attributes.find("domain");
    if (DomainIt == Attributes.end())
        LOG_ERROR_AND_THROW("Domain shader misses \"domain\" attribute");

    GlobalsSS << "layout(";
    if (DomainIt->second == "tri")
        GlobalsSS << "triangles";
    else if (DomainIt->second == "quad")
        GlobalsSS << "quads";
    else if (DomainIt->second == "isoline")
        GlobalsSS << "isolines";
    else
        LOG_ERROR_AND_THROW("Unexpected domain value \"", DomainIt->second, "\". String constant \"tri\", \"quad\" or \"isoline\" expected");

    auto PartitioningIt = Attributes.find("partitioning");
    if (PartitioningIt == Attributes.end())
        LOG_ERROR_AND_THROW("Undefined partitioning. In GLSL, partitioning is specified by the tessellation evaluation shader (domain shader) rather than by the tessellation control shader (hull shader)\n"
                            "Please use the following comment right above the function declaration to deine partitioning and output topology:\n"
                            "/* partitioning = {integer|fractional_even|fractional_odd}, outputtopology = {triangle_cw|triangle_ccw} */");

    if (PartitioningIt->second == "integer")
        GlobalsSS << ", equal_spacing";
    else if (PartitioningIt->second == "fractional_even")
        GlobalsSS << ", fractional_even_spacing";
    else if (PartitioningIt->second == "fractional_odd")
        GlobalsSS << ", fractional_odd_spacing";
    else if (PartitioningIt->second == "pow2")
    {
        LOG_WARNING_MESSAGE("pow2 partitioning is not supported by OpenGL. Using integer partitioning");
        GlobalsSS << ", equal_spacing";
    }
    else
        LOG_ERROR_AND_THROW("Unexpected partitioning \"", PartitioningIt->second, "\". String constant \"integer\", \"fractional_even\", \"fractional_odd\", or \"pow2\" expected");

    auto TopologyIt = Attributes.find("outputtopology");
    if (TopologyIt == Attributes.end())
        LOG_ERROR_AND_THROW("Undefined outputtopology. In GLSL, outputtopology is specified by the tessellation evaluation shader (domain shader) rather than by the tessellation control shader (hull shader)\n"
                            "Please use the following comment right above the function declaration to deine partitioning and output topology:\n"
                            "/* partitioning = {integer|fractional_even|fractional_odd}, outputtopology = {triangle_cw|triangle_ccw} */");

    if (TopologyIt->second == "point")
        GlobalsSS << "";
    else if (TopologyIt->second == "line")
        GlobalsSS << "";
    else if (TopologyIt->second == "triangle_cw")
        GlobalsSS << ", cw";
    else if (TopologyIt->second == "triangle_ccw")
        GlobalsSS << ", ccw";
    else
        LOG_ERROR_AND_THROW("Unexpected topology \"", TopologyIt->second, "\". String constant \"point\", \"line\", \"triangle_cw\", or \"triangle_ccw\" expected");

    GlobalsSS << ")in;\n";

    std::stringstream PrologueSS, InterfaceVarsInSS, InterfaceVarsOutSS;
    int               inLocation = 0, outLocation = 0;
    for (const auto& TopLevelParam : Params)
    {
        if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::In)
        {
            bool IsPatch = TopLevelParam.HSAttribs.PatchType == ShaderParameterInfo::HSAttributes::InOutPatchType::OutputPatch;
            if (IsPatch)
            {
                PrologueSS << "    const int _NumInputPoints = " << TopLevelParam.ArraySize << ";\n"; // gl_MaxPatchVertices
                PrologueSS << "    " << TopLevelParam.Type << ' ' << TopLevelParam.Name << "[_NumInputPoints];\n";
                // Iterate of the actual number of vertices in the input patch
                PrologueSS << "    for(int i=0; i < gl_PatchVerticesIn; ++i)\n    {\n";
            }

            ProcessShaderArgument(
                TopLevelParam, DSInd, InVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Getter) //
                {
                    // All inputs from vertex shaders to the TCS are aggregated into arrays, based on the size of the input patch.
                    // The size of these arrays is the number of input patches provided by the patch primitive.
                    // https://www.khronos.org/opengl/wiki/Tessellation_Control_Shader#Inputs
                    String FullIndexedParamName = BuildParameterName(MemberStack, '.', "", "", IsPatch ? "[i]" : "");
                    if (IsPatch)
                        PrologueSS << "    ";
                    if (Getter.empty())
                    {
                        auto VarName      = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_dsin_" : "_");
                        auto InputVarName = VarName + (IsPatch ? "[i]" : "");
                        // User-defined inputs can be declared as unbounded arrays
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? inLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat in" : "in",
                                           Param.Type, VarName + (IsPatch ? "[]" : ""), InterfaceVarsInSS);
                        InitVariable(FullIndexedParamName, InputVarName, PrologueSS);
                    }
                    else
                        PrologueSS << "    " << Getter << '(' << FullIndexedParamName << ");\n";
                } //
            );

            if (IsPatch)
            {
                PrologueSS << "    }\n";
            }
        }
        else if (TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::Out ||
                 TopLevelParam.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret)
        {
            ProcessShaderArgument(
                TopLevelParam, DSInd, OutVar, PrologueSS,
                [&](const std::vector<const ShaderParameterInfo*>& MemberStack, const ShaderParameterInfo& Param, const String& Setter) //
                {
                    String SrcParamName = BuildParameterName(MemberStack, '.', "", Param.storageQualifier == ShaderParameterInfo::StorageQualifier::Ret ? "_RET_VAL_" : "");
                    if (Setter.empty())
                    {
                        auto OutputVarName = BuildParameterName(MemberStack, '_', m_bUseInOutLocationQualifiers ? "_dsout_" : "_");
                        // Per-vertex outputs are aggregated into arrays.
                        // https://www.khronos.org/opengl/wiki/Tessellation_Control_Shader#Outputs
                        DefineInterfaceVar(m_bUseInOutLocationQualifiers ? outLocation++ : -1,
                                           RequiresFlatQualifier(Param.Type) ? "flat out" : "out",
                                           Param.Type, OutputVarName, InterfaceVarsOutSS);
                        // A TCS can only ever write to the per-vertex output variable that corresponds to their invocation,
                        // so writes to per-vertex outputs must be of the form vertexTexCoord[gl_InvocationID]
                        ReturnHandlerSS << OutputVarName << " = " << SrcParamName << ";\\\n";
                    }
                    else
                        ReturnHandlerSS << Setter << '(' << SrcParamName << ");\\\n";
                } //
            );
        }
    }
    Prologue = PrologueSS.str();
    Globals  = GlobalsSS.str() + InterfaceVarsInSS.str() + InterfaceVarsOutSS.str();
}


void HLSL2GLSLConverterImpl::ConversionStream::ProcessReturnStatements(TokenListType::iterator& Token, bool IsVoid, const Char* EntryPoint, const Char* MacroName)
{
    // void main ()
    // {
    // ^
    VERIFY_EXPR(Token->Type == TokenType::OpenBrace);

    ++Token; // Skip open brace
    int BraceCount = 1;
    // Find matching closing brace
    while (Token != m_Tokens.end())
    {
        if (Token->Type == TokenType::OpenBrace)
            ++BraceCount;
        else if (Token->Type == TokenType::ClosingBrace)
        {
            --BraceCount;
            if (BraceCount == 0)
                break;
        }
        else if (Token->IsFlowControl())
        {
            if (Token->Type == TokenType::kw_return)
            {
                //if( x < 0.5 ) return float4(0.0, 0.0, 0.0, 1.0);
                //              ^
                Token->Type    = TokenType::Identifier;
                Token->Literal = MacroName;
                //if( x < 0.5 ) _RETURN_ float4(0.0, 0.0, 0.0, 1.0);
                //              ^

                ++Token;
                //if( x < 0.5 ) _RETURN_ float4(0.0, 0.0, 0.0, 1.0);
                //                       ^

                if (Token->Type != TokenType::Semicolon)
                {
                    m_Tokens.insert(Token, TokenInfo(TokenType::OpenBracket, "("));
                    //if( x < 0.5 ) _RETURN_( float4(0.0, 0.0, 0.0, 1.0);
                    //                        ^

                    while (Token != m_Tokens.end() && Token->Type != TokenType::Semicolon)
                        ++Token;
                    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected end of file while looking for the \';\'");
                    //if( x < 0.5 ) _RETURN_( float4(0.0, 0.0, 0.0, 1.0);
                    //                                                  ^

                    // Replace semicolon with ')'
                    Token->Type    = TokenType::ClosingBracket;
                    Token->Literal = ")";
                    //if( x < 0.5 ) _RETURN_( float4(0.0, 0.0, 0.0, 1.0))
                    //                                                  ^
                }
                else
                {
                    //if( x < 0.5 ) _RETURN_ ;
                    //                       ^
                    auto SemicolonToken = Token;
                    ++Token;
                    //if( x < 0.5 ) _RETURN_ ;
                    //else
                    //^
                    m_Tokens.erase(SemicolonToken);
                    //if( x < 0.5 ) _RETURN_
                    //else
                    //^
                }

                continue;
            }
        }
        ++Token;
    }
    VERIFY_PARSER_STATE(Token, BraceCount == 0, "No matching closing bracket found");

    // void main ()
    // {
    //      ...
    // }
    // ^
    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected end of file while looking for the end of body of shader entry point \"", EntryPoint, "\".");
    VERIFY_EXPR(Token->Type == TokenType::ClosingBrace);
    if (IsVoid)
    {
        // Insert return handler before the closing brace
        m_Tokens.insert(Token, TokenInfo(TokenType::TextBlock, MacroName, Token->Delimiter.c_str()));
        Token->Delimiter = "\n";
        // void main ()
        // {
        //      ...
        //      _RETURN_
        // }
        // ^
    }
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessGSOutStreamOperations(TokenListType::iterator& Token, const String& OutStreamName, const char* EntryPoint)
{
    VERIFY_EXPR(Token->Type == TokenType::OpenBrace);

    ++Token; // Skip open brace
    int BraceCount = 1;
    // Find matching closing brace
    while (Token != m_Tokens.end())
    {
        if (Token->Type == TokenType::OpenBrace)
            ++BraceCount;
        else if (Token->Type == TokenType::ClosingBrace)
        {
            --BraceCount;
            if (BraceCount == 0)
                break;
        }
        if (Token->Type == TokenType::Identifier && Token->Literal == OutStreamName)
        {
            // triStream.Append( Out );
            // ^
            ++Token;
            // triStream.Append( Out );
            //          ^
            VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF");
            VERIFY_PARSER_STATE(Token, Token->Literal == ".", "\'.\' expected");
            Token->Literal = "_";
            Token->Delimiter.clear();
            // triStream_Append( Out );
            //          ^
            ++Token;
            // triStream_Append( Out );
            //           ^
            VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF");
            Token->Delimiter.clear();
            ++Token;
        }
        else
            ++Token;
    }
}

void HLSL2GLSLConverterImpl::ConversionStream::ProcessShaderDeclaration(TokenListType::iterator EntryPointToken, SHADER_TYPE ShaderType)
{
    const auto* EntryPoint = EntryPointToken->Literal.c_str();

    auto TypeToken = EntryPointToken;
    --TypeToken;
    // void TestPS  ( in VSOutput In,
    // ^
    // TypeToken
    VERIFY_PARSER_STATE(TypeToken, TypeToken != m_Tokens.begin(), "Function \"", EntryPointToken->Literal, "\" misses return type");

    std::vector<ShaderParameterInfo> ShaderParams;

    auto ArgsListEndToken = TypeToken;
    bool bIsVoid          = false;
    ProcessFunctionParameters(ArgsListEndToken, ShaderParams, bIsVoid);

    EntryPointToken->Literal = "main";
    //void main ()

    std::stringstream ReturnHandlerSS;
    const Char*       ReturnMacroName = "_RETURN_";
    // Some GLES compilers cannot properly handle macros with empty argument lists, such as _RETURN_().
    // Also, some compilers generate an error if there is no whitespace after the macro without arguments: _RETURN_{
    ReturnHandlerSS << "#define " << ReturnMacroName << (bIsVoid ? "" : "(_RET_VAL_)") << " {\\\n";

    String GlobalVariables, Prologue;
    try
    {
        if (ShaderType == SHADER_TYPE_PIXEL)
        {
            ProcessFragmentShaderArguments(ShaderParams, GlobalVariables, ReturnHandlerSS, Prologue);
        }
        else if (ShaderType == SHADER_TYPE_VERTEX)
        {
            ProcessVertexShaderArguments(ShaderParams, GlobalVariables, ReturnHandlerSS, Prologue);
        }
        else if (ShaderType == SHADER_TYPE_GEOMETRY)
        {
            ProcessGeometryShaderArguments(TypeToken, ShaderParams, GlobalVariables, Prologue);
        }
        else if (ShaderType == SHADER_TYPE_HULL)
        {
            ProcessHullShaderArguments(TypeToken, ShaderParams, GlobalVariables, ReturnHandlerSS, Prologue);
        }
        else if (ShaderType == SHADER_TYPE_DOMAIN)
        {
            ProcessDomainShaderArguments(TypeToken, ShaderParams, GlobalVariables, ReturnHandlerSS, Prologue);
        }
        else if (ShaderType == SHADER_TYPE_COMPUTE)
        {
            ProcessComputeShaderArguments(TypeToken, ShaderParams, GlobalVariables, Prologue);
        }
    }
    catch (const std::runtime_error&)
    {
        LOG_ERROR_AND_THROW("Failed to process shader parameters for shader \"", EntryPoint, "\".");
    }
    ReturnHandlerSS << "return;}\n";


    // void main ()
    // ^
    // TypeToken

    // Insert global variables & return handler before the function
    m_Tokens.insert(TypeToken, TokenInfo(TokenType::TextBlock, GlobalVariables.c_str(), TypeToken->Delimiter.c_str()));
    m_Tokens.insert(TypeToken, TokenInfo(TokenType::TextBlock, ReturnHandlerSS.str().c_str(), "\n"));
    TypeToken->Delimiter = "\n";
    auto BodyStartToken  = ArgsListEndToken;
    while (BodyStartToken != m_Tokens.end() && BodyStartToken->Type != TokenType::OpenBrace)
        ++BodyStartToken;
    // void main ()
    // {
    // ^
    VERIFY_PARSER_STATE(BodyStartToken, BodyStartToken != m_Tokens.end(), "Unexpected end of file while looking for the body of shader entry point \"", EntryPoint, "\".");
    auto FirstStatementToken = BodyStartToken;
    ++FirstStatementToken;
    // void main ()
    // {
    //      int a;
    //      ^
    VERIFY_PARSER_STATE(FirstStatementToken, FirstStatementToken != m_Tokens.end(), "Unexpected end of file while looking for the body of shader entry point \"", EntryPoint, "\".");

    // Insert prologue before the first token
    m_Tokens.insert(FirstStatementToken, TokenInfo(TokenType::TextBlock, Prologue.c_str(), "\n"));

    auto BodyEndToken = BodyStartToken;
    if (ShaderType == SHADER_TYPE_VERTEX || ShaderType == SHADER_TYPE_HULL || ShaderType == SHADER_TYPE_DOMAIN || ShaderType == SHADER_TYPE_PIXEL)
    {
        ProcessReturnStatements(BodyEndToken, bIsVoid, EntryPoint, ReturnMacroName);
    }
    else if (ShaderType == SHADER_TYPE_GEOMETRY)
    {
        auto OutStreamParamIt = ShaderParams.begin();
        for (; OutStreamParamIt != ShaderParams.end(); ++OutStreamParamIt)
            if (OutStreamParamIt->GSAttribs.Stream != ShaderParameterInfo::GSAttributes::StreamType::Undefined)
                break;
        VERIFY_PARSER_STATE(FirstStatementToken, OutStreamParamIt != ShaderParams.end(), "Unable to find output stream variable");
        ProcessGSOutStreamOperations(BodyEndToken, OutStreamParamIt->Name, EntryPoint);
    }
}


void HLSL2GLSLConverterImpl::ConversionStream::RemoveSemanticsFromBlock(TokenListType::iterator& Token, TokenType OpenBracketType, TokenType ClosingBracketType)
{
    VERIFY_EXPR(Token->Type == OpenBracketType);
    ProcessScope(
        Token, m_Tokens.end(), OpenBracketType, ClosingBracketType,
        [&](TokenListType::iterator& tkn, int) //
        {
            if (tkn->Literal == ":")
            {
                // float4 Pos : POSITION;
                //            ^
                auto ColonToken = tkn;
                ++tkn;
                // float4 Pos : POSITION;
                //              ^
                if (tkn->Type == TokenType::Identifier)
                {
                    ++tkn;
                    // float4 Pos : POSITION;
                    //                      ^

                    // float4 Pos : POSITION, Normal : NORMAL;
                    //                      ^

                    // float4 Pos : POSITION)
                    //                      ^
                    if (tkn->Type == TokenType::Semicolon || tkn->Literal == "," || tkn->Type == TokenType::ClosingBracket)
                    {
                        m_Tokens.erase(ColonToken, tkn);
                        // float4 Pos ;
                        //            ^
                    }
                }
            }
            else
                ++tkn;
        } //
    );
    // float4 TestPS()
    //               ^

    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while parsing scope");
    VERIFY_EXPR(Token->Type == ClosingBracketType);
    ++Token;
}

void HLSL2GLSLConverterImpl::ConversionStream::RemoveSemantics()
{
    auto ScopeStartToken = m_Tokens.begin();
    ProcessScope(
        ScopeStartToken, m_Tokens.end(), TokenType::OpenBrace, TokenType::ClosingBrace,
        [&](TokenListType::iterator& Token, int ScopeDepth) //
        {
            // Search global scope only
            if (ScopeDepth == 0)
            {
                if (Token->Type == TokenType::kw_struct)
                {
                    //struct MyStruct
                    //^
                    while (Token != m_Tokens.end() && Token->Type != TokenType::OpenBrace)
                        ++Token;

                    VERIFY_PARSER_STATE(Token, Token != m_Tokens.end(), "Unexpected EOF while searching for the structure body");
                    //struct MyStruct
                    //{
                    //^
                    RemoveSemanticsFromBlock(Token, TokenType::OpenBrace, TokenType::ClosingBrace);

                    // struct MyStruct
                    // {
                    //    ...
                    // };
                    //  ^
                }
                else if (Token->Type == TokenType::Identifier)
                {
                    // Searh for "Identifier(" pattern
                    // In global scope this should be texture declaration
                    // It can also be other things like macro. But this is not a problem.
                    ++Token;
                    if (Token == m_Tokens.end())
                        return;
                    if (Token->Type == TokenType::OpenBracket)
                    {
                        RemoveSemanticsFromBlock(Token, TokenType::OpenBracket, TokenType::ClosingBracket);
                        // void TestVS( ... )
                        // {
                        // ^

                        if (Token != m_Tokens.end() && Token->Literal == ":")
                        {
                            // float4 TestPS() : SV_Target
                            //                 ^
                            auto ColonToken = Token;
                            ++Token;
                            // float4 TestPS() : SV_Target
                            //                   ^
                            if (Token->Type == TokenType::Identifier)
                            {
                                ++Token;
                                if (Token->Type == TokenType::OpenBrace)
                                {
                                    // float4 TestPS() : SV_Target
                                    // {
                                    // ^
                                    m_Tokens.erase(ColonToken, Token);
                                    // float4 TestPS()
                                    // {
                                    // ^
                                }
                            }
                        }
                    }
                }
                else
                    ++Token;
            }
            else
                ++Token;
        } //
    );
}

// Remove special shader attributes such as [numthreads(16, 16, 1)]
void HLSL2GLSLConverterImpl::ConversionStream::RemoveSpecialShaderAttributes()
{
    auto ScopeStartToken = m_Tokens.begin();
    ProcessScope(
        ScopeStartToken, m_Tokens.end(), TokenType::OpenBrace, TokenType::ClosingBrace,
        [&](TokenListType::iterator& Token, int ScopeDepth) //
        {
            // Search global scope only
            if (ScopeDepth != 0 || Token->Type != TokenType::OpenStaple)
            {
                ++Token;
                return;
            }

            // [numthreads(16, 16, 1)]
            // ^
            auto OpenStaple = Token;
            ++Token;
            if (Token == m_Tokens.end())
                return;
            // [numthreads(16, 16, 1)]
            //  ^
            if (Token->Literal == "numthreads")
            {
                ++Token;
                // [numthreads(16, 16, 1)]
                //            ^
                if (Token->Type != TokenType::OpenBracket)
                    return;
                while (Token != m_Tokens.end() && Token->Type != TokenType::ClosingStaple)
                    ++Token;
                // [numthreads(16, 16, 1)]
                //                       ^
                if (Token == m_Tokens.end())
                    return;
                ++Token;
                // [numthreads(16, 16, 1)]
                // void CS(uint3 ThreadId  : SV_DispatchThreadID)
                // ^
                if (Token != m_Tokens.end())
                    Token->Delimiter = OpenStaple->Delimiter + Token->Delimiter;
                m_Tokens.erase(OpenStaple, Token);
            }
            else
                ++Token;
        } //
    );
}

String HLSL2GLSLConverterImpl::ConversionStream::BuildGLSLSource()
{
    String Output;
    for (const auto& Token : m_Tokens)
    {
        Output.append(Token.Delimiter);
        Output.append(Token.Literal);
    }
    return Output;
}

HLSL2GLSLConverterImpl::ConversionStream::ConversionStream(IReferenceCounters*              pRefCounters,
                                                           const HLSL2GLSLConverterImpl&    Converter,
                                                           const char*                      InputFileName,
                                                           IShaderSourceInputStreamFactory* pInputStreamFactory,
                                                           const Char*                      HLSLSource,
                                                           size_t                           NumSymbols,
                                                           bool                             bPreserveTokens) :
    // clang-format off
    TBase            {pRefCounters   },
    m_bPreserveTokens{bPreserveTokens},
    m_Converter      {Converter      },
    m_InputFileName  {InputFileName != nullptr ? InputFileName : "<Unknown>"}
// clang-format on
{
    RefCntAutoPtr<IDataBlob> pFileData;
    if (HLSLSource == nullptr)
    {
        if (InputFileName == nullptr)
            LOG_ERROR_AND_THROW("Input file name must not be null when HLSL source code is not provided");

        if (pInputStreamFactory == nullptr)
            LOG_ERROR_AND_THROW("Input stream factory must not be null when HLSL source code is not provided");

        RefCntAutoPtr<IFileStream> pSourceStream;
        pInputStreamFactory->CreateInputStream(InputFileName, &pSourceStream);
        if (pSourceStream == nullptr)
            LOG_ERROR_AND_THROW("Failed to open shader source file ", InputFileName);

        pFileData = MakeNewRCObj<DataBlobImpl>()(0);
        pSourceStream->ReadBlob(pFileData);
        HLSLSource = reinterpret_cast<char*>(pFileData->GetDataPtr());
        NumSymbols = pFileData->GetSize();
    }

    String Source(HLSLSource, NumSymbols);

    InsertIncludes(Source, pInputStreamFactory);

    Tokenize(Source);
}


String HLSL2GLSLConverterImpl::Convert(ConversionAttribs& Attribs) const
{
    if (Attribs.ppConversionStream == nullptr)
    {
        try
        {
            ConversionStream Stream(nullptr, *this, Attribs.InputFileName, Attribs.pSourceStreamFactory, Attribs.HLSLSource, Attribs.NumSymbols, false);
            return Stream.Convert(Attribs.EntryPoint, Attribs.ShaderType, Attribs.IncludeDefinitions, Attribs.SamplerSuffix, Attribs.UseInOutLocationQualifiers);
        }
        catch (std::runtime_error&)
        {
            return "";
        }
    }
    else
    {
        ConversionStream* pStream = nullptr;
        if (*Attribs.ppConversionStream != nullptr)
        {
            pStream = ValidatedCast<ConversionStream>(*Attribs.ppConversionStream);

            const auto& FileNameFromStream = pStream->GetInputFileName();
            if (FileNameFromStream != Attribs.InputFileName)
            {
                LOG_WARNING_MESSAGE("Input stream was initialized for input file \"", FileNameFromStream, "\" that does not match the name of the file to be converted \"", Attribs.InputFileName, "\". New stream will be created");
                (*Attribs.ppConversionStream)->Release();
                *Attribs.ppConversionStream = nullptr;
            }
        }

        if (*Attribs.ppConversionStream == nullptr)
        {
            CreateStream(Attribs.InputFileName, Attribs.pSourceStreamFactory, Attribs.HLSLSource, Attribs.NumSymbols, Attribs.ppConversionStream);
            pStream = ValidatedCast<ConversionStream>(*Attribs.ppConversionStream);
        }

        return pStream->Convert(Attribs.EntryPoint, Attribs.ShaderType, Attribs.IncludeDefinitions, Attribs.SamplerSuffix, Attribs.UseInOutLocationQualifiers);
    }
}

void HLSL2GLSLConverterImpl::CreateStream(const Char*                      InputFileName,
                                          IShaderSourceInputStreamFactory* pSourceStreamFactory,
                                          const Char*                      HLSLSource,
                                          size_t                           NumSymbols,
                                          IHLSL2GLSLConversionStream**     ppStream) const
{
    try
    {
        auto* pStream = NEW_RC_OBJ(GetRawAllocator(), "HLSL2GLSLConverterImpl::ConversionStream object instance", ConversionStream)(*this, InputFileName, pSourceStreamFactory, HLSLSource, NumSymbols, true);
        pStream->QueryInterface(IID_HLSL2GLSLConversionStream, reinterpret_cast<IObject**>(ppStream));
    }
    catch (std::runtime_error&)
    {
        *ppStream = nullptr;
    }
}

void HLSL2GLSLConverterImpl::ConversionStream::Convert(const Char* EntryPoint,
                                                       SHADER_TYPE ShaderType,
                                                       bool        IncludeDefintions,
                                                       const char* SamplerSuffix,
                                                       bool        UseInOutLocationQualifiers,
                                                       IDataBlob** ppGLSLSource)
{
    try
    {
        auto                GLSLSource = Convert(EntryPoint, ShaderType, IncludeDefintions, SamplerSuffix, UseInOutLocationQualifiers);
        StringDataBlobImpl* pDataBlob  = MakeNewRCObj<StringDataBlobImpl>()(std::move(GLSLSource));
        pDataBlob->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ppGLSLSource));
    }
    catch (std::runtime_error&)
    {
        *ppGLSLSource = nullptr;
    }
}

String HLSL2GLSLConverterImpl::ConversionStream::Convert(const Char* EntryPoint,
                                                         SHADER_TYPE ShaderType,
                                                         bool        IncludeDefintions,
                                                         const char* SamplerSuffix,
                                                         bool        UseInOutLocationQualifiers)
{
    m_bUseInOutLocationQualifiers = UseInOutLocationQualifiers;
    TokenListType TokensCopy(m_bPreserveTokens ? m_Tokens : TokenListType());

    Uint32 ShaderStorageBlockBinding = 0;
    Uint32 ImageBinding              = 0;

    std::unordered_map<String, bool> SamplersHash;

    auto Token = m_Tokens.begin();
    // Process constant buffers, fix floating point constants,
    // remove flow control attributes and sampler registers
    while (Token != m_Tokens.end())
    {
        switch (Token->Type)
        {
            case TokenType::kw_cbuffer:
                ProcessConstantBuffer(Token);
                break;

            case TokenType::kw_RWStructuredBuffer:
            case TokenType::kw_StructuredBuffer:
                ProcessStructuredBuffer(Token, ShaderStorageBlockBinding);
                break;

            case TokenType::kw_struct:
                RegisterStruct(Token);
                break;

            case TokenType::NumericConstant:
                // This all work is only required because some GLSL compilers are so stupid that
                // flood shader output with insane warnings like this:
                // WARNING: 0:259: Only GLSL version > 110 allows postfix "F" or "f" for float
                // even when compiling for GL 4.3 AND the code IS UNDER #if 0
                if (Token->Literal.back() == 'f' || Token->Literal.back() == 'F')
                    Token->Literal.pop_back();
                ++Token;
                break;

            case TokenType::kw_SamplerState:
            case TokenType::kw_SamplerComparisonState:
                RemoveSamplerRegister(Token);
                break;

            default:
                if (Token->IsFlowControl())
                {
                    // Remove flow control attributes like [flatten], [branch], [loop], etc.
                    RemoveFlowControlAttribute(Token);
                }
                ++Token;
        }
    }

    auto ShaderEntryPointToken = m_Tokens.end();
    // Process textures and search for the shader entry point.
    // GLSL does not allow local variables of sampler type, so the
    // only two scopes where textures can be declared are global scope
    // and a function argument list.
    {
        TokenListType::iterator      FunctionStart = m_Tokens.end();
        std::vector<SamplerHashType> Samplers;

        // Find all samplers in the global scope
        Samplers.emplace_back();
        m_Objects.emplace_back();
        Token = m_Tokens.begin();
        ParseSamplers(Token, Samplers.back());
        VERIFY_EXPR(Token == m_Tokens.end());

        Int32 ScopeDepth = 0;

        Token = m_Tokens.begin();
        while (Token != m_Tokens.end())
        {
            // Detect global function declaration by looking for the pattern
            //     <return type> Identifier (
            // in global scope
            if (ScopeDepth == 0 && Token->Type == TokenType::Identifier)
            {
                // float4 Func ( in float2 f2UV,
                //        ^
                //      Token
                auto ReturnTypeToken = Token;
                --ReturnTypeToken;
                if (ReturnTypeToken == m_Tokens.begin())
                    break;
                auto OpenParenToken = Token;
                ++OpenParenToken;
                if (OpenParenToken == m_Tokens.end())
                    break;
                // ReturnTypeToken
                // |     Token
                // |      |
                // float4 Func ( in float2 f2UV,
                //             ^
                //       OpenParenToken
                if ((ReturnTypeToken->IsBuiltInType() || ReturnTypeToken->Type == TokenType::Identifier) &&
                    OpenParenToken->Type == TokenType::OpenBracket)
                {
                    if (Token->Literal == EntryPoint)
                        ShaderEntryPointToken = Token;

                    Token = OpenParenToken;
                    // float4 Func ( in float2 f2UV,
                    //             ^
                    //           Token

                    // Parse samplers in the function argument list
                    Samplers.emplace_back(SamplerHashType());
                    // GLSL does not support sampler variables,
                    // so the only place where a new sampler
                    // declaration is allowed is function argument
                    // list
                    auto ArgListEnd = Token;
                    ParseSamplers(ArgListEnd, Samplers.back());
                    // float4 Func ( in float2 f2UV )
                    //                              ^
                    //                          ArgListEnd
                    auto TmpToken = ArgListEnd;
                    ++TmpToken;
                    if (TmpToken != m_Tokens.end() && TmpToken->Literal == ":")
                    {
                        // float4 Func ( in float2 f2UV ) : SV_Target
                        //                                ^
                        ++TmpToken;
                        // float4 Func ( in float2 f2UV ) : SV_Target
                        //                                  ^
                        if (TmpToken != m_Tokens.end() && TmpToken->Type == TokenType::Identifier)
                            ++TmpToken;
                    }
                    // float4 Func ( in float2 f2UV ) : SV_Target
                    // {
                    // ^
                    if (TmpToken != m_Tokens.end() && TmpToken->Type == TokenType::OpenBrace)
                    {
                        // We need to go through the function argument
                        // list as there may be texture declaraions
                        ++Token;
                        // float4 Func ( in float2 f2UV,
                        //               ^
                        //             Token

                        // Put empty table on top of the object stack
                        m_Objects.emplace_back(ObjectsTypeHashType());
                    }
                    else
                    {
                        // For some reason there is no open brace after
                        // what should be argument list - pop the samplers
                        Samplers.pop_back();
                    }
                }
            }

            if (Token->Type == TokenType::OpenBrace)
            {
                if (Samplers.size() == 2 && ScopeDepth == 0)
                {
                    VERIFY_EXPR(FunctionStart == m_Tokens.end());
                    // This is the first open brace after the
                    // Samplers stack has grown to two -> this is
                    // the beginning of a function body
                    FunctionStart = Token;
                }
                ++ScopeDepth;
                ++Token;
            }
            else if (Token->Type == TokenType::ClosingBrace)
            {
                --ScopeDepth;
                if (Samplers.size() == 2 && ScopeDepth == 0)
                {
                    // We are returning to the global scope now and
                    // the samplers stack size is 2 -> this was a function
                    // body. We need to process it now.

                    ProcessObjectMethods(FunctionStart, Token);

                    ProcessRWTextures(FunctionStart, Token);

                    ProcessAtomics(FunctionStart, Token);

                    // Pop function arguments from the sampler and object
                    // stacks
                    Samplers.pop_back();
                    m_Objects.pop_back();
                    FunctionStart = m_Tokens.end();
                }
                ++Token;
            }
            // clang-format off
            else if (Token->Type == TokenType::kw_Texture1D      ||
                     Token->Type == TokenType::kw_Texture1DArray ||
                     Token->Type == TokenType::kw_Texture2D      ||
                     Token->Type == TokenType::kw_Texture2DArray ||
                     Token->Type == TokenType::kw_Texture3D      ||
                     Token->Type == TokenType::kw_TextureCube    ||
                     Token->Type == TokenType::kw_TextureCubeArray ||
                     Token->Type == TokenType::kw_Texture2DMS      ||
                     Token->Type == TokenType::kw_Texture2DMSArray ||
                     Token->Type == TokenType::kw_Buffer           ||
                     Token->Type == TokenType::kw_RWTexture1D      ||
                     Token->Type == TokenType::kw_RWTexture1DArray ||
                     Token->Type == TokenType::kw_RWTexture2D      ||
                     Token->Type == TokenType::kw_RWTexture2DArray ||
                     Token->Type == TokenType::kw_RWTexture3D      ||
                     Token->Type == TokenType::kw_RWBuffer)
            // clang-format on
            {
                // Process texture declaration, and add it to the top of the
                // object stack
                ProcessTextureDeclaration(Token, Samplers, m_Objects.back(), SamplerSuffix, ImageBinding);
            }
            else
                ++Token;
        }
    }
    VERIFY_PARSER_STATE(ShaderEntryPointToken, ShaderEntryPointToken != m_Tokens.end(), "Unable to find shader entry point \"", EntryPoint, '\"');

    ProcessShaderDeclaration(ShaderEntryPointToken, ShaderType);

    RemoveSemantics();

    RemoveSpecialShaderAttributes();

    auto GLSLSource = BuildGLSLSource();

    if (m_bPreserveTokens)
    {
        m_Tokens.swap(TokensCopy);
        m_StructDefinitions.clear();
        m_Objects.clear();
    }

    if (IncludeDefintions)
        GLSLSource.insert(0, g_GLSLDefinitions);

    return GLSLSource;
}

} // namespace Diligent
