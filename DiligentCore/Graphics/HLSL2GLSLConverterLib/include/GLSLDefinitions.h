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

#ifndef _GLSL_DEFINITIONS_
#define _GLSL_DEFINITIONS_

#define GLSL

#ifdef GL_ES
// From GLES 3.1 spec:
//    Except for image variables qualified with the format qualifiers r32f, r32i, and r32ui,
//    image variables must specify either memory qualifier readonly or the memory qualifier writeonly.
#   define IMAGE_WRITEONLY writeonly
#else
#   define IMAGE_WRITEONLY
#endif 

#define float4 vec4
#define float3 vec3
#define float2 vec2

#define int4 ivec4
#define int3 ivec3
#define int2 ivec2

#define uint4 uvec4
#define uint3 uvec3
#define uint2 uvec2

#define bool4 bvec4
#define bool3 bvec3
#define bool2 bvec2

// OpenGL matrices in GLSL are always as column-major 
// (this is not related to how they are stored)
#define float2x2 mat2x2
#define float2x3 mat3x2
#define float2x4 mat4x2

#define float3x2 mat2x3
#define float3x3 mat3x3
#define float3x4 mat4x3

#define float4x2 mat2x4
#define float4x3 mat3x4
#define float4x4 mat4x4
#define matrix mat4x4

#define static

#define SamplerState int
#define SamplerComparisonState int

// https://www.opengl.org/wiki/Memory_Model#Incoherent_memory_access
// Shared variable access uses the rules for incoherent memory access. 
// This means that the user must perform certain synchronization in 
// order to ensure that shared variables are visible.
// At the same time, shared variables are all implicitly declared coherent, 
// so one don't need to (and can't) use that qualifier.
#define groupshared shared

#ifdef FRAGMENT_SHADER
#   define ddx dFdx
#   define ddy dFdy
#else
#   define ddx(x) (x) // GLSL compiler fails when it sees derivatives 
#   define ddy(x) (x) // in any shader but fragment
#endif

#define ddx_coarse ddx
#define ddy_coarse ddy
#define ddx_fine ddx
#define ddy_fine ddy

#define mul(a, b) ((a)*(b))
#define frac fract
#define atan2 atan
#define rsqrt inversesqrt
#define fmod mod
#define lerp mix
#define dst distance
#define countbits bitCount
#define firstbithigh findMSB
#define firstbitlow findLSB
#define reversebits bitfieldReverse

float rcp( float x ){ return 1.0 / x; }
vec2  rcp( vec2  x ){ return vec2(1.0,1.0) / x; }
vec3  rcp( vec3  x ){ return vec3(1.0,1.0,1.0) / x; }
vec4  rcp( vec4  x ){ return vec4(1.0,1.0,1.0,1.0) / x; }

float saturate( float x ){ return clamp( x, 0.0,                      1.0 ); }
vec2  saturate( vec2  x ){ return clamp( x, vec2(0.0, 0.0),           vec2(1.0, 1.0) ); }
vec3  saturate( vec3  x ){ return clamp( x, vec3(0.0, 0.0, 0.0),      vec3(1.0, 1.0, 1.0) ); }
vec4  saturate( vec4  x ){ return clamp( x, vec4(0.0, 0.0, 0.0, 0.0), vec4(1.0, 1.0, 1.0, 1.0) ); }

void sincos( float x, out float s, out float c ){ s = sin( x ); c = cos( x ); }
void sincos( vec2  x, out vec2  s, out vec2  c ){ s = sin( x ); c = cos( x ); }
void sincos( vec3  x, out vec3  s, out vec3  c ){ s = sin( x ); c = cos( x ); }
void sincos( vec4  x, out vec4  s, out vec4  c ){ s = sin( x ); c = cos( x ); }


// Bit conversion operations

float asfloat( float x ){ return x; }
vec2  asfloat( vec2  x ){ return x; }
vec3  asfloat( vec3  x ){ return x; }
vec4  asfloat( vec4  x ){ return x; }

float asfloat( int   x ){ return intBitsToFloat(x); }
vec2  asfloat( ivec2 x ){ return intBitsToFloat(x); }
vec3  asfloat( ivec3 x ){ return intBitsToFloat(x); }
vec4  asfloat( ivec4 x ){ return intBitsToFloat(x); }

float asfloat( uint  x ){ return uintBitsToFloat(x); }
vec2  asfloat( uvec2 x ){ return uintBitsToFloat(x); }
vec3  asfloat( uvec3 x ){ return uintBitsToFloat(x); }
vec4  asfloat( uvec4 x ){ return uintBitsToFloat(x); }


int   asint( int   x ){ return x; }
ivec2 asint( ivec2 x ){ return x; }
ivec3 asint( ivec3 x ){ return x; }
ivec4 asint( ivec4 x ){ return x; }

int   asint( uint  x ){ return int(x);   }
ivec2 asint( uvec2 x ){ return ivec2(x); }
ivec3 asint( uvec3 x ){ return ivec3(x); }
ivec4 asint( uvec4 x ){ return ivec4(x); }

int   asint( float x ){ return floatBitsToInt(x); }
ivec2 asint( vec2  x ){ return floatBitsToInt(x); }
ivec3 asint( vec3  x ){ return floatBitsToInt(x); }
ivec4 asint( vec4  x ){ return floatBitsToInt(x); }


uint  asuint( uint  x ){ return x; }
uvec2 asuint( uvec2 x ){ return x; }
uvec3 asuint( uvec3 x ){ return x; }
uvec4 asuint( uvec4 x ){ return x; }

uint  asuint( int  x  ){ return  uint(x); }
uvec2 asuint( ivec2 x ){ return uvec2(x); }
uvec3 asuint( ivec3 x ){ return uvec3(x); }
uvec4 asuint( ivec4 x ){ return uvec4(x); }

uint  asuint( float x ){ return floatBitsToUint(x); }
uvec2 asuint( vec2  x ){ return floatBitsToUint(x); }
uvec3 asuint( vec3  x ){ return floatBitsToUint(x); }
uvec4 asuint( vec4  x ){ return floatBitsToUint(x); }

#if defined(GL_ES) && (__VERSION__>=310) || !defined(GL_ES) && (__VERSION__>=420)
float f16tof32( uint u1 )
{
    return unpackHalf2x16( u1 ).x;
}
vec2 f16tof32( uvec2 u2 )
{ 
    uint u2PackedHalf = (u2.x & 0x0ffffu) | ((u2.y & 0x0ffffu) << 16u);
    return unpackHalf2x16( u2PackedHalf ); 
}
vec3 f16tof32( uvec3 u3 )
{ 
    return vec3( f16tof32( u3.xy ), f16tof32( u3.z ) );
}
vec4 f16tof32( uvec4 u4 )
{ 
    return vec4( f16tof32( u4.xy ), f16tof32( u4.zw ) );
}
float f16tof32( int   i1 ){ return f16tof32( uint ( i1 ) ); }
vec2  f16tof32( ivec2 i2 ){ return f16tof32( uvec2( i2 ) ); }
vec3  f16tof32( ivec3 i3 ){ return f16tof32( uvec3( i3 ) ); }
vec4  f16tof32( ivec4 i4 ){ return f16tof32( uvec4( i4 ) ); }

uint f32tof16( float f )
{ 
    return packHalf2x16( vec2( f, 0.0 ) ) & 0x0ffffu;
}
uvec2 f32tof16( vec2 f2 )
{ 
    uint u2PackedHalf = packHalf2x16( f2 );
    return uvec2( u2PackedHalf & 0x0ffffu, u2PackedHalf >> 16u );
}
uvec3 f32tof16( vec3 f3 )
{
    return uvec3( f32tof16( f3.xy ), f32tof16( f3.z ) );
}
uvec4 f32tof16( vec4 f4 )
{
    return uvec4( f32tof16( f4.xy ), f32tof16( f4.zw ) );
}
#endif

