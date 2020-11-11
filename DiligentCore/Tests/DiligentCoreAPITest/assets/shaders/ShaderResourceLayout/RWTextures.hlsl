RWTexture2D<float4 /*format=rgba32f*/> g_RWTex2D_Static;
RWTexture2D<float4 /*format=rgba32f*/> g_RWTex2D_Mut;
RWTexture2D<float4 /*format=rgba32f*/> g_RWTex2D_Dyn;

RWTexture2D<float4 /*format=rgba32f*/> g_RWTex2DArr_Static[STATIC_TEX_ARRAY_SIZE];  // 2
RWTexture2D<float4 /*format=rgba32f*/> g_RWTex2DArr_Mut   [MUTABLE_TEX_ARRAY_SIZE]; // 4 or 2 in D3D11
RWTexture2D<float4 /*format=rgba32f*/> g_RWTex2DArr_Dyn   [DYNAMIC_TEX_ARRAY_SIZE]; // 3 or 1 in D3D11

void UseResources()
{
    float4 f4Color = float4(1.0, 2.0, 3.0, 4.0);
    g_RWTex2D_Static[int2(0,0)] = f4Color;
    g_RWTex2D_Mut   [int2(0,0)] = f4Color;
    g_RWTex2D_Dyn   [int2(0,0)] = f4Color;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    g_RWTex2DArr_Static[0][int2(0,0)] = f4Color;
    g_RWTex2DArr_Static[1][int2(0,0)] = f4Color;

    g_RWTex2DArr_Mut[0][int2(0,0)] = f4Color;
    g_RWTex2DArr_Mut[1][int2(0,0)] = f4Color;
#if (MUTABLE_TEX_ARRAY_SIZE == 4)
    g_RWTex2DArr_Mut[2][int2(0,0)] = f4Color;
    g_RWTex2DArr_Mut[3][int2(0,0)] = f4Color;
#endif

    g_RWTex2DArr_Dyn[0][int2(0,0)] = f4Color;
#if (DYNAMIC_TEX_ARRAY_SIZE == 3)
    g_RWTex2DArr_Dyn[1][int2(0,0)] = f4Color;
    g_RWTex2DArr_Dyn[2][int2(0,0)] = f4Color;
#endif
}

[numthreads(1,1,1)]
void main()
{
    UseResources();
}
