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

static const char g_BrokenShaderSource[] = R"(
void VSMain(out float4 pos : SV_POSITION)
{
    pos = float3(0.0, 0.0, 0.0, 0.0);
}
)";

TEST(Shader, CompilationFailure)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    ShaderCreateInfo Attrs;
    Attrs.Source                     = g_BrokenShaderSource;
    Attrs.EntryPoint                 = "VSMain";
    Attrs.Desc.ShaderType            = SHADER_TYPE_VERTEX;
    Attrs.Desc.Name                  = "Broken shader test";
    Attrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    Attrs.ShaderCompiler             = pEnv->GetDefaultCompiler(Attrs.SourceLanguage);
    Attrs.UseCombinedTextureSamplers = true;

    IDataBlob* pErrors     = nullptr;
    Attrs.ppCompilerOutput = &pErrors;

    pEnv->SetErrorAllowance(pDevice->GetDeviceCaps().IsVulkanDevice() ? 3 : 2, "\n\nNo worries, testing broken shader...\n\n");
    RefCntAutoPtr<IShader> pBrokenShader;
    pDevice->CreateShader(Attrs, &pBrokenShader);
    EXPECT_FALSE(pBrokenShader);
    EXPECT_TRUE(pErrors);
    const char* Msg = reinterpret_cast<const char*>(pErrors->GetDataPtr());
    LOG_INFO_MESSAGE("Compiler output:\n", Msg);
    pErrors->Release();
}

} // namespace
