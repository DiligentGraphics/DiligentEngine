#include "common_defines.h"
#include "shader_common.h"

#ifdef BINDLESS

Texture2DArray<float4> Tex[NUM_UNIQUE_TEXTURES];
SamplerState           Tex_sampler;

#else

Texture2DArray<float4> Tex;
SamplerState           Tex_sampler;

#endif

void asteroid_ps_diligent(in float4 position : SV_Position,
                          in VSOut vs_output,
                          out float4 color : SV_Target)
{
    // Tweaking
    float3 lightPos    = float3(0.5, -0.25, -1);
    bool applyNoise    = true;
    bool applyLight    = true;
    bool applyCoverage = true;

    float3 normal = normalize(vs_output.normalWorld);

    // Triplanar projection
    float3 blendWeights = abs(normalize(vs_output.positionModel));
    float3 uvw = vs_output.positionModel * 0.5f + 0.5f;
    // Tighten up the blending zone
    blendWeights = saturate((blendWeights - 0.2f) * 7.0f);
    blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z).xxx;

    float3 coords1 = float3(uvw.yz, 0);
    float3 coords2 = float3(uvw.zx, 1);
    float3 coords3 = float3(uvw.xy, 2);

    // TODO: Should really branch out zero'd weight ones, but FXC is being a pain
    // and forward substituting the above and then refusing to compile "divergent"
    // coordinates...
    float3 detailTex = float3(0.0, 0.0, 0.0);
#ifdef BINDLESS
    detailTex += blendWeights.x * Tex[vs_output.textureId].Sample(Tex_sampler, coords1).xyz;
    detailTex += blendWeights.y * Tex[vs_output.textureId].Sample(Tex_sampler, coords2).xyz;
    detailTex += blendWeights.z * Tex[vs_output.textureId].Sample(Tex_sampler, coords3).xyz;
#else
    detailTex += blendWeights.x * Tex.Sample(Tex_sampler, coords1).xyz;
    detailTex += blendWeights.y * Tex.Sample(Tex_sampler, coords2).xyz;
    detailTex += blendWeights.z * Tex.Sample(Tex_sampler, coords3).xyz;
#endif

    float wrap = 0.0f;
    float wrap_diffuse = saturate((dot(normal, normalize(lightPos)) + wrap) / (1.0f + wrap));
    float light = 3.0f * wrap_diffuse + 0.06f;

    // Approximate partial coverage on distant asteroids (by fading them out)
    float coverage = saturate(position.z * 4000.0f);
    
    color.rgb = vs_output.albedo.rgb;
    [flatten] if (applyNoise)    color.rgb = color.rgb * (2.0f * detailTex);
    [flatten] if (applyLight)    color.rgb = color.rgb * light;
    [flatten] if (applyCoverage) color.rgb = color.rgb * coverage;
    color.a = 1.0;
}
