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

#include <sstream>

#include "TestingEnvironment.hpp"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static const char g_TrivialVSSource[] = R"(
void VSMain(out float4 pos : SV_POSITION)
{
	pos = float4(0.0, 0.0, 0.0, 0.0);
}
)";

class BlendStateTestBase
{
protected:
    static void InitResources()
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        ShaderCreateInfo Attrs;
        Attrs.Source                     = g_TrivialVSSource;
        Attrs.EntryPoint                 = "VSMain";
        Attrs.Desc.ShaderType            = SHADER_TYPE_VERTEX;
        Attrs.Desc.Name                  = "TrivialVS (TestPipelineStateBase)";
        Attrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        Attrs.ShaderCompiler             = pEnv->GetDefaultCompiler(Attrs.SourceLanguage);
        Attrs.UseCombinedTextureSamplers = true;
        pDevice->CreateShader(Attrs, &sm_Resources.pTrivialVS);

        Uint32 MaxTestRenderTargets = pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_GLES ? 4 : 8;
        for (Uint32 NumRTs = 1; NumRTs <= MaxTestRenderTargets; ++NumRTs)
        {
            std::stringstream source_ss;
            source_ss << "void PSMain(";
            for (Uint32 rt = 0; rt < NumRTs; ++rt)
            {
                if (rt > 0)
                {
                    source_ss << ",\n"
                              << "            ";
                }
                source_ss << "out float4 col" << rt << " : SV_TARGET" << rt;
            }
            source_ss << ")\n{";
            for (Uint32 rt = 0; rt < NumRTs; ++rt)
            {
                source_ss << "\n    col" << rt << " = float4(0.0, 0.0, 0.0, 0.0);";
            }
            source_ss << "\n}\n";

            auto source           = source_ss.str();
            Attrs.Source          = source.c_str();
            Attrs.EntryPoint      = "PSMain";
            Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
            Attrs.Desc.Name       = "TrivialPS (TestPipelineStateBase)";
            pDevice->CreateShader(Attrs, &sm_Resources.pTrivialPS[NumRTs]);
        }

        auto& PSOCreateInfo    = sm_Resources.PSOCreateInfo;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSOCreateInfo.pVS                  = sm_Resources.pTrivialVS;
        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.NumRenderTargets  = 1;
        GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
        GraphicsPipeline.DSVFormat         = TEX_FORMAT_D32_FLOAT;
    }

    static void ReleaseResources()
    {
        sm_Resources = Resources{};
        TestingEnvironment::GetInstance()->Reset();
    }

    static RefCntAutoPtr<IPipelineState> CreateTestPSO(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, bool BindPSO)
    {
        auto* pEnv           = TestingEnvironment::GetInstance();
        auto* pDevice        = pEnv->GetDevice();
        auto* pDeviceContext = pEnv->GetDeviceContext();

        RefCntAutoPtr<IPipelineState> pPSO;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        EXPECT_TRUE(pPSO);
        if (pPSO)
        {
            pDeviceContext->SetPipelineState(pPSO);
            const auto& RefBlendDesc  = PSOCreateInfo.GraphicsPipeline.BlendDesc;
            const auto& TestBlendDesc = pPSO->GetGraphicsPipelineDesc().BlendDesc;
            EXPECT_EQ(RefBlendDesc, TestBlendDesc);
        }

        return pPSO;
    }

    static GraphicsPipelineStateCreateInfo GetPSOCreateInfo(Uint32 NumRenderTargets)
    {
        auto  PSOCreateInfo               = sm_Resources.PSOCreateInfo;
        auto& GraphicsPipeline            = PSOCreateInfo.GraphicsPipeline;
        GraphicsPipeline.NumRenderTargets = static_cast<Uint8>(NumRenderTargets);
        PSOCreateInfo.pPS                 = sm_Resources.pTrivialPS[NumRenderTargets];
        for (Uint32 rt = 1; rt < NumRenderTargets; ++rt)
            GraphicsPipeline.RTVFormats[rt] = GraphicsPipeline.RTVFormats[0];

        return PSOCreateInfo;
    }

private:
    struct Resources
    {
        RefCntAutoPtr<IShader> pTrivialVS;
        RefCntAutoPtr<IShader> pTrivialPS[MAX_RENDER_TARGETS + 1];

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
    };

    static Resources sm_Resources;
};

BlendStateTestBase::Resources BlendStateTestBase::sm_Resources;