// Use #define as double is not supported on GLES
#define asdouble(lowbits, highbits) packDouble2x32( uvec2( lowbits, highbits ) )

// Floating point functions

bool isfinite( float x )
{
    return !isinf( x ) && !isnan( x );
}

bool2 isfinite( vec2 f2 )
{
    return bool2( isfinite( f2.x ), isfinite( f2.y ) );
}

bool3 isfinite( vec3 f3 )
{
    return bool3( isfinite( f3.xy ), isfinite( f3.z ) );
}

bool4 isfinite( vec4 f4 )
{
    return bool4( isfinite( f4.xyz ), isfinite( f4.w ) );
}

float log10( float x )
{
    return log( x ) / log( 10.0 );
}
vec2 log10( vec2 x )
{
    float _lg10 = log( 10.0 );
    return log( x ) / vec2(_lg10, _lg10);
}
vec3 log10( vec3 x )
{
    float _lg10 = log( 10.0 );
    return log( x ) / vec3(_lg10, _lg10, _lg10);
}
vec4 log10( vec4 x )
{
    float _lg10 = log( 10.0 );
    return log( x ) / vec4(_lg10, _lg10, _lg10, _lg10);
}


#ifdef GL_ES
#   define mad(a,b,c) ((a)*(b)+(c))
#else
#   define mad fma
#endif


// Relational and logical operators
#define Less lessThan
#define LessEqual lessThanEqual
#define Greater greaterThan
#define GreaterEqual greaterThanEqual
#define Equal equal
#define NotEqual notEqual
#define Not not
bool4 And(bool4 L, bool4 R)
{
    return bool4(L.x && R.x,
                 L.y && R.y,
                 L.z && R.z,
                 L.w && R.w);
}
bool3 And(bool3 L, bool3 R)
{
    return bool3(L.x && R.x,
                 L.y && R.y,
                 L.z && R.z);
}
bool2 And(bool2 L, bool2 R)
{
    return bool2(L.x && R.x,
                 L.y && R.y);
}
bool And(bool L, bool R)
{
    return (L && R);
}


bool4 Or(bool4 L, bool4 R)
{
    return bool4(L.x || R.x,
                 L.y || R.y,
                 L.z || R.z,
                 L.w || R.w);
}
bool3 Or(bool3 L, bool3 R)
{
    return bool3(L.x || R.x,
                 L.y || R.y,
                 L.z || R.z);
}
bool2 Or(bool2 L, bool2 R)
{
    return bool2(L.x || R.x,
                 L.y || R.y);
}
bool Or(bool L, bool R)
{
    return (L || R);
}

float4 BoolToFloat( bool4 b4 )
{
    return float4(b4.x ? 1.0 : 0.0,
                  b4.y ? 1.0 : 0.0,
                  b4.z ? 1.0 : 0.0,
                  b4.w ? 1.0 : 0.0);
}
float3 BoolToFloat( bool3 b3 )
{
    return float3(b3.x ? 1.0 : 0.0,
                  b3.y ? 1.0 : 0.0,
                  b3.z ? 1.0 : 0.0);
}
float2 BoolToFloat( bool2 b2 )
{
    return float2(b2.x ? 1.0 : 0.0,
                  b2.y ? 1.0 : 0.0);
}
float BoolToFloat( bool b )
{
    return b ? 1.0 : 0.0;
}


// Synchronization functions

#ifdef COMPUTE_SHADER

// https://www.opengl.org/wiki/Memory_Model#Incoherent_memory_access

// MSDN: GroupMemoryBarrier() blocks execution of all threads 
// in a group until all group SHARED accesses have been completed.
void GroupMemoryBarrier()
{
    // OpenGL.org: groupMemoryBarrier() waits on the completion of all memory accesses 
    // performed by an invocation of a compute shader relative to the same access performed 
    // by other invocations in the same work group and then returns with no other effect.

    // groupMemoryBarrier() acts like memoryBarrier(), ordering memory writes for all kinds 
    // of variables, but it only orders read/writes for the current work group.
    groupMemoryBarrier();

    // OpenGL.org: memoryBarrierShared() waits on the completion of 
    // all memory accesses resulting from the use of SHARED variables
    // and then returns with no other effect. 
    memoryBarrierShared();
}

// MSDN: GroupMemoryBarrierWithGroupSync() blocks execution of all 
// threads in a group until all memory accesses have been completed 
// and all threads in the group have reached this call.
void GroupMemoryBarrierWithGroupSync()
{
    // Issue memory barrier first!
    GroupMemoryBarrier();
    barrier();
}

// MSDN: DeviceMemoryBarrier() blocks execution of all threads 
// in a group until all device memory accesses have been completed.
void DeviceMemoryBarrier()
{
    // Call all memory barriers except for shared memory
    
    // Do we need to call groupMemoryBarrier() ????? 

    // OpenGL.org: memoryBarrierBuffer() waits on the completion of 
    // all memory accesses resulting from the use of BUFFER variables 
    // and then returns with no other effect
    memoryBarrierBuffer();

    // OpenGL.org: memoryBarrierImage() waits on the completion of all 
    // memory accesses resulting from the use of IMAGE variables and then 
    // returns with no other effect. 
    memoryBarrierImage();

    // OpenGL.org: memoryBarrierAtomicCounter() waits on the completion of 
    // all accesses resulting from the use of ATOMIC COUNTERS and then returns 
    // with no other effect. 
    memoryBarrierAtomicCounter();
}

// MSDN: DeviceMemoryBarrierWithGroupSync() blocks execution of 
// all threads in a group until all device memory accesses have 
// been completed and all threads in the group have reached this call.
void DeviceMemoryBarrierWithGroupSync()
{
    DeviceMemoryBarrier();
    barrier();
}

// MSDN: AllMemoryBarrier() blocks execution of all threads in a 
// group until all memory accesses have been completed.
void AllMemoryBarrier()
{
    // OpenGL.org: memoryBarrier() waits on the completion of ALL 
    // memory accesses resulting from the use of IMAGE variables or 
    // ATOMIC COUNTERS and then returns with no other effect.
    memoryBarrier();
    // NOTE: nothing is said about buffer memory and shared memory,
    // so call memoryBarrierBuffer() and memoryBarrierShared() for safety

    // OpenGL.org: memoryBarrierBuffer() waits on the completion of 
    // all memory accesses resulting from the use of BUFFER variables 
    // and then returns with no other effect
    memoryBarrierBuffer();

    // OpenGL.org: memoryBarrierShared() waits on the completion of 
    // all memory accesses resulting from the use of SHARED variables
    // and then returns with no other effect. 
    memoryBarrierShared();

    // Call all memory barrier functions. They should have no effect
    // if everything is synchronized.
    
    // OpenGL.org: memoryBarrierImage() waits on the completion of all 
    // memory accesses resulting from the use of IMAGE variables and then 
    // returns with no other effect. 
    memoryBarrierImage();

    // OpenGL.org: memoryBarrierAtomicCounter() waits on the completion of 
    // all accesses resulting from the use of ATOMIC COUNTERS and then returns 
    // with no other effect. 
    memoryBarrierAtomicCounter();

    // groupMemoryBarrier waits on the completion of all memory accesses performed 
    // by an invocation of a compute shader relative to the same access performed by 
    // other invocations in the same work group and then returns with no other effect.
    groupMemoryBarrier();
}

