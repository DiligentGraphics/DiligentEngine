#ifndef COMMON_HLSL
#define COMMON_HLSL

struct VSIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct VSOut
{
    float4 position      : SV_Position;
    float3 positionModel : POSITIONMODEL;
    float3 normalWorld   : NORMAL;
    float3 albedo        : ALBEDO; // Alternatively, can pass just "ao" to PS and read cbuffer in PS
};


struct Skybox_VSIn
{
    float3 position : POSITION;
    float3 uvFace   : UVFACE;
};

struct Skybox_VSOut
{
    float4 position : SV_Position;
    float3 coords   : UVFACE;
};


struct Font_VSIn
{
    float2 position : POSITION;
    float2 uv       : UV;
};

struct Font_VSOut
{
    float4 position : SV_Position;
    float2 uv       : UV;
};


#endif
