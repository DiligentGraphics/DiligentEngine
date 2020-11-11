#include "IncludeTest.h"
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

RWTexture1D<float /* format=r32f */ >Tex1D_F1 /*comment*/:/*comment*/ register(u0)/*comment*/;
RWTexture1D<int2  /*format=rg32i*/ >Tex1D_I;
RWTexture1D<uint4 /* format= rgba32ui */ >Tex1D_U;

RWTexture1DArray </* format = r32f */ float >  Tex1D_F_A : register(u1);
RWTexture1DArray </* format = rg16i */ int2 >   Tex1D_I_A;
RWTexture1DArray </* format = rgba16ui */ uint4 >  Tex1D_U_A : register(u2);

void TestGetDimensions()
{
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
}

void TestLoad()
{
    int4 Location = int4(2, 5, 1, 10);

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
}



void TestStore(uint2 Location)
{
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
}
