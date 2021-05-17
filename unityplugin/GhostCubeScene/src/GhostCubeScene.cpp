/*
 *  Copyright 2019-2021 Diligent Graphics LLC
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

#include "GhostCubeScene.h"
#include "PlatformDefinitions.h"
#include "BasicMath.hpp"
#include "GraphicsUtilities.h"
#include "MapHelper.hpp"
#include "CommonlyUsedStates.h"

#if D3D12_SUPPORTED
#   include "GhostCubeSceneResTrsnHelper.h"
#endif

using namespace Diligent;

UnitySceneBase* CreateScene()
{
    return new GhostCubeScene;
}


void GhostCubeScene::OnPluginLoad(TLoadPluginFunction LoadPluginFunctionCallback)
{
    SetMatrixFromUnity = reinterpret_cast<TSetMatrixFromUnity>( LoadPluginFunctionCallback("SetMatrixFromUnity") );
    VERIFY_EXPR(SetMatrixFromUnity != nullptr);

    SetTexturesFromUnity = reinterpret_cast<TSetTexturesFromUnity>( LoadPluginFunctionCallback("SetTexturesFromUnity") );
    VERIFY_EXPR(SetTexturesFromUnity != nullptr);
}

void GhostCubeScene::OnPluginUnload()
{
}

void GhostCubeScene::OnGraphicsInitialized()
{
    auto pDevice = m_DiligentGraphics->GetDevice();
    TextureDesc TexDesc;
    TexDesc.Name = "Mirror render target";
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = 1024;
    TexDesc.Height = 1024;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    TexDesc.ClearValue.Color[0] = 0.f;
    TexDesc.ClearValue.Color[1] = 0.2f;
    TexDesc.ClearValue.Color[2] = 0.5f;
    TexDesc.ClearValue.Color[3] = 1.0f;
    pDevice->CreateTexture(TexDesc, nullptr, &m_pRenderTarget);

    TexDesc.Name = "Mirror depth buffer";
    TexDesc.Format = TEX_FORMAT_D32_FLOAT;
    TexDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
    TexDesc.ClearValue.DepthStencil.Depth = 0.f;
    pDevice->CreateTexture(TexDesc, nullptr, &m_pDepthBuffer);

    //auto deviceType = pDevice->GetDeviceCaps().DevType;
    {
        const auto &SCDesc = m_DiligentGraphics->GetSwapChain()->GetDesc();
        auto UseReverseZ = m_DiligentGraphics->UsesReverseZ();

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&      PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&   GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
        PSODesc.Name = "Mirror PSO";
        GraphicsPipeline.NumRenderTargets = 1;

        GraphicsPipeline.RTVFormats[0] = SCDesc.ColorBufferFormat;
        GraphicsPipeline.DSVFormat = SCDesc.DepthBufferFormat;
        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
        GraphicsPipeline.DepthStencilDesc.DepthFunc = UseReverseZ ? COMPARISON_FUNC_GREATER_EQUAL : COMPARISON_FUNC_LESS_EQUAL;

        ShaderCreateInfo ShaderCI;
        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders", &pShaderSourceFactory);
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = true;

        CreateUniformBuffer(pDevice, sizeof(float4x4), "Mirror VS constants CB", &m_pMirrorVSConstants);

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Mirror VS";
            ShaderCI.FilePath = "Mirror.vsh";
            pDevice->CreateShader(ShaderCI, &pVS);
        }

        
        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Mirror PS";
            ShaderCI.FilePath = "Mirror.psh";
            pDevice->CreateShader(ShaderCI, &pPS);
        }

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        ImmutableSamplerDesc ImtblSamplers[] =
        {
            {SHADER_TYPE_PIXEL, "g_tex2Reflection", Sam_Aniso4xClamp}
        };
        PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
        PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pMirrorPSO);
        m_pMirrorPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_pMirrorVSConstants);
        m_pMirrorPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2Reflection")->Set(m_pRenderTarget->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_pMirrorPSO->CreateShaderResourceBinding(&m_pMirrorSRB, true);
    }
#if D3D12_SUPPORTED
    m_pStateTransitionHandler.reset(new GhostCubeSceneResTrsnHelper(*this));
#endif
}

void GhostCubeScene::Update(double CurrTime, double ElapsedTime)
{
    m_CubeWorldView = float4x4::Scale(1, 2, 1) * float4x4::RotationY(static_cast<float>(CurrTime) * 2.0f) * float4x4::RotationX(PI_F * 0.3f) * float4x4::Translation(0.f, 0.0f, 10.0f);
}


void GhostCubeScene::Render(UnityRenderingEvent RenderEventFunc)
{
    auto* pDevice = m_DiligentGraphics->GetDevice();
    auto* pCtx = m_DiligentGraphics->GetContext();
    auto* pSwapChain = m_DiligentGraphics->GetSwapChain();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();
    const bool bIsGL = DeviceInfo.IsGLDevice();
    auto ReverseZ = m_DiligentGraphics->UsesReverseZ();

    // In OpenGL, render targets must be bound to the pipeline to be cleared
    ITextureView *pRTV = m_pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    ITextureView *pDSV = m_pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    pCtx->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    const float ClearColor[] = { 0.f, 0.2f, 0.5f, 1.0f };
    pCtx->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pCtx->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, ReverseZ ? 0.f : 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    if (DeviceInfo.Type == RENDER_DEVICE_TYPE_D3D12)
    {
        // D3D12 context must be flushed so that the commands are submitted before the
        // commands issued by the plugin
        pCtx->Flush();
    }


    // Render ghost cube into the mirror texture
    {
        // Create fake reflection matrix
        float NearPlane = 0.3f;
        float FarPlane = 1000.f;
        if (ReverseZ)
            std::swap(NearPlane, FarPlane);
        float aspectRatio = 1.0f;
        float4x4 ReflectionCameraProj = float4x4::Projection(PI_F / 4.f, aspectRatio, NearPlane, FarPlane, bIsGL);
        auto wvp = m_CubeWorldView * ReflectionCameraProj;
        float fReverseZ = bIsGL ? +1.f : -1.f;
        SetMatrixFromUnity(wvp.m00, fReverseZ * wvp.m01, wvp.m02, wvp.m03, 
                           wvp.m10, fReverseZ * wvp.m11, wvp.m12, wvp.m13,
                           wvp.m20, fReverseZ * wvp.m21, wvp.m22, wvp.m23,
                           wvp.m30, fReverseZ * wvp.m31, wvp.m32, wvp.m33);

        SetTexturesFromUnity(m_pRenderTarget->GetNativeHandle(), m_pDepthBuffer->GetNativeHandle());

        // Call the plugin
        RenderEventFunc(0);
    }
    
    // We need to invalidate the context state since the plugin has used d3d11 context
    pCtx->InvalidateState();
    pRTV = pSwapChain->GetCurrentBackBufferRTV();
    pDSV = pSwapChain->GetDepthBufferDSV();
    pCtx->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pCtx->SetPipelineState(m_pMirrorPSO);
    pCtx->CommitShaderResources(m_pMirrorSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        float4x4 MirrorWorldView = float4x4::Scale(5,5,5) * float4x4::RotationX(-PI_F*0.6f) * float4x4::Translation(0.f, -3.0f, 10.0f);
        float NearPlane = 0.3f;
        float FarPlane = 1000.f;
        if (ReverseZ)
            std::swap(NearPlane, FarPlane);
        float AspectRatio = static_cast<float>(m_WindowWidth) / static_cast<float>(std::max(m_WindowHeight, 1));
        float4x4 MainCameraProj = float4x4::Projection(PI_F / 3.f, AspectRatio, NearPlane, FarPlane, bIsGL);
        auto wvp = MirrorWorldView * MainCameraProj;
        MapHelper<float4x4> CBConstants(pCtx, m_pMirrorVSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = wvp.Transpose();
    }

    DrawAttribs DrawAttrs(4, DRAW_FLAG_VERIFY_ALL);
    pCtx->Draw(DrawAttrs);
}