// MSDN: AllMemoryBarrierWithGroupSync() blocks execution of all 
// threads in a group until all memory accesses have been completed 
// and all threads in the group have reached this call.
void AllMemoryBarrierWithGroupSync()
{
    AllMemoryBarrier();
    barrier();
}

#else

void AllMemoryBarrier(){}
void AllMemoryBarrierWithGroupSync(){}
void DeviceMemoryBarrier(){}
void DeviceMemoryBarrierWithGroupSync(){}
void GroupMemoryBarrier(){}
void GroupMemoryBarrierWithGroupSync(){}

#endif


// Type conversion functions

vec4 _ExpandVector( float x ){ return vec4(    x,    x,    x,    x ); }
vec4 _ExpandVector( vec2 f2 ){ return vec4( f2.x, f2.y,  0.0,  0.0 ); }
vec4 _ExpandVector( vec3 f3 ){ return vec4( f3.x, f3.y, f3.z,  0.0 ); }
vec4 _ExpandVector( vec4 f4 ){ return vec4( f4.x, f4.y, f4.z, f4.w ); }

ivec4 _ExpandVector( int    x ){ return ivec4(    x,    x,    x,    x ); }
ivec4 _ExpandVector( ivec2 i2 ){ return ivec4( i2.x, i2.y,    0,    0 ); }
ivec4 _ExpandVector( ivec3 i3 ){ return ivec4( i3.x, i3.y, i3.z,    0 ); }
ivec4 _ExpandVector( ivec4 i4 ){ return ivec4( i4.x, i4.y, i4.z, i4.w ); }

uvec4 _ExpandVector( uint   x ){ return uvec4(    x,    x,    x,    x ); }
uvec4 _ExpandVector( uvec2 u2 ){ return uvec4( u2.x, u2.y,   0u,   0u ); }
uvec4 _ExpandVector( uvec3 u3 ){ return uvec4( u3.x, u3.y, u3.z,   0u ); }
uvec4 _ExpandVector( uvec4 u4 ){ return uvec4( u4.x, u4.y, u4.z, u4.w ); }

bvec4 _ExpandVector( bool   x ){ return bvec4(    x,    x,     x,     x ); }
bvec4 _ExpandVector( bvec2 b2 ){ return bvec4( b2.x, b2.y, false, false ); }
bvec4 _ExpandVector( bvec3 b3 ){ return bvec4( b3.x, b3.y,  b3.z, false ); }
bvec4 _ExpandVector( bvec4 b4 ){ return bvec4( b4.x, b4.y,  b4.z,  b4.w ); }

void _ResizeVector(out vec4 outVec4, in vec4 inVec4){outVec4 = inVec4;}
void _ResizeVector(out vec3 outVec3, in vec4 inVec4){outVec3 = inVec4.xyz;}
void _ResizeVector(out vec2 outVec2, in vec4 inVec4){outVec2 = inVec4.xy;}
void _ResizeVector(out float outFlt, in vec4 inVec4){outFlt  = inVec4.x;}

void _ResizeVector(out vec4 outVec4, in vec3 inVec3){outVec4 = vec4(inVec3, 0.0);}
void _ResizeVector(out vec3 outVec3, in vec3 inVec3){outVec3 = inVec3;}
void _ResizeVector(out vec2 outVec2, in vec3 inVec3){outVec2 = inVec3.xy;}
void _ResizeVector(out float outFlt, in vec3 inVec3){outFlt  = inVec3.x;}

void _ResizeVector(out vec4 outVec4, in vec2 inVec2){outVec4 = vec4(inVec2, 0.0, 0.0);}
void _ResizeVector(out vec3 outVec3, in vec2 inVec2){outVec3 = vec3(inVec2, 0.0);}
void _ResizeVector(out vec2 outVec2, in vec2 inVec2){outVec2 = inVec2;}
void _ResizeVector(out float outFlt, in vec2 inVec2){outFlt  = inVec2.x;}

void _ResizeVector(out vec4 outVec4, in float v){outVec4 = vec4(v, 0.0, 0.0, 0.0);}
void _ResizeVector(out vec3 outVec3, in float v){outVec3 = vec3(v, 0.0, 0.0);}
void _ResizeVector(out vec2 outVec2, in float v){outVec2 = vec2(v, 0.0);}
void _ResizeVector(out float outFlt, in float v){outFlt  = v;}


void _TypeConvertStore( out float Dst, in int   Src ){ Dst = float( Src );    }
void _TypeConvertStore( out float Dst, in uint  Src ){ Dst = float( Src );    }
void _TypeConvertStore( out float Dst, in float Src ){ Dst = float( Src );    }
void _TypeConvertStore( out float Dst, in bool  Src ){ Dst = Src ? 1.0 : 0.0; }

void _TypeConvertStore( out uint  Dst, in int   Src ){ Dst = uint( Src );   }
void _TypeConvertStore( out uint  Dst, in uint  Src ){ Dst = uint( Src );   }
void _TypeConvertStore( out uint  Dst, in float Src ){ Dst = uint( Src );   }
void _TypeConvertStore( out uint  Dst, in bool  Src ){ Dst = Src ? 1u : 0u; }

void _TypeConvertStore( out int   Dst, in int   Src ){ Dst = int( Src );  }
void _TypeConvertStore( out int   Dst, in uint  Src ){ Dst = int( Src );  }
void _TypeConvertStore( out int   Dst, in float Src ){ Dst = int( Src );  }
void _TypeConvertStore( out int   Dst, in bool  Src ){ Dst = Src ? 1 : 0; }

void _TypeConvertStore( out bool  Dst, in int   Src ){ Dst = (Src != 0);   }
void _TypeConvertStore( out bool  Dst, in uint  Src ){ Dst = (Src != 0u);  }
void _TypeConvertStore( out bool  Dst, in float Src ){ Dst = (Src != 0.0); }
void _TypeConvertStore( out bool  Dst, in bool  Src ){ Dst = Src;          }


int _ToInt( int x )    { return int(x);     }
int _ToInt( ivec2 v )  { return int(v.x);   }
int _ToInt( ivec3 v )  { return int(v.x);   }
int _ToInt( ivec4 v )  { return int(v.x);   }

int _ToInt( uint x )   { return int(x);     }
int _ToInt( uvec2 v )  { return int(v.x);   }
int _ToInt( uvec3 v )  { return int(v.x);   }
int _ToInt( uvec4 v )  { return int(v.x);   }

int _ToInt( float x )  { return int(x);     }
int _ToInt( vec2 v )   { return int(v.x);   }
int _ToInt( vec3 v )   { return int(v.x);   }
int _ToInt( vec4 v )   { return int(v.x);   }

int _ToInt( bool x )   { return x   ? 1 : 0;}
int _ToInt( bvec2 v )  { return v.x ? 1 : 0;}
int _ToInt( bvec3 v )  { return v.x ? 1 : 0;}
int _ToInt( bvec4 v )  { return v.x ? 1 : 0;}



float _ToFloat( int x )  { return float(x);     }
float _ToFloat( ivec2 v ){ return float(v.x);   }
float _ToFloat( ivec3 v ){ return float(v.x);   }
float _ToFloat( ivec4 v ){ return float(v.x);   }

float _ToFloat( uint x ) { return float(x);     }
float _ToFloat( uvec2 v ){ return float(v.x);   }
float _ToFloat( uvec3 v ){ return float(v.x);   }
float _ToFloat( uvec4 v ){ return float(v.x);   }

float _ToFloat( float x ){ return float(x);     }
float _ToFloat( vec2 v ) { return float(v.x);   }
float _ToFloat( vec3 v ) { return float(v.x);   }
float _ToFloat( vec4 v ) { return float(v.x);   }

