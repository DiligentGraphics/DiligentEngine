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
#include "TestShaderResourceLayout.h"

using namespace Diligent;

void PrintShaderResources(IShader* pShader);

TestShaderResourceLayout::TestShaderResourceLayout( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Shader resource layout initialization test"),
    m_pDeviceContext(pContext)
{
    if (pDevice->GetDeviceCaps().DevType != DeviceType::Vulkan)
    {
        SetStatus(TestResult::Skipped);
        return;
    }

    ShaderCreateInfo CreationAttrs;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("Shaders", &pShaderSourceFactory);
    CreationAttrs.pShaderSourceStreamFactory = pShaderSourceFactory;
    CreationAttrs.EntryPoint = "main";
    CreationAttrs.UseCombinedTextureSamplers = false;

    RefCntAutoPtr<ISampler> pSamplers[4];
    IDeviceObject *pSams[4];
    for (int i = 0; i < 4; ++i)
    {
        SamplerDesc SamDesc;
        pDevice->CreateSampler(SamDesc, &(pSamplers[i]));
        pSams[i] = pSamplers[i];
    }

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
        pDevice->CreateTexture(TexDesc, nullptr, &(pTex[i]));
        auto *pSRV = pTex[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        pSRV->SetSampler(pSamplers[i]);
        pSRVs[i] = pSRV;
    }

    TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
    RefCntAutoPtr<ITexture> pStorageTex[4];
    IDeviceObject *pUAVs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateTexture(TexDesc, nullptr, &(pStorageTex[i]));
        pUAVs[i] = pStorageTex[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
    }

    TexDesc.BindFlags = BIND_RENDER_TARGET;
    RefCntAutoPtr<ITexture> pRenderTarget;
    pDevice->CreateTexture(TexDesc, nullptr, &pRenderTarget);
    auto *pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    m_pDeviceContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float Zero[4] = {};
    m_pDeviceContext->ClearRenderTarget(pRTV, Zero, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    BufferDesc BuffDesc;
    BuffDesc.uiSizeInBytes = 1024;
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
    RefCntAutoPtr<IBuffer> pUniformBuffs[4];
    IDeviceObject *pUBs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateBuffer(BuffDesc, nullptr, &(pUniformBuffs[i]));
        pUBs[i] = pUniformBuffs[i];
    }

    BuffDesc.BindFlags = BIND_UNORDERED_ACCESS;
    BuffDesc.Mode = BUFFER_MODE_STRUCTURED;
    BuffDesc.ElementByteStride = 16;
    RefCntAutoPtr<IBuffer> pStorgeBuffs[4];
    IDeviceObject *pSBUAVs[4];
    for (int i = 0; i < 4; ++i)
    {
        pDevice->CreateBuffer(BuffDesc, nullptr, &(pStorgeBuffs[i]));
        pSBUAVs[i] = pStorgeBuffs[i]->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
    }

    RefCntAutoPtr<IBuffer> pUniformTexelBuff, pStorageTexelBuff;
    RefCntAutoPtr<IBufferView> pUniformTexelBuffSRV , pStorageTexelBuffUAV;
    {
        BufferDesc TxlBuffDesc;
        TxlBuffDesc.Name = "Uniform texel buffer test";
        TxlBuffDesc.uiSizeInBytes = 256;
        TxlBuffDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
        TxlBuffDesc.Usage = USAGE_DEFAULT;
        TxlBuffDesc.ElementByteStride = 16;
        TxlBuffDesc.Mode = BUFFER_MODE_FORMATTED;
        pDevice->CreateBuffer(TxlBuffDesc, nullptr, &pUniformTexelBuff);

        BufferViewDesc TxlBuffViewDesc;
        TxlBuffViewDesc.Name = "Uniform texel buffer SRV";
        TxlBuffViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        TxlBuffViewDesc.Format.ValueType = VT_FLOAT32;
        TxlBuffViewDesc.Format.NumComponents = 4;
        TxlBuffViewDesc.Format.IsNormalized = false;
        pUniformTexelBuff->CreateView(TxlBuffViewDesc, &pUniformTexelBuffSRV);

        TxlBuffDesc.Name = "Storage texel buffer test";
        TxlBuffDesc.BindFlags = BIND_UNORDERED_ACCESS;
        pDevice->CreateBuffer(TxlBuffDesc, nullptr, &pStorageTexelBuff);
        
        TxlBuffViewDesc.Name = "Storage texel buffer UAV";
        TxlBuffViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
        pUniformTexelBuff->CreateView(TxlBuffViewDesc, &pStorageTexelBuffUAV);
    }
    
    ResourceMappingDesc ResMappingDesc;
    ResourceMappingEntry MappingEntries[] = 
    {
        {"g_tex2D_Static", pSRVs[0]},
        {"g_tex2DArr_Static", pSRVs[0], 0},
        {"g_tex2DArr_Static", pSRVs[1], 1},
        {"g_sepTex2D_static", pSRVs[0]},
        {"g_sepTex2DArr_static", pSRVs[0], 0},
        {"g_sepTex2DArr_static", pSRVs[1], 1},
        {"g_tex2D_Mut", pSRVs[0]},
        {"g_tex2DArr_Mut", pSRVs[0], 0},
        {"g_tex2DArr_Mut", pSRVs[1], 1},
        {"g_tex2DArr_Mut", pSRVs[2], 2},
        {"g_tex2D_Dyn", pSRVs[0]},
        {"g_tex2DArr_Dyn", pSRVs[0], 0},
        {"g_tex2DArr_Dyn", pSRVs[1], 1},
        {"g_tex2DArr_Dyn", pSRVs[2], 2},
        {"g_tex2DArr_Dyn", pSRVs[3], 3},
        {"g_tex2DArr_Dyn", pSRVs[0], 4},
        {"g_tex2DArr_Dyn", pSRVs[1], 5},
        {"g_tex2DArr_Dyn", pSRVs[2], 6},
        {"g_tex2DArr_Dyn", pSRVs[3], 7},
        {"g_sepTex2D_mut", pSRVs[0]},
        {"g_sepTex2DArr_mut", pSRVs[0], 0},
        {"g_sepTex2DArr_mut", pSRVs[1], 1},
        {"g_sepTex2DArr_mut", pSRVs[2], 2},
        {"g_sepTex2D_dyn", pSRVs[0]},
        {"g_sepTex2DArr_dyn", pSRVs[0], 0},
        {"g_sepTex2DArr_dyn", pSRVs[1], 1},
        {"g_sepTex2DArr_dyn", pSRVs[2], 2},
        {"g_sepTex2DArr_dyn", pSRVs[3], 3},
        {"g_sepTex2DArr_dyn", pSRVs[0], 4},
        {"g_sepTex2DArr_dyn", pSRVs[1], 5},
        {"g_sepTex2DArr_dyn", pSRVs[2], 6},
        {"g_sepTex2DArr_dyn", pSRVs[3], 7},
        {}
    };
    ResMappingDesc.pEntries = MappingEntries;
    RefCntAutoPtr<IResourceMapping> pResMapping;
    pDevice->CreateResourceMapping(ResMappingDesc, &pResMapping);

    RefCntAutoPtr<IShader> pVS;
    {
        CreationAttrs.Desc.Name = "Shader resource layout test VS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.vsh";

        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        CreationAttrs.Desc.Name = "Shader resource layout test FS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.psh";

        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS);

        PrintShaderResources(pPS);
    }

    PipelineStateDesc PSODesc;

    ShaderResourceVariableDesc VarDesc[] = 
    {
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DArr_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DArr_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_sepTex2DArr_static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_sepTex2D_mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_sepTex2DArr_mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_sepTex2D_dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_sepTex2DArr_dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_SamArr_static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sam_mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_SamArr_mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sam_dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_SamArr_dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuffArr_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "storageBuff_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "storageBuff_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "storageBuffArr_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "storageBuffArr_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DNoResourceTest", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_UniformTexelBuff_mut", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_StorageTexelBuff_mut", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
    };
    StaticSamplerDesc StaticSamplers[] = 
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Static", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DArr_Mut", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sam_static", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_SamArr_mut", SamplerDesc{} },
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sam_dyn", SamplerDesc{} },
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2DNoStaticSamplerTest", SamplerDesc{} }
    };
    PSODesc.ResourceLayout.Variables = VarDesc;
    PSODesc.ResourceLayout.NumVariables = _countof(VarDesc);
    PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;
    PSODesc.ResourceLayout.NumStaticSamplers = _countof(StaticSamplers);

    PSODesc.Name = "Shader resource layout test";
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
    PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_UNKNOWN;
    PSODesc.SRBAllocationGranularity = 16;

    RefCntAutoPtr<IPipelineState> pTestPSO;
    LOG_INFO_MESSAGE("The 2 warnings below about missing shader resources are part of the test");
    pDevice->CreatePipelineState(PSODesc, &pTestPSO);
   

    VERIFY_EXPR(pTestPSO);

    {
        //pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Static")->Set(pSRVs[0]);
        //pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2DArr_Static")->SetArray(pSRVs, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_sepTex2D_static")->Set(pSRVs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_sepTex2DArr_static")->SetArray(pSRVs, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_SamArr_static")->SetArray(pSams, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat")->Set(pUBs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "UniformBuffArr_Stat")->SetArray(pUBs, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "storageBuff_Static")->Set(pSBUAVs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "storageBuffArr_Static")->SetArray(pSBUAVs, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2DStorageImg_Stat")->Set(pUAVs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_UniformTexelBuff")->Set(pUniformTexelBuffSRV);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_StorageTexelBuff")->Set(pStorageTexelBuffUAV);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut");
        auto* pStaticSam = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Sam_static");
        VERIFY_EXPR(pStaticSam == nullptr);
        

        auto NumVSVars = pTestPSO->GetStaticVariableCount(SHADER_TYPE_VERTEX);
        for(Uint32 v=0; v < NumVSVars; ++v)
        {
            auto pVar = pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_VERTEX, v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
            auto pVar2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, pVar->GetResourceDesc().Name);
            VERIFY_EXPR(pVar == pVar2);
        }
    }

    {
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Static")->Set(pSRVs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2DArr_Static")->SetArray(pSRVs, 0, 2);
        //pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_sepTex2D_static")->Set(pSRVs[0]);
        //pTestPSO->GetStaticVariableByNames(SHADER_TYPE_PIXEL, "g_sepTex2DArr_static")->SetArray(pSRVs, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_SamArr_static")->SetArray(pSams, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Stat")->Set(pUBs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "UniformBuffArr_Stat")->SetArray(pUBs, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "storageBuff_Static")->Set(pSBUAVs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "storageBuffArr_Static")->SetArray(pSBUAVs, 0, 2);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2DStorageImg_Stat")->Set(pUAVs[0]);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_UniformTexelBuff")->Set(pUniformTexelBuffSRV);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_StorageTexelBuff")->Set(pStorageTexelBuffUAV);
        pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "storageBuff_Dyn");
        auto* pStaticSam = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Sam_static");
        VERIFY_EXPR(pStaticSam == nullptr);


        auto NumPSVars = pTestPSO->GetStaticVariableCount(SHADER_TYPE_PIXEL);
        for(Uint32 v=0; v < NumPSVars; ++v)
        {
            auto pVar = pTestPSO->GetStaticVariableByIndex(SHADER_TYPE_PIXEL, v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
            auto pVar2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, pVar->GetResourceDesc().Name);
            VERIFY_EXPR(pVar == pVar2);
        }
    }

    pTestPSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED | BIND_SHADER_RESOURCES_UPDATE_STATIC);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pTestPSO->CreateShaderResourceBinding(&pSRB, true);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat");
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_sepTex2DArr_static");
    LOG_INFO_MESSAGE("The above 2 errors about missing shader resources are part of the test");

    //pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut")->Set(pSRVs[0]);
    //pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2DArr_Mut")->SetArray(pSRVs, 0, 3);
    //pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn")->Set(pSRVs[0]);
    //pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2DArr_Dyn")->SetArray(pSRVs, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_sepTex2D_mut")->Set(pSRVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_sepTex2DArr_mut")->SetArray(pSRVs, 0, 3);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_sepTex2D_dyn")->Set(pSRVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_sepTex2DArr_dyn")->SetArray(pSRVs, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Sam_mut")->Set(pSams[0]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_SamArr_dyn")->SetArray(pSams, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Mut")->Set(pUBs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuffArr_Mut")->SetArray(pUBs, 0, 3);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Dyn")->Set(pUBs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuffArr_Dyn")->SetArray(pUBs, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "storageBuff_Mut")->Set(pSBUAVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "storageBuffArr_Mut")->SetArray(pSBUAVs, 0, 3);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "storageBuff_Dyn")->Set(pSBUAVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "storageBuffArr_Dyn")->SetArray(pSBUAVs, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Mut")->SetArray(pUAVs, 0, 2);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs, 0, 2);

    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_UniformTexelBuff_mut")->Set(pUniformTexelBuffSRV);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_StorageTexelBuff_mut")->Set(pStorageTexelBuffUAV);

    

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut")->Set(pSRVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DArr_Mut")->SetArray(pSRVs, 0, 3);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn")->Set(pSRVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DArr_Dyn")->SetArray(pSRVs, 0, 4);

    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2D_mut")->Set(pSRVs[0]);
    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2DArr_mut")->SetArray(pSRVs, 0, 3);
    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2D_dyn")->Set(pSRVs[0]);
    //pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_sepTex2DArr_dyn")->SetArray(pSRVs, 0, 4);
    
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Sam_mut")->Set(pSams[0]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_SamArr_dyn")->SetArray(pSams, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Mut")->Set(pUBs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuffArr_Mut")->SetArray(pUBs, 0, 3);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Dyn")->Set(pUBs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn")->SetArray(pUBs, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "storageBuff_Mut")->Set(pSBUAVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "storageBuffArr_Mut")->SetArray(pSBUAVs, 0, 3);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "storageBuff_Dyn")->Set(pSBUAVs[0]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "storageBuffArr_Dyn")->SetArray(pSBUAVs, 0, 4);

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Mut")->SetArray(pUAVs, 0, 2);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs, 0, 2);

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_UniformTexelBuff_mut")->Set(pUniformTexelBuffSRV);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_StorageTexelBuff_mut")->Set(pStorageTexelBuffUAV);

    pSRB->BindResources(SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED | BIND_SHADER_RESOURCES_UPDATE_MUTABLE | BIND_SHADER_RESOURCES_UPDATE_DYNAMIC);

    pContext->SetPipelineState(pTestPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "storageBuff_Dyn")->Set(pSBUAVs[1]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn")->Set(pSRVs[1]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_sepTex2D_dyn")->Set(pSRVs[1]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_SamArr_dyn")->SetArray(pSams+1, 1, 3);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Dyn")->Set(pUBs[1]);
    pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2DStorageImgArr_Dyn")->SetArray(pUAVs+1, 1, 1);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->Draw(DrawAttrs);

    {
        auto NumVSVars = pSRB->GetVariableCount(SHADER_TYPE_VERTEX);
        for(Uint32 v=0; v < NumVSVars; ++v)
        {
            auto pVar = pSRB->GetVariableByIndex(SHADER_TYPE_VERTEX, v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE || pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
            auto pVar2 = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, pVar->GetResourceDesc().Name);
            VERIFY_EXPR(pVar == pVar2);
        }
    }

    {
        auto NumPSVars = pSRB->GetVariableCount(SHADER_TYPE_PIXEL);
        for(Uint32 v=0; v < NumPSVars; ++v)
        {
            auto pVar = pSRB->GetVariableByIndex(SHADER_TYPE_PIXEL, v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE || pVar->GetType() == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
            auto pVar2 = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, pVar->GetResourceDesc().Name);
            VERIFY_EXPR(pVar == pVar2);
        }
    }

    SetStatus(TestResult::Succeeded);
}
