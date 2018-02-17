#if defined(GL_ES) && (__VERSION__<=300)
#   define GLES30 1
#else
#   define GLES30 0
#endif

/***//* Some comment * ** * * * / ** //// */ //Another comment
//
// Comment

/* More *//*com*///ments/**/
//
//


#   include /*Comment*/ <IncludeTest.fxh>
// #include "NonExistingFile.h"


//#define TEXTURE2D Texture2D <- Macros do not work currently
//TEXTURE2D MacroTex2D;

/******//* /* /**** / */
void EmptyFunc(){}cbuffer cbTest1{int a;}cbuffer cbTest2{int b;}/*comment
test

**/cbuffer cbTest3{int c;}//Single line comment
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

int GlobalIntVar;Texture2D Tex2D_Test1;Texture2D Tex2D_Test2;/*Comment* / *//* /** Comment2**/Texture2D Tex2D_Test3;

Texture2D Tex2D_M1, Tex2D_M2;

// Test texture declaration

#ifndef GL_ES

Texture1D Tex1D_F1;
Texture1D<float>Tex1D_F2;
Texture1D<int2>Tex1D_I;
Texture1D<uint4>Tex1D_U;

SamplerState Tex1D_F1_sampler;

Texture1DArray          Tex1D_F_A1;
Texture1DArray <float>  Tex1D_F_A2;
Texture1DArray <int2>   Tex1D_I_A;
Texture1DArray <uint4>  Tex1D_U_A;

SamplerState Tex1D_F_A1_sampler;

Texture1D Tex1DS1;
Texture1D<float>Tex1DS2,Tex1DS3;
SamplerComparisonState Tex1DS1_sampler, Tex1DS2_sampler, TestCmpSamplerArr[2], Tex1DS3_sampler;

Texture1DArray Tex1DAS1;
SamplerComparisonState Tex1DAS1_sampler, Tex1DAS2_sampler;
Texture1DArray<float>Tex1DAS2;

#endif

Texture2D Tex2D_F1;
Texture2D<float>Tex2D_F3[2],Tex2D_F2,Tex2DS_F4,Tex2DS_F5;
Texture2D<float4>Tex2D_F6;
Texture2D<int3>Tex2D_I;
Texture2D<uint4>Tex2D_U;

SamplerState Tex2D_F1_sampler,Tex2D_F6_sampler;
SamplerComparisonState DummySampler, Tex2DS_F4_sampler,Tex2DS_F5_sampler;

Texture2DArray          Tex2D_F_A1;
Texture2DArray <float>  Tex2D_F_A2;
Texture2DArray <float4> Tex2D_F_A3;
Texture2DArray <int2>   Tex2D_I_A;
Texture2DArray <uint4>  Tex2D_U_A;

SamplerState Tex2D_F_A1_sampler,Tex2D_F_A3_sampler;

#if !GLES30
Texture2DMS < float2 > Tex2DMS_F1;
Texture2DMS < float, 4 > Tex2DMS_F2;
Texture2DMS < int >   Tex2DMS_I;
Texture2DMS < uint > Tex2DMS_U;
#endif

#ifndef GL_ES
Texture2DMSArray  <  float3  > Tex2DMS_F_A1;
Texture2DMSArray  <  float, 4>  Tex2DMS_F_A2;
Texture2DMSArray  <  int2   >  Tex2DMS_I_A;
Texture2DMSArray  <  uint4  >  Tex2DMS_U_A;
#endif

Texture3D           Tex3D_F1;
Texture3D< float4 > Tex3D_F2;
Texture3D< float  > Tex3D_F3;
Texture3D< int >    Tex3D_I;
Texture3D< uint2 >  Tex3D_U;

SamplerState Tex3D_F1_sampler;

TextureCube          TexC_F1;
TextureCube <float2> TexC_F2;
TextureCube <int4>   TexC_I;
TextureCube <uint>   TexC_U;

SamplerState TexC_F1_sampler;

#ifndef GL_ES
TextureCubeArray            TexC_F_A1;
TextureCubeArray <float2>   TexC_F_A2;
TextureCubeArray <int4  >   TexC_I_A;
TextureCubeArray <uint  >   TexC_U_A;

SamplerState TexC_F_A1_sampler;
#endif

Texture2D Tex2DS1;
SamplerComparisonState Tex2DS1_sampler;
Texture2D<float>Tex2DS2;
SamplerComparisonState Tex2DS2_sampler;

Texture2DArray Tex2DAS1;
SamplerComparisonState Tex2DAS1_sampler;
Texture2DArray<float>Tex2DAS2;
SamplerComparisonState Tex2DAS2_sampler;

TextureCube TexCS1;
SamplerComparisonState TexCS1_sampler;
TextureCube< float > TexCS2;
SamplerComparisonState TexCS2_sampler;

#ifndef GL_ES
TextureCubeArray TexCAS1;
SamplerComparisonState TexCAS1_sampler;
TextureCubeArray <float> TexCAS2;
SamplerComparisonState TexCAS2_sampler;
#endif

int intvar1;SamplerState Dummy;int intvar2;

int Texture2D_fake, Texture2DArray_fake, fakeTexture2D, fakeTexture2DArray;
int Texture2DMS_fake;
int fakeTexture2DMS;
int Texture2DMSArray_fake;
int fakeTexture2DMSArray;
int Texture3D_fake;
int fakeTexture3D;
int TextureCube_fake, TextureCubeArray_fake;
int fakeTextureCube, fakeTextureCubeArray;
int SamplerState_fake;
int SamplerComparisonState_fake;
int fakeSamplerState;
int fakeSamplerComparisonState;
int Texture4D;
int Texture2d;
int TextureCub;
int Texture2DArr;
int Texture2DM;
int Texture2DMSArr;

