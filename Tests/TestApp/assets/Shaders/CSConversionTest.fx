#include "IncludeTest.fxh"
// # include <- fix commented include


//#define TEXTURE2D Texture2D <- Macros do not work currently
//TEXTURE2D MacroTex2D;

/******//* /* /**** / */
void EmptyFunc(){}cbuffer cbTest1{int a;}cbuffer cbTest2{int b;}/*comment
test

*/cbuffer cbTest3{int c;}//Single line comment
cbuffer cbTest4{int d;}

cbuffer cbTest5
{
    float4 e;
}

cbuffer cbTest6
{
    float4 f;
};

int cbuffer_fake;
int fakecbuffer;

#ifndef GL_ES

RWTexture1D<float /* format=r32f */ >Tex1D_F1;
RWTexture1D<int2  /*format=rg32i*/ >Tex1D_I;
RWTexture1D<uint4 /* format= rgba32ui */ >Tex1D_U;

RWTexture1DArray </* format = r32f */ float >  Tex1D_F_A;
RWTexture1DArray </* format = rg16i */ int2 >   Tex1D_I_A;
RWTexture1DArray </* format = rgba16ui */ uint4 >  Tex1D_U_A;

#endif

RWTexture2D<float/* format = r32f */>Tex2D_F1;
RWTexture2D<float/* format = r32f */>Tex2D_F2[2],Tex2D_F3,/*cmt*/Tex2D_F4,  Tex2D_F5;
RWTexture2D<int2/* format = rg32i */>Tex2D_I;
RWTexture2D<uint4/* format = rgba16ui */>Tex2D_U;
RWTexture2D<int/* format = r32i */>Tex2D_I1;
RWTexture2D<uint/* format = r32ui */>Tex2D_U1;

RWTexture2DArray <float// format = r32f 
                  >  Tex2D_F_A;
RWTexture2DArray <int2// format = rg8i 
                  >   Tex2D_I_A;
RWTexture2DArray <uint4// format = rgba8ui 
                  >  Tex2D_U_A;

RWTexture3D< float4/*
format
=
rgba32f
*/> Tex3D_F;
RWTexture3D< int /* format = r8i */>    Tex3D_I;
RWTexture3D< uint2 /* format = rg8ui */>  Tex3D_U;
RWTexture3D< float/*format = r32f*/> Tex3D_F2;


int GlobalIntVar;Texture2D Tex2D_Test1;Texture2D Tex2D_Test2;/*Comment* / *//* /** Comment2*/Texture2D Tex2D_Test3 /*Cmnt*/,
//  Comment
Tex2D_Test4
;

Texture2D Tex2D_M1, Tex2D_M2;

groupshared float4 g_f4TestSharedArr[10];
groupshared int4 g_i4TestSharedArr[10];
groupshared uint4 g_u4TestSharedArr[10];
groupshared float4 g_f4TestSharedVar;
groupshared int4 g_i4TestSharedVar;
groupshared uint4 g_u4TestSharedVar;


