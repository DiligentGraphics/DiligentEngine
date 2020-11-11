
#include "HostSharedTerrainStructs.fxh"
#include "ToneMappingStructures.fxh"
#include "EpipolarLightScatteringStructures.fxh"
#include "TerrainShadersCommon.fxh"
#include "Shadows.fxh"

cbuffer cbTerrainAttribs
{
    TerrainAttribs g_TerrainAttribs;
};

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

cbuffer cbLightAttribs
{
    LightAttribs g_LightAttribs;
};


#define EARTH_REFLECTANCE 0.4


// Normal map stores only x,y components. z component is calculated as sqrt(1 - x^2 - y^2)
Texture2D    g_tex2DNormalMap;
SamplerState g_tex2DNormalMap_sampler; // Linear Mirror

Texture2D<float4> g_tex2DMtrlMap;
SamplerState      g_tex2DMtrlMap_sampler; // Linear Mirror

Texture2DArray<float>   g_tex2DShadowMap;
SamplerComparisonState g_tex2DShadowMap_sampler; // Comparison

Texture2D<float3> g_tex2DTileDiffuse[5];   // Material diffuse
SamplerState      g_tex2DTileDiffuse_sampler;   

Texture2D<float3> g_tex2DTileNM[5];   // Material NM
SamplerState      g_tex2DTileNM_sampler;   

void CombineMaterials(in float4 MtrlWeights,
                      in float2 f2TileUV,
                      out float3 SurfaceColor,
                      out float3 SurfaceNormalTS)
{
    SurfaceColor = float3(0.0, 0.0, 0.0);
    SurfaceNormalTS = float3(0.0, 0.0, 1.0);

    // Normalize weights and compute base material weight
    MtrlWeights /= max( dot(MtrlWeights, float4(1.0, 1.0, 1.0, 1.0)) , 1.0 );
    float BaseMaterialWeight = saturate(1.0 - dot(MtrlWeights, float4(1.0, 1.0, 1.0, 1.0)));
    
    // The mask is already sharp

    ////Sharpen the mask
    //float2 TmpMin2 = min(MtrlWeights.rg, MtrlWeights.ba);
    //float Min = min(TmpMin2.r, TmpMin2.g);
    //Min = min(Min, BaseMaterialWeight);
    //float p = 4;
    //BaseMaterialWeight = pow(BaseMaterialWeight-Min, p);
    //MtrlWeights = pow(MtrlWeights-Min, p);
    //float NormalizationFactor = dot(MtrlWeights, float4(1,1,1,1)) + BaseMaterialWeight;
    //MtrlWeights /= NormalizationFactor;
    //BaseMaterialWeight /= NormalizationFactor;

	// Get diffuse color of the base material
    float3 BaseMaterialDiffuse = g_tex2DTileDiffuse[0].Sample( g_tex2DTileDiffuse_sampler, f2TileUV.xy / g_TerrainAttribs.m_fBaseMtrlTilingScale );
    float3 MaterialColors[NUM_TILE_TEXTURES];

    // Get tangent space normal of the base material
#if TEXTURING_MODE == TM_MATERIAL_MASK_NM
    float3 BaseMaterialNormal = g_tex2DTileNM[0].Sample(g_tex2DTileNM_sampler, f2TileUV.xy / g_TerrainAttribs.m_fBaseMtrlTilingScale);
    float3 MaterialNormals[NUM_TILE_TEXTURES];
#endif

    float4 f4TilingScale = g_TerrainAttribs.m_f4TilingScale;
    float fTilingScale[5]; 
    fTilingScale[0] = 0.0;
    fTilingScale[1] = f4TilingScale.x;
    fTilingScale[2] = f4TilingScale.y;
    fTilingScale[3] = f4TilingScale.z;
    fTilingScale[4] = f4TilingScale.w;
    // Load material colors and normals
    const float fThresholdWeight = 3.f/256.f;
    MaterialColors[1] = MtrlWeights.x > fThresholdWeight ? g_tex2DTileDiffuse[1].Sample(g_tex2DTileDiffuse_sampler, f2TileUV.xy  / fTilingScale[1]) : float3(0.0, 0.0, 0.0);
    MaterialColors[2] = MtrlWeights.y > fThresholdWeight ? g_tex2DTileDiffuse[2].Sample(g_tex2DTileDiffuse_sampler, f2TileUV.xy  / fTilingScale[2]) : float3(0.0, 0.0, 0.0);
    MaterialColors[3] = MtrlWeights.z > fThresholdWeight ? g_tex2DTileDiffuse[3].Sample(g_tex2DTileDiffuse_sampler, f2TileUV.xy  / fTilingScale[3]) : float3(0.0, 0.0, 0.0);
    MaterialColors[4] = MtrlWeights.w > fThresholdWeight ? g_tex2DTileDiffuse[4].Sample(g_tex2DTileDiffuse_sampler, f2TileUV.xy  / fTilingScale[4]) : float3(0.0, 0.0, 0.0);

#if TEXTURING_MODE == TM_MATERIAL_MASK_NM
    MaterialNormals[1] = MtrlWeights.x > fThresholdWeight ? g_tex2DTileNM[1].Sample(g_tex2DTileNM_sampler, f2TileUV.xy  / fTilingScale[1]) : float3(0.0, 0.0, 1.0);
    MaterialNormals[2] = MtrlWeights.y > fThresholdWeight ? g_tex2DTileNM[2].Sample(g_tex2DTileNM_sampler, f2TileUV.xy  / fTilingScale[2]) : float3(0.0, 0.0, 1.0);
    MaterialNormals[3] = MtrlWeights.z > fThresholdWeight ? g_tex2DTileNM[3].Sample(g_tex2DTileNM_sampler, f2TileUV.xy  / fTilingScale[3]) : float3(0.0, 0.0, 1.0);
    MaterialNormals[4] = MtrlWeights.w > fThresholdWeight ? g_tex2DTileNM[4].Sample(g_tex2DTileNM_sampler, f2TileUV.xy  / fTilingScale[4]) : float3(0.0, 0.0, 1.0);
#endif
    // Blend materials and normals using the weights
    SurfaceColor = BaseMaterialDiffuse.rgb * BaseMaterialWeight + 
        MaterialColors[1] * MtrlWeights.x + 
        MaterialColors[2] * MtrlWeights.y + 
        MaterialColors[3] * MtrlWeights.z + 
        MaterialColors[4] * MtrlWeights.w;

#if TEXTURING_MODE == TM_MATERIAL_MASK_NM
    SurfaceNormalTS = BaseMaterialNormal * BaseMaterialWeight + 
        MaterialNormals[1] * MtrlWeights.x + 
        MaterialNormals[2] * MtrlWeights.y + 
        MaterialNormals[3] * MtrlWeights.z + 
        MaterialNormals[4] * MtrlWeights.w;
    SurfaceNormalTS = SurfaceNormalTS * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
#endif
}

