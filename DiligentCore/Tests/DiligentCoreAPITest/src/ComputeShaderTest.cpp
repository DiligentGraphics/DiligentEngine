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
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/ComputeShaderTestHLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void ComputeShaderReferenceD3D11(ISwapChain* pSwapChain);
#endif

#if D3D12_SUPPORTED
void ComputeShaderReferenceD3D12(ISwapChain* pSwapChain);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void ComputeShaderReferenceGL(ISwapChain* pSwapChain);
#endif

#if VULKAN_SUPPORTED
void ComputeShaderReferenceVk(ISwapChain* pSwapChain);
#endif

#if METAL_SUPPORTED
void ComputeShaderReferenceMtl(ISwapChain* pSwapChain);
#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(ComputeShaderTest, FillTexture)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceCaps().Features.ComputeShaders)
    {
        GTEST_SKIP() << "Compute shaders are not supported by this device";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (!pTestingSwapChain)
    {
        GTEST_SKIP() << "Compute shader test requires testing swap chain";
    }

    pContext->Flush();
    pContext->InvalidateState();

    auto deviceType = pDevice->GetDeviceCaps().DevType;
    switch (deviceType)
    {
#if D3D11_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D11:
            ComputeShaderReferenceD3D11(pSwapChain);
            break;
#endif

#if D3D12_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D12:
            ComputeShaderReferenceD3D12(pSwapChain);
            break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
            ComputeShaderReferenceGL(pSwapChain);
            break;

#endif

#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
            ComputeShaderReferenceVk(pSwapChain);
            break;
#endif

#if METAL_SUPPORTED
        case RENDER_DEVICE_TYPE_METAL:
            ComputeShaderReferenceMtl(pSwapChain);
            break;
#endif

        default:
            LOG_ERROR_AND_THROW("Unsupported device type");
    }

    pTestingSwapChain->TakeSnapshot();

    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
    ShaderCI.EntryPoint                 = "main";
    ShaderCI.Desc.Name                  = "Compute shader test";
    ShaderCI.Source                     = HLSL::FillTextureCS.c_str();
    RefCntAutoPtr<IShader> pCS;
    pDevice->CreateShader(ShaderCI, &pCS);
    ASSERT_NE(pCS, nullptr);

    ComputePipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Compute shader test";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    PSOCreateInfo.pCS                  = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    const auto& SCDesc = pSwapChain->GetDesc();

    pPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "g_tex2DUAV")->Set(pTestingSwapChain->GetCurrentBackBufferUAV());

    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    pPSO->CreateShaderResourceBinding(&pSRB, true);
    ASSERT_NE(pSRB, nullptr);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DispatchComputeAttribs DispatchAttribs;
    DispatchAttribs.ThreadGroupCountX = (SCDesc.Width + 15) / 16;
    DispatchAttribs.ThreadGroupCountY = (SCDesc.Height + 15) / 16;
    pContext->DispatchCompute(DispatchAttribs);

    pSwapChain->Present();
}

} // namespace
