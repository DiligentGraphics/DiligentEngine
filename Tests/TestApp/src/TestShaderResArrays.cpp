/*     Copyright 2015-2017 Egor Yusov
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
#include "TestShaderResArrays.h"
#include "GraphicsUtilities.h"
#include "BasicShaderSourceStreamFactory.h"
#include "TestTexturing.h"

using namespace Diligent;

TestShaderResArrays::TestShaderResArrays(IRenderDevice *pDevice, IDeviceContext *pDeviceContext, bool bUseOpenGL, float fMinXCoord, float fMinYCoord, float fXExtent, float fYExtent) :
    UnitTestBase("Shader resource array test")
{
    m_pRenderDevice = pDevice;
    m_pDeviceContext = pDeviceContext;
    
    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory;
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.Desc.TargetProfile = SHADER_PROFILE_DX_5_0;

    RefCntAutoPtr<Diligent::IShader> pVS, pPS;
    {
        CreationAttrs.FilePath = "Shaders\\ShaderResArrayTest.vsh";
        CreationAttrs.Desc.ShaderType =  SHADER_TYPE_VERTEX;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        m_pRenderDevice->CreateShader( CreationAttrs, &pVS );
    }

    {
        CreationAttrs.FilePath = "Shaders\\ShaderResArrayTest.psh";
        
        StaticSamplerDesc StaticSampler;
        StaticSampler.Desc.MinFilter = FILTER_TYPE_LINEAR;
        StaticSampler.Desc.MagFilter = FILTER_TYPE_LINEAR;
        StaticSampler.Desc.MipFilter = FILTER_TYPE_LINEAR;
        StaticSampler.TextureName = "g_tex2DTest";
        CreationAttrs.Desc.NumStaticSamplers = 1;
        CreationAttrs.Desc.StaticSamplers = &StaticSampler;
        CreationAttrs.Desc.ShaderType =  SHADER_TYPE_PIXEL;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderVariableDesc Vars[] = 
        {
            {"g_tex2DTest", SHADER_VARIABLE_TYPE_MUTABLE},
            {"g_tex2DTest2", SHADER_VARIABLE_TYPE_STATIC},
            {"g_tex2D", SHADER_VARIABLE_TYPE_DYNAMIC}
        };
        CreationAttrs.Desc.VariableDesc = Vars;
        CreationAttrs.Desc.NumVariables = _countof(Vars);

        m_pRenderDevice->CreateShader( CreationAttrs, &pPS );
    }

    PipelineStateDesc PSODesc;
    PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    PSODesc.GraphicsPipeline.BlendDesc.IndependentBlendEnable = False;
    PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = False;
    PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM_SRGB;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;

    LayoutElement Elems[] =
    {
        LayoutElement( 0, 0, 3, Diligent::VT_FLOAT32, false, 0 ),
        LayoutElement( 1, 0, 2, Diligent::VT_FLOAT32, false, sizeof( float ) * 3 )
    };
    PSODesc.GraphicsPipeline.InputLayout.LayoutElements = Elems;
    PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof( Elems );
    pDevice->CreatePipelineState(PSODesc, &m_pPSO);
    m_pPSO->CreateShaderResourceBinding(&m_pSRB);
    
    float Vertices[] = 
    {
         0,  0, 0,   0,1,
         0,  1, 0,   0,0,
         1,  0, 0,   1,1,
         1,  1, 0,   1,0
    };
    for(int v=0; v < 4; ++v)
    {
        Vertices[v*5+0] = Vertices[v*5+0] * fXExtent + fMinXCoord;
        Vertices[v*5+1] = Vertices[v*5+1] * fYExtent + fMinYCoord;
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = sizeof(Vertices);
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_STATIC;
        Diligent::BufferData BuffData;
        BuffData.pData = Vertices;
        BuffData.DataSize = BuffDesc.uiSizeInBytes;
        m_pRenderDevice->CreateBuffer(BuffDesc, BuffData, &m_pVertexBuff);
    }

    RefCntAutoPtr<ISampler> pSampler;
    SamplerDesc SamDesc;
    pDevice->CreateSampler(SamDesc, &pSampler);
    for(auto t=0; t < _countof(m_pTextures); ++t)
    {
        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Width = 256;
        TexDesc.Height = 256;
        TexDesc.MipLevels = 8;
        TexDesc.Usage = USAGE_STATIC;
        TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.Name = "Test Texture";

        std::vector<Uint8> Data;
        std::vector<TextureSubResData> SubResouces;
        float ColorOffset[4] = {(float)t*0.13f, (float)t*0.21f, (float)t*0.29f, 0};
        TestTexturing::GenerateTextureData(m_pRenderDevice, Data, SubResouces, TexDesc, ColorOffset);
        TextureData TexData;
        TexData.pSubResources = SubResouces.data();
        TexData.NumSubresources = (Uint32)SubResouces.size();

        m_pRenderDevice->CreateTexture( TexDesc, TexData, &m_pTextures[t] );
        m_pTextures[t]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(pSampler);
    }

    ResourceMappingEntry ResMpEntries [] =
    {
        ResourceMappingEntry("g_tex2DTest", m_pTextures[0]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 0),
        ResourceMappingEntry("g_tex2DTest", m_pTextures[1]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 1),
        ResourceMappingEntry("g_tex2DTest", m_pTextures[2]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 2),
        ResourceMappingEntry("g_tex2DTest2", m_pTextures[5]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 0),
        ResourceMappingEntry("g_tex2D", m_pTextures[6]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 0),
        ResourceMappingEntry()
    };
    
    ResourceMappingDesc ResMappingDesc;
    ResMappingDesc.pEntries = ResMpEntries;
    RefCntAutoPtr<IResourceMapping> pResMapping;
    m_pRenderDevice->CreateResourceMapping(ResMappingDesc, &pResMapping);

    //pVS->BindResources(m_pResourceMapping, 0);
    IDeviceObject *ppSRVs[] = {m_pTextures[3]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)};
    pPS->BindResources(pResMapping, BIND_SHADER_RESOURCES_RESET_BINDINGS | BIND_SHADER_RESOURCES_UPDATE_UNRESOLVED);
    pPS->GetShaderVariable("g_tex2DTest2")->SetArray( ppSRVs, 1, 1);

    m_pSRB->BindResources(SHADER_TYPE_PIXEL, pResMapping, BIND_SHADER_RESOURCES_RESET_BINDINGS | BIND_SHADER_RESOURCES_UPDATE_UNRESOLVED);
    ppSRVs[0] = m_pTextures[4]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2DTest")->SetArray(ppSRVs, 3, 1);
}
    
void TestShaderResArrays::Draw()
{
    m_pDeviceContext->SetPipelineState(m_pPSO);
    IDeviceObject *ppSRVs[] = {m_pTextures[7]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)};
    m_pSRB->GetVariable(SHADER_TYPE_PIXEL, "g_tex2D")->SetArray(ppSRVs, 1, 1);

    m_pDeviceContext->CommitShaderResources(m_pSRB, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
    
    IBuffer *pBuffs[] = {m_pVertexBuff};
    Uint32 Strides[] = {sizeof(float)*5};
    Uint32 Offsets[] = {0};
    m_pDeviceContext->SetVertexBuffers( 0, 1, pBuffs, Strides, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET );

    Diligent::DrawAttribs DrawAttrs;
    DrawAttrs.Topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    DrawAttrs.NumVertices = 4; // Draw quad
    m_pDeviceContext->Draw( DrawAttrs );
    
    SetStatus(TestResult::Succeeded);
}