void TestGetDimensions()
{
#ifndef GL_ES
    // RWTexture1D 
    {
        uint uWidth;
        float fWidth;
        int iWidth;
        Tex1D_F1.GetDimensions(uWidth);
        Tex1D_I.GetDimensions( uWidth);
        Tex1D_U.GetDimensions( uWidth );

        Tex1D_F1.GetDimensions(fWidth);
        Tex1D_I.GetDimensions( fWidth);
        Tex1D_U.GetDimensions( fWidth );

        Tex1D_F1.GetDimensions(iWidth);
        Tex1D_I.GetDimensions( iWidth);
        Tex1D_U.GetDimensions( iWidth );
    }

    // RWTexture1DArray
    {
        uint uWidth, uElems;
        int iWidth, iElems;
        float fWidth, fElems;
        Tex1D_F_A.GetDimensions(uWidth, uElems);
        Tex1D_U_A.GetDimensions( uWidth, uElems);
        Tex1D_I_A.GetDimensions( uWidth , uElems );

        Tex1D_F_A.GetDimensions(iWidth, iElems);
        Tex1D_U_A.GetDimensions( iWidth, iElems);
        Tex1D_I_A.GetDimensions( iWidth , iElems );

        Tex1D_F_A.GetDimensions(fWidth, fElems);
        Tex1D_U_A.GetDimensions( fWidth, fElems);
        Tex1D_I_A.GetDimensions( fWidth , fElems );
    }
#endif

    //RWTexture2D
    {
        uint uWidth, uHeight;
        int iWidth, iHeight;
        float fWidth, fHeight;
        Tex2D_F1.GetDimensions(uWidth, uHeight);
        Tex2D_F3.GetDimensions(uWidth, uHeight);
        Tex2D_F4.GetDimensions(uWidth, uHeight);
        Tex2D_F2[0].GetDimensions( uWidth, uHeight);
        Tex2D_I.GetDimensions (uWidth, uHeight );
        Tex2D_U.GetDimensions ( uWidth, uHeight );

        Tex2D_F1.GetDimensions(iWidth, iHeight);
        Tex2D_F2[0].GetDimensions( iWidth, iHeight);
        Tex2D_I.GetDimensions (iWidth, iHeight );
        Tex2D_U.GetDimensions ( iWidth, iHeight );

        Tex2D_F1.GetDimensions(fWidth, fHeight);
        Tex2D_F2[0].GetDimensions( fWidth, fHeight);
        Tex2D_I.GetDimensions (fWidth, fHeight );
        Tex2D_U.GetDimensions ( fWidth, fHeight );
    }

    //RWTexture2DArray
    {
        uint uWidth, uHeight, uElems;
        float fWidth, fHeight, fElems;
        int iWidth, iHeight, iElems;
        Tex2D_F_A.GetDimensions(uWidth,uHeight,uElems);
        Tex2D_U_A.GetDimensions( uWidth, uHeight, uElems );
        Tex2D_I_A.GetDimensions (uWidth , uHeight , uElems ) ;

        Tex2D_F_A.GetDimensions(iWidth,iHeight,iElems);
        Tex2D_U_A.GetDimensions( iWidth, iHeight, iElems );
        Tex2D_I_A.GetDimensions (iWidth , iHeight , iElems ) ;

        Tex2D_F_A.GetDimensions(fWidth,fHeight,fElems);
        Tex2D_U_A.GetDimensions( fWidth, fHeight, fElems );
        Tex2D_I_A.GetDimensions (fWidth , fHeight , fElems ) ;
    }

    //RWTexture3D
    {
        uint uWidth, uHeight, uDepth;
        int iWidth, iHeight, iDepth;
        float fWidth, fHeight, fDepth;
        Tex3D_F.GetDimensions ( uWidth, uHeight, uDepth );
        Tex3D_U.GetDimensions(uWidth,uHeight,uDepth);
        Tex3D_I.GetDimensions ( uWidth , uHeight , uDepth );

        Tex3D_F.GetDimensions ( iWidth, iHeight, iDepth );
        Tex3D_U.GetDimensions(iWidth,iHeight,iDepth);
        Tex3D_I.GetDimensions ( iWidth , iHeight , iDepth );

        Tex3D_F.GetDimensions ( fWidth, fHeight, fDepth );
        Tex3D_U.GetDimensions(fWidth,fHeight,fDepth);
        Tex3D_I.GetDimensions ( fWidth , fHeight , fDepth );
    }
}

void TestLoad()
{
    int4 Location = int4(2, 5, 1, 10);

#ifndef GL_ES
    // Texture1D 
    {
        float f  = Tex1D_F1.Load(Location.x);
        int2 i2  = Tex1D_I.Load(Location.x).xy;
        uint4 u4 = Tex1D_U.Load(Location.x);
    }

    // Texture1DArray
    {
        float f  = Tex1D_F_A.Load(Location.xy);
        uint4 u4 = Tex1D_U_A.Load(Location.xy);
        int2 i2  = Tex1D_I_A.Load(Location.xy);
    }
#endif

    //Texture2D
    {
        float f = Tex2D_F1.Load(Location.xy);
              f = Tex2D_F1.Load(Tex2D_I.Load(Location.xy) + Tex2D_I.Load(Location.xy).yx);
        int2 i2 = Tex2D_I.Load(Location.xy);
        uint4 u4= Tex2D_U.Load(Location.xy).xyzw;
    }

    //Texture2DArray
    {
        float f  = Tex2D_F_A.Load(Location.xyz);
        uint4 u4 = Tex2D_U_A.Load(Location.xyz);
        int2 i2  = Tex2D_I_A.Load(Location.xyz);
    }

    //Texture3D
    {
        float4 f4 = Tex3D_F.Load(Location.xyz).xyzw;
        uint2  u2 = Tex3D_U.Load(Location.xyz).xy;
        int    i  = Tex3D_I.Load(Location.xyz);
    }
}



