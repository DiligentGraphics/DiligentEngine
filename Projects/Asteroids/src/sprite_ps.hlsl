#include "sprite_vs.hlsl"

Texture2D<float4> Tex;// : register(t0);
SamplerState Tex_sampler;// : register(s0);

void sprite_ps(in float4 position : SV_Position,
               in Font_VSOut vs_out,
               out float4 color : SV_Target )
{   
    color = Tex.Sample(Tex_sampler, vs_out.uv);
}
