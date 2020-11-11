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
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "TestingEnvironment.hpp"
#include "ShaderMacroHelper.hpp"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace Diligent
{
namespace Test
{
void PrintShaderResources(IShader* pShader);
}
} // namespace Diligent

namespace
{

class ShaderResourceLayoutTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv          = TestingEnvironment::GetInstance();
        auto  pRenderTarget = pEnv->CreateTexture("ShaderResourceLayoutTest: test RTV", TEX_FORMAT_RGBA8_UNORM, BIND_RENDER_TARGET, 512, 512);
        ASSERT_NE(pRenderTarget, nullptr);
        pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    }

    static void TearDownTestSuite()
    {
        TestingEnvironment::GetInstance()->Reset();
    }

    static void VerifyShaderResources(IShader*                 pShader,
                                      const ShaderResourceDesc ExpectedResources[],
                                      Uint32                   ExpectedResCount)
    {
        auto ResCount = pShader->GetResourceCount();
        EXPECT_EQ(ResCount, ExpectedResCount) << "Actual number of resources (" << ResCount << ") in shader '"
                                              << pShader->GetDesc().Name << "' does not match the expected number of resources (" << ExpectedResCount << ')';
        std::unordered_map<std::string, ShaderResourceDesc> Resources;
        for (Uint32 i = 0; i < ResCount; ++i)
        {
            ShaderResourceDesc ResDesc;
            pShader->GetResourceDesc(i, ResDesc);
            Resources.emplace(ResDesc.Name, ResDesc);
        }

        for (Uint32 i = 0; i < ExpectedResCount; ++i)
        {
            const auto& ExpectedRes = ExpectedResources[i];

            auto it = Resources.find(ExpectedRes.Name);
            if (it != Resources.end())
            {
                EXPECT_EQ(it->second.Type, ExpectedRes.Type) << "Unexpected type of resource '" << ExpectedRes.Name << '\'';
                EXPECT_EQ(it->second.ArraySize, ExpectedRes.ArraySize) << "Unexpected array size of resource '" << ExpectedRes.Name << '\'';
                Resources.erase(it);
            }
            else
            {
                ADD_FAILURE() << "Unable to find resource '" << ExpectedRes.Name << "' in shader '" << pShader->GetDesc().Name << "'";
            }
        }

        for (auto it : Resources)
        {
            ADD_FAILURE() << "Unexpected resource '" << it.second.Name << "' in shader '" << pShader->GetDesc().Name << "'";
        }
    }

    template <typename TModifyShaderCI>
    static RefCntAutoPtr<IShader> CreateShader(const char*               ShaderName,
                                               const char*               FileName,
                                               const char*               EntryPoint,
                                               SHADER_TYPE               ShaderType,
                                               SHADER_SOURCE_LANGUAGE    SrcLang,
                                               const ShaderMacro*        Macros,
                                               const ShaderResourceDesc* ExpectedResources,
                                               Uint32                    NumExpectedResources,
                                               TModifyShaderCI           ModifyShaderCI)
    {
        auto* const pEnv       = TestingEnvironment::GetInstance();
        auto* const pDevice    = pEnv->GetDevice();
        const auto& deviceCaps = pDevice->GetDeviceCaps();

        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders/ShaderResourceLayout", &pShaderSourceFactory);

        ShaderCreateInfo ShaderCI;
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
        ShaderCI.UseCombinedTextureSamplers = deviceCaps.IsGLDevice();

        ShaderCI.FilePath        = FileName;
        ShaderCI.Desc.Name       = ShaderName;
        ShaderCI.EntryPoint      = EntryPoint;
        ShaderCI.Desc.ShaderType = ShaderType;
        ShaderCI.SourceLanguage  = SrcLang;
        ShaderCI.Macros          = Macros;
        ShaderCI.ShaderCompiler  = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        ModifyShaderCI(ShaderCI);

        RefCntAutoPtr<IShader> pShader;
        pDevice->CreateShader(ShaderCI, &pShader);

        if (pShader && deviceCaps.Features.ShaderResourceQueries)
        {
            VerifyShaderResources(pShader, ExpectedResources, NumExpectedResources);
            Diligent::Test::PrintShaderResources(pShader);
        }

        return pShader;
    }

    static RefCntAutoPtr<IShader> CreateShader(const char*               ShaderName,
                                               const char*               FileName,
                                               const char*               EntryPoint,
                                               SHADER_TYPE               ShaderType,
                                               SHADER_SOURCE_LANGUAGE    SrcLang,
                                               const ShaderMacro*        Macros,
                                               const ShaderResourceDesc* ExpectedResources,
                                               Uint32                    NumExpectedResources)
    {
        return CreateShader(ShaderName,
                            FileName,
                            EntryPoint,
                            ShaderType,
                            SrcLang,
                            Macros,
                            ExpectedResources,
                            NumExpectedResources,
                            [](const ShaderCreateInfo&) {});
    }


    static void CreateGraphicsPSO(IShader*                               pVS,
                                  IShader*                               pPS,
                                  const PipelineResourceLayoutDesc&      ResourceLayout,
                                  RefCntAutoPtr<IPipelineState>&         pPSO,
                                  RefCntAutoPtr<IShaderResourceBinding>& pSRB)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        PSODesc.Name                     = "Shader resource layout test";
        PSODesc.ResourceLayout           = ResourceLayout;
        PSODesc.SRBAllocationGranularity = 16;

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.NumRenderTargets  = 1;
        GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
        GraphicsPipeline.DSVFormat         = TEX_FORMAT_UNKNOWN;

        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        if (pPSO)
            pPSO->CreateShaderResourceBinding(&pSRB, false);
    }

    static RefCntAutoPtr<IBufferView> CreateResourceBufferView(BUFFER_MODE BufferMode, BUFFER_VIEW_TYPE ViewType)
    {
        VERIFY_EXPR(ViewType == BUFFER_VIEW_SHADER_RESOURCE || ViewType == BUFFER_VIEW_UNORDERED_ACCESS);

        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        BufferDesc BuffDesc;
        BuffDesc.Name              = "Formatted buffer";
        BuffDesc.uiSizeInBytes     = 256;
        BuffDesc.BindFlags         = ViewType == BUFFER_VIEW_SHADER_RESOURCE ? BIND_SHADER_RESOURCE : BIND_UNORDERED_ACCESS;
        BuffDesc.Usage             = USAGE_DEFAULT;
        BuffDesc.ElementByteStride = 16;
        BuffDesc.Mode              = BufferMode;
        RefCntAutoPtr<IBuffer>     pBuffer;
        RefCntAutoPtr<IBufferView> pBufferView;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
        if (!pBuffer)
        {
            ADD_FAILURE() << "Unable to create buffer " << BuffDesc;
            return pBufferView;
        }

        if (BufferMode == BUFFER_MODE_FORMATTED)
        {
            BufferViewDesc BuffViewDesc;
            BuffViewDesc.Name                 = "Formatted buffer SRV";
            BuffViewDesc.ViewType             = ViewType;
            BuffViewDesc.Format.ValueType     = VT_FLOAT32;
            BuffViewDesc.Format.NumComponents = 4;
            BuffViewDesc.Format.IsNormalized  = false;
            pBuffer->CreateView(BuffViewDesc, &pBufferView);
        }
        else
        {
            pBufferView = pBuffer->GetDefaultView(ViewType);
        }

        return pBufferView;
    }

    static void CreateComputePSO(IShader*                               pCS,
                                 const PipelineResourceLayoutDesc&      ResourceLayout,
                                 RefCntAutoPtr<IPipelineState>&         pPSO,
                                 RefCntAutoPtr<IShaderResourceBinding>& pSRB)
    {
        ComputePipelineStateCreateInfo PSOCreateInfo;

        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        PSODesc.Name           = "Shader resource layout test";
        PSODesc.PipelineType   = PIPELINE_TYPE_COMPUTE;
        PSODesc.ResourceLayout = ResourceLayout;
        PSOCreateInfo.pCS      = pCS;

        pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
        if (pPSO)
            pPSO->CreateShaderResourceBinding(&pSRB, false);
    }

    void TestTexturesAndImtblSamplers(bool TestImtblSamplers);
    void TestStructuredOrFormattedBuffer(bool IsFormatted);
    void TestRWStructuredOrFormattedBuffer(bool IsFormatted);

    static RefCntAutoPtr<ITextureView> pRTV;
};