void TestGetDimensions()
{
#ifndef GL_ES
    // Texture1D 
    {
        uint uWidth, uMipLevels;
        int iWidth, iMipLevels;
        float fWidth, fMipLevels;
        Tex1D_F1.GetDimensions(uWidth);
        Tex1D_F1.GetDimensions(0, uWidth, uMipLevels);
        Tex1D_I.GetDimensions(uWidth);
        Tex1D_I.GetDimensions(0, uWidth, uMipLevels);
        Tex1D_U.GetDimensions(uWidth);
        Tex1D_U.GetDimensions(0, uWidth, uMipLevels);
        Tex1DS1.GetDimensions(uWidth);
        Tex1DS1.GetDimensions(0, uWidth, uMipLevels);

        Tex1D_F1.GetDimensions(fWidth);
        Tex1D_F1.GetDimensions(0, fWidth, fMipLevels);
        Tex1D_I.GetDimensions(fWidth);
        Tex1D_I.GetDimensions(0, fWidth, fMipLevels);
        Tex1D_U.GetDimensions(fWidth);
        Tex1D_U.GetDimensions(0, fWidth, fMipLevels);
        Tex1DS1.GetDimensions(fWidth);
        Tex1DS1.GetDimensions(0, fWidth, fMipLevels);

        Tex1D_F1.GetDimensions(iWidth);
        Tex1D_F1.GetDimensions(0, iWidth, iMipLevels);
        Tex1D_I.GetDimensions(iWidth);
        Tex1D_I.GetDimensions(0, iWidth, iMipLevels);
        Tex1D_U.GetDimensions(iWidth);
        Tex1D_U.GetDimensions(0, iWidth, iMipLevels);
        Tex1DS1.GetDimensions(iWidth);
        Tex1DS1.GetDimensions(0, iWidth, iMipLevels);
    }

    // Texture1DArray
    {
        uint uWidth, uMipLevels, uElems;
        float fWidth, fMipLevels, fElems;
        int iWidth, iMipLevels, iElems;

        Tex1D_F_A1.GetDimensions(0, uWidth, uElems, uMipLevels);
        Tex1D_F_A1.GetDimensions(uWidth, uElems);
        Tex1D_U_A.GetDimensions(0, uWidth, uElems, uMipLevels);
        Tex1D_U_A.GetDimensions(uWidth, uElems);
        Tex1D_I_A.GetDimensions(0, uWidth, uElems, uMipLevels);
        Tex1D_I_A.GetDimensions(uWidth, uElems);
        Tex1DAS1.GetDimensions(0, uWidth, uElems, uMipLevels);
        Tex1DAS1.GetDimensions(uWidth, uElems);

        Tex1D_F_A1.GetDimensions(0, iWidth, iElems, iMipLevels);
        Tex1D_F_A1.GetDimensions(iWidth, iElems);
        Tex1D_U_A.GetDimensions(0, iWidth, iElems, iMipLevels);
        Tex1D_U_A.GetDimensions(iWidth, iElems);
        Tex1D_I_A.GetDimensions(0, iWidth, iElems, iMipLevels);
        Tex1D_I_A.GetDimensions(iWidth, iElems);
        Tex1DAS1.GetDimensions(0, iWidth, iElems, iMipLevels);
        Tex1DAS1.GetDimensions(iWidth, iElems);

        Tex1D_F_A1.GetDimensions(0, fWidth, fElems, fMipLevels);
        Tex1D_F_A1.GetDimensions(fWidth, fElems);
        Tex1D_U_A.GetDimensions(0, fWidth, fElems, fMipLevels);
        Tex1D_U_A.GetDimensions(fWidth, fElems);
        Tex1D_I_A.GetDimensions(0, fWidth, fElems, fMipLevels);
        Tex1D_I_A.GetDimensions(fWidth, fElems);
        Tex1DAS1.GetDimensions(0, fWidth, fElems, fMipLevels);
        Tex1DAS1.GetDimensions(fWidth, fElems);
    }
#endif

    //Texture2D
    {
        uint uWidth, uHeight, uMipLevels;
        int iWidth, iHeight, iMipLevels;
        float fWidth, fHeight, fMipLevels;

        Tex2D_F1.GetDimensions(uWidth, uHeight);
        Tex2D_F1.GetDimensions(0, uWidth, uHeight, uMipLevels);
        Tex2D_F3[0].GetDimensions( 0, uWidth, uHeight, uMipLevels );
        Tex2D_I.GetDimensions(uWidth, uHeight);
        Tex2D_I.GetDimensions(0, uWidth, uHeight, uMipLevels);
        Tex2D_U.GetDimensions(uWidth, uHeight);
        Tex2D_U.GetDimensions(0, uWidth, uHeight, uMipLevels);
        Tex2DS1.GetDimensions(uWidth, uHeight);
        Tex2DS1.GetDimensions(0, uWidth, uHeight, uMipLevels);

        Tex2D_F1.GetDimensions(iWidth, iHeight);
        Tex2D_F1.GetDimensions(0, iWidth, iHeight, iMipLevels);
        Tex2D_F3[0].GetDimensions( 0, iWidth, iHeight, iMipLevels );
        Tex2D_I.GetDimensions(iWidth, iHeight);
        Tex2D_I.GetDimensions(0, iWidth, iHeight, iMipLevels);
        Tex2D_U.GetDimensions(iWidth, iHeight);
        Tex2D_U.GetDimensions(0, iWidth, iHeight, iMipLevels);
        Tex2DS1.GetDimensions(iWidth, iHeight);
        Tex2DS1.GetDimensions(0, iWidth, iHeight, iMipLevels);


        Tex2D_F1.GetDimensions(fWidth, fHeight);
        Tex2D_F1.GetDimensions(0, fWidth, fHeight, fMipLevels);
        Tex2D_F3[0].GetDimensions( 0, fWidth, fHeight, fMipLevels );
        Tex2D_I.GetDimensions(fWidth, fHeight);
        Tex2D_I.GetDimensions(0, fWidth, fHeight, fMipLevels);
        Tex2D_U.GetDimensions(fWidth, fHeight);
        Tex2D_U.GetDimensions(0, fWidth, fHeight, fMipLevels);
        Tex2DS1.GetDimensions(fWidth, fHeight);
        Tex2DS1.GetDimensions(0, fWidth, fHeight, fMipLevels);
    }

    //Texture2DArray
    {
        uint uWidth, uHeight, uMipLevels, uElems;
        int iWidth, iHeight, iMipLevels, iElems;
        float fWidth, fHeight, fMipLevels, fElems;

        Tex2D_F_A1.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        Tex2D_F_A1.GetDimensions(uWidth, uHeight, uElems);
        Tex2D_U_A.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        Tex2D_U_A.GetDimensions(uWidth, uHeight, uElems);
        Tex2D_I_A.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        Tex2D_I_A.GetDimensions(uWidth, uHeight, uElems);
        Tex2DAS1.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        Tex2DAS1.GetDimensions(uWidth, uHeight, uElems);

        Tex2D_F_A1.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        Tex2D_F_A1.GetDimensions(iWidth, iHeight, iElems);
        Tex2D_U_A.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        Tex2D_U_A.GetDimensions(iWidth, iHeight, iElems);
        Tex2D_I_A.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        Tex2D_I_A.GetDimensions(iWidth, iHeight, iElems);
        Tex2DAS1.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        Tex2DAS1.GetDimensions(iWidth, iHeight, iElems);

        Tex2D_F_A1.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        Tex2D_F_A1.GetDimensions(fWidth, fHeight, fElems);
        Tex2D_U_A.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        Tex2D_U_A.GetDimensions(fWidth, fHeight, fElems);
        Tex2D_I_A.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        Tex2D_I_A.GetDimensions(fWidth, fHeight, fElems);
        Tex2DAS1.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        Tex2DAS1.GetDimensions(fWidth, fHeight, fElems);
    }

    //Texture3D
    {
        uint uWidth, uHeight, uDepth, uMipLevels;
        int iWidth, iHeight, iDepth, iMipLevels;
        float fWidth, fHeight, fDepth, fMipLevels;
        Tex3D_F1.GetDimensions(0, uWidth, uHeight, uDepth, uMipLevels);
        Tex3D_F1.GetDimensions(uWidth, uHeight, uDepth);
        Tex3D_U.GetDimensions(0, uWidth, uHeight, uDepth, uMipLevels);
        Tex3D_U.GetDimensions(uWidth, uHeight, uDepth);
        Tex3D_I.GetDimensions(0, uWidth, uHeight, uDepth, uMipLevels);
        Tex3D_I.GetDimensions(uWidth, uHeight, uDepth);

        Tex3D_F1.GetDimensions(0, iWidth, iHeight, iDepth, iMipLevels);
        Tex3D_F1.GetDimensions(iWidth, iHeight, iDepth);
        Tex3D_U.GetDimensions(0, iWidth, iHeight, iDepth, iMipLevels);
        Tex3D_U.GetDimensions(iWidth, iHeight, iDepth);
        Tex3D_I.GetDimensions(0, iWidth, iHeight, iDepth, iMipLevels);
        Tex3D_I.GetDimensions(iWidth, iHeight, iDepth);

        Tex3D_F1.GetDimensions(0, fWidth, fHeight, fDepth, fMipLevels);
        Tex3D_F1.GetDimensions(fWidth, fHeight, fDepth);
        Tex3D_U.GetDimensions(0, fWidth, fHeight, fDepth, fMipLevels);
        Tex3D_U.GetDimensions(fWidth, fHeight, fDepth);
        Tex3D_I.GetDimensions(0, fWidth, fHeight, fDepth, fMipLevels);
        Tex3D_I.GetDimensions(fWidth, fHeight, fDepth);
    }

    //TextureCube ~ Texture2D
    {
        uint uWidth, uHeight, uMipLevels;
        int iWidth, iHeight, iMipLevels;
        float fWidth, fHeight, fMipLevels;

        TexC_F1.GetDimensions(0, uWidth, uHeight, uMipLevels);
        TexC_F1.GetDimensions(uWidth, uHeight);
        TexC_I.GetDimensions(0, uWidth, uHeight, uMipLevels);
        TexC_I.GetDimensions(uWidth, uHeight);
        TexC_U.GetDimensions(0, uWidth, uHeight, uMipLevels);
        TexC_U.GetDimensions(uWidth, uHeight);
        TexCS1.GetDimensions(0, uWidth, uHeight, uMipLevels);
        TexCS1.GetDimensions(uWidth, uHeight);

        TexC_F1.GetDimensions(0, iWidth, iHeight, uMipLevels);
        TexC_F1.GetDimensions(iWidth, iHeight);
        TexC_I.GetDimensions(0, iWidth, iHeight, uMipLevels);
        TexC_I.GetDimensions(iWidth, iHeight);
        TexC_U.GetDimensions(0, iWidth, iHeight, uMipLevels);
        TexC_U.GetDimensions(iWidth, iHeight);
        TexCS1.GetDimensions(0, iWidth, iHeight, uMipLevels);
        TexCS1.GetDimensions(iWidth, iHeight);

        TexC_F1.GetDimensions(0, fWidth, fHeight, uMipLevels);
        TexC_F1.GetDimensions(fWidth, fHeight);
        TexC_I.GetDimensions(0, fWidth, fHeight, uMipLevels);
        TexC_I.GetDimensions(fWidth, fHeight);
        TexC_U.GetDimensions(0, fWidth, fHeight, uMipLevels);
        TexC_U.GetDimensions(fWidth, fHeight);
        TexCS1.GetDimensions(0, fWidth, fHeight, uMipLevels);
        TexCS1.GetDimensions(fWidth, fHeight);
    }

#ifndef GL_ES
    //TextureCubeArray ~ Texture2DArray
    {
        uint uWidth, uHeight, uMipLevels, uElems;
        float fWidth, fHeight, fMipLevels, fElems;
        int iWidth, iHeight, iMipLevels, iElems;

        TexC_F_A1.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        TexC_F_A1.GetDimensions(uWidth, uHeight, uElems);
        TexC_I_A.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        TexC_I_A.GetDimensions(uWidth, uHeight, uElems);
        TexC_U_A.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        TexC_U_A.GetDimensions(uWidth, uHeight, uElems);
        TexCAS1.GetDimensions(0, uWidth, uHeight, uElems, uMipLevels);
        TexCAS1.GetDimensions(uWidth, uHeight, uElems);

        TexC_F_A1.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        TexC_F_A1.GetDimensions(iWidth, iHeight, iElems);
        TexC_I_A.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        TexC_I_A.GetDimensions(iWidth, iHeight, iElems);
        TexC_U_A.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        TexC_U_A.GetDimensions(iWidth, iHeight, iElems);
        TexCAS1.GetDimensions(0, iWidth, iHeight, iElems, iMipLevels);
        TexCAS1.GetDimensions(iWidth, iHeight, iElems);

        TexC_F_A1.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        TexC_F_A1.GetDimensions(fWidth, fHeight, fElems);
        TexC_I_A.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        TexC_I_A.GetDimensions(fWidth, fHeight, fElems);
        TexC_U_A.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        TexC_U_A.GetDimensions(fWidth, fHeight, fElems);
        TexCAS1.GetDimensions(0, fWidth, fHeight, fElems, fMipLevels);
        TexCAS1.GetDimensions(fWidth, fHeight, fElems);
    }
#endif


#ifndef GL_ES // This should work on ES3.1, but compiler fails for no reason
    // Texture2DMS
    {
        uint uWidth, uHeight, uNumSamples;
        float fWidth, fHeight, fNumSamples;
        int iWidth, iHeight, iNumSamples;
        Tex2DMS_F1.GetDimensions(uWidth, uHeight, uNumSamples);
        Tex2DMS_I.GetDimensions(uWidth, uHeight, uNumSamples);
        Tex2DMS_U.GetDimensions(uWidth, uHeight, uNumSamples);

        Tex2DMS_F1.GetDimensions(fWidth, fHeight, fNumSamples);
        Tex2DMS_I.GetDimensions(fWidth, fHeight, fNumSamples);
        Tex2DMS_U.GetDimensions(fWidth, fHeight, fNumSamples);

        Tex2DMS_F1.GetDimensions(iWidth, iHeight, iNumSamples);
        Tex2DMS_I.GetDimensions(iWidth, iHeight, iNumSamples);
        Tex2DMS_U.GetDimensions(iWidth, iHeight, iNumSamples);
    }
#endif

#ifndef GL_ES
    // Texture2DMSArray
    {
        uint uWidth, uHeight, uElems, uNumSamples;
        int iWidth, iHeight, iElems, iNumSamples;
        float fWidth, fHeight, fElems, fNumSamples;
        Tex2DMS_F_A1.GetDimensions(uWidth, uHeight, uElems, uNumSamples);
        Tex2DMS_I_A.GetDimensions(uWidth, uHeight, uElems, uNumSamples);
        // OpenGL4.2 only supports 32 texture units and this one is 33rd:
        // Tex2DMS_U_A.GetDimensions(Width, Height, Elems, NumSamples);

        Tex2DMS_F_A1.GetDimensions(fWidth, fHeight, fElems, fNumSamples);
        Tex2DMS_I_A.GetDimensions(fWidth, fHeight, fElems, fNumSamples);

        Tex2DMS_F_A1.GetDimensions(iWidth, iHeight, iElems, iNumSamples);
        Tex2DMS_I_A.GetDimensions(iWidth, iHeight, iElems, iNumSamples);
    }
#endif
}