class BlendStateBasicTest : public BlendStateTestBase, public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        InitResources();
    }

    static void TearDownTestSuite()
    {
        ReleaseResources();
        TestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_F(BlendStateBasicTest, CreatePSO)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo = GetPSOCreateInfo(1);

    PSOCreateInfo.PSODesc.Name = "BlendStateBasicTest";

    BlendStateDesc& BSDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;

    auto& RT0 = BSDesc.RenderTargets[0];

    {
        RT0.BlendEnable = True;

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        EXPECT_NE(pPSO, nullptr);
        if (pPSO != nullptr)
        {
            EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[0].BlendEnable, RT0.BlendEnable);
        }
    }

    {
        BSDesc.AlphaToCoverageEnable = !BSDesc.AlphaToCoverageEnable;

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        EXPECT_NE(pPSO, nullptr);
        if (pPSO != nullptr)
        {
            EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().BlendDesc.AlphaToCoverageEnable, BSDesc.AlphaToCoverageEnable);
        }
    }

    {
        RT0.RenderTargetWriteMask = COLOR_MASK_BLUE;

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        EXPECT_NE(pPSO, nullptr);
        if (pPSO != nullptr)
        {
            EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[0].RenderTargetWriteMask, COLOR_MASK_BLUE);
        }
    }

    {
        RT0.RenderTargetWriteMask = COLOR_MASK_RED;

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        EXPECT_NE(pPSO, nullptr);
        if (pPSO != nullptr)
        {
            EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[0].RenderTargetWriteMask, COLOR_MASK_RED);
        }
    }

    {
        RT0.RenderTargetWriteMask = COLOR_MASK_GREEN;

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        EXPECT_NE(pPSO, nullptr);
        if (pPSO != nullptr)
        {
            EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[0].RenderTargetWriteMask, COLOR_MASK_GREEN);
        }
    }

    {
        RT0.RenderTargetWriteMask = COLOR_MASK_ALPHA;

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        EXPECT_NE(pPSO, nullptr);
        if (pPSO != nullptr)
        {
            EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[0].RenderTargetWriteMask, COLOR_MASK_ALPHA);
        }
    }
}

class BlendFactorTest : public BlendStateTestBase, public testing::TestWithParam<std::tuple<int, bool>>
{
protected:
    static void SetUpTestSuite()
    {
        InitResources();
    }

    static void TearDownTestSuite()
    {
        ReleaseResources();
        TestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_P(BlendFactorTest, CreatePSO)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& Param = GetParam();

    const auto BlendFactor  = static_cast<BLEND_FACTOR>(std::get<0>(Param));
    const bool TestingAlpha = std::get<1>(Param);

    const auto& DevCaps              = pDevice->GetDeviceCaps();
    Uint32      MaxTestRenderTargets = (DevCaps.DevType == RENDER_DEVICE_TYPE_GLES) ? 4 : 8;

    const bool TestSRC1 = DevCaps.DevType != RENDER_DEVICE_TYPE_GLES; // || DevCaps.Vendor == GPU_VENDOR::NVIDIA;
    if (BlendFactor == BLEND_FACTOR_SRC1_COLOR ||
        BlendFactor == BLEND_FACTOR_INV_SRC1_COLOR ||
        BlendFactor == BLEND_FACTOR_SRC1_ALPHA ||
        BlendFactor == BLEND_FACTOR_INV_SRC1_ALPHA)
    {
        if (!TestSRC1)
        {
            GTEST_SKIP();
        }
        else
        {
            MaxTestRenderTargets = 1;
        }
    }

    for (Uint32 NumRenderTarges = 1; NumRenderTarges < MaxTestRenderTargets; ++NumRenderTarges)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo = GetPSOCreateInfo(NumRenderTarges);

        PSOCreateInfo.PSODesc.Name = "SrcBlendFactorTest";

        BlendStateDesc& BSDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;

        BSDesc.IndependentBlendEnable = True;
        for (Uint32 i = 0; i < NumRenderTarges; ++i)
        {
            auto& RT = BSDesc.RenderTargets[i];

            RT.BlendEnable = True;
            if (TestingAlpha)
                RT.SrcBlendAlpha = BlendFactor;
            else
                RT.SrcBlend = BlendFactor;
        }

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO) << "Number of render targets: " << NumRenderTarges;
        for (Uint32 i = 0; i < NumRenderTarges; ++i)
        {
            auto& RT = pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[i];

            EXPECT_EQ(RT.BlendEnable, True) << "Render target: " << i;
            if (TestingAlpha)
            {
                EXPECT_EQ(RT.SrcBlendAlpha, BlendFactor) << "Render target: " << i;
            }
            else
            {
                EXPECT_EQ(RT.SrcBlend, BlendFactor) << "Render target: " << i;
            }
        }
    }

    for (Uint32 NumRenderTarges = 1; NumRenderTarges < MaxTestRenderTargets; ++NumRenderTarges)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo = GetPSOCreateInfo(NumRenderTarges);

        PSOCreateInfo.PSODesc.Name = "DstBlendFactorTest";

        BlendStateDesc& BSDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;

        BSDesc.IndependentBlendEnable = True;
        for (Uint32 i = 0; i < NumRenderTarges; ++i)
        {
            auto& RT = BSDesc.RenderTargets[i];

            RT.BlendEnable = True;
            if (TestingAlpha)
                RT.DestBlendAlpha = BlendFactor;
            else
                RT.DestBlend = BlendFactor;
        }

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO) << "Number of render targets: " << NumRenderTarges;
        for (Uint32 i = 0; i < NumRenderTarges; ++i)
        {
            auto& RT = pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[i];

            EXPECT_EQ(RT.BlendEnable, True) << "Render target: " << i;
            if (TestingAlpha)
            {
                EXPECT_EQ(RT.DestBlendAlpha, BlendFactor) << "Render target: " << i;
            }
            else
            {
                EXPECT_EQ(RT.DestBlend, BlendFactor) << "Render target: " << i;
            }
        }
    }
}

