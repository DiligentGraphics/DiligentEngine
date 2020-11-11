struct BufferData
{
    float4 data;
};

RWStructuredBuffer<BufferData> g_RWBuff_Static;
RWStructuredBuffer<BufferData> g_RWBuff_Mut;
RWStructuredBuffer<BufferData> g_RWBuff_Dyn;

RWStructuredBuffer<BufferData> g_RWBuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4 or 1 in D3D11
RWStructuredBuffer<BufferData> g_RWBuffArr_Mut   [MUTABLE_BUFF_ARRAY_SIZE]; // 3 or 2 in D3D11
RWStructuredBuffer<BufferData> g_RWBuffArr_Dyn   [DYNAMIC_BUFF_ARRAY_SIZE]; // 2

void UseResources()
{
    float4 f4Data = float4(1.0, 2.0, 3.0, 4.0);
    g_RWBuff_Static[0].data = f4Data;
    g_RWBuff_Mut   [0].data = f4Data;
    g_RWBuff_Dyn   [0].data = f4Data;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    g_RWBuffArr_Static[0][0].data = f4Data;
#if (STATIC_BUFF_ARRAY_SIZE == 4)
    g_RWBuffArr_Static[1][0].data = f4Data;
    g_RWBuffArr_Static[2][0].data = f4Data;
    g_RWBuffArr_Static[3][0].data = f4Data;
#endif

    g_RWBuffArr_Mut[0][0].data = f4Data;
    g_RWBuffArr_Mut[1][0].data = f4Data;
#if (MUTABLE_BUFF_ARRAY_SIZE == 3)
    g_RWBuffArr_Mut[2][0].data = f4Data;
#endif

    g_RWBuffArr_Dyn[0][0].data = f4Data;
    g_RWBuffArr_Dyn[1][0].data = f4Data;
}

[numthreads(1,1,1)]
void main()
{
    UseResources();
}
