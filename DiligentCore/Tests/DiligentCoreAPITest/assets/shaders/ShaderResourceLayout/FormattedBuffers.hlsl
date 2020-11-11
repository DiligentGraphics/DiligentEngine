Buffer g_Buff_Static;
Buffer g_Buff_Mut;
Buffer g_Buff_Dyn;

Buffer g_BuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4
Buffer g_BuffArr_Mut   [MUTABLE_BUFF_ARRAY_SIZE]; // 3
Buffer g_BuffArr_Dyn   [DYNAMIC_BUFF_ARRAY_SIZE]; // 2

float4 UseResources()
{
    float4 f4Color = float4(0.0, 0.0, 0.0, 0.0);
    f4Color += g_Buff_Static.Load(0);
    f4Color += g_Buff_Mut.   Load(0);
    f4Color += g_Buff_Dyn.   Load(0);

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    f4Color += g_BuffArr_Static[0].Load(0);
    f4Color += g_BuffArr_Static[1].Load(0);
    f4Color += g_BuffArr_Static[2].Load(0);
    f4Color += g_BuffArr_Static[3].Load(0);

    f4Color += g_BuffArr_Mut[0].Load(0);
    f4Color += g_BuffArr_Mut[1].Load(0);
    f4Color += g_BuffArr_Mut[2].Load(0);

    f4Color += g_BuffArr_Dyn[0].Load(0);
    f4Color += g_BuffArr_Dyn[1].Load(0);

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
