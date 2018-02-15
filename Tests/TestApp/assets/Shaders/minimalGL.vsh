// Vertex Shader – file "minimal.vert"
 
layout(location = 0) in  vec3 in_Position;
layout(location = 1) in  vec3 in_Color;

layout(location = 1) out vec3 ex_Color;

uniform cbTestBlock3
{
	vec4 Scale;
}g_TestBlock3;

uniform cbTestBlock4
{
	vec4 Scale;
}g_TestBlock4;

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
	ex_Color = in_Color;
	gl_Position = vec4(in_Position.x * g_TestBlock3.Scale.x, in_Position.y * g_TestBlock4.Scale.y, in_Position.z, 1.0);
}

