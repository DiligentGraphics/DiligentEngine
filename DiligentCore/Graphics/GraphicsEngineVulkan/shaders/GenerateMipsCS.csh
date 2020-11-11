#ifndef NON_POWER_OF_TWO
#define NON_POWER_OF_TWO 0
#endif

#ifndef CONVERT_TO_SRGB
#define CONVERT_TO_SRGB 0
#endif

#ifndef IMG_FORMAT
#define IMG_FORMAT rgba8
#endif

layout(IMG_FORMAT) uniform writeonly image2DArray OutMip0;
layout(IMG_FORMAT) uniform writeonly image2DArray OutMip1;
layout(IMG_FORMAT) uniform writeonly image2DArray OutMip2;
layout(IMG_FORMAT) uniform writeonly image2DArray OutMip3;

uniform sampler2DArray SrcMip;

uniform CB
{
    int SrcMipLevel;    // Texture level of source mip
    int NumMipLevels;   // Number of OutMips to write: [1, 4]
    int FirstArraySlice;
    int Dummy;
    vec2 TexelSize;     // 1.0 / OutMip1.Dimensions
};

//
// The reason for separating channels is to reduce bank conflicts in the
// local data memory controller.  A large stride will cause more threads
// to collide on the same memory bank.
shared float gs_R[64];
shared float gs_G[64];
shared float gs_B[64];
shared float gs_A[64];

void StoreColor( uint Index, vec4 Color )
{
    gs_R[Index] = Color.r;
    gs_G[Index] = Color.g;
    gs_B[Index] = Color.b;
    gs_A[Index] = Color.a;
}

vec4 LoadColor( uint Index )
{
    return vec4( gs_R[Index], gs_G[Index], gs_B[Index], gs_A[Index]);
}

float LinearToSRGB(float x)
{
    // This is exactly the sRGB curve
    //return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
     
    // This is cheaper but nearly equivalent
    return x < 0.0031308 ? 12.92 * x : 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719;
}

vec4 PackColor(vec4 Linear)
{
#if CONVERT_TO_SRGB
    return vec4(LinearToSRGB(Linear.r), LinearToSRGB(Linear.g), LinearToSRGB(Linear.b), Linear.a);
#else
    return Linear;
#endif
}