float _ToFloat( bool x ) { return x   ? 1.0 : 0.0;}
float _ToFloat( bvec2 v ){ return v.x ? 1.0 : 0.0;}
float _ToFloat( bvec3 v ){ return v.x ? 1.0 : 0.0;}
float _ToFloat( bvec4 v ){ return v.x ? 1.0 : 0.0;}



uint _ToUint( int x )  { return uint(x);     }
uint _ToUint( uint x ) { return uint(x);     }
uint _ToUint( float x ){ return uint(x);     }
uint _ToUint( bool x ) { return x ? 1u : 0u; }

bool _ToBool( int x )  { return x != 0   ? true : false; }
bool _ToBool( uint x ) { return x != 0u  ? true : false; }
bool _ToBool( float x ){ return x != 0.0 ? true : false; }
bool _ToBool( bool x ) { return x; }

#define _ToVec2(x,y)     vec2(_ToFloat(x), _ToFloat(y))
#define _ToVec3(x,y,z)   vec3(_ToFloat(x), _ToFloat(y), _ToFloat(z))
#define _ToVec4(x,y,z,w) vec4(_ToFloat(x), _ToFloat(y), _ToFloat(z), _ToFloat(w))

#define _ToIvec2(x,y)     ivec2(_ToInt(x), _ToInt(y))
#define _ToIvec3(x,y,z)   ivec3(_ToInt(x), _ToInt(y), _ToInt(z))
#define _ToIvec4(x,y,z,w) ivec4(_ToInt(x), _ToInt(y), _ToInt(z), _ToInt(w))

#define _ToUvec2(x,y)     uvec2(_ToUint(x), _ToUint(y))
#define _ToUvec3(x,y,z)   uvec3(_ToUint(x), _ToUint(y), _ToUint(z))
#define _ToUvec4(x,y,z,w) uvec4(_ToUint(x), _ToUint(y), _ToUint(z), _ToUint(w))

#define _ToBvec2(x,y)     bvec2(_ToBool(x), _ToBool(y))
#define _ToBvec3(x,y,z)   bvec3(_ToBool(x), _ToBool(y), _ToBool(z))
#define _ToBvec4(x,y,z,w) bvec4(_ToBool(x), _ToBool(y), _ToBool(z), _ToBool(w))


int   _ToIvec( uint  u1 ){ return _ToInt(   u1 ); }
ivec2 _ToIvec( uvec2 u2 ){ return _ToIvec2( u2.x, u2.y ); }
ivec3 _ToIvec( uvec3 u3 ){ return _ToIvec3( u3.x, u3.y, u3.z ); }
ivec4 _ToIvec( uvec4 u4 ){ return _ToIvec4( u4.x, u4.y, u4.z, u4.w ); }

int   _ToIvec( int   i1 ){ return i1; }
ivec2 _ToIvec( ivec2 i2 ){ return i2; }
ivec3 _ToIvec( ivec3 i3 ){ return i3; }
ivec4 _ToIvec( ivec4 i4 ){ return i4; }

int   _ToIvec( float f1 ){ return _ToInt(   f1 ); }
ivec2 _ToIvec( vec2  f2 ){ return _ToIvec2( f2.x, f2.y ); }
ivec3 _ToIvec( vec3  f3 ){ return _ToIvec3( f3.x, f3.y, f3.z ); }
ivec4 _ToIvec( vec4  f4 ){ return _ToIvec4( f4.x, f4.y, f4.z, f4.w ); }


float _ToVec( uint  u1 ){ return _ToFloat(u1); }
vec2  _ToVec( uvec2 u2 ){ return _ToVec2( u2.x, u2.y ); }
vec3  _ToVec( uvec3 u3 ){ return _ToVec3( u3.x, u3.y, u3.z ); }
vec4  _ToVec( uvec4 u4 ){ return _ToVec4( u4.x, u4.y, u4.z, u4.w ); }
         
float _ToVec( int   i1 ){ return _ToFloat(i1); }
vec2  _ToVec( ivec2 i2 ){ return _ToVec2( i2.x, i2.y ); }
vec3  _ToVec( ivec3 i3 ){ return _ToVec3( i3.x, i3.y, i3.z ); }
vec4  _ToVec( ivec4 i4 ){ return _ToVec4( i4.x, i4.y, i4.z, i4.w ); }
         
float _ToVec( float f1 ){ return f1; }
vec2  _ToVec( vec2  f2 ){ return f2; }
vec3  _ToVec( vec3  f3 ){ return f3; }
vec4  _ToVec( vec4  f4 ){ return f4; }


uint   _ToUvec( uint  u1 ){ return u1; }
uvec2  _ToUvec( uvec2 u2 ){ return u2; }
uvec3  _ToUvec( uvec3 u3 ){ return u3; }
uvec4  _ToUvec( uvec4 u4 ){ return u4; }
         
uint   _ToUvec( int   i1 ){ return _ToUint(  i1 ); }
uvec2  _ToUvec( ivec2 i2 ){ return _ToUvec2( i2.x, i2.y ); }
uvec3  _ToUvec( ivec3 i3 ){ return _ToUvec3( i3.x, i3.y, i3.z ); }
uvec4  _ToUvec( ivec4 i4 ){ return _ToUvec4( i4.x, i4.y, i4.z, i4.w ); }
         
uint   _ToUvec( float f1 ){ return _ToUint(  f1 ); }
uvec2  _ToUvec( vec2  f2 ){ return _ToUvec2( f2.x, f2.y ); }
uvec3  _ToUvec( vec3  f3 ){ return _ToUvec3( f3.x, f3.y, f3.z ); }
uvec4  _ToUvec( vec4  f4 ){ return _ToUvec4( f4.x, f4.y, f4.z, f4.w ); }

// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/frexp.xhtml
// https://www.khronos.org/registry/OpenGL-Refpages/es3.1/html/frexp.xhtml
#if defined(GL_ES) && (__VERSION__>=310) || !defined(GL_ES) && (__VERSION__>=400)
// We have to redefine 'float frexp(float, int)' as 'float frexp(float, float)'
float _frexp(float f1, out float fexp1)
{
    int iexp1;
    float sig1 = frexp(f1, iexp1);
    fexp1 = float(iexp1);
    return sig1;
}
vec2 _frexp(vec2 f2, out vec2 fexp2)
{
    ivec2 iexp2;
    vec2 sig2 = frexp(f2, iexp2);
    fexp2 = vec2(iexp2);
    return sig2;
}
vec3 _frexp(vec3 f3, out vec3 fexp3)
{
    ivec3 iexp3;
    vec3 sig3 = frexp(f3, iexp3);
    fexp3 = vec3(iexp3);
    return sig3;
}
vec4 _frexp(vec4 f4, out vec4 fexp4)
{
    ivec4 iexp4;
    vec4 sig4 = frexp(f4, iexp4);
    fexp4 = vec4(iexp4);
    return sig4;
}
#define frexp _frexp
#endif


// TEXTURE FUNCTION STUB MACROS
// https://www.opengl.org/wiki/Sampler_(GLSL)


// Texture size queries
// https://www.opengl.org/sdk/docs/man/html/textureSize.xhtml
// textureSize returns the dimensions of level lod (if present) of the texture bound to sampler. 
// The components in the return value are filled in, in order, with the width, height and depth 
// of the texture. For the array forms, the last component of the return value is the number of 
// layers in the texture array.

//#if !(defined(DESKTOP_GL) && __VERSION__ >= 430)
#   define textureQueryLevels(x) 0 // Only supported on 4.3+
//#endif

