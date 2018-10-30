
uniform sampler2D g_tex2D_Dyn;
uniform sampler2D g_tex2DArr_Dyn[8];
uniform sampler2D g_tex2D_Static;
uniform sampler2D g_tex2DArr_Static[2];
uniform sampler2D g_tex2D_Mut;
uniform sampler2D g_tex2DArr_Mut[3];

uniform texture2D g_sepTex2D_static;
uniform texture2D g_sepTex2DArr_static[2];
uniform texture2D g_sepTex2D_mut;
uniform texture2D g_sepTex2DArr_mut[3];
uniform texture2D g_sepTex2D_dyn;
uniform texture2D g_sepTex2DArr_dyn[4];

uniform sampler g_Sam_static;
uniform sampler g_SamArr_static[2];
uniform sampler g_Sam_mut;
uniform sampler g_SamArr_mut[3];
uniform sampler g_SamArr_dyn[4];
uniform sampler g_Sam_dyn;

uniform samplerBuffer g_UniformTexelBuff;
layout(rgba32f) uniform writeonly imageBuffer g_StorageTexelBuff;
uniform samplerBuffer g_UniformTexelBuff_mut;
layout(rgba32f) uniform writeonly imageBuffer g_StorageTexelBuff_mut;

uniform UniformBuff_Dyn
{
    vec2 UV;
}g_UniformBuff_Dyn;

uniform UniformBuffArr_Dyn
{
    vec2 UV;
}g_UniformBuffArr_Dyn[4];

uniform UniformBuff_Stat
{
    vec2 UV;
}g_UniformBuff_Stat;

uniform UniformBuffArr_Stat
{
    vec2 UV;
}g_UniformBuffArr_Stat[2];


uniform UniformBuff_Mut
{
    vec2 UV;
}g_UniformBuff_Mut;

uniform UniformBuffArr_Mut
{
    vec2 UV;
}g_UniformBuffArr_Mut[3];




layout(std140) buffer storageBuff_Dyn
{
    vec4 UV[];
}g_StorageBuff_Dyn;

layout(std140) buffer storageBuffArr_Dyn
{
    vec4 UV[];
}g_StorageBuffArr_Dyn[4];

layout(std140) buffer storageBuff_Static
{
    vec4 UV[];
}g_StorageBuff_Stat;

layout(std140) buffer storageBuffArr_Static
{
    vec4 UV[];
}g_StorageBuffArr_Stat[2];

layout(std140) buffer storageBuff_Mut
{
    vec4 UV[];
}g_StorageBuff_Mut;

layout(std140) buffer storageBuffArr_Mut
{
    vec4 UV[];
}g_StorageBuffArr_Mut[3];

layout(rgba8) uniform writeonly image2D g_tex2DStorageImg_Stat;
layout(rgba8) uniform writeonly image2D g_tex2DStorageImgArr_Mut[2];
layout(rgba8) uniform writeonly image2D g_tex2DStorageImgArr_Dyn[2];

layout(location = 0) out vec4 out_Color;

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

void main(void)
{
	out_Color = vec4(0.0, 0.0, 0.0, 0.0);

    out_Color += textureLod(g_tex2D_Dyn, g_UniformBuff_Dyn.UV + g_StorageBuff_Dyn.UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[0], g_UniformBuffArr_Dyn[0].UV + g_StorageBuffArr_Dyn[0].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[1], g_UniformBuffArr_Dyn[1].UV + g_StorageBuffArr_Dyn[1].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[2], g_UniformBuffArr_Dyn[2].UV + g_StorageBuffArr_Dyn[2].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[3], g_UniformBuffArr_Dyn[3].UV + g_StorageBuffArr_Dyn[3].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[4], g_UniformBuffArr_Dyn[0].UV + g_StorageBuffArr_Dyn[0].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[5], g_UniformBuffArr_Dyn[1].UV + g_StorageBuffArr_Dyn[1].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[6], g_UniformBuffArr_Dyn[2].UV + g_StorageBuffArr_Dyn[2].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Dyn[7], g_UniformBuffArr_Dyn[3].UV + g_StorageBuffArr_Dyn[3].UV[0].xy, 0.0);

    out_Color += textureLod(g_tex2D_Static, g_UniformBuff_Stat.UV + g_StorageBuff_Stat.UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Static[0], g_UniformBuffArr_Stat[0].UV + g_StorageBuffArr_Stat[0].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Static[1], g_UniformBuffArr_Stat[1].UV + g_StorageBuffArr_Stat[1].UV[0].xy, 0.0);

    out_Color += textureLod(g_tex2D_Mut, g_UniformBuff_Mut.UV + g_StorageBuff_Mut.UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Mut[0], g_UniformBuffArr_Mut[0].UV + g_StorageBuffArr_Mut[0].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Mut[1], g_UniformBuffArr_Mut[1].UV + g_StorageBuffArr_Mut[1].UV[0].xy, 0.0);
    out_Color += textureLod(g_tex2DArr_Mut[2], g_UniformBuffArr_Mut[2].UV + g_StorageBuffArr_Mut[2].UV[0].xy, 0.0);

    out_Color += textureLod(sampler2D(g_sepTex2D_static, g_Sam_static), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_static[0], g_SamArr_static[0]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_static[1], g_SamArr_static[1]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2D_mut, g_Sam_mut), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_mut[0], g_SamArr_mut[0]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_mut[1], g_SamArr_mut[1]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_mut[2], g_SamArr_mut[2]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2D_dyn, g_SamArr_dyn[0]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2D_dyn, g_SamArr_dyn[1]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2D_dyn, g_SamArr_dyn[2]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2D_dyn, g_SamArr_dyn[3]), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_dyn[0], g_Sam_dyn), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_dyn[1], g_Sam_dyn), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_dyn[2], g_Sam_dyn), vec2(0.5, 0.5), 0.0);
    out_Color += textureLod(sampler2D(g_sepTex2DArr_dyn[3], g_Sam_dyn), vec2(0.5, 0.5), 0.0);

    imageStore(g_tex2DStorageImg_Stat, ivec2(0,0), vec4(1.0, 2.0, 3.0, 4.0));
    imageStore(g_tex2DStorageImgArr_Mut[0], ivec2(0, 0), vec4(1.0, 2.0, 3.0, 4.0));
    imageStore(g_tex2DStorageImgArr_Mut[1], ivec2(0, 0), vec4(1.0, 2.0, 3.0, 4.0));
    imageStore(g_tex2DStorageImgArr_Dyn[0], ivec2(0, 0), vec4(1.0, 2.0, 3.0, 4.0));
    imageStore(g_tex2DStorageImgArr_Dyn[1], ivec2(0, 0), vec4(1.0, 2.0, 3.0, 4.0));

	gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}