void GroupMemoryBarrierWithGroupSync()
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

    // Thread execution barrier
    barrier();
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    uint LocalInd = gl_LocalInvocationIndex;
    uvec3 GlobalInd = gl_GlobalInvocationID;
    
    ivec3 SrcMipSize = textureSize(SrcMip, 0); // SrcMip is the view of the source mip level
    bool IsValidThread = GlobalInd.x < uint(SrcMipSize.x) && GlobalInd.y < uint(SrcMipSize.y);
    int ArraySlice = FirstArraySlice + int(GlobalInd.z);

    vec4 Src1 = vec4(0.0, 0.0, 0.0, 0.0);
    float fSrcMipLevel = 0.0; // SrcMip is the view of the source mip level
    if (IsValidThread)
    {
        // One bilinear sample is insufficient when scaling down by more than 2x.
        // You will slightly undersample in the case where the source dimension
        // is odd.  This is why it's a really good idea to only generate mips on
        // power-of-two sized textures.  Trying to handle the undersampling case
        // will force this shader to be slower and more complicated as it will
        // have to take more source texture samples.
#if NON_POWER_OF_TWO == 0
        vec2 UV = TexelSize * (vec2(GlobalInd.xy) + vec2(0.5, 0.5));
        Src1 = textureLod(SrcMip, vec3(UV, ArraySlice), fSrcMipLevel);
#elif NON_POWER_OF_TWO == 1
        // > 2:1 in X dimension
        // Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
        // horizontally.
        vec2 UV1 = TexelSize * (vec2(GlobalInd.xy) + vec2(0.25, 0.5));
        vec2 Off = TexelSize * vec2(0.5, 0.0);
        Src1 = 0.5 * (textureLod(SrcMip, vec3(UV1,       ArraySlice), fSrcMipLevel) +
                      textureLod(SrcMip, vec3(UV1 + Off, ArraySlice), fSrcMipLevel));
#elif NON_POWER_OF_TWO == 2
        // > 2:1 in Y dimension
        // Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
        // vertically.
        vec2 UV1 = TexelSize * (vec2(GlobalInd.xy) + vec2(0.5, 0.25));
        vec2 Off = TexelSize * vec2(0.0, 0.5);
        Src1 = 0.5 * (textureLod(SrcMip, vec3(UV1,       ArraySlice), fSrcMipLevel) +
                      textureLod(SrcMip, vec3(UV1 + Off, ArraySlice), fSrcMipLevel));
#elif NON_POWER_OF_TWO == 3
        // > 2:1 in in both dimensions
        // Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
        // in both directions.
        vec2 UV1 = TexelSize * (vec2(GlobalInd.xy) + vec2(0.25, 0.25));
        vec2 Off = TexelSize * 0.5;
        Src1 += textureLod(SrcMip, vec3(UV1,                      ArraySlice), fSrcMipLevel);
        Src1 += textureLod(SrcMip, vec3(UV1 + vec2(Off.x, 0.0),   ArraySlice), fSrcMipLevel);
        Src1 += textureLod(SrcMip, vec3(UV1 + vec2(0.0,   Off.y), ArraySlice), fSrcMipLevel);
        Src1 += textureLod(SrcMip, vec3(UV1 + vec2(Off.x, Off.y), ArraySlice), fSrcMipLevel);
        Src1 *= 0.25;
#endif

        imageStore(OutMip0, ivec3(GlobalInd.xy, ArraySlice), PackColor(Src1));
    }

    // A scalar (constant) branch can exit all threads coherently.
    if (NumMipLevels == 1)
        return;

    if (IsValidThread)
    {
        // Without lane swizzle operations, the only way to share data with other
        // threads is through LDS.
        StoreColor(LocalInd, Src1);
    }

    // This guarantees all LDS writes are complete and that all threads have
    // executed all instructions so far (and therefore have issued their LDS
    // write instructions.)
	GroupMemoryBarrierWithGroupSync();

    if (IsValidThread)
    {
        // With low three bits for X and high three bits for Y, this bit mask
        // (binary: 001001) checks that X and Y are even.
        if ((LocalInd & 0x9u) == 0u)
        {
            vec4 Src2 = LoadColor(LocalInd + 0x01u);
            vec4 Src3 = LoadColor(LocalInd + 0x08u);
            vec4 Src4 = LoadColor(LocalInd + 0x09u);
            Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

            imageStore(OutMip1, ivec3(GlobalInd.xy / 2u, ArraySlice), PackColor(Src1));
            StoreColor(LocalInd, Src1);
        }
    }

    if (NumMipLevels == 2)
        return;

	GroupMemoryBarrierWithGroupSync();

    if( IsValidThread )
    {
        // This bit mask (binary: 011011) checks that X and Y are multiples of four.
        if ((LocalInd & 0x1Bu) == 0u)
        {
            vec4 Src2 = LoadColor(LocalInd + 0x02u);
            vec4 Src3 = LoadColor(LocalInd + 0x10u);
            vec4 Src4 = LoadColor(LocalInd + 0x12u);
            Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

            imageStore(OutMip2, ivec3(GlobalInd.xy / 4u, ArraySlice), PackColor(Src1));
            StoreColor(LocalInd, Src1);
        }
    }

    if (NumMipLevels == 3)
        return;

	GroupMemoryBarrierWithGroupSync();

    if( IsValidThread )
    {
        // This bit mask would be 111111 (X & Y multiples of 8), but only one
        // thread fits that criteria.
        if (LocalInd == 0u)
        {
            vec4 Src2 = LoadColor(LocalInd + 0x04u);
            vec4 Src3 = LoadColor(LocalInd + 0x20u);
            vec4 Src4 = LoadColor(LocalInd + 0x24u);
            Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

            imageStore(OutMip3, ivec3(GlobalInd.xy / 8u, ArraySlice), PackColor(Src1));
        }
    }
}
