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

#include "InlineShaders/TessellationTestHLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void TessellationReferenceD3D11(ISwapChain* pSwapChain);
#endif

#if D3D12_SUPPORTED
void TessellationReferenceD3D12(ISwapChain* pSwapChain);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void TessellationReferenceGL(ISwapChain* pSwapChain);
#endif

#if VULKAN_SUPPORTED
void TessellationReferenceVk(ISwapChain* pSwapChain);
#endif

#if METAL_SUPPORTED

#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(TessellationTest, DrawQuad)
{
    auto* const pEnv       = TestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& deviceCaps = pDevice->GetDeviceCaps();

    if (!deviceCaps.Features.Tessellation)
    {
        GTEST_SKIP() << "Tessellation is not supported by this device";
    }

    if (!deviceCaps.Features.SeparablePrograms)
    {
        GTEST_SKIP() << "Tessellation test requires separable programs";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pConext    = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pConext->Flush();
        pConext->InvalidateState();

        switch (deviceCaps.DevType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                TessellationReferenceD3D11(pSwapChain);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                TessellationReferenceD3D12(pSwapChain);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                TessellationReferenceGL(pSwapChain);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                TessellationReferenceVk(pSwapChain);
                break;

#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.0f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name         = "Tessellation test";
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    auto& GraphicsPipeline                   = PSOCreateInfo.GraphicsPipeline;
    GraphicsPipeline.NumRenderTargets        = 1;
    GraphicsPipeline.RTVFormats[0]           = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology       = PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode =
        pDevice->GetDeviceCaps().Features.WireframeFill ?
        FILL_MODE_WIREFRAME :
        FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = pDevice->GetDeviceCaps().IsGLDevice();

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Tessellation test - VS";
        ShaderCI.Source          = HLSL::TessTest_VS.c_str();

        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pHS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_HULL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Tessellation test - HS";
        ShaderCI.Source          = HLSL::TessTest_HS.c_str();

        pDevice->CreateShader(ShaderCI, &pHS);
        ASSERT_NE(pHS, nullptr);
    }

    RefCntAutoPtr<IShader> pDS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_DOMAIN;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Tessellation test - DS";
        ShaderCI.Source          = HLSL::TessTest_DS.c_str();

        pDevice->CreateShader(ShaderCI, &pDS);
        ASSERT_NE(pDS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Tessellation test - PS";
        ShaderCI.Source          = HLSL::TessTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.PSODesc.Name = "Tessellation test";

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pHS = pHS;
    PSOCreateInfo.pDS = pDS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    pContext->SetPipelineState(pPSO);

    DrawAttribs drawAttrs(2, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(drawAttrs);

    pSwapChain->Present();
}

} // namespace