#define GetTex1DDimensions_1(Sampler, Width)\
{                                                       \
    _TypeConvertStore( Width, textureSize(Sampler, 0) );\
}

#define GetTex1DDimensions_3(Sampler, MipLevel, Width, NumberOfMipLevels)\
{                                                                        \
    _TypeConvertStore( Width, textureSize(Sampler, _ToInt(MipLevel)) );  \
    _TypeConvertStore( NumberOfMipLevels, textureQueryLevels(Sampler) ); \
}

#define GetTex1DArrDimensions_2(Sampler, Width, Elements)\
{                                           \
    ivec2 i2Size = textureSize(Sampler, 0); \
    _TypeConvertStore( Width,    i2Size.x );\
    _TypeConvertStore( Elements, i2Size.y );\
}

#define GetTex1DArrDimensions_4(Sampler, MipLevel, Width, Elements, NumberOfMipLevels)\
{                                                           \
    ivec2 i2Size = textureSize(Sampler, _ToInt(MipLevel));  \
    _TypeConvertStore( Width,    i2Size.x );                \
    _TypeConvertStore( Elements, i2Size.y );                \
    _TypeConvertStore( NumberOfMipLevels, textureQueryLevels(Sampler) );\
}

#define GetTex2DDimensions_2(Sampler, Width, Height)\
{                                                   \
    ivec2 i2Size = textureSize(Sampler, 0);         \
    _TypeConvertStore( Width,  i2Size.x );          \
    _TypeConvertStore( Height, i2Size.y );          \
}

#define GetTex2DDimensions_4(Sampler, MipLevel, Width, Height, NumberOfMipLevels)\
{                                                           \
    ivec2 i2Size = textureSize(Sampler, _ToInt(MipLevel) ); \
    _TypeConvertStore( Width,  i2Size.x );                              \
    _TypeConvertStore( Height, i2Size.y );                              \
    _TypeConvertStore( NumberOfMipLevels, textureQueryLevels(Sampler) );\
}

#define GetTex2DArrDimensions_3(Sampler, Width, Height, Elements)\
{                                           \
    ivec3 i3Size = textureSize(Sampler, 0); \
    _TypeConvertStore( Width,   i3Size.x ); \
    _TypeConvertStore( Height,  i3Size.y ); \
    _TypeConvertStore( Elements,i3Size.z ); \
}

#define GetTex2DArrDimensions_5(Sampler, MipLevel, Width, Height, Elements, NumberOfMipLevels)\
{                                                           \
    ivec3 i3Size = textureSize(Sampler, _ToInt(MipLevel));  \
    _TypeConvertStore( Width,    i3Size.x );                            \
    _TypeConvertStore( Height,   i3Size.y );                            \
    _TypeConvertStore( Elements, i3Size.z );                            \
    _TypeConvertStore( NumberOfMipLevels, textureQueryLevels(Sampler) );\
}

#define GetTex3DDimensions_3(Sampler, Width, Height, Depth)\
{                                           \
    ivec3 i3Size = textureSize(Sampler, 0); \
    _TypeConvertStore( Width,  i3Size.x );  \
    _TypeConvertStore( Height, i3Size.y );  \
    _TypeConvertStore( Depth,  i3Size.z );  \
}

#define GetTex3DDimensions_5(Sampler, MipLevel, Width, Height, Depth, NumberOfMipLevels)\
{                                                           \
    ivec3 i3Size = textureSize(Sampler, _ToInt(MipLevel));  \
    _TypeConvertStore( Width,  i3Size.x );                              \
    _TypeConvertStore( Height, i3Size.y );                              \
    _TypeConvertStore( Depth,  i3Size.z );                              \
    _TypeConvertStore( NumberOfMipLevels, textureQueryLevels(Sampler) );\
}

#define GetTex2DMSDimensions_3(Sampler, Width, Height, NumberOfSamples)\
{                                           \
    ivec2 i2Size = textureSize(Sampler);    \
    _TypeConvertStore( Width,  i2Size.x );  \
    _TypeConvertStore( Height, i2Size.y );  \
    _TypeConvertStore( NumberOfSamples, 0 );\
}

#define GetTex2DMSArrDimensions_4(Sampler, Width, Height, Elements, NumberOfSamples)\
{                                           \
    ivec3 i3Size = textureSize(Sampler);    \
    _TypeConvertStore( Width,    i3Size.x );\
    _TypeConvertStore( Height,   i3Size.y );\
    _TypeConvertStore( Elements, i3Size.z );\
    _TypeConvertStore( NumberOfSamples, 0 );\
}

#define GetTexBufferDimensions_1(Sampler, Width)\
{                                                    \
    _TypeConvertStore( Width, textureSize(Sampler) );\
}


// https://www.opengl.org/sdk/docs/man/html/imageSize.xhtml
// imageSize returns the dimensions of the image bound to image. The components in the 
// return value are filled in, in order, with the width, height and depth of the image. 
// For the array forms, the last component of the return value is the number of layers 
// in the texture array.

#define GetRWTex1DDimensions_1(Tex, Width)\
{                                               \
    _TypeConvertStore( Width, imageSize(Tex) ); \
}

#define GetRWTex1DArrDimensions_2(Tex, Width, Elements)\
{                                   \
    ivec2 i2Size = imageSize(Tex);  \
    _TypeConvertStore( Width,    i2Size.x ); \
    _TypeConvertStore( Elements, i2Size.y ); \
}

#define GetRWTex2DDimensions_2(Tex, Width, Height)\
{                                           \
    ivec2 i2Size = imageSize(Tex);          \
    _TypeConvertStore( Width,  i2Size.x );  \
    _TypeConvertStore( Height, i2Size.y );  \
}

#define GetRWTex2DArrDimensions_3(Tex, Width, Height, Elements)\
{                                           \
    ivec3 i3Size = imageSize(Tex);          \
    _TypeConvertStore( Width,    i3Size.x );\
    _TypeConvertStore( Height,   i3Size.y );\
    _TypeConvertStore( Elements, i3Size.z );\
}

#define GetRWTex3DDimensions_3(Tex, Width, Height, Depth)\
{                                           \
    ivec3 i3Size = imageSize(Tex);          \
    _TypeConvertStore( Width,  i3Size.x );  \
    _TypeConvertStore( Height, i3Size.y );  \
    _TypeConvertStore( Depth,  i3Size.z );  \
}

#define GetRWTexBufferDimensions_1(Tex, Width)\
{                                               \
    _TypeConvertStore( Width, imageSize(Tex) ); \
}


// Texture sampling operations


//                             IMPORTANT NOTE ABOUT OFFSET
// Offset parameter to all texture sampling functions must be a constant expression.
// If it is not, the shader will be successfully compiled, HOWEVER the value of Offset 
// will silently be zero. 
//
// A constant expression in GLSL is defined as follows:
// * A literal value.
// * A const-qualified variable with an explicit initializer (so not a function parameter).
// * The result of the length() function of an array, but only if the array has an explicit size.
// * The result of most operators, so long as all the operands are themselves constant expressions. 
//   The operators not on this list are any assignment operators (+= and so forth), and the comma operator.
// * The result of a constructor for a type, but only if all of the arguments to the constructor are 
//   themselves constant expressions.
// * The return value of any built-in function, but only if all of the arguments to the function are 
//   themselves constant expressions. Opaque Types are never constant expressions. Note that the 
//   functions dFdx, dFdy, and fwidth will return 0, when used in a context that requires a constant 
//   expression (such as a const variable initializer).
// 
// The list above does not include return value of a function, even when the value is compile-time expression.
// As a result, we cannot use type conversion functions for Offset parameter.

