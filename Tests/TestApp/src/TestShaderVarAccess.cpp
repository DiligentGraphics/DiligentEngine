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
#include "BasicShaderSourceStreamFactory.h"

using namespace Diligent;

TestShaderVarAccess::TestShaderVarAccess( IRenderDevice *pDevice, IDeviceContext *pContext, Diligent::ISwapChain* pSwapChain ) :
    UnitTestBase("Shader variable access test"),
    m_pDeviceContext(pContext)
{
    if( pDevice->GetDeviceCaps().DevType == DeviceType::OpenGLES )
        return;

    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory("Shaders");
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
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
        pDevice->CreateTexture(TexDesc, TextureData{}, &(pTex[i]));
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
        pDevice->CreateTexture(TexDesc, TextureData{}, &(pRWTex[i]));
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
    pDevice->CreateTexture(TexDesc, TextureData{}, &pRenderTarget);
    auto *pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);

    TexDesc.Format = pSwapChain->GetDesc().DepthBufferFormat;
    TexDesc.BindFlags = BIND_DEPTH_STENCIL;
    RefCntAutoPtr<ITexture> pDepthTex;
    pDevice->CreateTexture(TexDesc, TextureData{}, &pDepthTex);
    auto *pDSV = pDepthTex->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    BufferDesc BuffDesc;
    BuffDesc.uiSizeInBytes = 1024;
    BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
    RefCntAutoPtr<IBuffer> pUniformBuffs[2];
    IDeviceObject *pUBs[2];
    for (int i = 0; i < _countof(pUniformBuffs); ++i)
    {
        pDevice->CreateBuffer(BuffDesc, BufferData{}, &(pUniformBuffs[i]));
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
        pDevice->CreateBuffer(TxlBuffDesc, BufferData{}, &pFormattedBuff0);
        
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
            pDevice->CreateBuffer(TxlBuffDesc, BufferData{}, &(pFormattedBuff[i]));
            
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
            pDevice->CreateBuffer(TxlBuffDesc, BufferData{}, &(pRawBuff[i]));
            spRawBuffUAV[i]  = pRawBuff[i]->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
            spRawBuffSRVs[i] = pRawBuff[i]->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE);
        }
    }
 
    std::vector<ShaderVariableDesc> VarDesc = 
    {
        { "g_tex2D_Static", SHADER_VARIABLE_TYPE_STATIC },
        { "g_tex2D_StaticArr", SHADER_VARIABLE_TYPE_STATIC },
        { "g_tex2D_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
        { "g_tex2D_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
        { "g_tex2D_MutArr", SHADER_VARIABLE_TYPE_MUTABLE },
        { "g_tex2D_DynArr", SHADER_VARIABLE_TYPE_DYNAMIC },
        { "UniformBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
        { "UniformBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
        { "g_Buffer_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
        { "g_Buffer_MutArr", SHADER_VARIABLE_TYPE_MUTABLE },
        { "g_Buffer_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
        { "g_Buffer_DynArr", SHADER_VARIABLE_TYPE_DYNAMIC },
    };
    
    StaticSamplerDesc StaticSamplers[] = 
    {
        {"g_tex2D_Static", SamplerDesc{}},
        {"g_tex2D_StaticArr", SamplerDesc{}},
        {"g_tex2D_Mut", SamplerDesc{}},
        {"g_tex2D_MutArr", SamplerDesc{}},
        {"g_tex2D_Dyn", SamplerDesc{}},
        {"g_tex2D_DynArr", SamplerDesc{}}
    };
    RefCntAutoPtr<IShader> pVS;
    {
        CreationAttrs.Desc.Name = "Shader variable access test VS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_DEFAULT;
        CreationAttrs.FilePath = pDevice->GetDeviceCaps().IsD3DDevice() ? "Shaders\\ShaderVarAccessTestDX.vsh" : "Shaders\\ShaderVarAccessTestGL.vsh";
        
        CreationAttrs.Desc.VariableDesc = VarDesc.data();
        CreationAttrs.Desc.NumVariables = static_cast<Uint32>(VarDesc.size());
        CreationAttrs.Desc.NumStaticSamplers = _countof(StaticSamplers);
        CreationAttrs.Desc.StaticSamplers = StaticSamplers;

        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS);
        VERIFY_EXPR(pVS->GetVariableCount() == 6);

        
        auto tex2D_Static = pVS->GetShaderVariable("g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static->GetArraySize() == 1);
        VERIFY_EXPR(tex2D_Static == pVS->GetShaderVariable(tex2D_Static->GetName()));
        tex2D_Static->Set(pSRVs[0]);

        auto tex2D_StaticArr = pVS->GetShaderVariable("g_tex2D_StaticArr");
        VERIFY_EXPR(tex2D_StaticArr->GetArraySize() == 2);
        VERIFY_EXPR(tex2D_StaticArr == pVS->GetShaderVariable(tex2D_StaticArr->GetName()));
        tex2D_StaticArr->SetArray(pSRVs, 0, 2);

        auto UniformBuff_Stat = pVS->GetShaderVariable("UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Stat == pVS->GetShaderVariable(UniformBuff_Stat->GetName()));
        UniformBuff_Stat->Set(pUBs[0]);

        auto UniformBuff_Stat2 = pVS->GetShaderVariable("UniformBuff_Stat2");
        VERIFY_EXPR(UniformBuff_Stat2->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Stat2 == pVS->GetShaderVariable(UniformBuff_Stat2->GetName()));
        UniformBuff_Stat2->Set(pUBs[0]);

        auto Buffer_Static = pVS->GetShaderVariable("g_Buffer_Static");
        VERIFY_EXPR(Buffer_Static->GetArraySize() == 1);
        VERIFY_EXPR(Buffer_Static == pVS->GetShaderVariable(Buffer_Static->GetName()));
        Buffer_Static->Set(pFormattedBuffSRV);

        auto Buffer_StaticArr = pVS->GetShaderVariable("g_Buffer_StaticArr");
        VERIFY_EXPR(Buffer_StaticArr->GetArraySize() == 2);
        VERIFY_EXPR(Buffer_StaticArr == pVS->GetShaderVariable(Buffer_StaticArr->GetName()));
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,0,1);
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,1,1);
        


        auto tex2D_Mut = pVS->GetShaderVariable("g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut == nullptr);
        auto tex2D_Dyn = pVS->GetShaderVariable("g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn == nullptr);
    }

    {
        auto NumVSVars = pVS->GetVariableCount();
        for(Uint32 v=0; v < NumVSVars; ++v)
        {
            auto pVar = pVS->GetShaderVariable(v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_VARIABLE_TYPE_STATIC);
            auto pVar2 = pVS->GetShaderVariable(pVar->GetName());
            VERIFY_EXPR(pVar == pVar2);
        }
    }

    VarDesc.emplace_back( "g_rwtex2D_Mut", SHADER_VARIABLE_TYPE_MUTABLE );
    VarDesc.emplace_back( "g_rwtex2D_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC );
    VarDesc.emplace_back( "g_rwBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE );
    VarDesc.emplace_back( "g_rwBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC );

    RefCntAutoPtr<IShader> pPS;
    {
        CreationAttrs.Desc.Name = "Shader variable access test PS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_DEFAULT;
        CreationAttrs.FilePath = pDevice->GetDeviceCaps().IsD3DDevice() ? "Shaders\\ShaderVarAccessTestDX.psh" : "Shaders\\ShaderVarAccessTestGL.psh";
        CreationAttrs.Desc.VariableDesc = VarDesc.data();
        CreationAttrs.Desc.NumVariables = static_cast<Uint32>(VarDesc.size());

        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS);
        VERIFY_EXPR(pPS->GetVariableCount() == 9);

        auto tex2D_Static = pPS->GetShaderVariable("g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static->GetArraySize() == 1);
        VERIFY_EXPR(tex2D_Static == pPS->GetShaderVariable(tex2D_Static->GetName()));
        tex2D_Static->Set(pSRVs[0]);

        auto tex2D_StaticArr = pPS->GetShaderVariable("g_tex2D_StaticArr");
        VERIFY_EXPR(tex2D_StaticArr->GetArraySize() == 2);
        VERIFY_EXPR(tex2D_StaticArr == pPS->GetShaderVariable(tex2D_StaticArr->GetName()));
        tex2D_StaticArr->SetArray(pSRVs, 0, 2);

        auto UniformBuff_Stat = pPS->GetShaderVariable("UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Stat == pPS->GetShaderVariable(UniformBuff_Stat->GetName()));
        UniformBuff_Stat->Set(pUBs[0]);

        auto UniformBuff_Stat2 = pPS->GetShaderVariable("UniformBuff_Stat2");
        VERIFY_EXPR(UniformBuff_Stat2->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Stat2 == pPS->GetShaderVariable(UniformBuff_Stat2->GetName()));
        UniformBuff_Stat2->Set(pUBs[0]);

        auto Buffer_Static = pPS->GetShaderVariable("g_Buffer_Static");
        VERIFY_EXPR(Buffer_Static->GetArraySize() == 1);
        VERIFY_EXPR(Buffer_Static == pPS->GetShaderVariable(Buffer_Static->GetName()));
        Buffer_Static->Set(pFormattedBuffSRV);

        auto Buffer_StaticArr = pPS->GetShaderVariable("g_Buffer_StaticArr");
        VERIFY_EXPR(Buffer_StaticArr->GetArraySize() == 2);
        VERIFY_EXPR(Buffer_StaticArr == pPS->GetShaderVariable(Buffer_StaticArr->GetName()));
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,0,1);
        Buffer_StaticArr->SetArray(&pFormattedBuffSRV,1,1);
        

        auto rwtex2D_Static = pPS->GetShaderVariable("g_rwtex2D_Static");
        VERIFY_EXPR(rwtex2D_Static->GetArraySize() == 1);
        VERIFY_EXPR(rwtex2D_Static == pPS->GetShaderVariable(rwtex2D_Static->GetName()));
        rwtex2D_Static->Set(pTexUAVs[0]);

        auto rwtex2D_Static2 = pPS->GetShaderVariable("g_rwtex2D_Static2");
        VERIFY_EXPR(rwtex2D_Static2->GetArraySize() == 1);
        VERIFY_EXPR(rwtex2D_Static2 == pPS->GetShaderVariable(rwtex2D_Static2->GetName()));
        rwtex2D_Static2->Set(pTexUAVs[1]);

        auto rwBuff_Static = pPS->GetShaderVariable("g_rwBuff_Static");
        VERIFY_EXPR(rwBuff_Static->GetArraySize() == 1);
        VERIFY_EXPR(rwBuff_Static == pPS->GetShaderVariable(rwBuff_Static->GetName()));
        rwBuff_Static->Set(spRawBuffUAV[0]);


        auto tex2D_Mut = pPS->GetShaderVariable("g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut == nullptr);
        auto tex2D_Dyn = pPS->GetShaderVariable("g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn == nullptr);
    }

    {
        auto NumPSVars = pPS->GetVariableCount();
        for(Uint32 v=0; v < NumPSVars; ++v)
        {
            auto pVar = pPS->GetShaderVariable(v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_VARIABLE_TYPE_STATIC);
            auto pVar2 = pPS->GetShaderVariable(pVar->GetName());
            VERIFY_EXPR(pVar == pVar2);
        }
    }

    PipelineStateDesc PSODesc;
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

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pTestPSO->CreateShaderResourceBinding(&pSRB, true);

    {
        auto tex2D_Mut = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut->GetArraySize() == 1);
        VERIFY_EXPR(tex2D_Mut == pSRB->GetVariable(SHADER_TYPE_VERTEX, tex2D_Mut->GetName()));
        tex2D_Mut->Set(pSRVs[0]);

        auto tex2D_MutArr = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_MutArr");
        VERIFY_EXPR(tex2D_MutArr->GetArraySize() == 2);
        VERIFY_EXPR(tex2D_MutArr == pSRB->GetVariable(SHADER_TYPE_VERTEX, tex2D_MutArr->GetName()));
        tex2D_MutArr->SetArray(pSRVs, 0, 2);

        auto tex2D_Dyn = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(tex2D_Dyn == pSRB->GetVariable(SHADER_TYPE_VERTEX, tex2D_Dyn->GetName()));
        tex2D_Dyn->Set(pSRVs[0]);

        auto tex2D_DynArr = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_DynArr");
        VERIFY_EXPR(tex2D_DynArr->GetArraySize() == 2);
        VERIFY_EXPR(tex2D_DynArr == pSRB->GetVariable(SHADER_TYPE_VERTEX, tex2D_DynArr->GetName()));
        tex2D_DynArr->SetArray(pSRVs, 0, 2);

        auto UniformBuff_Mut = pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Mut");
        VERIFY_EXPR(UniformBuff_Mut->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Mut == pSRB->GetVariable(SHADER_TYPE_VERTEX, UniformBuff_Mut->GetName()));
        UniformBuff_Mut->Set(pUBs[0]);

        auto UniformBuff_Dyn = pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Dyn");
        VERIFY_EXPR(UniformBuff_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Dyn == pSRB->GetVariable(SHADER_TYPE_VERTEX, UniformBuff_Dyn->GetName()));
        UniformBuff_Dyn->Set(pUBs[0]);

        auto Buffer_Mut = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_Buffer_Mut");
        VERIFY_EXPR(Buffer_Mut->GetArraySize() == 1);
        VERIFY_EXPR(Buffer_Mut == pSRB->GetVariable(SHADER_TYPE_VERTEX, Buffer_Mut->GetName()));
        Buffer_Mut->Set(pFormattedBuffSRV);

        auto Buffer_MutArr = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_Buffer_MutArr");
        VERIFY_EXPR(Buffer_MutArr->GetArraySize() == 2);
        VERIFY_EXPR(Buffer_MutArr == pSRB->GetVariable(SHADER_TYPE_VERTEX, Buffer_MutArr->GetName()));
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto Buffer_Dyn = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_Buffer_Dyn");
        VERIFY_EXPR(Buffer_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(Buffer_Dyn == pSRB->GetVariable(SHADER_TYPE_VERTEX, Buffer_Dyn->GetName()));
        Buffer_Dyn->Set(pFormattedBuffSRV);

        auto Buffer_DynArr = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_Buffer_DynArr");
        VERIFY_EXPR(Buffer_DynArr->GetArraySize() == 2);
        VERIFY_EXPR(Buffer_DynArr == pSRB->GetVariable(SHADER_TYPE_VERTEX, Buffer_DynArr->GetName()));
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto tex2D_Static = pSRB->GetVariable(SHADER_TYPE_VERTEX, "g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static == nullptr);
        auto UniformBuff_Stat = pSRB->GetVariable(SHADER_TYPE_VERTEX, "UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat == nullptr);
    }



    {
        auto tex2D_Mut = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Mut");
        VERIFY_EXPR(tex2D_Mut->GetArraySize() == 1);
        VERIFY_EXPR(tex2D_Mut == pSRB->GetVariable(SHADER_TYPE_PIXEL, tex2D_Mut->GetName()));
        tex2D_Mut->Set(pRWTexSRVs[4]);

        auto tex2D_MutArr = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_MutArr");
        VERIFY_EXPR(tex2D_MutArr->GetArraySize() == 2);
        VERIFY_EXPR(tex2D_MutArr == pSRB->GetVariable(SHADER_TYPE_PIXEL, tex2D_MutArr->GetName()));
        tex2D_MutArr->SetArray(pRWTexSRVs+5, 0, 2);

        auto tex2D_Dyn = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Dyn");
        VERIFY_EXPR(tex2D_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(tex2D_Dyn == pSRB->GetVariable(SHADER_TYPE_PIXEL, tex2D_Dyn->GetName()));
        tex2D_Dyn->Set(pRWTexSRVs[7]);

        auto tex2D_DynArr = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_DynArr");
        VERIFY_EXPR(tex2D_DynArr->GetArraySize() == 2);
        VERIFY_EXPR(tex2D_DynArr == pSRB->GetVariable(SHADER_TYPE_PIXEL, tex2D_DynArr->GetName()));
        tex2D_DynArr->SetArray(pSRVs, 0, 2);

        auto UniformBuff_Mut = pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Mut");
        VERIFY_EXPR(UniformBuff_Mut->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Mut == pSRB->GetVariable(SHADER_TYPE_PIXEL, UniformBuff_Mut->GetName()));
        UniformBuff_Mut->Set(pUBs[0]);

        auto UniformBuff_Dyn = pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Dyn");
        VERIFY_EXPR(UniformBuff_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(UniformBuff_Dyn == pSRB->GetVariable(SHADER_TYPE_PIXEL, UniformBuff_Dyn->GetName()));
        UniformBuff_Dyn->Set(pUBs[0]);

        auto Buffer_Mut = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_Buffer_Mut");
        VERIFY_EXPR(Buffer_Mut->GetArraySize() == 1);
        VERIFY_EXPR(Buffer_Mut == pSRB->GetVariable(SHADER_TYPE_PIXEL, Buffer_Mut->GetName()));
        Buffer_Mut->Set(spRawBuffSRVs[1]);

        auto Buffer_MutArr = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_Buffer_MutArr");
        VERIFY_EXPR(Buffer_MutArr->GetArraySize() == 2);
        VERIFY_EXPR(Buffer_MutArr == pSRB->GetVariable(SHADER_TYPE_PIXEL, Buffer_MutArr->GetName()));
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_MutArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto Buffer_Dyn = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_Buffer_Dyn");
        VERIFY_EXPR(Buffer_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(Buffer_Dyn == pSRB->GetVariable(SHADER_TYPE_PIXEL, Buffer_Dyn->GetName()));
        Buffer_Dyn->Set(pFormattedBuffSRVs[3]);

        auto Buffer_DynArr = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_Buffer_DynArr");
        VERIFY_EXPR(Buffer_DynArr->GetArraySize() == 2);
        VERIFY_EXPR(Buffer_DynArr == pSRB->GetVariable(SHADER_TYPE_PIXEL, Buffer_DynArr->GetName()));
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 0, 1);
        Buffer_DynArr->SetArray(&pFormattedBuffSRV, 1, 1);

        auto rwtex2D_Mut = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_rwtex2D_Mut");
        VERIFY_EXPR(rwtex2D_Mut->GetArraySize() == 1);
        VERIFY_EXPR(rwtex2D_Mut == pSRB->GetVariable(SHADER_TYPE_PIXEL, rwtex2D_Mut->GetName()));
        rwtex2D_Mut->Set(pTexUAVs[2]);

        auto rwtex2D_Dyn = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn");
        VERIFY_EXPR(rwtex2D_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(rwtex2D_Dyn == pSRB->GetVariable(SHADER_TYPE_PIXEL, rwtex2D_Dyn->GetName()));
        rwtex2D_Dyn->Set(pTexUAVs[3]);

        auto rwBuff_Mut = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_rwBuff_Mut");
        VERIFY_EXPR(rwBuff_Mut->GetArraySize() == 1);
        VERIFY_EXPR(rwBuff_Mut == pSRB->GetVariable(SHADER_TYPE_PIXEL, rwBuff_Mut->GetName()));
        rwBuff_Mut->Set(pFormattedBuffUAV[1]);

        auto rwBuff_Dyn = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_rwBuff_Dyn");
        VERIFY_EXPR(rwBuff_Dyn->GetArraySize() == 1);
        VERIFY_EXPR(rwBuff_Dyn == pSRB->GetVariable(SHADER_TYPE_PIXEL, rwBuff_Dyn->GetName()));
        rwBuff_Dyn->Set(pFormattedBuffUAV[2]);
        
        auto tex2D_Static = pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Static");
        VERIFY_EXPR(tex2D_Static == nullptr);
        auto UniformBuff_Stat = pSRB->GetVariable(SHADER_TYPE_PIXEL, "UniformBuff_Stat");
        VERIFY_EXPR(UniformBuff_Stat == nullptr);
    }



    {
        auto NumVSVars = pSRB->GetVariableCount(SHADER_TYPE_VERTEX);
        for(Uint32 v=0; v < NumVSVars; ++v)
        {
            auto pVar = pSRB->GetVariable(SHADER_TYPE_VERTEX, v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_VARIABLE_TYPE_MUTABLE || pVar->GetType() == SHADER_VARIABLE_TYPE_DYNAMIC);
            auto pVar2 = pSRB->GetVariable(SHADER_TYPE_VERTEX, pVar->GetName());
            VERIFY_EXPR(pVar == pVar2);
        }
    }

    {
        auto NumPSVars = pSRB->GetVariableCount(SHADER_TYPE_PIXEL);
        for(Uint32 v=0; v < NumPSVars; ++v)
        {
            auto pVar = pSRB->GetVariable(SHADER_TYPE_PIXEL, v);
            VERIFY_EXPR(pVar->GetIndex() == v);
            VERIFY_EXPR(pVar->GetType() == SHADER_VARIABLE_TYPE_MUTABLE || pVar->GetType() == SHADER_VARIABLE_TYPE_DYNAMIC);
            auto pVar2 = pSRB->GetVariable(SHADER_TYPE_PIXEL, pVar->GetName());
            VERIFY_EXPR(pVar == pVar2);
        }
    }


    pContext->SetPipelineState(pTestPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_STATES);
    pContext->Draw(DrawAttrs);

    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_rwtex2D_Dyn")->Set(pTexUAVs[7]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D_Dyn")->Set(pRWTexSRVs[3]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_rwBuff_Dyn")->Set(pFormattedBuffUAV[3]);
    pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_Buffer_Dyn")->Set(pFormattedBuffSRVs[2]);

    LOG_INFO_MESSAGE("No worries about 3 errors below: attempting to access variables from inactive shader stage");
    auto pNonExistingVar = pSRB->GetVariable(SHADER_TYPE_GEOMETRY, "g_NonExistingVar");
    VERIFY_EXPR(pNonExistingVar == nullptr);
    pNonExistingVar = pSRB->GetVariable(SHADER_TYPE_GEOMETRY, 4);
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
