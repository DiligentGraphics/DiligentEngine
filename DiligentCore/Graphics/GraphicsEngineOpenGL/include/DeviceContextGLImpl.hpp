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

#pragma once

#include <vector>

#include "DeviceContextGL.h"
#include "DeviceContextBase.hpp"
#include "BaseInterfacesGL.h"
#include "GLContextState.hpp"
#include "GLObjectWrapper.hpp"
#include "BufferGLImpl.hpp"
#include "TextureBaseGL.hpp"
#include "QueryGLImpl.hpp"
#include "FramebufferGLImpl.hpp"
#include "RenderPassGLImpl.hpp"
#include "PipelineStateGLImpl.hpp"

namespace Diligent
{

struct DeviceContextGLImplTraits
{
    using BufferType        = BufferGLImpl;
    using TextureType       = TextureBaseGL;
    using PipelineStateType = PipelineStateGLImpl;
    using DeviceType        = RenderDeviceGLImpl;
    using QueryType         = QueryGLImpl;
    using FramebufferType   = FramebufferGLImpl;
    using RenderPassType    = RenderPassGLImpl;
};

/// Device context implementation in OpenGL backend.
class DeviceContextGLImpl final : public DeviceContextBase<IDeviceContextGL, DeviceContextGLImplTraits>
{
public:
    using TDeviceContextBase = DeviceContextBase<IDeviceContextGL, DeviceContextGLImplTraits>;

    DeviceContextGLImpl(IReferenceCounters* pRefCounters, RenderDeviceGLImpl* pDeviceGL, bool bIsDeferred);

    /// Queries the specific interface, see IObject::QueryInterface() for details.
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IDeviceContext::SetPipelineState() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetPipelineState(IPipelineState* pPipelineState) override final;

    /// Implementation of IDeviceContext::TransitionShaderResources() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE TransitionShaderResources(IPipelineState* pPipelineState, IShaderResourceBinding* pShaderResourceBinding) override final;

    /// Implementation of IDeviceContext::CommitShaderResources() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetStencilRef() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetStencilRef(Uint32 StencilRef) override final;

    /// Implementation of IDeviceContext::SetBlendFactors() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetBlendFactors(const float* pBlendFactors = nullptr) override final;

    /// Implementation of IDeviceContext::SetVertexBuffers() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                     Uint32                         NumBuffersSet,
                                                     IBuffer**                      ppBuffers,
                                                     Uint32*                        pOffsets,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                     SET_VERTEX_BUFFERS_FLAGS       Flags) override final;

    /// Implementation of IDeviceContext::InvalidateState() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE InvalidateState() override final;

    /// Implementation of IDeviceContext::SetIndexBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                                   Uint32                         ByteOffset,
                                                   RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetViewports() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetViewports(Uint32          NumViewports,
                                                 const Viewport* pViewports,
                                                 Uint32          RTWidth,
                                                 Uint32          RTHeight) override final;

