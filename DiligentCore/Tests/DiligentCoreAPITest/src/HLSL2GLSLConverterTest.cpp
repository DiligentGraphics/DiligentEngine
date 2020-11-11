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
#include "HLSL2GLSLConverter.h"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

RefCntAutoPtr<IShader> CreateTestShader(const char*                  FileName,
                                        const char*                  EntryPoint,
                                        SHADER_TYPE                  ShaderType,
                                        IHLSL2GLSLConversionStream** ppConversionStream = nullptr)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    ShaderCreateInfo ShaderCI;

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory("shaders/HLSL2GLSLConverter", &pShaderSourceFactory);

    ShaderCI.FilePath                   = FileName;
    ShaderCI.EntryPoint                 = EntryPoint;
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.Desc.Name                  = "Test converted shader";
    ShaderCI.Desc.ShaderType            = ShaderType;
    ShaderCI.UseCombinedTextureSamplers = pDevice->GetDeviceCaps().IsGLDevice();
    ShaderCI.ppConversionStream         = ppConversionStream;

    RefCntAutoPtr<IShader> pShader;
    pDevice->CreateShader(ShaderCI, &pShader);

    return pShader;
}

TEST(HLSL2GLSLConverterTest, VS_PS)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    RefCntAutoPtr<IHLSL2GLSLConversionStream> pStream;

    auto pVS = CreateTestShader("VS_PS.hlsl", "TestVS", SHADER_TYPE_VERTEX, &pStream);
    EXPECT_NE(pVS, nullptr);
    auto pPS = CreateTestShader("VS_PS.hlsl", "TestPS", SHADER_TYPE_PIXEL, &pStream);
    EXPECT_NE(pPS, nullptr);
}

TEST(HLSL2GLSLConverterTest, CS_RWTex1D)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = TestingEnvironment::GetInstance();
    if (!pEnv->GetDevice()->GetDeviceCaps().Features.ComputeShaders)
    {
        GTEST_SKIP() << "This device does not support compute shaders";
    }

    auto pCS = CreateTestShader("CS_RWTex1D.hlsl", "TestCS", SHADER_TYPE_COMPUTE);
    EXPECT_NE(pCS, nullptr);
}

TEST(HLSL2GLSLConverterTest, CS_RWTex2D_1)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = TestingEnvironment::GetInstance();
    if (!pEnv->GetDevice()->GetDeviceCaps().Features.ComputeShaders)
    {
        GTEST_SKIP() << "This device does not support compute shaders";
    }

    auto pCS = CreateTestShader("CS_RWTex2D_1.hlsl", "TestCS", SHADER_TYPE_COMPUTE);
    EXPECT_NE(pCS, nullptr);
}

TEST(HLSL2GLSLConverterTest, CS_RWTex2D_2)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = TestingEnvironment::GetInstance();
    if (!pEnv->GetDevice()->GetDeviceCaps().Features.ComputeShaders)
    {
        GTEST_SKIP() << "This device does not support compute shaders";
    }

    auto pCS = CreateTestShader("CS_RWTex2D_2.hlsl", "TestCS", SHADER_TYPE_COMPUTE);
    EXPECT_NE(pCS, nullptr);
}

TEST(HLSL2GLSLConverterTest, CS_RWBuff)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pEnv = TestingEnvironment::GetInstance();
    if (!pEnv->GetDevice()->GetDeviceCaps().Features.ComputeShaders)
    {
        GTEST_SKIP() << "This device does not support compute shaders";
    }

    auto pCS = CreateTestShader("CS_RWBuff.hlsl", "TestCS", SHADER_TYPE_COMPUTE);
    EXPECT_NE(pCS, nullptr);
}

} // namespace
