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

#include "pch.h"
#include <iostream>
#include <fstream>
#include <string>
#include <array>

#include "SwapChainGL.h"
#include "DeviceContextGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "GLTypeConversions.hpp"

#include "BufferGLImpl.hpp"
#include "ShaderGLImpl.hpp"
#include "VAOCache.hpp"
#include "Texture1D_OGL.hpp"
#include "Texture1DArray_OGL.hpp"
#include "Texture2D_OGL.hpp"
#include "Texture2DArray_OGL.hpp"
#include "Texture3D_OGL.hpp"
#include "SamplerGLImpl.hpp"
#include "GraphicsAccessories.hpp"
#include "BufferViewGLImpl.hpp"
#include "PipelineStateGLImpl.hpp"
#include "FenceGLImpl.hpp"
#include "ShaderResourceBindingGLImpl.hpp"

using namespace std;

namespace Diligent
{

DeviceContextGLImpl::DeviceContextGLImpl(IReferenceCounters* pRefCounters, class RenderDeviceGLImpl* pDeviceGL, bool bIsDeferred) :
    // clang-format off
    TDeviceContextBase
    {
        pRefCounters,
        pDeviceGL,
        bIsDeferred
    },
    m_ContextState                       {pDeviceGL},
    m_CommitedResourcesTentativeBarriers {0        },
    m_DefaultFBO                         {false    }
// clang-format on
{
    m_BoundWritableTextures.reserve(16);
    m_BoundWritableBuffers.reserve(16);
}

IMPLEMENT_QUERY_INTERFACE(DeviceContextGLImpl, IID_DeviceContextGL, TDeviceContextBase)


void DeviceContextGLImpl::SetPipelineState(IPipelineState* pPipelineState)
{
    auto* pPipelineStateGLImpl = ValidatedCast<PipelineStateGLImpl>(pPipelineState);
    if (PipelineStateGLImpl::IsSameObject(m_pPipelineState, pPipelineStateGLImpl))
        return;

    TDeviceContextBase::SetPipelineState(pPipelineStateGLImpl, 0 /*Dummy*/);

    const auto& Desc = pPipelineStateGLImpl->GetDesc();
    if (Desc.PipelineType == PIPELINE_TYPE_COMPUTE)
    {
    }
    else if (Desc.PipelineType == PIPELINE_TYPE_GRAPHICS)
    {
        const auto& GraphicsPipeline = pPipelineStateGLImpl->GetGraphicsPipelineDesc();
        // Set rasterizer state
        {
            const auto& RasterizerDesc = GraphicsPipeline.RasterizerDesc;

            m_ContextState.SetFillMode(RasterizerDesc.FillMode);
            m_ContextState.SetCullMode(RasterizerDesc.CullMode);
            m_ContextState.SetFrontFace(RasterizerDesc.FrontCounterClockwise);
            m_ContextState.SetDepthBias(static_cast<Float32>(RasterizerDesc.DepthBias), RasterizerDesc.SlopeScaledDepthBias);
            if (RasterizerDesc.DepthBiasClamp != 0)
                LOG_WARNING_MESSAGE("Depth bias clamp is not supported on OpenGL");

            // Enabling depth clamping in GL is the same as disabling clipping in Direct3D.
            // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc
            // https://www.khronos.org/opengl/wiki/GLAPI/glEnable
            m_ContextState.SetDepthClamp(!RasterizerDesc.DepthClipEnable);

            m_ContextState.EnableScissorTest(RasterizerDesc.ScissorEnable);
            if (RasterizerDesc.AntialiasedLineEnable)
                LOG_WARNING_MESSAGE("Line antialiasing is not supported on OpenGL");
        }

        // Set blend state
        {
            const auto& BSDsc = GraphicsPipeline.BlendDesc;
            m_ContextState.SetBlendState(BSDsc, GraphicsPipeline.SampleMask);
        }

        // Set depth-stencil state
        {
            const auto& DepthStencilDesc = GraphicsPipeline.DepthStencilDesc;

            m_ContextState.EnableDepthTest(DepthStencilDesc.DepthEnable);
            m_ContextState.EnableDepthWrites(DepthStencilDesc.DepthWriteEnable);
            m_ContextState.SetDepthFunc(DepthStencilDesc.DepthFunc);

            m_ContextState.EnableStencilTest(DepthStencilDesc.StencilEnable);

            m_ContextState.SetStencilWriteMask(DepthStencilDesc.StencilWriteMask);

            {
                const auto& FrontFace = DepthStencilDesc.FrontFace;
                m_ContextState.SetStencilFunc(GL_FRONT, FrontFace.StencilFunc, m_StencilRef, DepthStencilDesc.StencilReadMask);
                m_ContextState.SetStencilOp(GL_FRONT, FrontFace.StencilFailOp, FrontFace.StencilDepthFailOp, FrontFace.StencilPassOp);
            }

            {
                const auto& BackFace = DepthStencilDesc.BackFace;
                m_ContextState.SetStencilFunc(GL_BACK, BackFace.StencilFunc, m_StencilRef, DepthStencilDesc.StencilReadMask);
                m_ContextState.SetStencilOp(GL_BACK, BackFace.StencilFailOp, BackFace.StencilDepthFailOp, BackFace.StencilPassOp);
            }
        }
        m_ContextState.InvalidateVAO();
    }
    else
    {
        LOG_ERROR_MESSAGE(GetPipelineTypeString(Desc.PipelineType), " pipeline '", Desc.Name, "' is not supported in OpenGL");
        return;
    }

    // Note that the program may change if a shader is created after the call
    // (GLProgramResources needs to bind a program to load uniforms), but before
    // the draw command.
    m_pPipelineState->CommitProgram(m_ContextState);
}

void DeviceContextGLImpl::TransitionShaderResources(IPipelineState* pPipelineState, IShaderResourceBinding* pShaderResourceBinding)
{
    if (m_pActiveRenderPass)
    {
        LOG_ERROR_MESSAGE("State transitions are not allowed inside a render pass.");
        return;
    }
}

void DeviceContextGLImpl::CommitShaderResources(IShaderResourceBinding* pShaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (!DeviceContextBase::CommitShaderResources(pShaderResourceBinding, StateTransitionMode, 0))
        return;

    if (m_CommitedResourcesTentativeBarriers != 0)
        LOG_INFO_MESSAGE("Not all tentative resource barriers have been executed since the last call to CommitShaderResources(). Did you forget to call Draw()/DispatchCompute() ?");

    m_CommitedResourcesTentativeBarriers = 0;
    BindProgramResources(m_CommitedResourcesTentativeBarriers, pShaderResourceBinding);
    // m_CommitedResourcesTentativeBarriers will contain memory barriers that will be required
    // AFTER the actual draw/dispatch command is executed. Before that they have no meaning
}

void DeviceContextGLImpl::SetStencilRef(Uint32 StencilRef)
{
    if (TDeviceContextBase::SetStencilRef(StencilRef, 0))
    {
        m_ContextState.SetStencilRef(GL_FRONT, StencilRef);
        m_ContextState.SetStencilRef(GL_BACK, StencilRef);
    }
}

void DeviceContextGLImpl::SetBlendFactors(const float* pBlendFactors)
{
    if (TDeviceContextBase::SetBlendFactors(pBlendFactors, 0))
    {
        m_ContextState.SetBlendFactors(m_BlendFactors);
    }
}

void DeviceContextGLImpl::SetVertexBuffers(Uint32                         StartSlot,
                                           Uint32                         NumBuffersSet,
                                           IBuffer**                      ppBuffers,
                                           Uint32*                        pOffsets,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                           SET_VERTEX_BUFFERS_FLAGS       Flags)
{
    TDeviceContextBase::SetVertexBuffers(StartSlot, NumBuffersSet, ppBuffers, pOffsets, StateTransitionMode, Flags);
    m_ContextState.InvalidateVAO();
}

void DeviceContextGLImpl::InvalidateState()
{
    TDeviceContextBase::InvalidateState();

    m_ContextState.Invalidate();
    m_BoundWritableTextures.clear();
    m_BoundWritableBuffers.clear();
    m_IsDefaultFBOBound = false;
}

void DeviceContextGLImpl::SetIndexBuffer(IBuffer* pIndexBuffer, Uint32 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::SetIndexBuffer(pIndexBuffer, ByteOffset, StateTransitionMode);
    m_ContextState.InvalidateVAO();
}

void DeviceContextGLImpl::SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32 RTWidth, Uint32 RTHeight)
{
    TDeviceContextBase::SetViewports(NumViewports, pViewports, RTWidth, RTHeight);

    VERIFY(NumViewports == m_NumViewports, "Unexpected number of viewports");
    if (NumViewports == 1)
    {
        const auto& vp = m_Viewports[0];
        // Note that OpenGL and DirectX use different origin of
        // the viewport in window coordinates:
        //
        // DirectX (0,0)
        //     \ ____________
        //      |            |
        //      |            |
        //      |            |
        //      |            |
        //      |____________|
        //     /
        //  OpenGL (0,0)
        //
        float BottomLeftY = static_cast<float>(RTHeight) - (vp.TopLeftY + vp.Height);
        float BottomLeftX = vp.TopLeftX;

        Int32 x = static_cast<int>(BottomLeftX);
        Int32 y = static_cast<int>(BottomLeftY);
        Int32 w = static_cast<int>(vp.Width);
        Int32 h = static_cast<int>(vp.Height);
        if (static_cast<float>(x) == BottomLeftX &&
            static_cast<float>(y) == BottomLeftY &&
            static_cast<float>(w) == vp.Width &&
            static_cast<float>(h) == vp.Height)
        {
            // GL_INVALID_VALUE is generated if either width or height is negative
            // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glViewport.xml
            glViewport(x, y, w, h);
        }
        else
        {
            // GL_INVALID_VALUE is generated if either width or height is negative
            // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glViewportIndexed.xhtml
            glViewportIndexedf(0, BottomLeftX, BottomLeftY, vp.Width, vp.Height);
        }
        DEV_CHECK_GL_ERROR("Failed to set viewport");

        glDepthRangef(vp.MinDepth, vp.MaxDepth);
        DEV_CHECK_GL_ERROR("Failed to set depth range");
    }
    else
    {
        for (Uint32 i = 0; i < NumViewports; ++i)
        {
            const auto& vp          = m_Viewports[i];
            float       BottomLeftY = static_cast<float>(RTHeight) - (vp.TopLeftY + vp.Height);
            float       BottomLeftX = vp.TopLeftX;
            glViewportIndexedf(i, BottomLeftX, BottomLeftY, vp.Width, vp.Height);
            DEV_CHECK_GL_ERROR("Failed to set viewport #", i);
            glDepthRangef(vp.MinDepth, vp.MaxDepth);
            DEV_CHECK_GL_ERROR("Failed to set depth range for viewport #", i);
        }
    }
}

