/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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

#include "GhostCubeScene.h"
#include "BasicMath.h"
#include <algorithm>
#include "BasicShaderSourceStreamFactory.h"
#include "GraphicsUtilities.h"
#include "MapHelper.h"
#include "CommonlyUsedStates.h"

#if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIVERSAL_WINDOWS)
#   define D3D12_SUPPORTED 1
#else
#   define D3D12_SUPPORTED 0
#endif

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
    pDevice->CreateTexture(TexDesc, TextureData(), &m_pRenderTarget);

    TexDesc.Name = "Mirror depth buffer";
    TexDesc.Format = TEX_FORMAT_D32_FLOAT;
    TexDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
    TexDesc.ClearValue.DepthStencil.Depth = 0.f;
    pDevice->CreateTexture(TexDesc, TextureData(), &m_pDepthBuffer);

    //auto deviceType = pDevice->GetDeviceCaps().DevType;
    {
        const auto &SCDesc = m_DiligentGraphics->GetSwapChain()->GetDesc();
        auto UseReverseZ = m_DiligentGraphics->UsesReverseZ();

        PipelineStateDesc PSODesc;
        PSODesc.IsComputePipeline = false;
        PSODesc.Name = "Render sample cube PSO";
        PSODesc.GraphicsPipeline.NumRenderTargets = 1;

        PSODesc.GraphicsPipeline.RTVFormats[0] = SCDesc.ColorBufferFormat == TEX_FORMAT_RGBA8_UNORM ? TEX_FORMAT_RGBA8_UNORM_SRGB : SCDesc.ColorBufferFormat;
        PSODesc.GraphicsPipeline.DSVFormat = SCDesc.DepthBufferFormat;
        PSODesc.GraphicsPipeline.PrimitiveTopologyType = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
        PSODesc.GraphicsPipeline.DepthStencilDesc.DepthFunc = UseReverseZ ? COMPARISON_FUNC_GREATER_EQUAL : COMPARISON_FUNC_LESS_EQUAL;

        ShaderCreationAttribs CreationAttribs;
        BasicShaderSourceStreamFactory BasicSSSFactory("shaders");
        CreationAttribs.pShaderSourceStreamFactory = &BasicSSSFactory;
        CreationAttribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        CreationAttribs.Desc.DefaultVariableType = SHADER_VARIABLE_TYPE_STATIC;

        CreateUniformBuffer(pDevice, sizeof(float4x4), "Mirror VS constants CB", &m_pMirrorVSConstants);

        RefCntAutoPtr<IShader> pVS;
        {
            CreationAttribs.Desc.ShaderType = SHADER_TYPE_VERTEX;
            CreationAttribs.EntryPoint = "main";
            CreationAttribs.Desc.Name = "Mirror VS";
            CreationAttribs.FilePath = "Mirror.vsh";
            pDevice->CreateShader(CreationAttribs, &pVS);
            pVS->GetShaderVariable("Constants")->Set(m_pMirrorVSConstants);
        }

        
        RefCntAutoPtr<IShader> pPS;
        {
            CreationAttribs.Desc.ShaderType = SHADER_TYPE_PIXEL;
            CreationAttribs.EntryPoint = "main";
            CreationAttribs.Desc.Name = "Mirror PS";
            CreationAttribs.FilePath = "Mirror.psh";
            StaticSamplerDesc StaticSamplers[] =
            {
                {"g_tex2Reflection", Sam_Aniso4xClamp}
            };
            CreationAttribs.Desc.StaticSamplers = StaticSamplers;
            CreationAttribs.Desc.NumStaticSamplers = _countof(StaticSamplers);
            pDevice->CreateShader(CreationAttribs, &pPS);
            pPS->GetShaderVariable("g_tex2Reflection")->Set(m_pRenderTarget->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        }

        PSODesc.GraphicsPipeline.pVS = pVS;
        PSODesc.GraphicsPipeline.pPS = pPS;
        pDevice->CreatePipelineState(PSODesc, &m_pMirrorPSO);
    }
#if D3D12_SUPPORTED
    m_pStateTransitionHandler.reset(new GhostCubeSceneResTrsnHelper(*this));
#endif
}

