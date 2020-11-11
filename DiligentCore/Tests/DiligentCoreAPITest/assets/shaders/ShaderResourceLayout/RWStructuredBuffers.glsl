layout(std140, binding = 0) writeonly buffer g_RWBuff_Static
{
    vec4 data;
}g_StorageBuff_Static;

layout(std140, binding = 1) writeonly buffer g_RWBuff_Mut
{
    vec4 data;
}g_StorageBuff_Mut;

layout(std140, binding = 2) writeonly buffer g_RWBuff_Dyn
{
    vec4 data;
}g_StorageBuff_Dyn;


layout(std140, binding = 3) writeonly buffer g_RWBuffArr_Static
{
    vec4 data;
}g_StorageBuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4 or 1 in OpenGL

layout(std140, binding = 7) writeonly buffer g_RWBuffArr_Mut
{
    vec4 data;
}g_StorageBuffArr_Mut[MUTABLE_BUFF_ARRAY_SIZE]; // 3 or 2 in OpenGL

layout(std140, binding = 10) writeonly buffer g_RWBuffArr_Dyn
{
    vec4 data;
}g_StorageBuffArr_Dyn[DYNAMIC_BUFF_ARRAY_SIZE]; // 2

void UseResources()
{
    vec4 Data = vec4(1.0, 2.0, 3.0, 4.0);
    g_StorageBuff_Static.data = Data;
    g_StorageBuff_Mut   .data = Data;
    g_StorageBuff_Dyn   .data = Data;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    g_StorageBuffArr_Static[0].data = Data;
#if (STATIC_BUFF_ARRAY_SIZE == 4)
    g_StorageBuffArr_Static[1].data = Data;
    g_StorageBuffArr_Static[2].data = Data;
    g_StorageBuffArr_Static[3].data = Data;
#endif

    g_StorageBuffArr_Mut[0].data = Data;
    g_StorageBuffArr_Mut[1].data = Data;
#if (MUTABLE_BUFF_ARRAY_SIZE == 3)
    g_StorageBuffArr_Mut[2].data = Data;
#endif

    g_StorageBuffArr_Dyn[0].data = Data;
    g_StorageBuffArr_Dyn[1].data = Data;
}

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main()
{
    UseResources();
}
