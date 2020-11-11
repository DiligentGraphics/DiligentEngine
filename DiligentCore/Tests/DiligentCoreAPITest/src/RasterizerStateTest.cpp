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
#include "PSOTestBase.hpp"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

class RasterizerStateTest : public PSOTestBase, public ::testing::Test
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

TEST_F(RasterizerStateTest, CreatePSO)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo = GetPSOCreateInfo();

    RasterizerStateDesc& RSDesc = PSOCreateInfo.GraphicsPipeline.RasterizerDesc;

    ASSERT_TRUE(CreateTestPSO(PSOCreateInfo, true));

    for (auto FillMode = FILL_MODE_UNDEFINED + 1; FillMode < FILL_MODE_NUM_MODES; ++FillMode)
    {
        RSDesc.FillMode = static_cast<FILL_MODE>(FillMode);
        auto pPSO       = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO) << "Fill mode: " << GetFillModeLiteralName(RSDesc.FillMode);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.FillMode, RSDesc.FillMode);
    }

    for (auto CullMode = CULL_MODE_UNDEFINED + 1; CullMode < CULL_MODE_NUM_MODES; ++CullMode)
    {
        RSDesc.CullMode = static_cast<CULL_MODE>(CullMode);
        auto pPSO       = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO) << "Cull mode: " << GetCullModeLiteralName(RSDesc.CullMode);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.CullMode, RSDesc.CullMode);
    }

    {
        RSDesc.FrontCounterClockwise = !RSDesc.FrontCounterClockwise;
        auto pPSO                    = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.FrontCounterClockwise, RSDesc.FrontCounterClockwise);
    }

    {
        RSDesc.DepthBias = 100;
        auto pPSO        = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.DepthBias, RSDesc.DepthBias);
    }

    {
        RSDesc.DepthBiasClamp = 1.f;
        auto pPSO             = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.DepthBiasClamp, RSDesc.DepthBiasClamp);
    }

    {
        RSDesc.SlopeScaledDepthBias = 2.f;
        auto pPSO                   = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.SlopeScaledDepthBias, RSDesc.SlopeScaledDepthBias);
    }

    {
        RSDesc.DepthClipEnable = !RSDesc.DepthClipEnable;
        auto pPSO              = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.DepthClipEnable, RSDesc.DepthClipEnable);
    }

    {
        RSDesc.ScissorEnable = !RSDesc.ScissorEnable;
        auto pPSO            = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable, RSDesc.ScissorEnable);
    }

    {
        RSDesc.AntialiasedLineEnable = !RSDesc.AntialiasedLineEnable;
        auto pPSO                    = CreateTestPSO(PSOCreateInfo, true);
        ASSERT_TRUE(pPSO);
        EXPECT_EQ(pPSO->GetGraphicsPipelineDesc().RasterizerDesc.AntialiasedLineEnable, RSDesc.AntialiasedLineEnable);
    }
}

} // namespace
