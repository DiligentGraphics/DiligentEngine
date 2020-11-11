Texture2D g_Tex2D_Static;
Texture2D g_Tex2D_Mut;
Texture2D g_Tex2D_Dyn;

Texture2D g_Tex2DArr_Static[STATIC_TEX_ARRAY_SIZE];  // 2
Texture2D g_Tex2DArr_Mut   [MUTABLE_TEX_ARRAY_SIZE]; // 4
Texture2D g_Tex2DArr_Dyn   [DYNAMIC_TEX_ARRAY_SIZE]; // 3

SamplerState g_Tex2D_Static_sampler;
SamplerState g_Tex2D_Mut_sampler;
SamplerState g_Tex2D_Dyn_sampler;

SamplerState g_Tex2DArr_Static_sampler;
SamplerState g_Tex2DArr_Mut_sampler   [MUTABLE_TEX_ARRAY_SIZE];
SamplerState g_Tex2DArr_Dyn_sampler   [DYNAMIC_TEX_ARRAY_SIZE];


float4 UseResources()
{
    float2 UV = float2(0.0, 0.0);
    float4 f4Color = float4(0.0, 0.0, 0.0, 0.0);
    f4Color += g_Tex2D_Static.SampleLevel(g_Tex2D_Static_sampler, UV.xy, 0.0);
    f4Color += g_Tex2D_Mut.   SampleLevel(g_Tex2D_Mut_sampler,     UV.xy, 0.0);
    f4Color += g_Tex2D_Dyn.   SampleLevel(g_Tex2D_Dyn_sampler,    UV.xy, 0.0);

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    f4Color += g_Tex2DArr_Static[0].SampleLevel(g_Tex2DArr_Static_sampler, UV.xy, 0.0);
    f4Color += g_Tex2DArr_Static[1].SampleLevel(g_Tex2DArr_Static_sampler, UV.xy, 0.0);

    f4Color += g_Tex2DArr_Mut[0].SampleLevel(g_Tex2DArr_Mut_sampler[0],  UV.xy, 0.0);
    f4Color += g_Tex2DArr_Mut[1].SampleLevel(g_Tex2DArr_Mut_sampler[1],  UV.xy, 0.0);
    f4Color += g_Tex2DArr_Mut[2].SampleLevel(g_Tex2DArr_Mut_sampler[2],  UV.xy, 0.0);
    f4Color += g_Tex2DArr_Mut[3].SampleLevel(g_Tex2DArr_Mut_sampler[3],  UV.xy, 0.0);

    f4Color += g_Tex2DArr_Dyn[0].SampleLevel(g_Tex2DArr_Dyn_sampler[0],  UV.xy, 0.0);
    f4Color += g_Tex2DArr_Dyn[1].SampleLevel(g_Tex2DArr_Dyn_sampler[1],  UV.xy, 0.0);
    f4Color += g_Tex2DArr_Dyn[2].SampleLevel(g_Tex2DArr_Dyn_sampler[2],  UV.xy, 0.0);

	return f4Color;
}

void VSMain(out float4 f4Color    : COLOR,
            out float4 f4Position : SV_Position)
{
    f4Color = UseResources();
    f4Position = float4(0.0, 0.0, 0.0, 1.0);
}

float4 PSMain(in float4 in_f4Color : COLOR,
              in float4 f4Position : SV_Position) : SV_Target
{
    return in_f4Color + UseResources();
}
