
layout(rgba8, binding = 2) uniform writeonly image2D g_tex2DTestUAV;

layout (local_size_x = 16, local_size_y = 8, local_size_z = 1) in;
 
// Declare main program function which is executed once
// glDispatchCompute is called from the application.
void main()
{
	ivec2 Dim = imageSize(g_tex2DTestUAV);
	if( gl_GlobalInvocationID.x >= uint(Dim.x) || gl_GlobalInvocationID.y >= uint(Dim.y) )
		return;

	vec2 f2UV = vec2( ivec2(gl_GlobalInvocationID.xy)  % Dim ) / vec2(Dim);
	float DistFromCenter = length(f2UV - vec2(0.5,0.5));
	imageStore(g_tex2DTestUAV, ivec2(gl_GlobalInvocationID.xy), vec4((1.0-DistFromCenter), abs(f2UV.x-0.5), abs(0.5-f2UV.y), 0.0) );
}
