
Texture2D<float4> g_tex2DTest0 : register(t2);
SamplerState g_tex2DTest0_sampler : register(s3);

Texture2D<float4> g_tex2DTest1;
SamplerState g_tex2DTest1_sampler;

Texture2D<float4> g_tex2DTest2;
SamplerState g_tex2DTest2_sampler;

struct VSOut
{
	float4 f4Position : SV_Position;
	float2 f2UV	      : TEXCOORD;
};
 
float4 main(VSOut In) : SV_Target
{
    float4 Col0 = g_tex2DTest0.Sample( g_tex2DTest0_sampler, In.f2UV );
    float4 Col1 = g_tex2DTest1.Sample( g_tex2DTest1_sampler, In.f2UV );
    float4 Col2 = g_tex2DTest2.Sample( g_tex2DTest2_sampler, In.f2UV );
    return float4(Col0.x, Col1.y, Col2.z, 0.0);
}