void HemispherePS(in float4 f4Pos : SV_Position,
                  HemisphereVSOutput VSOut,
                  // IMPORTANT: non-system generated pixel shader input
                  // arguments must have the exact same name as vertex shader 
                  // outputs and must go in the same order.
                  
                  out float4 f4OutColor : SV_Target)
{
    float3 EarthNormal = normalize(VSOut.f3Normal);
    float3 EarthTangent = normalize(VSOut.f3Tangent);
    float3 EarthBitangent = normalize(VSOut.f3Bitangent);
    float3 f3TerrainNormal;
    f3TerrainNormal.xz = g_tex2DNormalMap.Sample(g_tex2DNormalMap_sampler, VSOut.f2MaskUV0.xy).xy * float2(2.0,2.0) - float2(1.0,1.0);
    // Since UVs are mirrored, we have to adjust normal coords accordingly:
    float2 f2XZSign = sign( float2(0.5,0.5) - frac(VSOut.f2MaskUV0.xy/2.0) );
    f3TerrainNormal.xz *= f2XZSign;

    f3TerrainNormal.y = sqrt( saturate(1.0 - dot(f3TerrainNormal.xz,f3TerrainNormal.xz)) );
    //float3 Tangent   = normalize(float3(1,0,VSOut.HeightMapGradients.x));
    //float3 Bitangent = normalize(float3(0,1,VSOut.HeightMapGradients.y));
    f3TerrainNormal = normalize( mul(f3TerrainNormal, float3x3(EarthTangent, EarthNormal, EarthBitangent)) );

    float4 MtrlWeights = g_tex2DMtrlMap.Sample( g_tex2DMtrlMap_sampler, VSOut.f2MaskUV0.xy );

    float3 SurfaceColor, SurfaceNormalTS = float3(0.0, 0.0, 1.0);
    CombineMaterials(MtrlWeights, VSOut.TileTexUV, SurfaceColor, SurfaceNormalTS);

    float3 f3TerrainTangent = normalize( cross(f3TerrainNormal, float3(0.0, 0.0, 1.0)) );
    float3 f3TerrainBitangent = normalize( cross(f3TerrainTangent, f3TerrainNormal) );
    float3 f3Normal = normalize(  SurfaceNormalTS.x * f3TerrainTangent + SurfaceNormalTS.z * f3TerrainNormal + SurfaceNormalTS.y * f3TerrainBitangent );


    // Attenuate extraterrestrial sun color with the extinction factor
    float3 f3SunLight = g_LightAttribs.f4Intensity.rgb * VSOut.f3SunLightExtinction;
    // Ambient sky light is not pre-multiplied with the sun intensity
    float3 f3AmbientSkyLight = g_LightAttribs.f4Intensity.rgb * VSOut.f3AmbientSkyLight;
    // Account for occlusion by the ground plane
    f3AmbientSkyLight *= saturate((1.0 + dot(EarthNormal, f3Normal))/2.f);

    // We need to divide diffuse color by PI to get the reflectance value
    float3 SurfaceReflectance = SurfaceColor * EARTH_REFLECTANCE / PI;

    float fCameraSpaceZ = f4Pos.w;
#   ifdef VULKAN
        // In Vulkan SV_Position.w == 1 / VSOutput.SV_Position.w
        fCameraSpaceZ = 1.0 / fCameraSpaceZ;
#   endif
    FilteredShadow Shadow = FilterShadowMap(g_LightAttribs.ShadowAttribs, g_tex2DShadowMap, g_tex2DShadowMap_sampler,
                                            VSOut.f3PosInLightViewSpace.xyz, ddx(VSOut.f3PosInLightViewSpace.xyz), ddy(VSOut.f3PosInLightViewSpace.xyz),
                                            fCameraSpaceZ);
    float DiffuseIllumination = max(0.0, dot(f3Normal, -g_LightAttribs.f4Direction.xyz));
    
    float3 f3CascadeColor = float3(0.0, 0.0, 0.0);
    if( g_LightAttribs.ShadowAttribs.bVisualizeCascades )
    {
        f3CascadeColor = GetCascadeColor(Shadow) * 0.25;
    }
    
    f4OutColor.rgb = f3CascadeColor +  SurfaceReflectance * (Shadow.fLightAmount*DiffuseIllumination*f3SunLight + f3AmbientSkyLight);
    f4OutColor.a = 1.0;
}
