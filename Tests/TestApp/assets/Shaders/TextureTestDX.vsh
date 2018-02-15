
struct VSIn
{
	float3 f3Position : ATTRIB0;
	float2 f2UV	: ATTRIB1;
};

struct VSOut
{
	float4 f4Position : SV_Position;
	float2 f2UV	: UV;
};

VSOut main(VSIn In)
{
	VSOut Out;
	Out.f4Position.xyz = In.f3Position;
	//Out.f4Position.x += In.fOffset;
	Out.f4Position.w = 1;
	Out.f2UV = In.f2UV;
	return Out;
}
