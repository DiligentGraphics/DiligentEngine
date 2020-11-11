cbuffer UniformBuff_Stat
{
    float4 g_Data_Stat;
}

cbuffer UniformBuff_Mut
{
    float4 g_Data_Mut;
}

cbuffer UniformBuff_Dyn
{
    float4 g_Data_Dyn;
}

#if ARRAYS_SUPPORTED
struct CBDataStat
{
    float4 Data;
};
struct CBDataMut
{
    float4 Data;
};
struct CBDataDyn
{
    float4 Data;
};
ConstantBuffer<CBDataStat> UniformBuffArr_Stat[STATIC_CB_ARRAY_SIZE];  // 2
ConstantBuffer<CBDataMut>  UniformBuffArr_Mut [MUTABLE_CB_ARRAY_SIZE]; // 4
ConstantBuffer<CBDataDyn>  UniformBuffArr_Dyn [DYNAMIC_CB_ARRAY_SIZE]; // 3
#endif

float4 UseResources()
{
    float4 f4Color = float4(0.0, 0.0, 0.0, 0.0);
    f4Color += g_Data_Stat;
    f4Color += g_Data_Mut;
    f4Color += g_Data_Dyn;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so
#if ARRAYS_SUPPORTED
    f4Color += UniformBuffArr_Stat[0].Data;
    f4Color += UniformBuffArr_Stat[1].Data;

    f4Color += UniformBuffArr_Mut[0].Data;
#if MUTABLE_CB_ARRAY_SIZE == 4
    f4Color += UniformBuffArr_Mut[1].Data;
    f4Color += UniformBuffArr_Mut[2].Data;
    f4Color += UniformBuffArr_Mut[3].Data;
#endif

    f4Color += UniformBuffArr_Dyn[0].Data;
#if DYNAMIC_CB_ARRAY_SIZE == 3
    f4Color += UniformBuffArr_Dyn[1].Data;
    f4Color += UniformBuffArr_Dyn[2].Data;
#endif

#endif

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
