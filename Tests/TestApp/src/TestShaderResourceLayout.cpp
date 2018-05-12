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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "TestShaderResourceLayout.h"
#include "BasicShaderSourceStreamFactory.h"

using namespace Diligent;

TestShaderResourceLayout::TestShaderResourceLayout( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Shader resource layout initialization test"),
    m_pDeviceContext(pContext)
{
    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory("Shaders");
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.EntryPoint = "main";

    RefCntAutoPtr<ITexture> pTex[4];
    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = 1024;
    TexDesc.Height = 1024;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    IDeviceObject *pSRVs[4];
    for(int i=0; i < 4; ++i)
    {
        pDevice->CreateTexture(TexDesc, TextureData{}, &(pTex[i]));
        pSRVs[i] = pTex[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }


    TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    RefCntAutoPtr<ITexture> pStorageTex[4];
    IDeviceObject *pUAVs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateTexture(TexDesc, TextureData{}, &(pStorageTex[i]));
        pUAVs[i] = pStorageTex[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
    }


    BufferDesc BuffDesc;
    BuffDesc.uiSizeInBytes = 1024;
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
    RefCntAutoPtr<IBuffer> pUniformBuffs[4];
    IDeviceObject *pUBs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateBuffer(BuffDesc, BufferData{}, &(pUniformBuffs[i]));
        pUBs[i] = pUniformBuffs[i];
    }

    BuffDesc.BindFlags = BIND_UNORDERED_ACCESS;
    RefCntAutoPtr<IBuffer> pStorgeBuffs[4];
    IDeviceObject *pSBs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateBuffer(BuffDesc, BufferData{}, &(pStorgeBuffs[i]));
        pSBs[i] = pStorgeBuffs[i];
    }

    RefCntAutoPtr<IShader> pVS;
    if(pDevice->GetDeviceCaps().DevType == DeviceType::Vulkan)
    {
        CreationAttrs.Desc.Name = "Shader resource layout test VS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.vsh";
        ShaderVariableDesc VarDesc[] = 
        {
            { "g_tex2D_Static", SHADER_VARIABLE_TYPE_STATIC },
            { "g_tex2D_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2D_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DArr_Static", SHADER_VARIABLE_TYPE_STATIC },
            { "g_tex2DArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2DArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "UniformBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "UniformBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "UniformBuffArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "UniformBuffArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "storageBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "storageBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "storageBuffArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "storageBuffArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DStorageImgArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2DStorageImgArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DNoResourceTest", SHADER_VARIABLE_TYPE_DYNAMIC }
        };
        StaticSamplerDesc StaticSamplers[] = 
        {
            {"g_tex2D_Static", SamplerDesc{}},
            {"g_tex2DArr_Mut", SamplerDesc{}},
            {"g_tex2DNoStaticSamplerTest", SamplerDesc{} }
        };
        CreationAttrs.Desc.VariableDesc = VarDesc;
        CreationAttrs.Desc.NumVariables = _countof(VarDesc);
        CreationAttrs.Desc.NumStaticSamplers = _countof(StaticSamplers);
        CreationAttrs.Desc.StaticSamplers = StaticSamplers;

        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS);

        pVS->GetShaderVariable("g_tex2D_Static")->Set(pSRVs[0]);
        pVS->GetShaderVariable("g_tex2DArr_Static")->SetArray(pSRVs, 0, 2);
        pVS->GetShaderVariable("UniformBuff_Stat")->Set(pUBs[0]);
        pVS->GetShaderVariable("UniformBuffArr_Stat")->SetArray(pUBs, 0, 2);
        pVS->GetShaderVariable("storageBuff_Static")->Set(pSBs[0]);
        pVS->GetShaderVariable("storageBuffArr_Static")->SetArray(pSBs, 0, 2);
        pVS->GetShaderVariable("g_tex2DStorageImg_Stat")->Set(pUAVs[0]);
    }

    RefCntAutoPtr<IShader> pPS;
    if (pDevice->GetDeviceCaps().DevType == DeviceType::Vulkan)
    {
        CreationAttrs.Desc.Name = "Shader resource layout test FS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.psh";
        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS);

        pPS->GetShaderVariable("g_tex2D_Static")->Set(pSRVs[0]);
        pPS->GetShaderVariable("g_tex2DArr_Static")->SetArray(pSRVs, 0, 2);
        pPS->GetShaderVariable("UniformBuff_Stat")->Set(pUBs[0]);
        pPS->GetShaderVariable("UniformBuffArr_Stat")->SetArray(pUBs, 0, 2);
        pPS->GetShaderVariable("storageBuff_Static")->Set(pSBs[0]);
        pPS->GetShaderVariable("storageBuffArr_Static")->SetArray(pSBs, 0, 2);
    }

    PipelineStateDesc PSODesc;
    PSODesc.Name = "Shader resource layout test";
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
    PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;

    RefCntAutoPtr<IPipelineState> pTestPSO;
    pDevice->CreatePipelineState(PSODesc, &pTestPSO);
    VERIFY_EXPR(pTestPSO);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pTestPSO->CreateShaderResourceBinding(&pSRB);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Mut")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DArr_Mut")->SetArray(pSRVs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Dyn")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DArr_Dyn")->SetArray(pSRVs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Mut")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuffArr_Mut")->SetArray(pUBs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Dyn")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuffArr_Dyn")->SetArray(pUBs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuff_Mut")->Set(pSBs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuffArr_Mut")->SetArray(pSBs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuff_Dyn")->Set(pSBs[0]);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "storageBuffArr_Dyn")->SetArray(pSBs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Mut")->SetArray(pUAVs, 0, 2);
    pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs, 0, 2);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Mut")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DArr_Mut")->SetArray(pSRVs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Dyn")->Set(pSRVs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DArr_Dyn")->SetArray(pSRVs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Mut")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuffArr_Mut")->SetArray(pUBs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Dyn")->Set(pUBs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn")->SetArray(pUBs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuff_Mut")->Set(pSBs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuffArr_Mut")->SetArray(pSBs, 0, 3);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuff_Dyn")->Set(pSBs[0]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "storageBuffArr_Dyn")->SetArray(pSBs, 0, 4);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Mut")->SetArray(pUAVs, 0, 2);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs, 0, 2);


    SetStatus(TestResult::Succeeded);
}
