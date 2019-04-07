/*     Copyright 2015-2019 Egor Yusov
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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "TestTessellation.h"
#include "MapHelper.h"

using namespace Diligent;

void TestTessellation::Init( IRenderDevice *pDevice, IDeviceContext *pDeviceContext, ISwapChain *pSwapChain)
{
    if(!pDevice->GetDeviceCaps().bTessellationSupported)
    {
        SetStatus(TestResult::Skipped, "Tessellation is not supported");
        return;
    }
    
    m_pDeviceContext = pDeviceContext;

    ShaderCreateInfo CreationAttrs;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("Shaders", &pShaderSourceFactory);
    CreationAttrs.pShaderSourceStreamFactory = pShaderSourceFactory;
    CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    CreationAttrs.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<Diligent::IShader> pVS, pHS, pDS, pPS;
    {
        CreationAttrs.FilePath = "Shaders\\TessTestQuadDX.vsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        pDevice->CreateShader( CreationAttrs, &pVS );
        VERIFY_EXPR(pVS);
    }

    {
        CreationAttrs.FilePath = "Shaders\\TessTestQuadDX.hsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_HULL;
        pDevice->CreateShader( CreationAttrs, &pHS );
        VERIFY_EXPR(pHS);
    }

    {
        CreationAttrs.FilePath = "Shaders\\TessTestQuadDX.dsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_DOMAIN;
        pDevice->CreateShader( CreationAttrs, &pDS );
        VERIFY_EXPR(pDS);
    }

    {
        CreationAttrs.FilePath = "Shaders\\GSTestDX.psh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        pDevice->CreateShader( CreationAttrs, &pPS );
        VERIFY_EXPR(pPS);
    }

    PipelineStateDesc PSODesc;
    PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    PSODesc.GraphicsPipeline.RasterizerDesc.FillMode = FILL_MODE_WIREFRAME;
    PSODesc.GraphicsPipeline.BlendDesc.IndependentBlendEnable = False;
    PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = False;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pHS = pHS;
    PSODesc.GraphicsPipeline.pDS = pDS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    
    pDevice->CreatePipelineState( PSODesc, &m_pQuadPSO );


    {
        pVS.Release();
        CreationAttrs.FilePath = "Shaders\\TessTestTriDX.vsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        pDevice->CreateShader( CreationAttrs, &pVS );
        VERIFY_EXPR(pVS);
    }

    {
        pHS.Release();
        CreationAttrs.FilePath = "Shaders\\TessTestTriDX.hsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_HULL;
        pDevice->CreateShader( CreationAttrs, &pHS );
        VERIFY_EXPR(pHS);
    }

    {
        pDS.Release();
        CreationAttrs.FilePath = "Shaders\\TessTestTriDX.dsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_DOMAIN;
        pDevice->CreateShader( CreationAttrs, &pDS );
        VERIFY_EXPR(pDS);
    }
 
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pHS = pHS;
    PSODesc.GraphicsPipeline.pDS = pDS;

    pDevice->CreatePipelineState( PSODesc, &m_pTriPSO );
}
    
void TestTessellation::Draw()
{
    if(!m_pDeviceContext)
        return;

    m_pDeviceContext->SetPipelineState(m_pQuadPSO);
    m_pDeviceContext->CommitShaderResources(nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    // Draw 2 quad patches
    Diligent::DrawAttribs DrawAttrs(2, DRAW_FLAG_VERIFY_ALL);
    m_pDeviceContext->Draw(DrawAttrs);

    m_pDeviceContext->SetPipelineState(m_pTriPSO);
    m_pDeviceContext->CommitShaderResources(nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    DrawAttrs.NumVertices = 1; // Draw 1 tri patch
    m_pDeviceContext->Draw(DrawAttrs);
    
    SetStatus(TestResult::Succeeded);
}
