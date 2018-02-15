
RWBuffer<uint4> bufIndices;

[numthreads(1,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	bufIndices[0] = uint4(0,1,2,3);
}
