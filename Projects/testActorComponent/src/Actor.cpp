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

#include <vector>
#include <stdio.h>

#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "TexturedCube.hpp"
#include "Actor.h"

namespace Diligent
{

Actor::Actor()
{

}

Actor::Actor(const SampleInitInfo& InitInfo)
{
    Initialize(InitInfo);
}

void Actor::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);
}

void Actor::RenderActor(const float4x4& CameraViewProj, bool IsShadowPass)
{
    // Render shadow map
    m_pImmediateContext->SetRenderTargets(0, nullptr, m_ShadowMapDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(m_ShadowMapDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    RenderShadowMap();
}

void Actor::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    UpdateActor(CurrTime, ElapsedTime);
}

void Actor::CreateShadowMap()
{
    TextureDesc SMDesc;
    SMDesc.Name      = "Shadow map";
    SMDesc.Type      = RESOURCE_DIM_TEX_2D;
    SMDesc.Width     = m_ShadowMapSize;
    SMDesc.Height    = m_ShadowMapSize;
    SMDesc.Format    = m_ShadowMapFormat;
    SMDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
    RefCntAutoPtr<ITexture> ShadowMap;
    m_pDevice->CreateTexture(SMDesc, nullptr, &ShadowMap);
    m_ShadowMapSRV = ShadowMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_ShadowMapDSV = ShadowMap->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    // Create SRBs that use shadow map as mutable variable
    m_SRB.Release();
    m_pPSO->CreateShaderResourceBinding(&m_SRB, true);
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(m_ShadowMapSRV);
}

void Actor::RenderShadowMapVis()
{
    m_pImmediateContext->SetPipelineState(m_pShadowMapVisPSO);
    m_pImmediateContext->CommitShaderResources(m_ShadowMapVisSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    DrawAttribs DrawAttrs(4, DRAW_FLAG_VERIFY_ALL);
    m_pImmediateContext->Draw(DrawAttrs);
}

void Actor::CreateShadowMapVisPSO()
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "Shadow Map Vis PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial renders to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    // No cull
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    // Disable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create shadow map visualization vertex shader
    RefCntAutoPtr<IShader> pShadowMapVisVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Shadow Map Vis VS";
        ShaderCI.FilePath        = "shadow_map_vis.vsh";
        m_pDevice->CreateShader(ShaderCI, &pShadowMapVisVS);
    }

    // Create shadow map visualization pixel shader
    RefCntAutoPtr<IShader> pShadowMapVisPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Shadow Map Vis PS";
        ShaderCI.FilePath        = "shadow_map_vis.psh";
        m_pDevice->CreateShader(ShaderCI, &pShadowMapVisPS);
    }

    PSOCreateInfo.pVS = pShadowMapVisVS;
    PSOCreateInfo.pPS = pShadowMapVisPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    // clang-format off
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR,
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_ShadowMap", SamLinearClampDesc}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pShadowMapVisPSO);
}

void Actor::RenderShadowMap()
{
    float3 f3LightSpaceX, f3LightSpaceY, f3LightSpaceZ;
    f3LightSpaceZ = normalize(m_LightDirection);

    auto min_cmp = std::min(std::min(std::abs(m_LightDirection.x), std::abs(m_LightDirection.y)), std::abs(m_LightDirection.z));
    if (min_cmp == std::abs(m_LightDirection.x))
        f3LightSpaceX = float3(1, 0, 0);
    else if (min_cmp == std::abs(m_LightDirection.y))
        f3LightSpaceX = float3(0, 1, 0);
    else
        f3LightSpaceX = float3(0, 0, 1);

    f3LightSpaceY = cross(f3LightSpaceZ, f3LightSpaceX);
    f3LightSpaceX = cross(f3LightSpaceY, f3LightSpaceZ);
    f3LightSpaceX = normalize(f3LightSpaceX);
    f3LightSpaceY = normalize(f3LightSpaceY);

    float4x4 WorldToLightViewSpaceMatr = float4x4::ViewFromBasis(f3LightSpaceX, f3LightSpaceY, f3LightSpaceZ);

    // For this tutorial we know that the scene center is at (0,0,0).
    // Real applications will want to compute tight bounds

    float3 f3SceneCenter = float3(0, 0, 0);
    float  SceneRadius   = std::sqrt(3.f);
    float3 f3MinXYZ      = f3SceneCenter - float3(SceneRadius, SceneRadius, SceneRadius);
    float3 f3MaxXYZ      = f3SceneCenter + float3(SceneRadius, SceneRadius, SceneRadius * 5);
    float3 f3SceneExtent = f3MaxXYZ - f3MinXYZ;

    const auto& DevCaps = m_pDevice->GetDeviceCaps();
    const bool  IsGL    = DevCaps.IsGLDevice();
    float4      f4LightSpaceScale;
    f4LightSpaceScale.x = 2.f / f3SceneExtent.x;
    f4LightSpaceScale.y = 2.f / f3SceneExtent.y;
    f4LightSpaceScale.z = (IsGL ? 2.f : 1.f) / f3SceneExtent.z;
    // Apply bias to shift the extent to [-1,1]x[-1,1]x[0,1] for DX or to [-1,1]x[-1,1]x[-1,1] for GL
    // Find bias such that f3MinXYZ -> (-1,-1,0) for DX or (-1,-1,-1) for GL
    float4 f4LightSpaceScaledBias;
    f4LightSpaceScaledBias.x = -f3MinXYZ.x * f4LightSpaceScale.x - 1.f;
    f4LightSpaceScaledBias.y = -f3MinXYZ.y * f4LightSpaceScale.y - 1.f;
    f4LightSpaceScaledBias.z = -f3MinXYZ.z * f4LightSpaceScale.z + (IsGL ? -1.f : 0.f);

    float4x4 ScaleMatrix      = float4x4::Scale(f4LightSpaceScale.x, f4LightSpaceScale.y, f4LightSpaceScale.z);
    float4x4 ScaledBiasMatrix = float4x4::Translation(f4LightSpaceScaledBias.x, f4LightSpaceScaledBias.y, f4LightSpaceScaledBias.z);

    // Note: bias is applied after scaling!
    float4x4 ShadowProjMatr = ScaleMatrix * ScaledBiasMatrix;

    // Adjust the world to light space transformation matrix
    float4x4 WorldToLightProjSpaceMatr = WorldToLightViewSpaceMatr * ShadowProjMatr;

    const auto& NDCAttribs    = DevCaps.GetNDCAttribs();
    float4x4    ProjToUVScale = float4x4::Scale(0.5f, NDCAttribs.YtoVScale, NDCAttribs.ZtoDepthScale);
    float4x4    ProjToUVBias  = float4x4::Translation(0.5f, 0.5f, NDCAttribs.GetZtoDepthBias());

    m_WorldToShadowMapUVDepthMatr = WorldToLightProjSpaceMatr * ProjToUVScale * ProjToUVBias;
}

} // namespace Diligent
