/*
 *  Copyright 2019-2020 Diligent Graphics LLC
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

#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(ShaderResourceLayout, ResourceArray)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders", &pShaderSourceFactory);
    ShaderCreateInfo ShaderCI;
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.HLSLVersion                = ShaderVersion{5, 0};

    RefCntAutoPtr<IShader> pVS, pPS;
    {
        ShaderCI.Desc.Name       = "ShaderResourceArrayTest: VS";
        ShaderCI.FilePath        = "ShaderResourceArrayTest.vsh";
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_HLSL;
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    {
        ShaderCI.Desc.Name       = "ShaderResourceArrayTest: PS";
        ShaderCI.FilePath        = "ShaderResourceArrayTest.psh";
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_HLSL;
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }


    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    ImmutableSamplerDesc ImtblSampler;
    ImtblSampler.Desc.MinFilter                 = FILTER_TYPE_LINEAR;
    ImtblSampler.Desc.MagFilter                 = FILTER_TYPE_LINEAR;
    ImtblSampler.Desc.MipFilter                 = FILTER_TYPE_LINEAR;
    ImtblSampler.ShaderStages                   = SHADER_TYPE_PIXEL;
    ImtblSampler.SamplerOrTextureName           = "g_tex2DTest";
    PSODesc.ResourceLayout.NumImmutableSamplers = 1;
    PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSampler;
    // clang-format off
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_tex2DTest",  SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_tex2DTest2", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "g_tex2D",      SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on
    PSODesc.ResourceLayout.Variables                        = Vars;
    PSODesc.ResourceLayout.NumVariables                     = _countof(Vars);
    GraphicsPipeline.DepthStencilDesc.DepthEnable           = False;
    GraphicsPipeline.RasterizerDesc.CullMode                = CULL_MODE_NONE;
    GraphicsPipeline.BlendDesc.IndependentBlendEnable       = False;
    GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = False;
    GraphicsPipeline.NumRenderTargets                       = 1;

    constexpr TEXTURE_FORMAT RTVFormat = TEX_FORMAT_RGBA8_UNORM;
    constexpr TEXTURE_FORMAT DSVFormat = TEX_FORMAT_D32_FLOAT;
    GraphicsPipeline.RTVFormats[0]     = RTVFormat;
    GraphicsPipeline.DSVFormat         = DSVFormat;
    PSOCreateInfo.pVS                  = pVS;
    PSOCreateInfo.pPS                  = pPS;

    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    // clang-format off
    LayoutElement Elems[] =
    {
        LayoutElement{ 0, 0, 3, VT_FLOAT32, false, 0 },
        LayoutElement{ 1, 0, 2, VT_FLOAT32, false, sizeof( float ) * 3 }
    };
    // clang-format on
    GraphicsPipeline.InputLayout.LayoutElements = Elems;
    GraphicsPipeline.InputLayout.NumElements    = _countof(Elems);
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB);
    ASSERT_NE(pSRB, nullptr);

    // clang-format off
    float Vertices[] = 
    {
         0,  0, 0,   0,1,
         0,  1, 0,   0,0,
         1,  0, 0,   1,1,
         1,  1, 0,   1,0
    };
    // clang-format on
    constexpr float fMinXCoord = 0.4f;
    constexpr float fMinYCoord = -0.9f;
    constexpr float fXExtent   = 0.5f;
    constexpr float fYExtent   = 0.5f;
    for (int v = 0; v < 4; ++v)
    {
        Vertices[v * 5 + 0] = Vertices[v * 5 + 0] * fXExtent + fMinXCoord;
        Vertices[v * 5 + 1] = Vertices[v * 5 + 1] * fYExtent + fMinYCoord;
    }

    RefCntAutoPtr<IBuffer> pVertexBuff;
    {
        BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = sizeof(Vertices);
        BuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
        BuffDesc.Usage         = USAGE_IMMUTABLE;
        BufferData BuffData;
        BuffData.pData    = Vertices;
        BuffData.DataSize = BuffDesc.uiSizeInBytes;
        pDevice->CreateBuffer(BuffDesc, &BuffData, &pVertexBuff);
        ASSERT_NE(pVertexBuff, nullptr);
    }

    RefCntAutoPtr<ITextureView> pRTV, pDSV;
    {
        auto pRenderTarget = pEnv->CreateTexture("ShaderResourceLayout: offscreen render target", TEX_FORMAT_RGBA8_UNORM, BIND_RENDER_TARGET, 256, 256);
        ASSERT_NE(pRenderTarget, nullptr);
        pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        ASSERT_NE(pRTV, nullptr);

        auto pDepth = pEnv->CreateTexture("ShaderResourceLayout: offscreen depth", TEX_FORMAT_D32_FLOAT, BIND_DEPTH_STENCIL, 256, 256);
        ASSERT_NE(pDepth, nullptr);
        pDSV = pDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        ASSERT_NE(pDSV, nullptr);
    }

    RefCntAutoPtr<ISampler> pSampler;
    SamplerDesc             SamDesc;
    pDevice->CreateSampler(SamDesc, &pSampler);
    RefCntAutoPtr<ITexture> pTextures[8];
    for (auto t = 0; t < _countof(pTextures); ++t)
    {
        TextureDesc TexDesc;
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Width     = 256;
        TexDesc.Height    = 256;
        TexDesc.MipLevels = 8;
        TexDesc.Usage     = USAGE_IMMUTABLE;
        TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.Name      = "Test Texture";

        std::vector<Uint8>             Data(TexDesc.Width * TexDesc.Height * 4, 128);
        std::vector<TextureSubResData> SubResouces(TexDesc.MipLevels);
        for (Uint32 i = 0; i < TexDesc.MipLevels; ++i)
        {
            auto& SubResData  = SubResouces[i];
            SubResData.pData  = Data.data();
            SubResData.Stride = TexDesc.Width * 4;
        }

        //float ColorOffset[4] = {(float)t * 0.13f, (float)t * 0.21f, (float)t * 0.29f, 0};
        //TestTexturing::GenerateTextureData(pDevice, Data, SubResouces, TexDesc, ColorOffset);
        TextureData TexData;
        TexData.pSubResources   = SubResouces.data();
        TexData.NumSubresources = (Uint32)SubResouces.size();

        pDevice->CreateTexture(TexDesc, &TexData, &pTextures[t]);
        ASSERT_NE(pTextures[t], nullptr);
        pTextures[t]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(pSampler);
    }

    // clang-format off
    ResourceMappingEntry ResMpEntries [] =
    {
        ResourceMappingEntry("g_tex2DTest", pTextures[0]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 0),
        ResourceMappingEntry("g_tex2DTest", pTextures[1]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 1),
        ResourceMappingEntry("g_tex2DTest", pTextures[2]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 2),
        ResourceMappingEntry("g_tex2DTest2", pTextures[5]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 0),
        ResourceMappingEntry("g_tex2D", pTextures[6]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), 0),
        ResourceMappingEntry()
    };
    // clang-format on

    ResourceMappingDesc ResMappingDesc;
    ResMappingDesc.pEntries = ResMpEntries;
    RefCntAutoPtr<IResourceMapping> pResMapping;
    pDevice->CreateResourceMapping(ResMappingDesc, &pResMapping);

    //pVS->BindResources(m_pResourceMapping, 0);
    IDeviceObject* ppSRVs[] = {pTextures[3]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)};
    pPSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING);
    pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_tex2DTest2")->SetArray(ppSRVs, 1, 1);

    pSRB->InitializeStaticResources();
    pSRB->BindResources(SHADER_TYPE_PIXEL, pResMapping, BIND_SHADER_RESOURCES_KEEP_EXISTING | BIND_SHADER_RESOURCES_UPDATE_MUTABLE | BIND_SHADER_RESOURCES_UPDATE_DYNAMIC);
    ppSRVs[0] = pTextures[4]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DTest")->SetArray(ppSRVs, 3, 1);

    ITextureView* pRTVs[] = {pRTV};
    pContext->SetRenderTargets(1, pRTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    ppSRVs[0] = {pTextures[7]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)};
    pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2D")->SetArray(ppSRVs, 1, 1);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    IBuffer* pBuffs[]  = {pVertexBuff};
    Uint32   Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pBuffs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    // Draw a quad
    DrawAttribs DrawAttrs(4, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);
}

} // namespace
