/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <algorithm>
#include <unordered_set>
#include <array>
#include <cstring>

#include "EpipolarLightScattering.hpp"
#include "ShaderMacroHelper.hpp"
#include "GraphicsUtilities.h"
#include "GraphicsAccessories.hpp"
#include "../../../Utilities/include/DiligentFXShaderSourceStreamFactory.hpp"
#include "MapHelper.hpp"
#include "CommonlyUsedStates.h"
#include "Align.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Diligent
{

// clang-format off
static const DepthStencilStateDesc DSS_CmpEqNoWrites
{    
	True,                 // DepthEnable
    False,                // DepthWriteEnable
	COMPARISON_FUNC_EQUAL // DepthFunc
};

// Disable depth testing and always increment stencil value.
// This depth stencil state is used to mark samples which will undergo further processing.
// Pixel shader discards pixels that should not be further processed, thus keeping the
// stencil value untouched.
// For instance, pixel shader performing epipolar coordinates generation discards all 
// sampes, whoose coordinates are outside the screen [-1,1]x[-1,1] area.
static const DepthStencilStateDesc DSS_IncStencilAlways
{
    False,                  // DepthEnable
    False,                  // DepthWriteEnable
    COMPARISON_FUNC_LESS,   // DepthFunc
    True,                   // StencilEnable
    0xFF,                   // StencilReadMask
    0xFF,                   // StencilWriteMask
    StencilOpDesc
    {
        STENCIL_OP_KEEP,        // StencilFailOp
        STENCIL_OP_KEEP,        // StencilDepthFailOp
        STENCIL_OP_INCR_SAT,    // StencilPassOp
		COMPARISON_FUNC_ALWAYS  // StencilFunc
	},
    StencilOpDesc
    {
        STENCIL_OP_KEEP,        // StencilFailOp
        STENCIL_OP_KEEP,        // StencilDepthFailOp
        STENCIL_OP_INCR_SAT,    // StencilPassOp
		COMPARISON_FUNC_ALWAYS  // StencilFunc
	}
};


// Disable depth testing, stencil testing function equal, increment stencil.
// This state is used to process only those pixels that were marked at the previous pass.
// All pixels whith different stencil value are discarded from further processing as well
// as some pixels can also be discarded during the draw call.
// For instance, pixel shader marking ray marching samples processes only those pixels which are inside
// the screen. It also discards all but those samples that are interpolated from themselves.
static const DepthStencilStateDesc DSS_StencilEqIncStencil
{
    False,                  // DepthEnable
    False,                  // DepthWriteEnable
    COMPARISON_FUNC_LESS,   // DepthFunc
    True,                   // StencilEnable
    0xFF,                   // StencilReadMask
    0xFF,                   // StencilWriteMask
    StencilOpDesc
    {
        STENCIL_OP_KEEP,        // StencilFailOp
        STENCIL_OP_KEEP,        // StencilDepthFailOp
        STENCIL_OP_INCR_SAT,    // StencilPassOp
		COMPARISON_FUNC_EQUAL   // StencilFunc
	},
    StencilOpDesc
    {
        STENCIL_OP_KEEP,        // StencilFailOp
        STENCIL_OP_KEEP,        // StencilDepthFailOp
        STENCIL_OP_INCR_SAT,    // StencilPassOp
		COMPARISON_FUNC_EQUAL   // StencilFunc
	}
};


// Disable depth testing, stencil testing function equal, keep stencil.
static const DepthStencilStateDesc DSS_StencilEqKeepStencil = 
{
    False,                  // DepthEnable
    False,                  // DepthWriteEnable
    COMPARISON_FUNC_LESS,   // DepthFunc
    True,                   // StencilEnable
    0xFF,                   // StencilReadMask
    0xFF,                   // StencilWriteMask
    StencilOpDesc
    {
        STENCIL_OP_KEEP,        // StencilFailOp
        STENCIL_OP_KEEP,        // StencilDepthFailOp
        STENCIL_OP_KEEP,        // StencilPassOp
		COMPARISON_FUNC_EQUAL   // StencilFunc
	},
    StencilOpDesc
    {
        STENCIL_OP_KEEP,        // StencilFailOp
        STENCIL_OP_KEEP,        // StencilDepthFailOp
        STENCIL_OP_KEEP,        // StencilPassOp
		COMPARISON_FUNC_EQUAL   // StencilFunc
	}
};

static const BlendStateDesc BS_AdditiveBlend = 
{
    False, // AlphaToCoverageEnable
    False, // IndependentBlendEnable
    RenderTargetBlendDesc
    {
        True,                // BlendEnable
        False,               // LogicOperationEnable
        BLEND_FACTOR_ONE,    // SrcBlend
        BLEND_FACTOR_ONE,    // DestBlend
        BLEND_OPERATION_ADD, // BlendOp
        BLEND_FACTOR_ONE,    // SrcBlendAlpha
        BLEND_FACTOR_ONE,    // DestBlendAlpha
        BLEND_OPERATION_ADD  // BlendOpAlpha
    }
};
// clang-format on

static void RenderFullScreenTriangle(IDeviceContext*         pDeviceContext,
                                     IPipelineState*         PSO,
                                     IShaderResourceBinding* SRB,
                                     Uint8                   StencilRef = 0,
                                     Uint32                  NumQuads   = 1)
{
    pDeviceContext->SetPipelineState(PSO);
    pDeviceContext->CommitShaderResources(SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pDeviceContext->SetStencilRef(StencilRef);

    DrawAttribs FullScreenTriangleDrawAttrs;
    FullScreenTriangleDrawAttrs.NumVertices  = 3;
    FullScreenTriangleDrawAttrs.NumInstances = NumQuads;
    pDeviceContext->Draw(FullScreenTriangleDrawAttrs);
}


void EpipolarLightScattering::RenderTechnique::InitializeFullScreenTriangleTechnique(
    IRenderDevice*                    pDevice,
    const char*                       PSOName,
    IShader*                          VertexShader,
    IShader*                          PixelShader,
    const PipelineResourceLayoutDesc& ResourceLayout,
    Uint8                             NumRTVs,
    TEXTURE_FORMAT                    RTVFmts[],
    TEXTURE_FORMAT                    DSVFmt  = TEX_FORMAT_UNKNOWN,
    const DepthStencilStateDesc&      DSSDesc = DSS_Default,
    const BlendStateDesc&             BSDesc  = BS_Default)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

    PSODesc.Name           = PSOName;
    PSODesc.ResourceLayout = ResourceLayout;

    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_NONE;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = true;
    GraphicsPipeline.DepthStencilDesc                     = DSSDesc;
    GraphicsPipeline.BlendDesc                            = BSDesc;
    PSOCreateInfo.pVS                                     = VertexShader;
    PSOCreateInfo.pPS                                     = PixelShader;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    GraphicsPipeline.NumRenderTargets                     = NumRTVs;
    GraphicsPipeline.DSVFormat                            = DSVFmt;
    for (Uint32 rt = 0; rt < NumRTVs; ++rt)
        GraphicsPipeline.RTVFormats[rt] = RTVFmts[rt];

    PSO.Release();
    SRB.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &PSO);
}

void EpipolarLightScattering::RenderTechnique::InitializeFullScreenTriangleTechnique(
    IRenderDevice*                    pDevice,
    const char*                       PSOName,
    IShader*                          VertexShader,
    IShader*                          PixelShader,
    const PipelineResourceLayoutDesc& ResourceLayout,
    TEXTURE_FORMAT                    RTVFmt,
    TEXTURE_FORMAT                    DSVFmt  = TEX_FORMAT_UNKNOWN,
    const DepthStencilStateDesc&      DSSDesc = DSS_DisableDepth,
    const BlendStateDesc&             BSDesc  = BS_Default)
{
    InitializeFullScreenTriangleTechnique(pDevice, PSOName, VertexShader, PixelShader, ResourceLayout, 1, &RTVFmt, DSVFmt, DSSDesc, BSDesc);
}

void EpipolarLightScattering::RenderTechnique::InitializeComputeTechnique(IRenderDevice*                    pDevice,
                                                                          const char*                       PSOName,
                                                                          IShader*                          ComputeShader,
                                                                          const PipelineResourceLayoutDesc& ResourceLayout)
{
    ComputePipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&             PSODesc = PSOCreateInfo.PSODesc;

    PSODesc.Name           = PSOName;
    PSODesc.ResourceLayout = ResourceLayout;
    PSODesc.PipelineType   = PIPELINE_TYPE_COMPUTE;
    PSOCreateInfo.pCS      = ComputeShader;
    PSO.Release();
    SRB.Release();
    pDevice->CreateComputePipelineState(PSOCreateInfo, &PSO);
}

void EpipolarLightScattering::RenderTechnique::PrepareSRB(IRenderDevice* pDevice, IResourceMapping* pResMapping, Uint32 Flags = BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED)
{
    if (!SRB)
    {
        PSO->CreateShaderResourceBinding(&SRB, true);
        SRB->BindResources(SHADER_TYPE_PIXEL | SHADER_TYPE_COMPUTE, pResMapping, Flags);
    }
}

void EpipolarLightScattering::RenderTechnique::Render(IDeviceContext* pDeviceContext, Uint8 StencilRef, Uint32 NumQuads)
{
    RenderFullScreenTriangle(pDeviceContext, PSO, SRB, StencilRef, NumQuads);
}

