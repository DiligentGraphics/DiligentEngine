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
#include "TestSeparateTextureSampler.h"

static const char g_VSShaderSource[] = 
R"(
void VSMain(out float4 pos : SV_POSITION)
{
	pos = float4(0.0, 0.0, 0.0, 0.0);
}
)";

static const char g_PSShaderSource[] = 
R"(

Texture2D g_Tex;
SamplerState g_Sam;
Texture2D g_Tex2;
SamplerState g_Sam2;
SamplerState g_Sam3[2];
SamplerState g_Sam4[2];

void PSMain(out float4 col : SV_TARGET)
{
	col = g_Tex.Sample(g_Sam, float2(0.5, 0.5)) + 
          g_Tex2.Sample(g_Sam2, float2(0.5, 0.5)) + 
          g_Tex2.Sample(g_Sam3[0], float2(0.5, 0.5)) + 
          g_Tex2.Sample(g_Sam3[1], float2(0.5, 0.5)) +
          g_Tex2.Sample(g_Sam4[0], float2(0.5, 0.5)) + 
          g_Tex2.Sample(g_Sam4[1], float2(0.5, 0.5));
}
)";

using namespace Diligent;

TestSeparateTextureSampler::TestSeparateTextureSampler(IRenderDevice *pDevice, IDeviceContext *pContext) : 
    UnitTestBase("Test separate texture sampler")
{
    if (pDevice->GetDeviceCaps().IsD3DDevice() || pDevice->GetDeviceCaps().IsVulkanDevice())
    {
        ShaderCreateInfo Attrs;
        Attrs.Source = g_VSShaderSource;
        Attrs.EntryPoint = "VSMain";
        Attrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        Attrs.Desc.Name = "VSMain (TestSeparateTextureSampler)";
        Attrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        RefCntAutoPtr<IShader> pVS;
        pDevice->CreateShader(Attrs, &pVS);

        Attrs.Source = g_PSShaderSource;
        Attrs.EntryPoint = "PSMain";
        Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        Attrs.Desc.Name = "PSMain (TestSeparateTextureSampler)";
        
        RefCntAutoPtr<IShader> pPS;
        pDevice->CreateShader(Attrs, &pPS);

        PipelineStateDesc PSODesc;
        PSODesc.GraphicsPipeline.pVS = pVS;
        PSODesc.GraphicsPipeline.pPS = pPS;
        PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSODesc.GraphicsPipeline.NumRenderTargets = 1;
        PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
        PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_UNKNOWN;
        PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

        ShaderResourceVariableDesc Vars[] = 
        {
            {SHADER_TYPE_PIXEL, "g_Tex", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL, "g_Sam", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL, "g_Tex2", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {SHADER_TYPE_PIXEL, "g_Sam2", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL, "g_Sam4", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
        };
        PSODesc.ResourceLayout.Variables = Vars;
        PSODesc.ResourceLayout.NumVariables = _countof(Vars);
        
        StaticSamplerDesc StaticSamplers[] = 
        {
            {SHADER_TYPE_PIXEL, "g_Sam2", SamplerDesc{}}
        };
        PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;
        PSODesc.ResourceLayout.NumStaticSamplers = _countof(StaticSamplers);

        RefCntAutoPtr<IPipelineState> pPSO;
        pDevice->CreatePipelineState(PSODesc, &pPSO);
        
        TextureDesc TexDesc;
        TexDesc.Name = "Separate sampler texture test";
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Width = 256;
        TexDesc.Height = 256;
        TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.Usage = USAGE_DEFAULT;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        RefCntAutoPtr<ITexture> pTexture;
        pDevice->CreateTexture( TexDesc, nullptr, &pTexture );

        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler( SamplerDesc{}, &pSampler );
        IDeviceObject* ppSamplers[2] = {pSampler, pSampler};
        pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Sam3")->SetArray(ppSamplers, 0, 2);

        RefCntAutoPtr<IShaderResourceBinding> pSRB;
        pPSO->CreateShaderResourceBinding(&pSRB, true);

        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Tex")->Set(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Sam")->Set(pSampler);
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Sam4")->SetArray(ppSamplers, 0, 2);
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Tex2")->Set(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        VERIFY_EXPR(pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Sam2") == nullptr);

        auto VarCount = pSRB->GetVariableCount(SHADER_TYPE_PIXEL);
        VERIFY_EXPR(VarCount == 4);
        for(Uint32 v=0; v < VarCount; ++v)
        {
            auto* pVar = pSRB->GetVariableByIndex(SHADER_TYPE_PIXEL, v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE || pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
            auto pVar2 = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, pVar->GetResourceDesc().Name);
            VERIFY_EXPR(pVar == pVar2);
            (void)pVar2;
        }

        TextureDesc RenderTargetDesc;
        RenderTargetDesc.Type = RESOURCE_DIM_TEX_2D;
        RenderTargetDesc.Width  = 256;
        RenderTargetDesc.Height = 256;
        RenderTargetDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
        RenderTargetDesc.Format = TEX_FORMAT_RGBA8_UNORM;
        RenderTargetDesc.Name = "TestSeparateTextureSampler: test render target";
        RefCntAutoPtr<ITexture> pRenderTarget;
        pDevice->CreateTexture(RenderTargetDesc, nullptr, &pRenderTarget);

        ITextureView* pRTV[] = {pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};
        pContext->SetRenderTargets(1, pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        float Zero[4] = {};
        pContext->ClearRenderTarget(pRTV[0], Zero, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
        pContext->Draw(DrawAttrs);
        
        pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        SetStatus(TestResult::Succeeded);
    }
    else
    {
        SetStatus(TestResult::Skipped);
    }
}
