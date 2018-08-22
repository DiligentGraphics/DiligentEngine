uniform sampler2D g_tex2D_Static;
uniform sampler2D g_tex2D_StaticArr[2];
uniform sampler2D g_tex2D_Mut;
uniform sampler2D g_tex2D_MutArr[2];
uniform sampler2D g_tex2D_Dyn;
uniform sampler2D g_tex2D_DynArr[2];

uniform UniformBuff_Stat
{
	vec4 g_f4Color0;
}g_UniformBuff_Stat;

uniform UniformBuff_Stat2
{
	vec4 g_f4Color01;
}g_UniformBuff_Stat2;

uniform UniformBuff_Mut
{
	vec4 g_f4Color1;
}g_UniformBuff_Mut;

uniform UniformBuff_Dyn
{
	vec4 g_f4Color2;
}g_UniformBuff_Dyn;

uniform samplerBuffer g_Buffer_Static;
uniform samplerBuffer g_Buffer_StaticArr[2];
uniform samplerBuffer g_Buffer_Mut;
uniform samplerBuffer g_Buffer_MutArr[2];
uniform samplerBuffer g_Buffer_Dyn;
uniform samplerBuffer g_Buffer_DynArr[2];

layout(location = 0) out vec4 f4Color;

#ifndef GL_ES
out gl_PerVertex
{
	vec4 gl_Position;
};
#endif

void main()
{
	gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
    f4Color = vec4(0.0, 0.0, 0.0, 0.0);
    f4Color += textureLod(g_tex2D_Static, vec2(0.5,0.5), 0.0);
    f4Color += textureLod(g_tex2D_StaticArr[0], vec2(0.5,0.5), 0.0) + textureLod(g_tex2D_StaticArr[1], vec2(0.5,0.5), 0.0);
    f4Color += textureLod(g_tex2D_Mut, vec2(0.5,0.5), 0.0);
    f4Color += textureLod(g_tex2D_MutArr[0], vec2(0.5,0.5), 0.0) + textureLod(g_tex2D_MutArr[1], vec2(0.5,0.5), 0.0);
    f4Color += textureLod(g_tex2D_Dyn, vec2(0.5,0.5), 0.0);
    f4Color += textureLod(g_tex2D_DynArr[0], vec2(0.5,0.5), 0.0) + textureLod(g_tex2D_DynArr[1], vec2(0.5,0.5), 0.0);
    f4Color += g_UniformBuff_Stat.g_f4Color0 + g_UniformBuff_Stat2.g_f4Color01 + g_UniformBuff_Mut.g_f4Color1 + g_UniformBuff_Dyn.g_f4Color2;
    f4Color += texelFetch(g_Buffer_Static, 0);
    f4Color += texelFetch(g_Buffer_StaticArr[0], 0) + texelFetch(g_Buffer_StaticArr[1],0);
    f4Color += texelFetch(g_Buffer_Mut, 0);
    f4Color += texelFetch(g_Buffer_MutArr[0], 0) + texelFetch(g_Buffer_MutArr[1], 0);
    f4Color += texelFetch(g_Buffer_Dyn, 0);
    f4Color += texelFetch(g_Buffer_DynArr[0], 0) + texelFetch(g_Buffer_DynArr[1], 0);
}
