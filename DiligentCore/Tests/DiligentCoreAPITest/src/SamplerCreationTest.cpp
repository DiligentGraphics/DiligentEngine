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

#include <algorithm>
#include <cctype>

#include "TestingEnvironment.hpp"
#include "Sampler.h"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

extern "C"
{
    int TestSamplerCInterface(void* pSampler);
}

namespace
{

class FilterTypeTest : public testing::TestWithParam<std::tuple<FILTER_TYPE, FILTER_TYPE, FILTER_TYPE>>
{
protected:
    static void TearDownTestSuite()
    {
        TestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_P(FilterTypeTest, CreateSampler)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    SamplerDesc SamplerDesc;
    const auto& Param     = GetParam();
    SamplerDesc.MinFilter = std::get<0>(Param);
    SamplerDesc.MagFilter = std::get<1>(Param);
    SamplerDesc.MipFilter = std::get<2>(Param);
    RefCntAutoPtr<ISampler> pSampler;
    pDevice->CreateSampler(SamplerDesc, &pSampler);
    ASSERT_TRUE(pSampler);
    EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);

    TestSamplerCInterface(pSampler);
}

std::string GetSamplerFilterTestName(const testing::TestParamInfo<std::tuple<FILTER_TYPE, FILTER_TYPE, FILTER_TYPE>>& info) //
{
    std::string name =
        std::string{GetFilterTypeLiteralName(std::get<0>(info.param), false)} + std::string{"__"} +
        std::string{GetFilterTypeLiteralName(std::get<1>(info.param), false)} + std::string{"__"} +
        std::string{GetFilterTypeLiteralName(std::get<2>(info.param), false)};
    for (size_t i = 0; i < name.length(); ++i)
    {
        if (!std::isalnum(name[i]))
            name[i] = '_';
    }
    return name;
}


INSTANTIATE_TEST_SUITE_P(RegularFitlers,
                         FilterTypeTest,
                         testing::Combine(
                             testing::Values(FILTER_TYPE_POINT, FILTER_TYPE_LINEAR),
                             testing::Values(FILTER_TYPE_POINT, FILTER_TYPE_LINEAR),
                             testing::Values(FILTER_TYPE_POINT, FILTER_TYPE_LINEAR)),
                         GetSamplerFilterTestName);

INSTANTIATE_TEST_SUITE_P(ComparisonFilters,
                         FilterTypeTest,
                         testing::Combine(
                             testing::Values(FILTER_TYPE_COMPARISON_POINT, FILTER_TYPE_COMPARISON_LINEAR),
                             testing::Values(FILTER_TYPE_COMPARISON_POINT, FILTER_TYPE_COMPARISON_LINEAR),
                             testing::Values(FILTER_TYPE_COMPARISON_POINT, FILTER_TYPE_COMPARISON_LINEAR)),
                         GetSamplerFilterTestName);

TEST(FilterTypeTest, AnisotropicFilter)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    {
        SamplerDesc SamplerDesc;
        SamplerDesc.MinFilter = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MagFilter = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MipFilter = FILTER_TYPE_ANISOTROPIC;

        SamplerDesc.MaxAnisotropy = 4;
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc, &pSampler);
        ASSERT_TRUE(pSampler);
        ASSERT_TRUE(pSampler);
        EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);
    }

    {
        SamplerDesc SamplerDesc;
        SamplerDesc.MinFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;
        SamplerDesc.MagFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;
        SamplerDesc.MipFilter = FILTER_TYPE_COMPARISON_ANISOTROPIC;

        SamplerDesc.MaxAnisotropy = 4;
        RefCntAutoPtr<ISampler> pSampler;
        pDevice->CreateSampler(SamplerDesc, &pSampler);
        ASSERT_TRUE(pSampler);
        ASSERT_TRUE(pSampler);
        EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);
    }
}


class AddressModeTest : public testing::TestWithParam<TEXTURE_ADDRESS_MODE>
{
protected:
    static void TearDownTestSuite()
    {
        TestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_P(AddressModeTest, CreateSampler)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    SamplerDesc SamplerDesc;
    SamplerDesc.MinFilter = FILTER_TYPE_LINEAR;
    SamplerDesc.MagFilter = FILTER_TYPE_LINEAR;
    SamplerDesc.MipFilter = FILTER_TYPE_LINEAR;
    SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = GetParam();
    RefCntAutoPtr<ISampler> pSampler;
    pDevice->CreateSampler(SamplerDesc, &pSampler);
    ASSERT_TRUE(pSampler);
    EXPECT_EQ(pSampler->GetDesc(), SamplerDesc);

    pEnv->ReleaseResources();
}

INSTANTIATE_TEST_SUITE_P(AddressModes,
                         AddressModeTest,
                         testing::Values(
                             TEXTURE_ADDRESS_WRAP,
                             TEXTURE_ADDRESS_MIRROR,
                             TEXTURE_ADDRESS_CLAMP,
                             TEXTURE_ADDRESS_BORDER),
                         [](const testing::TestParamInfo<TEXTURE_ADDRESS_MODE>& info) //
                         {
                             return std::string{GetTextureAddressModeLiteralName(info.param, true)};
                         } //
);


class ComparisonFuncTest : public testing::TestWithParam<COMPARISON_FUNCTION>
{
protected:
    static void TearDownTestSuite()
    {
        TestingEnvironment::GetInstance()->ReleaseResources();
    }
};

TEST_P(ComparisonFuncTest, CreateSampler)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    SamplerDesc SamplerDesc;
    SamplerDesc.MinFilter      = FILTER_TYPE_LINEAR;
    SamplerDesc.MagFilter      = FILTER_TYPE_LINEAR;
    SamplerDesc.MipFilter      = FILTER_TYPE_LINEAR;
    SamplerDesc.ComparisonFunc = GetParam();
    RefCntAutoPtr<ISampler> pSampler1, pSampler2;
    SamplerDesc.Name = "Sam1";
    pDevice->CreateSampler(SamplerDesc, &pSampler1);
    SamplerDesc.Name = "Sam2";
    pDevice->CreateSampler(SamplerDesc, &pSampler2);
    ASSERT_TRUE(pSampler1);
    ASSERT_TRUE(pSampler2);
    EXPECT_EQ(pSampler1, pSampler2);
    EXPECT_EQ(pSampler1->GetDesc(), SamplerDesc);
}

INSTANTIATE_TEST_SUITE_P(ComparisonFunctions,
                         ComparisonFuncTest,
                         testing::Values(
                             COMPARISON_FUNC_NEVER,
                             COMPARISON_FUNC_LESS,
                             COMPARISON_FUNC_EQUAL,
                             COMPARISON_FUNC_LESS_EQUAL,
                             COMPARISON_FUNC_GREATER,
                             COMPARISON_FUNC_NOT_EQUAL,
                             COMPARISON_FUNC_GREATER_EQUAL,
                             COMPARISON_FUNC_ALWAYS),
                         [](const testing::TestParamInfo<COMPARISON_FUNCTION>& info) //
                         {
                             return std::string{GetComparisonFunctionLiteralName(info.param, true)};
                         } //
);

} // namespace