void DeviceContextGLImpl::SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32 RTWidth, Uint32 RTHeight)
{
    TDeviceContextBase::SetScissorRects(NumRects, pRects, RTWidth, RTHeight);

    VERIFY(NumRects == m_NumScissorRects, "Unexpected number of scissor rects");
    if (NumRects == 1)
    {
        const auto& Rect = m_ScissorRects[0];
        // Note that OpenGL and DirectX use different origin
        // of the viewport in window coordinates:
        //
        // DirectX (0,0)
        //     \ ____________
        //      |            |
        //      |            |
        //      |            |
        //      |            |
        //      |____________|
        //     /
        //  OpenGL (0,0)
        //
        auto glBottom = RTHeight - Rect.bottom;

        auto width  = Rect.right - Rect.left;
        auto height = Rect.bottom - Rect.top;
        glScissor(Rect.left, glBottom, width, height);
        DEV_CHECK_GL_ERROR("Failed to set scissor rect");
    }
    else
    {
        for (Uint32 sr = 0; sr < NumRects; ++sr)
        {
            const auto& Rect     = m_ScissorRects[sr];
            auto        glBottom = RTHeight - Rect.bottom;
            auto        width    = Rect.right - Rect.left;
            auto        height   = Rect.bottom - Rect.top;
            glScissorIndexed(sr, Rect.left, glBottom, width, height);
            DEV_CHECK_GL_ERROR("Failed to set scissor rect #", sr);
        }
    }
}

void DeviceContextGLImpl::SetSwapChain(ISwapChainGL* pSwapChain)
{
    m_pSwapChain = pSwapChain;
}

void DeviceContextGLImpl::CommitRenderTargets()
{
    VERIFY(m_pActiveRenderPass == nullptr, "This method must not be called inside render pass");

    if (!m_IsDefaultFBOBound && m_NumBoundRenderTargets == 0 && !m_pBoundDepthStencil)
        return;

    if (m_IsDefaultFBOBound)
    {
        GLuint DefaultFBOHandle = m_pSwapChain->GetDefaultFBO();
        if (m_DefaultFBO != DefaultFBOHandle)
        {
            m_DefaultFBO = GLObjectWrappers::GLFrameBufferObj{true, GLObjectWrappers::GLFBOCreateReleaseHelper(DefaultFBOHandle)};
        }
        m_ContextState.BindFBO(m_DefaultFBO);
    }
    else
    {
        VERIFY(m_NumBoundRenderTargets != 0 || m_pBoundDepthStencil, "At least one render target or a depth stencil is expected");

        Uint32 NumRenderTargets = m_NumBoundRenderTargets;
        VERIFY(NumRenderTargets < MAX_RENDER_TARGETS, "Too many render targets (", NumRenderTargets, ") are being set");
        NumRenderTargets = std::min(NumRenderTargets, MAX_RENDER_TARGETS);

        const auto& CtxCaps = m_ContextState.GetContextCaps();
        VERIFY(NumRenderTargets < static_cast<Uint32>(CtxCaps.m_iMaxDrawBuffers), "This device only supports ", CtxCaps.m_iMaxDrawBuffers, " draw buffers, but ", NumRenderTargets, " are being set");
        NumRenderTargets = std::min(NumRenderTargets, static_cast<Uint32>(CtxCaps.m_iMaxDrawBuffers));

        TextureViewGLImpl* pBoundRTVs[MAX_RENDER_TARGETS] = {};
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        {
            pBoundRTVs[rt] = m_pBoundRenderTargets[rt];
            DEV_CHECK_ERR(!pBoundRTVs[rt] || pBoundRTVs[rt]->GetTexture<TextureBaseGL>()->GetGLHandle(),
                          "Color buffer of the default framebuffer can only be bound with the default framebuffer's depth buffer "
                          "and cannot be combined with any other render target or depth buffer in OpenGL backend.");
        }

        DEV_CHECK_ERR(!m_pBoundDepthStencil || m_pBoundDepthStencil->GetTexture<TextureBaseGL>()->GetGLHandle(),
                      "Depth buffer of the default framebuffer can only be bound with the default framebuffer's color buffer "
                      "and cannot be combined with any other render target in OpenGL backend.");

        auto        CurrentNativeGLContext = m_ContextState.GetCurrentGLContext();
        auto&       FBOCache               = m_pDevice->GetFBOCache(CurrentNativeGLContext);
        const auto& FBO                    = FBOCache.GetFBO(NumRenderTargets, pBoundRTVs, m_pBoundDepthStencil, m_ContextState);
        // Even though the write mask only applies to writes to a framebuffer, the mask state is NOT
        // Framebuffer state. So it is NOT part of a Framebuffer Object or the Default Framebuffer.
        // Binding a new framebuffer will NOT affect the mask.
        m_ContextState.BindFBO(FBO);
    }
    // Set the viewport to match the render target size
    SetViewports(1, nullptr, 0, 0);
}

void DeviceContextGLImpl::SetRenderTargets(Uint32                         NumRenderTargets,
                                           ITextureView*                  ppRenderTargets[],
                                           ITextureView*                  pDepthStencil,
                                           RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
#ifdef DILIGENT_DEVELOPMENT
    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Calling SetRenderTargets inside active render pass is invalid. End the render pass first");
        return;
    }
#endif

    if (TDeviceContextBase::SetRenderTargets(NumRenderTargets, ppRenderTargets, pDepthStencil))
    {
        if (m_NumBoundRenderTargets == 1 && m_pBoundRenderTargets[0] && m_pBoundRenderTargets[0]->GetTexture<TextureBaseGL>()->GetGLHandle() == 0)
        {
            DEV_CHECK_ERR(!m_pBoundDepthStencil || m_pBoundDepthStencil->GetTexture<TextureBaseGL>()->GetGLHandle() == 0,
                          "Attempting to bind texture '", m_pBoundDepthStencil->GetTexture()->GetDesc().Name,
                          "' as depth buffer with the default framebuffer's color buffer: color buffer of the default framebuffer "
                          "can only be bound with the default framebuffer's depth buffer and cannot be combined with any other depth buffer in OpenGL backend.");
            m_IsDefaultFBOBound = true;
        }
        else if (m_NumBoundRenderTargets == 0 && m_pBoundDepthStencil && m_pBoundDepthStencil->GetTexture<TextureBaseGL>()->GetGLHandle() == 0)
        {
            m_IsDefaultFBOBound = true;
        }
        else
        {
            m_IsDefaultFBOBound = false;
        }

        CommitRenderTargets();
    }
}

void DeviceContextGLImpl::ResetRenderTargets()
{
    TDeviceContextBase::ResetRenderTargets();
    m_IsDefaultFBOBound = false;
    m_ContextState.InvalidateFBO();
}