std::string PrintBlendFactorTestName(const testing::TestParamInfo<std::tuple<int, bool>>& info) //
{
    auto BlendFactor = static_cast<BLEND_FACTOR>(std::get<0>(info.param));

    std::stringstream name_ss;
    name_ss << GetBlendFactorLiteralName(BlendFactor);
    return name_ss.str();
}

INSTANTIATE_TEST_SUITE_P(ColorBlendFactors,
                         BlendFactorTest,
                         testing::Combine(
                             testing::Range<int>(BLEND_FACTOR_UNDEFINED + 1, BLEND_FACTOR_NUM_FACTORS),
                             testing::Values(false)),
                         PrintBlendFactorTestName);


INSTANTIATE_TEST_SUITE_P(AlphaBlendFactors,
                         BlendFactorTest,
                         testing::Combine(
                             testing::Values<int>(BLEND_FACTOR_ZERO,
                                                  BLEND_FACTOR_ONE,
                                                  BLEND_FACTOR_SRC_ALPHA,
                                                  BLEND_FACTOR_INV_SRC_ALPHA,
                                                  BLEND_FACTOR_DEST_ALPHA,
                                                  BLEND_FACTOR_INV_DEST_ALPHA,
                                                  BLEND_FACTOR_SRC_ALPHA_SAT,
                                                  BLEND_FACTOR_BLEND_FACTOR,
                                                  BLEND_FACTOR_INV_BLEND_FACTOR,
                                                  BLEND_FACTOR_SRC1_ALPHA,
                                                  BLEND_FACTOR_INV_SRC1_ALPHA),
                             testing::Values(true)),
                         PrintBlendFactorTestName);


class BlendOperationTest : public BlendStateTestBase, public testing::TestWithParam<std::tuple<int, bool>>
{
protected:
    static void SetUpTestSuite()
    {
        InitResources();
    }

    static void TearDownTestSuite()
    {
        ReleaseResources();
        TestingEnvironment::GetInstance()->ReleaseResources();
    }
};


TEST_P(BlendOperationTest, CreatePSO)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& Param = GetParam();

    const auto BlendOp      = static_cast<BLEND_OPERATION>(std::get<0>(Param));
    const auto TestingAlpha = std::get<1>(Param);

    const auto&  DevCaps              = pDevice->GetDeviceCaps();
    const Uint32 MaxTestRenderTargets = (DevCaps.DevType == RENDER_DEVICE_TYPE_GLES) ? 4 : 8;

    for (Uint32 NumRenderTarges = 1; NumRenderTarges < MaxTestRenderTargets; ++NumRenderTarges)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo = GetPSOCreateInfo(NumRenderTarges);

        PSOCreateInfo.PSODesc.Name = "BlendOperationTest";

        BlendStateDesc& BSDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;

        BSDesc.IndependentBlendEnable = True;
        for (Uint32 i = 0; i < NumRenderTarges; ++i)
        {
            auto& RT          = BSDesc.RenderTargets[i];
            RT.BlendEnable    = True;
            RT.SrcBlend       = BLEND_FACTOR_SRC_COLOR;
            RT.DestBlend      = BLEND_FACTOR_INV_SRC_COLOR;
            RT.SrcBlendAlpha  = BLEND_FACTOR_SRC_ALPHA;
            RT.DestBlendAlpha = BLEND_FACTOR_INV_SRC_ALPHA;
            if (TestingAlpha)
                RT.BlendOpAlpha = BlendOp;
            else
                RT.BlendOp = BlendOp;
        }

        auto pPSO = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO) << "Number of render targets: " << NumRenderTarges;
        for (Uint32 i = 0; i < NumRenderTarges; ++i)
        {
            auto& RT = pPSO->GetGraphicsPipelineDesc().BlendDesc.RenderTargets[i];

            EXPECT_EQ(RT.BlendEnable, True) << "Render target: " << i;
            if (TestingAlpha)
            {
                EXPECT_EQ(RT.BlendOpAlpha, BlendOp) << "Render target: " << i;
            }
            else
            {
                EXPECT_EQ(RT.BlendOp, BlendOp) << "Render target: " << i;
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(BlendOperations,
                         BlendOperationTest,
                         testing::Combine(
                             testing::Range<int>(BLEND_OPERATION_UNDEFINED + 1, BLEND_OPERATION_NUM_OPERATIONS),
                             testing::Values(true, false)),
                         [](const testing::TestParamInfo<std::tuple<int, bool>>& info) //
                         {
                             auto BlendOp        = static_cast<BLEND_OPERATION>(std::get<0>(info.param));
                             auto IsTestingAlpha = std::get<1>(info.param);

                             std::stringstream name_ss;
                             name_ss << (IsTestingAlpha ? "Alpha_" : "Color_") << GetBlendOperationLiteralName(BlendOp);
                             return name_ss.str();
                         } //
);

} // namespace
