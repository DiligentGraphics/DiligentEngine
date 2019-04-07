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
#include "TestShaderVarAccess.h"
#include "GraphicsAccessories.h"
#include "../../../DiligentCore/Graphics/GraphicsEngineD3DBase/interface/ShaderD3D.h"

using namespace Diligent;

void PrintShaderResources(IShader* pShader)
{
    RefCntAutoPtr<IShaderD3D> pShaderD3D(pShader, IID_ShaderD3D);

    std::stringstream ss;
    ss << "Resources of shader '" << pShader->GetDesc().Name << "':" << std::endl;
    for (Uint32 res=0; res < pShader->GetResourceCount(); ++res)
    {
        auto ResDec = pShader->GetResource(res);
        std::stringstream name_ss;
        name_ss << ResDec.Name;
        if(ResDec.ArraySize > 1)
            name_ss << "[" << ResDec.ArraySize << "]";
        ss << std::setw(2) << std::right << res << ": " << std::setw(25) << std::left << name_ss.str();
        if (pShaderD3D)
        {
            auto HLSLResDesc = pShaderD3D->GetHLSLResource(res);
            ss << "  hlsl register " << std::setw(2) <<  HLSLResDesc.ShaderRegister;
        }
        ss << "   " << GetShaderResourceTypeLiteralName(ResDec.Type) << std::endl;
    }
    LOG_INFO_MESSAGE(ss.str());
}

