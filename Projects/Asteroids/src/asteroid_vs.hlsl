#include "shader_common.h"

#ifdef BINDLESS

StructuredBuffer<AsteroidData> g_Data;

#else

cbuffer DrawConstantBuffer
{
    AsteroidData g_Data;
};

#endif

float linstep(float min, float max, float s)
{
    return saturate((s - min) / (max - min));
}


void asteroid_vs(in	float3 in_pos      : ATTRIB0,
                 in	float3 in_normal   : ATTRIB1,
#ifdef BINDLESS
                 in uint   AsteroidId  : ATTRIB2, // SV_InstanceId is not affected by BaseInstance
#endif
                 out float4 position   : SV_Position,
                 out VSOut vs_output)
{
#ifdef BINDLESS
    AsteroidData Data = g_Data[AsteroidId];
#else
    AsteroidData Data = g_Data;
#endif

    float3 positionWorld = mul(Data.World, float4(in_pos, 1.0f)).xyz;
    position = mul(Data.ViewProjection, float4(positionWorld, 1.0f));

    vs_output.positionModel = in_pos;
    vs_output.normalWorld = mul(Data.World, float4(in_normal, 0.0f)).xyz; // No non-uniform scaling
    
    float depth = linstep(0.5f, 0.7f, length(in_pos.xyz));
    vs_output.albedo = lerp(Data.DeepColor.xyz, Data.SurfaceColor.xyz, depth);

    vs_output.textureId = Data.TextureIndex;
}
