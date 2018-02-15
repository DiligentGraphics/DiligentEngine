struct VSOut
{
    float2 f2UV	: UV;
};

void main(in float3 f3Position : ATTRIB0,
          in float2 f2UV	: ATTRIB1,
          out float4 f4Position : SV_Position, 
          out VSOut vsOut)
{
	f4Position.xyz = f3Position;
	//Out.f4Position.x += In.fOffset;
	f4Position.w = 1.0;
	vsOut.f2UV = f2UV;
}