void GhostCubeScene::Render(UnityRenderingEvent RenderEventFunc, double CurrTime, double ElapsedTime)
{
    auto pDevice = m_DiligentGraphics->GetDevice();
    auto pCtx = m_DiligentGraphics->GetContext();
    auto DevType = pDevice->GetDeviceCaps().DevType;
    bool IsDX = DevType == DeviceType::D3D11 || DevType == DeviceType::D3D12;
    auto ReverseZ = m_DiligentGraphics->UsesReverseZ();

    // In OpenGL, render targets must be bound to the pipeline to be cleared
    ITextureView *pRTVs[] = { m_pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) };
    ITextureView *pDSV = m_pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    pCtx->SetRenderTargets(1, pRTVs, pDSV);
    const float ClearColor[] = { 0.f, 0.2f, 0.5f, 1.0f };
    pCtx->ClearRenderTarget(pRTVs[0], ClearColor);
    pCtx->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, ReverseZ ? 0.f : 1.f, 0);

    if (DevType == DeviceType::D3D12)
    {
        // D3D12 context must be flushed so that the commands are submitted before the
        // commands issued by the plugin
        pCtx->Flush();
    }


    // Render ghost cube into the mirror texture
    {
        // Create fake reflection matrix
        float4x4 CubeWorldView = scaleMatrix(1,2,1) * rotationY( -static_cast<float>(CurrTime) * 2.0f) * rotationX(-PI_F*0.3f) * translationMatrix(0.f, 0.0f, 10.0f);
        float NearPlane = 0.3f;
        float FarPlane = 1000.f;
        if (ReverseZ)
            std::swap(NearPlane, FarPlane);
        float aspectRatio = 1.0f;
        float4x4 ReflectionCameraProj = Projection(PI_F / 4.f, aspectRatio, NearPlane, FarPlane, IsDX);
        auto wvp = CubeWorldView * ReflectionCameraProj;
        float fReverseZ = IsDX ? -1.f : +1.f;
        SetMatrixFromUnity(wvp._m00, fReverseZ * wvp._m01, wvp._m02, wvp._m03, 
                           wvp._m10, fReverseZ * wvp._m11, wvp._m12, wvp._m13,
                           wvp._m20, fReverseZ * wvp._m21, wvp._m22, wvp._m23,
                           wvp._m30, fReverseZ * wvp._m31, wvp._m32, wvp._m33);

        SetTexturesFromUnity(m_pRenderTarget->GetNativeHandle(), m_pDepthBuffer->GetNativeHandle());

        // Call the plugin
        RenderEventFunc(0);
    }

    // We need to invalidate the context state since the plugin has used d3d11 context
    pCtx->InvalidateState();
    pCtx->SetRenderTargets(0, nullptr, nullptr);
    pCtx->SetPipelineState(m_pMirrorPSO);
    pCtx->CommitShaderResources(nullptr, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);

    {
        float4x4 MirrorWorldView = scaleMatrix(5,5,5) * rotationX(PI_F*0.6f) * translationMatrix(0.f, -3.0f, 10.0f);
        float NearPlane = 0.3f;
        float FarPlane = 1000.f;
        if (ReverseZ)
            std::swap(NearPlane, FarPlane);
        float AspectRatio = static_cast<float>(m_WindowWidth) / static_cast<float>(std::max(m_WindowHeight, 1));
        float4x4 MainCameraProj = Projection(PI_F / 3.f, AspectRatio, NearPlane, FarPlane, IsDX);
        auto wvp = MirrorWorldView * MainCameraProj;
        MapHelper<float4x4> CBConstants(pCtx, m_pMirrorVSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = transposeMatrix(wvp);
    }

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 4;
    DrawAttrs.Topology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pCtx->Draw(DrawAttrs);
}
