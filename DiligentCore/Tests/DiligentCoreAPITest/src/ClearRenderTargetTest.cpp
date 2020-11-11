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

extern "C"
{
    int TestSwapChainCInterface(void* pSwapChain);
};

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void ClearRenderTargetReferenceD3D11(ISwapChain* pSwapChain, const float ClearColor[]);
#endif

#if D3D12_SUPPORTED
void ClearRenderTargetReferenceD3D12(ISwapChain* pSwapChain, const float ClearColor[]);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void ClearRenderTargetReferenceGL(ISwapChain* pSwapChain, const float ClearColor[]);
#endif

#if VULKAN_SUPPORTED
void ClearRenderTargetReferenceVk(ISwapChain* pSwapChain, const float ClearColor[]);
#endif

#if METAL_SUPPORTED

#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

void ClearRenderTargetReference(IRenderDevice* pDevice,
                                ISwapChain*    pSwapChain,
                                const float    ClearColor[])
{
    auto deviceType = pDevice->GetDeviceCaps().DevType;
    switch (deviceType)
    {
#if D3D11_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D11:
            ClearRenderTargetReferenceD3D11(pSwapChain, ClearColor);
            break;
#endif

#if D3D12_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D12:
            ClearRenderTargetReferenceD3D12(pSwapChain, ClearColor);
            break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
            ClearRenderTargetReferenceGL(pSwapChain, ClearColor);
            break;

#endif

#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
            ClearRenderTargetReferenceVk(pSwapChain, ClearColor);
            break;

#endif

        default:
            LOG_ERROR_AND_THROW("Unsupported device type");
    }
}


TEST(ClearRenderTargetTest, AsRenderTarget)
{
    auto* pEnv       = TestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pConext    = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);

    constexpr float ClearColor[] = {0.25f, 0.5f, 0.75f, 1.0f};

    if (pTestingSwapChain)
    {
        pConext->Flush();
        pConext->InvalidateState();
        ClearRenderTargetReference(pDevice, pSwapChain, ClearColor);
        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    auto* pContext = pEnv->GetDeviceContext();

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pSwapChain->Present();

    TestSwapChainCInterface(pSwapChain);
}


TEST(ClearRenderTargetTest, AsAttachment)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_D3D12)
    {
        GTEST_SKIP() << "D3D12 does not allow render target clears within render pass";
    }

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pConext    = pEnv->GetDeviceContext();

    constexpr float ClearColor[] = {0.75f, 0.1875f, 0.375f, 1.0f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pConext->Flush();
        pConext->InvalidateState();
        ClearRenderTargetReference(pDevice, pSwapChain, ClearColor);
        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    RenderPassAttachmentDesc Attachments[1];
    auto*                    pRTV = pSwapChain->GetCurrentBackBufferRTV();
    ASSERT_NE(pRTV, nullptr);
    const auto& BackBufferDesc  = pRTV->GetTexture()->GetDesc();
    Attachments[0].Format       = BackBufferDesc.Format;
    Attachments[0].SampleCount  = static_cast<Uint8>(BackBufferDesc.SampleCount);
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_DISCARD;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[1] = {};

    Subpasses[0].RenderTargetAttachmentCount = 1;
    AttachmentReference RTAttachmentRef{0, RESOURCE_STATE_RENDER_TARGET};
    Subpasses[0].pRenderTargetAttachments = &RTAttachmentRef;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Clear attachment test render pass";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    FramebufferDesc FBDesc;
    FBDesc.Name               = "Clear attachment test framebuffer";
    FBDesc.pRenderPass        = pRenderPass;
    FBDesc.AttachmentCount    = _countof(Attachments);
    ITextureView* pTexViews[] = {pRTV};
    FBDesc.ppAttachments      = pTexViews;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    auto* pContext = pEnv->GetDeviceContext();

    BeginRenderPassAttribs BeginRPInfo;
    BeginRPInfo.pRenderPass         = pRenderPass;
    BeginRPInfo.pFramebuffer        = pFramebuffer;
    BeginRPInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->BeginRenderPass(BeginRPInfo);

    pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->EndRenderPass();

    pSwapChain->Present();
}



TEST(ClearRenderTargetTest, LoadOpClear)
{
    auto* pEnv       = TestingEnvironment::GetInstance();
    auto* pDevice    = pEnv->GetDevice();
    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pConext    = pEnv->GetDeviceContext();

    constexpr float ClearColor[] = {0.875f, 0.3125, 0.4375, 1.0f};

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pConext->Flush();
        pConext->InvalidateState();
        ClearRenderTargetReference(pDevice, pSwapChain, ClearColor);
        pTestingSwapChain->TakeSnapshot();
    }
    TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

    RenderPassAttachmentDesc Attachments[1];
    auto*                    pRTV = pSwapChain->GetCurrentBackBufferRTV();
    ASSERT_NE(pRTV, nullptr);
    const auto& BackBufferDesc  = pRTV->GetTexture()->GetDesc();
    Attachments[0].Format       = BackBufferDesc.Format;
    Attachments[0].SampleCount  = static_cast<Uint8>(BackBufferDesc.SampleCount);
    Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
    Attachments[0].LoadOp       = ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].StoreOp      = ATTACHMENT_STORE_OP_STORE;

    SubpassDesc Subpasses[1] = {};

    Subpasses[0].RenderTargetAttachmentCount = 1;
    AttachmentReference RTAttachmentRef{0, RESOURCE_STATE_RENDER_TARGET};
    Subpasses[0].pRenderTargetAttachments = &RTAttachmentRef;

    RenderPassDesc RPDesc;
    RPDesc.Name            = "Load op clear test render pass";
    RPDesc.AttachmentCount = _countof(Attachments);
    RPDesc.pAttachments    = Attachments;
    RPDesc.SubpassCount    = _countof(Subpasses);
    RPDesc.pSubpasses      = Subpasses;

    RefCntAutoPtr<IRenderPass> pRenderPass;
    pDevice->CreateRenderPass(RPDesc, &pRenderPass);
    ASSERT_NE(pRenderPass, nullptr);

    FramebufferDesc FBDesc;
    FBDesc.Name               = "Load op clear test framebuffer";
    FBDesc.pRenderPass        = pRenderPass;
    FBDesc.AttachmentCount    = _countof(Attachments);
    ITextureView* pTexViews[] = {pRTV};
    FBDesc.ppAttachments      = pTexViews;
    RefCntAutoPtr<IFramebuffer> pFramebuffer;
    pDevice->CreateFramebuffer(FBDesc, &pFramebuffer);
    ASSERT_TRUE(pFramebuffer);

    auto* pContext = pEnv->GetDeviceContext();

    BeginRenderPassAttribs BeginRPInfo;
    BeginRPInfo.pRenderPass         = pRenderPass;
    BeginRPInfo.pFramebuffer        = pFramebuffer;
    BeginRPInfo.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    OptimizedClearValue ClearValue;
    ClearValue.Color[0] = ClearColor[0];
    ClearValue.Color[1] = ClearColor[1];
    ClearValue.Color[2] = ClearColor[2];
    ClearValue.Color[3] = ClearColor[3];

    BeginRPInfo.ClearValueCount = 1;
    BeginRPInfo.pClearValues    = &ClearValue;

    pContext->BeginRenderPass(BeginRPInfo);

    pContext->EndRenderPass();

    pSwapChain->Present();
}

} // namespace
