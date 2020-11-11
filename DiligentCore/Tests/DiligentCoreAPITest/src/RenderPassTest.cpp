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

#include "TestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/DrawCommandTestGLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void RenderDrawCommandReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassMSResolveReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if D3D12_SUPPORTED
void RenderDrawCommandReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassMSResolveReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void RenderDrawCommandReferenceGL(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassMSResolveReferenceGL(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceGL(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if VULKAN_SUPPORTED
void RenderDrawCommandReferenceVk(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassMSResolveReferenceVk(ISwapChain* pSwapChain, const float* pClearColor);
void RenderPassInputAttachmentReferenceVk(ISwapChain* pSwapChain, const float* pClearColor);
#endif

#if METAL_SUPPORTED

#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

#include "InlineShaders/DrawCommandTestHLSL.h"

namespace
{


class RenderPassTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        ShaderCI.UseCombinedTextureSamplers = true;

        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Render pass test vertex shader";
            ShaderCI.Source          = HLSL::DrawTest_ProceduralTriangleVS.c_str();
            pDevice->CreateShader(ShaderCI, &sm_pVS);
            ASSERT_NE(sm_pVS, nullptr);
        }

        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Render pass test pixel shader";
            ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &sm_pPS);
            ASSERT_NE(sm_pPS, nullptr);
        }
    }

    static void TearDownTestSuite()
    {
        sm_pVS.Release();
        sm_pPS.Release();

        auto* pEnv = TestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static void CreateDrawTrisPSO(IRenderPass*                           pRenderPass,
                                  Uint8                                  SampleCount,
                                  RefCntAutoPtr<IPipelineState>&         pPSO,
                                  RefCntAutoPtr<IShaderResourceBinding>& pSRB)
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Render pass test - draw triangles";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.pRenderPass                  = pRenderPass;
        GraphicsPipeline.SubpassIndex                 = 0;
        GraphicsPipeline.SmplDesc.Count               = SampleCount;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        PSOCreateInfo.pVS = sm_pVS;
        PSOCreateInfo.pPS = sm_pPS;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        ASSERT_NE(pPSO, nullptr);
        pPSO->CreateShaderResourceBinding(&pSRB);
        ASSERT_NE(pSRB, nullptr);
    }

    static void DrawTris(IRenderPass*            pRenderPass,
                         IFramebuffer*           pFramebuffer,
                         IPipelineState*         pPSO,
                         IShaderResourceBinding* pSRB,
                         const float             ClearColor[])
    {
        auto* pEnv     = TestingEnvironment::GetInstance();
        auto* pContext = pEnv->GetDeviceContext();

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        BeginRenderPassAttribs RPBeginInfo;
        RPBeginInfo.pRenderPass  = pRenderPass;
        RPBeginInfo.pFramebuffer = pFramebuffer;

        OptimizedClearValue ClearValues[1];
        ClearValues[0].Color[0] = ClearColor[0];
        ClearValues[0].Color[1] = ClearColor[1];
        ClearValues[0].Color[2] = ClearColor[2];
        ClearValues[0].Color[3] = ClearColor[3];

        RPBeginInfo.pClearValues        = ClearValues;
        RPBeginInfo.ClearValueCount     = _countof(ClearValues);
        RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        pContext->BeginRenderPass(RPBeginInfo);

        DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(DrawAttrs);

        pContext->EndRenderPass();
    }

    static void Present()
    {
        auto* pEnv       = TestingEnvironment::GetInstance();
        auto* pSwapChain = pEnv->GetSwapChain();
        auto* pContext   = pEnv->GetDeviceContext();

        pSwapChain->Present();

        pContext->Flush();
        pContext->InvalidateState();
    }

    static RefCntAutoPtr<IShader> sm_pVS;
    static RefCntAutoPtr<IShader> sm_pPS;
};

RefCntAutoPtr<IShader> RenderPassTest::sm_pVS;
RefCntAutoPtr<IShader> RenderPassTest::sm_pPS;

