

layout(rgba32ui, binding = 3) uniform writeonly uimageBuffer bufIndirectDrawArgs;

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
// Declare main program function which is executed once
// glDispatchCompute is called from the application.
void main()
{
    imageStore(bufIndirectDrawArgs, 0, uvec4(4,1,0,0));
    imageStore(bufIndirectDrawArgs, 1, uvec4(0,0,0,0));
}