void DeviceContextGLImpl::BeginSubpass()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& SubpassDesc = RPDesc.pSubpasses[m_SubpassIndex];
    const auto& FBDesc      = m_pBoundFramebuffer->GetDesc();

    const auto& RenderTargetFBO = m_pBoundFramebuffer->GetSubpassFramebuffer(m_SubpassIndex).RenderTarget;
    if (RenderTargetFBO != 0)
    {
        m_ContextState.BindFBO(RenderTargetFBO);
    }
    else
    {
        GLuint DefaultFBOHandle = m_pSwapChain->GetDefaultFBO();
        if (m_DefaultFBO != DefaultFBOHandle)
        {
            m_DefaultFBO = GLObjectWrappers::GLFrameBufferObj{true, GLObjectWrappers::GLFBOCreateReleaseHelper(DefaultFBOHandle)};
        }
        m_ContextState.BindFBO(m_DefaultFBO);
    }

    for (Uint32 rt = 0; rt < SubpassDesc.RenderTargetAttachmentCount; ++rt)
    {
        const auto& RTAttachmentRef = SubpassDesc.pRenderTargetAttachments[rt];
        if (RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            auto* const pRTV = ValidatedCast<TextureViewGLImpl>(FBDesc.ppAttachments[RTAttachmentRef.AttachmentIndex]);
            if (pRTV == nullptr)
                continue;

            auto* const pColorTexGL = pRTV->GetTexture<TextureBaseGL>();
            pColorTexGL->TextureMemoryBarrier(
                GL_FRAMEBUFFER_BARRIER_BIT, // Reads and writes via framebuffer object attachments after the
                                            // barrier will reflect data written by shaders prior to the barrier.
                                            // Additionally, framebuffer writes issued after the barrier will wait
                                            // on the completion of all shader writes issued prior to the barrier.
                m_ContextState);

            const auto& AttachmentDesc = RPDesc.pAttachments[RTAttachmentRef.AttachmentIndex];
            auto        FirstLastUse   = m_pActiveRenderPass->GetAttachmentFirstLastUse(RTAttachmentRef.AttachmentIndex);
            if (FirstLastUse.first == m_SubpassIndex && AttachmentDesc.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
            {
                ClearRenderTarget(pRTV, m_AttachmentClearValues[RTAttachmentRef.AttachmentIndex].Color, RESOURCE_STATE_TRANSITION_MODE_NONE);
            }
        }
    }

    if (SubpassDesc.pDepthStencilAttachment != nullptr)
    {
        const auto DepthAttachmentIndex = SubpassDesc.pDepthStencilAttachment->AttachmentIndex;
        if (DepthAttachmentIndex != ATTACHMENT_UNUSED)
        {
            auto* const pDSV = ValidatedCast<TextureViewGLImpl>(FBDesc.ppAttachments[DepthAttachmentIndex]);
            if (pDSV != nullptr)
            {
                auto* pDepthTexGL = pDSV->GetTexture<TextureBaseGL>();
                pDepthTexGL->TextureMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT, m_ContextState);

                const auto& AttachmentDesc = RPDesc.pAttachments[DepthAttachmentIndex];
                auto        FirstLastUse   = m_pActiveRenderPass->GetAttachmentFirstLastUse(DepthAttachmentIndex);
                if (FirstLastUse.first == m_SubpassIndex && AttachmentDesc.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
                {
                    const auto& ClearVal = m_AttachmentClearValues[DepthAttachmentIndex].DepthStencil;
                    ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG | CLEAR_STENCIL_FLAG, ClearVal.Depth, ClearVal.Stencil, RESOURCE_STATE_TRANSITION_MODE_NONE);
                }
            }
        }
    }
}

void DeviceContextGLImpl::EndSubpass()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& SubpassDesc = RPDesc.pSubpasses[m_SubpassIndex];

    const auto& SubpassFBOs = m_pBoundFramebuffer->GetSubpassFramebuffer(m_SubpassIndex);
#ifdef DILIGENT_DEBUG
    {
        GLint glCurrReadFB = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &glCurrReadFB);
        CHECK_GL_ERROR("Failed to get current read framebuffer");
        GLuint glExpectedReadFB = SubpassFBOs.RenderTarget != 0 ? static_cast<GLuint>(SubpassFBOs.RenderTarget) : m_pSwapChain->GetDefaultFBO();
        VERIFY(static_cast<GLuint>(glCurrReadFB) == glExpectedReadFB, "Unexpected read framebuffer");
    }
#endif

    if (SubpassDesc.pResolveAttachments != nullptr)
    {
        GLuint ResolveDstFBO = SubpassFBOs.Resolve;
        if (ResolveDstFBO == 0)
        {
            ResolveDstFBO = m_pSwapChain.RawPtr<ISwapChainGL>()->GetDefaultFBO();
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ResolveDstFBO);
        DEV_CHECK_GL_ERROR("Failed to bind resolve destination FBO as draw framebuffer");

        const auto& FBODesc = m_pBoundFramebuffer->GetDesc();
        glBlitFramebuffer(0, 0, static_cast<GLint>(FBODesc.Width), static_cast<GLint>(FBODesc.Height),
                          0, 0, static_cast<GLint>(FBODesc.Width), static_cast<GLint>(FBODesc.Height),
                          GL_COLOR_BUFFER_BIT,
                          GL_NEAREST // Filter is ignored
        );
        DEV_CHECK_GL_ERROR("glBlitFramebuffer() failed when resolving multi-sampled attachments");
    }

    if (glInvalidateFramebuffer != nullptr)
    {
        // It is crucially important to invalidate the framebuffer while it is bound
        // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/mali-performance-2-how-to-correctly-handle-framebuffers
        GLsizei InvalidateAttachmentsCount = 0;

        std::array<GLenum, MAX_RENDER_TARGETS + 1> InvalidateAttachments;
        for (Uint32 rt = 0; rt < SubpassDesc.RenderTargetAttachmentCount; ++rt)
        {
            const auto RTAttachmentIdx = SubpassDesc.pRenderTargetAttachments[rt].AttachmentIndex;
            if (RTAttachmentIdx != ATTACHMENT_UNUSED)
            {
                auto AttachmentLastUse = m_pActiveRenderPass->GetAttachmentFirstLastUse(RTAttachmentIdx).second;
                if (AttachmentLastUse == m_SubpassIndex && RPDesc.pAttachments[RTAttachmentIdx].StoreOp == ATTACHMENT_STORE_OP_DISCARD)
                {
                    if (SubpassFBOs.RenderTarget == 0)
                    {
                        VERIFY(rt == 0, "Default framebuffer can only have single color attachment");
                        InvalidateAttachments[InvalidateAttachmentsCount++] = GL_COLOR;
                    }
                    else
                    {
                        InvalidateAttachments[InvalidateAttachmentsCount++] = GL_COLOR_ATTACHMENT0 + rt;
                    }
                }
            }
        }

        if (SubpassDesc.pDepthStencilAttachment != nullptr)
        {
            const auto DSAttachmentIdx = SubpassDesc.pDepthStencilAttachment->AttachmentIndex;
            if (DSAttachmentIdx != ATTACHMENT_UNUSED)
            {
                auto AttachmentLastUse = m_pActiveRenderPass->GetAttachmentFirstLastUse(DSAttachmentIdx).second;
                if (AttachmentLastUse == m_SubpassIndex && RPDesc.pAttachments[DSAttachmentIdx].StoreOp == ATTACHMENT_STORE_OP_DISCARD)
                {
                    const auto& FmtAttribs = GetTextureFormatAttribs(RPDesc.pAttachments[DSAttachmentIdx].Format);
                    VERIFY_EXPR(FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH || FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL);
                    if (SubpassFBOs.RenderTarget == 0)
                    {
                        InvalidateAttachments[InvalidateAttachmentsCount++] = GL_DEPTH;
                        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
                            InvalidateAttachments[InvalidateAttachmentsCount++] = GL_STENCIL;
                    }
                    else
                    {
                        InvalidateAttachments[InvalidateAttachmentsCount++] = FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;
                    }
                }
            }
        }

        if (InvalidateAttachmentsCount > 0)
        {
            glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, InvalidateAttachmentsCount, InvalidateAttachments.data());
            DEV_CHECK_GL_ERROR("glInvalidateFramebuffer() failed");
        }
    }

    // TODO: invalidate input attachments using glInvalidateTexImage

    m_ContextState.InvalidateFBO();
}

void DeviceContextGLImpl::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    TDeviceContextBase::BeginRenderPass(Attribs);

    m_AttachmentClearValues.resize(Attribs.ClearValueCount);
    for (Uint32 i = 0; i < Attribs.ClearValueCount; ++i)
        m_AttachmentClearValues[i] = Attribs.pClearValues[i];

    VERIFY_EXPR(m_pBoundFramebuffer);

    SetViewports(1, nullptr, 0, 0);

    BeginSubpass();
}

void DeviceContextGLImpl::NextSubpass()
{
    EndSubpass();
    TDeviceContextBase::NextSubpass();
    BeginSubpass();
    m_AttachmentClearValues.clear();
}

void DeviceContextGLImpl::EndRenderPass()
{
    EndSubpass();
    TDeviceContextBase::EndRenderPass();
    m_ContextState.InvalidateFBO();
}

void DeviceContextGLImpl::BindProgramResources(Uint32& NewMemoryBarriers, IShaderResourceBinding* pResBinding)
{
    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("No pipeline state is bound");
        return;
    }

    if (pResBinding == nullptr)
        return;

    auto*       pShaderResBindingGL = ValidatedCast<ShaderResourceBindingGLImpl>(pResBinding);
    const auto& ResourceCache       = pShaderResBindingGL->GetResourceCache(m_pPipelineState);
#ifdef DILIGENT_DEVELOPMENT
    m_pPipelineState->GetResourceLayout().dvpVerifyBindings(ResourceCache);