void EpipolarLightScattering::RenderTechnique::DispatchCompute(IDeviceContext* pDeviceContext, const DispatchComputeAttribs& DispatchAttrs)
{
    pDeviceContext->SetPipelineState(PSO);
    pDeviceContext->CommitShaderResources(SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pDeviceContext->DispatchCompute(DispatchAttrs);
}

void EpipolarLightScattering::RenderTechnique::CheckStaleFlags(Uint32 StalePSODependencies, Uint32 StaleSRBDependencies)
{
    if ((PSODependencyFlags & StalePSODependencies) != 0)
    {
        PSO.Release();
    }

    if ((SRBDependencyFlags & StaleSRBDependencies) != 0)
    {
        SRB.Release();
    }
}

static RefCntAutoPtr<IShader> CreateShader(IRenderDevice*     pDevice,
                                           const Char*        FileName,
                                           const Char*        EntryPoint,
                                           SHADER_TYPE        Type,
                                           const ShaderMacro* Macros = nullptr)
{
    ShaderCreateInfo ShaderCI;
    ShaderCI.EntryPoint                 = EntryPoint;
    ShaderCI.FilePath                   = FileName;
    ShaderCI.Macros                     = Macros;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.ShaderType            = Type;
    ShaderCI.Desc.Name                  = EntryPoint;
    ShaderCI.pShaderSourceStreamFactory = &DiligentFXShaderSourceStreamFactory::GetInstance();
    ShaderCI.UseCombinedTextureSamplers = true;
    RefCntAutoPtr<IShader> pShader;
    pDevice->CreateShader(ShaderCI, &pShader);
    return pShader;
}

EpipolarLightScattering::EpipolarLightScattering(IRenderDevice*              pDevice,
                                                 IDeviceContext*             pContext,
                                                 TEXTURE_FORMAT              BackBufferFmt,
                                                 TEXTURE_FORMAT              DepthBufferFmt,
                                                 TEXTURE_FORMAT              OffscreenBackBufferFmt,
                                                 const AirScatteringAttribs& ScatteringAttibs) :
    m_BackBufferFmt(BackBufferFmt),
    m_DepthBufferFmt(DepthBufferFmt),
    m_OffscreenBackBufferFmt(OffscreenBackBufferFmt),
    m_bUseCombinedMinMaxTexture(false),
    m_uiSampleRefinementCSThreadGroupSize(0),
    // Using small group size is inefficient because a lot of SIMD lanes become idle
    m_uiSampleRefinementCSMinimumThreadGroupSize(128), // Must be greater than 32
    m_MediaParams(ScatteringAttibs),
    m_uiUpToDateResourceFlags(0)
{
    VERIFY_EXPR(m_MediaParams.fAtmTopAltitude > m_MediaParams.fAtmBottomAltitude);
    m_MediaParams.fAtmTopRadius        = m_MediaParams.fEarthRadius + m_MediaParams.fAtmTopAltitude;
    m_MediaParams.fAtmBottomRadius     = m_MediaParams.fEarthRadius + m_MediaParams.fAtmBottomAltitude;
    m_MediaParams.fAtmAltitudeRangeInv = 1.f / (m_MediaParams.fAtmTopAltitude - m_MediaParams.fAtmBottomAltitude);

    pDevice->CreateResourceMapping(ResourceMappingDesc(), &m_pResMapping);
    const auto& deviceCaps = pDevice->GetDeviceCaps();
    if (deviceCaps.DevType == RENDER_DEVICE_TYPE_GLES || deviceCaps.AdapterInfo.Type == ADAPTER_TYPE_SOFTWARE)
    {
        m_uiNumRandomSamplesOnSphere /= 2;
        m_iPrecomputedSctrUDim /= 2;
        m_iPrecomputedSctrVDim /= 2;
        m_iPrecomputedSctrWDim /= 2;
        m_iPrecomputedSctrQDim /= 2;
    }

    // clang-format off
    CreateUniformBuffer(pDevice, sizeof(EpipolarLightScatteringAttribs), "Epipolar Light Scattering Attribs CB", &m_pcbPostProcessingAttribs);
    CreateUniformBuffer(pDevice, sizeof(MiscDynamicParams),              "Misc Dynamic Params CB",               &m_pcbMiscParams);
    // clang-format on

    {
        BufferDesc CBDesc;
        CBDesc.Usage         = USAGE_DEFAULT;
        CBDesc.BindFlags     = BIND_UNIFORM_BUFFER;
        CBDesc.uiSizeInBytes = sizeof(AirScatteringAttribs);

        BufferData InitData{&m_MediaParams, CBDesc.uiSizeInBytes};
        pDevice->CreateBuffer(CBDesc, &InitData, &m_pcbMediaAttribs);
    }

    // clang-format off
    // Add uniform buffers to the shader resource mapping. These buffers will never change.
    // Note that only buffer objects will stay unchanged, while the buffer contents can be updated.
    m_pResMapping->AddResource("cbPostProcessingAttribs",              m_pcbPostProcessingAttribs, true);
    m_pResMapping->AddResource("cbParticipatingMediaScatteringParams", m_pcbMediaAttribs,          true);
    m_pResMapping->AddResource("cbMiscDynamicParams",                  m_pcbMiscParams,            true);
    // clang-format on

    pDevice->CreateSampler(Sam_LinearClamp, &m_pLinearClampSampler);
    pDevice->CreateSampler(Sam_PointClamp, &m_pPointClampSampler);
    m_pFullScreenTriangleVS = CreateShader(pDevice, "FullScreenTriangleVS.fx", "FullScreenTriangleVS", SHADER_TYPE_VERTEX);

    ComputeScatteringCoefficients(pContext);

    PrecomputeOpticalDepthTexture(pDevice, pContext);

    CreateAmbientSkyLightTexture(pDevice);
}

EpipolarLightScattering::~EpipolarLightScattering()
{
}


void EpipolarLightScattering::OnWindowResize(IRenderDevice* pDevice, Uint32 uiBackBufferWidth, Uint32 uiBackBufferHeight)
{
    m_uiBackBufferWidth  = uiBackBufferWidth;
    m_uiBackBufferHeight = uiBackBufferHeight;
    m_ptex2DCamSpaceZRTV.Release();
}

void EpipolarLightScattering::DefineMacros(ShaderMacroHelper& Macros)
{
    // Define common shader macros

    // clang-format off
    Macros.AddShaderMacro("OPTIMIZE_SAMPLE_LOCATIONS",    m_PostProcessingAttribs.bOptimizeSampleLocations);
    Macros.AddShaderMacro("USE_COMBINED_MIN_MAX_TEXTURE", m_bUseCombinedMinMaxTexture);
    Macros.AddShaderMacro("EXTINCTION_EVAL_MODE",         m_PostProcessingAttribs.iExtinctionEvalMode);
    Macros.AddShaderMacro("ENABLE_LIGHT_SHAFTS",          m_PostProcessingAttribs.bEnableLightShafts);
    Macros.AddShaderMacro("MULTIPLE_SCATTERING_MODE",     m_PostProcessingAttribs.iMultipleScatteringMode);
    Macros.AddShaderMacro("SINGLE_SCATTERING_MODE",       m_PostProcessingAttribs.iSingleScatteringMode);
    // clang-format on

    {
        std::stringstream ss;
        ss << "float4(" << m_iPrecomputedSctrUDim << ".0,"
           << m_iPrecomputedSctrVDim << ".0,"
           << m_iPrecomputedSctrWDim << ".0,"
           << m_iPrecomputedSctrQDim << ".0)";
        Macros.AddShaderMacro("PRECOMPUTED_SCTR_LUT_DIM", ss.str());
    }
}

void EpipolarLightScattering::PrecomputeOpticalDepthTexture(IRenderDevice*  pDevice,
                                                            IDeviceContext* pDeviceContext)
{
    if (!m_ptex2DOccludedNetDensityToAtmTopSRV)
    {
        // Create texture if it has not been created yet.
        // Do not recreate texture if it already exists as this may
        // break static resource bindings.
        TextureDesc TexDesc;
        TexDesc.Name      = "Occluded Net Density to Atm Top";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Width     = sm_iNumPrecomputedHeights;
        TexDesc.Height    = sm_iNumPrecomputedAngles;
        TexDesc.Format    = PrecomputedNetDensityTexFmt;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
        RefCntAutoPtr<ITexture> tex2DOccludedNetDensityToAtmTop;
        pDevice->CreateTexture(TexDesc, nullptr, &tex2DOccludedNetDensityToAtmTop);
        m_ptex2DOccludedNetDensityToAtmTopSRV = tex2DOccludedNetDensityToAtmTop->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex2DOccludedNetDensityToAtmTopSRV->SetSampler(m_pLinearClampSampler);
        m_ptex2DOccludedNetDensityToAtmTopRTV = tex2DOccludedNetDensityToAtmTop->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        m_pResMapping->AddResource("g_tex2DOccludedNetDensityToAtmTop", m_ptex2DOccludedNetDensityToAtmTopSRV, false);
    }

    ITextureView* pRTVs[] = {m_ptex2DOccludedNetDensityToAtmTopRTV};
    pDeviceContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    auto& PrecomputeNetDensityToAtmTopTech = m_RenderTech[RENDER_TECH_PRECOMPUTE_NET_DENSITY_TO_ATM_TOP];
    if (!PrecomputeNetDensityToAtmTopTech.PSO)
    {
        RefCntAutoPtr<IShader> pPrecomputeNetDensityToAtmTopPS;
        pPrecomputeNetDensityToAtmTopPS = CreateShader(pDevice, "PrecomputeNetDensityToAtmTop.fx", "PrecomputeNetDensityToAtmTopPS", SHADER_TYPE_PIXEL);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        PrecomputeNetDensityToAtmTopTech.InitializeFullScreenTriangleTechnique(pDevice, "PrecomputeNetDensityToAtmTopPSO", m_pFullScreenTriangleVS,
                                                                               pPrecomputeNetDensityToAtmTopPS, ResourceLayout, PrecomputedNetDensityTexFmt);
        PrecomputeNetDensityToAtmTopTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
    }

    PrecomputeNetDensityToAtmTopTech.PrepareSRB(pDevice, m_pResMapping);
    PrecomputeNetDensityToAtmTopTech.Render(pDeviceContext);

    m_uiUpToDateResourceFlags |= UpToDateResourceFlags::PrecomputedOpticalDepthTex;
}



void EpipolarLightScattering::CreateRandomSphereSamplingTexture(IRenderDevice* pDevice)
{
    TextureDesc RandomSphereSamplingTexDesc;
    RandomSphereSamplingTexDesc.Type      = RESOURCE_DIM_TEX_2D;
    RandomSphereSamplingTexDesc.Width     = m_uiNumRandomSamplesOnSphere;
    RandomSphereSamplingTexDesc.Height    = 1;
    RandomSphereSamplingTexDesc.MipLevels = 1;
    RandomSphereSamplingTexDesc.Format    = TEX_FORMAT_RGBA32_FLOAT;
    RandomSphereSamplingTexDesc.Usage     = USAGE_IMMUTABLE;
    RandomSphereSamplingTexDesc.BindFlags = BIND_SHADER_RESOURCE;

    std::vector<float4> SphereSampling(m_uiNumRandomSamplesOnSphere);
    for (Uint32 iSample = 0; iSample < m_uiNumRandomSamplesOnSphere; ++iSample)
    {
        float4& f4Sample = SphereSampling[iSample];
        f4Sample.z       = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f;
        float t          = ((float)rand() / (float)RAND_MAX) * 2.f * PI_F;
        float r          = sqrt(std::max(1.f - f4Sample.z * f4Sample.z, 0.f));
        f4Sample.x       = r * cos(t);
        f4Sample.y       = r * sin(t);
        f4Sample.w       = 0;
    }
    TextureSubResData Mip0Data;
    Mip0Data.pData  = SphereSampling.data();
    Mip0Data.Stride = m_uiNumRandomSamplesOnSphere * sizeof(float4);

    TextureData TexData;
    TexData.NumSubresources = 1;
    TexData.pSubResources   = &Mip0Data;

    RefCntAutoPtr<ITexture> ptex2DSphereRandomSampling;
    pDevice->CreateTexture(RandomSphereSamplingTexDesc, &TexData, &ptex2DSphereRandomSampling);
    m_ptex2DSphereRandomSamplingSRV = ptex2DSphereRandomSampling->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ptex2DSphereRandomSamplingSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_tex2DSphereRandomSampling", m_ptex2DSphereRandomSamplingSRV, true);
}

void EpipolarLightScattering::CreateEpipolarTextures(IRenderDevice* pDevice)
{
    TextureDesc TexDesc;
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.MipLevels = 1;
    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    TexDesc.Width     = m_PostProcessingAttribs.uiMaxSamplesInSlice;
    TexDesc.Height    = m_PostProcessingAttribs.uiNumEpipolarSlices;

    {
        // MaxSamplesInSlice x NumSlices RG32F texture to store screen-space coordinates
        // for every epipolar sample
        TexDesc.Name                = "Coordinate Texture";
        TexDesc.Format              = CoordinateTexFmt;
        TexDesc.ClearValue.Format   = TexDesc.Format;
        TexDesc.ClearValue.Color[0] = -1e+30f;
        TexDesc.ClearValue.Color[1] = -1e+30f;
        TexDesc.ClearValue.Color[2] = -1e+30f;
        TexDesc.ClearValue.Color[3] = -1e+30f;

        RefCntAutoPtr<ITexture> tex2DCoordinateTexture;
        pDevice->CreateTexture(TexDesc, nullptr, &tex2DCoordinateTexture);
        auto* tex2DCoordinateTextureSRV = tex2DCoordinateTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex2DCoordinateTextureRTV    = tex2DCoordinateTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        tex2DCoordinateTextureSRV->SetSampler(m_pLinearClampSampler);
        m_pResMapping->AddResource("g_tex2DCoordinates", tex2DCoordinateTextureSRV, false);
    }

    TexDesc.ClearValue.Format = TEX_FORMAT_UNKNOWN;

    {
        TexDesc.Name = "Interpolation Source";
        // MaxSamplesInSlice x NumSlices RG16U texture to store two indices from which
        // the sample should be interpolated, for every epipolar sample

        // In fact we only need RG16U texture to store interpolation source indices.
        // However, NVidia GLES does not supported imge load/store operations on this format,
        // so we have to resort to RGBA32U.
        TexDesc.Format = InterpolationSourceTexFmt;

        TexDesc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
        RefCntAutoPtr<ITexture> tex2DInterpolationSource;
        pDevice->CreateTexture(TexDesc, nullptr, &tex2DInterpolationSource);
        auto* tex2DInterpolationSourceSRV = tex2DInterpolationSource->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        auto* tex2DInterpolationSourceUAV = tex2DInterpolationSource->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
        tex2DInterpolationSourceSRV->SetSampler(m_pPointClampSampler);
        m_pResMapping->AddResource("g_tex2DInterpolationSource", tex2DInterpolationSourceSRV, false);
        m_pResMapping->AddResource("g_rwtex2DInterpolationSource", tex2DInterpolationSourceUAV, false);
    }

    {
        // MaxSamplesInSlice x NumSlices R32F texture to store camera-space Z coordinate,
        // for every epipolar sample
        TexDesc.Name      = "Epipolar Cam Space Z";
        TexDesc.Format    = EpipolarCamSpaceZFmt;
        TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
        RefCntAutoPtr<ITexture> tex2DEpipolarCamSpaceZ;
        pDevice->CreateTexture(TexDesc, nullptr, &tex2DEpipolarCamSpaceZ);
        auto* tex2DEpipolarCamSpaceZSRV = tex2DEpipolarCamSpaceZ->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex2DEpipolarCamSpaceZRTV    = tex2DEpipolarCamSpaceZ->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        tex2DEpipolarCamSpaceZSRV->SetSampler(m_pLinearClampSampler);
        m_pResMapping->AddResource("g_tex2DEpipolarCamSpaceZ", tex2DEpipolarCamSpaceZSRV, false);
    }

    {
        // MaxSamplesInSlice x NumSlices RGBA16F texture to store interpolated inscattered light,
        // for every epipolar sample
        TexDesc.Name                = "Epipolar Inscattering";
        TexDesc.Format              = EpipolarInsctrTexFmt;
        constexpr float flt16max    = 65504.f;
        TexDesc.ClearValue.Format   = TexDesc.Format;
        TexDesc.ClearValue.Color[0] = -flt16max;
        TexDesc.ClearValue.Color[1] = -flt16max;
        TexDesc.ClearValue.Color[2] = -flt16max;
        TexDesc.ClearValue.Color[3] = -flt16max;
        RefCntAutoPtr<ITexture> tex2DEpipolarInscattering;
        pDevice->CreateTexture(TexDesc, nullptr, &tex2DEpipolarInscattering);
        auto* tex2DEpipolarInscatteringSRV = tex2DEpipolarInscattering->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex2DEpipolarInscatteringRTV    = tex2DEpipolarInscattering->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        tex2DEpipolarInscatteringSRV->SetSampler(m_pLinearClampSampler);
        m_pResMapping->AddResource("g_tex2DScatteredColor", tex2DEpipolarInscatteringSRV, false);
    }

    {
        // MaxSamplesInSlice x NumSlices RGBA16F texture to store initial inscattered light,
        // for every epipolar sample
        TexDesc.Name                = "Initial Scattered Light";
        TexDesc.ClearValue.Format   = TexDesc.Format;
        TexDesc.ClearValue.Color[0] = 0;
        TexDesc.ClearValue.Color[1] = 0;
        TexDesc.ClearValue.Color[2] = 0;
        TexDesc.ClearValue.Color[3] = 0;
        RefCntAutoPtr<ITexture> tex2DInitialScatteredLight;
        pDevice->CreateTexture(TexDesc, nullptr, &tex2DInitialScatteredLight);
        auto* tex2DInitialScatteredLightSRV = tex2DInitialScatteredLight->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex2DInitialScatteredLightRTV    = tex2DInitialScatteredLight->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        tex2DInitialScatteredLightSRV->SetSampler(m_pLinearClampSampler);
        m_pResMapping->AddResource("g_tex2DInitialInsctrIrradiance", tex2DInitialScatteredLightSRV, false);
    }

    TexDesc.ClearValue.Format = TEX_FORMAT_UNKNOWN;

    {
        // MaxSamplesInSlice x NumSlices depth stencil texture to mark samples for processing,
        // for every epipolar sample
        TexDesc.Name   = "Epipolar Image Depth";
        TexDesc.Format = TEX_FORMAT_UNKNOWN;
        for (auto Fmt : {EpipolarImageDepthFmt0, EpipolarImageDepthFmt1})
        {
            const auto& FmtInfo = pDevice->GetTextureFormatInfoExt(Fmt);
            if (FmtInfo.BindFlags & BIND_DEPTH_STENCIL)
            {
                TexDesc.Format = Fmt;
                break;
            }
        }
        if (TexDesc.Format == TEX_FORMAT_UNKNOWN)
            LOG_ERROR_AND_THROW("Failed to find suitable depth-stencil format for epipolar image depth buffer");

        TexDesc.BindFlags                       = BIND_DEPTH_STENCIL;
        TexDesc.ClearValue.Format               = TexDesc.Format;
        TexDesc.ClearValue.DepthStencil.Depth   = 1;
        TexDesc.ClearValue.DepthStencil.Stencil = 0;
        RefCntAutoPtr<ITexture> tex2DEpipolarImageDepth;
        pDevice->CreateTexture(TexDesc, nullptr, &tex2DEpipolarImageDepth);
        m_ptex2DEpipolarImageDSV = tex2DEpipolarImageDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    }
}

void EpipolarLightScattering::CreateSliceEndPointsTexture(IRenderDevice* pDevice)
{
    // NumSlices x 1 RGBA32F texture to store end point coordinates for every epipolar slice
    TextureDesc TexDesc;
    TexDesc.Name      = "Slice Endpoints";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.MipLevels = 1;
    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    TexDesc.Width     = m_PostProcessingAttribs.uiNumEpipolarSlices;
    TexDesc.Height    = 1;
    TexDesc.Format    = SliceEndpointsFmt;

    TexDesc.ClearValue.Format   = TexDesc.Format;
    TexDesc.ClearValue.Color[0] = -1e+30f;
    TexDesc.ClearValue.Color[1] = -1e+30f;
    TexDesc.ClearValue.Color[2] = -1e+30f;
    TexDesc.ClearValue.Color[3] = -1e+30f;

    RefCntAutoPtr<ITexture> tex2DSliceEndpoints;
    pDevice->CreateTexture(TexDesc, nullptr, &tex2DSliceEndpoints);
    auto* tex2DSliceEndpointsSRV = tex2DSliceEndpoints->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ptex2DSliceEndpointsRTV    = tex2DSliceEndpoints->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    tex2DSliceEndpointsSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_tex2DSliceEndPoints", tex2DSliceEndpointsSRV, false);
}

void EpipolarLightScattering::PrecomputeScatteringLUT(IRenderDevice* pDevice, IDeviceContext* pContext)
{
    const int ThreadGroupSize          = pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_GLES ? 8 : 16;
    auto&     PrecomputeSingleSctrTech = m_RenderTech[RENDER_TECH_PRECOMPUTE_SINGLE_SCATTERING];
    if (!PrecomputeSingleSctrTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("THREAD_GROUP_SIZE", ThreadGroupSize);
        Macros.Finalize();
        auto pPrecomputeSingleSctrCS =
            CreateShader(pDevice, "PrecomputeSingleScattering.fx", "PrecomputeSingleScatteringCS",
                         SHADER_TYPE_COMPUTE, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        PrecomputeSingleSctrTech.InitializeComputeTechnique(pDevice, "PrecomputeSingleScattering", pPrecomputeSingleSctrCS, ResourceLayout);
        PrecomputeSingleSctrTech.PrepareSRB(pDevice, m_pResMapping, 0);
    }

    auto& ComputeSctrRadianceTech = m_RenderTech[RENDER_TECH_COMPUTE_SCATTERING_RADIANCE];
    if (!ComputeSctrRadianceTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("THREAD_GROUP_SIZE", ThreadGroupSize);
        Macros.AddShaderMacro("NUM_RANDOM_SPHERE_SAMPLES", static_cast<Int32>(m_uiNumRandomSamplesOnSphere));
        Macros.Finalize();
        auto pComputeSctrRadianceCS =
            CreateShader(pDevice, "ComputeSctrRadiance.fx", "ComputeSctrRadianceCS",
                         SHADER_TYPE_COMPUTE, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        ComputeSctrRadianceTech.InitializeComputeTechnique(pDevice, "ComputeSctrRadiance", pComputeSctrRadianceCS, ResourceLayout);
        ComputeSctrRadianceTech.PrepareSRB(pDevice, m_pResMapping, 0);
    }

    auto& ComputeScatteringOrderTech = m_RenderTech[RENDER_TECH_COMPUTE_SCATTERING_ORDER];
    if (!ComputeScatteringOrderTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("THREAD_GROUP_SIZE", ThreadGroupSize);
        Macros.Finalize();
        auto pComputeScatteringOrderCS =
            CreateShader(pDevice, "ComputeScatteringOrder.fx", "ComputeScatteringOrderCS",
                         SHADER_TYPE_COMPUTE, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        ComputeScatteringOrderTech.InitializeComputeTechnique(pDevice, "ComputeScatteringOrder", pComputeScatteringOrderCS, ResourceLayout);
        ComputeScatteringOrderTech.PrepareSRB(pDevice, m_pResMapping, 0);
    }

    auto& InitHighOrderScatteringTech = m_RenderTech[RENDER_TECH_INIT_HIGH_ORDER_SCATTERING];
    if (!InitHighOrderScatteringTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("THREAD_GROUP_SIZE", ThreadGroupSize);
        Macros.Finalize();
        auto pInitHighOrderScatteringCS =
            CreateShader(pDevice, "InitHighOrderScattering.fx", "InitHighOrderScatteringCS",
                         SHADER_TYPE_COMPUTE, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        InitHighOrderScatteringTech.InitializeComputeTechnique(pDevice, "InitHighOrderScattering", pInitHighOrderScatteringCS, ResourceLayout);
        InitHighOrderScatteringTech.PrepareSRB(pDevice, m_pResMapping, 0);
    }

    auto& UpdateHighOrderScatteringTech = m_RenderTech[RENDER_TECH_UPDATE_HIGH_ORDER_SCATTERING];
    if (!UpdateHighOrderScatteringTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("THREAD_GROUP_SIZE", ThreadGroupSize);
        Macros.Finalize();
        auto pUpdateHighOrderScatteringCS =
            CreateShader(pDevice, "UpdateHighOrderScattering.fx", "UpdateHighOrderScatteringCS",
                         SHADER_TYPE_COMPUTE, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        UpdateHighOrderScatteringTech.InitializeComputeTechnique(pDevice, "UpdateHighOrderScattering", pUpdateHighOrderScatteringCS, ResourceLayout);
        UpdateHighOrderScatteringTech.PrepareSRB(pDevice, m_pResMapping, 0);
    }

    auto& CombineScatteringOrdersTech = m_RenderTech[RENDER_TECH_COMBINE_SCATTERING_ORDERS];
    if (!CombineScatteringOrdersTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("THREAD_GROUP_SIZE", ThreadGroupSize);
        Macros.Finalize();
        auto pCombineScatteringOrdersCS =
            CreateShader(pDevice, "CombineScatteringOrders.fx", "CombineScatteringOrdersCS",
                         SHADER_TYPE_COMPUTE, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
        CombineScatteringOrdersTech.InitializeComputeTechnique(pDevice, "CombineScatteringOrders", pCombineScatteringOrdersCS, ResourceLayout);
        CombineScatteringOrdersTech.PrepareSRB(pDevice, m_pResMapping, 0);
    }

    if (!m_ptex2DSphereRandomSamplingSRV)
        CreateRandomSphereSamplingTexture(pDevice);

    TextureDesc PrecomputedSctrTexDesc;
    PrecomputedSctrTexDesc.Type      = RESOURCE_DIM_TEX_3D;
    PrecomputedSctrTexDesc.Width     = m_iPrecomputedSctrUDim;
    PrecomputedSctrTexDesc.Height    = m_iPrecomputedSctrVDim;
    PrecomputedSctrTexDesc.Depth     = m_iPrecomputedSctrWDim * m_iPrecomputedSctrQDim;
    PrecomputedSctrTexDesc.MipLevels = 1;
    PrecomputedSctrTexDesc.Format    = TEX_FORMAT_RGBA16_FLOAT;
    PrecomputedSctrTexDesc.Usage     = USAGE_DEFAULT;
    PrecomputedSctrTexDesc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;

    if (!m_ptex3DSingleScatteringSRV)
    {
        RefCntAutoPtr<ITexture> ptex3DSingleSctr, ptex3DMultipleSctr;
        pDevice->CreateTexture(PrecomputedSctrTexDesc, nullptr, &ptex3DSingleSctr);
        m_ptex3DSingleScatteringSRV = ptex3DSingleSctr->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex3DSingleScatteringSRV->SetSampler(m_pLinearClampSampler);
        m_pResMapping->AddResource("g_rwtex3DSingleScattering", ptex3DSingleSctr->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS), true);

        // We have to bother with two texture, because HLSL only allows read-write operations on single
        // component textures
        pDevice->CreateTexture(PrecomputedSctrTexDesc, nullptr, &m_ptex3DHighOrderSctr);
        pDevice->CreateTexture(PrecomputedSctrTexDesc, nullptr, &m_ptex3DHighOrderSctr2);
        m_ptex3DHighOrderSctr->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(m_pLinearClampSampler);
        m_ptex3DHighOrderSctr2->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(m_pLinearClampSampler);


        pDevice->CreateTexture(PrecomputedSctrTexDesc, nullptr, &ptex3DMultipleSctr);
        m_ptex3DMultipleScatteringSRV = ptex3DMultipleSctr->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex3DMultipleScatteringSRV->SetSampler(m_pLinearClampSampler);
        m_pResMapping->AddResource("g_rwtex3DMultipleSctr", ptex3DMultipleSctr->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS), true);

        m_pResMapping->AddResource("g_tex3DSingleSctrLUT", m_ptex3DSingleScatteringSRV, true);

        m_pResMapping->AddResource("g_tex3DMultipleSctrLUT", m_ptex3DMultipleScatteringSRV, true);
    }

    // Precompute single scattering
    PrecomputeSingleSctrTech.SRB->BindResources(SHADER_TYPE_COMPUTE, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
    DispatchComputeAttribs DispatchAttrs{
        PrecomputedSctrTexDesc.Width / ThreadGroupSize,
        PrecomputedSctrTexDesc.Height / ThreadGroupSize,
        PrecomputedSctrTexDesc.Depth};
    PrecomputeSingleSctrTech.DispatchCompute(pContext, DispatchAttrs);

    // Precompute multiple scattering
    // We need higher precision to store intermediate data
    PrecomputedSctrTexDesc.Format = TEX_FORMAT_RGBA32_FLOAT;
    RefCntAutoPtr<ITexture>     ptex3DSctrRadiance, ptex3DInsctrOrder;
    RefCntAutoPtr<ITextureView> ptex3DSctrRadianceSRV, ptex3DInsctrOrderSRV;
    pDevice->CreateTexture(PrecomputedSctrTexDesc, nullptr, &ptex3DSctrRadiance);
    pDevice->CreateTexture(PrecomputedSctrTexDesc, nullptr, &ptex3DInsctrOrder);
    ptex3DSctrRadianceSRV = ptex3DSctrRadiance->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    ptex3DInsctrOrderSRV  = ptex3DInsctrOrder->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    ptex3DSctrRadianceSRV->SetSampler(m_pLinearClampSampler);
    ptex3DInsctrOrderSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_rwtex3DSctrRadiance", ptex3DSctrRadiance->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS), true);
    m_pResMapping->AddResource("g_rwtex3DInsctrOrder", ptex3DInsctrOrder->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS), true);


    ComputeSctrRadianceTech.SRB->BindResources(SHADER_TYPE_COMPUTE, m_pResMapping, 0);
    ComputeScatteringOrderTech.SRB->BindResources(SHADER_TYPE_COMPUTE, m_pResMapping, 0);
    InitHighOrderScatteringTech.SRB->BindResources(SHADER_TYPE_COMPUTE, m_pResMapping, 0);
    UpdateHighOrderScatteringTech.SRB->BindResources(SHADER_TYPE_COMPUTE, m_pResMapping, 0);

    const int iNumScatteringOrders = pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_GLES ? 3 : 4;
    for (int iSctrOrder = 1; iSctrOrder < iNumScatteringOrders; ++iSctrOrder)
    {
        // Step 1: compute differential in-scattering
        ComputeSctrRadianceTech.SRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex3DPreviousSctrOrder")->Set((iSctrOrder == 1) ? m_ptex3DSingleScatteringSRV : ptex3DInsctrOrderSRV);
        ComputeSctrRadianceTech.DispatchCompute(pContext, DispatchAttrs);

        // It seemse like on Intel GPU, the driver accumulates work into big batch.
        // The resulting batch turns out to be too big for GPU to process it in allowed time
        // limit, and the system kills the driver. So we have to flush the command buffer to
        // force execution of compute shaders.
        pContext->Flush();

        // Step 2: integrate differential in-scattering
        ComputeScatteringOrderTech.SRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex3DPointwiseSctrRadiance")->Set(ptex3DSctrRadianceSRV);
        ComputeScatteringOrderTech.DispatchCompute(pContext, DispatchAttrs);

        RenderTechnique* pRenderTech = nullptr;
        // Step 3: accumulate high-order scattering scattering
        if (iSctrOrder == 1)
        {
            pRenderTech = &m_RenderTech[RENDER_TECH_INIT_HIGH_ORDER_SCATTERING];
        }
        else
        {
            pRenderTech = &m_RenderTech[RENDER_TECH_UPDATE_HIGH_ORDER_SCATTERING];
            std::swap(m_ptex3DHighOrderSctr, m_ptex3DHighOrderSctr2);
            pRenderTech->SRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex3DHighOrderOrderScattering")->Set(m_ptex3DHighOrderSctr2->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        }
        pRenderTech->SRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_rwtex3DHighOrderSctr")->Set(m_ptex3DHighOrderSctr->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
        pRenderTech->SRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex3DCurrentOrderScattering")->Set(ptex3DInsctrOrderSRV);
        pRenderTech->DispatchCompute(pContext, DispatchAttrs);

        // Flush the command buffer to force execution of compute shaders and avoid device
        // reset on low-end Intel GPUs.
        pContext->Flush();
    }

    // Note that m_ptex3DHighOrderSctr and m_ptex3DHighOrderSctr2 are ping-ponged during pre-processing
    m_ptex3DHighOrderScatteringSRV = m_ptex3DHighOrderSctr->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ptex3DHighOrderScatteringSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_tex3DHighOrderSctrLUT", m_ptex3DHighOrderScatteringSRV, false);


    // Combine single scattering and higher order scattering into single texture
    CombineScatteringOrdersTech.SRB->BindResources(SHADER_TYPE_COMPUTE, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
    CombineScatteringOrdersTech.DispatchCompute(pContext, DispatchAttrs);

    // Remove temporary textures from the resource mapping
    m_pResMapping->RemoveResourceByName("g_rwtex3DSctrRadiance");
    m_pResMapping->RemoveResourceByName("g_rwtex3DInsctrOrder");

    m_uiUpToDateResourceFlags |= UpToDateResourceFlags::PrecomputedIntegralsTex;
}

void EpipolarLightScattering::CreateLowResLuminanceTexture(IRenderDevice* pDevice, IDeviceContext* pDeviceCtx)
{
    // Create low-resolution texture to store image luminance
    TextureDesc TexDesc;
    TexDesc.Name      = "Low Res Luminance";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = 1 << (sm_iLowResLuminanceMips - 1);
    TexDesc.Height    = 1 << (sm_iLowResLuminanceMips - 1);
    TexDesc.Format    = WeightedLogLumTexFmt,
    TexDesc.MipLevels = sm_iLowResLuminanceMips;
    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    TexDesc.MiscFlags = MISC_TEXTURE_FLAG_GENERATE_MIPS;

    RefCntAutoPtr<ITexture> tex2DLowResLuminance;
    pDevice->CreateTexture(TexDesc, nullptr, &tex2DLowResLuminance);
    m_ptex2DLowResLuminanceSRV = tex2DLowResLuminance->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ptex2DLowResLuminanceRTV = tex2DLowResLuminance->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    m_ptex2DLowResLuminanceSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_tex2DLowResLuminance", m_ptex2DLowResLuminanceSRV, false);


    TexDesc.Name      = "Average Luminance";
    TexDesc.Width     = 1;
    TexDesc.Height    = 1;
    TexDesc.MipLevels = 1;
    TexDesc.MiscFlags = MISC_TEXTURE_FLAG_NONE;
    TexDesc.Format    = AverageLuminanceTexFmt;

    TexDesc.ClearValue.Color[0] = 0.1f;
    TexDesc.ClearValue.Color[1] = 0.1f;
    TexDesc.ClearValue.Color[2] = 0.1f;
    TexDesc.ClearValue.Color[3] = 0.1f;

    RefCntAutoPtr<ITexture> tex2DAverageLuminance;
    pDevice->CreateTexture(TexDesc, nullptr, &tex2DAverageLuminance);
    auto* tex2DAverageLuminanceSRV = tex2DAverageLuminance->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ptex2DAverageLuminanceRTV    = tex2DAverageLuminance->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    tex2DAverageLuminanceSRV->SetSampler(m_pLinearClampSampler);
    // Set intial luminance to 1
    ITextureView* pRTVs[] = {m_ptex2DAverageLuminanceRTV};
    pDeviceCtx->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pDeviceCtx->ClearRenderTarget(m_ptex2DAverageLuminanceRTV, TexDesc.ClearValue.Color, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    tex2DAverageLuminanceSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_tex2DAverageLuminance", tex2DAverageLuminanceSRV, false);
}

void EpipolarLightScattering::CreateSliceUVDirAndOriginTexture(IRenderDevice* pDevice)
{
    TextureDesc TexDesc;
    TexDesc.Name      = "Slice UV Dir and Origin";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = m_PostProcessingAttribs.uiNumEpipolarSlices;
    TexDesc.Height    = m_PostProcessingAttribs.iNumCascades;
    TexDesc.Format    = SliceUVDirAndOriginTexFmt;
    TexDesc.MipLevels = 1;
    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

    RefCntAutoPtr<ITexture> tex2DSliceUVDirAndOrigin;
    pDevice->CreateTexture(TexDesc, nullptr, &tex2DSliceUVDirAndOrigin);
    auto* tex2DSliceUVDirAndOriginSRV = tex2DSliceUVDirAndOrigin->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ptex2DSliceUVDirAndOriginRTV    = tex2DSliceUVDirAndOrigin->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    tex2DSliceUVDirAndOriginSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_tex2DSliceUVDirAndOrigin", tex2DSliceUVDirAndOriginSRV, false);
}

void EpipolarLightScattering::CreateCamSpaceZTexture(IRenderDevice* pDevice)
{
    TextureDesc TexDesc;
    TexDesc.Name      = "Cam-space Z";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = m_uiBackBufferWidth;
    TexDesc.Height    = m_uiBackBufferHeight;
    TexDesc.Format    = CamSpaceZFmt;
    TexDesc.MipLevels = 1;
    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

    RefCntAutoPtr<ITexture> ptex2DCamSpaceZ;
    pDevice->CreateTexture(TexDesc, nullptr, &ptex2DCamSpaceZ);
    m_ptex2DCamSpaceZRTV    = ptex2DCamSpaceZ->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    auto* tex2DCamSpaceZSRV = ptex2DCamSpaceZ->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    tex2DCamSpaceZSRV->SetSampler(m_pLinearClampSampler);

    // Add texture to resource mapping
    m_pResMapping->AddResource("g_tex2DCamSpaceZ", tex2DCamSpaceZSRV, false);
}

void EpipolarLightScattering::ReconstructCameraSpaceZ()
{
    // Depth buffer is non-linear and cannot be interpolated directly
    // We have to reconstruct camera space z to be able to use bilinear filtering
    auto& ReconstrCamSpaceZTech = m_RenderTech[RENDER_TECH_RECONSTRUCT_CAM_SPACE_Z];
    if (!ReconstrCamSpaceZTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();
        auto pReconstrCamSpaceZPS =
            CreateShader(m_FrameAttribs.pDevice, "ReconstructCameraSpaceZ.fx", "ReconstructCameraSpaceZPS",
                         SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        ReconstrCamSpaceZTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "ReconstructCameraSpaceZPSO", m_pFullScreenTriangleVS,
                                                                    pReconstrCamSpaceZPS, ResourceLayout, CamSpaceZFmt);
        // Bind input resources required by the shader
        ReconstrCamSpaceZTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        ReconstrCamSpaceZTech.SRBDependencyFlags =
            SRB_DEPENDENCY_CAMERA_ATTRIBS |
            SRB_DEPENDENCY_SRC_DEPTH_BUFFER;
    }
    ReconstrCamSpaceZTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING);
    ReconstrCamSpaceZTech.SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DDepthBuffer")->Set(m_FrameAttribs.ptex2DSrcDepthBufferSRV);
    ITextureView* ppRTVs[] = {m_ptex2DCamSpaceZRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    ReconstrCamSpaceZTech.Render(m_FrameAttribs.pDeviceContext);
}

void EpipolarLightScattering::RenderSliceEndpoints()
{
    auto& RendedSliceEndpointsTech = m_RenderTech[RENDER_TECH_RENDER_SLICE_END_POINTS];
    if (!RendedSliceEndpointsTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();

        auto pRendedSliceEndpointsPS =
            CreateShader(m_FrameAttribs.pDevice, "RenderSliceEndPoints.fx", "GenerateSliceEndpointsPS",
                         SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        ShaderResourceVariableDesc Vars[] =
            {
                {SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}};
        ResourceLayout.Variables    = Vars;
        ResourceLayout.NumVariables = _countof(Vars);
        RendedSliceEndpointsTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "RenderSliceEndPoints", m_pFullScreenTriangleVS,
                                                                       pRendedSliceEndpointsPS, ResourceLayout, SliceEndpointsFmt);
        // Bind static resources required by the shaders
        RendedSliceEndpointsTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        RendedSliceEndpointsTech.PSODependencyFlags = PSO_DEPENDENCY_OPTIMIZE_SAMPLE_LOCATIONS;
        RendedSliceEndpointsTech.SRBDependencyFlags = 0;
    }

    RendedSliceEndpointsTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);
    ITextureView* ppRTVs[] = {m_ptex2DSliceEndpointsRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    RendedSliceEndpointsTech.Render(m_FrameAttribs.pDeviceContext);
}

void EpipolarLightScattering::RenderCoordinateTexture()
{
    auto& RendedCoordTexTech = m_RenderTech[RENDER_TECH_RENDER_COORD_TEXTURE];
    if (!RendedCoordTexTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();
        auto                       pRendedCoordTexPS = CreateShader(m_FrameAttribs.pDevice, "RenderCoordinateTexture.fx", "GenerateCoordinateTexturePS",
                                              SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ImmutableSamplerDesc ImtblSamplers[] =
        {
            {SHADER_TYPE_PIXEL, "g_tex2DCamSpaceZ", Sam_LinearClamp}
        };
        // clang-format on
        ResourceLayout.ImmutableSamplers     = ImtblSamplers;
        ResourceLayout.NumImmutableSamplers  = _countof(ImtblSamplers);
        TEXTURE_FORMAT RTVFmts[]             = {CoordinateTexFmt, EpipolarCamSpaceZFmt};
        auto           EpipolarImageDepthFmt = m_ptex2DEpipolarImageDSV->GetTexture()->GetDesc().Format;
        RendedCoordTexTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "RenderCoordinateTexture", m_pFullScreenTriangleVS,
                                                                 pRendedCoordTexPS, ResourceLayout, 2, RTVFmts, EpipolarImageDepthFmt, DSS_IncStencilAlways);
        RendedCoordTexTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        RendedCoordTexTech.PSODependencyFlags = 0;
        RendedCoordTexTech.SRBDependencyFlags =
            SRB_DEPENDENCY_CAM_SPACE_Z_TEX |
            SRB_DEPENDENCY_SLICE_END_POINTS_TEX;
    }

    RendedCoordTexTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);

    ITextureView* ppRTVs[] = {m_ptex2DCoordinateTextureRTV, m_ptex2DEpipolarCamSpaceZRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(2, ppRTVs, m_ptex2DEpipolarImageDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    // Clear both render targets with values that can't be correct projection space coordinates and camera space Z:
    float InvalidCoords[] = {-1e+30f, -1e+30f, -1e+30f, -1e+30f};
    m_FrameAttribs.pDeviceContext->ClearRenderTarget(m_ptex2DCoordinateTextureRTV, InvalidCoords, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_FrameAttribs.pDeviceContext->ClearRenderTarget(m_ptex2DEpipolarCamSpaceZRTV, InvalidCoords, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_FrameAttribs.pDeviceContext->ClearDepthStencil(m_ptex2DEpipolarImageDSV, CLEAR_DEPTH_FLAG | CLEAR_STENCIL_FLAG, 1.0, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    // Depth stencil state is configured to always increment stencil value. If coordinates are outside the screen,
    // the pixel shader discards the pixel and stencil value is left untouched. All such pixels will be skipped from
    // further processing
    RendedCoordTexTech.Render(m_FrameAttribs.pDeviceContext);
}

void EpipolarLightScattering::RenderCoarseUnshadowedInctr()
{
    auto& RenderCoarseUnshadowedInsctrTech = m_RenderTech[RENDER_TECH_RENDER_COARSE_UNSHADOWED_INSCTR];
    if (!RenderCoarseUnshadowedInsctrTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();
        auto EntryPoint =
            m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR ?
            "RenderCoarseUnshadowedInsctrAndExtinctionPS" :
            "RenderCoarseUnshadowedInsctrPS";

        auto pRenderCoarseUnshadowedInsctrPS =
            CreateShader(m_FrameAttribs.pDevice, "CoarseInsctr.fx", EntryPoint, SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        std::vector<ShaderResourceVariableDesc> Vars;
        std::vector<ImmutableSamplerDesc>       ImtblSamplers;

        std::unordered_set<std::string> ResourceNames;

        const auto ResCount = pRenderCoarseUnshadowedInsctrPS->GetResourceCount();
        for (Uint32 r = 0; r < ResCount; ++r)
        {
            ShaderResourceDesc ResourceDesc;
            pRenderCoarseUnshadowedInsctrPS->GetResourceDesc(r, ResourceDesc);
            ResourceNames.emplace(ResourceDesc.Name);
        }

        // clang-format off
        const std::array<std::string, 4> StaticTextures =
        {
            std::string{"g_tex3DSingleSctrLUT"},
            std::string{"g_tex3DHighOrderSctrLUT"},
            std::string{"g_tex3DMultipleSctrLUT"},
            std::string{"g_tex2DOccludedNetDensityToAtmTop"}
        };
        // clang-format on
        for (const auto& Tex : StaticTextures)
        {
            if (ResourceNames.find(Tex) != ResourceNames.end())
            {
                Vars.emplace_back(SHADER_TYPE_PIXEL, Tex.c_str(), SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
                ImtblSamplers.emplace_back(SHADER_TYPE_PIXEL, Tex.c_str(), Sam_LinearClamp);
            }
        }

        if (ResourceNames.find("cbParticipatingMediaScatteringParams") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbParticipatingMediaScatteringParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        if (ResourceNames.find("cbPostProcessingAttribs") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

        ResourceLayout.Variables            = Vars.data();
        ResourceLayout.NumVariables         = static_cast<Uint32>(Vars.size());
        ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
        ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

        const auto* PSOName = m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR ?
            "RenderCoarseUnshadowedInsctrAndExtinctionPSO" :
            "RenderCoarseUnshadowedInsctrPSO";
        TEXTURE_FORMAT RTVFmts[]             = {EpipolarInsctrTexFmt, EpipolarExtinctionFmt};
        Uint8          NumRTVs               = m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR ? 2 : 1;
        auto           EpipolarImageDepthFmt = m_ptex2DEpipolarImageDSV->GetTexture()->GetDesc().Format;
        RenderCoarseUnshadowedInsctrTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, PSOName, m_pFullScreenTriangleVS,
                                                                               pRenderCoarseUnshadowedInsctrPS, ResourceLayout, NumRTVs, RTVFmts,
                                                                               EpipolarImageDepthFmt, DSS_StencilEqKeepStencil);
        RenderCoarseUnshadowedInsctrTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        RenderCoarseUnshadowedInsctrTech.PSODependencyFlags =
            PSO_DEPENDENCY_EXTINCTION_EVAL_MODE |
            PSO_DEPENDENCY_SINGLE_SCATTERING_MODE |
            PSO_DEPENDENCY_MULTIPLE_SCATTERING_MODE;
        RenderCoarseUnshadowedInsctrTech.SRBDependencyFlags =
            SRB_DEPENDENCY_CAMERA_ATTRIBS |
            SRB_DEPENDENCY_LIGHT_ATTRIBS |
            SRB_DEPENDENCY_EPIPOLAR_CAM_SPACE_Z_TEX |
            SRB_DEPENDENCY_COORDINATE_TEX;
    }

    if (m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR &&
        !m_ptex2DEpipolarExtinctionRTV)
    {
        // Extinction texture size is num_slices x max_samples_in_slice. So the texture must be re-created when either changes.
        CreateExtinctionTexture(m_FrameAttribs.pDevice);
    }

    ITextureView* ppRTVs[] = {m_ptex2DEpipolarInscatteringRTV, m_ptex2DEpipolarExtinctionRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR ? 2 : 1, ppRTVs, m_ptex2DEpipolarImageDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float       flt16max        = 65504.f; // Epipolar Inscattering is 16-bit float
    const float InvalidInsctr[] = {-flt16max, -flt16max, -flt16max, -flt16max};
    if (m_ptex2DEpipolarInscatteringRTV)
        m_FrameAttribs.pDeviceContext->ClearRenderTarget(m_ptex2DEpipolarInscatteringRTV, InvalidInsctr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR)
    {
        const float One[] = {1, 1, 1, 1};
        m_FrameAttribs.pDeviceContext->ClearRenderTarget(m_ptex2DEpipolarExtinctionRTV, One, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    RenderCoarseUnshadowedInsctrTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);
    RenderCoarseUnshadowedInsctrTech.Render(m_FrameAttribs.pDeviceContext, 1);
}

void EpipolarLightScattering::RefineSampleLocations()
{
    auto& RefineSampleLocationsTech = m_RenderTech[RENDER_TECH_REFINE_SAMPLE_LOCATIONS];
    if (!RefineSampleLocationsTech.PSO)
    {
        // Thread group size must be at least as large as initial sample step
        m_uiSampleRefinementCSThreadGroupSize = std::max(m_uiSampleRefinementCSMinimumThreadGroupSize, m_PostProcessingAttribs.uiInitialSampleStepInSlice);
        // Thread group size cannot be larger than the total number of samples in slice
        m_uiSampleRefinementCSThreadGroupSize = std::min(m_uiSampleRefinementCSThreadGroupSize, m_PostProcessingAttribs.uiMaxSamplesInSlice);
        // Using small group size is inefficient since a lot of SIMD lanes become idle

        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        // clang-format off
        Macros.AddShaderMacro("INITIAL_SAMPLE_STEP",  static_cast<Int32>(m_PostProcessingAttribs.uiInitialSampleStepInSlice));
        Macros.AddShaderMacro("THREAD_GROUP_SIZE",    static_cast<Int32>(m_uiSampleRefinementCSThreadGroupSize));
        Macros.AddShaderMacro("REFINEMENT_CRITERION", m_PostProcessingAttribs.iRefinementCriterion);
        Macros.AddShaderMacro("AUTO_EXPOSURE",        m_PostProcessingAttribs.ToneMapping.bAutoExposure);
        // clang-format on
        Macros.Finalize();

        auto pRefineSampleLocationsCS = CreateShader(m_FrameAttribs.pDevice, "RefineSampleLocations.fx", "RefineSampleLocationsCS",
                                                     SHADER_TYPE_COMPUTE, Macros);

        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_COMPUTE, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };
        // clang-format on
        ResourceLayout.Variables    = Vars;
        ResourceLayout.NumVariables = _countof(Vars);

        RefineSampleLocationsTech.InitializeComputeTechnique(m_FrameAttribs.pDevice, "RefineSampleLocations", pRefineSampleLocationsCS, ResourceLayout);
        RefineSampleLocationsTech.PSO->BindStaticResources(SHADER_TYPE_COMPUTE, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        RefineSampleLocationsTech.PSODependencyFlags =
            PSO_DEPENDENCY_INITIAL_SAMPLE_STEP |
            PSO_DEPENDENCY_REFINEMENT_CRITERION |
            PSO_DEPENDENCY_AUTO_EXPOSURE;
        RefineSampleLocationsTech.SRBDependencyFlags =
            SRB_DEPENDENCY_INTERPOLATION_SOURCE_TEX |
            SRB_DEPENDENCY_COORDINATE_TEX |
            SRB_DEPENDENCY_EPIPOLAR_CAM_SPACE_Z_TEX |
            SRB_DEPENDENCY_EPIPOLAR_INSCTR_TEX |
            SRB_DEPENDENCY_AVERAGE_LUMINANCE_TEX;
    }

    RefineSampleLocationsTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);
    DispatchComputeAttribs DispatchAttrs{
        m_PostProcessingAttribs.uiMaxSamplesInSlice / m_uiSampleRefinementCSThreadGroupSize,
        m_PostProcessingAttribs.uiNumEpipolarSlices};
    RefineSampleLocationsTech.DispatchCompute(m_FrameAttribs.pDeviceContext, DispatchAttrs);
}

void EpipolarLightScattering::MarkRayMarchingSamples()
{
    auto& MarkRayMarchingSamplesInStencilTech = m_RenderTech[RENDER_TECH_MARK_RAY_MARCHING_SAMPLES];
    if (!MarkRayMarchingSamplesInStencilTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();

        auto pMarkRayMarchingSamplesInStencilPS =
            CreateShader(m_FrameAttribs.pDevice, "MarkRayMarchingSamples.fx", "MarkRayMarchingSamplesInStencilPS",
                         SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        auto EpipolarImageDepthFmt         = m_ptex2DEpipolarImageDSV->GetTexture()->GetDesc().Format;
        MarkRayMarchingSamplesInStencilTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "MarkRayMarchingSamples",
                                                                                  m_pFullScreenTriangleVS, pMarkRayMarchingSamplesInStencilPS,
                                                                                  ResourceLayout, 0, nullptr, EpipolarImageDepthFmt, DSS_StencilEqIncStencil);
        MarkRayMarchingSamplesInStencilTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        MarkRayMarchingSamplesInStencilTech.SRBDependencyFlags = SRB_DEPENDENCY_INTERPOLATION_SOURCE_TEX;
    }

    // Mark ray marching samples in the stencil
    // The depth stencil state is configured to pass only pixels, whose stencil value equals 1. Thus all epipolar samples with
    // coordinates outsied the screen (generated on the previous pass) are automatically discarded. The pixel shader only
    // passes samples which are interpolated from themselves, the rest are discarded. Thus after this pass all ray
    // marching samples will be marked with 2 in stencil
    MarkRayMarchingSamplesInStencilTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);
    m_FrameAttribs.pDeviceContext->SetRenderTargets(0, nullptr, m_ptex2DEpipolarImageDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    MarkRayMarchingSamplesInStencilTech.Render(m_FrameAttribs.pDeviceContext, 1);
}

void EpipolarLightScattering::RenderSliceUVDirAndOrig()
{
    auto& RenderSliceUVDirInSMTech = m_RenderTech[RENDER_TECH_RENDER_SLICE_UV_DIRECTION];
    if (!RenderSliceUVDirInSMTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();

        auto pRenderSliceUVDirInSMPS =
            CreateShader(m_FrameAttribs.pDevice, "SliceUVDirection.fx", "RenderSliceUVDirInShadowMapTexturePS",
                         SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };
        // clang-format on
        ResourceLayout.Variables    = Vars;
        ResourceLayout.NumVariables = _countof(Vars);

        RenderSliceUVDirInSMTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "RenderSliceUVDirAndOrigin",
                                                                       m_pFullScreenTriangleVS, pRenderSliceUVDirInSMPS,
                                                                       ResourceLayout, SliceUVDirAndOriginTexFmt);
        RenderSliceUVDirInSMTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        RenderSliceUVDirInSMTech.SRBDependencyFlags =
            SRB_DEPENDENCY_CAMERA_ATTRIBS |
            SRB_DEPENDENCY_LIGHT_ATTRIBS |
            SRB_DEPENDENCY_SLICE_END_POINTS_TEX;
    }

    if (!m_ptex2DSliceUVDirAndOriginRTV)
    {
        CreateSliceUVDirAndOriginTexture(m_FrameAttribs.pDevice);
    }

    RenderSliceUVDirInSMTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);

    ITextureView* ppRTVs[] = {m_ptex2DSliceUVDirAndOriginRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    RenderSliceUVDirInSMTech.Render(m_FrameAttribs.pDeviceContext);
}

void EpipolarLightScattering::Build1DMinMaxMipMap(int iCascadeIndex)
{
    auto& InitMinMaxShadowMapTech = m_RenderTech[RENDER_TECH_INIT_MIN_MAX_SHADOW_MAP];
    if (!InitMinMaxShadowMapTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("IS_32BIT_MIN_MAX_MAP", m_PostProcessingAttribs.bIs32BitMinMaxMipMap);
        Macros.Finalize();

        auto                       pInitializeMinMaxShadowMapPS = CreateShader(m_FrameAttribs.pDevice, "InitializeMinMaxShadowMap.fx", "InitializeMinMaxShadowMapPS",
                                                         SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_PIXEL, m_bUseCombinedMinMaxTexture ? "cbPostProcessingAttribs" : "cbMiscDynamicParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };

        ImmutableSamplerDesc ImtblSamplers[] =
        {
            {SHADER_TYPE_PIXEL, "g_tex2DLightSpaceDepthMap", Sam_LinearClamp} // Linear, not comparison
        };
        // clang-format on

        ResourceLayout.Variables            = Vars;
        ResourceLayout.NumVariables         = _countof(Vars);
        ResourceLayout.ImmutableSamplers    = ImtblSamplers;
        ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

        TEXTURE_FORMAT ShadowMapFmt = m_ptex2DMinMaxShadowMapSRV[0]->GetTexture()->GetDesc().Format;
        InitMinMaxShadowMapTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "InitMinMaxShadowMap",
                                                                      m_pFullScreenTriangleVS, pInitializeMinMaxShadowMapPS,
                                                                      ResourceLayout, ShadowMapFmt);
        InitMinMaxShadowMapTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        InitMinMaxShadowMapTech.PSODependencyFlags =
            PSO_DEPENDENCY_USE_1D_MIN_MAX_TREE |
            PSO_DEPENDENCY_USE_COMBINED_MIN_MAX_TEX |
            PSO_DEPENDENCY_IS_32_BIT_MIN_MAX_TREE;
        InitMinMaxShadowMapTech.SRBDependencyFlags =
            SRB_DEPENDENCY_SLICE_UV_DIR_TEX |
            SRB_DEPENDENCY_SHADOW_MAP;
    }

    auto& ComputeMinMaxSMLevelTech = m_RenderTech[RENDER_TECH_COMPUTE_MIN_MAX_SHADOW_MAP_LEVEL];
    if (!ComputeMinMaxSMLevelTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();

        auto pComputeMinMaxSMLevelPS = CreateShader(m_FrameAttribs.pDevice, "ComputeMinMaxShadowMapLevel.fx", "ComputeMinMaxShadowMapLevelPS",
                                                    SHADER_TYPE_PIXEL, Macros);

        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc VarDesc[] =
        {
            {SHADER_TYPE_PIXEL, "cbMiscDynamicParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };
        // clang-format on
        ResourceLayout.Variables    = VarDesc;
        ResourceLayout.NumVariables = _countof(VarDesc);

        TEXTURE_FORMAT ShadowMapFmt = m_ptex2DMinMaxShadowMapSRV[0]->GetTexture()->GetDesc().Format;
        ComputeMinMaxSMLevelTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "ComputeMinMaxShadowMapLevel",
                                                                       m_pFullScreenTriangleVS, pComputeMinMaxSMLevelPS, ResourceLayout, ShadowMapFmt);
        ComputeMinMaxSMLevelTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        ComputeMinMaxSMLevelTech.PSODependencyFlags = PSO_DEPENDENCY_IS_32_BIT_MIN_MAX_TREE;
        ComputeMinMaxSMLevelTech.SRBDependencyFlags = SRB_DEPENDENCY_MIN_MAX_SHADOW_MAP;

        m_pComputeMinMaxSMLevelSRB[0].Release();
        m_pComputeMinMaxSMLevelSRB[1].Release();
    }

    if (!m_pComputeMinMaxSMLevelSRB[0])
    {
        for (int Parity = 0; Parity < 2; ++Parity)
        {
            ComputeMinMaxSMLevelTech.PSO->CreateShaderResourceBinding(&m_pComputeMinMaxSMLevelSRB[Parity], true);
            auto* pVar = m_pComputeMinMaxSMLevelSRB[Parity]->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DMinMaxLightSpaceDepth");
            pVar->Set(m_ptex2DMinMaxShadowMapSRV[Parity]);
            m_pComputeMinMaxSMLevelSRB[Parity]->BindResources(SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, m_pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
        }
    }

    auto iMinMaxTexHeight = m_PostProcessingAttribs.uiNumEpipolarSlices;
    if (m_bUseCombinedMinMaxTexture)
        iMinMaxTexHeight *= (m_PostProcessingAttribs.iNumCascades - m_PostProcessingAttribs.iFirstCascadeToRayMarch);

    auto tex2DMinMaxShadowMap0 = m_ptex2DMinMaxShadowMapRTV[0]->GetTexture();
    auto tex2DMinMaxShadowMap1 = m_ptex2DMinMaxShadowMapRTV[1]->GetTexture();

    // Computing min/max mip map using compute shader is much slower because a lot of threads are idle
    Uint32 uiXOffset     = 0;
    Uint32 uiPrevXOffset = 0;
    Uint32 uiParity      = 0;
#ifdef DILIGENT_DEBUG
    {
        const auto& MinMaxShadowMapTexDesc = m_ptex2DMinMaxShadowMapRTV[0]->GetTexture()->GetDesc();
        VERIFY_EXPR(MinMaxShadowMapTexDesc.Width == m_PostProcessingAttribs.uiMinMaxShadowMapResolution);
        VERIFY_EXPR(MinMaxShadowMapTexDesc.Height == iMinMaxTexHeight);
    }
#endif
    // Note that we start rendering min/max shadow map from step == 2
    for (Uint32 iStep = 2; iStep <= (Uint32)m_PostProcessingAttribs.fMaxShadowMapStep; iStep *= 2, uiParity = (uiParity + 1) % 2)
    {
        // Use two buffers which are in turn used as the source and destination
        ITextureView* pRTVs[] = {m_ptex2DMinMaxShadowMapRTV[uiParity]};
        m_FrameAttribs.pDeviceContext->SetRenderTargets(_countof(pRTVs), pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Viewport VP;
        VP.Width    = static_cast<float>(m_PostProcessingAttribs.uiMinMaxShadowMapResolution / iStep);
        VP.Height   = static_cast<float>(iMinMaxTexHeight);
        VP.TopLeftX = static_cast<float>(uiXOffset);
        VP.TopLeftY = 0;
        m_FrameAttribs.pDeviceContext->SetViewports(1, &VP, 0, 0);

        // Set source and destination min/max data offsets:
        {
            MapHelper<MiscDynamicParams> pMiscDynamicParams(m_FrameAttribs.pDeviceContext, m_pcbMiscParams, MAP_WRITE, MAP_FLAG_DISCARD);
            pMiscDynamicParams->ui4SrcMinMaxLevelXOffset = uiPrevXOffset;
            pMiscDynamicParams->ui4DstMinMaxLevelXOffset = uiXOffset;
            pMiscDynamicParams->fCascadeInd              = static_cast<float>(iCascadeIndex);
        }

        if (iStep == 2)
        {
            // At the initial pass, the shader gathers 8 depths which will be used for
            // PCF filtering at the sample location and its next neighbor along the slice
            // and outputs min/max depths
            InitMinMaxShadowMapTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);
            InitMinMaxShadowMapTech.Render(m_FrameAttribs.pDeviceContext);
        }
        else
        {
            // At the subsequent passes, the shader loads two min/max values from the next finer level
            // to compute next level of the binary tree
            RenderFullScreenTriangle(m_FrameAttribs.pDeviceContext, ComputeMinMaxSMLevelTech.PSO, m_pComputeMinMaxSMLevelSRB[(uiParity + 1) % 2]);
        }

        // All the data must reside in 0-th texture, so copy current level, if necessary, from 1-st texture
        if (uiParity == 1)
        {
            Box SrcBox;
            SrcBox.MinX = uiXOffset;
            SrcBox.MaxX = uiXOffset + m_PostProcessingAttribs.uiMinMaxShadowMapResolution / iStep;
            SrcBox.MinY = 0;
            SrcBox.MaxY = iMinMaxTexHeight;

            m_FrameAttribs.pDeviceContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

            CopyTextureAttribs CopyAttribs(tex2DMinMaxShadowMap1, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, tex2DMinMaxShadowMap0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            CopyAttribs.pSrcBox = &SrcBox;
            CopyAttribs.DstX    = uiXOffset;
            m_FrameAttribs.pDeviceContext->CopyTexture(CopyAttribs);
        }

        uiPrevXOffset = uiXOffset;
        uiXOffset += m_PostProcessingAttribs.uiMinMaxShadowMapResolution / iStep;
    }
}

void EpipolarLightScattering::DoRayMarching(Uint32 uiMaxStepsAlongRay,
                                            int    iCascadeIndex)
{
    auto& DoRayMarchTech = m_RenderTech[m_PostProcessingAttribs.bUse1DMinMaxTree ? RENDER_TECH_RAY_MARCH_MIN_MAX_OPT : RENDER_TECH_RAY_MARCH_NO_MIN_MAX_OPT];
    if (!DoRayMarchTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("CASCADE_PROCESSING_MODE", m_PostProcessingAttribs.iCascadeProcessingMode);
        Macros.AddShaderMacro("USE_1D_MIN_MAX_TREE", m_PostProcessingAttribs.bUse1DMinMaxTree);
        Macros.Finalize();

        auto pDoRayMarchPS =
            CreateShader(m_FrameAttribs.pDevice, "RayMarch.fx", "RayMarchPS", SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        std::vector<ShaderResourceVariableDesc> Vars;
        std::vector<ImmutableSamplerDesc>       ImtblSamplers;

        std::unordered_set<std::string> ResourceNames;

        const auto ResCount = pDoRayMarchPS->GetResourceCount();
        for (Uint32 r = 0; r < ResCount; ++r)
        {
            ShaderResourceDesc ResourceDesc;
            pDoRayMarchPS->GetResourceDesc(r, ResourceDesc);
            ResourceNames.emplace(ResourceDesc.Name);
        }

        // clang-format off
        const std::array<std::string, 4> StaticLinearTextures =
        {
            "g_tex3DSingleSctrLUT",
            "g_tex3DHighOrderSctrLUT",
            "g_tex3DMultipleSctrLUT",
            "g_tex2DOccludedNetDensityToAtmTop"
        };
        // clang-format on
        for (const auto& Tex : StaticLinearTextures)
        {
            if (ResourceNames.find(Tex) != ResourceNames.end())
            {
                Vars.emplace_back(SHADER_TYPE_PIXEL, Tex.c_str(), SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
                ImtblSamplers.emplace_back(SHADER_TYPE_PIXEL, Tex.c_str(), Sam_LinearClamp);
            }
        }

        if (ResourceNames.find("cbParticipatingMediaScatteringParams") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbParticipatingMediaScatteringParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        if (ResourceNames.find("cbPostProcessingAttribs") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        if (ResourceNames.find("cbMiscDynamicParams") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbMiscDynamicParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

        if (ResourceNames.find("g_tex2DCamSpaceZ") != ResourceNames.end())
            ImtblSamplers.emplace_back(SHADER_TYPE_PIXEL, "g_tex2DCamSpaceZ", Sam_LinearClamp);

        ResourceLayout.Variables            = Vars.data();
        ResourceLayout.NumVariables         = static_cast<Uint32>(Vars.size());
        ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
        ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

        auto EpipolarImageDepthFmt = m_ptex2DEpipolarImageDSV->GetTexture()->GetDesc().Format;
        DoRayMarchTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "RayMarch", m_pFullScreenTriangleVS,
                                                             pDoRayMarchPS, ResourceLayout, EpipolarInsctrTexFmt, EpipolarImageDepthFmt,
                                                             DSS_StencilEqKeepStencil, BS_AdditiveBlend);
        DoRayMarchTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        DoRayMarchTech.PSODependencyFlags =
            PSO_DEPENDENCY_USE_1D_MIN_MAX_TREE |
            PSO_DEPENDENCY_CASCADE_PROCESSING_MODE |
            PSO_DEPENDENCY_USE_COMBINED_MIN_MAX_TEX |
            PSO_DEPENDENCY_ENABLE_LIGHT_SHAFTS |
            PSO_DEPENDENCY_MULTIPLE_SCATTERING_MODE |
            PSO_DEPENDENCY_SINGLE_SCATTERING_MODE;

        DoRayMarchTech.SRBDependencyFlags =
            SRB_DEPENDENCY_CAMERA_ATTRIBS |
            SRB_DEPENDENCY_LIGHT_ATTRIBS |
            SRB_DEPENDENCY_EPIPOLAR_CAM_SPACE_Z_TEX |
            SRB_DEPENDENCY_SLICE_UV_DIR_TEX |
            SRB_DEPENDENCY_SHADOW_MAP |
            SRB_DEPENDENCY_MIN_MAX_SHADOW_MAP |
            SRB_DEPENDENCY_COORDINATE_TEX |
            SRB_DEPENDENCY_CAM_SPACE_Z_TEX |
            SRB_DEPENDENCY_SRC_COLOR_BUFFER |
            SRB_DEPENDENCY_AVERAGE_LUMINANCE_TEX;
    }

    {
        MapHelper<MiscDynamicParams> pMiscDynamicParams(m_FrameAttribs.pDeviceContext, m_pcbMiscParams, MAP_WRITE, MAP_FLAG_DISCARD);
        pMiscDynamicParams->fMaxStepsAlongRay = static_cast<float>(uiMaxStepsAlongRay);
        pMiscDynamicParams->fCascadeInd       = static_cast<float>(iCascadeIndex);
    }

    int iNumInst = 0;
    if (m_PostProcessingAttribs.bEnableLightShafts)
    {
        switch (m_PostProcessingAttribs.iCascadeProcessingMode)
        {
            case CASCADE_PROCESSING_MODE_SINGLE_PASS:
            case CASCADE_PROCESSING_MODE_MULTI_PASS:
                iNumInst = 1;
                break;
            case CASCADE_PROCESSING_MODE_MULTI_PASS_INST:
                iNumInst = m_PostProcessingAttribs.iNumCascades - m_PostProcessingAttribs.iFirstCascadeToRayMarch;
                break;
        }
    }
    else
    {
        iNumInst = 1;
    }

    // Depth stencil view now contains 2 for these pixels, for which ray marchings is to be performed
    // Depth stencil state is configured to pass only these pixels and discard the rest
    DoRayMarchTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);

    ITextureView* ppRTVs[] = {m_ptex2DInitialScatteredLightRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(1, ppRTVs, m_ptex2DEpipolarImageDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DoRayMarchTech.Render(m_FrameAttribs.pDeviceContext, 2, iNumInst);
}

void EpipolarLightScattering::InterpolateInsctrIrradiance()
{
    auto& InterpolateIrradianceTech = m_RenderTech[RENDER_TECH_INTERPOLATE_IRRADIANCE];
    if (!InterpolateIrradianceTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();

        auto pInterpolateIrradiancePS = CreateShader(m_FrameAttribs.pDevice, "InterpolateIrradiance.fx", "InterpolateIrradiancePS",
                                                     SHADER_TYPE_PIXEL, Macros);

        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        InterpolateIrradianceTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "InterpolateIrradiance",
                                                                        m_pFullScreenTriangleVS, pInterpolateIrradiancePS,
                                                                        ResourceLayout, EpipolarInsctrTexFmt);
        InterpolateIrradianceTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        InterpolateIrradianceTech.SRBDependencyFlags =
            SRB_DEPENDENCY_INTERPOLATION_SOURCE_TEX |
            SRB_DEPENDENCY_INITIAL_SCTR_LIGHT_TEX;
    }

    InterpolateIrradianceTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);

    ITextureView* ppRTVs[] = {m_ptex2DEpipolarInscatteringRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    InterpolateIrradianceTech.Render(m_FrameAttribs.pDeviceContext);
}


void EpipolarLightScattering::UnwarpEpipolarScattering(bool bRenderLuminance)
{
    static constexpr Uint32 SRBDependencies =
        SRB_DEPENDENCY_CAMERA_ATTRIBS |
        SRB_DEPENDENCY_LIGHT_ATTRIBS |
        SRB_DEPENDENCY_SRC_COLOR_BUFFER |
        SRB_DEPENDENCY_SLICE_END_POINTS_TEX |
        SRB_DEPENDENCY_EPIPOLAR_CAM_SPACE_Z_TEX |
        SRB_DEPENDENCY_EPIPOLAR_INSCTR_TEX |
        SRB_DEPENDENCY_SLICE_UV_DIR_TEX |
        SRB_DEPENDENCY_MIN_MAX_SHADOW_MAP |
        SRB_DEPENDENCY_SHADOW_MAP |
        SRB_DEPENDENCY_AVERAGE_LUMINANCE_TEX |
        SRB_DEPENDENCY_EPIPOLAR_EXTINCTION_TEX;

    auto& UnwarpEpipolarSctrImgTech = m_RenderTech[RENDER_TECH_UNWARP_EPIPOLAR_SCATTERING];
    if (!UnwarpEpipolarSctrImgTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        // clang-format off
        Macros.AddShaderMacro("PERFORM_TONE_MAPPING",                 true);
        Macros.AddShaderMacro("AUTO_EXPOSURE",                        m_PostProcessingAttribs.ToneMapping.bAutoExposure);
        Macros.AddShaderMacro("TONE_MAPPING_MODE",                    m_PostProcessingAttribs.ToneMapping.iToneMappingMode);
        Macros.AddShaderMacro("CORRECT_INSCATTERING_AT_DEPTH_BREAKS", m_PostProcessingAttribs.bCorrectScatteringAtDepthBreaks);
        // clang-format on
        Macros.Finalize();

        auto pUnwarpEpipolarSctrImgPS = CreateShader(m_FrameAttribs.pDevice, "UnwarpEpipolarScattering.fx", "ApplyInscatteredRadiancePS",
                                                     SHADER_TYPE_PIXEL, Macros);

        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            //{SHADER_TYPE_PIXEL, "cbParticipatingMediaScatteringParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}, // To avoid warnings
            {SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };

        std::vector<ImmutableSamplerDesc> ImtblSamplers =
        {
            {SHADER_TYPE_PIXEL, "g_tex2DSliceEndPoints",    Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DEpipolarCamSpaceZ", Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DScatteredColor",    Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DCamSpaceZ",         Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DColorBuffer",       Sam_PointClamp}
        };
        // clang-format on
        if (m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR)
            ImtblSamplers.emplace_back(SHADER_TYPE_PIXEL, "g_tex2DEpipolarExtinction", Sam_LinearClamp);

        ResourceLayout.Variables            = Vars;
        ResourceLayout.NumVariables         = _countof(Vars);
        ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
        ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

        UnwarpEpipolarSctrImgTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "UnwarpEpipolarScattering",
                                                                        m_pFullScreenTriangleVS, pUnwarpEpipolarSctrImgPS,
                                                                        ResourceLayout, m_BackBufferFmt, m_DepthBufferFmt, DSS_Default);
        UnwarpEpipolarSctrImgTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, 0);

        UnwarpEpipolarSctrImgTech.PSODependencyFlags =
            PSO_DEPENDENCY_AUTO_EXPOSURE |
            PSO_DEPENDENCY_TONE_MAPPING_MODE |
            PSO_DEPENDENCY_CORRECT_SCATTERING |
            PSO_DEPENDENCY_EXTINCTION_EVAL_MODE;

        UnwarpEpipolarSctrImgTech.SRBDependencyFlags = SRBDependencies;
    }

    auto& UnwarpAndRenderLuminanceTech = m_RenderTech[RENDER_TECH_UNWARP_AND_RENDER_LUMINANCE];
    if (!UnwarpAndRenderLuminanceTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("PERFORM_TONE_MAPPING", false);
        // No inscattering correction - we need to render the entire image in low resolution
        Macros.AddShaderMacro("CORRECT_INSCATTERING_AT_DEPTH_BREAKS", false);
        Macros.Finalize();

        auto pUnwarpAndRenderLuminancePS = CreateShader(m_FrameAttribs.pDevice, "UnwarpEpipolarScattering.fx", "ApplyInscatteredRadiancePS",
                                                        SHADER_TYPE_PIXEL, Macros);

        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            //{SHADER_TYPE_PIXEL, "cbParticipatingMediaScatteringParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}, // To avoid warnings
            {SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };

        std::vector<ImmutableSamplerDesc> ImtblSamplers =
        {
            {SHADER_TYPE_PIXEL, "g_tex2DSliceEndPoints",    Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DEpipolarCamSpaceZ", Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DScatteredColor",    Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DCamSpaceZ",         Sam_LinearClamp},
            {SHADER_TYPE_PIXEL, "g_tex2DColorBuffer",       Sam_PointClamp}
        };
        // clang-format on

        if (m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR)
            ImtblSamplers.emplace_back(SHADER_TYPE_PIXEL, "g_tex2DEpipolarExtinction", Sam_LinearClamp);

        ResourceLayout.Variables            = Vars;
        ResourceLayout.NumVariables         = _countof(Vars);
        ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
        ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

        UnwarpAndRenderLuminanceTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "UnwarpAndRenderLuminance",
                                                                           m_pFullScreenTriangleVS, pUnwarpAndRenderLuminancePS,
                                                                           ResourceLayout, WeightedLogLumTexFmt);
        UnwarpAndRenderLuminanceTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, 0);

        UnwarpAndRenderLuminanceTech.PSODependencyFlags = PSO_DEPENDENCY_EXTINCTION_EVAL_MODE;
        UnwarpAndRenderLuminanceTech.SRBDependencyFlags = SRBDependencies;
    }

    // Unwarp inscattering image and apply it to attenuated backgorund
    if (bRenderLuminance)
    {
        UnwarpAndRenderLuminanceTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING);

        // Disable depth testing - we need to render the entire image in low resolution
        UnwarpAndRenderLuminanceTech.Render(m_FrameAttribs.pDeviceContext);
    }
    else
    {
        UnwarpEpipolarSctrImgTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING);

        // Enable depth testing to write 0.0 to the depth buffer. All pixel that require
        // inscattering correction (if enabled) will be discarded, so that 1.0 will be retained
        // This 1.0 will then be used to perform inscattering correction
        UnwarpEpipolarSctrImgTech.Render(m_FrameAttribs.pDeviceContext);
    }
}

void EpipolarLightScattering::UpdateAverageLuminance()
{
    auto& UpdateAverageLuminanceTech = m_RenderTech[RENDER_TECH_UPDATE_AVERAGE_LUMINANCE];
    if (!UpdateAverageLuminanceTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.AddShaderMacro("LIGHT_ADAPTATION", m_PostProcessingAttribs.ToneMapping.bLightAdaptation);
        // static_cast<int>() is required because Run() gets its arguments by reference
        // and gcc will try to find reference to sm_iLowResLuminanceMips, which does not exist
        Macros.AddShaderMacro("LOW_RES_LUMINANCE_MIPS", static_cast<int>(sm_iLowResLuminanceMips));
        Macros.Finalize();

        auto pUpdateAverageLuminancePS = CreateShader(m_FrameAttribs.pDevice, "UpdateAverageLuminance.fx", "UpdateAverageLuminancePS",
                                                      SHADER_TYPE_PIXEL, Macros);

        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_PIXEL, "cbMiscDynamicParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };
        // clang-format on

        if (m_PostProcessingAttribs.ToneMapping.bLightAdaptation)
        {
            ResourceLayout.Variables    = Vars;
            ResourceLayout.NumVariables = _countof(Vars);
        }
        UpdateAverageLuminanceTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "UpdateAverageLuminance",
                                                                         m_pFullScreenTriangleVS, pUpdateAverageLuminancePS,
                                                                         ResourceLayout, AverageLuminanceTexFmt, TEX_FORMAT_UNKNOWN,
                                                                         DSS_DisableDepth, BS_AlphaBlend);
        UpdateAverageLuminanceTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        UpdateAverageLuminanceTech.PSODependencyFlags = PSO_DEPENDENCY_LIGHT_ADAPTATION;
    }

    {
        MapHelper<MiscDynamicParams> pMiscDynamicParams(m_FrameAttribs.pDeviceContext, m_pcbMiscParams, MAP_WRITE, MAP_FLAG_DISCARD);
        pMiscDynamicParams->fElapsedTime = (float)m_FrameAttribs.dElapsedTime;
    }

    UpdateAverageLuminanceTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping);

    ITextureView* ppRTVs[] = {m_ptex2DAverageLuminanceRTV};
    m_FrameAttribs.pDeviceContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    UpdateAverageLuminanceTech.Render(m_FrameAttribs.pDeviceContext);
}


void EpipolarLightScattering::FixInscatteringAtDepthBreaks(Uint32               uiMaxStepsAlongRay,
                                                           EFixInscatteringMode Mode)
{
    auto& FixInsctrAtDepthBreaksTech = m_RenderTech[RENDER_TECH_FIX_INSCATTERING_LUM_ONLY + static_cast<int>(Mode)];
    if (!FixInsctrAtDepthBreaksTech.PSO)
    {
        bool              bRenderLuminance = Mode == EFixInscatteringMode::LuminanceOnly;
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        // clang-format off
        Macros.AddShaderMacro("CASCADE_PROCESSING_MODE", CASCADE_PROCESSING_MODE_SINGLE_PASS);
        Macros.AddShaderMacro("PERFORM_TONE_MAPPING",    !bRenderLuminance);
        Macros.AddShaderMacro("AUTO_EXPOSURE",           m_PostProcessingAttribs.ToneMapping.bAutoExposure);
        Macros.AddShaderMacro("TONE_MAPPING_MODE",       m_PostProcessingAttribs.ToneMapping.iToneMappingMode);
        Macros.AddShaderMacro("USE_1D_MIN_MAX_TREE",     false);
        // clang-format on
        Macros.Finalize();

        auto pFixInsctrAtDepthBreaksPS =
            CreateShader(m_FrameAttribs.pDevice, "RayMarch.fx", "FixAndApplyInscatteredRadiancePS",
                         SHADER_TYPE_PIXEL, Macros);
        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        std::vector<ShaderResourceVariableDesc> Vars;
        std::vector<ImmutableSamplerDesc>       ImtblSamplers;

        std::unordered_set<std::string> ResourceNames;

        const auto ResCount = pFixInsctrAtDepthBreaksPS->GetResourceCount();
        for (Uint32 r = 0; r < ResCount; ++r)
        {
            ShaderResourceDesc ResourceDesc;
            pFixInsctrAtDepthBreaksPS->GetResourceDesc(r, ResourceDesc);
            ResourceNames.emplace(ResourceDesc.Name);
        }

        // clang-format off
        const std::array<std::string, 4> StaticLinearTextures =
        {
            "g_tex3DSingleSctrLUT",
            "g_tex3DHighOrderSctrLUT",
            "g_tex3DMultipleSctrLUT",
            "g_tex2DOccludedNetDensityToAtmTop"
        };
        // clang-format on
        for (const auto& Tex : StaticLinearTextures)
        {
            if (ResourceNames.find(Tex) != ResourceNames.end())
            {
                Vars.emplace_back(SHADER_TYPE_PIXEL, Tex.c_str(), SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
                ImtblSamplers.emplace_back(SHADER_TYPE_PIXEL, Tex.c_str(), Sam_LinearClamp);
            }
        }

        if (ResourceNames.find("cbParticipatingMediaScatteringParams") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbParticipatingMediaScatteringParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        if (ResourceNames.find("cbPostProcessingAttribs") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        if (ResourceNames.find("cbMiscDynamicParams") != ResourceNames.end())
            Vars.emplace_back(SHADER_TYPE_PIXEL, "cbMiscDynamicParams", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

        if (ResourceNames.find("g_tex2DCamSpaceZ") != ResourceNames.end())
            ImtblSamplers.emplace_back(SHADER_TYPE_PIXEL, "g_tex2DCamSpaceZ", Sam_LinearClamp);

        ResourceLayout.Variables            = Vars.data();
        ResourceLayout.NumVariables         = static_cast<Uint32>(Vars.size());
        ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
        ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

        if (Mode == EFixInscatteringMode::LuminanceOnly)
        {
            // Luminance Only
            // Disable depth and stencil tests to render all pixels
            // Use default blend state to overwrite old luminance values
            FixInsctrAtDepthBreaksTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "FixInsctrAtDepthBreaksLumOnly",
                                                                             m_pFullScreenTriangleVS, pFixInsctrAtDepthBreaksPS,
                                                                             ResourceLayout, WeightedLogLumTexFmt);
        }
        else if (Mode == EFixInscatteringMode::FixInscattering)
        {
            // Fix Inscattering
            // Depth breaks are marked with 1.0 in depth, so we enable depth test
            // to render only pixels that require correction
            // Use default blend state - the rendering is always done in single pass
            FixInsctrAtDepthBreaksTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "FixInsctrAtDepthBreaks",
                                                                             m_pFullScreenTriangleVS, pFixInsctrAtDepthBreaksPS,
                                                                             ResourceLayout, m_BackBufferFmt, m_DepthBufferFmt, DSS_Default);
        }
        else if (Mode == EFixInscatteringMode::FullScreenRayMarching)
        {
            // Full Screen Ray Marching
            // Disable depth and stencil tests since we are performing
            // full screen ray marching
            // Use default blend state - the rendering is always done in single pass
            FixInsctrAtDepthBreaksTech.InitializeFullScreenTriangleTechnique(m_FrameAttribs.pDevice, "FixInsctrAtDepthBreaks",
                                                                             m_pFullScreenTriangleVS, pFixInsctrAtDepthBreaksPS,
                                                                             ResourceLayout, m_BackBufferFmt, m_DepthBufferFmt, DSS_DisableDepth);
        }
        FixInsctrAtDepthBreaksTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);


        FixInsctrAtDepthBreaksTech.PSODependencyFlags =
            PSO_DEPENDENCY_CASCADE_PROCESSING_MODE |
            PSO_DEPENDENCY_USE_COMBINED_MIN_MAX_TEX |
            PSO_DEPENDENCY_ENABLE_LIGHT_SHAFTS |
            PSO_DEPENDENCY_MULTIPLE_SCATTERING_MODE |
            PSO_DEPENDENCY_SINGLE_SCATTERING_MODE |
            PSO_DEPENDENCY_AUTO_EXPOSURE |
            PSO_DEPENDENCY_TONE_MAPPING_MODE;

        FixInsctrAtDepthBreaksTech.SRBDependencyFlags =
            SRB_DEPENDENCY_CAMERA_ATTRIBS |
            SRB_DEPENDENCY_LIGHT_ATTRIBS |
            SRB_DEPENDENCY_EPIPOLAR_CAM_SPACE_Z_TEX |
            SRB_DEPENDENCY_SLICE_UV_DIR_TEX |
            SRB_DEPENDENCY_MIN_MAX_SHADOW_MAP |
            SRB_DEPENDENCY_COORDINATE_TEX |
            SRB_DEPENDENCY_CAM_SPACE_Z_TEX |
            SRB_DEPENDENCY_SRC_COLOR_BUFFER |
            SRB_DEPENDENCY_AVERAGE_LUMINANCE_TEX;
    }

    {
        MapHelper<MiscDynamicParams> pMiscDynamicParams(m_FrameAttribs.pDeviceContext, m_pcbMiscParams, MAP_WRITE, MAP_FLAG_DISCARD);
        pMiscDynamicParams->fMaxStepsAlongRay = static_cast<float>(uiMaxStepsAlongRay);
        pMiscDynamicParams->fCascadeInd       = static_cast<float>(m_PostProcessingAttribs.iFirstCascadeToRayMarch);
    }

    FixInsctrAtDepthBreaksTech.PrepareSRB(m_FrameAttribs.pDevice, m_pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING);

    FixInsctrAtDepthBreaksTech.Render(m_FrameAttribs.pDeviceContext);
}

void EpipolarLightScattering::RenderSampleLocations()
{
    auto& RenderSampleLocationsTech = m_RenderTech[RENDER_TECH_RENDER_SAMPLE_LOCATIONS];
    if (!RenderSampleLocationsTech.PSO)
    {
        ShaderMacroHelper Macros;
        DefineMacros(Macros);
        Macros.Finalize();

        auto pRenderSampleLocationsVS = CreateShader(m_FrameAttribs.pDevice, "RenderSampling.fx", "RenderSampleLocationsVS",
                                                     SHADER_TYPE_VERTEX, Macros);
        auto pRenderSampleLocationsPS = CreateShader(m_FrameAttribs.pDevice, "RenderSampling.fx", "RenderSampleLocationsPS",
                                                     SHADER_TYPE_PIXEL, Macros);

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_VERTEX, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };
        // clang-format on

        PSODesc.ResourceLayout.Variables    = Vars;
        PSODesc.ResourceLayout.NumVariables = _countof(Vars);

        PSODesc.Name                                          = "Render sample locations PSO";
        auto& GraphicsPipeline                                = PSOCreateInfo.GraphicsPipeline;
        GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
        GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_NONE;
        GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = true;
        GraphicsPipeline.DepthStencilDesc                     = DSS_DisableDepth;
        GraphicsPipeline.BlendDesc                            = BS_AlphaBlend;
        PSOCreateInfo.pVS                                     = pRenderSampleLocationsVS;
        PSOCreateInfo.pPS                                     = pRenderSampleLocationsPS;
        GraphicsPipeline.NumRenderTargets                     = 1;
        GraphicsPipeline.RTVFormats[0]                        = m_BackBufferFmt;
        GraphicsPipeline.DSVFormat                            = m_DepthBufferFmt;
        GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        m_FrameAttribs.pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &RenderSampleLocationsTech.PSO);
        RenderSampleLocationsTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        RenderSampleLocationsTech.SRB.Release();

        RenderSampleLocationsTech.PSODependencyFlags = 0;
        RenderSampleLocationsTech.SRBDependencyFlags =
            SRB_DEPENDENCY_INTERPOLATION_SOURCE_TEX |
            SRB_DEPENDENCY_COORDINATE_TEX;
    }

    if (!RenderSampleLocationsTech.SRB)
    {
        RenderSampleLocationsTech.PSO->CreateShaderResourceBinding(&RenderSampleLocationsTech.SRB, true);
        RenderSampleLocationsTech.SRB->BindResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
    }

    DrawAttribs Attribs;
    Attribs.NumVertices  = 4;
    Attribs.NumInstances = m_PostProcessingAttribs.uiMaxSamplesInSlice * m_PostProcessingAttribs.uiNumEpipolarSlices;
    m_FrameAttribs.pDeviceContext->SetPipelineState(RenderSampleLocationsTech.PSO);
    m_FrameAttribs.pDeviceContext->CommitShaderResources(RenderSampleLocationsTech.SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_FrameAttribs.pDeviceContext->Draw(Attribs);
}

void EpipolarLightScattering::CreateExtinctionTexture(IRenderDevice* pDevice)
{
    TextureDesc TexDesc;
    TexDesc.Name                = "Epipolar Extinction",
    TexDesc.Type                = RESOURCE_DIM_TEX_2D;
    TexDesc.Width               = m_PostProcessingAttribs.uiMaxSamplesInSlice;
    TexDesc.Height              = m_PostProcessingAttribs.uiNumEpipolarSlices;
    TexDesc.Format              = EpipolarExtinctionFmt;
    TexDesc.MipLevels           = 1;
    TexDesc.Usage               = USAGE_DEFAULT;
    TexDesc.BindFlags           = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    TexDesc.ClearValue.Format   = TEX_FORMAT_UNKNOWN;
    TexDesc.ClearValue.Color[0] = 1;
    TexDesc.ClearValue.Color[1] = 1;
    TexDesc.ClearValue.Color[2] = 1;
    TexDesc.ClearValue.Color[3] = 1;

    // MaxSamplesInSlice x NumSlices RGBA8_UNORM texture to store extinction
    // for every epipolar sample
    RefCntAutoPtr<ITexture> tex2DEpipolarExtinction;
    pDevice->CreateTexture(TexDesc, nullptr, &tex2DEpipolarExtinction);
    auto* tex2DEpipolarExtinctionSRV = tex2DEpipolarExtinction->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    tex2DEpipolarExtinctionSRV->SetSampler(m_pLinearClampSampler);
    m_pResMapping->AddResource("g_tex2DEpipolarExtinction", tex2DEpipolarExtinctionSRV, false);
    m_ptex2DEpipolarExtinctionRTV = tex2DEpipolarExtinction->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
}

void EpipolarLightScattering::CreateAmbientSkyLightTexture(IRenderDevice* pDevice)
{
    TextureDesc TexDesc;
    TexDesc.Name      = "Ambient Sky Light";
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = sm_iAmbientSkyLightTexDim;
    TexDesc.Height    = 1;
    TexDesc.Format    = AmbientSkyLightTexFmt;
    TexDesc.MipLevels = 1;
    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    RefCntAutoPtr<ITexture> tex2DAmbientSkyLight;
    pDevice->CreateTexture(TexDesc, nullptr, &tex2DAmbientSkyLight);

    m_ptex2DAmbientSkyLightSRV = tex2DAmbientSkyLight->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ptex2DAmbientSkyLightRTV = tex2DAmbientSkyLight->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    m_ptex2DAmbientSkyLightSRV->SetSampler(m_pLinearClampSampler);
}

void EpipolarLightScattering::PrepareForNewFrame(FrameAttribs&                   frameAttribs,
                                                 EpipolarLightScatteringAttribs& PPAttribs)
{
    DEV_CHECK_ERR(frameAttribs.ptex2DSrcColorBufferSRV, "Source color buffer SRV must not be null");
    DEV_CHECK_ERR(frameAttribs.ptex2DSrcDepthBufferSRV, "Source depth buffer SRV must not be null");
    DEV_CHECK_ERR(frameAttribs.ptex2DDstColorBufferRTV, "Destination color buffer RTV must not be null");
    DEV_CHECK_ERR(frameAttribs.ptex2DDstDepthBufferDSV, "Source depth buffer DSV must not be null");
    DEV_CHECK_ERR(frameAttribs.ptex2DShadowMapSRV, "Shadow map SRV must not be null");

    DEV_CHECK_ERR(PPAttribs.uiNumEpipolarSlices > 0, "Number of epipolar slices must not be 0");
    DEV_CHECK_ERR(PPAttribs.uiMaxSamplesInSlice > 0, "Max samples in slice must not be 0");
    DEV_CHECK_ERR(PPAttribs.uiInitialSampleStepInSlice > 0, "Initial sample step in slice must not be 0");
    DEV_CHECK_ERR(IsPowerOfTwo(PPAttribs.uiInitialSampleStepInSlice), "Initial sample step in slice (", PPAttribs.uiInitialSampleStepInSlice, ") must be power of two");
    DEV_CHECK_ERR(PPAttribs.uiEpipoleSamplingDensityFactor > 0, "Epipole sampling density factor must not be 0");
    DEV_CHECK_ERR(IsPowerOfTwo(PPAttribs.uiEpipoleSamplingDensityFactor), "Epipole sampling desity factor (", PPAttribs.uiEpipoleSamplingDensityFactor, ") must be power of two");
    DEV_CHECK_ERR(PPAttribs.uiInstrIntegralSteps > 0, "Inscattering integral steps must not be 0");
    DEV_CHECK_ERR(PPAttribs.f2ShadowMapTexelSize.x != 0 && PPAttribs.f2ShadowMapTexelSize.y != 0, "Shadow map texel size must not be 0");
    DEV_CHECK_ERR(PPAttribs.uiMaxSamplesOnTheRay != 0, "Max samples on the ray must not be 0");
    DEV_CHECK_ERR(!PPAttribs.bCorrectScatteringAtDepthBreaks || PPAttribs.uiNumSamplesOnTheRayAtDepthBreak != 0, "Num samples on the ray at depth correction pass must not be 0");
    DEV_CHECK_ERR(PPAttribs.uiMinMaxShadowMapResolution != 0, "Minmax shadow map resolution must not be 0");
    DEV_CHECK_ERR(PPAttribs.iNumCascades != 0, "Num cascades must not be 0");
    DEV_CHECK_ERR(PPAttribs.iFirstCascadeToRayMarch < PPAttribs.iNumCascades, "First cascade to ray march (", PPAttribs.fFirstCascadeToRayMarch, ") is invalid");
    DEV_CHECK_ERR(PPAttribs.fMaxShadowMapStep != 0, "Max shadow map step must not be 0");
    // clang-format off
    DEV_CHECK_ERR(PPAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING ||
                  PPAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_BRUTE_FORCE,
                  "Incorrect light scattering technique (", PPAttribs.iLightSctrTechnique, ")");
    DEV_CHECK_ERR(PPAttribs.iCascadeProcessingMode == CASCADE_PROCESSING_MODE_SINGLE_PASS ||
                  PPAttribs.iCascadeProcessingMode == CASCADE_PROCESSING_MODE_MULTI_PASS ||
                  PPAttribs.iCascadeProcessingMode == CASCADE_PROCESSING_MODE_MULTI_PASS_INST,
                  "Incorrect cascade processing mode (", PPAttribs.iCascadeProcessingMode, ")");
    DEV_CHECK_ERR(PPAttribs.iRefinementCriterion == REFINEMENT_CRITERION_DEPTH_DIFF ||
                  PPAttribs.iRefinementCriterion == REFINEMENT_CRITERION_INSCTR_DIFF,
                  "Incorrect refinement criterion (", PPAttribs.iRefinementCriterion, ")");
    DEV_CHECK_ERR(PPAttribs.iSingleScatteringMode == SINGLE_SCTR_MODE_NONE ||
                  PPAttribs.iSingleScatteringMode == SINGLE_SCTR_MODE_INTEGRATION ||
                  PPAttribs.iSingleScatteringMode == SINGLE_SCTR_MODE_LUT,
                  "Incorrect single scattering mode (", PPAttribs.iSingleScatteringMode, ")");
    DEV_CHECK_ERR(PPAttribs.iMultipleScatteringMode == MULTIPLE_SCTR_MODE_NONE ||
                  PPAttribs.iMultipleScatteringMode == MULTIPLE_SCTR_MODE_UNOCCLUDED ||
                  PPAttribs.iMultipleScatteringMode == MULTIPLE_SCTR_MODE_OCCLUDED,
                  "Incorrect multiple scattering mode (", PPAttribs.iMultipleScatteringMode, ")");
    DEV_CHECK_ERR(PPAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_PER_PIXEL ||
                  PPAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR,
                  "Incorrect extinction evaluation mode (", PPAttribs.iExtinctionEvalMode, ")");
    
    Uint32 StalePSODependencyFlags = 0;
#define CHECK_PSO_DEPENDENCY(Flag, Member)StalePSODependencyFlags |= (PPAttribs.Member != m_PostProcessingAttribs.Member) ? Flag : 0
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_INITIAL_SAMPLE_STEP,        uiInitialSampleStepInSlice);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_EPIPOLE_SAMPLING_DENSITY,   uiEpipoleSamplingDensityFactor);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_CORRECT_SCATTERING,         bCorrectScatteringAtDepthBreaks); 
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_OPTIMIZE_SAMPLE_LOCATIONS,  bOptimizeSampleLocations);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_ENABLE_LIGHT_SHAFTS,        bEnableLightShafts);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_USE_1D_MIN_MAX_TREE,        bUse1DMinMaxTree);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_LIGHT_SCTR_TECHNIQUE,       iLightSctrTechnique);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_CASCADE_PROCESSING_MODE,    iCascadeProcessingMode);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_REFINEMENT_CRITERION,       iRefinementCriterion);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_IS_32_BIT_MIN_MAX_TREE,     bIs32BitMinMaxMipMap);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_MULTIPLE_SCATTERING_MODE,   iMultipleScatteringMode);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_SINGLE_SCATTERING_MODE,     iSingleScatteringMode);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_AUTO_EXPOSURE,              ToneMapping.bAutoExposure);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_TONE_MAPPING_MODE,          ToneMapping.iToneMappingMode);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_LIGHT_ADAPTATION,           ToneMapping.bLightAdaptation);
    CHECK_PSO_DEPENDENCY(PSO_DEPENDENCY_EXTINCTION_EVAL_MODE,       iExtinctionEvalMode);
#undef CHECK_PSO_DEPENDENCY

    bool bUseCombinedMinMaxTexture = PPAttribs.iCascadeProcessingMode == CASCADE_PROCESSING_MODE_SINGLE_PASS     ||
                                     PPAttribs.iCascadeProcessingMode == CASCADE_PROCESSING_MODE_MULTI_PASS_INST ||
                                     PPAttribs.bCorrectScatteringAtDepthBreaks                                    || 
                                     PPAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_BRUTE_FORCE;
    StalePSODependencyFlags |= (m_bUseCombinedMinMaxTexture != bUseCombinedMinMaxTexture) ? PSO_DEPENDENCY_USE_COMBINED_MIN_MAX_TEX : 0;

    auto* pcbCameraAttribs = frameAttribs.pcbCameraAttribs != nullptr ? frameAttribs.pcbCameraAttribs : m_pcbCameraAttribs;
    auto* pcbLightAttribs  = frameAttribs.pcbLightAttribs  != nullptr ? frameAttribs.pcbLightAttribs  : m_pcbLightAttribs;

    UserResourceIds NewUserResourceIds;
    NewUserResourceIds.LightAttribs      = pcbCameraAttribs != nullptr ? pcbCameraAttribs->GetUniqueID() : -1;
    NewUserResourceIds.CameraAttribs     = pcbLightAttribs  != nullptr ? pcbLightAttribs->GetUniqueID()  : -1;
    NewUserResourceIds.SrcColorBufferSRV = frameAttribs.ptex2DSrcColorBufferSRV->GetUniqueID();
    NewUserResourceIds.SrcDepthBufferSRV = frameAttribs.ptex2DSrcDepthBufferSRV->GetUniqueID();
    NewUserResourceIds.ShadowMapSRV      = frameAttribs.ptex2DShadowMapSRV->GetUniqueID();
    // clang-format on

    Uint32 StaleSRBDependencyFlags = 0;
#define CHECK_SRB_DEPENDENCY(Flag, Member) StaleSRBDependencyFlags |= (m_UserResourceIds.Member != NewUserResourceIds.Member) ? Flag : 0
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_SRC_COLOR_BUFFER, SrcColorBufferSRV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_SRC_DEPTH_BUFFER, SrcDepthBufferSRV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_SHADOW_MAP, ShadowMapSRV);
#undef CHECK_SRB_DEPENDENCY

    StaleSRBDependencyFlags |= (!pcbCameraAttribs || m_UserResourceIds.CameraAttribs != NewUserResourceIds.CameraAttribs) ? SRB_DEPENDENCY_CAMERA_ATTRIBS : 0;
    StaleSRBDependencyFlags |= (!pcbLightAttribs || m_UserResourceIds.LightAttribs != NewUserResourceIds.LightAttribs) ? SRB_DEPENDENCY_LIGHT_ATTRIBS : 0;

    if (PPAttribs.uiNumEpipolarSlices != m_PostProcessingAttribs.uiNumEpipolarSlices ||
        PPAttribs.uiMaxSamplesInSlice != m_PostProcessingAttribs.uiMaxSamplesInSlice)
    {
        m_ptex2DCoordinateTextureRTV.Release();     // Max Samples X Num Slices   RG32F
        m_ptex2DEpipolarCamSpaceZRTV.Release();     // Max Samples X Num Slices   R32F
        m_ptex2DEpipolarInscatteringRTV.Release();  // Max Samples X Num Slices   RGBA16F
        m_ptex2DEpipolarExtinctionRTV.Release();    // Max Samples X Num Slices   RGBA8_UNORM
        m_ptex2DEpipolarImageDSV.Release();         // Max Samples X Num Slices   D24S8
        m_ptex2DInitialScatteredLightRTV.Release(); // Max Samples X Num Slices   RGBA16F
        StaleSRBDependencyFlags |= SRB_DEPENDENCY_INTERPOLATION_SOURCE_TEX;
    }

    if (PPAttribs.uiNumEpipolarSlices != m_PostProcessingAttribs.uiNumEpipolarSlices)
    {
        m_ptex2DSliceEndpointsRTV.Release(); // Num Slices  X 1            RGBA32F
    }

    if (PPAttribs.uiNumEpipolarSlices != m_PostProcessingAttribs.uiNumEpipolarSlices ||
        PPAttribs.iNumCascades != m_PostProcessingAttribs.iNumCascades)
    {
        m_ptex2DSliceUVDirAndOriginRTV.Release(); // Num Slices  X Num Cascaes  RGBA32F
    }

    // clang-format off
    if (PPAttribs.uiMinMaxShadowMapResolution != m_PostProcessingAttribs.uiMinMaxShadowMapResolution || 
        PPAttribs.uiNumEpipolarSlices         != m_PostProcessingAttribs.uiNumEpipolarSlices         ||
        PPAttribs.bUse1DMinMaxTree            != m_PostProcessingAttribs.bUse1DMinMaxTree            ||
        PPAttribs.bIs32BitMinMaxMipMap        != m_PostProcessingAttribs.bIs32BitMinMaxMipMap        ||
        bUseCombinedMinMaxTexture             != m_bUseCombinedMinMaxTexture                         ||
        (bUseCombinedMinMaxTexture && 
            (PPAttribs.iFirstCascadeToRayMarch != m_PostProcessingAttribs.iFirstCascadeToRayMarch || 
             PPAttribs.iNumCascades            != m_PostProcessingAttribs.iNumCascades)))
    {
        for (int i = 0; i < _countof(m_ptex2DMinMaxShadowMapSRV); ++i)
            m_ptex2DMinMaxShadowMapSRV[i].Release();
        for (int i = 0; i < _countof(m_ptex2DMinMaxShadowMapRTV); ++i)
            m_ptex2DMinMaxShadowMapRTV[i].Release();
    }

