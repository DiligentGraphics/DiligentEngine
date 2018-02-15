
RWBuffer<uint4> bufIndirectDrawArgs;

[numthreads(1,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	bufIndirectDrawArgs[0] = uint4(4,1,0,0);
	bufIndirectDrawArgs[1] = uint4(0,0,0,0);
}
