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

#include "InlineShaders/MeshShaderTestHLSL.h"
#include "InlineShaders/MeshShaderTestGLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D12_SUPPORTED
void MeshShaderDrawReferenceD3D12(ISwapChain* pSwapChain);
void MeshShaderIndirectDrawReferenceD3D12(ISwapChain* pSwapChain);
void AmplificationShaderDrawReferenceD3D12(ISwapChain* pSwapChain);
#endif

#if VULKAN_SUPPORTED
void MeshShaderDrawReferenceVk(ISwapChain* pSwapChain);
void MeshShaderIndirectDrawReferenceVk(ISwapChain* pSwapChain);
void AmplificationShaderDrawReferenceVk(ISwapChain* pSwapChain);
#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(MeshShaderTest, DrawQuad)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceCaps().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pConext    = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pConext->Flush();
        pConext->InvalidateState();

        auto deviceType = pDevice->GetDeviceCaps().DevType;
        switch (deviceType)
        {
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                MeshShaderDrawReferenceD3D12(pSwapChain);
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                MeshShaderDrawReferenceVk(pSwapChain);
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

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Mesh shader test";

    PSODesc.PipelineType                                  = PIPELINE_TYPE_MESH;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_UNDEFINED; // unused
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    const bool IsVulkan = pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_VULKAN;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = IsVulkan ? SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM : SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = IsVulkan ? SHADER_COMPILER_DEFAULT : SHADER_COMPILER_DXC;
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pMS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - MS";
        ShaderCI.Source          = IsVulkan ? GLSL::MeshShaderTest_MS.c_str() : HLSL::MeshShaderTest_MS.c_str();

        pDevice->CreateShader(ShaderCI, &pMS);
        ASSERT_NE(pMS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - PS";
        ShaderCI.Source          = IsVulkan ? GLSL::MeshShaderTest_FS.c_str() : HLSL::MeshShaderTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSODesc.Name = "Mesh shader test";

    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    pContext->SetPipelineState(pPSO);

    DrawMeshAttribs drawAttrs(1, DRAW_FLAG_VERIFY_ALL);
    pContext->DrawMesh(drawAttrs);

    pSwapChain->Present();
}

TEST(MeshShaderTest, DrawQuadIndirect)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceCaps().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pConext    = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pConext->Flush();
        pConext->InvalidateState();

        auto deviceType = pDevice->GetDeviceCaps().DevType;
        switch (deviceType)
        {
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                MeshShaderIndirectDrawReferenceD3D12(pSwapChain);
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                MeshShaderIndirectDrawReferenceVk(pSwapChain);
                break;
#endif

            case RENDER_DEVICE_TYPE_D3D11:
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
            case RENDER_DEVICE_TYPE_METAL:
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Mesh shader test";

    PSODesc.PipelineType                                  = PIPELINE_TYPE_MESH;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = pDevice->GetDeviceCaps().IsGLDevice();

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    const bool IsVulkan = pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_VULKAN;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = IsVulkan ? SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM : SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = IsVulkan ? SHADER_COMPILER_DEFAULT : SHADER_COMPILER_DXC;
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pMS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - MS";
        ShaderCI.Source          = IsVulkan ? GLSL::MeshShaderTest_MS.c_str() : HLSL::MeshShaderTest_MS.c_str();

        pDevice->CreateShader(ShaderCI, &pMS);
        ASSERT_NE(pMS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Mesh shader test - PS";
        ShaderCI.Source          = IsVulkan ? GLSL::MeshShaderTest_FS.c_str() : HLSL::MeshShaderTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    Uint32 IndirectBufferData[3];

    if (pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_VULKAN)
    {
        IndirectBufferData[0] = 1; // TaskCount
        IndirectBufferData[1] = 0; // FirstTask
    }
    else
    {
        IndirectBufferData[0] = 1; // ThreadGroupCountX
        IndirectBufferData[1] = 1; // ThreadGroupCountY
        IndirectBufferData[2] = 1; // ThreadGroupCountZ
    }

    BufferDesc IndirectBufferDesc;
    IndirectBufferDesc.Name          = "Indirect buffer";
    IndirectBufferDesc.Usage         = USAGE_IMMUTABLE;
    IndirectBufferDesc.uiSizeInBytes = sizeof(IndirectBufferData);
    IndirectBufferDesc.BindFlags     = BIND_INDIRECT_DRAW_ARGS;

    BufferData InitData;
    InitData.pData    = &IndirectBufferData;
    InitData.DataSize = IndirectBufferDesc.uiSizeInBytes;

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(IndirectBufferDesc, &InitData, &pBuffer);

    pContext->SetPipelineState(pPSO);

    DrawMeshIndirectAttribs drawAttrs(DRAW_FLAG_VERIFY_ALL, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->DrawMeshIndirect(drawAttrs, pBuffer);

    pSwapChain->Present();
}

TEST(MeshShaderTest, DrawQuadsWithAmplificationShader)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceCaps().Features.MeshShaders)
    {
        GTEST_SKIP() << "Mesh shader is not supported by this device";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pConext    = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pConext->Flush();
        pConext->InvalidateState();

        auto deviceType = pDevice->GetDeviceCaps().DevType;
        switch (deviceType)
        {
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                AmplificationShaderDrawReferenceD3D12(pSwapChain);
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                AmplificationShaderDrawReferenceVk(pSwapChain);
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

    float ClearColor[] = {0.f, 0.f, 0.f, 0.f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Amplification shader test";

    PSODesc.PipelineType                                  = PIPELINE_TYPE_MESH;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.RTVFormats[0]                        = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_UNDEFINED; // unused
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    const bool IsVulkan = pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_VULKAN;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage             = IsVulkan ? SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM : SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler             = IsVulkan ? SHADER_COMPILER_DEFAULT : SHADER_COMPILER_DXC;
    ShaderCI.UseCombinedTextureSamplers = true;

    RefCntAutoPtr<IShader> pAS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_AMPLIFICATION;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Amplification shader test - AS";
        ShaderCI.Source          = IsVulkan ? GLSL::AmplificationShaderTest_TS.c_str() : HLSL::AmplificationShaderTest_AS.c_str();

        pDevice->CreateShader(ShaderCI, &pAS);
        ASSERT_NE(pAS, nullptr);
    }

    RefCntAutoPtr<IShader> pMS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_MESH;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Amplification shader test - MS";
        ShaderCI.Source          = IsVulkan ? GLSL::AmplificationShaderTest_MS.c_str() : HLSL::AmplificationShaderTest_MS.c_str();

        pDevice->CreateShader(ShaderCI, &pMS);
        ASSERT_NE(pMS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Amplification shader test - PS";
        ShaderCI.Source          = IsVulkan ? GLSL::AmplificationShaderTest_FS.c_str() : HLSL::AmplificationShaderTest_PS.c_str();

        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSOCreateInfo.pAS = pAS;
    PSOCreateInfo.pMS = pMS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    pContext->SetPipelineState(pPSO);

    DrawMeshAttribs drawAttrs(8, DRAW_FLAG_VERIFY_ALL);
    pContext->DrawMesh(drawAttrs);

    pSwapChain->Present();
}

} // namespace