void TestSample()
{
    float2 f2UV = float2(0.2, 0.3);
    float3 f3UVW = float3(0.2, 0.3, 0.5);
    const int3 Offset = int3(3, 6, 2);
    float4 f4UVWQ = float4(0.2, 0.3, 0.5, 10.0);

#ifndef GL_ES
    // Texture1D 
    Tex1D_F1.Sample(Tex1D_F1_sampler, f3UVW.x);
    Tex1D_F1.Sample(Tex1D_F1_sampler, f3UVW.x, Offset.x);

    // Texture1DArray
    Tex1D_F_A1.Sample(Tex1D_F_A1_sampler, f3UVW.xy);
    Tex1D_F_A1.Sample(Tex1D_F_A1_sampler, f3UVW.xy, Offset.x);
#endif

    //Texture2D
    Tex2D_F1.Sample( Tex2D_F1_sampler, f3UVW.xy ).xyzw.xyzw;
    Tex2D_F1.Sample( Tex2D_F1_sampler, f3UVW.xy, Offset.xy );
    Tex2D_F1.Sample( Tex2D_F1_sampler, float2(0.1, 0.3) );
    Tex2D_F1.Sample( Tex2D_F1_sampler, float2(0.1, (0.3+0.1) ), int2( (3-1) ,5) );

    //Texture2DArray
    Tex2D_F_A1.Sample( Tex2D_F_A1_sampler, f3UVW.xyz );
    Tex2D_F_A1.Sample( Tex2D_F_A1_sampler, f3UVW.xyz, Offset.xy );

    //Texture3D
    Tex3D_F1.Sample(Tex3D_F1_sampler, f3UVW.xyz );
    Tex3D_F1.Sample(Tex3D_F1_sampler, f3UVW.xyz, Offset.xyz );
    
    //TextureCube
    TexC_F1.Sample(TexC_F1_sampler, f3UVW.xyz );
    // Offset not supported

#ifndef GL_ES
    // TextureCubeArray
    TexC_F_A1.Sample( TexC_F_A1_sampler, f4UVWQ.xyzw );
    TexC_F_A1.Sample( TexC_F_A1_sampler, float4(0.5, (0.2+(0.01+0.02)), 0.4, 7.0) );
    // Offset not supported
#endif
}