RefCntAutoPtr<ITextureView> ShaderResourceLayoutTest::pRTV;

#define SET_STATIC_VAR(PSO, ShaderFlags, VarName, SetMethod, ...)                                \
    do                                                                                           \
    {                                                                                            \
        auto pStaticVar = PSO->GetStaticVariableByName(ShaderFlags, VarName);                    \
        EXPECT_NE(pStaticVar, nullptr) << "Unable to find static variable '" << VarName << '\''; \
        if (pStaticVar != nullptr)                                                               \
            pStaticVar->SetMethod(__VA_ARGS__);                                                  \
    } while (false)


#define SET_SRB_VAR(SRB, ShaderFlags, VarName, SetMethod, ...)                          \
    do                                                                                  \
    {                                                                                   \
        auto pVar = SRB->GetVariableByName(ShaderFlags, VarName);                       \
        EXPECT_NE(pVar, nullptr) << "Unable to find SRB variable '" << VarName << '\''; \
        if (pVar != nullptr)                                                            \
            pVar->SetMethod(__VA_ARGS__);                                               \
    } while (false)


void ShaderResourceLayoutTest::TestTexturesAndImtblSamplers(bool TestImtblSamplers)
{
    TestingEnvironment::ScopedReset AutoResetEnvironment;

    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    static constexpr Uint32 StaticTexArraySize  = 2;
    static constexpr Uint32 MutableTexArraySize = 4;
    static constexpr Uint32 DynamicTexArraySize = 3;
    ShaderMacroHelper       Macros;
    Macros.AddShaderMacro("STATIC_TEX_ARRAY_SIZE", static_cast<int>(StaticTexArraySize));
    Macros.AddShaderMacro("MUTABLE_TEX_ARRAY_SIZE", static_cast<int>(MutableTexArraySize));
    Macros.AddShaderMacro("DYNAMIC_TEX_ARRAY_SIZE", static_cast<int>(DynamicTexArraySize));

    // clang-format off
    std::vector<ShaderResourceDesc> Resources = 
    {
        ShaderResourceDesc{"g_Tex2D_Static",      SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2D_Mut",         SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2D_Dyn",         SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
        ShaderResourceDesc{"g_Tex2DArr_Static",   SHADER_RESOURCE_TYPE_TEXTURE_SRV, StaticTexArraySize},
        ShaderResourceDesc{"g_Tex2DArr_Mut",      SHADER_RESOURCE_TYPE_TEXTURE_SRV, MutableTexArraySize},
        ShaderResourceDesc{"g_Tex2DArr_Dyn",      SHADER_RESOURCE_TYPE_TEXTURE_SRV, DynamicTexArraySize}
    };
    if (!pDevice->GetDeviceCaps().IsGLDevice())
    {
        if (TestImtblSamplers)
        {
            Resources.emplace_back("g_Tex2D_Static_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2D_Mut_sampler",       SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2D_Dyn_sampler",       SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2DArr_Static_sampler", SHADER_RESOURCE_TYPE_SAMPLER, 1);
            Resources.emplace_back("g_Tex2DArr_Mut_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, MutableTexArraySize);
            Resources.emplace_back("g_Tex2DArr_Dyn_sampler",    SHADER_RESOURCE_TYPE_SAMPLER, DynamicTexArraySize);
        }
        else
        {
            Resources.emplace_back("g_Sampler", SHADER_RESOURCE_TYPE_SAMPLER, 1);
        }
    }
    // clang-format on

    auto ModifyShaderCI = [TestImtblSamplers](ShaderCreateInfo& ShaderCI) {
        if (TestImtblSamplers)
        {
            ShaderCI.UseCombinedTextureSamplers = true;
            // Immutable sampler arrays are not allowed in 5.1, and DXC only supports 6.0+
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
            ShaderCI.HLSLVersion    = ShaderVersion{5, 0};
        }
    };
    auto pVS = CreateShader(TestImtblSamplers ? "ShaderResourceLayoutTest.ImtblSamplers - VS" : "ShaderResourceLayoutTest.Textures - VS",
                            TestImtblSamplers ? "ImmutableSamplers.hlsl" : "Textures.hlsl",
                            "VSMain",
                            SHADER_TYPE_VERTEX, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()),
                            ModifyShaderCI);
    auto pPS = CreateShader(TestImtblSamplers ? "ShaderResourceLayoutTest.ImtblSamplers - PS" : "ShaderResourceLayoutTest.Textures - PS",
                            TestImtblSamplers ? "ImmutableSamplers.hlsl" : "Textures.hlsl",
                            "PSMain",
                            SHADER_TYPE_PIXEL, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()),
                            ModifyShaderCI);
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);


    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    std::vector<ImmutableSamplerDesc> ImtblSamplers;
    if (TestImtblSamplers)
    {
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Static",    SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Mut",       SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D_Dyn",       SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Static", SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Mut",    SamplerDesc{});
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn",    SamplerDesc{});
    }
    else
    {
        ImtblSamplers.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sampler", SamplerDesc{});
    }
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables            = Vars;
    ResourceLayout.NumVariables         = _countof(Vars);
    ResourceLayout.ImmutableSamplers    = ImtblSamplers.data();
    ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto MaxTextures = std::max(std::max(StaticTexArraySize, MutableTexArraySize), DynamicTexArraySize);

    std::vector<RefCntAutoPtr<ITexture>> pTextures(MaxTextures);
    std::vector<IDeviceObject*>          pTexSRVs(MaxTextures);

    for (Uint32 i = 0; i < MaxTextures; ++i)
    {
        pTextures[i] = pEnv->CreateTexture("Test texture", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, 256, 256);
        pTexSRVs[i]  = pTextures[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_Tex2D_Static", Set, pTexSRVs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_Tex2DArr_Static", SetArray, pTexSRVs.data(), 0, StaticTexArraySize);

    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Tex2D_Static", Set, pTexSRVs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Tex2DArr_Static", SetArray, pTexSRVs.data(), 0, StaticTexArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_Mut", Set, pTexSRVs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_Dyn", Set, pTexSRVs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2DArr_Mut", SetArray, pTexSRVs.data(), 0, MutableTexArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2DArr_Dyn", SetArray, pTexSRVs.data(), 0, DynamicTexArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Mut", Set, pTexSRVs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, pTexSRVs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2DArr_Mut", SetArray, pTexSRVs.data(), 0, MutableTexArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn", SetArray, pTexSRVs.data(), 0, DynamicTexArraySize);

    pSRB->InitializeStaticResources(pPSO);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pRTV};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2D_Dyn", Set, pTexSRVs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Tex2DArr_Dyn", SetArray, pTexSRVs.data(), 1, DynamicTexArraySize - 1);

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2D_Dyn", Set, pTexSRVs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Tex2DArr_Dyn", SetArray, pTexSRVs.data(), 1, DynamicTexArraySize - 1);

    pContext->Draw(DrawAttrs);
}