    /// Implementation of IDeviceContext::SetScissorRects() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetScissorRects(Uint32      NumRects,
                                                    const Rect* pRects,
                                                    Uint32      RTWidth,
                                                    Uint32      RTHeight) override final;

    /// Implementation of IDeviceContext::SetRenderTargets() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SetRenderTargets(Uint32                         NumRenderTargets,
                                                     ITextureView*                  ppRenderTargets[],
                                                     ITextureView*                  pDepthStencil,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::BeginRenderPass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BeginRenderPass(const BeginRenderPassAttribs& Attribs) override final;

    /// Implementation of IDeviceContext::NextSubpass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE NextSubpass() override final;

    /// Implementation of IDeviceContext::EndRenderPass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE EndRenderPass() override final;

    // clang-format off

    /// Implementation of IDeviceContext::Draw() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Draw               (const DrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexed() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexed        (const DrawIndexedAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawIndirect       (const DrawIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    /// Implementation of IDeviceContext::DrawIndexedIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    /// Implementation of IDeviceContext::DrawMesh() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawMesh           (const DrawMeshAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMeshIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DrawMeshIndirect   (const DrawMeshIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;

    /// Implementation of IDeviceContext::DispatchCompute() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DispatchCompute        (const DispatchComputeAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DispatchComputeIndirect() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;

    // clang-format on

    /// Implementation of IDeviceContext::ClearDepthStencil() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ClearDepthStencil(ITextureView*                  pView,
                                                      CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                                      float                          fDepth,
                                                      Uint8                          Stencil,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::ClearRenderTarget() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ClearRenderTarget(ITextureView*                  pView,
                                                      const float*                   RGBA,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::UpdateBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint32                         Offset,
                                                 Uint32                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint32                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint32                         DstOffset,
                                               Uint32                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override final;

    /// Implementation of IDeviceContext::MapBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData) override final;

    /// Implementation of IDeviceContext::UnmapBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override final;

    /// Implementation of IDeviceContext::UpdateTexture() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferStateTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE TextureStateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyTexture() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override final;

    /// Implementation of IDeviceContext::MapTextureSubresource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override final;

    /// Implementation of IDeviceContext::UnmapTextureSubresource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice) override final;

    /// Implementation of IDeviceContext::GenerateMips() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTexView) override;

    /// Implementation of IDeviceContext::FinishFrame() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE FinishFrame() override final;

    /// Implementation of IDeviceContext::TransitionResourceStates() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE TransitionResourceStates(Uint32 BarrierCount, StateTransitionDesc* pResourceBarriers) override final;

    /// Implementation of IDeviceContext::ResolveTextureSubresource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override final;

    /// Implementation of IDeviceContext::FinishCommandList() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE FinishCommandList(class ICommandList** ppCommandList) override final;

    /// Implementation of IDeviceContext::ExecuteCommandList() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ExecuteCommandList(class ICommandList* pCommandList) override final;

    /// Implementation of IDeviceContext::SignalFence() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE SignalFence(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::WaitForFence() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE WaitForFence(IFence* pFence, Uint64 Value, bool FlushContext) override final;

    /// Implementation of IDeviceContext::WaitForIdle() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of IDeviceContext::BeginQuery() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BeginQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::EndQuery() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE EndQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::Flush() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE Flush() override final;

    /// Implementation of IDeviceContextGL::UpdateCurrentGLContext().
    virtual bool DILIGENT_CALL_TYPE UpdateCurrentGLContext() override final;

    void BindProgramResources(Uint32& NewMemoryBarriers, IShaderResourceBinding* pResBinding);

    GLContextState& GetContextState() { return m_ContextState; }

    void CommitRenderTargets();

    virtual void DILIGENT_CALL_TYPE SetSwapChain(ISwapChainGL* pSwapChain) override final;

    virtual void ResetRenderTargets() override final;


protected:
    friend class BufferGLImpl;
    friend class TextureBaseGL;
    friend class ShaderGLImpl;

    GLContextState m_ContextState;

private:
    __forceinline void PrepareForDraw(DRAW_FLAGS Flags, bool IsIndexed, GLenum& GlTopology);
    __forceinline void PrepareForIndexedDraw(VALUE_TYPE IndexType, Uint32 FirstIndexLocation, GLenum& GLIndexType, Uint32& FirstIndexByteOffset);
    __forceinline void PrepareForIndirectDraw(IBuffer* pAttribsBuffer);
    __forceinline void PostDraw();

    void BeginSubpass();
    void EndSubpass();

    Uint32 m_CommitedResourcesTentativeBarriers = 0;

    std::vector<class TextureBaseGL*> m_BoundWritableTextures;
    std::vector<class BufferGLImpl*>  m_BoundWritableBuffers;

    RefCntAutoPtr<ISwapChainGL> m_pSwapChain;

    bool m_IsDefaultFBOBound = false;

    GLObjectWrappers::GLFrameBufferObj m_DefaultFBO;

    std::vector<OptimizedClearValue> m_AttachmentClearValues;
};

} // namespace Diligent