void TestSampleBias()
{
    float2 f2UV = float2(0.2, 0.3);
    float3 f3UVW = float3(0.2, 0.3, 0.5);
    const int3 Offset = int3(3, 6, 2);
    float4 f4UVWQ = float4(0.2, 0.3, 0.5, 10.0);

#ifndef GL_ES
    // Texture1D 
    Tex1D_F1.SampleBias(Tex1D_F1_sampler, f3UVW.x, 1.5);
    Tex1D_F1.SampleBias(Tex1D_F1_sampler, f3UVW.x, 1.5, Offset.x);

    // Texture1DArray
    Tex1D_F_A1.SampleBias(Tex1D_F_A1_sampler, f3UVW.xy, 1.5);
    Tex1D_F_A1.SampleBias(Tex1D_F_A1_sampler, f3UVW.xy, 1.5, Offset.x);
#endif

    //Texture2D
    Tex2D_F1.SampleBias( Tex2D_F1_sampler, f3UVW.xy, 1.5 );
    Tex2D_F1.SampleBias( Tex2D_F1_sampler, f3UVW.xy, 1.5, Offset.xy );
    Tex2D_F1.SampleBias( Tex2D_F1_sampler, float2(0.1, 0.3), 1.5 );
    Tex2D_F1.SampleBias( Tex2D_F1_sampler, float2(0.1, (0.3+0.1) ), 1.5, int2( (3-1) ,5) );

    //Texture2DArray
    Tex2D_F_A1.SampleBias( Tex2D_F_A1_sampler, f3UVW.xyz, 1.5 );
    Tex2D_F_A1.SampleBias( Tex2D_F_A1_sampler, f3UVW.xyz, 1.5, Offset.xy );

    //Texture3D
    Tex3D_F1.SampleBias(Tex3D_F1_sampler, f3UVW.xyz, 1.5 );
    Tex3D_F1.SampleBias(Tex3D_F1_sampler, f3UVW.xyz, 1.5, Offset.xyz );
    
    //TextureCube
    TexC_F1.SampleBias(TexC_F1_sampler, f3UVW.xyz, 1.5 );
    // Offset not supported

#ifndef GL_ES
    // TextureCubeArray
    TexC_F_A1.SampleBias( TexC_F_A1_sampler, f4UVWQ.xyzw, 1.5 );
    TexC_F_A1.SampleBias( TexC_F_A1_sampler, float4(0.5, (0.2+(0.01+0.02)), 0.4, 7.0), 1.6 );
    // Offset not supported
#endif
}

void TestSampleLevel()
{
    float2 f2UV = float2(0.2, 0.3);
    float3 f3UVW = float3(0.2, 0.3, 0.5);
    float Level = 1.8;
    const int3 Offset = int3(3, 6, 2);
    float4 f4UVWQ = float4(0.2, 0.3, 0.5, 10.0);

#ifndef GL_ES
    // Texture1D 
    Tex1D_F1.SampleLevel(Tex1D_F1_sampler, f3UVW.x, Level);
    Tex1D_F1.SampleLevel(Tex1D_F1_sampler, f3UVW.x, Level, Offset.x);

    // Texture1DArray
    Tex1D_F_A1.SampleLevel(Tex1D_F_A1_sampler, f3UVW.xy, Level);
    Tex1D_F_A1.SampleLevel(Tex1D_F_A1_sampler, f3UVW.xy, Level, Offset.x);
#endif

    //Texture2D
    Tex2D_F1.SampleLevel( Tex2D_F1_sampler, f3UVW.xy, Level );
    Tex2D_F1.SampleLevel( Tex2D_F1_sampler, Tex2D_F1.SampleLevel( Tex2D_F1_sampler, f3UVW.xy, Level ).xy + Tex2D_F1.SampleLevel( Tex2D_F1_sampler, f3UVW.xy, Level ).xy, Level );
    Tex2D_F1.SampleLevel( Tex2D_F1_sampler, f3UVW.xy, Level, Offset.xy );
    Tex2D_F1.SampleLevel( Tex2D_F1_sampler, float2(0.1, 0.3), 4.0 );
    Tex2D_F1.SampleLevel( Tex2D_F1_sampler, float2(0.1, (0.3+0.1) ), 4.0, int2( (3-1) ,5) );

    //Texture2DArray
    Tex2D_F_A1.SampleLevel( Tex2D_F_A1_sampler, f3UVW.xyz, Level );
    Tex2D_F_A1.SampleLevel( Tex2D_F_A1_sampler, f3UVW.xyz, Level, Offset.xy );

    //Texture3D
    Tex3D_F1.SampleLevel(Tex3D_F1_sampler, f3UVW.xyz, Level );
    Tex3D_F1.SampleLevel(Tex3D_F1_sampler, f3UVW.xyz, Level, Offset.xyz );
    
    //TextureCube
    TexC_F1.SampleLevel(TexC_F1_sampler, f3UVW.xyz, Level );
    // Offset not supported

#ifndef GL_ES
    // TextureCubeArray
    TexC_F_A1.SampleLevel( TexC_F_A1_sampler, f4UVWQ.xyzw, Level );
    TexC_F_A1.SampleLevel( TexC_F_A1_sampler, float4(0.5, (0.2+(0.01+0.02)), 0.4, 7.0), 5 );
    // Offset not supported
#endif
}


void TestSampleGrad()
{
    float2 f2UV = float2(0.2, 0.3);
    float2 f2ddxUV = float2(0.01, -0.02);
    float2 f2ddyUV = float2(-0.01, 0.01);
    float3 f3UVW = float3(0.2, 0.3, 0.5);
    float3 f3ddxUVW = float3(-0.02, 0.03, 0.05);
    float3 f3ddyUVW = float3( 0.01, -0.02, 0.02);
    const int3 Offset = int3(3, 6, 2);
    float4 f4UVWQ = float4(0.2, 0.3, 0.5, 10.0);

#ifndef GL_ES
    // Texture1D 
    Tex1D_F1.SampleGrad(Tex1D_F1_sampler, f3UVW.x, f2ddxUV.x, f2ddyUV.x);
    Tex1D_F1.SampleGrad(Tex1D_F1_sampler, f3UVW.x, f2ddxUV.x, f2ddyUV.x, Offset.x);

    // Texture1DArray
    Tex1D_F_A1.SampleGrad(Tex1D_F_A1_sampler, f3UVW.xy, f2ddxUV.x, f2ddyUV.x);
    Tex1D_F_A1.SampleGrad(Tex1D_F_A1_sampler, f3UVW.xy, f2ddxUV.x, f2ddyUV.x, Offset.x);
#endif

    //Texture2D
    Tex2D_F1.SampleGrad( Tex2D_F1_sampler, f3UVW.xy, f2ddxUV.xy, f2ddyUV.xy );
    Tex2D_F1.SampleGrad( Tex2D_F1_sampler, f3UVW.xy, f2ddxUV.xy, f2ddyUV.xy, Offset.xy );
    Tex2D_F1.SampleGrad( Tex2D_F1_sampler, float2(0.1, 0.3), float2(0.01, 0.02), float2(-0.02, 0.01) );
    Tex2D_F1.SampleGrad( Tex2D_F1_sampler, float2(0.1, (0.3+0.1) ), float2(0.01, 0.02), float2(-0.02, 0.01), int2( (3-1) ,5) );

    //Texture2DArray
    Tex2D_F_A1.SampleGrad( Tex2D_F_A1_sampler, f3UVW.xyz, f2ddxUV.xy, f2ddyUV.xy );
    Tex2D_F_A1.SampleGrad( Tex2D_F_A1_sampler, f3UVW.xyz, f2ddxUV.xy, f2ddyUV.xy, Offset.xy );

    //Texture3D
    Tex3D_F1.SampleGrad(Tex3D_F1_sampler, f3UVW.xyz, f3ddxUVW.xyz, f3ddyUVW.xyz );
    Tex3D_F1.SampleGrad(Tex3D_F1_sampler, f3UVW.xyz, f3ddxUVW.xyz, f3ddyUVW.xyz, Offset.xyz );
    
    //TextureCube
    TexC_F1.SampleGrad(TexC_F1_sampler, f3UVW.xyz, f3ddxUVW.xyz, f3ddyUVW.xyz );
    // Offset not supported

#ifndef GL_ES
    // TextureCubeArray
    TexC_F_A1.SampleGrad( TexC_F_A1_sampler, f4UVWQ.xyzw, f3ddxUVW.xyz, f3ddyUVW.xyz );
    TexC_F_A1.SampleGrad( TexC_F_A1_sampler, float4(0.5, (0.2+(0.01+0.02)), 0.4, 7.0), float3(0.01,0.02,0.03), float3(-0.01,-0.02,-0.03) );
    // Offset not supported
#endif
}