void TestStore()
{
    int4 Location = int4(2, 5, 1, 10);

#ifndef GL_ES
    // Texture1D 
    {
        Tex1D_F1[Location.x] = 1.0;
        Tex1D_I[ Location.x] = int2(3,6);
        Tex1D_U[ Location.x ] = uint4(0,4,7,8);
    }

    // Texture1DArray
    {
        Tex1D_F_A[Location.xy] = 3.5;
        Tex1D_U_A[ Location.xy] = uint4(2,4,2,5);
        Tex1D_I_A[Location.xy ] = int2( 13, 19);
    }
#endif

    //Texture2D
    {
        Tex2D_F1[Location.xy] = 10.0;
        Tex2D_F1[Tex2D_I.Load(Location.xy).xy + Tex2D_I.Load(Location.xy).xy] = 20.0;
        Tex2D_I[Location.xy] = int2( 43, 23);
        Tex2D_U[Location.xy] = uint4( 3,7,1,7);
    }

    //Texture2DArray
    {
        Tex2D_F_A[Location.xyz] = 30.0;
        Tex2D_U_A[Location.xyz] = uint4(0,5,7,87);
        Tex2D_I_A[Location.xyz] = int2(-3, 5);
    }

    //Texture3D
    {
        Tex3D_F[Location.xyz] = float4(10.0, 25.0, 26.0, 27.0);
        Tex3D_U[Location.xyz] = uint2(0,6);
        Tex3D_I[Location.xyz] = -5;
    }
}

/*
void TestImageArgs1(RWTexture2D</* format = r32i * /int> in_RWTex, int2 Location)
{
    int Width, Height;
    in_RWTex.GetDimensions (Width, Height );
    in_RWTex[Location] = Width;
    InterlockedAdd(in_RWTex[Location], 3);
    int i  = in_RWTex.Load(Location.xy);
}

void TestImageArgs2(RWTexture3D<float/*format=r32f* /> in_RWTex)
{
    int Width, Height, Depth;
    in_RWTex.GetDimensions (Width, Height, Depth );
    float f  = in_RWTex.Load(int3(10,11,23));
    in_RWTex[int3(1,2,3)] = float4(10.0, 25.0, 26.0, 27.0);
}
*/

int2 GetCoords( uint x, uint y )
{
    return int2(x, y);
}

struct CSInputSubstr
{
    uint3 DTid : SV_DispatchThreadID;
};
struct CSInput
{
    uint GroupInd : SV_GroupIndex;
    CSInputSubstr substr;
};

