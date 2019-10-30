/*     Copyright 2019 Diligent Graphics LLC
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
#include "TestGeometryShader.h"
#include "MapHelper.h"

using namespace Diligent;

void TestGeometryShader::Init( IRenderDevice *pDevice, IDeviceContext *pDeviceContext, ISwapChain *pSwapChain)
{
    if(!pDevice->GetDeviceCaps().bGeometryShadersSupported)
    {
        SetStatus(TestResult::Skipped, "Geometry shaders are not supported");
        return;
    }

    m_pDeviceContext = pDeviceContext;

    ShaderCreateInfo CreationAttrs;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    CreationAttrs.pShaderSourceStreamFactory = pShaderSourceFactory;
    CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    CreationAttrs.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<Diligent::IShader> pVS, pGS, pPS;
    {
        CreationAttrs.FilePath = "Shaders\\GSTestDX.vsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        pDevice->CreateShader( CreationAttrs, &pVS );
    }

    {
        CreationAttrs.FilePath = "Shaders\\GSTestDX.gsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_GEOMETRY;
        pDevice->CreateShader( CreationAttrs, &pGS );
    }

    {
        CreationAttrs.FilePath = "Shaders\\GSTestDX.psh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        pDevice->CreateShader( CreationAttrs, &pPS );
    }

    PipelineStateDesc PSODesc;
    PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    PSODesc.GraphicsPipeline.BlendDesc.IndependentBlendEnable = False;
    PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = False;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pGS = pGS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_POINT_LIST;
    
    pDevice->CreatePipelineState( PSODesc, &m_pPSO );
}
    
void TestGeometryShader::Draw()
{
    if(!m_pDeviceContext)
        return;

    m_pDeviceContext->SetPipelineState(m_pPSO);
    m_pDeviceContext->CommitShaderResources(nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    // Draw 2 triangles
    Diligent::DrawAttribs DrawAttrs(2, DRAW_FLAG_VERIFY_ALL);
    m_pDeviceContext->Draw(DrawAttrs);
    
    SetStatus(TestResult::Succeeded);
}
