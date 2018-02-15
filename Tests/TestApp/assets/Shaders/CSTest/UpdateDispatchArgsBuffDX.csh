
RWBuffer<uint4> bufIndirectDispatchArgs;

[numthreads(1,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	bufIndirectDispatchArgs[0] = uint4(0,0,0,0);
	bufIndirectDispatchArgs[1] = uint4(1,1,1,0);
}