// In all texture sampling functions, the last component of Coords is used as Dsub and the array layer is specified 
// in the second to last component of Coords. (The second component of Coords is unused for 1D shadow lookups.)
// For cube array textures, Dsub is specified as a separate parameter
//                                                                                                                                     mip
#define SampleCmpLevel0Tex1D_3(Tex, Sampler, Coords, CompareValue)      textureLod(Tex, _ToVec3( Coords,           0.0, CompareValue), 0.0)
#define SampleCmpLevel0Tex1DArr_3(Tex, Sampler, Coords, CompareValue)   textureLod(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue), 0.0)
#define SampleCmpLevel0Tex2D_3(Tex, Sampler, Coords, CompareValue)      textureLod(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue), 0.0)
#define SampleCmpLevel0Tex2DArr_3(Tex, Sampler, Coords, CompareValue)   0.0 // No textureLod for sampler2DArrayShadow
#define SampleCmpLevel0TexCube_3(Tex, Sampler, Coords, CompareValue)    0.0 // No textureLod for samplerCubeShadow
#define SampleCmpLevel0TexCubeArr_3(Tex, Sampler, Coords, CompareValue) 0.0 // No textureLod for samplerCubeArrayShadow

//                                                                                                                                                 mip
#define SampleCmpLevel0Tex1D_4(Tex, Sampler, Coords, CompareValue, Offset)    textureLodOffset(Tex, _ToVec3( Coords,           0.0, CompareValue), 0.0, int(Offset))
#define SampleCmpLevel0Tex1DArr_4(Tex, Sampler, Coords, CompareValue, Offset) textureLodOffset(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue), 0.0, int(Offset))
#define SampleCmpLevel0Tex2D_4(Tex, Sampler, Coords, CompareValue, Offset)    textureLodOffset(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue), 0.0, ivec2((Offset).xy))
#define SampleCmpLevel0Tex2DArr_4(Tex, Sampler, Coords, CompareValue, Offset) 0.0 // No textureLodOffset for sampler2DArrayShadow


// https://www.opengl.org/sdk/docs/man/html/texture.xhtml - note: there are many mistakes on the page
#ifdef FRAGMENT_SHADER

#   define Sample_2(Tex, Sampler, Coords)         texture      (Tex, _ToVec(Coords))
#   define Sample_3(Tex, Sampler, Coords, Offset) textureOffset(Tex, _ToVec(Coords), Offset)
#   define SampleBias_3(Tex, Sampler, Coords, Bias)         texture      (Tex, _ToVec(Coords), _ToFloat(Bias))
#   define SampleBias_4(Tex, Sampler, Coords, Bias, Offset) textureOffset(Tex, _ToVec(Coords), Offset, _ToFloat(Bias))

#   define SampleCmpTex1D_3(Tex, Sampler, Coords, CompareValue)      texture(Tex, _ToVec3( Coords,           0.0, CompareValue))
#   define SampleCmpTex1DArr_3(Tex, Sampler, Coords, CompareValue)   texture(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue))
#   define SampleCmpTex2D_3(Tex, Sampler, Coords, CompareValue)      texture(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue))
#   define SampleCmpTex2DArr_3(Tex, Sampler, Coords, CompareValue)   texture(Tex, _ToVec4((Coords).x, (Coords).y, (Coords).z, CompareValue))
#   define SampleCmpTexCube_3(Tex, Sampler, Coords, CompareValue)    texture(Tex, _ToVec4((Coords).x, (Coords).y, (Coords).z, CompareValue))
#   define SampleCmpTexCubeArr_3(Tex, Sampler, Coords, CompareValue) texture(Tex, _ToVec4((Coords).x, (Coords).y, (Coords).z, (Coords).w), _ToFloat(CompareValue))

#   define SampleCmpTex1D_4(Tex, Sampler, Coords, CompareValue, Offset)    textureOffset(Tex, _ToVec3( Coords,           0.0, CompareValue), int(Offset))
#   define SampleCmpTex1DArr_4(Tex, Sampler, Coords, CompareValue, Offset) textureOffset(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue), int(Offset))
#   define SampleCmpTex2D_4(Tex, Sampler, Coords, CompareValue, Offset)    textureOffset(Tex, _ToVec3((Coords).x, (Coords).y, CompareValue), ivec2((Offset).xy))
#   define SampleCmpTex2DArr_4(Tex, Sampler, Coords, CompareValue, Offset) textureOffset(Tex, _ToVec4((Coords).x, (Coords).y, (Coords).z, CompareValue), ivec2((Offset).xy))

#else

// Derivatives are only available in fragment shader. GLSL compiler fails when it
// encounters texture() or textureOffset() instructions in other types of shaders. So
// to let the shader be compiled and to have something meaningful, replace such operations
// with textureLod() and textureLodOffset()

#   define Sample_2(Tex, Sampler, Coords)         textureLod      (Tex, _ToVec(Coords), 0.0)
#   define Sample_3(Tex, Sampler, Coords, Offset) textureLodOffset(Tex, _ToVec(Coords), 0.0, Offset)
#   define SampleBias_3(Tex, Sampler, Coords, Bias)         textureLod      (Tex, _ToVec(Coords), 0.0 + _ToFloat(Bias))
#   define SampleBias_4(Tex, Sampler, Coords, Bias, Offset) textureLodOffset(Tex, _ToVec(Coords), 0.0 + _ToFloat(Bias), Offset)

#   define SampleCmpTex1D_3        SampleCmpLevel0Tex1D_3
#   define SampleCmpTex1DArr_3     SampleCmpLevel0Tex1DArr_3
#   define SampleCmpTex2D_3        SampleCmpLevel0Tex2D_3
#   define SampleCmpTex2DArr_3     SampleCmpLevel0Tex2DArr_3
#   define SampleCmpTexCube_3      SampleCmpLevel0TexCube_3
#   define SampleCmpTexCubeArr_3   SampleCmpLevel0TexCubeArr_3
                                            
#   define SampleCmpTex1D_4        SampleCmpLevel0Tex1D_4
#   define SampleCmpTex1DArr_4     SampleCmpLevel0Tex1DArr_4
#   define SampleCmpTex2D_4        SampleCmpLevel0Tex2D_4
#   define SampleCmpTex2DArr_4     SampleCmpLevel0Tex2DArr_4

#endif

// https://www.opengl.org/sdk/docs/man/html/textureLod.xhtml
#define SampleLevel_3(Tex, Sampler, Coords, Level)         textureLod      (Tex, _ToVec(Coords), _ToFloat(Level))
#define SampleLevel_4(Tex, Sampler, Coords, Level, Offset) textureLodOffset(Tex, _ToVec(Coords), _ToFloat(Level), Offset)

// https://www.opengl.org/sdk/docs/man/html/textureGrad.xhtml
#define SampleGrad_4(Tex, Sampler, Coords, DDX, DDY)         textureGrad      (Tex, _ToVec(Coords), _ToVec(DDX), _ToVec(DDY))
#define SampleGrad_5(Tex, Sampler, Coords, DDX, DDY, Offset) textureGradOffset(Tex, _ToVec(Coords), _ToVec(DDX), _ToVec(DDY), Offset)


// texelFetch performs a lookup of a single texel from texture coordinate P in the texture 
// bound to sampler. The array layer is specified in the last component of P for array forms. 
// The lod parameter (if present) specifies the level-of-detail from which the texel will be fetched. 
// The sample specifies which sample within the texel will be returned when reading from a multi-sample texure.