[numthreads(2,4,8)]
void TestCS(CSInput In,
            uint3 Gid : SV_GroupID,
            uint3 GTid : SV_GroupThreadID)
{
    uint uOldVal;
    int iOldVal;

    if( GTid.y == 0u )
    {
        g_i4TestSharedVar = int4(0, 0, 0, 0);
        g_u4TestSharedVar = uint4(0u, 0u, 0u, 0u);
    }
    g_i4TestSharedArr[In.GroupInd] = int4(0, 0, 0, 0);
    g_u4TestSharedArr[In.GroupInd] = uint4(0u, 0u, 0u, 0u);

    InterlockedAdd(g_i4TestSharedVar.x, 1);
    InterlockedAdd(g_u4TestSharedVar.x, 1u);
    InterlockedAdd(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedAdd(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);

    InterlockedAdd(Tex2D_I1[GTid.xy], 1);
    InterlockedAdd(Tex2D_U1[GTid.xy], 1u);
    InterlockedAdd(Tex2D_I1[GTid.xy], 1, iOldVal);
    InterlockedAdd(Tex2D_U1[GTid.xy], 1u, uOldVal);
    InterlockedAdd(Tex2D_I1[ GetCoords(GTid.x, GTid.y) ], 1);


    InterlockedAnd(g_i4TestSharedVar.x, 1);
    InterlockedAnd(g_u4TestSharedVar.x, 1u);
    InterlockedAnd(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedAnd(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedAnd(Tex2D_I1[GTid.xy], 1);
    InterlockedAnd(Tex2D_U1[GTid.xy], 1u);
    InterlockedAnd(Tex2D_I1[GTid.xy], 1, iOldVal);
    InterlockedAnd(Tex2D_U1[GTid.xy], 1u, uOldVal);


    InterlockedOr(g_i4TestSharedVar.x, 1);
    InterlockedOr(g_u4TestSharedVar.x, 1u);
    InterlockedOr(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedOr(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedOr(Tex2D_I1[GTid.xy], 1);
    InterlockedOr(Tex2D_U1[GTid.xy], 1u);
    InterlockedOr(Tex2D_I1[GTid.xy], 1, iOldVal);
    InterlockedOr(Tex2D_U1[GTid.xy], 1u, uOldVal);

        
    InterlockedXor(g_i4TestSharedVar.x, 1);
    InterlockedXor(g_u4TestSharedVar.x, 1u);
    InterlockedXor(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedXor(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedXor(Tex2D_I1[GTid.xy], 1);
    InterlockedXor(Tex2D_U1[GTid.xy], 1u);
    InterlockedXor(Tex2D_I1[GTid.xy], 1, iOldVal);
    InterlockedXor(Tex2D_U1[GTid.xy], 1u, uOldVal);

   
    InterlockedMax(g_i4TestSharedVar.x, 1);
    InterlockedMax(g_u4TestSharedVar.x, 1u);
    InterlockedMax(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedMax(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedMax(Tex2D_I1[GTid.xy], 1);
    InterlockedMax(Tex2D_U1[GTid.xy], 1u);
    InterlockedMax(Tex2D_I1[GTid.xy], 1, iOldVal);
    InterlockedMax(Tex2D_U1[GTid.xy], 1u, uOldVal);


    InterlockedMin(g_i4TestSharedVar.x, 1);
    InterlockedMin(g_u4TestSharedVar.x, 1u);
    InterlockedMin(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedMin(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedMin(Tex2D_I1[GTid.xy], 1);
    InterlockedMin(Tex2D_U1[GTid.xy], 1u);
    InterlockedMin(Tex2D_I1[GTid.xy], 1, iOldVal);
    InterlockedMin(Tex2D_U1[GTid.xy], 1u, uOldVal);

    // There is actually no InterlockedExchange() with 2 arguments
    //InterlockedExchange(g_i4TestSharedVar.x, 1);
    //InterlockedExchange(g_u4TestSharedVar.x, 1u);
    InterlockedExchange(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedExchange(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    //InterlockedExchange(Tex2D_I1[GTid.xy], 1);
    //InterlockedExchange(Tex2D_U1[GTid.xy], 1u);
    InterlockedExchange(Tex2D_I1[GTid.xy], 1, iOldVal);
    InterlockedExchange(Tex2D_U1[GTid.xy], 1u, uOldVal);

    InterlockedCompareStore(g_i4TestSharedVar.x, 1, 10);
    InterlockedCompareStore(g_u4TestSharedVar.x, 1u, 10u);
    InterlockedCompareExchange(g_i4TestSharedArr[GTid.x].x, 1, 10, iOldVal);
    InterlockedCompareExchange(g_u4TestSharedArr[Gid.x].x, 1u, 10u, uOldVal);
                
    InterlockedCompareStore(Tex2D_I1[GTid.xy], 1, 20);
    InterlockedCompareStore(Tex2D_U1[GTid.xy], 1u, 20u);
    InterlockedCompareExchange(Tex2D_I1[GTid.xy], 1, 20, iOldVal);
    InterlockedCompareExchange(Tex2D_U1[GTid.xy], 1u, 20u, uOldVal);

	//uint2 ui2Dim;
	//g_tex2DTestUAV.GetDimensions(ui2Dim.x, ui2Dim.y);
	//if( DTid.x >= ui2Dim.x || DTid.y >= ui2Dim.y )return;

	//float2 f2UV = float2(DTid.xy) / float2(ui2Dim);
	//float DistFromCenter = length(f2UV - float2(0.5,0.5));
	//g_tex2DTestUAV[DTid.xy] = float4((1-DistFromCenter), abs(f2UV.x-0.5), abs(0.5-f2UV.y), 0);
    GroupMemoryBarrier();
    GroupMemoryBarrierWithGroupSync();
    DeviceMemoryBarrier();
    DeviceMemoryBarrierWithGroupSync();
    AllMemoryBarrier();
    AllMemoryBarrierWithGroupSync();

    //TestImageArgs1(Tex2D_I1, int2(GTid.xy));
    //TestImageArgs2( Tex3D_F2 );
}
