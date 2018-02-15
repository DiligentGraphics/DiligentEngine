// Vertex Shader – file "minimal.vert"
 
layout(location = 0) in  vec3 in_Position;
layout(location = 1) in  vec3 in_Color;
layout(location = 2) in  vec2 in_Offset;

layout(location = 1) out vec3 ex_Color;

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
	gl_Position = vec4(in_Position + vec3(in_Offset.xy,0.0), 1.0);
}

