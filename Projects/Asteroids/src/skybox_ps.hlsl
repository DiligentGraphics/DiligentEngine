struct Skybox_VSOut
{
	float3 coords   : UVFACE;
};

TextureCube Skybox;// : register(t0);
SamplerState Skybox_sampler;// : register(s0);

void skybox_ps(in float4 position : SV_Position,
               in Skybox_VSOut vsoutput,
               out float4 color : SV_Target)
{   
    //return input.coords.xyzx;
    float4 tex = Skybox.Sample(Skybox_sampler, vsoutput.coords);
    color = float4(tex.xyz, 1.0f);
}