#define LoadTex1D_1(Tex, Location)        texelFetch      (Tex, _ToInt((Location).x), _ToInt((Location).y))
#define LoadTex1D_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToInt((Location).x), _ToInt((Location).y), int(Offset))
#define LoadTex1DArr_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xy), _ToInt((Location).z) )
#define LoadTex1DArr_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xy), _ToInt((Location).z), int(Offset))
#define LoadTex2D_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xy), _ToInt((Location).z))
#define LoadTex2D_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xy), _ToInt((Location).z), ivec2( (Offset).xy) )
#define LoadTex2DArr_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xyz), _ToInt((Location).w) )
#define LoadTex2DArr_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xyz), _ToInt((Location).w), ivec2( (Offset).xy))
#define LoadTex3D_1(Tex, Location)        texelFetch      (Tex, _ToIvec( (Location).xyz), _ToInt((Location).w))
#define LoadTex3D_2(Tex, Location, Offset)texelFetchOffset(Tex, _ToIvec( (Location).xyz), _ToInt((Location).w), ivec3( (Offset).xyz))
#define LoadTex2DMS_2(Tex, Location, Sample)        texelFetch(Tex, _ToIvec( (Location).xy), _ToInt(Sample))
#define LoadTex2DMS_3(Tex, Location, Sample, Offset)texelFetch(Tex, _ToIvec2( (Location).x + (Offset).x, (Location).y + (Offset).y), int(Sample) ) // No texelFetchOffset for texture2DMS
#define LoadTex2DMSArr_2(Tex, Location, Sample)        texelFetch(Tex, _ToIvec( (Location).xyz), _ToInt(Sample))
#define LoadTex2DMSArr_3(Tex, Location, Sample, Offset)texelFetch(Tex, _ToIvec3( (Location).x + (Offset).x, (Location).y + (Offset).y, (Location).z), int(Sample)) // No texelFetchOffset for texture2DMSArray
#define LoadTexBuffer_1(Tex, Location)  texelFetch(Tex, _ToInt(Location))

//https://www.opengl.org/sdk/docs/man/html/imageLoad.xhtml
#define LoadRWTex1D_1(Tex, Location)    imageLoad(Tex, _ToInt(Location)        )
#define LoadRWTex1DArr_1(Tex, Location) imageLoad(Tex, _ToIvec((Location).xy)  )
#define LoadRWTex2D_1(Tex, Location)    imageLoad(Tex, _ToIvec((Location).xy)  )
#define LoadRWTex2DArr_1(Tex, Location) imageLoad(Tex, _ToIvec((Location).xyz) )
#define LoadRWTex3D_1(Tex, Location)    imageLoad(Tex, _ToIvec((Location).xyz) )
#define LoadRWTexBuffer_1(Tex, Location)imageLoad(Tex, _ToInt(Location)        )

#define Gather_2(Tex, Sampler, Location)        textureGather      (Tex, _ToVec(Location))
#define Gather_3(Tex, Sampler, Location, Offset)textureGatherOffset(Tex, _ToVec(Location), Offset)

#define GatherCmp_3(Tex, Sampler, Location, CompareVal)        textureGather      (Tex, _ToVec(Location), _ToFloat(CompareVal))
#define GatherCmp_4(Tex, Sampler, Location, CompareVal, Offset)textureGatherOffset(Tex, _ToVec(Location), _ToFloat(CompareVal), Offset)

// Atomic operations
#define InterlockedAddSharedVar_2(dest, value)                      atomicAdd(dest, value)
#define InterlockedAddSharedVar_3(dest, value, orig_val) orig_val = atomicAdd(dest, value)
#define InterlockedAddImage_2(img, coords, value)                     imageAtomicAdd(img, _ToIvec(coords), value)
#define InterlockedAddImage_3(img, coords, value, orig_val)orig_val = imageAtomicAdd(img, _ToIvec(coords), value)

#define InterlockedAndSharedVar_2(dest, value)                      atomicAnd(dest, value)
#define InterlockedAndSharedVar_3(dest, value, orig_val) orig_val = atomicAnd(dest, value)
#define InterlockedAndImage_2(img, coords, value)                     imageAtomicAnd(img, _ToIvec(coords), value)
#define InterlockedAndImage_3(img, coords, value, orig_val)orig_val = imageAtomicAnd(img, _ToIvec(coords), value)

#define InterlockedMaxSharedVar_2(dest, value)                      atomicMax(dest, value)
#define InterlockedMaxSharedVar_3(dest, value, orig_val) orig_val = atomicMax(dest, value)
#define InterlockedMaxImage_2(img, coords, value)                     imageAtomicMax(img, _ToIvec(coords), value)
#define InterlockedMaxImage_3(img, coords, value, orig_val)orig_val = imageAtomicMax(img, _ToIvec(coords), value)

#define InterlockedMinSharedVar_2(dest, value)                      atomicMin(dest, value)
#define InterlockedMinSharedVar_3(dest, value, orig_val) orig_val = atomicMin(dest, value)
#define InterlockedMinImage_2(img, coords, value)                     imageAtomicMin(img, _ToIvec(coords), value)
#define InterlockedMinImage_3(img, coords, value, orig_val)orig_val = imageAtomicMin(img, _ToIvec(coords), value)

#define InterlockedOrSharedVar_2(dest, value)                      atomicOr(dest, value)
#define InterlockedOrSharedVar_3(dest, value, orig_val) orig_val = atomicOr(dest, value)
#define InterlockedOrImage_2(img, coords, value)                     imageAtomicOr(img, _ToIvec(coords), value)
#define InterlockedOrImage_3(img, coords, value, orig_val)orig_val = imageAtomicOr(img, _ToIvec(coords), value)

#define InterlockedXorSharedVar_2(dest, value)                      atomicXor(dest, value)
#define InterlockedXorSharedVar_3(dest, value, orig_val) orig_val = atomicXor(dest, value)
#define InterlockedXorImage_2(img, coords, value)                     imageAtomicXor(img, _ToIvec(coords), value)
#define InterlockedXorImage_3(img, coords, value, orig_val)orig_val = imageAtomicXor(img, _ToIvec(coords), value)

// There is actually no InterlockedExchange() with 2 arguments
#define InterlockedExchangeSharedVar_2(dest, value)                      atomicExchange(dest, value)
#define InterlockedExchangeSharedVar_3(dest, value, orig_val) orig_val = atomicExchange(dest, value)
#define InterlockedExchangeImage_2(img, coords, value)                     imageAtomicExchange(img, _ToIvec(coords), value)
#define InterlockedExchangeImage_3(img, coords, value, orig_val)orig_val = imageAtomicExchange(img, _ToIvec(coords), value)

//uint imageAtomicCompSwap(	image img,    IVec P,       nint compare,   nint data);
//void InterlockedCompareExchange(     in R dest, in T compare_value, in  T value, out T original_value);
#define InterlockedCompareExchangeSharedVar_4(dest, cmp_val, value, orig_val)  orig_val = atomicCompSwap(dest, cmp_val, value)
#define InterlockedCompareExchangeImage_4(img, coords, cmp_val, value, orig_val) orig_val = imageAtomicCompSwap(img, _ToIvec(coords), cmp_val, value)

#define InterlockedCompareStoreSharedVar_3(dest, cmp_val, value) atomicCompSwap(dest, cmp_val, value)
#define InterlockedCompareStoreImage_3(img, coords, cmp_val, value)imageAtomicCompSwap(img, _ToIvec(coords), cmp_val, value)


// Swizzling macros
#define _SWIZZLE0
#define _SWIZZLE1 .x
#define _SWIZZLE2 .xy
#define _SWIZZLE3 .xyz
#define _SWIZZLE4 .xyzw

// Helper functions

