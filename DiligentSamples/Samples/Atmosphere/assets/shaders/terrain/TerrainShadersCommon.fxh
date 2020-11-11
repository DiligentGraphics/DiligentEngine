#ifndef _TERRAIN_SHADERS_COMMON_FXH_
#define _TERRAIN_SHADERS_COMMON_FXH_

// Texturing modes
#define TM_HEIGHT_BASED 0             // Simple height-based texturing mode using 1D look-up table
#define TM_MATERIAL_MASK 1
#define TM_MATERIAL_MASK_NM 2

#ifndef TEXTURING_MODE
#   define TEXTURING_MODE TM_MATERIAL_MASK
#endif

#ifndef NUM_TILE_TEXTURES
#	define NUM_TILE_TEXTURES 5
#endif

#ifndef NUM_SHADOW_CASCADES
#   define NUM_SHADOW_CASCADES 4
#endif

#ifndef BEST_CASCADE_SEARCH
#   define BEST_CASCADE_SEARCH 1
#endif

#ifndef SMOOTH_SHADOWS
#   define SMOOTH_SHADOWS 1
#endif


struct HemisphereVSOutput
{
    float2 TileTexUV  : TileTextureUV;
    float3 f3Normal : Normal;
    float3 f3PosInLightViewSpace : POS_IN_LIGHT_VIEW_SPACE;
    float2 f2MaskUV0 : MASK_UV0;
    float3 f3Tangent : TANGENT;
    float3 f3Bitangent : BITANGENT;
    float3 f3SunLightExtinction : EXTINCTION;
    float3 f3AmbientSkyLight : AMBIENT_SKY_LIGHT;
};

#endif //_TERRAIN_SHADERS_COMMON_FXH_