#endif

    VERIFY_EXPR(m_BoundWritableTextures.empty());
    VERIFY_EXPR(m_BoundWritableBuffers.empty());

    for (Uint32 ub = 0; ub < ResourceCache.GetUBCount(); ++ub)
    {
        const auto& UB = ResourceCache.GetConstUB(ub);
        if (!UB.pBuffer)
            continue;

        auto* pBufferGL = UB.pBuffer.RawPtr<BufferGLImpl>();
        pBufferGL->BufferMemoryBarrier(
            GL_UNIFORM_BARRIER_BIT, // Shader uniforms sourced from buffer objects after the barrier
                                    // will reflect data written by shaders prior to the barrier
            m_ContextState);

        m_ContextState.BindUniformBuffer(ub, pBufferGL->m_GlBuffer);
        //glBindBufferRange(GL_UNIFORM_BUFFER, it->Index, pBufferGL->m_GlBuffer, 0, pBufferGL->GetDesc().uiSizeInBytes);
    }

    for (Uint32 s = 0; s < ResourceCache.GetSamplerCount(); ++s)
    {
        const auto& Sam = ResourceCache.GetConstSampler(s);
        if (!Sam.pView)
            continue;

        // We must check 'pTexture' first as 'pBuffer' is in union with 'pSampler'
        if (Sam.pTexture != nullptr)
        {
            auto* pTexViewGL = Sam.pView.RawPtr<TextureViewGLImpl>();
            auto* pTextureGL = ValidatedCast<TextureBaseGL>(Sam.pTexture);
            VERIFY_EXPR(pTextureGL == pTexViewGL->GetTexture());
            m_ContextState.BindTexture(s, pTexViewGL->GetBindTarget(), pTexViewGL->GetHandle());

            pTextureGL->TextureMemoryBarrier(
                GL_TEXTURE_FETCH_BARRIER_BIT, // Texture fetches from shaders, including fetches from buffer object
                                              // memory via buffer textures, after the barrier will reflect data
                                              // written by shaders prior to the barrier
                m_ContextState);

            if (Sam.pSampler)
            {
                m_ContextState.BindSampler(s, Sam.pSampler->GetHandle());
            }
            else
            {
                m_ContextState.BindSampler(s, GLObjectWrappers::GLSamplerObj(false));
            }
        }
        else if (Sam.pBuffer != nullptr)
        {
            auto* pBufViewGL = Sam.pView.RawPtr<BufferViewGLImpl>();
            auto* pBufferGL  = ValidatedCast<BufferGLImpl>(Sam.pBuffer);
            VERIFY_EXPR(pBufferGL == pBufViewGL->GetBuffer());

            m_ContextState.BindTexture(s, GL_TEXTURE_BUFFER, pBufViewGL->GetTexBufferHandle());
            m_ContextState.BindSampler(s, GLObjectWrappers::GLSamplerObj(false)); // Use default texture sampling parameters

            pBufferGL->BufferMemoryBarrier(
                GL_TEXTURE_FETCH_BARRIER_BIT, // Texture fetches from shaders, including fetches from buffer object
                                              // memory via buffer textures, after the barrier will reflect data
                                              // written by shaders prior to the barrier
                m_ContextState);
        }
    }

#if GL_ARB_shader_image_load_store
    for (Uint32 img = 0; img < ResourceCache.GetImageCount(); ++img)
    {
        const auto& Img = ResourceCache.GetConstImage(img);
        if (!Img.pView)
            continue;

        // We must check 'pTexture' first as 'pBuffer' is in union with 'pSampler'
        if (Img.pTexture != nullptr)
        {
            auto* pTexViewGL = Img.pView.RawPtr<TextureViewGLImpl>();
            auto* pTextureGL = ValidatedCast<TextureBaseGL>(Img.pTexture);
            VERIFY_EXPR(pTextureGL == pTexViewGL->GetTexture());

            const auto& ViewDesc = pTexViewGL->GetDesc();
            VERIFY(ViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS, "Unexpected buffer view type");

            if (ViewDesc.AccessFlags & UAV_ACCESS_FLAG_WRITE)
            {
                pTextureGL->TextureMemoryBarrier(
                    GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, // Memory accesses using shader image load, store, and atomic built-in
                                                        // functions issued after the barrier will reflect data written by shaders
                                                        // prior to the barrier. Additionally, image stores and atomics issued after
                                                        // the barrier will not execute until all memory accesses (e.g., loads,
                                                        // stores, texture fetches, vertex fetches) initiated prior to the barrier
                                                        // complete.
                    m_ContextState);
                // We cannot set pending memory barriers here, because
                // if some texture is bound twice, the logic will fail
                m_BoundWritableTextures.push_back(pTextureGL);
            }

#    ifdef DILIGENT_DEBUG
            // Check that the texure being bound has immutable storage
            {
                m_ContextState.BindTexture(-1, pTexViewGL->GetBindTarget(), pTexViewGL->GetHandle());
                GLint IsImmutable = 0;
                glGetTexParameteriv(pTexViewGL->GetBindTarget(), GL_TEXTURE_IMMUTABLE_FORMAT, &IsImmutable);
                DEV_CHECK_GL_ERROR("glGetTexParameteriv() failed");
                VERIFY(IsImmutable, "Only immutable textures can be bound to pipeline using glBindImageTexture()");
                m_ContextState.BindTexture(-1, pTexViewGL->GetBindTarget(), GLObjectWrappers::GLTextureObj::Null());
            }
#    endif
            auto GlTexFormat = TexFormatToGLInternalTexFormat(ViewDesc.Format);
            // Note that if a format qulifier is specified in the shader, the format
            // must match it

            GLboolean Layered = ViewDesc.NumArraySlices > 1 && ViewDesc.FirstArraySlice == 0;
            // If "layered" is TRUE, the entire Mip level is bound. Layer parameter is ignored in this
            // case. If "layered" is FALSE, only the single layer identified by "layer" will
            // be bound. When "layered" is FALSE, the single bound layer is treated as a 2D texture.
            GLint Layer = ViewDesc.FirstArraySlice;

            auto GLAccess = AccessFlags2GLAccess(ViewDesc.AccessFlags);
            // WARNING: Texture being bound to the image unit must be complete
            // That means that if an integer texture is being bound, its
            // GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER must be NEAREST,
            // otherwise it will be incomplete
            m_ContextState.BindImage(img, pTexViewGL, ViewDesc.MostDetailedMip, Layered, Layer, GLAccess, GlTexFormat);
            // Do not use binding points from reflection as they may not be initialized
        }
        else if (Img.pBuffer != nullptr)
        {
            auto* pBuffViewGL = Img.pView.RawPtr<BufferViewGLImpl>();
            auto* pBufferGL   = ValidatedCast<BufferGLImpl>(Img.pBuffer);
            VERIFY_EXPR(pBufferGL == pBuffViewGL->GetBuffer());

            const auto& ViewDesc = pBuffViewGL->GetDesc();
            VERIFY(ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS, "Unexpected buffer view type");

            pBufferGL->BufferMemoryBarrier(
                GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, // Memory accesses using shader image load, store, and atomic built-in
                                                    // functions issued after the barrier will reflect data written by shaders
                                                    // prior to the barrier. Additionally, image stores and atomics issued after
                                                    // the barrier will not execute until all memory accesses (e.g., loads,
                                                    // stores, texture fetches, vertex fetches) initiated prior to the barrier
                                                    // complete.
                m_ContextState);

            m_BoundWritableBuffers.push_back(pBufferGL);

            auto GlFormat = TypeToGLTexFormat(ViewDesc.Format.ValueType, ViewDesc.Format.NumComponents, ViewDesc.Format.IsNormalized);
            m_ContextState.BindImage(img, pBuffViewGL, GL_READ_WRITE, GlFormat);
        }
    }
#endif


#if GL_ARB_shader_storage_buffer_object
    for (Uint32 ssbo = 0; ssbo < ResourceCache.GetSSBOCount(); ++ssbo)
    {
        const auto& SSBO = ResourceCache.GetConstSSBO(ssbo);
        if (!SSBO.pBufferView)
            return;

        auto*       pBufferViewGL = SSBO.pBufferView.RawPtr<BufferViewGLImpl>();
        const auto& ViewDesc      = pBufferViewGL->GetDesc();
        VERIFY(ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS || ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE, "Unexpected buffer view type");

        auto* pBufferGL = pBufferViewGL->GetBuffer<BufferGLImpl>();
        pBufferGL->BufferMemoryBarrier(
            GL_SHADER_STORAGE_BARRIER_BIT, // Accesses to shader storage blocks after the barrier
                                           // will reflect writes prior to the barrier
            m_ContextState);

        m_ContextState.BindStorageBlock(ssbo, pBufferGL->m_GlBuffer, ViewDesc.ByteOffset, ViewDesc.ByteWidth);

        if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS)
            m_BoundWritableBuffers.push_back(pBufferGL);
    }
#endif


#if GL_ARB_shader_image_load_store
    // Go through the list of textures bound as AUVs and set the required memory barriers
    for (auto* pWritableTex : m_BoundWritableTextures)
    {
        Uint32 TextureMemBarriers =
            GL_TEXTURE_UPDATE_BARRIER_BIT // Writes to a texture via glTex(Sub)Image*, glCopyTex(Sub)Image*,
                                          // glClearTex*Image, glCompressedTex(Sub)Image*, and reads via
                                          // glGetTexImage() after the barrier will reflect data written by
                                          // shaders prior to the barrier

            | GL_TEXTURE_FETCH_BARRIER_BIT // Texture fetches from shaders, including fetches from buffer object
                                           // memory via buffer textures, after the barrier will reflect data
                                           // written by shaders prior to the barrier

            | GL_PIXEL_BUFFER_BARRIER_BIT // Reads and writes of buffer objects via the GL_PIXEL_PACK_BUFFER and
                                          // GL_PIXEL_UNPACK_BUFFER bidnings after the barrier will reflect data
                                          // written by shaders prior to the barrier

            | GL_FRAMEBUFFER_BARRIER_BIT // Reads and writes via framebuffer object attachments after the
                                         // barrier will reflect data written by shaders prior to the barrier.
                                         // Additionally, framebuffer writes issued after the barrier will wait
                                         // on the completion of all shader writes issued prior to the barrier.

            | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;

        NewMemoryBarriers |= TextureMemBarriers;

        // Set new required barriers for the time when texture is used next time
        pWritableTex->SetPendingMemoryBarriers(TextureMemBarriers);
    }
    m_BoundWritableTextures.clear();

    for (auto* pWritableBuff : m_BoundWritableBuffers)
    {
        // clang-format off
        Uint32 BufferMemoryBarriers =
            GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT  |
            GL_ELEMENT_ARRAY_BARRIER_BIT        |
            GL_UNIFORM_BARRIER_BIT              |
            GL_COMMAND_BARRIER_BIT              | 
            GL_BUFFER_UPDATE_BARRIER_BIT        |
            GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT |
            GL_SHADER_STORAGE_BARRIER_BIT       |
            GL_TEXTURE_FETCH_BARRIER_BIT        |
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        // clang-format on

        NewMemoryBarriers |= BufferMemoryBarriers;
        // Set new required barriers for the time when buffer is used next time
        pWritableBuff->SetPendingMemoryBarriers(BufferMemoryBarriers);
    }
    m_BoundWritableBuffers.clear();