TEST_F(ShaderResourceLayoutTest, Textures)
{
    TestTexturesAndImtblSamplers(false);
}

TEST_F(ShaderResourceLayoutTest, ImmutableSamplers)
{
    TestTexturesAndImtblSamplers(true);
}


void ShaderResourceLayoutTest::TestStructuredOrFormattedBuffer(bool IsFormatted)
{
    TestingEnvironment::ScopedReset AutoResetEnvironment;

    static constexpr int StaticBuffArraySize  = 4;
    static constexpr int MutableBuffArraySize = 3;
    static constexpr int DynamicBuffArraySize = 2;
    ShaderMacroHelper    Macros;
    Macros.AddShaderMacro("STATIC_BUFF_ARRAY_SIZE", StaticBuffArraySize);
    Macros.AddShaderMacro("MUTABLE_BUFF_ARRAY_SIZE", MutableBuffArraySize);
    Macros.AddShaderMacro("DYNAMIC_BUFF_ARRAY_SIZE", DynamicBuffArraySize);

    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& deviceCaps = pDevice->GetDeviceCaps();

    // Vulkan only allows 16 dynamic storage buffer bindings among all stages, so
    // use arrays only in fragment shader for structured buffer test.
    const auto UseArraysInPSOnly = !IsFormatted && (deviceCaps.IsVulkanDevice() || deviceCaps.IsMetalDevice());

    // clang-format off
    std::vector<ShaderResourceDesc> Resources = 
    {
        {"g_Buff_Static", SHADER_RESOURCE_TYPE_BUFFER_SRV, 1},
        {"g_Buff_Mut",    SHADER_RESOURCE_TYPE_BUFFER_SRV, 1},
        {"g_Buff_Dyn",    SHADER_RESOURCE_TYPE_BUFFER_SRV, 1}
    };

    auto AddArrayResources = [&Resources]()
    {
        Resources.emplace_back("g_BuffArr_Static", SHADER_RESOURCE_TYPE_BUFFER_SRV, StaticBuffArraySize);
        Resources.emplace_back("g_BuffArr_Mut",    SHADER_RESOURCE_TYPE_BUFFER_SRV, MutableBuffArraySize);
        Resources.emplace_back("g_BuffArr_Dyn",    SHADER_RESOURCE_TYPE_BUFFER_SRV, DynamicBuffArraySize);
    };
    // clang-format on
    if (!UseArraysInPSOnly)
    {
        AddArrayResources();
    }

    const char*            ShaderFileName = nullptr;
    SHADER_SOURCE_LANGUAGE SrcLang        = SHADER_SOURCE_LANGUAGE_DEFAULT;
    if (pDevice->GetDeviceCaps().IsD3DDevice())
    {
        ShaderFileName = IsFormatted ? "FormattedBuffers.hlsl" : "StructuredBuffers.hlsl";
        SrcLang        = SHADER_SOURCE_LANGUAGE_HLSL;
    }
    else if (pDevice->GetDeviceCaps().IsVulkanDevice() || pDevice->GetDeviceCaps().IsGLDevice() || pDevice->GetDeviceCaps().IsMetalDevice())
    {
        ShaderFileName = IsFormatted ? "FormattedBuffers.hlsl" : "StructuredBuffers.glsl";
        SrcLang        = IsFormatted ? SHADER_SOURCE_LANGUAGE_HLSL : SHADER_SOURCE_LANGUAGE_GLSL;
    }
    else
    {
        GTEST_FAIL() << "Unexpected device type";
    }
    auto pVS = CreateShader(IsFormatted ? "ShaderResourceLayoutTest.FormattedBuffers - VS" : "ShaderResourceLayoutTest.StructuredBuffers - VS",
                            ShaderFileName, SrcLang == SHADER_SOURCE_LANGUAGE_HLSL ? "VSMain" : "main",
                            SHADER_TYPE_VERTEX, SrcLang, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()));
    if (UseArraysInPSOnly)
    {
        AddArrayResources();
    }
    auto pPS = CreateShader(IsFormatted ? "ShaderResourceLayoutTest.FormattedBuffers - PS" : "ShaderResourceLayoutTest.StructuredBuffers - PS",
                            ShaderFileName, SrcLang == SHADER_SOURCE_LANGUAGE_HLSL ? "PSMain" : "main",
                            SHADER_TYPE_PIXEL, SrcLang, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()));
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);


    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Buff_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Buff_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Buff_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_BuffArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_BuffArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_BuffArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;

    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto MaxBuffers = std::max(std::max(StaticBuffArraySize, MutableBuffArraySize), DynamicBuffArraySize);

    std::vector<RefCntAutoPtr<IBufferView>> pBufferViews(MaxBuffers);
    std::vector<IDeviceObject*>             pBuffSRVs(MaxBuffers);

    for (Uint32 i = 0; i < MaxBuffers; ++i)
    {
        pBufferViews[i] = CreateResourceBufferView(IsFormatted ? BUFFER_MODE_FORMATTED : BUFFER_MODE_STRUCTURED, BUFFER_VIEW_SHADER_RESOURCE);
        ASSERT_NE(pBufferViews[i], nullptr) << "Unable to formatted buffer view ";
        pBuffSRVs[i] = pBufferViews[i];
    }

    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_Buff_Static", Set, pBuffSRVs[0]);
    if (!UseArraysInPSOnly)
    {
        SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_BuffArr_Static", SetArray, pBuffSRVs.data(), 0, StaticBuffArraySize);
    }
    else
    {
        EXPECT_EQ(pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "g_BuffArr_Static"), nullptr);
    }
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Buff_Static", Set, pBuffSRVs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_BuffArr_Static", SetArray, pBuffSRVs.data(), 0, StaticBuffArraySize);


    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Buff_Mut", Set, pBuffSRVs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Buff_Dyn", Set, pBuffSRVs[0]);
    if (!UseArraysInPSOnly)
    {
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_BuffArr_Mut", SetArray, pBuffSRVs.data(), 0, MutableBuffArraySize);
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_BuffArr_Dyn", SetArray, pBuffSRVs.data(), 0, DynamicBuffArraySize);
    }
    else
    {
        EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_BuffArr_Mut"), nullptr);
        EXPECT_EQ(pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "g_BuffArr_Dyn"), nullptr);
    }

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Buff_Mut", Set, pBuffSRVs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Buff_Dyn", Set, pBuffSRVs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_BuffArr_Mut", SetArray, pBuffSRVs.data(), 0, MutableBuffArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_BuffArr_Dyn", SetArray, pBuffSRVs.data(), 0, DynamicBuffArraySize);

    pSRB->InitializeStaticResources(pPSO);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pRTV};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Buff_Dyn", Set, pBuffSRVs[1]);
    if (!UseArraysInPSOnly)
    {
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_BuffArr_Dyn", SetArray, pBuffSRVs.data(), 1, DynamicBuffArraySize - 1);
    }

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Buff_Dyn", Set, pBuffSRVs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_BuffArr_Dyn", SetArray, pBuffSRVs.data(), 1, DynamicBuffArraySize - 1);

    pContext->Draw(DrawAttrs);
}