#ifdef VULKAN

#define NDC_MIN_Z 0.0 // Minimal z in the normalized device space

// Note that Vulkan itself does not invert Y coordinate when transforming
// normalized device Y to window space. However, we use negative viewport
// height which achieves the same effect as in D3D, thererfore we need to
// invert y (see comments in DeviceContextVkImpl::CommitViewports() for details)
#define F3NDC_XYZ_TO_UVD_SCALE float3(0.5, -0.5, 1.0)

#else

#define NDC_MIN_Z -1.0 // Minimal z in the normalized device space
#define F3NDC_XYZ_TO_UVD_SCALE float3(0.5, 0.5, 0.5)

#endif

float2 NormalizedDeviceXYToTexUV( float2 f2ProjSpaceXY )
{
    return float2(0.5,0.5) + F3NDC_XYZ_TO_UVD_SCALE.xy * f2ProjSpaceXY.xy;
}
float2 TexUVToNormalizedDeviceXY( float2 TexUV)
{
    return (TexUV.xy - float2(0.5, 0.5)) / F3NDC_XYZ_TO_UVD_SCALE.xy;
}

float NormalizedDeviceZToDepth(float fNDC_Z)
{
    return (fNDC_Z - NDC_MIN_Z) * F3NDC_XYZ_TO_UVD_SCALE.z;
}
float DepthToNormalizedDeviceZ(float fDepth)
{
    return fDepth / F3NDC_XYZ_TO_UVD_SCALE.z + NDC_MIN_Z;
}

#define MATRIX_ELEMENT(mat, row, col) mat[col][row]

float4x4 MatrixFromRows(float4 row0, float4 row1, float4 row2, float4 row3)
{
    return transpose(float4x4(row0, row1, row2, row3));
}

float3x3 MatrixFromRows(float3 row0, float3 row1, float3 row2)
{
    return transpose(float3x3(row0, row1, row2));
}

float2x2 MatrixFromRows(float2 row0, float2 row1)
{
    return transpose(float2x2(row0, row1));
}

// ---------------------------------- Vertex shader ----------------------------------
#ifdef VERTEX_SHADER

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

#define _GET_GL_VERTEX_ID(VertexId)_TypeConvertStore(VertexId, gl_VertexID)
#define _GET_GL_INSTANCE_ID(InstId)_TypeConvertStore(InstId, gl_InstanceID)
#define _SET_GL_POSITION(Pos)gl_Position=_ExpandVector(Pos)

#endif


// --------------------------------- Fragment shader ---------------------------------
#ifdef FRAGMENT_SHADER

// SV_Position.w == w, while gl_FragCoord.w == 1/w
#define _GET_GL_FRAG_COORD(FragCoord)_ResizeVector(FragCoord, vec4(gl_FragCoord.xyz, 1.0/gl_FragCoord.w))
#define _GET_GL_FRONT_FACING(FrontFacing)_TypeConvertStore(FrontFacing, gl_FrontFacing)
#define _SET_GL_FRAG_DEPTH(Depth)_TypeConvertStore(gl_FragDepth, Depth)

#endif


// --------------------------------- Geometry shader ---------------------------------
#ifdef GEOMETRY_SHADER

// ARB_separate_shader_objects requires built-in block gl_PerVertex to be redeclared before accessing its members
// declaring gl_PointSize and gl_ClipDistance causes compilation error on Android
in gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
};

#define _GET_GL_POSITION(Pos)_ResizeVector(Pos, gl_in[i].gl_Position)
#define _GET_GL_PRIMITIVE_ID(PrimId)_TypeConvertStore(PrimId, gl_in[i].gl_PrimitiveIDIn)

#define _SET_GL_POSITION(Pos)gl_Position=_ExpandVector(Pos)
#define _SET_GL_LAYER(Layer)_TypeConvertStore(gl_Layer,Layer)

#endif


// --------------------------- Tessellation control shader ---------------------------
#ifdef TESS_CONTROL_SHADER

in gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
} gl_in[gl_MaxPatchVertices];

out gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
} gl_out[];

#define _GET_GL_INVOCATION_ID(InvocId)_TypeConvertStore(InvocId, gl_InvocationID)
#define _GET_GL_PRIMITIVE_ID(PrimId)_TypeConvertStore(PrimId, gl_PrimitiveID)
#define _GET_GL_POSITION(Pos)_ResizeVector(Pos, gl_in[i].gl_Position)

#define _SET_GL_POSITION(Pos)gl_out[gl_InvocationID].gl_Position=_ExpandVector(Pos)

void _SetGLTessLevelOuter(float OuterLevel[2])
{
    for(int i=0; i < 2; ++i)
        gl_TessLevelOuter[i] = OuterLevel[i];
}
void _SetGLTessLevelOuter(float OuterLevel[3])
{
    for(int i=0; i < 3; ++i)
        gl_TessLevelOuter[i] = OuterLevel[i];
}
void _SetGLTessLevelOuter(float OuterLevel[4])
{
    for(int i=0; i < 4; ++i)
        gl_TessLevelOuter[i] = OuterLevel[i];
}


void _SetGLTessLevelInner(float InnerLevel[2])
{
    gl_TessLevelInner[0] = InnerLevel[0];
    gl_TessLevelInner[1] = InnerLevel[1];
}
void _SetGLTessLevelInner(float InnerLevel)
{
    gl_TessLevelInner[0] = InnerLevel;
}

#endif


// --------------------------- Tessellation evaluation shader ---------------------------
#ifdef TESS_EVALUATION_SHADER

in gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
} gl_in[gl_MaxPatchVertices];

out gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
};

#define _GET_GL_POSITION(Pos)_ResizeVector(Pos, gl_in[i].gl_Position)

void _GetGLTessLevelOuter(out float OuterLevel[2])
{
    for(int i=0; i < 2; ++i)
        OuterLevel[i] = gl_TessLevelOuter[i];
}
void _GetGLTessLevelOuter(out float OuterLevel[3])
{
    for(int i=0; i < 3; ++i)
        OuterLevel[i] = gl_TessLevelOuter[i];
}
void _GetGLTessLevelOuter(out float OuterLevel[4])
{
    for(int i=0; i < 4; ++i)
        OuterLevel[i] = gl_TessLevelOuter[i];
}

void _GetGLTessLevelInner(out float InnerLevel[2])
{
    InnerLevel[0] = gl_TessLevelInner[0];
    InnerLevel[1] = gl_TessLevelInner[1];
}
void _GetGLTessLevelInner(out float InnerLevel)
{
    InnerLevel = gl_TessLevelInner[0];
}

#define _GET_GL_TESS_COORD(TessCoord)_ResizeVector(TessCoord, gl_TessCoord)
#define _GET_GL_PRIMITIVE_ID(PrimId)_TypeConvertStore(PrimId, gl_PrimitiveID)

#define _SET_GL_POSITION(Pos)gl_Position=_ExpandVector(Pos)

#endif


// ---------------------------------- Compute shader ----------------------------------
#ifdef COMPUTE_SHADER

#define _GET_GL_GLOBAL_INVOCATION_ID(Type, InvocId)InvocId=Type(gl_GlobalInvocationID)
#define _GET_GL_WORK_GROUP_ID(Type, GroupId)GroupId=Type(gl_WorkGroupID)
#define _GET_GL_LOCAL_INVOCATION_ID(Type, InvocId)InvocId=Type(gl_LocalInvocationID)
#define _GET_GL_LOCAL_INVOCATION_INDEX(Type, InvocInd)InvocInd=Type(gl_LocalInvocationIndex)

#endif

#endif // _GLSL_DEFINITIONS_
