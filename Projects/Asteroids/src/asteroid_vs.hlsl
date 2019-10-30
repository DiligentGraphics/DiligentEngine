#include "shader_common.h"

cbuffer DrawConstantBuffer
{
	float4x4 mWorld;
	float4x4 mViewProjection;
	float4 mSurfaceColor;
	float4 mDeepColor;
	uint mTextureIndex;
};

float linstep(float min, float max, float s)
{
    return saturate((s - min) / (max - min));
}

void asteroid_vs(in float3 in_pos      : ATTRIB0,
                 in float3 in_normal   : ATTRIB1,
                 out float4 position   : SV_Position,
                 out VSOut vs_output)
{
    float3 positionWorld = mul(mWorld, float4(in_pos, 1.0f)).xyz;
    position = mul(mViewProjection, float4(positionWorld, 1.0f));

    vs_output.positionModel = in_pos;
    vs_output.normalWorld = mul(mWorld, float4(in_normal, 0.0f)).xyz; // No non-uniform scaling
    
    float depth = linstep(0.5f, 0.7f, length(in_pos.xyz));
    vs_output.albedo = lerp(mDeepColor.xyz, mSurfaceColor.xyz, depth);
    vs_output.textureId = mTextureIndex;
}