TEST_F(ShaderResourceLayoutTest, FormattedBuffers)
{
    TestStructuredOrFormattedBuffer(true /*IsFormatted*/);
}

TEST_F(ShaderResourceLayoutTest, StructuredBuffers)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceCaps().IsGLDevice())
    {
        GTEST_SKIP() << "Read-only structured buffers in glsl are currently "
                        "identified as UAVs in OpenGL backend because "
                        "there seems to be no way to detect read-only property on the host";
    }

    TestStructuredOrFormattedBuffer(false /*IsFormatted*/);
}


void ShaderResourceLayoutTest::TestRWStructuredOrFormattedBuffer(bool IsFormatted)
{
    TestingEnvironment::ScopedReset AutoResetEnvironment;

    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& deviceCaps = pDevice->GetDeviceCaps();
    auto        deviceType = deviceCaps.DevType;

    const Uint32 StaticBuffArraySize  = deviceType == RENDER_DEVICE_TYPE_D3D11 || deviceCaps.IsGLDevice() ? 1 : 4;
    const Uint32 MutableBuffArraySize = deviceType == RENDER_DEVICE_TYPE_D3D11 || deviceCaps.IsGLDevice() ? 2 : 3;
    const Uint32 DynamicBuffArraySize = 2;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("STATIC_BUFF_ARRAY_SIZE", static_cast<int>(StaticBuffArraySize));
    Macros.AddShaderMacro("MUTABLE_BUFF_ARRAY_SIZE", static_cast<int>(MutableBuffArraySize));
    Macros.AddShaderMacro("DYNAMIC_BUFF_ARRAY_SIZE", static_cast<int>(DynamicBuffArraySize));

    // clang-format off
    ShaderResourceDesc Resources[] = 
    {
        {"g_RWBuff_Static",    SHADER_RESOURCE_TYPE_BUFFER_UAV, 1},
        {"g_RWBuff_Mut",       SHADER_RESOURCE_TYPE_BUFFER_UAV, 1},
        {"g_RWBuff_Dyn",       SHADER_RESOURCE_TYPE_BUFFER_UAV, 1},
        {"g_RWBuffArr_Static", SHADER_RESOURCE_TYPE_BUFFER_UAV, StaticBuffArraySize },
        {"g_RWBuffArr_Mut",    SHADER_RESOURCE_TYPE_BUFFER_UAV, MutableBuffArraySize},
        {"g_RWBuffArr_Dyn",    SHADER_RESOURCE_TYPE_BUFFER_UAV, DynamicBuffArraySize}
    };

    const char*            ShaderFileName = nullptr;
    SHADER_SOURCE_LANGUAGE SrcLang        = SHADER_SOURCE_LANGUAGE_DEFAULT;
    if (pDevice->GetDeviceCaps().IsD3DDevice())
    {
        ShaderFileName = IsFormatted ? "RWFormattedBuffers.hlsl" : "RWStructuredBuffers.hlsl";
        SrcLang        = SHADER_SOURCE_LANGUAGE_HLSL;
    }
    else if (deviceCaps.IsVulkanDevice() || deviceCaps.IsGLDevice() || deviceCaps.IsMetalDevice())
    {
        ShaderFileName = IsFormatted ? "RWFormattedBuffers.hlsl" : "RWStructuredBuffers.glsl";
        SrcLang        = IsFormatted ? SHADER_SOURCE_LANGUAGE_HLSL : SHADER_SOURCE_LANGUAGE_GLSL;
    }
    else
    {
        GTEST_FAIL() << "Unexpected device type";
    }
    auto pCS = CreateShader(IsFormatted ? "ShaderResourceLayoutTest.RWFormattedBuffers - CS" : "ShaderResourceLayoutTest.RWtructuredBuffers - CS",
                            ShaderFileName, "main",
                            SHADER_TYPE_COMPUTE, SrcLang, Macros,
                            Resources, _countof(Resources));
    ASSERT_NE(pCS, nullptr);

    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_COMPUTE, "g_RWBuff_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWBuff_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWBuff_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_COMPUTE, "g_RWBuffArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWBuffArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWBuffArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;

    CreateComputePSO(pCS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto TotalBuffers = StaticBuffArraySize + MutableBuffArraySize + DynamicBuffArraySize + 3 + 2;

    std::vector<RefCntAutoPtr<IBufferView>> pBufferViews(TotalBuffers);
    std::vector<IDeviceObject*>             pBuffUAVs(TotalBuffers);

    for (Uint32 i = 0; i < TotalBuffers; ++i)
    {
        pBufferViews[i] = CreateResourceBufferView(IsFormatted ? BUFFER_MODE_FORMATTED : BUFFER_MODE_STRUCTURED, BUFFER_VIEW_UNORDERED_ACCESS);
        ASSERT_NE(pBufferViews[i], nullptr) << "Unable to formatted buffer view ";
        pBuffUAVs[i] = pBufferViews[i];
    }

    Uint32 uav = 0;
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWBuff_Static", Set, pBuffUAVs[uav++]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Static", SetArray, &pBuffUAVs[uav], 0, StaticBuffArraySize);
    uav += StaticBuffArraySize;

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuff_Mut", Set, pBuffUAVs[uav++]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuff_Dyn", Set, pBuffUAVs[uav++]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Mut", SetArray, &pBuffUAVs[uav], 0, MutableBuffArraySize);
    uav += MutableBuffArraySize;
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Dyn", SetArray, &pBuffUAVs[uav], 0, DynamicBuffArraySize);
    uav += DynamicBuffArraySize;
    VERIFY_EXPR(uav + 2 == pBuffUAVs.size());

    pSRB->InitializeStaticResources(pPSO);

    auto* pContext = pEnv->GetDeviceContext();

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DispatchComputeAttribs DispatchAttribs(1, 1, 1);
    pContext->DispatchCompute(DispatchAttribs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuff_Dyn", Set, pBuffUAVs[uav++]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWBuffArr_Dyn", SetArray, &pBuffUAVs[uav++], 1, 1);
    pContext->DispatchCompute(DispatchAttribs);
}