void TestSampleCmp()
{
    float3 f3UVW = float3(0.2, 0.3, 0.5);
    const int3 Offset = int3(3, 6, 2);
    float4 f4UVWQ = float4(0.2, 0.3, 0.5, 10.0);
    float CompareVal = 0.7;

#ifndef GL_ES
    // Texture1D 
    Tex1DS1.SampleCmp(Tex1DS1_sampler, f3UVW.x, CompareVal);
    Tex1DS1.SampleCmp(Tex1DS1_sampler, f3UVW.x, CompareVal, Offset.x);

    // Texture1DArray
    Tex1DAS1.SampleCmp(Tex1DAS1_sampler, f3UVW.xy, CompareVal);
    Tex1DAS1.SampleCmp(Tex1DAS1_sampler, f3UVW.xy, CompareVal, Offset.x);
#endif

    //Texture2D
    Tex2DS1.SampleCmp( Tex2DS1_sampler, f3UVW.xy, CompareVal );
    Tex2DS1.SampleCmp( Tex2DS1_sampler, f3UVW.xy, CompareVal, Offset.xy );
    Tex2DS1.SampleCmp( Tex2DS1_sampler, float2(0.1, 0.3), 4.0 );
    Tex2DS1.SampleCmp( Tex2DS1_sampler, float2(0.1, (0.3+0.1) ), 4.0, int2( (3-1) ,5) );
    Tex2DS_F5.SampleCmp( Tex2DS_F5_sampler, f3UVW.xy, CompareVal );

    //Texture2DArray
    Tex2DAS1.SampleCmp( Tex2DAS1_sampler, f3UVW.xyz, CompareVal );
    // This seems to be another bug on Intel driver: the following line does not compile:
    // Tex2DAS1.SampleCmp( Tex2DAS1_sampler, f3UVW.xyz, CompareVal, Offset.xy );

    //TextureCube
    TexCS1.SampleCmp(TexCS1_sampler, f3UVW.xyz, CompareVal );
    // Offset not supported

#ifndef GL_ES
    // TextureCubeArray
    TexCAS1.SampleCmp( TexCAS1_sampler, f4UVWQ.xyzw, CompareVal );
    TexCAS1.SampleCmp( TexCAS1_sampler, float4(0.5, (0.2+(0.01+0.02)), 0.4, 7.0), 0.5 );
    // Offset not supported
#endif
}



void TestSampleCmpLevelZero()
{
    float3 f3UVW = float3(0.2, 0.3, 0.5);
    const int3 Offset = int3(3, 6, 2);
    float4 f4UVWQ = float4(0.2, 0.3, 0.5, 10.0);
    float CompareVal = 0.7;

#ifndef GL_ES
    // Texture1D 
    Tex1DS1.SampleCmpLevelZero(Tex1DS1_sampler, f3UVW.x, CompareVal);
    Tex1DS1.SampleCmpLevelZero(Tex1DS1_sampler, f3UVW.x, CompareVal, Offset.x);

    // Texture1DArray
    Tex1DAS1.SampleCmpLevelZero(Tex1DAS1_sampler, f3UVW.xy, CompareVal);
    Tex1DAS1.SampleCmpLevelZero(Tex1DAS1_sampler, f3UVW.xy, CompareVal, Offset.x);
#endif

    //Texture2D
    Tex2DS1.SampleCmpLevelZero( Tex2DS1_sampler, f3UVW.xy, CompareVal );
    Tex2DS1.SampleCmpLevelZero( Tex2DS1_sampler, f3UVW.xy, CompareVal, Offset.xy );
    Tex2DS1.SampleCmpLevelZero( Tex2DS1_sampler, float2(0.1, 0.3), 4.0 );
    Tex2DS1.SampleCmpLevelZero( Tex2DS1_sampler, float2(0.1, (0.3+0.1) ), 4.0, int2( (3-1) ,5) );

    //Texture2DArray
    Tex2DAS1.SampleCmpLevelZero( Tex2DAS1_sampler, f3UVW.xyz, CompareVal );
    // This seems to be another bug on Intel driver: the following line does not compile:
    // Tex2DAS1.SampleCmpLevelZero( Tex2DAS1_sampler, f3UVW.xyz, CompareVal, Offset.xy );

    //TextureCube
    TexCS1.SampleCmpLevelZero(TexCS1_sampler, f3UVW.xyz, CompareVal );
    // Offset not supported

#ifndef GL_ES
    // TextureCubeArray
    TexCAS1.SampleCmpLevelZero( TexCAS1_sampler, f4UVWQ.xyzw, CompareVal );
    TexCAS1.SampleCmpLevelZero( TexCAS1_sampler, float4(0.5, (0.2+(0.01+0.02)), 0.4, 7.0), 0.5 );
    // Offset not supported
#endif
}



void TestLoad()
{
    int4 Location = int4(2, 5, 1, 10);
    const int3 Offset = int3(5, 10, 20);

#ifndef GL_ES
    // Texture1D 
    {
        Tex1D_F1.Load(Location.xy);
        Tex1D_F1.Load(Location.xy, Offset.x);
        Tex1D_I.Load(Location.xy);
        Tex1D_I.Load(Location.xy, Offset.x);
        Tex1D_U.Load(Location.xy);
        Tex1D_U.Load(Location.xy, Offset.x);
    }

    // Texture1DArray
    {
        Tex1D_F_A1.Load(Location.xyz);
        Tex1D_F_A1.Load(Location.xyz, Offset.x);
        Tex1D_U_A.Load(Location.xyz);
        Tex1D_U_A.Load(Location.xyz, Offset.x);
        Tex1D_I_A.Load(Location.xyz);
        Tex1D_I_A.Load(Location.xyz, Offset.x);
    }
#endif

    //Texture2D
    {
        Tex2D_F1.Load(Location.xyz);
        Tex2D_F1.Load(Location.xyz, Offset.xy);
        Tex2D_F1.Load(Tex2D_I.Load(Location.xyz).xyz + Tex2D_I.Load(Location.xyz).xyz);
        Tex2D_I.Load(Location.xyz);
        Tex2D_I.Load(Location.xyz, Offset.xy);
        Tex2D_U.Load(Location.xyz);
        Tex2D_U.Load(Location.xyz, Offset.xy);
    }

    //Texture2DArray
    {
        Tex2D_F_A1.Load(Location.xyzw);
        Tex2D_F_A1.Load(Location.xyzw, Offset.xy);
        Tex2D_U_A.Load(Location.xyzw);
        Tex2D_U_A.Load(Location.xyzw, Offset.xy);
        Tex2D_I_A.Load(Location.xyzw);
        Tex2D_I_A.Load(Location.xyzw, Offset.xy);
    }

    //Texture3D
    {
        Tex3D_F1.Load(Location.xyzw);
        Tex3D_F1.Load(Location.xyzw, Offset.xyz);
        Tex3D_U.Load(Location.xyzw);
        Tex3D_U.Load(Location.xyzw, Offset.xyz);
        Tex3D_I.Load(Location.xyzw);
        Tex3D_I.Load(Location.xyzw, Offset.xyz);
    }

#ifndef GL_ES // This should work on ES3.1, but compiler fails for no reason
    // Texture2DMS
    {
        Tex2DMS_F1.Load(Location.xy, 1);
        Tex2DMS_F1.Load(Location.xy, 1, Offset.xy);
        Tex2DMS_I.Load(Location.xy, 1);
        Tex2DMS_I.Load(Location.xy, 1, Offset.xy);
        Tex2DMS_U.Load(Location.xy, 1);
        Tex2DMS_U.Load(Location.xy, 1, Offset.xy);
    }
#endif

#ifndef GL_ES
    // Texture2DMSArray
    {
        Tex2DMS_F_A1.Load(Location.xyz, 1);
        Tex2DMS_F_A1.Load(Location.xyz, 1, Offset.xy);
        Tex2DMS_I_A.Load(Location.xyz, 1);
        Tex2DMS_I_A.Load(Location.xyz, 1, Offset.xy);
        Tex2DMS_U_A.Load(Location.xyz, 1);
        Tex2DMS_U_A.Load(Location.xyz, 1, Offset.xy);
    }
#endif
}




