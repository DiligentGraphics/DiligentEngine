
// Declare input/output buffer from/to wich we will read/write data.
// In this particular shader we only write data into the buffer.
// If you do not want your data to be aligned by compiler try to use:
// packed or shared instead of std140 keyword.
// We also bind the buffer to index 0. You need to set the buffer binding
// in the range [0..3] – this is the minimum range approved by Khronos.
// Notice that various platforms might support more indices than that.
layout(std140, binding = 2) buffer bufPositions
{
	vec4 data[];
}g_Positions;

layout(std140) buffer bufTexcoord
{
	vec4 data[];
}g_Texcoords;


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
	CurrPos = CurrPos*vec2(0.3, 0.3) + vec2(0.0, -1.0);

	vec2 uv[4];
	uv[0] = vec2(0.0, 1.0);
	uv[1] = vec2(0.0, 0.0);
	uv[2] = vec2(1.0, 1.0);
	uv[3] = vec2(1.0, 0.0);

	g_Positions.data[Ind] = vec4(CurrPos.xy, 0.0, 0.0);
	g_Texcoords.data[Ind] = vec4(uv[Ind], 0.0, 0.0);
}