TEST_F(ShaderResourceLayoutTest, FormattedRWBuffers)
{
    TestRWStructuredOrFormattedBuffer(true /*IsFormatted*/);
}

TEST_F(ShaderResourceLayoutTest, StructuredRWBuffers)
{
    TestRWStructuredOrFormattedBuffer(false /*IsFormatted*/);
}

TEST_F(ShaderResourceLayoutTest, RWTextures)
{
    TestingEnvironment::ScopedReset AutoResetEnvironment;

    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& deviceCaps = pDevice->GetDeviceCaps();
    auto        deviceType = deviceCaps.DevType;

    const Uint32 StaticTexArraySize  = 2;
    const Uint32 MutableTexArraySize = deviceType == RENDER_DEVICE_TYPE_D3D11 || deviceCaps.IsGLDevice() ? 2 : 4;
    const Uint32 DynamicTexArraySize = deviceType == RENDER_DEVICE_TYPE_D3D11 || deviceCaps.IsGLDevice() ? 1 : 3;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("STATIC_TEX_ARRAY_SIZE", static_cast<int>(StaticTexArraySize));
    Macros.AddShaderMacro("MUTABLE_TEX_ARRAY_SIZE", static_cast<int>(MutableTexArraySize));
    Macros.AddShaderMacro("DYNAMIC_TEX_ARRAY_SIZE", static_cast<int>(DynamicTexArraySize));

    // clang-format off
    ShaderResourceDesc Resources[] = 
    {
        {"g_RWTex2D_Static",    SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWTex2D_Mut",       SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWTex2D_Dyn",       SHADER_RESOURCE_TYPE_TEXTURE_UAV, 1},
        {"g_RWTex2DArr_Static", SHADER_RESOURCE_TYPE_TEXTURE_UAV, StaticTexArraySize },
        {"g_RWTex2DArr_Mut",    SHADER_RESOURCE_TYPE_TEXTURE_UAV, MutableTexArraySize},
        {"g_RWTex2DArr_Dyn",    SHADER_RESOURCE_TYPE_TEXTURE_UAV, DynamicTexArraySize}
    };

    auto pCS = CreateShader("ShaderResourceLayoutTest.RWTextures - CS",
                            "RWTextures.hlsl", "main",
                            SHADER_TYPE_COMPUTE, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources, _countof(Resources));
    ASSERT_NE(pCS, nullptr);

    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_COMPUTE, "g_RWTex2D_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWTex2D_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWTex2D_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;

    CreateComputePSO(pCS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto TotalTextures = StaticTexArraySize + MutableTexArraySize + DynamicTexArraySize + 3 + 2;

    std::vector<RefCntAutoPtr<ITexture>> pTextures(TotalTextures);
    std::vector<IDeviceObject*>          pUAVs(TotalTextures);

    for (Uint32 i = 0; i < TotalTextures; ++i)
    {
        pTextures[i] = pEnv->CreateTexture("Test RW texture", TEX_FORMAT_RGBA32_FLOAT, BIND_UNORDERED_ACCESS, 256, 256);
        ASSERT_NE(pTextures[i], nullptr);
        pUAVs[i] = pTextures[i]->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS);
        ASSERT_NE(pUAVs[i], nullptr);
    }

    Uint32 uav = 0;
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWTex2D_Static", Set, pUAVs[uav++]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Static", SetArray, &pUAVs[uav], 0, StaticTexArraySize);
    uav += StaticTexArraySize;

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2D_Mut", Set, pUAVs[uav++]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2D_Dyn", Set, pUAVs[uav++]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Mut", SetArray, &pUAVs[uav], 0, MutableTexArraySize);
    uav += MutableTexArraySize;
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Dyn", SetArray, &pUAVs[uav], 0, DynamicTexArraySize);
    uav += DynamicTexArraySize;
    VERIFY_EXPR(uav + 2 == pUAVs.size());

    pSRB->InitializeStaticResources(pPSO);

    auto* pContext = pEnv->GetDeviceContext();

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DispatchComputeAttribs DispatchAttribs(1, 1, 1);
    pContext->DispatchCompute(DispatchAttribs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2D_Dyn", Set, pUAVs[uav++]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_COMPUTE, "g_RWTex2DArr_Dyn", SetArray, &pUAVs[uav++], 0, 1);
    pContext->DispatchCompute(DispatchAttribs);
}

