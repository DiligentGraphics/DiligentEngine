
layout(rgba32f) uniform writeonly imageBuffer bufPositions;
layout(rgba32f) uniform writeonly imageBuffer bufTexcoord;

uniform samplerBuffer Offsets;

layout (local_size_x = 2, local_size_y = 1, local_size_z = 1) in;
// Declare main program function which is executed once
// glDispatchCompute is called from the application.
void main()
{
	int Ind = int(gl_GlobalInvocationID.x);

	vec2 pos[4]; 
	pos[0] = vec2(0.0, 0.0);
    pos[1] = vec2(0.0, 1.0);
    pos[2] = vec2(1.0, 0.0);
    pos[3] = vec2(1.0, 1.0);

	vec2 CurrPos = pos[Ind];

	vec4 Offset = texelFetch(Offsets, Ind);
	CurrPos += Offset.xy;

	CurrPos = CurrPos*vec2(0.3, 0.3) + vec2(0.0, -1.0);

	vec2 uv[4];
	uv[0] = vec2(0.0, 1.0);
	uv[1] = vec2(0.0, 0.0);
	uv[2] = vec2(1.0, 1.0);
	uv[3] = vec2(1.0, 0.0);

	imageStore(bufPositions, Ind, vec4(CurrPos.xy, 0.0, 0.0));
	imageStore(bufTexcoord,  Ind, vec4(uv[Ind].xy, 0.0, 0.0));
}
