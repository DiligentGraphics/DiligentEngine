layout(std140) readonly buffer g_Buff_Static
{
    vec4 data;
}g_StorageBuff_Static;

layout(std140) readonly buffer g_Buff_Mut
{
    vec4 data;
}g_StorageBuff_Mut;

layout(std140) readonly buffer g_Buff_Dyn
{
    vec4 data;
}g_StorageBuff_Dyn;


#ifdef FRAGMENT_SHADER
// Vulkan only allows 16 dynamic storage buffer bindings among all stages, so
// use arrays only in fragment shader
layout(std140) readonly buffer g_BuffArr_Static
{
    vec4 data;
}g_StorageBuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4

layout(std140) readonly buffer g_BuffArr_Mut
{
    vec4 data;
}g_StorageBuffArr_Mut[MUTABLE_BUFF_ARRAY_SIZE]; // 3

layout(std140) readonly buffer g_BuffArr_Dyn
{
    vec4 data;
}g_StorageBuffArr_Dyn[DYNAMIC_BUFF_ARRAY_SIZE]; // 2
#endif

vec4 UseResources()
{
    vec4 f4Color = vec4(0.0, 0.0, 0.0, 0.0);
    f4Color += g_StorageBuff_Static.data;
    f4Color += g_StorageBuff_Mut   .data;
    f4Color += g_StorageBuff_Dyn   .data;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so
#ifdef FRAGMENT_SHADER
    f4Color += g_StorageBuffArr_Static[0].data;
    f4Color += g_StorageBuffArr_Static[1].data;
    f4Color += g_StorageBuffArr_Static[2].data;
    f4Color += g_StorageBuffArr_Static[3].data;

    f4Color += g_StorageBuffArr_Mut[0].data;
    f4Color += g_StorageBuffArr_Mut[1].data;
    f4Color += g_StorageBuffArr_Mut[2].data;

    f4Color += g_StorageBuffArr_Dyn[0].data;
    f4Color += g_StorageBuffArr_Dyn[1].data;
#endif    
	return f4Color;
}

#ifdef VERTEX_SHADER

//To use any built-in input or output in the gl_PerVertex and
//gl_PerFragment blocks in separable program objects, shader code must
//redeclare those blocks prior to use. 
//
// Declaring this block causes compilation error on NVidia GLES
#ifndef GL_ES
out gl_PerVertex
{
	vec4 gl_Position;
};
#endif

layout(location = 0)out vec4 out_Color;
void main()
{
    out_Color = UseResources();
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}   
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0)in vec4 in_Color;
layout(location = 0)out vec4 out_Color;
void main()
{
    out_Color = UseResources();
}
#endif
