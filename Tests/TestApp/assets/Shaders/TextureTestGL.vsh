// Vertex Shader – file "minimal.vert"
 
layout(location = 0) in  vec3 in_Position;
layout(location = 1) in  vec2 in_UV;

layout(location = 1) out vec2 out_UV;


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
	out_UV = in_UV;
	gl_Position = vec4(in_Position, 1.0);
}
