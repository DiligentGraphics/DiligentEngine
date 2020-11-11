#include "IncludeTest.h"
// # include <- fix commented include


//#define TEXTURE2D Texture2D <- Macros do not work currently
//TEXTURE2D MacroTex2D;

/******//* /* /**** / */
void EmptyFunc(){}cbuffer cbTest1{int a;}cbuffer cbTest2/*comment*/:/*comment*/register(b0)/*comment*/{int b;}/*comment
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

RWTexture2D<float/* format = r32f */>Tex2D_F1/*comment*/: /*comment*/register(u0)/*comment*/;
RWTexture2D<float/* format = r32f */>Tex2D_F2[2]: register(u1),Tex2D_F3: register(u2),/*cmt*/Tex2D_F4,  Tex2D_F5
/*comment*/:/*comment*/
register(u3);
RWTexture2D<int2/* format = rg32i */>Tex2D_I;
RWTexture2D<uint4/* format = rgba16ui */>Tex2D_U;


int GlobalIntVar;Texture2D Tex2D_Test1:register(t0);Texture2D Tex2D_Test2;/*Comment* / *//* /** Comment2*/Texture2D Tex2D_Test3 /*Cmnt*/: /*comment*/register(t1)/*comment*/,/*comment*/
//  Comment
Tex2D_Test4:register(t2)
;

Texture2D Tex2D_M1, Tex2D_M2:register(t3);

groupshared float4 g_f4TestSharedArr[10];
groupshared int4 g_i4TestSharedArr[10];
groupshared uint4 g_u4TestSharedArr[10];
groupshared float4 g_f4TestSharedVar;
groupshared int4 g_i4TestSharedVar;
groupshared uint4 g_u4TestSharedVar;


void TestGetDimensions()
{
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
}


void TestLoad()
{
    int4 Location = int4(2, 5, 1, 10);

    //Texture2D
    {
        float f = Tex2D_F1.Load(Location.xy);
              f = Tex2D_F1.Load(Tex2D_I.Load(Location.xy) + Tex2D_I.Load(Location.xy).yx);
        int2 i2 = Tex2D_I.Load(Location.xy);
        uint4 u4= Tex2D_U.Load(Location.xy).xyzw;
    }

}



void TestStore(uint2 Location)
{
    //Texture2D
    {
        Tex2D_F1[Location.xy] = 10.0;
        Tex2D_F1[Tex2D_I.Load(Location.xy).xy + Tex2D_I.Load(Location.xy).xy] = 20.0;
        Tex2D_I[Location.xy] = int2( 43, 23);
        Tex2D_U[Location.xy] = uint4( 3,7,1,7);
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
    TestGetDimensions();
    TestLoad();
    TestStore(GTid.xy);

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

    InterlockedAnd(g_i4TestSharedVar.x, 1);
    InterlockedAnd(g_u4TestSharedVar.x, 1u);
    InterlockedAnd(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedAnd(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedOr(g_i4TestSharedVar.x, 1);
    InterlockedOr(g_u4TestSharedVar.x, 1u);
    InterlockedOr(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedOr(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedXor(g_i4TestSharedVar.x, 1);
    InterlockedXor(g_u4TestSharedVar.x, 1u);
    InterlockedXor(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedXor(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
   
    InterlockedMax(g_i4TestSharedVar.x, 1);
    InterlockedMax(g_u4TestSharedVar.x, 1u);
    InterlockedMax(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedMax(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);

    InterlockedMin(g_i4TestSharedVar.x, 1);
    InterlockedMin(g_u4TestSharedVar.x, 1u);
    InterlockedMin(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedMin(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    // There is actually no InterlockedExchange() with 2 arguments
    //InterlockedExchange(g_i4TestSharedVar.x, 1);
    //InterlockedExchange(g_u4TestSharedVar.x, 1u);
    InterlockedExchange(g_i4TestSharedArr[GTid.x].x, 1, iOldVal);
    InterlockedExchange(g_u4TestSharedArr[Gid.x].x, 1u, uOldVal);
                
    InterlockedCompareStore(g_i4TestSharedVar.x, 1, 10);
    InterlockedCompareStore(g_u4TestSharedVar.x, 1u, 10u);
    InterlockedCompareExchange(g_i4TestSharedArr[GTid.x].x, 1, 10, iOldVal);
    InterlockedCompareExchange(g_u4TestSharedArr[Gid.x].x, 1u, 10u, uOldVal);
                
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
    //AllMemoryBarrierWithGroupSync();
}