void TestGather()
{
#if !GLES30 // no textureGather in GLES3.0
    float4 Location = float4(0.2, 0.5, 0.1, 0.7);
    const int3 Offset = int3(5, 10, 20);
    
    //Texture2D
    {
        Tex2D_F1.Gather(Tex2D_F1_sampler, Location.xy);
        Tex2D_F1.Gather(Tex2D_F1_sampler, Location.xy, Offset.xy);
        //Tex2D_I.Gather(Location.xyz);
        //Tex2D_I.Gather(Location.xyz, Offset.xy);
        //Tex2D_U.Gather(Location.xyz);
        //Tex2D_U.Gather(Location.xyz, Offset.xy);
    }

    //Texture2DArray
    {
        Tex2D_F_A1.Gather(Tex2D_F_A1_sampler, Location.xyz);
        Tex2D_F_A1.Gather(Tex2D_F_A1_sampler, Location.xyz, Offset.xy);
        //Tex2D_U_A.Gather(Location.xyzw);
        //Tex2D_U_A.Gather(Location.xyzw, Offset.xy);
        //Tex2D_I_A.Gather(Location.xyzw);
        //Tex2D_I_A.Gather(Location.xyzw, Offset.xy);
    }

    // TextureCube
    {
        TexC_F1.Gather(TexC_F1_sampler, Location.xyz);
        //TexC_I.Gather(Location.xyz);
        //TexC_U.Gather(Location.xyz);
    }
#ifndef GL_ES 
    // TextureCubeArray
    {
        TexC_F_A1.Gather(TexC_F_A1_sampler, Location.xyzw);
        //TexC_I_A.Gather(Location.xyzw);
        //TexC_U_A.Gather(Location.xyzw);
    }
#endif

#endif
}



void TestGatherCmp()
{
#if !GLES30 // no textureGather in GLES3.0
    float4 Location = float4(0.2, 0.5, 0.1, 0.7);
    const int3 Offset = int3(5, 10, 20);
    float CompareVal = 0.01;

    //Texture2D
    {
        Tex2DS1.GatherCmp(Tex2DS1_sampler, Location.xy, CompareVal);
        Tex2DS1.GatherCmp(Tex2DS1_sampler, Location.xy, CompareVal, Offset.xy);
    }

    //Texture2DArray
    {
        Tex2DAS1.GatherCmp(Tex2DAS1_sampler, Location.xyz, CompareVal);
        Tex2DAS1.GatherCmp(Tex2DAS1_sampler, Location.xyz, CompareVal, Offset.xy);
    }

    // TextureCube
    {
        TexCS1.GatherCmp(TexCS1_sampler, Location.xyz, CompareVal);
    }
#ifndef GL_ES 
    // TextureCubeArray
    {
        TexCAS1.GatherCmp(TexCAS1_sampler, Location.xyzw, CompareVal);
    }
#endif

#endif
}

struct InnerStruct
{
    float f[4];
    int i;
    uint u;
    bool b;
};

struct OuterStruct
{
    InnerStruct inner;
    float f;
    int i;
    uint u;
    bool b;
};

struct VSInputSubStruct
{
    float3 f3Normal : ATTRIB2;
    uint VertexId : SV_VertexId;
};

struct VSInput
{
    float3 f3PosWS : ATTRIB0;
    float2 f2UV  : ATTRIB1;
    VSInputSubStruct SubStruct;
};

struct VSOutputSubStruct
{
    float4 f4Attrib : F4_ATTRIB2;
    int iAttrib   : I_ATTRIB2;
};
struct VSOutput
{
    float2 f2Attrib : F2_ATTRIB;
    float3 f3Attrib : F3_ATTRIB;
    float4 f4Attrib : F4_ATTRIB;
    uint uiAttrib   : UI_ATTRIB;
    uint3 ui3Attrib : UI3_ATTRIB;
    int iAttrib : I_ATTRIB;
    float4 f4PosPS : SV_Position;
    VSOutputSubStruct SubStruct;
};

void TestVS  ( VSInput In,
               in float3 f3UV  : ATTRIB3,
               in uint InstID : SV_InstanceID,
               out VSOutput Out, 
               out float  fAttrib : F_ATTRIB,
               out int4 i4Attrib : I4_ATTRIB)
{
    Out.f4PosPS = float4(1.0, 2.0, 3.0, 4.0);
    fAttrib = 1.0;
    Out.f2Attrib = float2(5.0, 6.0) + In.f2UV;
    Out.f3Attrib = float3(7.0, 8.0, 9.0) + In.f3PosWS + f3UV + In.SubStruct.f3Normal;
    Out.f4Attrib = float4(10.0, 11.0, 12.0, 13.0);
    
    Out.SubStruct.f4Attrib = float4(10.0, 11.0, 12.0, 13.0);
    Out.uiAttrib = 1u;
    Out.ui3Attrib = uint3(2u, 3u, 4u);
    Out.iAttrib   = 5;
    Out.SubStruct.iAttrib = 20;
    i4Attrib = int4(5, 6, 7, 8);

    if( In.f2UV.x < 0.5 )
        return;

    Out.f4Attrib.z = 0.1;
    Out.SubStruct.f4Attrib.zw = float2(12.0, 31.0);
    Out.ui3Attrib.y = 2u;
    i4Attrib.yzw = int3(15, 16, 17);
}


void TestFuncArgs1( Texture2D<float4> Arg1,
                    SamplerState Arg1_sampler,
                    Texture2D<float> Arg2, 
                    SamplerComparisonState Arg2_sampler,
                    Texture3D<float> Arg3)
{
    uint uWidth, uHeight, uMipLevels;
    Arg1.GetDimensions(uWidth, uHeight);

    Arg2.SampleCmp( Arg2_sampler, float2(0.5,0.5), 0.1 );
    
    Arg3.Load( int4(1, 2, 3, 0) );
}

void TestFuncArgs2( Texture3D<int> Arg1,
                    Texture2DArray<float4> Arg2, 
                    SamplerState Arg2_sampler,
                    Texture2D<float> Arg3)
{
    Arg1.Load( int4(1, 2, 3, 0) );

    uint uWidth, uHeight, uElems;
    Arg2.GetDimensions(uWidth, uHeight, uElems);

    Arg3.Load(int3(10,15,3) );
}

struct PSOutputSubStruct
{
    float4 Color4 : SV_Target1;
};
struct PSOutput
{
    float4 Color3 : SV_Target3;
    PSOutputSubStruct substr;
};

