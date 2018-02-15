
struct VSIn
{
	float3 f3Position : ATTRIB0;
	float3 f3Color	: ATTRIB1;
	float2 f2Offset :  ATTRIB2;
};

struct VSOut
{
	float4 f4Position : SV_Position;
	float3 f3Color	: COLOR;
};

VSOut main(VSIn In)
{
	VSOut Out;
	Out.f4Position.xyz = In.f3Position;
	Out.f4Position.xy += In.f2Offset;
	Out.f4Position.w = 1;
	Out.f3Color = In.f3Color;
	return Out;
}