#endif
}

void DeviceContextGLImpl::PrepareForDraw(DRAW_FLAGS Flags, bool IsIndexed, GLenum& GlTopology)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_RENDER_TARGETS) != 0)
        DvpVerifyRenderTargets();
#endif

    // The program might have changed since the last SetPipelineState call if a shader was
    // created after the call (GLProgramResources needs to bind a program to load uniforms).
    m_pPipelineState->CommitProgram(m_ContextState);

    auto        CurrNativeGLContext = m_pDevice->m_GLContext.GetCurrentNativeGLContext();
    const auto& PipelineDesc        = m_pPipelineState->GetGraphicsPipelineDesc();
    if (!m_ContextState.IsValidVAOBound())
    {
        auto&    VAOCache     = m_pDevice->GetVAOCache(CurrNativeGLContext);
        IBuffer* pIndexBuffer = IsIndexed ? m_pIndexBuffer.RawPtr() : nullptr;
        if (PipelineDesc.InputLayout.NumElements > 0 || pIndexBuffer != nullptr)
        {
            const auto& VAO = VAOCache.GetVAO(m_pPipelineState, pIndexBuffer, m_VertexStreams, m_NumVertexStreams, m_ContextState);
            m_ContextState.BindVAO(VAO);
        }
        else
        {
            // Draw command will fail if no VAO is bound. If no vertex description is set
            // (which is the case if, for instance, the command only inputs VertexID),
            // use empty VAO
            const auto& VAO = VAOCache.GetEmptyVAO();
            m_ContextState.BindVAO(VAO);
        }
    }

    auto Topology = PipelineDesc.PrimitiveTopology;
    if (Topology >= PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)
    {
#if GL_ARB_tessellation_shader
        GlTopology       = GL_PATCHES;
        auto NumVertices = static_cast<Int32>(Topology - PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + 1);
        m_ContextState.SetNumPatchVertices(NumVertices);
#else
        UNSUPPORTED("Tessellation is not supported");
#endif
    }
    else
    {
        GlTopology = PrimitiveTopologyToGLTopology(Topology);
    }
}

void DeviceContextGLImpl::PrepareForIndexedDraw(VALUE_TYPE IndexType, Uint32 FirstIndexLocation, GLenum& GLIndexType, Uint32& FirstIndexByteOffset)
{
    GLIndexType = TypeToGLType(IndexType);
    VERIFY(GLIndexType == GL_UNSIGNED_BYTE || GLIndexType == GL_UNSIGNED_SHORT || GLIndexType == GL_UNSIGNED_INT,
           "Unsupported index type");
    VERIFY(m_pIndexBuffer, "Index Buffer is not bound to the pipeline");
    FirstIndexByteOffset = static_cast<Uint32>(GetValueSize(IndexType)) * FirstIndexLocation + m_IndexDataStartOffset;
}

void DeviceContextGLImpl::PostDraw()
{
    // IMPORTANT: new pending memory barriers in the context must be set
    // after all previous barriers have been executed.
    // m_CommitedResourcesTentativeBarriers contains memory barriers that will be required
    // AFTER the actual draw/dispatch command is executed.
    m_ContextState.SetPendingMemoryBarriers(m_CommitedResourcesTentativeBarriers);
    m_CommitedResourcesTentativeBarriers = 0;
}

void DeviceContextGLImpl::Draw(const DrawAttribs& Attribs)
{
    if (!DvpVerifyDrawArguments(Attribs))
        return;

    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, false, GlTopology);

    if (Attribs.NumInstances > 1 || Attribs.FirstInstanceLocation != 0)
    {
        if (Attribs.FirstInstanceLocation != 0)
            glDrawArraysInstancedBaseInstance(GlTopology, Attribs.StartVertexLocation, Attribs.NumVertices, Attribs.NumInstances, Attribs.FirstInstanceLocation);
        else
            glDrawArraysInstanced(GlTopology, Attribs.StartVertexLocation, Attribs.NumVertices, Attribs.NumInstances);
    }
    else
    {
        glDrawArrays(GlTopology, Attribs.StartVertexLocation, Attribs.NumVertices);
    }
    DEV_CHECK_GL_ERROR("OpenGL draw command failed");

    PostDraw();
}

void DeviceContextGLImpl::DrawIndexed(const DrawIndexedAttribs& Attribs)
{
    if (!DvpVerifyDrawIndexedArguments(Attribs))
        return;

    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, true, GlTopology);
    GLenum GLIndexType;
    Uint32 FirstIndexByteOffset;
    PrepareForIndexedDraw(Attribs.IndexType, Attribs.FirstIndexLocation, GLIndexType, FirstIndexByteOffset);

    // NOTE: Base Vertex and Base Instance versions are not supported even in OpenGL ES 3.1
    // This functionality can be emulated by adjusting stream offsets. This, however may cause
    // errors in case instance data is read from the same stream as vertex data. Thus handling
    // such cases is left to the application

    if (Attribs.NumInstances > 1 || Attribs.FirstInstanceLocation != 0)
    {
        if (Attribs.BaseVertex > 0)
        {
            if (Attribs.FirstInstanceLocation != 0)
                glDrawElementsInstancedBaseVertexBaseInstance(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances, Attribs.BaseVertex, Attribs.FirstInstanceLocation);
            else
                glDrawElementsInstancedBaseVertex(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances, Attribs.BaseVertex);
        }
        else
        {
            if (Attribs.FirstInstanceLocation != 0)
                glDrawElementsInstancedBaseInstance(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances, Attribs.FirstInstanceLocation);
            else
                glDrawElementsInstanced(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.NumInstances);
        }
    }
    else
    {
        if (Attribs.BaseVertex > 0)
            glDrawElementsBaseVertex(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)), Attribs.BaseVertex);
        else
            glDrawElements(GlTopology, Attribs.NumIndices, GLIndexType, reinterpret_cast<GLvoid*>(static_cast<size_t>(FirstIndexByteOffset)));
    }
    DEV_CHECK_GL_ERROR("OpenGL draw command failed");

    PostDraw();
}

void DeviceContextGLImpl::PrepareForIndirectDraw(IBuffer* pAttribsBuffer)
{
#if GL_ARB_draw_indirect
    auto* pIndirectDrawAttribsGL = ValidatedCast<BufferGLImpl>(pAttribsBuffer);
    // The indirect rendering functions take their data from the buffer currently bound to the
    // GL_DRAW_INDIRECT_BUFFER binding. Thus, any of indirect draw functions will fail if no buffer is
    // bound to that binding.
    pIndirectDrawAttribsGL->BufferMemoryBarrier(
        GL_COMMAND_BARRIER_BIT, // Command data sourced from buffer objects by
                                // Draw*Indirect and DispatchComputeIndirect commands after the barrier
                                // will reflect data written by shaders prior to the barrier.The buffer
                                // objects affected by this bit are derived from the DRAW_INDIRECT_BUFFER
                                // and DISPATCH_INDIRECT_BUFFER bindings.
        m_ContextState);
    constexpr bool ResetVAO = false; // GL_DRAW_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DRAW_INDIRECT_BUFFER, pIndirectDrawAttribsGL->m_GlBuffer, ResetVAO);
#endif
}

void DeviceContextGLImpl::DrawIndirect(const DrawIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDrawIndirectArguments(Attribs, pAttribsBuffer))
        return;

#if GL_ARB_draw_indirect
    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, true, GlTopology);

    // http://www.opengl.org/wiki/Vertex_Rendering
    PrepareForIndirectDraw(pAttribsBuffer);

    //typedef  struct {
    //   GLuint  count;
    //   GLuint  instanceCount;
    //   GLuint  first;
    //   GLuint  baseInstance;
    //} DrawArraysIndirectCommand;
    glDrawArraysIndirect(GlTopology, reinterpret_cast<const void*>(static_cast<size_t>(Attribs.IndirectDrawArgsOffset)));
    // Note that on GLES 3.1, baseInstance is present but reserved and must be zero
    DEV_CHECK_GL_ERROR("glDrawArraysIndirect() failed");

    constexpr bool ResetVAO = false; // GL_DRAW_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DRAW_INDIRECT_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    PostDraw();
#else
    LOG_ERROR_MESSAGE("Indirect rendering is not supported");
#endif
}

void DeviceContextGLImpl::DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDrawIndexedIndirectArguments(Attribs, pAttribsBuffer))
        return;