void TestPS  ( in VSOutput In,
               out float4 Color : SV_Target,
               out float3 Color2 : SV_Target2,
               out PSOutput Out)
{
    float4 Pos = In.f4PosPS;

    Out.Color3 = float4(0.0, 1.0, 2.0, 3.0);
    Out.substr.Color4 = float4(0.0, 1.0, 2.0, 3.0);

    TestFuncArgs1( Tex2D_F6,
                   Tex2D_F6_sampler,
                   Tex2DS_F5, 
                   Tex2DS_F5_sampler,
                   Tex3D_F3);
    TestFuncArgs2( Tex3D_I,
                   Tex2D_F_A3, 
                   Tex2D_F_A3_sampler,
                   Tex2D_F2);
    {
        float2 a = float2(0.0, 0.0);
        float2 b = float2(1.0, 1.0);
        float2 c = float2(2.0, 2.0);
        a += b;
        a-=b;
        a *=c;
        a/= c;
        float2 d = a;

        int2 x = int2(1, 2);
        int2 y = int2(2, 1);
        x %= y;
        x &= y;
        x |= y;
        x ^= y;
        x >>= y;
        x <<= y;
        x = x | y;
        x = x & y;
        x = x % y;
        x = x ^ y;
        x = ~y;
        x = x >> y;
        x = x << y;
        x++;
        ++x;
        y--;
        --y;
        if( x.x>= y.x || y.y >=x.y && x.y == y.x && y.y==x.y && !(x.x==y.y) )
            x += y;

        [flatten]
        if(x.x==y.x)
            x.x=y.x;

        [branch]
        if(x.x==y.x)
            x.x+=y.x;
        {
            [loop]
            for(int i=0;i<10;++i)
                x.x+=1;
        }

        {
            [unroll]
            for(int i=0;i<10;++i)
                y.x+=1;
        }

        Color =  In.f4Attrib;
        Color2 = In.f3Attrib;
        if( Pos.x < 0.2 ) return;
    }

    {
        int  i1 = 1;
        int2 i2 = int2(1, 2);
        int3 i3 = int3(1, 2, 3);
        int4 i4 = int4(1,2,3,4);

        float  f1 = 1.0;
        float2 f2 = float2(1.0, 2.0);
        float3 f3 = float3(1.0, 2.0, 3.0);
        float4 f4 = float4(1.0,2.0,3.0,4.0);
        float  f1_ = 2.0;
        float2 f2_ = float2(11.0, 12.0);
        float3 f3_ = float3(11.0, 12.0, 13.0);
        float4 f4_ = float4(11.0,12.0,13.0,14.0);

        uint  u1  = 1u;
        uint2 u2 = uint2(1u, 2u);
        uint3 u3 = uint3(1u, 2u, 3u);
        uint4 u4 = uint4(1u, 2u,3u,4u);

        bool  b1 = true;
        bool2 b2 = bool2(true, false);
        bool3 b3 = bool3(true, false, true);
        bool4 b4 = bool4(true, false, true, false);

        i1 = abs ( i1 ); i2 = abs ( i2 ); i3 = abs ( i3 ); i4 = abs ( i4 );
        f1 = abs ( f1 ); f2 = abs ( f2 ); f3 = abs ( f3 ); f4 = abs ( f4 );

                         b1 = all ( b2 ); b1 = all ( b3 ); b1 = all ( b4 );
                         b1 = any ( b2 ); b1 = any ( b3 ); b1 = any ( b4 );

        f1 = ceil( f1 ); f2 = ceil( f2 ); f3 = ceil( f3 ); f4 = ceil( f4 );

        f1 = clamp(f1,f1,f1); f2 = clamp(f2,f2,f2); f3 = clamp(f3,f3,f3); f4 = clamp(f4,f4,f4);
        i1 = clamp(i1,i1,i1); i2 = clamp(i2,i2,i2); i3 = clamp(i3,i3,i3); i4 = clamp(i4,i4,i4);
        u1 = clamp(u1,u1,u1); u2 = clamp(u2,u2,u2); u3 = clamp(u3,u3,u3); u4 = clamp(u4,u4,u4);

        // Trigonometric functions
        f1 = cos( f1 ); f2 = cos( f2 ); f3 = cos( f3 ); f4 = cos( f4 );
        f1 = sin( f1 ); f2 = sin( f2 ); f3 = sin( f3 ); f4 = sin( f4 );
        f1 = tan( f1 ); f2 = tan( f2 ); f3 = tan( f3 ); f4 = tan( f4 );
        f1 = cosh( f1 ); f2 = cosh( f2 ); f3 = cosh( f3 ); f4 = cosh( f4 );
        f1 = sinh( f1 ); f2 = sinh( f2 ); f3 = sinh( f3 ); f4 = sinh( f4 );
        f1 = tanh( f1 ); f2 = tanh( f2 ); f3 = tanh( f3 ); f4 = tanh( f4 );
        f1 = acos( f1 ); f2 = acos( f2 ); f3 = acos( f3 ); f4 = acos( f4 );
        f1 = asin( f1 ); f2 = asin( f2 ); f3 = asin( f3 ); f4 = asin( f4 );
        f1 = atan( f1 ); f2 = atan( f2 ); f3 = atan( f3 ); f4 = atan( f4 );
        f1 = atan2(f1,f1); f2 = atan2(f2,f2); f3 = atan2(f3,f3); f4 = atan2(f4,f4);
        f1 = degrees( f1 ); f2 = degrees( f2 ); f3 = degrees( f3 ); f4 = degrees( f4 );
        f1 = radians( f1 ); f2 = radians( f2 ); f3 = radians( f3 ); f4 = radians( f4 );

        // Exponential functions
        f1 = pow( f1, f1 ); f2 = pow( f2,f2 ); f3 = pow( f3,f3 ); f4 = pow( f4,f4 );
        f1 = exp( f1 ); f2 = exp( f2 ); f3 = exp( f3 ); f4 = exp( f4 );
        f1 = log( f1 ); f2 = log( f2 ); f3 = log( f3 ); f4 = log( f4 );
        f1 = exp2( f1 ); f2 = exp2( f2 ); f3 = exp2( f3 ); f4 = exp2( f4 );
        f1 = log2( f1 ); f2 = log2( f2 ); f3 = log2( f3 ); f4 = log2( f4 );
        f1 = sqrt( f1 ); f2 = sqrt( f2 ); f3 = sqrt( f3 ); f4 = sqrt( f4 );
        f1 = rsqrt( f1 ); f2 = rsqrt( f2 ); f3 = rsqrt( f3 ); f4 = rsqrt( f4 );
        f1 = log10( f1 ); f2 = log10( f2 ); f3 = log10( f3 ); f4 = log10( f4 );

        i1 = sign ( i1 ); i2 = sign ( i2 ); i3 = sign ( i3 ); i4 = sign ( i4 );
        f1 = sign ( f1 ); f2 = sign ( f2 ); f3 = sign ( f3 ); f4 = sign ( f4 );
        f1 = floor ( f1 ); f2 = floor( f2 ); f3 = floor( f3 ); f4 = floor( f4 );
        f1 = trunc ( f1 ); f2 = trunc( f2 ); f3 = trunc( f3 ); f4 = trunc( f4 );
        f1 = round ( f1 ); f2 = round( f2 ); f3 = round( f3 ); f4 = round( f4 );
        f1 = frac  ( f1 ); f2 = frac ( f2 ); f3 = frac ( f3 ); f4 = frac ( f4 );
        
        f1 = 1.0;
        f2 = float2(1.0, 2.0);
        f3 = float3(1.0, 2.0, 3.0);
        f4 = float4(1.0,2.0,3.0,4.0);
        f1 = fmod  ( f1, f1 ); f2 = fmod ( f2, f2 ); f3 = fmod ( f3, f3 ); f4 = fmod ( f4, f4 );
        f1 = modf  ( f1, f1 ); f2 = modf ( f2, f2 ); f3 = modf ( f3, f3 ); f4 = modf ( f4, f4 );

        f1 = min(f1,f1); f2 = min(f2,f2); f3 = min(f3,f3); f4 = min(f4,f4);
        i1 = min(i1,i1); i2 = min(i2,i2); i3 = min(i3,i3); i4 = min(i4,i4);
        u1 = min(u1,u1); u2 = min(u2,u2); u3 = min(u3,u3); u4 = min(u4,u4);
        
        f1 = max(f1,f1); f2 = max(f2,f2); f3 = max(f3,f3); f4 = max(f4,f4);
        i1 = max(i1,i1); i2 = max(i2,i2); i3 = max(i3,i3); i4 = max(i4,i4);
        u1 = max(u1,u1); u2 = max(u2,u2); u3 = max(u3,u3); u4 = max(u4,u4);

        f1 = lerp(f1,f1,f1); f2 = lerp(f2,f2,f2); f3 = lerp(f3,f3,f3); f4 = lerp(f4,f4,f4);
        f1 = step(f1,f1); f2 = step(f2,f2); f3 = step(f3,f3); f4 = step(f4,f4);
        f1 = smoothstep(f1,f1_,f1); f2 = smoothstep(f2,f2_,f2); f3 = smoothstep(f3,f3_,f3); f4 = smoothstep(f4,f4_,f4);

        b1 = isnan ( f1/Pos.x ); b2 = isnan( f2/Pos.x ); b3 = isnan( f3/Pos.x ); b4 = isnan( f4/Pos.x );
        b1 = isinf ( f1/Pos.x ); b2 = isinf( f2/Pos.x ); b3 = isinf( f3/Pos.x ); b4 = isinf( f4/Pos.x );
        b1 = isfinite ( f1/Pos.x ); b2 = isfinite( f2/Pos.x ); b3 = isfinite( f3/Pos.x ); b4 = isfinite( f4/Pos.x );
        f1 = mad(f1,f1,f1); f2 = mad(f2,f2,f2); f3 = mad(f3,f3,f3); f4 = mad(f4,f4,f4);

        f1 = distance(f1,f1); f1 = distance(f2,f2); f1 = distance(f3,f3); f1 = distance(f4,f4);
        f1 = length(f1); f1 = length(f2); f1 = length(f3); f1 = length(f4);
        f1 = dot(f1,f1); f1 = dot(f2,f2); f1 = dot(f3,f3); f1 = dot(f4,f4);
        f3 = cross( f3, f3 );

        f1 = 1.0;
        f2 = float2(1.0, 2.0);
        f3 = float3(1.0, 2.0, 3.0);
        f4 = float4(1.0,2.0,3.0,4.0);
        f1 = normalize(f1); f2 = normalize(f2); f3 = normalize(f3); f4 = normalize(f4);
        f1 = reflect(f1,f1); f2 = reflect(f2,f2); f3 = reflect(f3,f3); f4 = reflect(f4,f4);
        f1 = refract(f1,f1,1.0); f2 = refract(f2,f2,1.0); f3 = refract(f3,f3,1.0); f4 = refract(f4,f4,1.0);
        f1 = faceforward(f1,f1,f1); f2 = faceforward(f2,f2,f2); f3 = faceforward(f3,f3,f3); f4 = faceforward(f4,f4,f4);

        //f1 = dst(f1,f1); f1 = dst(f2,f2); f1 = dst(f3,f3); f1 = dst(f4,f4);
        f1 = rcp(f1); f2 = rcp(f2); f3 = rcp(f3); f4 = rcp(f4);

#if !GLES30 // no bit operations in GLES3.0
        i1 = countbits(u1); i2 = countbits(u2); i3 = countbits(u3); i4 = countbits(u4);
        i1 = countbits(i1); i2 = countbits(i2); i3 = countbits(i3); i4 = countbits(i4);
        i1 = firstbithigh(u1); i2 = firstbithigh(u2); i3 = firstbithigh(u3); i4 = firstbithigh(u4);
        i1 = firstbithigh(i1); i2 = firstbithigh(i2); i3 = firstbithigh(i3); i4 = firstbithigh(i4);
        i1 = firstbitlow(u1); i2 = firstbitlow(u2); i3 = firstbitlow(u3); i4 = firstbitlow(u4);
        i1 = firstbitlow(i1); i2 = firstbitlow(i2); i3 = firstbitlow(i3); i4 = firstbitlow(i4);
        u1 = reversebits(u1); u2 = reversebits(u2); u3 = reversebits(u3); u4 = reversebits(u4);
        i1 = reversebits(i1); i2 = reversebits(i2); i3 = reversebits(i3); i4 = reversebits(i4);
#endif

        f1 = saturate(f1); f2 = saturate(f2); f3 = saturate(f3); f4 = saturate(f4);
        sincos(f1,f1,f1_); sincos(f2,f2,f2_); sincos(f3,f3,f3_); sincos(f4,f4,f4_);

#if !GLES30
        f1 = frexp(f1_, i1); f2 = frexp(f2_, i2); f3 = frexp(f3_, i3); f4 = frexp(f4_, i4);
        f1 = ldexp(f1, i1); f2 = ldexp(f2, i2); f3 = ldexp(f3, i3); f4 = ldexp(f4, i4);
#endif

        float4x4 f4x4;
        f4x4[0] = float4(0.0, 1.0/Pos.x, 2.0/Pos.y, 3.0);
        f4x4[1] = float4(0.0, 1.0/Pos.x, 2.0/Pos.y, 3.0);
        f4x4[2] = float4(0.0, 1.0/Pos.x, 2.0/Pos.y, 3.0);
        f4x4[3] = float4(0.0, 1.0/Pos.x, 2.0/Pos.y, 3.0);
        f4x4 = transpose(f4x4);
        f1 = determinant( f4x4 );

        float3x3 f3x3;
        f3x3[0] = float3(0.0, 1.0/Pos.x, 2.0/Pos.y);
        f3x3[1] = float3(0.0, 1.0/Pos.x, 2.0/Pos.y);
        f3x3[2] = float3(0.0, 1.0/Pos.x, 2.0/Pos.y);
        f3x3 = transpose(f3x3);
        f1 = determinant( f3x3 );

        float2x2 f2x2;
        f2x2[0] = float2(0.0, 1.0/Pos.x);
        f2x2[1] = float2(0.0, 1.0/Pos.x);
        f2x2 = transpose(f2x2);
        f1 = determinant( f2x2 );

        f1 = ddx( Pos.x ); f2 = ddx( Pos.xy ); f3 = ddx( Pos.xyz ); f4 = ddx( Pos.xyzw );
        f1 = ddy( Pos.x ); f2 = ddy( Pos.xy ); f3 = ddy( Pos.xyz ); f4 = ddy( Pos.xyzw );
        f1 = ddx_coarse( Pos.x ); f2 = ddx_coarse( Pos.xy ); f3 = ddx_coarse( Pos.xyz ); f4 = ddx_coarse( Pos.xyzw );
        f1 = ddy_coarse( Pos.x ); f2 = ddy_coarse( Pos.xy ); f3 = ddy_coarse( Pos.xyz ); f4 = ddy_coarse( Pos.xyzw );
        f1 = ddx_fine( Pos.x ); f2 = ddx_fine( Pos.xy ); f3 = ddx_fine( Pos.xyz ); f4 = ddx_fine( Pos.xyzw );
        f1 = ddy_fine( Pos.x ); f2 = ddy_fine( Pos.xy ); f3 = ddy_fine( Pos.xyz ); f4 = ddy_fine( Pos.xyzw );
#ifdef FRAGMENT_SHADER
        f1 = fwidth( Pos.x ); f2 = fwidth( Pos.xy ); f3 = fwidth( Pos.xyz ); f4 = fwidth( Pos.xyzw );
#endif

        f1 = asfloat( f1 ); f2 = asfloat( f2 ); f3 = asfloat( f3 ); f4 = asfloat( f4 );
        f1 = asfloat( i1 ); f2 = asfloat( i2 ); f3 = asfloat( i3 ); f4 = asfloat( i4 );
        f1 = asfloat( u1 ); f2 = asfloat( u2 ); f3 = asfloat( u3 ); f4 = asfloat( u4 );

        i1 = asint( f1 ); i2 = asint( f2 ); i3 = asint( f3 ); i4 = asint( f4 );
        i1 = asint( i1 ); i2 = asint( i2 ); i3 = asint( i3 ); i4 = asint( i4 );
        i1 = asint( u1 ); i2 = asint( u2 ); i3 = asint( u3 ); i4 = asint( u4 );

        u1 = asuint( f1 ); u2 = asuint( f2 ); u3 = asuint( f3 ); u4 = asuint( f4 );
        u1 = asuint( i1 ); u2 = asuint( i2 ); u3 = asuint( i3 ); u4 = asuint( i4 );
        u1 = asuint( u1 ); u2 = asuint( u2 ); u3 = asuint( u3 ); u4 = asuint( u4 );

#if defined(GL_ES) && (__VERSION__>=310) || !defined(GL_ES) && (__VERSION__>=420)
        f1 = f16tof32( u1 ); f2 = f16tof32( u2 ); f3 = f16tof32( u3 ); f4 = f16tof32( u4 );
        f1 = f16tof32( i1 ); f2 = f16tof32( i2 ); f3 = f16tof32( i3 ); f4 = f16tof32( i4 );
        u1 = f32tof16( f1 ); u2 = f32tof16( f2 ); u3 = f32tof16( f3 ); u4 = f32tof16( f4 );
#endif

#ifndef GL_ES
        double d = asdouble( u1, u1 );
#endif

        f1 = noise( f1 ); f2 = noise( f2 ); f3 = noise( f3 ); f4 = noise( f4 );
    }
}
