Texture2D g_Tex2D;

SamplerState g_Sam_Static;
SamplerState g_Sam_Mut;
SamplerState g_Sam_Dyn;

SamplerState g_SamArr_Static[STATIC_SAM_ARRAY_SIZE];  // 2
SamplerState g_SamArr_Mut   [MUTABLE_SAM_ARRAY_SIZE]; // 4
SamplerState g_SamArr_Dyn   [DYNAMIC_SAM_ARRAY_SIZE]; // 3

SamplerState g_Sampler;

float4 UseResources()
{
    float2 UV = float2(0.0, 0.0);
    float4 f4Color = float4(0.0, 0.0, 0.0, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_Sam_Static, UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_Sam_Mut, UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_Sam_Dyn, UV.xy, 0.0);

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    f4Color += g_Tex2D.SampleLevel(g_SamArr_Static[0], UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_SamArr_Static[1], UV.xy, 0.0);

    f4Color += g_Tex2D.SampleLevel(g_SamArr_Mut[0], UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_SamArr_Mut[1], UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_SamArr_Mut[2], UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_SamArr_Mut[3], UV.xy, 0.0);

    f4Color += g_Tex2D.SampleLevel(g_SamArr_Dyn[0], UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_SamArr_Dyn[1], UV.xy, 0.0);
    f4Color += g_Tex2D.SampleLevel(g_SamArr_Dyn[2], UV.xy, 0.0);

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