#if GL_ARB_draw_indirect
    GLenum GlTopology;
    PrepareForDraw(Attribs.Flags, true, GlTopology);
    GLenum GLIndexType;
    Uint32 FirstIndexByteOffset;
    PrepareForIndexedDraw(Attribs.IndexType, 0, GLIndexType, FirstIndexByteOffset);

    // http://www.opengl.org/wiki/Vertex_Rendering
    PrepareForIndirectDraw(pAttribsBuffer);

    //typedef  struct {
    //    GLuint  count;
    //    GLuint  instanceCount;
    //    GLuint  firstIndex;
    //    GLuint  baseVertex;
    //    GLuint  baseInstance;
    //} DrawElementsIndirectCommand;
    glDrawElementsIndirect(GlTopology, GLIndexType, reinterpret_cast<const void*>(static_cast<size_t>(Attribs.IndirectDrawArgsOffset)));
    // Note that on GLES 3.1, baseInstance is present but reserved and must be zero
    DEV_CHECK_GL_ERROR("glDrawElementsIndirect() failed");

    constexpr bool ResetVAO = false; // GL_DISPATCH_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DRAW_INDIRECT_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    PostDraw();
#else
    LOG_ERROR_MESSAGE("Indirect rendering is not supported");
#endif
}

void DeviceContextGLImpl::DrawMesh(const DrawMeshAttribs& Attribs)
{
    UNSUPPORTED("DrawMesh is not supported in OpenGL");
}

void DeviceContextGLImpl::DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    UNSUPPORTED("DrawMeshIndirect is not supported in OpenGL");
}


void DeviceContextGLImpl::DispatchCompute(const DispatchComputeAttribs& Attribs)
{
    if (!DvpVerifyDispatchArguments(Attribs))
        return;

#if GL_ARB_compute_shader
    // The program might have changed since the last SetPipelineState call if a shader was
    // created after the call (GLProgramResources needs to bind a program to load uniforms).
    m_pPipelineState->CommitProgram(m_ContextState);
    glDispatchCompute(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
    DEV_CHECK_GL_ERROR("glDispatchCompute() failed");

    PostDraw();
#else
    UNSUPPORTED("Compute shaders are not supported");
#endif
}

void DeviceContextGLImpl::DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDispatchIndirectArguments(Attribs, pAttribsBuffer))
        return;

#if GL_ARB_compute_shader
    // The program might have changed since the last SetPipelineState call if a shader was
    // created after the call (GLProgramResources needs to bind a program to load uniforms).
    m_pPipelineState->CommitProgram(m_ContextState);

    auto* pBufferGL = ValidatedCast<BufferGLImpl>(pAttribsBuffer);
    pBufferGL->BufferMemoryBarrier(
        GL_COMMAND_BARRIER_BIT, // Command data sourced from buffer objects by
                                // Draw*Indirect and DispatchComputeIndirect commands after the barrier
                                // will reflect data written by shaders prior to the barrier.The buffer
                                // objects affected by this bit are derived from the DRAW_INDIRECT_BUFFER
                                // and DISPATCH_INDIRECT_BUFFER bindings.
        m_ContextState);

    constexpr bool ResetVAO = false; // GL_DISPATCH_INDIRECT_BUFFER does not affect VAO
    m_ContextState.BindBuffer(GL_DISPATCH_INDIRECT_BUFFER, pBufferGL->m_GlBuffer, ResetVAO);
    DEV_CHECK_GL_ERROR("Failed to bind a buffer for dispatch indirect command");

    glDispatchComputeIndirect(Attribs.DispatchArgsByteOffset);
    DEV_CHECK_GL_ERROR("glDispatchComputeIndirect() failed");

    m_ContextState.BindBuffer(GL_DISPATCH_INDIRECT_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    PostDraw();
#else
    UNSUPPORTED("Compute shaders are not supported");
#endif
}

void DeviceContextGLImpl::ClearDepthStencil(ITextureView*                  pView,
                                            CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                            float                          fDepth,
                                            Uint8                          Stencil,
                                            RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (!TDeviceContextBase::ClearDepthStencil(pView))
        return;

    VERIFY_EXPR(pView != nullptr);

    if (pView != m_pBoundDepthStencil)
    {
        LOG_ERROR_MESSAGE("Depth stencil buffer must be bound to the context to be cleared in OpenGL backend");
        return;
    }

    Uint32 glClearFlags = 0;
    if (ClearFlags & CLEAR_DEPTH_FLAG) glClearFlags |= GL_DEPTH_BUFFER_BIT;
    if (ClearFlags & CLEAR_STENCIL_FLAG) glClearFlags |= GL_STENCIL_BUFFER_BIT;
    glClearDepthf(fDepth);
    glClearStencil(Stencil);
    // If depth writes are disabled, glClear() will not clear depth buffer!
    bool DepthWritesEnabled = m_ContextState.GetDepthWritesEnabled();
    m_ContextState.EnableDepthWrites(True);

    // Unlike OpenGL, in D3D10+, the full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied.

    bool ScissorTestEnabled = m_ContextState.GetScissorTestEnabled();
    m_ContextState.EnableScissorTest(False);
    // The pixel ownership test, the scissor test, dithering, and the buffer writemasks affect
    // the operation of glClear. The scissor box bounds the cleared region. Alpha function,
    // blend function, logical operation, stenciling, texture mapping, and depth-buffering
    // are ignored by glClear.
    glClear(glClearFlags);
    DEV_CHECK_GL_ERROR("glClear() failed");
    m_ContextState.EnableDepthWrites(DepthWritesEnabled);
    m_ContextState.EnableScissorTest(ScissorTestEnabled);
}

void DeviceContextGLImpl::ClearRenderTarget(ITextureView* pView, const float* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (!TDeviceContextBase::ClearRenderTarget(pView))
        return;

    VERIFY_EXPR(pView != nullptr);

    Int32 RTIndex = -1;
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (m_pBoundRenderTargets[rt] == pView)
        {
            RTIndex = rt;
            break;
        }
    }

    if (RTIndex == -1)
    {
        LOG_ERROR_MESSAGE("Render target must be bound to the context to be cleared in OpenGL backend");
        return;
    }

    static const float Zero[4] = {0, 0, 0, 0};
    if (RGBA == nullptr)
        RGBA = Zero;

    // The pixel ownership test, the scissor test, dithering, and the buffer writemasks affect
    // the operation of glClear. The scissor box bounds the cleared region. Alpha function,
    // blend function, logical operation, stenciling, texture mapping, and depth-buffering
    // are ignored by glClear.

    // Unlike OpenGL, in D3D10+, the full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied.

    // Disable scissor test
    bool ScissorTestEnabled = m_ContextState.GetScissorTestEnabled();
    m_ContextState.EnableScissorTest(False);

    // Set write mask
    Uint32 WriteMask         = 0;
    Bool   bIndependentBlend = False;
    m_ContextState.GetColorWriteMask(RTIndex, WriteMask, bIndependentBlend);
    m_ContextState.SetColorWriteMask(RTIndex, COLOR_MASK_ALL, bIndependentBlend);

    glClearBufferfv(GL_COLOR, RTIndex, RGBA);
    DEV_CHECK_GL_ERROR("glClearBufferfv() failed");

    m_ContextState.SetColorWriteMask(RTIndex, WriteMask, bIndependentBlend);
    m_ContextState.EnableScissorTest(ScissorTestEnabled);
}

void DeviceContextGLImpl::Flush()
{
    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Flushing device context inside an active render pass.");
    }

    glFlush();
}

void DeviceContextGLImpl::FinishFrame()
{
}

void DeviceContextGLImpl::FinishCommandList(class ICommandList** ppCommandList)
{
    LOG_ERROR("Deferred contexts are not supported in OpenGL mode");
}

void DeviceContextGLImpl::ExecuteCommandList(class ICommandList* pCommandList)
{
    LOG_ERROR("Deferred contexts are not supported in OpenGL mode");
}

void DeviceContextGLImpl::SignalFence(IFence* pFence, Uint64 Value)
{
    VERIFY(!m_bIsDeferred, "Fence can only be signaled from immediate context");
    GLObjectWrappers::GLSyncObj GLFence{glFenceSync(
        GL_SYNC_GPU_COMMANDS_COMPLETE, // Condition must always be GL_SYNC_GPU_COMMANDS_COMPLETE
        0                              // Flags, must be 0
        )};
    DEV_CHECK_GL_ERROR("Failed to create gl fence");
    auto* pFenceGLImpl = ValidatedCast<FenceGLImpl>(pFence);
    pFenceGLImpl->AddPendingFence(std::move(GLFence), Value);
}

void DeviceContextGLImpl::WaitForFence(IFence* pFence, Uint64 Value, bool FlushContext)
{
    VERIFY(!m_bIsDeferred, "Fence can only be waited from immediate context");
    auto* pFenceGLImpl = ValidatedCast<FenceGLImpl>(pFence);
    pFenceGLImpl->Wait(Value, FlushContext);
}

void DeviceContextGLImpl::WaitForIdle()
{
    VERIFY(!m_bIsDeferred, "Only immediate contexts can be idled");
    Flush();
    glFinish();
}