TestShaderVarAccess::TestShaderVarAccess( IRenderDevice *pDevice, IDeviceContext *pContext, Diligent::ISwapChain* pSwapChain ) :
    UnitTestBase("Shader variable access test"),
    m_pDeviceContext(pContext)
{
    if( pDevice->GetDeviceCaps().DevType == DeviceType::OpenGLES )
        return;

    ShaderCreateInfo CreationAttrs;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("Shaders", &pShaderSourceFactory);
    CreationAttrs.pShaderSourceStreamFactory = pShaderSourceFactory;
    CreationAttrs.EntryPoint = "main";
    CreationAttrs.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<ISampler> pSamplers[2];
    IDeviceObject *pSams[2];
    for (size_t i = 0; i < _countof(pSams); ++i)
    {
        SamplerDesc SamDesc;
        pDevice->CreateSampler(SamDesc, &(pSamplers[i]));
        pSams[i] = pSamplers[i];
    }

    RefCntAutoPtr<ITexture> pTex[2];
    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = 1024;
    TexDesc.Height = 1024;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    IDeviceObject *pSRVs[2];
    for(size_t i=0; i < _countof(pSRVs); ++i)
    {
        pDevice->CreateTexture(TexDesc, nullptr, &(pTex[i]));
        auto *pSRV = pTex[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        pSRV->SetSampler(pSamplers[i]);
        pSRVs[i] = pSRV;
    }

    RefCntAutoPtr<ITexture> pRWTex[8];
    IDeviceObject *pTexUAVs[_countof(pRWTex)];
    IDeviceObject *pRWTexSRVs[_countof(pRWTex)];
    TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    TexDesc.Format = TEX_FORMAT_RGBA32_FLOAT;
    for(size_t i=0; i < _countof(pRWTex); ++i)
    {
        pDevice->CreateTexture(TexDesc, nullptr, &(pRWTex[i]));
        pTexUAVs[i] = pRWTex[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
        pRWTexSRVs[i] = pRWTex[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }


    //RefCntAutoPtr<ITexture> pStorageTex[4];
    //IDeviceObject *pUAVs[4];
    //for (int i = 0; i < 4; ++i)
    //{
    //    pDevice->CreateTexture(TexDesc, TextureData{}, &(pStorageTex[i]));
    //    pUAVs[i] = pStorageTex[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
    //}

    TexDesc.Format = pSwapChain->GetDesc().ColorBufferFormat;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    RefCntAutoPtr<ITexture> pRenderTarget;
    pDevice->CreateTexture(TexDesc, nullptr, &pRenderTarget);
    auto *pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);

    TexDesc.Format = pSwapChain->GetDesc().DepthBufferFormat;
    TexDesc.BindFlags = BIND_DEPTH_STENCIL;
    RefCntAutoPtr<ITexture> pDepthTex;
    pDevice->CreateTexture(TexDesc, nullptr, &pDepthTex);
    auto *pDSV = pDepthTex->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    BufferDesc BuffDesc;
    BuffDesc.uiSizeInBytes = 1024;
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
    RefCntAutoPtr<IBuffer> pUniformBuffs[2];
    IDeviceObject *pUBs[2];
    for (int i = 0; i < _countof(pUniformBuffs); ++i)
    {
        pDevice->CreateBuffer(BuffDesc, nullptr, &(pUniformBuffs[i]));
        pUBs[i] = pUniformBuffs[i];
    }

    //BuffDesc.BindFlags = BIND_UNORDERED_ACCESS;
    //BuffDesc.Mode = BUFFER_MODE_STRUCTURED;
    //BuffDesc.ElementByteStride = 16;
    //RefCntAutoPtr<IBuffer> pStorgeBuffs[4];
    //IDeviceObject *pSBUAVs[4];
    //for (int i = 0; i < 4; ++i)
    //{
    //    pDevice->CreateBuffer(BuffDesc, BufferData{}, &(pStorgeBuffs[i]));
    //    pSBUAVs[i] = pStorgeBuffs[i]->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
    //}

    RefCntAutoPtr<IBuffer> pFormattedBuff0, pFormattedBuff[4], pRawBuff[2];
    IDeviceObject *pFormattedBuffSRV = nullptr, *pFormattedBuffUAV[4] = {}, *pFormattedBuffSRVs[4] = {};
    RefCntAutoPtr<IBufferView> spFormattedBuffSRV, spFormattedBuffUAV[4], spFormattedBuffSRVs[4];
    RefCntAutoPtr<IBufferView> spRawBuffUAV[2], spRawBuffSRVs[2];
    {
        Diligent::BufferDesc TxlBuffDesc;
        TxlBuffDesc.Name = "Uniform texel buffer test";
        TxlBuffDesc.uiSizeInBytes = 256;
        TxlBuffDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
        TxlBuffDesc.Usage = USAGE_DEFAULT;
        TxlBuffDesc.ElementByteStride = 16;
        TxlBuffDesc.Mode = BUFFER_MODE_FORMATTED;
        pDevice->CreateBuffer(TxlBuffDesc, nullptr, &pFormattedBuff0);
        
        Diligent::BufferViewDesc ViewDesc;
        ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        ViewDesc.Format.ValueType = VT_FLOAT32;
        ViewDesc.Format.NumComponents = 4;
        ViewDesc.Format.IsNormalized = false;
        pFormattedBuff0->CreateView(ViewDesc, &spFormattedBuffSRV);
        pFormattedBuffSRV = spFormattedBuffSRV;

        for(size_t i=0; i < _countof(pFormattedBuff); ++i)
        {
            TxlBuffDesc.Name = "UAV buffer test";
            TxlBuffDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
            pDevice->CreateBuffer(TxlBuffDesc, nullptr, &(pFormattedBuff[i]));
            
            ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
            pFormattedBuff[i]->CreateView(ViewDesc, &(spFormattedBuffUAV[i]));
            pFormattedBuffUAV[i] = spFormattedBuffUAV[i];

            ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
            pFormattedBuff[i]->CreateView(ViewDesc, &(spFormattedBuffSRVs[i]));
            pFormattedBuffSRVs[i] = spFormattedBuffSRVs[i];
        }

        TxlBuffDesc.Mode = BUFFER_MODE_RAW;
        ViewDesc.Format.ValueType = VT_UNDEFINED;
        for(size_t i=0; i < _countof(pRawBuff); ++i)
        {
            TxlBuffDesc.Name = "Raw buffer test";
            pDevice->CreateBuffer(TxlBuffDesc, nullptr, &(pRawBuff[i]));
            spRawBuffUAV[i]  = pRawBuff[i]->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
            spRawBuffSRVs[i] = pRawBuff[i]->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);
        }
    }
 
    RefCntAutoPtr<IShader> pVS;
    {
        CreationAttrs.Desc.Name = "Shader variable access test VS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_DEFAULT;
        CreationAttrs.FilePath = pDevice->GetDeviceCaps().IsD3DDevice() ? "Shaders\\ShaderVarAccessTestDX.vsh" : "Shaders\\ShaderVarAccessTestGL.vsh";

        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS);

        PrintShaderResources(pVS);
    }

    std::vector<ShaderResourceVariableDesc> VarDesc = 
    {
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_StaticArr", SHADER_RESOURCE_VARIABLE_TYPE_STATIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_MutArr", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_DynArr", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Buffer_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Buffer_MutArr", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Buffer_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Buffer_DynArr", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        {SHADER_TYPE_PIXEL, "g_rwtex2D_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_rwBuff_Mut", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}, 
        {SHADER_TYPE_PIXEL, "g_rwBuff_Dyn", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}, 
    };
    
    StaticSamplerDesc StaticSamplers[] = 
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Static", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_StaticArr", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Mut", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_MutArr", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_Dyn", SamplerDesc{}},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_tex2D_DynArr", SamplerDesc{}}
    };

    RefCntAutoPtr<IShader> pPS;
    {
        CreationAttrs.Desc.Name = "Shader variable access test PS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_DEFAULT;
        CreationAttrs.FilePath = pDevice->GetDeviceCaps().IsD3DDevice() ? "Shaders\\ShaderVarAccessTestDX.psh" : "Shaders\\ShaderVarAccessTestGL.psh";
        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS);

        PrintShaderResources(pPS);
    }


    PipelineStateDesc PSODesc;

    PSODesc.ResourceLayout.Variables = VarDesc.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(VarDesc.size());
    PSODesc.ResourceLayout.NumStaticSamplers = _countof(StaticSamplers);
    PSODesc.ResourceLayout.StaticSamplers = StaticSamplers;

    PSODesc.Name = "Shader variable access test PSO";
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    PSODesc.SRBAllocationGranularity = 16;

    RefCntAutoPtr<IPipelineState> pTestPSO;
    pDevice->CreatePipelineState(PSODesc, &pTestPSO);
    VERIFY_EXPR(pTestPSO);

    {
        VERIFY_EXPR(pTestPSO->GetStaticVariableCount(SHADER_TYPE_VERTEX) == 6);
        
        auto tex2D_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(tex2D_Static == pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, tex2D_Static->GetResourceDesc().Name));
        tex2D_Static->Set(pSRVs[0]);

        auto tex2D_Static_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Static_sampler");
        VERIFY_EXPR(tex2D_Static_sampler == nullptr);

        auto tex2D_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_StaticArr");
        VERIFY_EXPR(tex2D_StaticArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(tex2D_StaticArr == pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, tex2D_StaticArr->GetResourceDesc().Name));
        tex2D_StaticArr->SetArray(pSRVs, 0, 2);

        auto tex2D_StaticArr_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_StaticArr_sampler");
        VERIFY_EXPR(tex2D_StaticArr_sampler == nullptr);

        auto UniformBuff_Stat = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Stat == pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, UniformBuff_Stat->GetResourceDesc().Name));
        UniformBuff_Stat->Set(pUBs[0]);

        auto UniformBuff_Stat2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat2");
        VERIFY_EXPR(UniformBuff_Stat2->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Stat2 == pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, UniformBuff_Stat2->GetResourceDesc().Name));
        UniformBuff_Stat2->Set(pUBs[0]);

        auto Buffer_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_Static");
        VERIFY_EXPR(Buffer_Static->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(Buffer_Static == pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, Buffer_Static->GetResourceDesc().Name));
        Buffer_Static->Set(pFormattedBuffSRV);

        auto Buffer_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_StaticArr");
        VERIFY_EXPR(Buffer_StaticArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(Buffer_StaticArr == pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, Buffer_StaticArr->GetResourceDesc().Name));
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,0,1);
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,1,1);
        

        auto tex2D_Mut = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut == nullptr);
        auto tex2D_Dyn = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn == nullptr);
        auto tex2D_Mut_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut_sampler");
        VERIFY_EXPR(tex2D_Mut_sampler == nullptr);
        auto tex2D_Dyn_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_DynArr_sampler");
        VERIFY_EXPR(tex2D_Dyn_sampler == nullptr);


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
        VERIFY_EXPR(pTestPSO->GetStaticVariableCount(SHADER_TYPE_PIXEL) == 9);

        auto tex2D_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(tex2D_Static == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, tex2D_Static->GetResourceDesc().Name));
        tex2D_Static->Set(pSRVs[0]);

        auto tex2D_Static_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Static_sampler");
        VERIFY_EXPR(tex2D_Static_sampler == nullptr);

        auto tex2D_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_StaticArr");
        VERIFY_EXPR(tex2D_StaticArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(tex2D_StaticArr == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, tex2D_StaticArr->GetResourceDesc().Name));
        tex2D_StaticArr->SetArray(pSRVs, 0, 2);
        auto tex2D_StaticArr_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_StaticArr_sampler");
        VERIFY_EXPR(tex2D_StaticArr_sampler == nullptr);
        
        auto UniformBuff_Stat = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Stat == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, UniformBuff_Stat->GetResourceDesc().Name));
        UniformBuff_Stat->Set(pUBs[0]);

        auto UniformBuff_Stat2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Stat2");
        VERIFY_EXPR(UniformBuff_Stat2->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Stat2 == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, UniformBuff_Stat2->GetResourceDesc().Name));
        UniformBuff_Stat2->Set(pUBs[0]);

        auto Buffer_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Static");
        VERIFY_EXPR(Buffer_Static->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(Buffer_Static == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, Buffer_Static->GetResourceDesc().Name));
        Buffer_Static->Set(pFormattedBuffSRV);

        auto Buffer_StaticArr = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_StaticArr");
        VERIFY_EXPR(Buffer_StaticArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(Buffer_StaticArr == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, Buffer_StaticArr->GetResourceDesc().Name));
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,0,1);
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,1,1);
        

        auto rwtex2D_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Static");
        VERIFY_EXPR(rwtex2D_Static->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(rwtex2D_Static == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, rwtex2D_Static->GetResourceDesc().Name));
        rwtex2D_Static->Set(pTexUAVs[0]);

        auto rwtex2D_Static2 = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Static2");
        VERIFY_EXPR(rwtex2D_Static2->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(rwtex2D_Static2 == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, rwtex2D_Static2->GetResourceDesc().Name));
        rwtex2D_Static2->Set(pTexUAVs[1]);

        auto rwBuff_Static = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Static");
        VERIFY_EXPR(rwBuff_Static->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(rwBuff_Static == pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, rwBuff_Static->GetResourceDesc().Name));
        rwBuff_Static->Set(spRawBuffUAV[0]);


        auto tex2D_Mut = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut == nullptr);
        auto tex2D_Dyn = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn == nullptr);
        auto tex2D_Mut_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut_sampler");
        VERIFY_EXPR(tex2D_Mut_sampler == nullptr);
        auto tex2D_Dyn_sampler = pTestPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_DynArr_sampler");
        VERIFY_EXPR(tex2D_Dyn_sampler == nullptr);


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

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pTestPSO->CreateShaderResourceBinding(&pSRB, true);

    {
        auto tex2D_Mut = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(tex2D_Mut == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, tex2D_Mut->GetResourceDesc().Name));
        tex2D_Mut->Set(pSRVs[0]);

        auto tex2D_Mut_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Mut_sampler");
        VERIFY_EXPR(tex2D_Mut_sampler == nullptr);

        auto tex2D_MutArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_MutArr");
        VERIFY_EXPR(tex2D_MutArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(tex2D_MutArr == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, tex2D_MutArr->GetResourceDesc().Name));
        tex2D_MutArr->SetArray(pSRVs, 0, 2);

        auto tex2D_MutArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_MutArr_sampler");
        VERIFY_EXPR(tex2D_MutArr_sampler == nullptr);

        auto tex2D_Dyn = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(tex2D_Dyn == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, tex2D_Dyn->GetResourceDesc().Name));
        tex2D_Dyn->Set(pSRVs[0]);

        auto tex2D_Dyn_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Dyn_sampler");
        VERIFY_EXPR(tex2D_Dyn_sampler == nullptr);

        auto tex2D_DynArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_DynArr");
        VERIFY_EXPR(tex2D_DynArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(tex2D_DynArr == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, tex2D_DynArr->GetResourceDesc().Name));
        tex2D_DynArr->SetArray(pSRVs, 0, 2);

        auto tex2D_DynArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_DynArr_sampler");
        VERIFY_EXPR(tex2D_DynArr_sampler == nullptr);

        auto UniformBuff_Mut = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Mut");
        VERIFY_EXPR(UniformBuff_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Mut == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, UniformBuff_Mut->GetResourceDesc().Name));
        UniformBuff_Mut->Set(pUBs[0]);

        auto UniformBuff_Dyn = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Dyn");
        VERIFY_EXPR(UniformBuff_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Dyn == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, UniformBuff_Dyn->GetResourceDesc().Name));
        UniformBuff_Dyn->Set(pUBs[0]);

        auto Buffer_Mut = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_Mut");
        VERIFY_EXPR(Buffer_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(Buffer_Mut == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, Buffer_Mut->GetResourceDesc().Name));
        Buffer_Mut->Set(pFormattedBuffSRV);

        auto Buffer_MutArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_MutArr");
        VERIFY_EXPR(Buffer_MutArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(Buffer_MutArr == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, Buffer_MutArr->GetResourceDesc().Name));
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto Buffer_Dyn = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_Dyn");
        VERIFY_EXPR(Buffer_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(Buffer_Dyn == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, Buffer_Dyn->GetResourceDesc().Name));
        Buffer_Dyn->Set(pFormattedBuffSRV);

        auto Buffer_DynArr = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_Buffer_DynArr");
        VERIFY_EXPR(Buffer_DynArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(Buffer_DynArr == pSRB->GetVariableByName(SHADER_TYPE_VERTEX, Buffer_DynArr->GetResourceDesc().Name));
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto tex2D_Static = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static == nullptr);
        auto UniformBuff_Stat = pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat == nullptr);
    }



    {
        auto tex2D_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(tex2D_Mut == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, tex2D_Mut->GetResourceDesc().Name));
        tex2D_Mut->Set(pRWTexSRVs[4]);

        auto tex2D_Mut_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Mut_sampler");
        VERIFY_EXPR(tex2D_Mut_sampler == nullptr);

        auto tex2D_MutArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_MutArr");
        VERIFY_EXPR(tex2D_MutArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(tex2D_MutArr == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, tex2D_MutArr->GetResourceDesc().Name));
        tex2D_MutArr->SetArray(pRWTexSRVs+5, 0, 2);

        auto tex2D_MutArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_MutArr_sampler");
        VERIFY_EXPR(tex2D_MutArr_sampler == nullptr);

        auto tex2D_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(tex2D_Dyn == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, tex2D_Dyn->GetResourceDesc().Name));
        tex2D_Dyn->Set(pRWTexSRVs[7]);

        auto tex2D_Dyn_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn_sampler");
        VERIFY_EXPR(tex2D_Dyn_sampler == nullptr);

        auto tex2D_DynArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_DynArr");
        VERIFY_EXPR(tex2D_DynArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(tex2D_DynArr == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, tex2D_DynArr->GetResourceDesc().Name));
        tex2D_DynArr->SetArray(pSRVs, 0, 2);

        auto tex2D_DynArr_sampler = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_DynArr_sampler");
        VERIFY_EXPR(tex2D_DynArr_sampler == nullptr);


        auto UniformBuff_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Mut");
        VERIFY_EXPR(UniformBuff_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Mut == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, UniformBuff_Mut->GetResourceDesc().Name));
        UniformBuff_Mut->Set(pUBs[0]);

        auto UniformBuff_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Dyn");
        VERIFY_EXPR(UniformBuff_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(UniformBuff_Dyn == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, UniformBuff_Dyn->GetResourceDesc().Name));
        UniformBuff_Dyn->Set(pUBs[0]);

        auto Buffer_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Mut");
        VERIFY_EXPR(Buffer_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(Buffer_Mut == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, Buffer_Mut->GetResourceDesc().Name));
        Buffer_Mut->Set(spRawBuffSRVs[1]);

        auto Buffer_MutArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_MutArr");
        VERIFY_EXPR(Buffer_MutArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(Buffer_MutArr == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, Buffer_MutArr->GetResourceDesc().Name));
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto Buffer_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Dyn");
        VERIFY_EXPR(Buffer_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(Buffer_Dyn == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, Buffer_Dyn->GetResourceDesc().Name));
        Buffer_Dyn->Set(pFormattedBuffSRVs[3]);

        auto Buffer_DynArr = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_DynArr");
        VERIFY_EXPR(Buffer_DynArr->GetResourceDesc().ArraySize == 2);
        VERIFY_EXPR(Buffer_DynArr == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, Buffer_DynArr->GetResourceDesc().Name));
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto rwtex2D_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Mut");
        VERIFY_EXPR(rwtex2D_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(rwtex2D_Mut == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, rwtex2D_Mut->GetResourceDesc().Name));
        rwtex2D_Mut->Set(pTexUAVs[2]);

        auto rwtex2D_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn");
        VERIFY_EXPR(rwtex2D_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(rwtex2D_Dyn == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, rwtex2D_Dyn->GetResourceDesc().Name));
        rwtex2D_Dyn->Set(pTexUAVs[3]);

        auto rwBuff_Mut = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Mut");
        VERIFY_EXPR(rwBuff_Mut->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(rwBuff_Mut == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, rwBuff_Mut->GetResourceDesc().Name));
        rwBuff_Mut->Set(pFormattedBuffUAV[1]);

        auto rwBuff_Dyn = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Dyn");
        VERIFY_EXPR(rwBuff_Dyn->GetResourceDesc().ArraySize == 1);
        VERIFY_EXPR(rwBuff_Dyn == pSRB->GetVariableByName(SHADER_TYPE_PIXEL, rwBuff_Dyn->GetResourceDesc().Name));
        rwBuff_Dyn->Set(pFormattedBuffUAV[2]);
        
        auto tex2D_Static = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static == nullptr);
        auto UniformBuff_Stat = pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat == nullptr);
    }



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


    pContext->SetPipelineState(pTestPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);

    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn")->Set(pTexUAVs[7]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D_Dyn")->Set(pRWTexSRVs[3]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_rwBuff_Dyn")->Set(pFormattedBuffUAV[3]);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer_Dyn")->Set(pFormattedBuffSRVs[2]);

    LOG_INFO_MESSAGE("No worries about 3 errors below: attempting to access variables from inactive shader stage");
    auto pNonExistingVar = pSRB->GetVariableByName(SHADER_TYPE_GEOMETRY, "g_NonExistingVar");
    VERIFY_EXPR(pNonExistingVar == nullptr);
    pNonExistingVar = pSRB->GetVariableByIndex(SHADER_TYPE_GEOMETRY, 4);
    VERIFY_EXPR(pNonExistingVar == nullptr);
    VERIFY_EXPR(pSRB->GetVariableCount(SHADER_TYPE_GEOMETRY) == 0);

    m_pDeviceContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    float Zero[4] = {};
    m_pDeviceContext->ClearRenderTarget(pRTV, Zero, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    m_pDeviceContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->Draw(DrawAttrs);

    m_pDeviceContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    SetStatus(TestResult::Succeeded);
}
