#include "shader_common.h"

Texture2DArray<float4> Tex : register(t0);
sampler Sampler : register(s0);

float4 asteroid_ps_d3d11(in float4 position : SV_Position,
                         VSOut input) : SV_Target
{
    // Tweaking
    float3 lightPos    = float3(0.5, -0.25, -1);
    bool applyNoise    = true;
    bool applyLight    = true;
    bool applyCoverage = true;

    float3 normal = normalize(input.normalWorld);

    // Triplanar projection
    float3 blendWeights = abs(normalize(input.positionModel));
    float3 uvw = input.positionModel * 0.5f + 0.5f;
    // Tighten up the blending zone
    blendWeights = saturate((blendWeights - 0.2f) * 7.0f);
    blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z).xxx;

    float3 coords1 = float3(uvw.yz, 0);
    float3 coords2 = float3(uvw.zx, 1);
    float3 coords3 = float3(uvw.xy, 2);

    // TODO: Should really branch out zero'd weight ones, but FXC is being a pain
    // and forward substituting the above and then refusing to compile "divergent"
    // coordinates...
    float3 detailTex = 0.0f;
    detailTex += blendWeights.x * Tex.Sample(Sampler, coords1).xyz;
    detailTex += blendWeights.y * Tex.Sample(Sampler, coords2).xyz;
    detailTex += blendWeights.z * Tex.Sample(Sampler, coords3).xyz;

    float wrap = 0.0f;
    float wrap_diffuse = saturate((dot(normal, normalize(lightPos)) + wrap) / (1.0f + wrap));
    float light = 3.0f * wrap_diffuse + 0.06f;

    // Approximate partial coverage on distant asteroids (by fading them out)
    float coverage = saturate(position.z * 4000.0f);
    
    float3 color = input.albedo;
    [flatten] if (applyNoise)    color = color * (2.0f * detailTex);
    [flatten] if (applyLight)    color = color * light;
    [flatten] if (applyCoverage) color = color * coverage;
    return float4(color, 1.0f);
}