void DeviceContextGLImpl::BeginQuery(IQuery* pQuery)
{
    if (!TDeviceContextBase::BeginQuery(pQuery, 0))
        return;

    auto* pQueryGLImpl = ValidatedCast<QueryGLImpl>(pQuery);
    auto  QueryType    = pQueryGLImpl->GetDesc().Type;
    auto  glQuery      = pQueryGLImpl->GetGlQueryHandle();

    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
#if GL_SAMPLES_PASSED
            glBeginQuery(GL_SAMPLES_PASSED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_SAMPLES_PASSED query");
#else
            LOG_ERROR_MESSAGE_ONCE("GL_SAMPLES_PASSED query is not supported by this device");
#endif
            break;

        case QUERY_TYPE_BINARY_OCCLUSION:
            glBeginQuery(GL_ANY_SAMPLES_PASSED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_ANY_SAMPLES_PASSED query");
            break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
#if GL_PRIMITIVES_GENERATED
            glBeginQuery(GL_PRIMITIVES_GENERATED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_PRIMITIVES_GENERATED query");
#else
            LOG_ERROR_MESSAGE_ONCE("GL_PRIMITIVES_GENERATED query is not supported by this device");
#endif
            break;

        case QUERY_TYPE_DURATION:
#if GL_TIME_ELAPSED
            glBeginQuery(GL_TIME_ELAPSED, glQuery);
            DEV_CHECK_GL_ERROR("Failed to begin GL_TIME_ELAPSED query");
#else
            LOG_ERROR_MESSAGE_ONCE("Duration queries are not supported by this device");
#endif
            break;

        default:
            UNEXPECTED("Unexpected query type");
    }
}

void DeviceContextGLImpl::EndQuery(IQuery* pQuery)
{
    if (!TDeviceContextBase::EndQuery(pQuery, 0))
        return;

    auto* pQueryGLImpl = ValidatedCast<QueryGLImpl>(pQuery);
    auto  QueryType    = pQueryGLImpl->GetDesc().Type;
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
#if GL_SAMPLES_PASSED
            glEndQuery(GL_SAMPLES_PASSED);
            DEV_CHECK_GL_ERROR("Failed to end GL_SAMPLES_PASSED query");
#endif
            break;

        case QUERY_TYPE_BINARY_OCCLUSION:
            glEndQuery(GL_ANY_SAMPLES_PASSED);
            DEV_CHECK_GL_ERROR("Failed to end GL_ANY_SAMPLES_PASSED query");
            break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
#if GL_PRIMITIVES_GENERATED
            glEndQuery(GL_PRIMITIVES_GENERATED);
            DEV_CHECK_GL_ERROR("Failed to end GL_PRIMITIVES_GENERATED query");
#endif
            break;

        case QUERY_TYPE_TIMESTAMP:
#if GL_TIMESTAMP
            if (glQueryCounter != nullptr)
            {
                glQueryCounter(pQueryGLImpl->GetGlQueryHandle(), GL_TIMESTAMP);
                DEV_CHECK_GL_ERROR("glQueryCounter failed");
            }
            else
#endif
            {
                LOG_ERROR_MESSAGE_ONCE("Timer queries are not supported by this device");
            }
            break;

        case QUERY_TYPE_DURATION:
#if GL_TIME_ELAPSED
            glEndQuery(GL_TIME_ELAPSED);
            DEV_CHECK_GL_ERROR("Failed to end GL_TIME_ELAPSED query");
#else
            LOG_ERROR_MESSAGE_ONCE("Duration queries are not supported by this device");
#endif
            break;

        default:
            UNEXPECTED("Unexpected query type");
    }
}

bool DeviceContextGLImpl::UpdateCurrentGLContext()
{
    auto NativeGLContext = m_pDevice->m_GLContext.GetCurrentNativeGLContext();
    if (NativeGLContext == NULL)
        return false;

    m_ContextState.SetCurrentGLContext(NativeGLContext);
    return true;
}

void DeviceContextGLImpl::UpdateBuffer(IBuffer*                       pBuffer,
                                       Uint32                         Offset,
                                       Uint32                         Size,
                                       const void*                    pData,
                                       RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::UpdateBuffer(pBuffer, Offset, Size, pData, StateTransitionMode);

    auto* pBufferGL = ValidatedCast<BufferGLImpl>(pBuffer);
    pBufferGL->UpdateData(m_ContextState, Offset, Size, pData);
}

void DeviceContextGLImpl::CopyBuffer(IBuffer*                       pSrcBuffer,
                                     Uint32                         SrcOffset,
                                     RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                     IBuffer*                       pDstBuffer,
                                     Uint32                         DstOffset,
                                     Uint32                         Size,
                                     RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    TDeviceContextBase::CopyBuffer(pSrcBuffer, SrcOffset, SrcBufferTransitionMode, pDstBuffer, DstOffset, Size, DstBufferTransitionMode);

    auto* pSrcBufferGL = ValidatedCast<BufferGLImpl>(pSrcBuffer);
    auto* pDstBufferGL = ValidatedCast<BufferGLImpl>(pDstBuffer);
    pDstBufferGL->CopyData(m_ContextState, *pSrcBufferGL, SrcOffset, DstOffset, Size);
}

void DeviceContextGLImpl::MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData)
{
    TDeviceContextBase::MapBuffer(pBuffer, MapType, MapFlags, pMappedData);
    auto* pBufferGL = ValidatedCast<BufferGLImpl>(pBuffer);
    pBufferGL->Map(m_ContextState, MapType, MapFlags, pMappedData);
}

void DeviceContextGLImpl::UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    TDeviceContextBase::UnmapBuffer(pBuffer, MapType);
    auto* pBufferGL = ValidatedCast<BufferGLImpl>(pBuffer);
    pBufferGL->Unmap(m_ContextState);
}

void DeviceContextGLImpl::UpdateTexture(ITexture*                      pTexture,
                                        Uint32                         MipLevel,
                                        Uint32                         Slice,
                                        const Box&                     DstBox,
                                        const TextureSubResData&       SubresData,
                                        RESOURCE_STATE_TRANSITION_MODE SrcBufferStateTransitionMode,
                                        RESOURCE_STATE_TRANSITION_MODE TextureStateTransitionMode)
{
    TDeviceContextBase::UpdateTexture(pTexture, MipLevel, Slice, DstBox, SubresData, SrcBufferStateTransitionMode, TextureStateTransitionMode);
    auto* pTexGL = ValidatedCast<TextureBaseGL>(pTexture);
    pTexGL->UpdateData(m_ContextState, MipLevel, Slice, DstBox, SubresData);
}

void DeviceContextGLImpl::CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    TDeviceContextBase::CopyTexture(CopyAttribs);
    auto* pSrcTexGL = ValidatedCast<TextureBaseGL>(CopyAttribs.pSrcTexture);
    auto* pDstTexGL = ValidatedCast<TextureBaseGL>(CopyAttribs.pDstTexture);

    const auto& SrcTexDesc = pSrcTexGL->GetDesc();
    const auto& DstTexDesc = pDstTexGL->GetDesc();

    auto SrcMipLevelAttribs = GetMipLevelProperties(SrcTexDesc, CopyAttribs.SrcMipLevel);

    Box FullSrcBox;
    FullSrcBox.MaxX = SrcMipLevelAttribs.LogicalWidth;
    FullSrcBox.MaxY = SrcMipLevelAttribs.LogicalHeight;
    FullSrcBox.MaxZ = SrcMipLevelAttribs.Depth;
    auto* pSrcBox   = CopyAttribs.pSrcBox != nullptr ? CopyAttribs.pSrcBox : &FullSrcBox;

    if (SrcTexDesc.Usage == USAGE_STAGING && DstTexDesc.Usage != USAGE_STAGING)
    {
        TextureSubResData SubresData;
        SubresData.pData      = nullptr;
        SubresData.pSrcBuffer = pSrcTexGL->GetPBO();
        SubresData.SrcOffset =
            GetStagingTextureLocationOffset(SrcTexDesc, CopyAttribs.SrcSlice, CopyAttribs.SrcMipLevel,
                                            TextureBaseGL::PBOOffsetAlignment,
                                            pSrcBox->MinX, pSrcBox->MinY, pSrcBox->MinZ);
        SubresData.Stride      = SrcMipLevelAttribs.RowSize;
        SubresData.DepthStride = SrcMipLevelAttribs.DepthSliceSize;

        Box DstBox;
        DstBox.MinX = CopyAttribs.DstX;
        DstBox.MinY = CopyAttribs.DstY;
        DstBox.MinZ = CopyAttribs.DstZ;
        DstBox.MaxX = DstBox.MinX + pSrcBox->MaxX - pSrcBox->MinX;
        DstBox.MaxY = DstBox.MinY + pSrcBox->MaxY - pSrcBox->MinY;
        DstBox.MaxZ = DstBox.MinZ + pSrcBox->MaxZ - pSrcBox->MinZ;
        pDstTexGL->UpdateData(m_ContextState, CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, DstBox, SubresData);
    }
    else if (SrcTexDesc.Usage != USAGE_STAGING && DstTexDesc.Usage == USAGE_STAGING)
    {
        if (pSrcTexGL->GetGLTextureHandle() == 0)
        {
            auto*  pSwapChainGL     = m_pSwapChain.RawPtr<ISwapChainGL>();
            GLuint DefaultFBOHandle = pSwapChainGL->GetDefaultFBO();
            glBindFramebuffer(GL_READ_FRAMEBUFFER, DefaultFBOHandle);
            DEV_CHECK_GL_ERROR("Failed to bind default FBO as read framebuffer");
        }
        else
        {
            TextureViewDesc SrcTexViewDesc;
            SrcTexViewDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
            SrcTexViewDesc.MostDetailedMip = CopyAttribs.SrcMipLevel;
            SrcTexViewDesc.FirstArraySlice = CopyAttribs.SrcSlice;
            SrcTexViewDesc.NumArraySlices  = 1;
            SrcTexViewDesc.NumMipLevels    = 1;
            TextureViewGLImpl SrcTexView //
                {
                    nullptr, // pRefCounters
                    m_pDevice,
                    SrcTexViewDesc,
                    pSrcTexGL,
                    false, // bCreateGLViewTex
                    false  // bIsDefaultView
                };

            auto  CurrentNativeGLContext = m_ContextState.GetCurrentGLContext();
            auto& fboCache               = m_pDevice->GetFBOCache(CurrentNativeGLContext);

            TextureViewGLImpl* pSrcViews[] = {&SrcTexView};
            const auto&        SrcFBO      = fboCache.GetFBO(1, pSrcViews, nullptr, m_ContextState);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, SrcFBO);
            DEV_CHECK_GL_ERROR("Failed to bind FBO as read framebuffer");
        }

        auto* pDstBuffer = ValidatedCast<BufferGLImpl>(pDstTexGL->GetPBO());
        VERIFY(pDstBuffer != nullptr, "Internal staging buffer must not be null");
        // GetStagingTextureLocationOffset assumes pixels are tightly packed in every subresource - no padding
        // except between subresources.
        const auto DstOffset =
            GetStagingTextureLocationOffset(DstTexDesc, CopyAttribs.DstSlice, CopyAttribs.DstMipLevel,
                                            TextureBaseGL::PBOOffsetAlignment,
                                            CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ);

        m_ContextState.BindBuffer(GL_PIXEL_PACK_BUFFER, pDstBuffer->GetGLHandle(), true);

        const auto& TransferAttribs = GetNativePixelTransferAttribs(SrcTexDesc.Format);
        glReadPixels(pSrcBox->MinX, pSrcBox->MinY, pSrcBox->MaxX - pSrcBox->MinX, pSrcBox->MaxY - pSrcBox->MinY,
                     TransferAttribs.PixelFormat, TransferAttribs.DataType, reinterpret_cast<void*>(static_cast<size_t>(DstOffset)));
        DEV_CHECK_GL_ERROR("Failed to read pixel from framebuffer to pixel pack buffer");

        m_ContextState.BindBuffer(GL_PIXEL_PACK_BUFFER, GLObjectWrappers::GLBufferObj::Null(), true);
        // Restore original FBO
        m_ContextState.InvalidateFBO();
        CommitRenderTargets();
    }
    else
    {
        VERIFY(SrcTexDesc.Usage != USAGE_STAGING && DstTexDesc.Usage != USAGE_STAGING, "Copying between staging textures is not supported");
        pDstTexGL->CopyData(this, pSrcTexGL, CopyAttribs.SrcMipLevel, CopyAttribs.SrcSlice, CopyAttribs.pSrcBox,
                            CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ);
    }
}

