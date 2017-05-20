#include "sprite_vs.hlsl"

Texture2D Tex;// : register(t0);
SamplerState Tex_sampler;// : register(s0);

void font_ps(in float4 position : SV_Position,
             in Font_VSOut vs_out,
             out float4 color : SV_Target )
{   
    float alpha = Tex.Sample(Tex_sampler, vs_out.uv).r;
    color = float4(1, 1, 1, 1) * alpha;
}
