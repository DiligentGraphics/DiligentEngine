
// Declare input/output buffer from/to wich we will read/write data.
// In this particular shader we only write data into the buffer.
// If you do not want your data to be aligned by compiler try to use:
// packed or shared instead of std140 keyword.
// We also bind the buffer to index 0. You need to set the buffer binding
// in the range [0..3] – this is the minimum range approved by Khronos.
// Notice that various platforms might support more indices than that.
layout(std140, binding = 3) buffer bufIndirectDispatchArgs
{
	uvec4 data[];
}g_DispatchArgs;


layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
// Declare main program function which is executed once
// glDispatchCompute is called from the application.
void main()
{
	g_DispatchArgs.data[0] = uvec4(0,0,0,0);
	g_DispatchArgs.data[1] = uvec4(1,1,1,0);
}