void DeviceContextGLImpl::MapTextureSubresource(ITexture*                 pTexture,
                                                Uint32                    MipLevel,
                                                Uint32                    ArraySlice,
                                                MAP_TYPE                  MapType,
                                                MAP_FLAGS                 MapFlags,
                                                const Box*                pMapRegion,
                                                MappedTextureSubresource& MappedData)
{
    TDeviceContextBase::MapTextureSubresource(pTexture, MipLevel, ArraySlice, MapType, MapFlags, pMapRegion, MappedData);
    auto*       pTexGL  = ValidatedCast<TextureBaseGL>(pTexture);
    const auto& TexDesc = pTexGL->GetDesc();
    if (TexDesc.Usage == USAGE_STAGING)
    {
        auto PBOOffset       = GetStagingTextureSubresourceOffset(TexDesc, ArraySlice, MipLevel, TextureBaseGL::PBOOffsetAlignment);
        auto MipLevelAttribs = GetMipLevelProperties(TexDesc, MipLevel);
        auto pPBO            = ValidatedCast<BufferGLImpl>(pTexGL->GetPBO());
        pPBO->MapRange(m_ContextState, MapType, MapFlags, PBOOffset, MipLevelAttribs.MipSize, MappedData.pData);

        MappedData.Stride      = MipLevelAttribs.RowSize;
        MappedData.DepthStride = MipLevelAttribs.MipSize;
    }
    else
    {
        LOG_ERROR_MESSAGE("Only staging textures can be mapped in OpenGL");
        MappedData = MappedTextureSubresource{};
    }
}


void DeviceContextGLImpl::UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice)
{
    TDeviceContextBase::UnmapTextureSubresource(pTexture, MipLevel, ArraySlice);
    auto*       pTexGL  = ValidatedCast<TextureBaseGL>(pTexture);
    const auto& TexDesc = pTexGL->GetDesc();
    if (TexDesc.Usage == USAGE_STAGING)
    {
        auto pPBO = ValidatedCast<BufferGLImpl>(pTexGL->GetPBO());
        pPBO->Unmap(m_ContextState);
    }
    else
    {
        LOG_ERROR_MESSAGE("Only staging textures can be mapped in OpenGL");
    }
}

void DeviceContextGLImpl::GenerateMips(ITextureView* pTexView)
{
    TDeviceContextBase::GenerateMips(pTexView);
    auto* pTexViewGL = ValidatedCast<TextureViewGLImpl>(pTexView);
    auto  BindTarget = pTexViewGL->GetBindTarget();
    m_ContextState.BindTexture(-1, BindTarget, pTexViewGL->GetHandle());
    glGenerateMipmap(BindTarget);
    DEV_CHECK_GL_ERROR("Failed to generate mip maps");
    m_ContextState.BindTexture(-1, BindTarget, GLObjectWrappers::GLTextureObj::Null());
}

void DeviceContextGLImpl::TransitionResourceStates(Uint32 BarrierCount, StateTransitionDesc* pResourceBarriers)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");
}

void DeviceContextGLImpl::ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                    ITexture*                               pDstTexture,
                                                    const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
    TDeviceContextBase::ResolveTextureSubresource(pSrcTexture, pDstTexture, ResolveAttribs);
    auto*       pSrcTexGl  = ValidatedCast<TextureBaseGL>(pSrcTexture);
    auto*       pDstTexGl  = ValidatedCast<TextureBaseGL>(pDstTexture);
    const auto& SrcTexDesc = pSrcTexGl->GetDesc();
    //const auto& DstTexDesc = pDstTexGl->GetDesc();

    auto  CurrentNativeGLContext = m_ContextState.GetCurrentGLContext();
    auto& FBOCache               = m_pDevice->GetFBOCache(CurrentNativeGLContext);

    {
        TextureViewDesc SrcTexViewDesc;
        SrcTexViewDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
        SrcTexViewDesc.MostDetailedMip = ResolveAttribs.SrcMipLevel;
        SrcTexViewDesc.FirstArraySlice = ResolveAttribs.SrcSlice;
        TextureViewGLImpl SrcTexView //
            {
                nullptr, // pRefCounters
                m_pDevice,
                SrcTexViewDesc,
                pSrcTexGl,
                false, // bCreateGLViewTex
                false  // bIsDefaultView
            };

        TextureViewGLImpl* pSrcViews[] = {&SrcTexView};
        const auto&        SrcFBO      = FBOCache.GetFBO(1, pSrcViews, nullptr, m_ContextState);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, SrcFBO);
        DEV_CHECK_GL_ERROR("Failed to bind FBO as read framebuffer");
    }

    if (pDstTexGl->GetGLHandle())
    {
        TextureViewDesc DstTexViewDesc;
        DstTexViewDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
        DstTexViewDesc.MostDetailedMip = ResolveAttribs.DstMipLevel;
        DstTexViewDesc.FirstArraySlice = ResolveAttribs.DstSlice;
        TextureViewGLImpl DstTexView //
            {
                nullptr, // pRefCounters
                m_pDevice,
                DstTexViewDesc,
                pDstTexGl,
                false, // bCreateGLViewTex
                false  // bIsDefaultView
            };

        TextureViewGLImpl* pDstViews[] = {&DstTexView};
        const auto&        DstFBO      = FBOCache.GetFBO(1, pDstViews, nullptr, m_ContextState);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DstFBO);
        DEV_CHECK_GL_ERROR("Failed to bind FBO as draw framebuffer");
    }
    else
    {
        auto*  pSwapChainGL     = m_pSwapChain.RawPtr<ISwapChainGL>();
        GLuint DefaultFBOHandle = pSwapChainGL->GetDefaultFBO();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DefaultFBOHandle);
        DEV_CHECK_GL_ERROR("Failed to bind default FBO as draw framebuffer");
    }

    const auto& MipAttribs = GetMipLevelProperties(SrcTexDesc, ResolveAttribs.SrcMipLevel);
    glBlitFramebuffer(0, 0, static_cast<GLint>(MipAttribs.LogicalWidth), static_cast<GLint>(MipAttribs.LogicalHeight),
                      0, 0, static_cast<GLint>(MipAttribs.LogicalWidth), static_cast<GLint>(MipAttribs.LogicalHeight),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST // Filter is ignored
    );
    DEV_CHECK_GL_ERROR("glBlitFramebuffer() failed when resolving multi-sampled texture");

    // Restore original FBO
    m_ContextState.InvalidateFBO();
    CommitRenderTargets();
}

} // namespace Diligent