TEST_F(ShaderResourceLayoutTest, ConstantBuffers)
{
    TestingEnvironment::ScopedReset AutoResetEnvironment;

    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto deviceCaps = pDevice->GetDeviceCaps();
    //  Vulkan allows 15 dynamic uniform buffer bindings among all stages
    const Uint32 StaticCBArraySize  = 2;
    const Uint32 MutableCBArraySize = deviceCaps.IsVulkanDevice() ? 1 : 4;
    const Uint32 DynamicCBArraySize = deviceCaps.IsVulkanDevice() ? 1 : 3;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("STATIC_CB_ARRAY_SIZE", static_cast<int>(StaticCBArraySize));
    Macros.AddShaderMacro("MUTABLE_CB_ARRAY_SIZE", static_cast<int>(MutableCBArraySize));
    Macros.AddShaderMacro("DYNAMIC_CB_ARRAY_SIZE", static_cast<int>(DynamicCBArraySize));

    const auto CBArraysSupported =
        deviceCaps.DevType == RENDER_DEVICE_TYPE_D3D12 ||
        deviceCaps.DevType == RENDER_DEVICE_TYPE_VULKAN ||
        deviceCaps.DevType == RENDER_DEVICE_TYPE_METAL;
    Macros.AddShaderMacro("ARRAYS_SUPPORTED", CBArraysSupported);

    // clang-format off
    std::vector<ShaderResourceDesc> Resources = 
    {
        ShaderResourceDesc{"UniformBuff_Stat", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1},
        ShaderResourceDesc{"UniformBuff_Mut",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1},
        ShaderResourceDesc{"UniformBuff_Dyn",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, 1}
    };

    if (CBArraysSupported)
    {
        Resources.emplace_back("UniformBuffArr_Stat", SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, StaticCBArraySize);
        Resources.emplace_back("UniformBuffArr_Mut",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, MutableCBArraySize);
        Resources.emplace_back("UniformBuffArr_Dyn",  SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, DynamicCBArraySize);
    }
    // clang-format on

    auto pVS = CreateShader("ShaderResourceLayoutTest.ConstantBuffers - VS",
                            "ConstantBuffers.hlsl",
                            "VSMain",
                            SHADER_TYPE_VERTEX, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()));
    auto pPS = CreateShader("ShaderResourceLayoutTest.ConstantBuffers - PS",
                            "ConstantBuffers.hlsl",
                            "PSMain",
                            SHADER_TYPE_PIXEL, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources.data(), static_cast<Uint32>(Resources.size()));
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);


    // clang-format off
    std::vector<ShaderResourceVariableDesc> Vars =
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Stat", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Mut",  SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuff_Dyn",  SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };

    if (CBArraysSupported)
    {
        Vars.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuffArr_Stat", SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
        Vars.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuffArr_Mut",  SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE);
        Vars.emplace_back(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn",  SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars.data();
    ResourceLayout.NumVariables = static_cast<Uint32>(Vars.size());

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto MaxBuffers = std::max(std::max(StaticCBArraySize, MutableCBArraySize), DynamicCBArraySize);

    std::vector<RefCntAutoPtr<IBuffer>> pBuffers(MaxBuffers);
    std::vector<IDeviceObject*>         pCBObjs(MaxBuffers);

    for (Uint32 i = 0; i < MaxBuffers; ++i)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name          = "Constant buffer";
        BuffDesc.uiSizeInBytes = 256;
        BuffDesc.BindFlags     = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage         = USAGE_DEFAULT;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffers[i]);
        ASSERT_NE(pBuffers[i], nullptr);
        pCBObjs[i] = pBuffers[i];
    }

    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "UniformBuff_Stat", Set, pCBObjs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "UniformBuff_Stat", Set, pCBObjs[0]);
    if (CBArraysSupported)
    {
        SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "UniformBuffArr_Stat", SetArray, pCBObjs.data(), 0, StaticCBArraySize);
        SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "UniformBuffArr_Stat", SetArray, pCBObjs.data(), 0, StaticCBArraySize);
    }

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuff_Mut", Set, pCBObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuff_Dyn", Set, pCBObjs[0]);
    if (CBArraysSupported)
    {
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuffArr_Mut", SetArray, pCBObjs.data(), 0, MutableCBArraySize);
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuffArr_Dyn", SetArray, pCBObjs.data(), 0, DynamicCBArraySize);
    }

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuff_Mut", Set, pCBObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuff_Dyn", Set, pCBObjs[0]);
    if (CBArraysSupported)
    {
        SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuffArr_Mut", SetArray, pCBObjs.data(), 0, MutableCBArraySize);
        SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn", SetArray, pCBObjs.data(), 0, DynamicCBArraySize);
    }

    pSRB->InitializeStaticResources(pPSO);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pRTV};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuff_Dyn", Set, pCBObjs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuff_Dyn", Set, pCBObjs[1]);
    if (CBArraysSupported)
    {
        SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "UniformBuffArr_Dyn", SetArray, &pCBObjs[1], 0, DynamicCBArraySize - 1);
        SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "UniformBuffArr_Dyn", SetArray, &pCBObjs[1], 0, DynamicCBArraySize - 1);
    }

    pContext->Draw(DrawAttrs);
}

