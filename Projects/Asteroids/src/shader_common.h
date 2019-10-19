#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

struct VSOut
{
	float3 positionModel : POSITIONMODEL;
	float3 normalWorld   : NORMAL;
	float3 albedo        : ALBEDO;
    uint   textureId     : TEXTURE_ID;
};

struct AsteroidData
{
	float4x4 World;
	float4x4 ViewProjection;
	float4 SurfaceColor;
	float4 DeepColor;

	uint TextureIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

#endif