#define CHECK_SRB_DEPENDENCY(Flag, Res)StaleSRBDependencyFlags |= !Res ? Flag : 0
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_COORDINATE_TEX,           m_ptex2DCoordinateTextureRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_SLICE_END_POINTS_TEX,     m_ptex2DSliceEndpointsRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_EPIPOLAR_CAM_SPACE_Z_TEX, m_ptex2DEpipolarCamSpaceZRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_EPIPOLAR_INSCTR_TEX,      m_ptex2DEpipolarInscatteringRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_EPIPOLAR_EXTINCTION_TEX,  m_ptex2DEpipolarExtinctionRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_EPIPOLAR_IMAGE_DEPTH,     m_ptex2DEpipolarImageDSV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_INITIAL_SCTR_LIGHT_TEX,   m_ptex2DInitialScatteredLightRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_AVERAGE_LUMINANCE_TEX,    m_ptex2DAverageLuminanceRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_SLICE_UV_DIR_TEX,         m_ptex2DSliceUVDirAndOriginRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_CAM_SPACE_Z_TEX,          m_ptex2DCamSpaceZRTV);
    CHECK_SRB_DEPENDENCY(SRB_DEPENDENCY_MIN_MAX_SHADOW_MAP,       m_ptex2DMinMaxShadowMapRTV[0]);