TEST_F(ShaderResourceLayoutTest, Samplers)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceCaps().IsGLDevice())
    {
        GTEST_SKIP() << "OpenGL does not support separate samplers";
    }

    TestingEnvironment::ScopedReset AutoResetEnvironment;

    static constexpr Uint32 StaticSamArraySize  = 2;
    static constexpr Uint32 MutableSamArraySize = 4;
    static constexpr Uint32 DynamicSamArraySize = 3;
    ShaderMacroHelper       Macros;
    Macros.AddShaderMacro("STATIC_SAM_ARRAY_SIZE", static_cast<int>(StaticSamArraySize));
    Macros.AddShaderMacro("MUTABLE_SAM_ARRAY_SIZE", static_cast<int>(MutableSamArraySize));
    Macros.AddShaderMacro("DYNAMIC_SAM_ARRAY_SIZE", static_cast<int>(DynamicSamArraySize));

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    // clang-format off
    ShaderResourceDesc Resources[] = 
    {
        {"g_Sam_Static",      SHADER_RESOURCE_TYPE_SAMPLER,     1},
        {"g_Sam_Mut",         SHADER_RESOURCE_TYPE_SAMPLER,     1},
        {"g_Sam_Dyn",         SHADER_RESOURCE_TYPE_SAMPLER,     1},
        {"g_SamArr_Static",   SHADER_RESOURCE_TYPE_SAMPLER,     StaticSamArraySize},
        {"g_SamArr_Mut",      SHADER_RESOURCE_TYPE_SAMPLER,     MutableSamArraySize},
        {"g_SamArr_Dyn",      SHADER_RESOURCE_TYPE_SAMPLER,     DynamicSamArraySize},
        {"g_Tex2D",           SHADER_RESOURCE_TYPE_TEXTURE_SRV, 1},
    };
    // clang-format on
    auto pVS = CreateShader("ShaderResourceLayoutTest.Samplers - VS", "Samplers.hlsl", "VSMain",
                            SHADER_TYPE_VERTEX, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources, _countof(Resources));
    auto pPS = CreateShader("ShaderResourceLayoutTest.Samplers - PS", "Samplers.hlsl", "PSMain",
                            SHADER_TYPE_PIXEL, SHADER_SOURCE_LANGUAGE_HLSL, Macros,
                            Resources, _countof(Resources));
    ASSERT_NE(pVS, nullptr);
    ASSERT_NE(pPS, nullptr);


    // clang-format off
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Tex2D",         SHADER_RESOURCE_VARIABLE_TYPE_STATIC},

        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sam_Static",    SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sam_Mut",       SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Sam_Dyn",       SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_SamArr_Static", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_SamArr_Mut",    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_SamArr_Dyn",    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    // clang-format on

    PipelineResourceLayoutDesc ResourceLayout;
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    CreateGraphicsPSO(pVS, pPS, ResourceLayout, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto MaxSamplers = std::max(std::max(StaticSamArraySize, MutableSamArraySize), DynamicSamArraySize);

    std::vector<RefCntAutoPtr<ISampler>> pSamplers(MaxSamplers);
    std::vector<IDeviceObject*>          pSamObjs(MaxSamplers);

    for (Uint32 i = 0; i < MaxSamplers; ++i)
    {
        SamplerDesc SamDesc;
        pDevice->CreateSampler(SamDesc, &pSamplers[i]);
        ASSERT_NE(pSamplers[i], nullptr);
        pSamObjs[i] = pSamplers[i];
    }

    auto  pTex2D    = pEnv->CreateTexture("ShaderResourceLayoutTest: test RTV", TEX_FORMAT_RGBA8_UNORM, BIND_SHADER_RESOURCE, 512, 512);
    auto* pTex2DSRV = pTex2D->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_Tex2D", Set, pTex2DSRV);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Tex2D", Set, pTex2DSRV);


    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_Sam_Static", Set, pSamObjs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_VERTEX, "g_SamArr_Static", SetArray, pSamObjs.data(), 0, StaticSamArraySize);

    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_Sam_Static", Set, pSamObjs[0]);
    SET_STATIC_VAR(pPSO, SHADER_TYPE_PIXEL, "g_SamArr_Static", SetArray, pSamObjs.data(), 0, StaticSamArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Sam_Mut", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Sam_Dyn", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_SamArr_Mut", SetArray, pSamObjs.data(), 0, MutableSamArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 0, DynamicSamArraySize);

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Sam_Mut", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Sam_Dyn", Set, pSamObjs[0]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_SamArr_Mut", SetArray, pSamObjs.data(), 0, MutableSamArraySize);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 0, DynamicSamArraySize);

    pSRB->InitializeStaticResources(pPSO);

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* ppRTVs[] = {pRTV};
    pContext->SetRenderTargets(1, ppRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(DrawAttrs);

    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_Sam_Dyn", Set, pSamObjs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_VERTEX, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 1, DynamicSamArraySize - 1);

    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_Sam_Dyn", Set, pSamObjs[1]);
    SET_SRB_VAR(pSRB, SHADER_TYPE_PIXEL, "g_SamArr_Dyn", SetArray, pSamObjs.data(), 1, DynamicSamArraySize - 1);

    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->Draw(DrawAttrs);
}

} // namespace
