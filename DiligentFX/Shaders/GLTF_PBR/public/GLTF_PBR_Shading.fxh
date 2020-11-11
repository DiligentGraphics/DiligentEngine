#ifndef _GLTF_PBR_SHADING_FXH_
#define _GLTF_PBR_SHADING_FXH_

#include "GLTF_PBR_Structures.fxh"
#include "PBR_Common.fxh"
#include "ShaderUtilities.fxh"

#ifndef GLTF_PBR_MANUAL_SRGB
#   define  GLTF_PBR_MANUAL_SRGB    1
#endif

#ifndef SRGB_FAST_APPROXIMATION
#   define  SRGB_FAST_APPROXIMATION 1
#endif

#define GLTF_PBR_USE_ENV_MAP_LOD
#define GLTF_PBR_USE_HDR_CUBEMAPS

float GetPerceivedBrightness(float3 rgb)
{
    return sqrt(0.299 * rgb.r * rgb.r + 0.587 * rgb.g * rgb.g + 0.114 * rgb.b * rgb.b);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34
float GLTF_PBR_SolveMetallic(float3 diffuse,
                             float3 specular,
                             float  oneMinusSpecularStrength)
{
    const float c_MinReflectance = 0.04;
    float specularBrightness = GetPerceivedBrightness(specular);
    if (specularBrightness < c_MinReflectance)
    {
        return 0.0;
    }

    float diffuseBrightness = GetPerceivedBrightness(diffuse);

    float a = c_MinReflectance;
    float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;
    float c = c_MinReflectance - specularBrightness;
    float D = b * b - 4.0 * a * c;

    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}


float3 SRGBtoLINEAR(float3 srgbIn)
{
#ifdef GLTF_PBR_MANUAL_SRGB
#   ifdef SRGB_FAST_APPROXIMATION
    	float3 linOut = pow(saturate(srgbIn.xyz), float3(2.2, 2.2, 2.2));
#   else
	    float3 bLess  = step(float3(0.04045, 0.04045, 0.04045), srgbIn.xyz);
	    float3 linOut = mix( srgbIn.xyz/12.92, pow(saturate((srgbIn.xyz + float3(0.055, 0.055, 0.055)) / 1.055), float3(2.4, 2.4, 2.4)), bLess );
#   endif
	    return linOut;
#else
	return srgbIn;
#endif
}

float4 SRGBtoLINEAR(float4 srgbIn)
{
    return float4(SRGBtoLINEAR(srgbIn.xyz), srgbIn.w);
}


float3 GLTF_PBR_ApplyDirectionalLight(float3 lightDir, float3 lightColor, SurfaceReflectanceInfo srfInfo, float3 normal, float3 view)
{
    float3 pointToLight = -lightDir;
    float3 diffuseContrib, specContrib;
    float  NdotL;
    BRDF(pointToLight, normal, view, srfInfo, diffuseContrib, specContrib, NdotL);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    float3 shade = (diffuseContrib + specContrib) * NdotL;
    return lightColor * shade;
}


// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
float3 GLTF_PBR_PerturbNormal(in float3 dPos_dx,
                              in float3 dPos_dy,
                              in float2 dUV_dx,
                              in float2 dUV_dy,
                              in float3 Normal,
                              in float3 TSNormal,
                              bool      HasUV,
                              bool      IsFrontFace)
{
    // Retrieve the tangent space matrix
    float NormalLen = length(Normal);
    float3 ng;
    if (NormalLen > 1e-5)
    {
        ng = Normal/NormalLen;
    }
    else
    {
        ng = normalize(cross(dPos_dx, dPos_dy));
#if (defined(GLSL) || defined(GL_ES)) && !defined(VULKAN)
        // In OpenGL screen is upside-down, so we have to invert the vector
        ng *= -1.0;
#endif
    }

    if (HasUV)
    {
        return TransformTangentSpaceNormalGrad(dPos_dx, dPos_dy, dUV_dx, dUV_dy, ng, TSNormal * (IsFrontFace ? +1.0 : -1.0));
    }
    else
    {
        return ng * (IsFrontFace ? +1.0 : -1.0);
    }
}


struct GLTF_PBR_IBL_Contribution
{
    float3 f3Diffuse;
    float3 f3Specular;
};

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
GLTF_PBR_IBL_Contribution GLTF_PBR_GetIBLContribution(
                        in SurfaceReflectanceInfo SrfInfo,
                        in float3                 n,
                        in float3                 v,
                        in float                  PrefilteredCubeMipLevels,
                        in Texture2D              BRDF_LUT,
                        in SamplerState           BRDF_LUT_sampler,
                        in TextureCube            IrradianceMap,
                        in SamplerState           IrradianceMap_sampler,
                        in TextureCube            PrefilteredEnvMap,
                        in SamplerState           PrefilteredEnvMap_sampler)
{
    float NdotV = clamp(dot(n, v), 0.0, 1.0);

    float lod = clamp(SrfInfo.PerceptualRoughness * PrefilteredCubeMipLevels, 0.0, PrefilteredCubeMipLevels);
    float3 reflection = normalize(reflect(-v, n));

    float2 brdfSamplePoint = clamp(float2(NdotV, SrfInfo.PerceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));
    // retrieve a scale and bias to F0. See [1], Figure 3
    float2 brdf = BRDF_LUT.Sample(BRDF_LUT_sampler, brdfSamplePoint).rg;

    float4 diffuseSample = IrradianceMap.Sample(IrradianceMap_sampler, n);

#ifdef GLTF_PBR_USE_ENV_MAP_LOD
    float4 specularSample = PrefilteredEnvMap.SampleLevel(PrefilteredEnvMap_sampler, reflection, lod);
#else
    float4 specularSample = PrefilteredEnvMap.Sample(PrefilteredEnvMap_sampler, reflection);
#endif

#ifdef GLTF_PBR_USE_HDR_CUBEMAPS
    // Already linear.
    float3 diffuseLight  = diffuseSample.rgb;
    float3 specularLight = specularSample.rgb;
#else
    float3 diffuseLight  = SRGBtoLINEAR(diffuseSample).rgb;
    float3 specularLight = SRGBtoLINEAR(specularSample).rgb;
#endif

    GLTF_PBR_IBL_Contribution IBLContrib;
    IBLContrib.f3Diffuse  = diffuseLight * SrfInfo.DiffuseColor;
    IBLContrib.f3Specular = specularLight * (SrfInfo.Reflectance0 * brdf.x + SrfInfo.Reflectance90 * brdf.y);
    return IBLContrib;
}

/// Calculates surface reflectance info

/// \param [in]  Workflow     - PBR workflow (PBR_WORKFLOW_SPECULAR_GLOSINESS or PBR_WORKFLOW_METALLIC_ROUGHNESS).
/// \param [in]  BaseColor    - Material base color.
/// \param [in]  PhysicalDesc - Physical material description. For Metallic-roughness workflow,
///                             'g' channel stores roughness, 'b' channel stores metallic.
/// \param [out] Metallic     - Metallic value used for shading.
SurfaceReflectanceInfo GLTF_PBR_GetSurfaceReflectance(int        Workflow,
                                                      float4     BaseColor,
                                                      float4     PhysicalDesc,
                                                      out float  Metallic)
{
    SurfaceReflectanceInfo SrfInfo;

    float3 specularColor;

    float3 f0 = float3(0.04, 0.04, 0.04);

    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    if (Workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
    {
        SrfInfo.PerceptualRoughness = 1.0 - PhysicalDesc.a; // glossiness to roughness
        f0 = PhysicalDesc.rgb;

        // f0 = specular
        specularColor = f0;
        float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
        SrfInfo.DiffuseColor = BaseColor.rgb * oneMinusSpecularStrength;

        // do conversion between metallic M-R and S-G metallic
        Metallic = GLTF_PBR_SolveMetallic(BaseColor.rgb, specularColor, oneMinusSpecularStrength);
    }
    else if (Workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
    {
        // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
        // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
        SrfInfo.PerceptualRoughness = PhysicalDesc.g;
        Metallic                    = PhysicalDesc.b;

        SrfInfo.DiffuseColor  = BaseColor.rgb * (float3(1.0, 1.0, 1.0) - f0) * (1.0 - Metallic);
        specularColor         = lerp(f0, BaseColor.rgb, Metallic);
    }

//#ifdef ALPHAMODE_OPAQUE
//    baseColor.a = 1.0;
//#endif
//
//#ifdef MATERIAL_UNLIT
//    gl_FragColor = float4(gammaCorrection(baseColor.rgb), baseColor.a);
//    return;
//#endif

    SrfInfo.PerceptualRoughness = clamp(SrfInfo.PerceptualRoughness, 0.0, 1.0);

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    SrfInfo.Reflectance0  = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    SrfInfo.Reflectance90 = clamp(reflectance * 50.0, 0.0, 1.0) * float3(1.0, 1.0, 1.0);

    return SrfInfo;
}

#endif // _GLTF_PBR_SHADING_FXH_
