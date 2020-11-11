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

#include "PSOTestBase.hpp"
#include "TestingEnvironment.hpp"

namespace Diligent
{

namespace Testing
{

PSOTestBase::Resources PSOTestBase::sm_Resources;

static const char g_ShaderSource[] = R"(
void VSMain(out float4 pos : SV_POSITION)
{
	pos = float4(0.0, 0.0, 0.0, 0.0);
}

void PSMain(out float4 col : SV_TARGET)
{
	col = float4(0.0, 0.0, 0.0, 0.0);
}
)";

void PSOTestBase::InitResources()
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    ShaderCreateInfo Attrs;
    Attrs.Source                     = g_ShaderSource;
    Attrs.EntryPoint                 = "VSMain";
    Attrs.Desc.ShaderType            = SHADER_TYPE_VERTEX;
    Attrs.Desc.Name                  = "TrivialVS (TestPipelineStateBase)";
    Attrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    Attrs.ShaderCompiler             = pEnv->GetDefaultCompiler(Attrs.SourceLanguage);
    Attrs.UseCombinedTextureSamplers = true;
    pDevice->CreateShader(Attrs, &sm_Resources.pTrivialVS);

    Attrs.EntryPoint      = "PSMain";
    Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
    Attrs.Desc.Name       = "TrivialPS (TestPipelineStateBase)";
    pDevice->CreateShader(Attrs, &sm_Resources.pTrivialPS);

    auto& PSOCreateInfo = sm_Resources.PSOCreateInfo;

    PSOCreateInfo.pVS                                = sm_Resources.pTrivialVS;
    PSOCreateInfo.pPS                                = sm_Resources.pTrivialPS;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets  = 1;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
    PSOCreateInfo.GraphicsPipeline.DSVFormat         = TEX_FORMAT_D32_FLOAT;
}

void PSOTestBase::ReleaseResources()
{
    sm_Resources = Resources{};
    TestingEnvironment::GetInstance()->Reset();
}

RefCntAutoPtr<IPipelineState> PSOTestBase::CreateTestPSO(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, bool BindPSO)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    if (BindPSO && pPSO)
    {
        pContext->SetPipelineState(pPSO);
    }
    return pPSO;
}

} // namespace Testing

} // namespace Diligent