TEST_F(RenderPassTest, CreateRenderPassAndFramebuffer)
{
    auto*      pDevice    = TestingEnvironment::GetInstance()->GetDevice();
    auto*      pContext   = TestingEnvironment::GetInstance()->GetDeviceContext();
    const auto DeviceType = pDevice->GetDeviceCaps().DevType;

    RenderPassAttachmentDesc Attachments[6];
    Attachments[0].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[0].SampleCount  = 4;
    Attachments[0].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_LOAD;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    Attachments[1].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[1].SampleCount  = 4;
    Attachments[1].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[1].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[2].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[2].SampleCount  = 1;
    Attachments[2].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[2].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[2].LoadOp       = ATTACHMENT_LOAD_OP_DISCARD;
    Attachments[2].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    Attachments[3].Format         = TEX_FORMAT_D32_FLOAT_S8X24_UINT;
    Attachments[3].SampleCount    = 4;
    Attachments[3].InitialState   = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[3].FinalState     = RESOURCE_STATE_DEPTH_WRITE;
    Attachments[3].LoadOp         = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[3].StoreOp        = ATTACHMENT_STORE_OP_DISCARD;
    Attachments[3].StencilLoadOp  = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[3].StencilStoreOp = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[4].Format       = TEX_FORMAT_RGBA32_FLOAT;
    Attachments[4].SampleCount  = 1;
    Attachments[4].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[4].FinalState   = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[4].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[4].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    Attachments[5].Format       = TEX_FORMAT_RGBA8_UNORM;
    Attachments[5].SampleCount  = 1;
    Attachments[5].InitialState = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[5].FinalState   = RESOURCE_STATE_SHADER_RESOURCE;
    Attachments[5].LoadOp       = ATTACHMENT_LOAD_OP_LOAD;
    Attachments[5].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[2];

    // clang-format off
    AttachmentReference RTAttachmentRefs0[] = 
    {
        {0, RESOURCE_STATE_RENDER_TARGET},
        {1, RESOURCE_STATE_RENDER_TARGET}
    };
    AttachmentReference RslvAttachmentRefs0[] = 
    {
        {ATTACHMENT_UNUSED, RESOURCE_STATE_RESOLVE_DEST},
        {2, RESOURCE_STATE_RESOLVE_DEST}
    };
    // clang-format on
    AttachmentReference DSAttachmentRef0{3, RESOURCE_STATE_DEPTH_WRITE};
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;
    Subpasses[0].pResolveAttachments         = RslvAttachmentRefs0;
    Subpasses[0].pDepthStencilAttachment     = &DSAttachmentRef0;

    // clang-format off
    AttachmentReference RTAttachmentRefs1[] = 
    {
        {4, RESOURCE_STATE_RENDER_TARGET}
    };
    AttachmentReference InptAttachmentRefs1[] = 
    {
        {2, RESOURCE_STATE_INPUT_ATTACHMENT},
        {5, RESOURCE_STATE_INPUT_ATTACHMENT}
    };
    Uint32 PrsvAttachmentRefs1[] =
    {
        0
    };
    // clang-format on
    Subpasses[1].InputAttachmentCount        = _countof(InptAttachmentRefs1);
    Subpasses[1].pInputAttachments           = InptAttachmentRefs1;
    Subpasses[1].RenderTargetAttachmentCount = _countof(RTAttachmentRefs1);
    Subpasses[1].pRenderTargetAttachments    = RTAttachmentRefs1;
    Subpasses[1].PreserveAttachmentCount     = _countof(PrsvAttachmentRefs1);
    Subpasses[1].pPreserveAttachments        = PrsvAttachmentRefs1;

    SubpassDependencyDesc Dependencies[2] = {};
    Dependencies[0].SrcSubpass            = 0;
    Dependencies[0].DstSubpass            = 1;
    Dependencies[0].SrcStageMask          = PIPELINE_STAGE_FLAG_VERTEX_SHADER;
    Dependencies[0].DstStageMask          = PIPELINE_STAGE_FLAG_PIXEL_SHADER;
    Dependencies[0].SrcAccessMask         = ACCESS_FLAG_SHADER_WRITE;
    Dependencies[0].DstAccessMask         = ACCESS_FLAG_SHADER_READ;

    Dependencies[1].SrcSubpass    = 0;
    Dependencies[1].DstSubpass    = 1;
    Dependencies[1].SrcStageMask  = PIPELINE_STAGE_FLAG_VERTEX_INPUT;
    Dependencies[1].DstStageMask  = PIPELINE_STAGE_FLAG_PIXEL_SHADER;
    Dependencies[1].SrcAccessMask = ACCESS_FLAG_INDEX_READ;
    Dependencies[1].DstAccessMask = ACCESS_FLAG_SHADER_READ;


    RenderPassDesc RPDesc;
    RPDesc.Name            = "Test render pass";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;
    RPDesc.DependencyCount = _countof(Dependencies);
    RPDesc.pDependencies   = Dependencies;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    const auto& RPDesc2 = pRenderPass->GetDesc();
    EXPECT_EQ(RPDesc.AttachmentCount, RPDesc2.AttachmentCount);
    for (Uint32 i = 0; i < std::min(RPDesc.AttachmentCount, RPDesc2.AttachmentCount); ++i)
        EXPECT_EQ(RPDesc.pAttachments[i], RPDesc2.pAttachments[i]);

    EXPECT_EQ(RPDesc.SubpassCount, RPDesc2.SubpassCount);
    if (DeviceType != RENDER_DEVICE_TYPE_VULKAN)
    {
        for (Uint32 i = 0; i < std::min(RPDesc.SubpassCount, RPDesc2.SubpassCount); ++i)
            EXPECT_EQ(RPDesc.pSubpasses[i], RPDesc2.pSubpasses[i]);
    }
    else
    {
        auto CompareSubpassDescVk = [](const SubpassDesc& SP1, const SubpassDesc& SP2) //
        {
            if (SP1.InputAttachmentCount != SP2.InputAttachmentCount ||
                SP1.RenderTargetAttachmentCount != SP2.RenderTargetAttachmentCount ||
                SP1.PreserveAttachmentCount != SP2.PreserveAttachmentCount)
                return false;

            for (Uint32 i = 0; i < SP1.InputAttachmentCount; ++i)
            {
                if (SP1.pInputAttachments[i] != SP2.pInputAttachments[i])
                    return false;
            }

            for (Uint32 i = 0; i < SP1.RenderTargetAttachmentCount; ++i)
            {
                if (SP1.pRenderTargetAttachments[i] != SP2.pRenderTargetAttachments[i])
                    return false;
            }

            if ((SP1.pResolveAttachments == nullptr && SP2.pResolveAttachments != nullptr) ||
                (SP1.pResolveAttachments != nullptr && SP2.pResolveAttachments == nullptr))
                return false;

            if (SP1.pResolveAttachments != nullptr && SP2.pResolveAttachments != nullptr)
            {
                for (Uint32 i = 0; i < SP1.RenderTargetAttachmentCount; ++i)
                {
                    if (SP1.pResolveAttachments[i].AttachmentIndex != SP2.pResolveAttachments[i].AttachmentIndex)
                        return false;

                    if (!((SP1.pResolveAttachments[i].State == SP2.pResolveAttachments[i].State) ||
                          (SP1.pResolveAttachments[i].State == RESOURCE_STATE_RESOLVE_DEST && SP2.pResolveAttachments[i].State == RESOURCE_STATE_RENDER_TARGET)))
                        return false;
                }
            }

            if ((SP1.pDepthStencilAttachment == nullptr && SP2.pDepthStencilAttachment != nullptr) ||
                (SP1.pDepthStencilAttachment != nullptr && SP2.pDepthStencilAttachment == nullptr))
                return false;

            if (SP1.pDepthStencilAttachment != nullptr && SP2.pDepthStencilAttachment != nullptr)
            {
                if (*SP1.pDepthStencilAttachment != *SP2.pDepthStencilAttachment)
                    return false;
            }

            if ((SP1.pPreserveAttachments == nullptr && SP2.pPreserveAttachments != nullptr) ||
                (SP1.pPreserveAttachments != nullptr && SP2.pPreserveAttachments == nullptr))
                return false;

            if (SP1.pPreserveAttachments != nullptr && SP2.pPreserveAttachments != nullptr)
            {
                for (Uint32 i = 0; i < SP1.PreserveAttachmentCount; ++i)
                {
                    if (SP1.pPreserveAttachments[i] != SP2.pPreserveAttachments[i])
                        return false;
                }
            }

            return true;
        };
        // Resolve attachment states may be corrected in Vulkan, so we can't use comparison operator
        for (Uint32 i = 0; i < std::min(RPDesc.SubpassCount, RPDesc2.SubpassCount); ++i)
        {
            EXPECT_TRUE(CompareSubpassDescVk(RPDesc.pSubpasses[i], RPDesc2.pSubpasses[i]));
        }
    }

    EXPECT_EQ(RPDesc.DependencyCount, RPDesc2.DependencyCount);
    for (Uint32 i = 0; i < std::min(RPDesc.DependencyCount, RPDesc2.DependencyCount); ++i)
        EXPECT_EQ(RPDesc.pDependencies[i], RPDesc2.pDependencies[i]);

    RefCntAutoPtr<ITexture> pTextures[_countof(Attachments)];
    ITextureView*           pTexViews[_countof(Attachments)] = {};
    for (Uint32 i = 0; i < _countof(pTextures); ++i)
    {
        TextureDesc TexDesc;
        std::string Name = "Test framebuffer attachment ";
        Name += std::to_string(i);
        TexDesc.Name        = Name.c_str();
        TexDesc.Type        = RESOURCE_DIM_TEX_2D;
        TexDesc.Format      = Attachments[i].Format;
        TexDesc.Width       = 1024;
        TexDesc.Height      = 1024;
        TexDesc.SampleCount = Attachments[i].SampleCount;

        const auto FmtAttribs = pDevice->GetTextureFormatInfo(TexDesc.Format);
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH ||
            FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            TexDesc.BindFlags = BIND_DEPTH_STENCIL;
        else
            TexDesc.BindFlags = BIND_RENDER_TARGET;

        if (i == 2 || i == 5)
            TexDesc.BindFlags |= BIND_INPUT_ATTACHMENT;

        const auto InitialState = Attachments[i].InitialState;
        if (InitialState == RESOURCE_STATE_SHADER_RESOURCE)
            TexDesc.BindFlags |= BIND_SHADER_RESOURCE;

        pDevice->CreateTexture(TexDesc, nullptr, &pTextures[i]);

        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH ||
            FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            pTexViews[i] = pTextures[i]->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        else
            pTexViews[i] = pTextures[i]->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    }

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pTexViews;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    const auto& FBDesc2 = pFramebuffer->GetDesc();
    EXPECT_EQ(FBDesc2.AttachmentCount, FBDesc.AttachmentCount);
    for (Uint32 i = 0; i < std::min(FBDesc.AttachmentCount, FBDesc2.AttachmentCount); ++i)
        EXPECT_EQ(FBDesc2.ppAttachments[i], FBDesc.ppAttachments[i]);

    BeginRenderPassAttribs RPBeginInfo;
    RPBeginInfo.pRenderPass  = pRenderPass;
    RPBeginInfo.pFramebuffer = pFramebuffer;
    OptimizedClearValue ClearValues[5];
    RPBeginInfo.pClearValues        = ClearValues;
    RPBeginInfo.ClearValueCount     = _countof(ClearValues);
    RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    pContext->BeginRenderPass(RPBeginInfo);

    if (DeviceType != RENDER_DEVICE_TYPE_D3D12)
    {
        // ClearDepthStencil is not allowed inside a render pass in Direct3D12
        pContext->ClearDepthStencil(pTexViews[3], CLEAR_DEPTH_FLAG, 1.0, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }

    pContext->NextSubpass();

    if (DeviceType != RENDER_DEVICE_TYPE_D3D12)
    {
        // ClearRenderTarget is not allowed inside a render pass in Direct3D12
        float ClearColor[] = {0, 0, 0, 0};
        pContext->ClearRenderTarget(pTexViews[4], ClearColor, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }

    pContext->EndRenderPass();
}

TEST_F(RenderPassTest, Draw)
{
    auto* pEnv       = TestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    constexpr float ClearColor[] = {0.2f, 0.375f, 0.5f, 0.75f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceCaps().DevType;
        switch (deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                RenderDrawCommandReferenceD3D11(pSwapChain, ClearColor);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                RenderDrawCommandReferenceD3D12(pSwapChain, ClearColor);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                RenderDrawCommandReferenceGL(pSwapChain, ClearColor);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                RenderDrawCommandReferenceVk(pSwapChain, ClearColor);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    const auto&              SCDesc = pSwapChain->GetDesc();
    RenderPassAttachmentDesc Attachments[1];
    Attachments[0].Format       = SCDesc.ColorBufferFormat;
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[1];

    // clang-format off
    AttachmentReference RTAttachmentRefs0[] = 
    {
        {0, RESOURCE_STATE_RENDER_TARGET}
    };
    // clang-format on
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Render pass draw test";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateDrawTrisPSO(pRenderPass, 1, pPSO, pSRB);
    ASSERT_TRUE(pPSO != nullptr && pSRB != nullptr);

    ITextureView* pRTAttachments[] = {pSwapChain->GetCurrentBackBufferRTV()};

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Render pass draw test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pRTAttachments;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    DrawTris(pRenderPass, pFramebuffer, pPSO, pSRB, ClearColor);

    Present();
}

TEST_F(RenderPassTest, MSResolve)
{
    auto* pEnv       = TestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    constexpr float ClearColor[] = {0.25f, 0.5f, 0.375f, 0.5f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceCaps().DevType;
        switch (deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                RenderPassMSResolveReferenceD3D11(pSwapChain, ClearColor);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                RenderPassMSResolveReferenceD3D12(pSwapChain, ClearColor);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                RenderPassMSResolveReferenceGL(pSwapChain, ClearColor);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                RenderPassMSResolveReferenceVk(pSwapChain, ClearColor);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    const auto& SCDesc = pSwapChain->GetDesc();

    RenderPassAttachmentDesc Attachments[2];
    Attachments[0].Format       = SCDesc.ColorBufferFormat;
    Attachments[0].SampleCount  = 4;
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[1].Format       = SCDesc.ColorBufferFormat;
    Attachments[1].SampleCount  = 1;
    Attachments[1].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_DISCARD;
    Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    RefCntAutoPtr<ITexture> pMSTex;
    {
        TextureDesc TexDesc;
        TexDesc.Type        = RESOURCE_DIM_TEX_2D;
        TexDesc.Format      = SCDesc.ColorBufferFormat;
        TexDesc.Width       = SCDesc.Width;
        TexDesc.Height      = SCDesc.Height;
        TexDesc.BindFlags   = BIND_RENDER_TARGET;
        TexDesc.MipLevels   = 1;
        TexDesc.SampleCount = Attachments[0].SampleCount;
        TexDesc.Usage       = USAGE_DEFAULT;

        pDevice->CreateTexture(TexDesc, nullptr, &pMSTex);
        ASSERT_NE(pMSTex, nullptr);
    }

    SubpassDesc Subpasses[1];

    // clang-format off
    AttachmentReference RTAttachmentRefs0[] = 
    {
        {0, RESOURCE_STATE_RENDER_TARGET}
    };
    AttachmentReference RslvAttachmentRefs0[] = 
    {
        {1, RESOURCE_STATE_RESOLVE_DEST}
    };
    // clang-format on
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;
    Subpasses[0].pResolveAttachments         = RslvAttachmentRefs0;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Render pass MS resolve test";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateDrawTrisPSO(pRenderPass, 4, pPSO, pSRB);
    ASSERT_TRUE(pPSO != nullptr && pSRB != nullptr);

    ITextureView* pRTAttachments[] = //
        {
            pMSTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
            pSwapChain->GetCurrentBackBufferRTV() //
        };

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Render pass resolve test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pRTAttachments;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    DrawTris(pRenderPass, pFramebuffer, pPSO, pSRB, ClearColor);

    Present();
}

TEST_F(RenderPassTest, InputAttachment)
{
    auto* pEnv       = TestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    constexpr float ClearColor[] = {0.5f, 0.125f, 0.25f, 0.25f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto deviceType = pDevice->GetDeviceCaps().DevType;
        switch (deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                RenderPassInputAttachmentReferenceD3D11(pSwapChain, ClearColor);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                RenderPassInputAttachmentReferenceD3D12(pSwapChain, ClearColor);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                RenderPassInputAttachmentReferenceGL(pSwapChain, ClearColor);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                RenderPassInputAttachmentReferenceVk(pSwapChain, ClearColor);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    const auto& SCDesc = pSwapChain->GetDesc();

    RenderPassAttachmentDesc Attachments[2];
    Attachments[0].Format       = SCDesc.ColorBufferFormat;
    Attachments[0].SampleCount  = 1;
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_INPUT_ATTACHMENT;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_DISCARD;

    Attachments[1].Format       = SCDesc.ColorBufferFormat;
    Attachments[1].SampleCount  = 1;
    Attachments[1].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[1].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[1].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    RefCntAutoPtr<ITexture> pTex;
    {
        TextureDesc TexDesc;
        TexDesc.Name      = "Input attachment test texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = SCDesc.ColorBufferFormat;
        TexDesc.Width     = SCDesc.Width;
        TexDesc.Height    = SCDesc.Height;
        TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_INPUT_ATTACHMENT;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;

        pDevice->CreateTexture(TexDesc, nullptr, &pTex);
        ASSERT_NE(pTex, nullptr);
    }

    SubpassDesc Subpasses[2];

    // clang-format off
    AttachmentReference RTAttachmentRefs0[] =
    {
        {0, RESOURCE_STATE_RENDER_TARGET}
    };
    AttachmentReference RTAttachmentRefs1[] =
    {
        {1, RESOURCE_STATE_RENDER_TARGET}
    };
    AttachmentReference InputAttachmentRefs1[] =
    {
        {0, RESOURCE_STATE_INPUT_ATTACHMENT}
    };
    // clang-format on
    Subpasses[0].RenderTargetAttachmentCount = _countof(RTAttachmentRefs0);
    Subpasses[0].pRenderTargetAttachments    = RTAttachmentRefs0;

    Subpasses[1].RenderTargetAttachmentCount = _countof(RTAttachmentRefs1);
    Subpasses[1].pRenderTargetAttachments    = RTAttachmentRefs1;
    Subpasses[1].InputAttachmentCount        = _countof(InputAttachmentRefs1);
    Subpasses[1].pInputAttachments           = InputAttachmentRefs1;

    SubpassDependencyDesc Dependencies[1];
    Dependencies[0].SrcSubpass    = 0;
    Dependencies[0].DstSubpass    = 1;
    Dependencies[0].SrcStageMask  = PIPELINE_STAGE_FLAG_RENDER_TARGET;
    Dependencies[0].DstStageMask  = PIPELINE_STAGE_FLAG_PIXEL_SHADER;
    Dependencies[0].SrcAccessMask = ACCESS_FLAG_RENDER_TARGET_WRITE;
    Dependencies[0].DstAccessMask = ACCESS_FLAG_SHADER_READ;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Render pass input attachment test";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;
    RPDesc.DependencyCount = _countof(Dependencies);
    RPDesc.pDependencies   = Dependencies;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateDrawTrisPSO(pRenderPass, 1, pPSO, pSRB);
    ASSERT_TRUE(pPSO != nullptr && pSRB != nullptr);

    RefCntAutoPtr<IPipelineState>         pInputAttachmentPSO;
    RefCntAutoPtr<IShaderResourceBinding> pInputAttachmentSRB;
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Render pass test - input attachment";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.pRenderPass                  = pRenderPass;
        GraphicsPipeline.SubpassIndex                 = 1;
        GraphicsPipeline.SmplDesc.Count               = 1;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        auto IsVulkan = pEnv->GetDevice()->GetDeviceCaps().IsVulkanDevice();

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = IsVulkan ?
            SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM :
            SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Input attachment test VS";
            ShaderCI.Source          = IsVulkan ?
                GLSL::DrawTest_ProceduralTriangleVS.c_str() :
                HLSL::DrawTest_ProceduralTriangleVS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Input attachment test PS";
            ShaderCI.Source          = IsVulkan ?
                GLSL::InputAttachmentTest_FS.c_str() :
                HLSL::InputAttachmentTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pPS, nullptr);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pInputAttachmentPSO);
        ASSERT_NE(pInputAttachmentPSO, nullptr);
        pInputAttachmentPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_SubpassInput")->Set(pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pInputAttachmentPSO->CreateShaderResourceBinding(&pInputAttachmentSRB, true);
        ASSERT_NE(pInputAttachmentSRB, nullptr);
    }

    ITextureView* pRTAttachments[] = //
        {
            pTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
            pSwapChain->GetCurrentBackBufferRTV() //
        };

    FramebufferDesc FBDesc;
    FBDesc.Name            = "Render pass input attachment test framebuffer";
    FBDesc.pRenderPass     = pRenderPass;
    FBDesc.AttachmentCount = _countof(Attachments);
    FBDesc.ppAttachments   = pRTAttachments;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    BeginRenderPassAttribs RPBeginInfo;
    RPBeginInfo.pRenderPass  = pRenderPass;
    RPBeginInfo.pFramebuffer = pFramebuffer;

    OptimizedClearValue ClearValues[2];
    ClearValues[0].Color[0] = 0;
    ClearValues[0].Color[1] = 0;
    ClearValues[0].Color[2] = 0;
    ClearValues[0].Color[3] = 0;

    ClearValues[1].Color[0] = ClearColor[0];
    ClearValues[1].Color[1] = ClearColor[1];
    ClearValues[1].Color[2] = ClearColor[2];
    ClearValues[1].Color[3] = ClearColor[3];

    RPBeginInfo.pClearValues        = ClearValues;
    RPBeginInfo.ClearValueCount     = _countof(ClearValues);
    RPBeginInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    pContext->BeginRenderPass(RPBeginInfo);

    DrawAttribs DrawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(DrawAttrs);

    pContext->NextSubpass();

    pContext->SetPipelineState(pInputAttachmentPSO);
    pContext->CommitShaderResources(pInputAttachmentSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    pContext->Draw(DrawAttrs);

    pContext->EndRenderPass();

    Present();
}

} // namespace