#undef CHECK_SRB_DEPENDENCY
    // clang-format on

    for (int i = 0; i < RENDER_TECH_TOTAL_TECHNIQUES; ++i)
        m_RenderTech[i].CheckStaleFlags(StalePSODependencyFlags, StaleSRBDependencyFlags);

    if (StaleSRBDependencyFlags & SRB_DEPENDENCY_SRC_COLOR_BUFFER)
        m_pResMapping->AddResource("g_tex2DColorBuffer", frameAttribs.ptex2DSrcColorBufferSRV, false);

    if (StaleSRBDependencyFlags & SRB_DEPENDENCY_MIN_MAX_SHADOW_MAP)
    {
        m_pComputeMinMaxSMLevelSRB[0].Release();
        m_pComputeMinMaxSMLevelSRB[1].Release();
    }

    //if (PPAttribs.uiExtinctionEvalMode != m_PostProcessingAttribs.uiExtinctionEvalMode)
    //{
    //    m_ptex2DEpipolarExtinctionRTV.Release();
    //}

    // clang-format off
    bool bRecomputeSctrCoeffs = m_PostProcessingAttribs.bUseCustomSctrCoeffs    != PPAttribs.bUseCustomSctrCoeffs    ||
                                m_PostProcessingAttribs.bUseOzoneApproximation  != PPAttribs.bUseOzoneApproximation  ||
                                m_PostProcessingAttribs.fAerosolDensityScale    != PPAttribs.fAerosolDensityScale    ||
                                m_PostProcessingAttribs.fAerosolAbsorbtionScale != PPAttribs.fAerosolAbsorbtionScale ||
                                (PPAttribs.bUseCustomSctrCoeffs && 
                                    (m_PostProcessingAttribs.f4CustomRlghBeta        != PPAttribs.f4CustomRlghBeta ||
                                     m_PostProcessingAttribs.f4CustomMieBeta         != PPAttribs.f4CustomMieBeta ||
                                     m_PostProcessingAttribs.f4CustomOzoneAbsorption != PPAttribs.f4CustomOzoneAbsorption) );
    // clang-format on

    m_PostProcessingAttribs = PPAttribs;

    m_PostProcessingAttribs.f4ScreenResolution = float4(
        static_cast<float>(m_uiBackBufferWidth),
        static_cast<float>(m_uiBackBufferHeight),
        1.f / static_cast<float>(m_uiBackBufferWidth),
        1.f / static_cast<float>(m_uiBackBufferHeight));

    auto   mCameraViewProj = frameAttribs.pCameraAttribs->mViewProjT.Transpose();
    float4 f4LightPosPS    = -frameAttribs.pLightAttribs->f4Direction * mCameraViewProj;
    f4LightPosPS.x /= f4LightPosPS.w;
    f4LightPosPS.y /= f4LightPosPS.w;
    f4LightPosPS.z /= f4LightPosPS.w;
    float fDistToLightOnScreen = length((float2&)f4LightPosPS);
    float fMaxDist             = 100;
    if (fDistToLightOnScreen > fMaxDist)
    {
        f4LightPosPS.x *= fMaxDist / fDistToLightOnScreen;
        f4LightPosPS.y *= fMaxDist / fDistToLightOnScreen;
    }
    m_PostProcessingAttribs.f4LightScreenPos = f4LightPosPS;

    // Note that in fact the outermost visible screen pixels do not land exactly on the boundary (+1 or -1), but are biased by
    // 0.5 screen pixel size inwards. Using these adjusted boundaries improves precision and results in
    // smaller number of pixels which require inscattering correction
    m_PostProcessingAttribs.bIsLightOnScreen =
        (fabs(m_PostProcessingAttribs.f4LightScreenPos.x) <= 1.f - 1.f / (float)m_uiBackBufferWidth &&
         fabs(m_PostProcessingAttribs.f4LightScreenPos.y) <= 1.f - 1.f / (float)m_uiBackBufferHeight);

    m_PostProcessingAttribs.fFirstCascadeToRayMarch = static_cast<float>(m_PostProcessingAttribs.iFirstCascadeToRayMarch);
    m_PostProcessingAttribs.fNumCascades            = static_cast<float>(m_PostProcessingAttribs.iNumCascades);

    m_bUseCombinedMinMaxTexture = bUseCombinedMinMaxTexture;

    m_FrameAttribs                  = frameAttribs;
    m_FrameAttribs.pcbCameraAttribs = pcbCameraAttribs;
    m_FrameAttribs.pcbLightAttribs  = pcbLightAttribs;
    m_UserResourceIds               = NewUserResourceIds;

    if (frameAttribs.pcbCameraAttribs == nullptr)
    {
        if (!m_pcbCameraAttribs)
        {
            CreateUniformBuffer(m_FrameAttribs.pDevice, sizeof(CameraAttribs), "Camera attribs", &m_pcbCameraAttribs);
            VERIFY_EXPR(StaleSRBDependencyFlags & SRB_DEPENDENCY_CAMERA_ATTRIBS);
        }
        MapHelper<CameraAttribs> CamAttribs(m_FrameAttribs.pDeviceContext, m_pcbCameraAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
        *CamAttribs = *m_FrameAttribs.pCameraAttribs;

        m_FrameAttribs.pcbCameraAttribs = m_pcbCameraAttribs;
    }

    if (frameAttribs.pcbLightAttribs == nullptr)
    {
        if (!m_pcbLightAttribs)
        {
            CreateUniformBuffer(m_FrameAttribs.pDevice, sizeof(LightAttribs), "Light attribs", &m_pcbLightAttribs);
            VERIFY_EXPR(StaleSRBDependencyFlags & SRB_DEPENDENCY_LIGHT_ATTRIBS);
        }
        MapHelper<LightAttribs> LightAttribs(m_FrameAttribs.pDeviceContext, m_pcbLightAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
        *LightAttribs = *m_FrameAttribs.pLightAttribs;

        m_FrameAttribs.pcbLightAttribs = m_pcbLightAttribs;
    }

    if (bRecomputeSctrCoeffs)
    {
        m_uiUpToDateResourceFlags &= ~UpToDateResourceFlags::PrecomputedOpticalDepthTex;
        m_uiUpToDateResourceFlags &= ~UpToDateResourceFlags::AmbientSkyLightTex;
        m_uiUpToDateResourceFlags &= ~UpToDateResourceFlags::PrecomputedIntegralsTex;
        ComputeScatteringCoefficients(m_FrameAttribs.pDeviceContext);
    }

    if (!m_ptex2DCoordinateTextureRTV)
    {
        CreateEpipolarTextures(m_FrameAttribs.pDevice);
    }

    if (!m_ptex2DSliceEndpointsRTV)
    {
        CreateSliceEndPointsTexture(m_FrameAttribs.pDevice);
    }

    if (!m_ptex2DCamSpaceZRTV)
    {
        CreateCamSpaceZTexture(m_FrameAttribs.pDevice);
    }

    if (m_PostProcessingAttribs.bEnableLightShafts && m_PostProcessingAttribs.bUse1DMinMaxTree && !m_ptex2DMinMaxShadowMapSRV[0])
    {
        CreateMinMaxShadowMap(m_FrameAttribs.pDevice);
    }

    {
        MapHelper<EpipolarLightScatteringAttribs> pPPAttribsBuffData(m_FrameAttribs.pDeviceContext, m_pcbPostProcessingAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
        memcpy(pPPAttribsBuffData, &m_PostProcessingAttribs, sizeof(m_PostProcessingAttribs));
    }

    // clang-format off
    m_pResMapping->AddResource("g_tex2DLightSpaceDepthMap", m_FrameAttribs.ptex2DShadowMapSRV, false);
    m_pResMapping->AddResource("cbCameraAttribs",           m_FrameAttribs.pcbCameraAttribs, false);
    m_pResMapping->AddResource("cbLightParams",             m_FrameAttribs.pcbLightAttribs, false);
    // clang-format on
}

void EpipolarLightScattering::PerformPostProcessing()
{
    // Note that pecomputation methods change render targets and pipelines
    // (CreateLowResLuminanceTexture changes render targets). If they are moved to
    // PrepareForNewFrame, an application must be required to restore states afterwards

    if (!(m_uiUpToDateResourceFlags & UpToDateResourceFlags::PrecomputedOpticalDepthTex))
    {
        PrecomputeOpticalDepthTexture(m_FrameAttribs.pDevice, m_FrameAttribs.pDeviceContext);
    }

    if ((m_PostProcessingAttribs.iMultipleScatteringMode > MULTIPLE_SCTR_MODE_NONE ||
         m_PostProcessingAttribs.iSingleScatteringMode == SINGLE_SCTR_MODE_LUT) &&
        !(m_uiUpToDateResourceFlags & UpToDateResourceFlags::PrecomputedIntegralsTex))
    {
        PrecomputeScatteringLUT(m_FrameAttribs.pDevice, m_FrameAttribs.pDeviceContext);
    }

    if (/*m_PostProcessingAttribs.ToneMapping.bAutoExposure &&*/ !m_ptex2DLowResLuminanceRTV)
    {
        CreateLowResLuminanceTexture(m_FrameAttribs.pDevice, m_FrameAttribs.pDeviceContext);
    }


    ReconstructCameraSpaceZ();

    if (m_PostProcessingAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING)
    {
        RenderSliceEndpoints();

        // Render coordinate texture and camera space z for epipolar location
        RenderCoordinateTexture();

        if (m_PostProcessingAttribs.iRefinementCriterion == REFINEMENT_CRITERION_INSCTR_DIFF ||
            m_PostProcessingAttribs.iExtinctionEvalMode == EXTINCTION_EVAL_MODE_EPIPOLAR)
        {
            RenderCoarseUnshadowedInctr();
        }

        // Refine initial ray marching samples
        RefineSampleLocations();

        // Mark all ray marching samples in stencil
        MarkRayMarchingSamples();

        if (m_PostProcessingAttribs.bEnableLightShafts && m_PostProcessingAttribs.bUse1DMinMaxTree)
        {
            RenderSliceUVDirAndOrig();
        }

        ITextureView* ppRTVs[] = {m_ptex2DInitialScatteredLightRTV};
        m_FrameAttribs.pDeviceContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        const float Zero[] = {0, 0, 0, 0};
        m_FrameAttribs.pDeviceContext->ClearRenderTarget(m_ptex2DInitialScatteredLightRTV, Zero, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        int iLastCascade = (m_PostProcessingAttribs.bEnableLightShafts && m_PostProcessingAttribs.iCascadeProcessingMode == CASCADE_PROCESSING_MODE_MULTI_PASS) ? m_PostProcessingAttribs.iNumCascades - 1 : m_PostProcessingAttribs.iFirstCascadeToRayMarch;
        for (int iCascadeInd = m_PostProcessingAttribs.iFirstCascadeToRayMarch; iCascadeInd <= iLastCascade; ++iCascadeInd)
        {
            // Build min/max mip map
            if (m_PostProcessingAttribs.bEnableLightShafts && m_PostProcessingAttribs.bUse1DMinMaxTree)
            {
                Build1DMinMaxMipMap(iCascadeInd);
            }
            // Perform ray marching for selected samples
            DoRayMarching(m_PostProcessingAttribs.uiMaxSamplesOnTheRay, iCascadeInd);
        }

        // Interpolate ray marching samples onto the rest of samples
        InterpolateInsctrIrradiance();

        if (m_PostProcessingAttribs.ToneMapping.bAutoExposure)
        {
            // Render scene luminance to low-resolution texture
            ITextureView* pRTVs[] = {m_ptex2DLowResLuminanceRTV};
            m_FrameAttribs.pDeviceContext->SetRenderTargets(_countof(pRTVs), pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            UnwarpEpipolarScattering(true);
            m_FrameAttribs.pDeviceContext->GenerateMips(m_ptex2DLowResLuminanceSRV);

            UpdateAverageLuminance();
        }
        // Set the main back & depth buffers
        m_FrameAttribs.pDeviceContext->SetRenderTargets(1, &m_FrameAttribs.ptex2DDstColorBufferRTV, m_FrameAttribs.ptex2DDstDepthBufferDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Clear depth to 1.0.
        m_FrameAttribs.pDeviceContext->ClearDepthStencil(m_FrameAttribs.ptex2DDstDepthBufferDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        // Transform inscattering irradiance from epipolar coordinates back to rectangular
        // The shader will write 0.0 to the depth buffer, but all pixel that require inscattering
        // correction will be discarded and will keep 1.0
        UnwarpEpipolarScattering(false);

        // Correct inscattering for pixels, for which no suitable interpolation sources were found
        if (m_PostProcessingAttribs.bCorrectScatteringAtDepthBreaks)
        {
            FixInscatteringAtDepthBreaks(m_PostProcessingAttribs.uiNumSamplesOnTheRayAtDepthBreak, EFixInscatteringMode::FixInscattering);
        }

        if (m_PostProcessingAttribs.bShowSampling)
        {
            RenderSampleLocations();
        }
    }
    else if (m_PostProcessingAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_BRUTE_FORCE)
    {
        if (m_PostProcessingAttribs.ToneMapping.bAutoExposure)
        {
            // Render scene luminance to low-resolution texture
            ITextureView* pRTVs[] = {m_ptex2DLowResLuminanceRTV};
            m_FrameAttribs.pDeviceContext->SetRenderTargets(_countof(pRTVs), pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            FixInscatteringAtDepthBreaks(m_PostProcessingAttribs.uiMaxSamplesOnTheRay, EFixInscatteringMode::LuminanceOnly);
            m_FrameAttribs.pDeviceContext->GenerateMips(m_ptex2DLowResLuminanceSRV);

            UpdateAverageLuminance();
        }

        // Set the main back & depth buffers
        m_FrameAttribs.pDeviceContext->SetRenderTargets(1, &m_FrameAttribs.ptex2DDstColorBufferRTV, m_FrameAttribs.ptex2DDstDepthBufferDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        FixInscatteringAtDepthBreaks(m_PostProcessingAttribs.uiMaxSamplesOnTheRay, EFixInscatteringMode::FullScreenRayMarching);
    }
}


void EpipolarLightScattering::CreateMinMaxShadowMap(IRenderDevice* pDevice)
{
    TextureDesc MinMaxShadowMapTexDesc;
    MinMaxShadowMapTexDesc.Type      = RESOURCE_DIM_TEX_2D;
    MinMaxShadowMapTexDesc.Width     = m_PostProcessingAttribs.uiMinMaxShadowMapResolution;
    MinMaxShadowMapTexDesc.Height    = m_PostProcessingAttribs.uiNumEpipolarSlices;
    MinMaxShadowMapTexDesc.MipLevels = 1;
    MinMaxShadowMapTexDesc.Format    = m_PostProcessingAttribs.bIs32BitMinMaxMipMap ? TEX_FORMAT_RG32_FLOAT : TEX_FORMAT_RG16_UNORM;
    MinMaxShadowMapTexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;

    if (m_bUseCombinedMinMaxTexture)
    {
        MinMaxShadowMapTexDesc.Height *= (m_PostProcessingAttribs.iNumCascades - m_PostProcessingAttribs.iFirstCascadeToRayMarch);
    }

    for (int i = 0; i < 2; ++i)
    {
        std::string name = "MinMaxShadowMap";
        name.push_back('0' + char(i));
        MinMaxShadowMapTexDesc.Name = name.c_str();
        m_ptex2DMinMaxShadowMapSRV[i].Release();
        m_ptex2DMinMaxShadowMapRTV[i].Release();
        RefCntAutoPtr<ITexture> ptex2DMinMaxShadowMap;
        // Create 2-D texture, shader resource and target view buffers on the device
        pDevice->CreateTexture(MinMaxShadowMapTexDesc, nullptr, &ptex2DMinMaxShadowMap);
        m_ptex2DMinMaxShadowMapSRV[i] = ptex2DMinMaxShadowMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_ptex2DMinMaxShadowMapSRV[i]->SetSampler(m_pLinearClampSampler);
        m_ptex2DMinMaxShadowMapRTV[i] = ptex2DMinMaxShadowMap->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);

        m_pResMapping->AddResource("g_tex2DMinMaxLightSpaceDepth", m_ptex2DMinMaxShadowMapSRV[0], false);
    }
}



float2 exp(const float2& fX) { return float2(::exp(fX.x), ::exp(fX.y)); }
float3 exp(const float3& fX) { return float3(::exp(fX.x), ::exp(fX.y), ::exp(fX.z)); }

// fCosChi = Pi/2
float2 ChapmanOrtho(const float2& f2x)
{
    static const float fConst  = sqrt(PI_F / 2.f);
    float2             f2SqrtX = float2(sqrt(f2x.x), sqrt(f2x.y));
    return fConst * (float2(1.f, 1.f) / (2.f * f2SqrtX) + f2SqrtX);
}

// |fCosChi| < Pi/2
float2 f2ChapmanRising(const float2& f2X, float fCosChi)
{
    float2 f2ChOrtho = ChapmanOrtho(f2X);
    return f2ChOrtho / ((f2ChOrtho - float2(1, 1)) * fCosChi + float2(1, 1));
}

float2 GetDensityIntegralFromChapmanFunc(float                       fHeightAboveSurface,
                                         const float3&               f3EarthCentreToPointDir,
                                         const float3&               f3RayDir,
                                         const AirScatteringAttribs& SctrMediaAttribs)
{
    // Note: there is no intersection test with the Earth. However,
    // optical depth through the Earth is large, which effectively
    // occludes the light
    float  fCosChi                  = dot(f3EarthCentreToPointDir, f3RayDir);
    auto   f2ParticleScaleHeight    = float2(SctrMediaAttribs.f4ParticleScaleHeight.x, SctrMediaAttribs.f4ParticleScaleHeight.y);
    auto   f2ParticleScaleHeightInv = float2(SctrMediaAttribs.f4ParticleScaleHeight.z, SctrMediaAttribs.f4ParticleScaleHeight.w);
    float2 f2x                      = (fHeightAboveSurface + SctrMediaAttribs.fEarthRadius) * f2ParticleScaleHeightInv;
    float2 f2VerticalAirMass        = f2ParticleScaleHeight * exp(-float2(fHeightAboveSurface, fHeightAboveSurface) * f2ParticleScaleHeightInv);
    if (fCosChi >= 0.f)
    {
        return f2VerticalAirMass * f2ChapmanRising(f2x, fCosChi);
    }
    else
    {
        float  fSinChi            = sqrt(1.f - fCosChi * fCosChi);
        float  fh0                = (fHeightAboveSurface + SctrMediaAttribs.fEarthRadius) * fSinChi - SctrMediaAttribs.fEarthRadius;
        float2 f2VerticalAirMass0 = f2ParticleScaleHeight * exp(-float2(fh0, fh0) * f2ParticleScaleHeightInv);
        float2 f2x0               = float2(fh0 + SctrMediaAttribs.fEarthRadius, fh0 + SctrMediaAttribs.fEarthRadius) * f2ParticleScaleHeightInv;
        float2 f2ChOrtho_x0       = ChapmanOrtho(f2x0);
        float2 f2Ch               = f2ChapmanRising(f2x, -fCosChi);
        return f2VerticalAirMass0 * (2.f * f2ChOrtho_x0) - f2VerticalAirMass * f2Ch;
    }
}

void EpipolarLightScattering::ComputeSunColor(const float3& vDirectionOnSun,
                                              const float4& f4ExtraterrestrialSunColor,
                                              float4&       f4SunColorAtGround,
                                              float4&       f4AmbientLight)
{

    // Compute the ambient light values
    float zenithFactor = std::min(std::max(vDirectionOnSun.y, 0.0f), 1.0f);
    f4AmbientLight.x   = zenithFactor * 0.15f;
    f4AmbientLight.y   = zenithFactor * 0.1f;
    f4AmbientLight.z   = std::max(0.005f, zenithFactor * 0.25f);
    f4AmbientLight.w   = 0.0f;

    float2 f2NetParticleDensityToAtmTop = GetDensityIntegralFromChapmanFunc(0, float3(0, 1, 0), vDirectionOnSun, m_MediaParams);


    float3      f3RlghExtCoeff     = std::max((float3&)m_MediaParams.f4RayleighExtinctionCoeff, float3(1e-8f, 1e-8f, 1e-8f));
    float3      f3RlghOpticalDepth = f3RlghExtCoeff * f2NetParticleDensityToAtmTop.x;
    float3      f3MieExtCoeff      = std::max((float3&)m_MediaParams.f4MieExtinctionCoeff, float3(1e-8f, 1e-8f, 1e-8f));
    float3      f3MieOpticalDepth  = f3MieExtCoeff * f2NetParticleDensityToAtmTop.y;
    float3      f3TotalExtinction  = exp(-(f3RlghOpticalDepth + f3MieOpticalDepth));
    const float fEarthReflectance  = 0.1f; // See [BN08]
    (float3&)f4SunColorAtGround    = ((float3&)f4ExtraterrestrialSunColor) * f3TotalExtinction * fEarthReflectance;
}

void EpipolarLightScattering::ComputeScatteringCoefficients(IDeviceContext* pDeviceCtx)
{
    // For details, see "A practical Analytic Model for Daylight" by Preetham & Hoffman, p.23

    // Wave lengths
    // [BN08] follows [REK04] and gives the following values for Rayleigh scattering coefficients:
    // RayleighBetha(lambda = (680nm, 550nm, 440nm) ) = (5.8, 13.5, 33.1)e-6
    static const double dWaveLengths[] =
        {
            680e-9, // red
            550e-9, // green
            440e-9  // blue
        };

    // Calculate angular and total scattering coefficients for Rayleigh scattering:
    {
        float4& f4AngularRayleighSctrCoeff = m_MediaParams.f4AngularRayleighSctrCoeff;
        float4& f4TotalRayleighSctrCoeff   = m_MediaParams.f4TotalRayleighSctrCoeff;
        float4& f4RayleighExtinctionCoeff  = m_MediaParams.f4RayleighExtinctionCoeff;

        constexpr double n  = 1.0003;    // - Refractive index of air in the visible spectrum
        constexpr double N  = 2.545e+25; // - Number of molecules per unit volume
        constexpr double Pn = 0.035;     // - Depolarization factor for air which exoresses corrections
                                         //   due to anisotropy of air molecules

        constexpr double dRayleighConst = 8.0 * PI * PI * PI * (n * n - 1.0) * (n * n - 1.0) / (3.0 * N) * (6.0 + 3.0 * Pn) / (6.0 - 7.0 * Pn);
        for (int WaveNum = 0; WaveNum < 3; WaveNum++)
        {
            double dSctrCoeff;
            if (m_PostProcessingAttribs.bUseCustomSctrCoeffs)
                dSctrCoeff = f4TotalRayleighSctrCoeff[WaveNum] = m_PostProcessingAttribs.f4CustomRlghBeta[WaveNum];
            else
            {
                double Lambda2 = dWaveLengths[WaveNum] * dWaveLengths[WaveNum];
                double Lambda4 = Lambda2 * Lambda2;
                dSctrCoeff     = dRayleighConst / Lambda4;
                // Total Rayleigh scattering coefficient is the integral of angular scattering coefficient in all directions
                f4TotalRayleighSctrCoeff[WaveNum] = static_cast<float>(dSctrCoeff);
            }
            // Angular scattering coefficient is essentially volumetric scattering coefficient multiplied by the
            // normalized phase function
            // p(Theta) = 3/(16*Pi) * (1 + cos^2(Theta))
            // f4AngularRayleighSctrCoeff contains all the terms exepting 1 + cos^2(Theta):
            f4AngularRayleighSctrCoeff[WaveNum] = static_cast<float>(3.0 / (16.0 * PI) * dSctrCoeff);
            // f4AngularRayleighSctrCoeff[WaveNum] = f4TotalRayleighSctrCoeff[WaveNum] * p(Theta)
        }
        // Air molecules do not absorb light, so extinction coefficient is only caused by out-scattering
        f4RayleighExtinctionCoeff = f4TotalRayleighSctrCoeff;
    }

    if (m_PostProcessingAttribs.bUseOzoneApproximation)
    {
        // As noted in [1], taking into account ozone particle absorption is essential to reproduce
        // the blue of the zenith sky. Without ozone, the sky can appear too yellow overall, especially
        // at sunset/sunrise. Ozone should be concentrated 32km up in the sky, however the author suggests
        // to use the same distribution as the Rayleigh particle distribution, which allows simply adding
        // ozone absorption to Rayleigh extinction.
        //
        // 1. Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite, Sebastien Hillaire
        //    (Physically-Based Shading in Theory and Practice, Siggraph 2016)

        if (m_PostProcessingAttribs.bUseCustomSctrCoeffs)
        {
            m_MediaParams.f4RayleighExtinctionCoeff += m_PostProcessingAttribs.f4CustomOzoneAbsorption;
        }
        else
        {
            // Add Ozone absorption coefficients from [2] to the total Rayleigh extinction coefficients
            //
            //  2. A Scalable and Production Ready Sky and Atmosphere Rendering Technique, Sebastien Hillaire,
            //     Eurographics Symposium on Rendering 2020

            const float4 f4OzoneAbsorption = float4{0.650f, 1.881f, 0.085f, 0.f} * 1e-6f;
            m_MediaParams.f4RayleighExtinctionCoeff += f4OzoneAbsorption;
        }
    }

    // Calculate angular and total scattering coefficients for Mie scattering:
    {
        float4& f4AngularMieSctrCoeff = m_MediaParams.f4AngularMieSctrCoeff;
        float4& f4TotalMieSctrCoeff   = m_MediaParams.f4TotalMieSctrCoeff;
        float4& f4MieExtinctionCoeff  = m_MediaParams.f4MieExtinctionCoeff;

        if (m_PostProcessingAttribs.bUseCustomSctrCoeffs)
        {
            f4TotalMieSctrCoeff = m_PostProcessingAttribs.f4CustomMieBeta * m_PostProcessingAttribs.fAerosolDensityScale;
        }
        else
        {
            const bool bUsePreethamMethod = false;
            if (bUsePreethamMethod)
            {
                // Values for K came from the table 2 in the "A practical Analytic Model
                // for Daylight" by Preetham & Hoffman, p.28
                constexpr double K[] =
                    {
                        0.68455,                    //  K[650nm]
                        0.678781,                   //  K[570nm]
                        (0.668532 + 0.669765) / 2.0 // (K[470nm]+K[480nm])/2
                    };

                VERIFY_EXPR(m_MediaParams.fTurbidity >= 1.f);

                // Beta is an Angstrom's turbidity coefficient and is approximated by:
                //float beta = 0.04608365822050f * m_fTurbidity - 0.04586025928522f; ???????

                const double     c = (0.6544 * m_MediaParams.fTurbidity - 0.6510) * 1E-16; // concentration factor
                constexpr double v = 4;                                                    // Junge's exponent

                const double dTotalMieBetaTerm = 0.434 * c * PI * pow(2.0 * PI, v - 2);

                for (int WaveNum = 0; WaveNum < 3; WaveNum++)
                {
                    double Lambdav_minus_2       = pow(dWaveLengths[WaveNum], v - 2);
                    double dTotalMieSctrCoeff    = dTotalMieBetaTerm * K[WaveNum] / Lambdav_minus_2;
                    f4TotalMieSctrCoeff[WaveNum] = static_cast<float>(dTotalMieSctrCoeff);
                }

                //AtmScatteringAttribs.f4AngularMieSctrCoeff *= 0.02f;
                //AtmScatteringAttribs.f4TotalMieSctrCoeff *= 0.02f;
            }
            else
            {
                // [BN08] uses the following value (independent of wavelength) for Mie scattering coefficient: 2e-5
                // For g=0.76 and MieBetha=2e-5 [BN08] was able to reproduce the same luminance as given by the
                // reference CIE sky light model
                const float fMieBethaBN08         = 2e-5f * m_PostProcessingAttribs.fAerosolDensityScale;
                m_MediaParams.f4TotalMieSctrCoeff = float4(fMieBethaBN08, fMieBethaBN08, fMieBethaBN08, 0);
            }
        }

        for (int WaveNum = 0; WaveNum < 3; WaveNum++)
        {
            // Normalized to unity Cornette-Shanks phase function has the following form:
            // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)
            // The angular scattering coefficient is the volumetric scattering coefficient multiplied by the phase
            // function. 1/(4*PI) is baked into the f4AngularMieSctrCoeff, the other terms are baked into f4CS_g
            f4AngularMieSctrCoeff[WaveNum] = f4TotalMieSctrCoeff[WaveNum] / (4.f * PI_F);
            // [BN08] also uses slight absorption factor which is 10% of scattering
            f4MieExtinctionCoeff[WaveNum] = f4TotalMieSctrCoeff[WaveNum] * (1.f + m_PostProcessingAttribs.fAerosolAbsorbtionScale);
        }
    }

    {
        // For g=0.76 and MieBetha=2e-5 [BN08] was able to reproduce the same luminance as is given by the
        // reference CIE sky light model
        // Cornette phase function (see Nishita et al. 93):
        // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)
        // 1/(4*PI) is baked into the f4AngularMieSctrCoeff
        float4& f4CS_g = m_MediaParams.f4CS_g;
        float   f_g    = m_MediaParams.fAerosolPhaseFuncG;
        f4CS_g.x       = 3 * (1.f - f_g * f_g) / (2 * (2.f + f_g * f_g));
        f4CS_g.y       = 1.f + f_g * f_g;
        f4CS_g.z       = -2.f * f_g;
        f4CS_g.w       = 1.f;
    }

    m_MediaParams.f4TotalExtinctionCoeff = m_MediaParams.f4RayleighExtinctionCoeff + m_MediaParams.f4MieExtinctionCoeff;

    if (pDeviceCtx && m_pcbMediaAttribs)
    {
        pDeviceCtx->UpdateBuffer(m_pcbMediaAttribs, 0, sizeof(m_MediaParams), &m_MediaParams, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
}


void EpipolarLightScattering::RenderSun(TEXTURE_FORMAT RTVFormat,
                                        TEXTURE_FORMAT DSVFormat,
                                        Uint8          SampleCount)
{
    if (m_PostProcessingAttribs.f4LightScreenPos.w <= 0)
        return;

    auto& RenderSunTech = m_RenderTech[RENDER_TECH_RENDER_SUN];
    if (RenderSunTech.PSO)
    {
        const auto& GraphicsPipeline = RenderSunTech.PSO->GetGraphicsPipelineDesc();
        if (GraphicsPipeline.RTVFormats[0] != RTVFormat ||
            GraphicsPipeline.DSVFormat != DSVFormat ||
            GraphicsPipeline.SmplDesc.Count != SampleCount)
        {
            RenderSunTech.PSO.Release();
            RenderSunTech.SRB.Release();
        }
    }

    if (!RenderSunTech.PSO)
    {
        RefCntAutoPtr<IShader> pSunVS = CreateShader(m_FrameAttribs.pDevice, "Sun.fx", "SunVS", SHADER_TYPE_VERTEX);
        RefCntAutoPtr<IShader> pSunPS = CreateShader(m_FrameAttribs.pDevice, "Sun.fx", "SunPS", SHADER_TYPE_PIXEL);

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        // clang-format off
        ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "cbPostProcessingAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
        };
        // clang-format on
        PSODesc.ResourceLayout.Variables    = Vars;
        PSODesc.ResourceLayout.NumVariables = _countof(Vars);

        PSODesc.Name                                          = "Render Sun";
        auto& GraphicsPipeline                                = PSOCreateInfo.GraphicsPipeline;
        GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
        GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_NONE;
        GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = true;
        GraphicsPipeline.DepthStencilDesc                     = DSS_CmpEqNoWrites;
        PSOCreateInfo.pVS                                     = pSunVS;
        PSOCreateInfo.pPS                                     = pSunPS;
        GraphicsPipeline.NumRenderTargets                     = 1;
        GraphicsPipeline.RTVFormats[0]                        = RTVFormat;
        GraphicsPipeline.DSVFormat                            = DSVFormat;
        GraphicsPipeline.SmplDesc.Count                       = SampleCount;
        GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        m_FrameAttribs.pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &RenderSunTech.PSO);
        RenderSunTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);

        RenderSunTech.SRBDependencyFlags = SRB_DEPENDENCY_CAMERA_ATTRIBS;
    }

    if (!RenderSunTech.SRB)
    {
        RenderSunTech.PSO->CreateShaderResourceBinding(&RenderSunTech.SRB, true);
        RenderSunTech.SRB->BindResources(SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
    }
    m_FrameAttribs.pDeviceContext->SetPipelineState(RenderSunTech.PSO);
    m_FrameAttribs.pDeviceContext->CommitShaderResources(RenderSunTech.SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 4;
    m_FrameAttribs.pDeviceContext->Draw(DrawAttrs);
}

void EpipolarLightScattering::ComputeAmbientSkyLightTexture(IRenderDevice* pDevice, IDeviceContext* pContext)
{
    if (!(m_uiUpToDateResourceFlags & UpToDateResourceFlags::PrecomputedOpticalDepthTex))
    {
        PrecomputeOpticalDepthTexture(pDevice, pContext);
    }

    if (!(m_uiUpToDateResourceFlags & UpToDateResourceFlags::PrecomputedIntegralsTex))
    {
        PrecomputeScatteringLUT(pDevice, pContext);
    }

    auto& PrecomputeAmbientSkyLightTech = m_RenderTech[RENDER_TECH_PRECOMPUTE_AMBIENT_SKY_LIGHT];
    if (!PrecomputeAmbientSkyLightTech.PSO)
    {
        ShaderMacroHelper Macros;
        Macros.AddShaderMacro("NUM_RANDOM_SPHERE_SAMPLES", static_cast<Int32>(m_uiNumRandomSamplesOnSphere));
        Macros.Finalize();
        auto pPrecomputeAmbientSkyLightPS = CreateShader(pDevice, "PrecomputeAmbientSkyLight.fx", "PrecomputeAmbientSkyLightPS",
                                                         SHADER_TYPE_PIXEL, Macros);

        PipelineResourceLayoutDesc ResourceLayout;
        ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        PrecomputeAmbientSkyLightTech.InitializeFullScreenTriangleTechnique(pDevice, "PrecomputeAmbientSkyLight",
                                                                            m_pFullScreenTriangleVS, pPrecomputeAmbientSkyLightPS,
                                                                            ResourceLayout, AmbientSkyLightTexFmt);
        PrecomputeAmbientSkyLightTech.PSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_pResMapping, BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
    }

    // Create 2-D texture, shader resource and target view buffers on the device
    ITextureView* pRTVs[] = {m_ptex2DAmbientSkyLightRTV};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    PrecomputeAmbientSkyLightTech.PrepareSRB(pDevice, m_pResMapping);
    PrecomputeAmbientSkyLightTech.Render(pContext);
    m_uiUpToDateResourceFlags |= UpToDateResourceFlags::AmbientSkyLightTex;
}


ITextureView* EpipolarLightScattering::GetAmbientSkyLightSRV(IRenderDevice* pDevice, IDeviceContext* pContext)
{
    if (!(m_uiUpToDateResourceFlags & UpToDateResourceFlags::AmbientSkyLightTex))
    {
        ComputeAmbientSkyLightTexture(pDevice, pContext);
    }

    return m_ptex2DAmbientSkyLightSRV;
}

} // namespace Diligent
