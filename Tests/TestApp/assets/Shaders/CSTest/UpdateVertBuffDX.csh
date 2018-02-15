
RWBuffer<float4> bufPositions : register(u3);
RWBuffer<float4> bufTexcoord;
Buffer<float4> Offsets;

[numthreads(2,1,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int Ind = int(DTid.x);

	float2 pos[4] = 
	{
		float2(0.0, 0.0),
        float2(0.0, 1.0),
        float2(1.0, 0.0),
        float2(1.0, 1.0)
	};

	float2 CurrPos = pos[Ind];
	float2 Offset = Offsets.Load(Ind);
	CurrPos += Offset;

	CurrPos = CurrPos*float2(0.3, 0.3) + float2(0.0, -1.0);

	float2 uv[4] =
	{
		float2(0.0, 1.0),
		float2(0.0, 0.0),
		float2(1.0, 1.0),
		float2(1.0, 0.0)
	};

	bufPositions[Ind] = float4(CurrPos.xy, 0.0, 0.0);
	bufTexcoord[Ind]  = float4(uv[Ind],0,0);
}
