
struct VSIn
{
	uint uiVertexId : SV_VertexID;
};

struct VSOutput
{
	float4 f4Position : SV_Position;
	float3 f3Color	: COLOR;
};

void main(VSIn In,
          out VSOutput Out)
{
    float4 Positions[2];
    Positions[0] = float4(-0.5,0.15,0.0,1.0);
    Positions[1] = float4(-0.65,0.65,0.0,1.0);
    float3 Color[2];
    Color[0] = float3(1.0,0.0,0.0);
    Color[1] = float3(0.0,1.0,0.0);
	Out.f4Position = Positions[In.uiVertexId];
    Out.f3Color = Color[In.uiVertexId];
}
