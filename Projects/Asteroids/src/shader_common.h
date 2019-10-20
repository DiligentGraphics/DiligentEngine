#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

struct VSOut
{
	float3 positionModel : POSITIONMODEL;
	float3 normalWorld   : NORMAL;
	float3 albedo        : ALBEDO;
    uint   textureId     : TEXTURE_ID;
};

#endif
