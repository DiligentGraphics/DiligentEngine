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

/// \file
/// Implementation of the Diligent::DeviceContextBase template class and related structures

#include <unordered_map>

#include "DeviceContext.h"
#include "DeviceObjectBase.hpp"
#include "ResourceMapping.h"
#include "Sampler.h"
#include "ObjectBase.hpp"
#include "DebugUtilities.hpp"
#include "ValidatedCast.hpp"
#include "GraphicsAccessories.hpp"
#include "TextureBase.hpp"

namespace Diligent
{

/// Describes input vertex stream
template <typename BufferImplType>
struct VertexStreamInfo
{
    VertexStreamInfo() {}

    /// Strong reference to the buffer object
    RefCntAutoPtr<BufferImplType> pBuffer;
    Uint32                        Offset = 0; ///< Offset in bytes
};

/// Base implementation of the device context.

/// \tparam BaseInterface         - base interface that this class will inheret.
/// \tparam ImplementationTraits  - implementation traits that define specific implementation details
///                                 (texture implemenation type, buffer implementation type, etc.)
/// \remarks Device context keeps strong references to all objects currently bound to
///          the pipeline: buffers, states, samplers, shaders, etc.
///          The context also keeps strong references to the device and
///          the swap chain.
template <typename BaseInterface, typename ImplementationTraits>
class DeviceContextBase : public ObjectBase<BaseInterface>
{
public:
    using TObjectBase           = ObjectBase<BaseInterface>;
    using DeviceImplType        = typename ImplementationTraits::DeviceType;
    using BufferImplType        = typename ImplementationTraits::BufferType;
    using TextureImplType       = typename ImplementationTraits::TextureType;
    using PipelineStateImplType = typename ImplementationTraits::PipelineStateType;
    using TextureViewImplType   = typename TextureImplType::ViewImplType;
    using QueryImplType         = typename ImplementationTraits::QueryType;
    using FramebufferImplType   = typename ImplementationTraits::FramebufferType;
    using RenderPassImplType    = typename ImplementationTraits::RenderPassType;

    /// \param pRefCounters - reference counters object that controls the lifetime of this device context.
    /// \param pRenderDevice - render device.
    /// \param bIsDeferred - flag indicating if this instance is a deferred context
    DeviceContextBase(IReferenceCounters* pRefCounters, DeviceImplType* pRenderDevice, bool bIsDeferred) :
        // clang-format off
        TObjectBase  {pRefCounters },
        m_pDevice    {pRenderDevice},
        m_bIsDeferred{bIsDeferred  }
    // clang-format on
    {
    }

    ~DeviceContextBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceContext, TObjectBase)

    /// Base implementation of IDeviceContext::SetVertexBuffers(); validates parameters and
    /// caches references to the buffers.
    inline virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                            Uint32                         NumBuffersSet,
                                                            IBuffer**                      ppBuffers,
                                                            Uint32*                        pOffsets,
                                                            RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                            SET_VERTEX_BUFFERS_FLAGS       Flags) override = 0;

    inline virtual void DILIGENT_CALL_TYPE InvalidateState() override = 0;

    /// Base implementation of IDeviceContext::CommitShaderResources(); validates parameters.
    inline bool CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                      int);

    /// Base implementation of IDeviceContext::SetIndexBuffer(); caches the strong reference to the index buffer
    inline virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer* pIndexBuffer, Uint32 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override = 0;

    /// Caches the viewports
    inline void SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32& RTWidth, Uint32& RTHeight);

    /// Caches the scissor rects
    inline void SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32& RTWidth, Uint32& RTHeight);

    virtual void DILIGENT_CALL_TYPE BeginRenderPass(const BeginRenderPassAttribs& Attribs) override = 0;

    virtual void DILIGENT_CALL_TYPE NextSubpass() override = 0;

    virtual void DILIGENT_CALL_TYPE EndRenderPass() override = 0;

    /// Base implementation of IDeviceContext::UpdateBuffer(); validates input parameters.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint32                         Offset,
                                                 Uint32                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override = 0;

    /// Base implementation of IDeviceContext::CopyBuffer(); validates input parameters.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint32                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint32                         DstOffset,
                                               Uint32                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override = 0;

    /// Base implementation of IDeviceContext::MapBuffer(); validates input parameters.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer*  pBuffer,
                                              MAP_TYPE  MapType,
                                              MAP_FLAGS MapFlags,
                                              PVoid&    pMappedData) override = 0;

    /// Base implementation of IDeviceContext::UnmapBuffer()
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override = 0;

    /// Base implementaiton of IDeviceContext::UpdateData(); validates input parameters
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode) override = 0;

    /// Base implementaiton of IDeviceContext::CopyTexture(); validates input parameters
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override = 0;

    /// Base implementaiton of IDeviceContext::MapTextureSubresource()
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override = 0;

    /// Base implementaiton of IDeviceContext::UnmapTextureSubresource()
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture,
                                                            Uint32    MipLevel,
                                                            Uint32    ArraySlice) override = 0;

    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTexView) override = 0;

    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override = 0;

    /// Returns currently bound pipeline state and blend factors
    inline void GetPipelineState(IPipelineState** ppPSO, float* BlendFactors, Uint32& StencilRef);

    /// Returns currently bound render targets
    inline void GetRenderTargets(Uint32& NumRenderTargets, ITextureView** ppRTVs, ITextureView** ppDSV);

    /// Returns currently set viewports
    inline void GetViewports(Uint32& NumViewports, Viewport* pViewports);

    /// Returns the render device
    IRenderDevice* GetDevice() { return m_pDevice; }

    virtual void ResetRenderTargets();

    bool IsDeferred() const { return m_bIsDeferred; }

    /// Checks if a texture is bound as a render target or depth-stencil buffer and
    /// resets render targets if it is.
    bool UnbindTextureFromFramebuffer(TextureImplType* pTexture, bool bShowMessage);

protected:
    /// Caches the render target and depth stencil views. Returns true if any view is different
    /// from the cached value and false otherwise.
    inline bool SetRenderTargets(Uint32 NumRenderTargets, ITextureView* ppRenderTargets[], ITextureView* pDepthStencil);

    /// Initializes render targets for the current subpass
    inline bool SetSubpassRenderTargets();

    inline bool SetBlendFactors(const float* BlendFactors, int Dummy);

    inline bool SetStencilRef(Uint32 StencilRef, int Dummy);

    inline void SetPipelineState(PipelineStateImplType* pPipelineState, int /*Dummy*/);

    /// Clears all cached resources
    inline void ClearStateCache();

    /// Checks if the texture is currently bound as a render target.
    bool CheckIfBoundAsRenderTarget(TextureImplType* pTexture);

    /// Checks if the texture is currently bound as depth-stencil buffer.
    bool CheckIfBoundAsDepthStencil(TextureImplType* pTexture);

    /// Updates the states of render pass attachments to match states within the gievn subpass
    void UpdateAttachmentStates(Uint32 SubpassIndex);

    bool ClearDepthStencil(ITextureView* pView);

    bool ClearRenderTarget(ITextureView* pView);

    bool BeginQuery(IQuery* pQuery, int);

    bool EndQuery(IQuery* pQuery, int);

#ifdef DILIGENT_DEVELOPMENT
    // clang-format off
    bool DvpVerifyDrawArguments               (const DrawAttribs&                Attribs)const;
    bool DvpVerifyDrawIndexedArguments        (const DrawIndexedAttribs&         Attribs)const;
    bool DvpVerifyDrawMeshArguments           (const DrawMeshAttribs&            Attribs)const;
    bool DvpVerifyDrawIndirectArguments       (const DrawIndirectAttribs&        Attribs, const IBuffer* pAttribsBuffer)const;
    bool DvpVerifyDrawIndexedIndirectArguments(const DrawIndexedIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer)const;
    bool DvpVerifyDrawMeshIndirectArguments   (const DrawMeshIndirectAttribs&    Attribs, const IBuffer* pAttribsBuffer)const;

    bool DvpVerifyDispatchArguments        (const DispatchComputeAttribs& Attribs)const;
    bool DvpVerifyDispatchIndirectArguments(const DispatchComputeIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer)const;

    void DvpVerifyRenderTargets()const;
    void DvpVerifyStateTransitionDesc(const StateTransitionDesc& Barrier)const;
    bool DvpVerifyTextureState(const TextureImplType& Texture, RESOURCE_STATE RequiredState, const char* OperationName)const;
    bool DvpVerifyBufferState (const BufferImplType&  Buffer,  RESOURCE_STATE RequiredState, const char* OperationName)const;
#else
    bool DvpVerifyDrawArguments               (const DrawAttribs&                Attribs)const {return true;}
    bool DvpVerifyDrawIndexedArguments        (const DrawIndexedAttribs&         Attribs)const {return true;}
    bool DvpVerifyDrawMeshArguments           (const DrawMeshAttribs&            Attribs)const {return true;}
    bool DvpVerifyDrawIndirectArguments       (const DrawIndirectAttribs&        Attribs, const IBuffer* pAttribsBuffer)const {return true;}
    bool DvpVerifyDrawIndexedIndirectArguments(const DrawIndexedIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer)const {return true;}
    bool DvpVerifyDrawMeshIndirectArguments   (const DrawMeshIndirectAttribs&    Attribs, const IBuffer* pAttribsBuffer)const {return true;}

    bool DvpVerifyDispatchArguments        (const DispatchComputeAttribs& Attribs)const {return true;}
    bool DvpVerifyDispatchIndirectArguments(const DispatchComputeIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer)const {return true;}

    void DvpVerifyRenderTargets()const {}
    void DvpVerifyStateTransitionDesc(const StateTransitionDesc& Barrier)const {}
    bool DvpVerifyTextureState(const TextureImplType& Texture, RESOURCE_STATE RequiredState, const char* OperationName)const {return true;}
    bool DvpVerifyBufferState (const BufferImplType&  Buffer,  RESOURCE_STATE RequiredState, const char* OperationName)const {return true;}
    // clang-format on
#endif

    /// Strong reference to the device.
    RefCntAutoPtr<DeviceImplType> m_pDevice;

    /// Vertex streams. Every stream holds strong reference to the buffer
    VertexStreamInfo<BufferImplType> m_VertexStreams[MAX_BUFFER_SLOTS];

    /// Number of bound vertex streams
    Uint32 m_NumVertexStreams = 0;

    /// Strong reference to the bound pipeline state object.
    /// Use final PSO implementation type to avoid virtual calls to AddRef()/Release().
    /// We need to keep strong reference as we examine previous pipeline state in
    /// SetPipelineState()
    RefCntAutoPtr<PipelineStateImplType> m_pPipelineState;

    /// Strong reference to the bound index buffer.
    /// Use final buffer implementation type to avoid virtual calls to AddRef()/Release()
    RefCntAutoPtr<BufferImplType> m_pIndexBuffer;

    /// Offset from the beginning of the index buffer to the start of the index data, in bytes.
    Uint32 m_IndexDataStartOffset = 0;

    /// Current stencil reference value
    Uint32 m_StencilRef = 0;

    /// Curent blend factors
    Float32 m_BlendFactors[4] = {-1, -1, -1, -1};

    /// Current viewports
    Viewport m_Viewports[MAX_VIEWPORTS];
    /// Number of current viewports
    Uint32 m_NumViewports = 0;

    /// Current scissor rects
    Rect m_ScissorRects[MAX_VIEWPORTS];
    /// Number of current scissor rects
    Uint32 m_NumScissorRects = 0;

    /// Vector of strong references to the bound render targets.
    /// Use final texture view implementation type to avoid virtual calls to AddRef()/Release()
    RefCntAutoPtr<TextureViewImplType> m_pBoundRenderTargets[MAX_RENDER_TARGETS];
    /// Number of bound render targets
    Uint32 m_NumBoundRenderTargets = 0;
    /// Width of the currently bound framebuffer
    Uint32 m_FramebufferWidth = 0;
    /// Height of the currently bound framebuffer
    Uint32 m_FramebufferHeight = 0;
    /// Number of array slices in the currently bound framebuffer
    Uint32 m_FramebufferSlices = 0;

    /// Strong references to the bound depth stencil view.
    /// Use final texture view implementation type to avoid virtual calls to AddRef()/Release()
    RefCntAutoPtr<TextureViewImplType> m_pBoundDepthStencil;

    /// Strong reference to the bound framebuffer.
    RefCntAutoPtr<FramebufferImplType> m_pBoundFramebuffer;

    /// Strong reference to the render pass.
    RefCntAutoPtr<RenderPassImplType> m_pActiveRenderPass;

    /// Current subpass index.
    Uint32 m_SubpassIndex = 0;

    /// Render pass attachments transition mode.
    RESOURCE_STATE_TRANSITION_MODE m_RenderPassAttachmentsTransitionMode = RESOURCE_STATE_TRANSITION_MODE_NONE;

    const bool m_bIsDeferred = false;

#ifdef DILIGENT_DEBUG
    // std::unordered_map is unbelievably slow. Keeping track of mapped buffers
    // in release builds is not feasible
    struct DbgMappedBufferInfo
    {
        MAP_TYPE MapType;
    };
    std::unordered_map<IBuffer*, DbgMappedBufferInfo> m_DbgMappedBuffers;
#endif
};


template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    SetVertexBuffers(Uint32                         StartSlot,
                     Uint32                         NumBuffersSet,
                     IBuffer**                      ppBuffers,
                     Uint32*                        pOffsets,
                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                     SET_VERTEX_BUFFERS_FLAGS       Flags)
{
#ifdef DILIGENT_DEVELOPMENT
    if (StartSlot >= MAX_BUFFER_SLOTS)
    {
        LOG_ERROR_MESSAGE("Start vertex buffer slot ", StartSlot, " is out of allowed range [0, ", MAX_BUFFER_SLOTS - 1, "].");
        return;
    }

    if (StartSlot + NumBuffersSet > MAX_BUFFER_SLOTS)
    {
        LOG_ERROR_MESSAGE("The range of vertex buffer slots being set [", StartSlot, ", ", StartSlot + NumBuffersSet - 1, "] is out of allowed range  [0, ", MAX_BUFFER_SLOTS - 1, "].");
        NumBuffersSet = MAX_BUFFER_SLOTS - StartSlot;
    }

    VERIFY(!(m_pActiveRenderPass != nullptr && StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION),
           "Resource state transitons are not allowed inside a render pass and may result in an undefined behavior. "
           "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");
#endif

    if (Flags & SET_VERTEX_BUFFERS_FLAG_RESET)
    {
        // Reset only these buffer slots that are not being set.
        // It is very important to not reset buffers that stay unchanged
        // as AddRef()/Release() are not free
        for (Uint32 s = 0; s < StartSlot; ++s)
            m_VertexStreams[s] = VertexStreamInfo<BufferImplType>{};
        for (Uint32 s = StartSlot + NumBuffersSet; s < m_NumVertexStreams; ++s)
            m_VertexStreams[s] = VertexStreamInfo<BufferImplType>{};
        m_NumVertexStreams = 0;
    }
    m_NumVertexStreams = std::max(m_NumVertexStreams, StartSlot + NumBuffersSet);

    for (Uint32 Buff = 0; Buff < NumBuffersSet; ++Buff)
    {
        auto& CurrStream   = m_VertexStreams[StartSlot + Buff];
        CurrStream.pBuffer = ppBuffers ? ValidatedCast<BufferImplType>(ppBuffers[Buff]) : nullptr;
        CurrStream.Offset  = pOffsets ? pOffsets[Buff] : 0;
#ifdef DILIGENT_DEVELOPMENT
        if (CurrStream.pBuffer)
        {
            const auto& BuffDesc = CurrStream.pBuffer->GetDesc();
            if (!(BuffDesc.BindFlags & BIND_VERTEX_BUFFER))
            {
                LOG_ERROR_MESSAGE("Buffer '", BuffDesc.Name ? BuffDesc.Name : "", "' being bound as vertex buffer to slot ", Buff, " was not created with BIND_VERTEX_BUFFER flag");
            }
        }
#endif
    }
    // Remove null buffers from the end of the array
    while (m_NumVertexStreams > 0 && !m_VertexStreams[m_NumVertexStreams - 1].pBuffer)
        m_VertexStreams[m_NumVertexStreams--] = VertexStreamInfo<BufferImplType>{};
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    SetPipelineState(PipelineStateImplType* pPipelineState, int /*Dummy*/)
{
    m_pPipelineState = pPipelineState;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    CommitShaderResources(IShaderResourceBinding* pShaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode, int)
{
#ifdef DILIGENT_DEVELOPMENT
    VERIFY(!(m_pActiveRenderPass != nullptr && StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION),
           "Resource state transitons are not allowed inside a render pass and may result in an undefined behavior. "
           "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");

    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("No pipeline state is bound to the pipeline");
        return false;
    }

    if (pShaderResourceBinding)
    {
        if (m_pPipelineState->IsIncompatibleWith(pShaderResourceBinding->GetPipelineState()))
        {
            LOG_ERROR_MESSAGE("Shader resource binding object is not compatible with the currently bound pipeline state '", m_pPipelineState->GetDesc().Name, '\'');
            return false;
        }
    }
#endif
    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::InvalidateState()
{
    if (m_pActiveRenderPass != nullptr)
        LOG_ERROR_MESSAGE("Invalidating context inside an active render pass. Call EndRenderPass() to finish the pass.");

    DeviceContextBase<BaseInterface, ImplementationTraits>::ClearStateCache();
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    SetIndexBuffer(IBuffer* pIndexBuffer, Uint32 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    m_pIndexBuffer         = ValidatedCast<BufferImplType>(pIndexBuffer);
    m_IndexDataStartOffset = ByteOffset;
#ifdef DILIGENT_DEVELOPMENT
    VERIFY(!(m_pActiveRenderPass != nullptr && StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION),
           "Resource state transitons are not allowed inside a render pass and may result in an undefined behavior. "
           "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");

    if (m_pIndexBuffer)
    {
        const auto& BuffDesc = m_pIndexBuffer->GetDesc();
        if (!(BuffDesc.BindFlags & BIND_INDEX_BUFFER))
        {
            LOG_ERROR_MESSAGE("Buffer '", BuffDesc.Name ? BuffDesc.Name : "", "' being bound as index buffer was not created with BIND_INDEX_BUFFER flag");
        }
    }
#endif
}


template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::GetPipelineState(IPipelineState** ppPSO, float* BlendFactors, Uint32& StencilRef)
{
    VERIFY(ppPSO != nullptr, "Null pointer provided null");
    VERIFY(*ppPSO == nullptr, "Memory address contains a pointer to a non-null blend state");
    if (m_pPipelineState)
    {
        m_pPipelineState->QueryInterface(IID_PipelineState, reinterpret_cast<IObject**>(ppPSO));
    }
    else
    {
        *ppPSO = nullptr;
    }

    for (Uint32 f = 0; f < 4; ++f)
        BlendFactors[f] = m_BlendFactors[f];
    StencilRef = m_StencilRef;
};

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::SetBlendFactors(const float* BlendFactors, int)
{
    bool FactorsDiffer = false;
    for (Uint32 f = 0; f < 4; ++f)
    {
        if (m_BlendFactors[f] != BlendFactors[f])
            FactorsDiffer = true;
        m_BlendFactors[f] = BlendFactors[f];
    }
    return FactorsDiffer;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::SetStencilRef(Uint32 StencilRef, int)
{
    if (m_StencilRef != StencilRef)
    {
        m_StencilRef = StencilRef;
        return true;
    }
    return false;
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32& RTWidth, Uint32& RTHeight)
{
    if (RTWidth == 0 || RTHeight == 0)
    {
        RTWidth  = m_FramebufferWidth;
        RTHeight = m_FramebufferHeight;
    }

    VERIFY(NumViewports < MAX_VIEWPORTS, "Number of viewports (", NumViewports, ") exceeds the limit (", MAX_VIEWPORTS, ")");
    m_NumViewports = std::min(MAX_VIEWPORTS, NumViewports);

    Viewport DefaultVP(0, 0, static_cast<float>(RTWidth), static_cast<float>(RTHeight));
    // If no viewports are specified, use default viewport
    if (m_NumViewports == 1 && pViewports == nullptr)
    {
        pViewports = &DefaultVP;
    }

    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
    {
        m_Viewports[vp] = pViewports[vp];
        VERIFY(m_Viewports[vp].Width >= 0, "Incorrect viewport width (", m_Viewports[vp].Width, ")");
        VERIFY(m_Viewports[vp].Height >= 0, "Incorrect viewport height (", m_Viewports[vp].Height, ")");
        VERIFY(m_Viewports[vp].MaxDepth >= m_Viewports[vp].MinDepth, "Incorrect viewport depth range [", m_Viewports[vp].MinDepth, ", ", m_Viewports[vp].MaxDepth, "]");
    }
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::GetViewports(Uint32& NumViewports, Viewport* pViewports)
{
    NumViewports = m_NumViewports;
    if (pViewports)
    {
        for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
            pViewports[vp] = m_Viewports[vp];
    }
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32& RTWidth, Uint32& RTHeight)
{
    if (RTWidth == 0 || RTHeight == 0)
    {
        RTWidth  = m_FramebufferWidth;
        RTHeight = m_FramebufferHeight;
    }

    VERIFY(NumRects < MAX_VIEWPORTS, "Number of scissor rects (", NumRects, ") exceeds the limit (", MAX_VIEWPORTS, ")");
    m_NumScissorRects = std::min(MAX_VIEWPORTS, NumRects);

    for (Uint32 sr = 0; sr < m_NumScissorRects; ++sr)
    {
        m_ScissorRects[sr] = pRects[sr];
        VERIFY(m_ScissorRects[sr].left <= m_ScissorRects[sr].right, "Incorrect horizontal bounds for a scissor rect [", m_ScissorRects[sr].left, ", ", m_ScissorRects[sr].right, ")");
        VERIFY(m_ScissorRects[sr].top <= m_ScissorRects[sr].bottom, "Incorrect vertical bounds for a scissor rect [", m_ScissorRects[sr].top, ", ", m_ScissorRects[sr].bottom, ")");
    }
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    SetRenderTargets(Uint32 NumRenderTargets, ITextureView* ppRenderTargets[], ITextureView* pDepthStencil)
{
    if (NumRenderTargets == 0 && pDepthStencil == nullptr)
    {
        ResetRenderTargets();
        return false;
    }

    bool bBindRenderTargets = false;
    m_FramebufferWidth      = 0;
    m_FramebufferHeight     = 0;
    m_FramebufferSlices     = 0;

    if (NumRenderTargets != m_NumBoundRenderTargets)
    {
        bBindRenderTargets = true;
        for (Uint32 rt = NumRenderTargets; rt < m_NumBoundRenderTargets; ++rt)
            m_pBoundRenderTargets[rt].Release();

        m_NumBoundRenderTargets = NumRenderTargets;
    }

    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        auto* pRTView = ppRenderTargets[rt];
        if (pRTView)
        {
            const auto& RTVDesc = pRTView->GetDesc();
#ifdef DILIGENT_DEVELOPMENT
            if (RTVDesc.ViewType != TEXTURE_VIEW_RENDER_TARGET)
                LOG_ERROR_MESSAGE("Texture view object named '", RTVDesc.Name ? RTVDesc.Name : "", "' has incorrect view type (", GetTexViewTypeLiteralName(RTVDesc.ViewType), "). Render target view is expected");
#endif
            // Use this RTV to set the render target size
            if (m_FramebufferWidth == 0)
            {
                auto*       pTex    = pRTView->GetTexture();
                const auto& TexDesc = pTex->GetDesc();
                m_FramebufferWidth  = std::max(TexDesc.Width >> RTVDesc.MostDetailedMip, 1U);
                m_FramebufferHeight = std::max(TexDesc.Height >> RTVDesc.MostDetailedMip, 1U);
                m_FramebufferSlices = RTVDesc.NumArraySlices;
            }
            else
            {
#ifdef DILIGENT_DEVELOPMENT
                const auto& TexDesc = pRTView->GetTexture()->GetDesc();
                if (m_FramebufferWidth != std::max(TexDesc.Width >> RTVDesc.MostDetailedMip, 1U))
                    LOG_ERROR_MESSAGE("Render target width (", std::max(TexDesc.Width >> RTVDesc.MostDetailedMip, 1U), ") specified by RTV '", RTVDesc.Name, "' is inconsistent with the width of previously bound render targets (", m_FramebufferWidth, ")");
                if (m_FramebufferHeight != std::max(TexDesc.Height >> RTVDesc.MostDetailedMip, 1U))
                    LOG_ERROR_MESSAGE("Render target height (", std::max(TexDesc.Height >> RTVDesc.MostDetailedMip, 1U), ") specified by RTV '", RTVDesc.Name, "' is inconsistent with the height of previously bound render targets (", m_FramebufferHeight, ")");
                if (m_FramebufferSlices != RTVDesc.NumArraySlices)
                    LOG_ERROR_MESSAGE("Number of slices (", RTVDesc.NumArraySlices, ") specified by RTV '", RTVDesc.Name, "' is inconsistent with the number of slices in previously bound render targets (", m_FramebufferSlices, ")");
#endif
            }
        }

        // Here both views are certainly live objects, since we store
        // strong references to all bound render targets. So we
        // can safely compare pointers.
        if (m_pBoundRenderTargets[rt] != pRTView)
        {
            m_pBoundRenderTargets[rt] = ValidatedCast<TextureViewImplType>(pRTView);
            bBindRenderTargets        = true;
        }
    }

    if (pDepthStencil != nullptr)
    {
        const auto& DSVDesc = pDepthStencil->GetDesc();
#ifdef DILIGENT_DEVELOPMENT
        if (DSVDesc.ViewType != TEXTURE_VIEW_DEPTH_STENCIL)
            LOG_ERROR_MESSAGE("Texture view object named '", DSVDesc.Name ? DSVDesc.Name : "", "' has incorrect view type (", GetTexViewTypeLiteralName(DSVDesc.ViewType), "). Depth stencil view is expected");
#endif

        // Use depth stencil size to set render target size
        if (m_FramebufferWidth == 0)
        {
            auto*       pTex    = pDepthStencil->GetTexture();
            const auto& TexDesc = pTex->GetDesc();
            m_FramebufferWidth  = std::max(TexDesc.Width >> DSVDesc.MostDetailedMip, 1U);
            m_FramebufferHeight = std::max(TexDesc.Height >> DSVDesc.MostDetailedMip, 1U);
            m_FramebufferSlices = DSVDesc.NumArraySlices;
        }
        else
        {
#ifdef DILIGENT_DEVELOPMENT
            const auto& TexDesc = pDepthStencil->GetTexture()->GetDesc();
            if (m_FramebufferWidth != std::max(TexDesc.Width >> DSVDesc.MostDetailedMip, 1U))
                LOG_ERROR_MESSAGE("Depth-stencil target width (", std::max(TexDesc.Width >> DSVDesc.MostDetailedMip, 1U), ") specified by DSV '", DSVDesc.Name, "' is inconsistent with the width of previously bound render targets (", m_FramebufferWidth, ")");
            if (m_FramebufferHeight != std::max(TexDesc.Height >> DSVDesc.MostDetailedMip, 1U))
                LOG_ERROR_MESSAGE("Depth-stencil target height (", std::max(TexDesc.Height >> DSVDesc.MostDetailedMip, 1U), ") specified by DSV '", DSVDesc.Name, "' is inconsistent with the height of previously bound render targets (", m_FramebufferHeight, ")");
            if (m_FramebufferSlices != DSVDesc.NumArraySlices)
                LOG_ERROR_MESSAGE("Number of slices (", DSVDesc.NumArraySlices, ") specified by DSV '", DSVDesc.Name, "' is inconsistent with the number of slices in previously bound render targets (", m_FramebufferSlices, ")");
#endif
        }
    }

    if (m_pBoundDepthStencil != pDepthStencil)
    {
        m_pBoundDepthStencil = ValidatedCast<TextureViewImplType>(pDepthStencil);
        bBindRenderTargets   = true;
    }


    VERIFY_EXPR(m_FramebufferWidth > 0 && m_FramebufferHeight > 0 && m_FramebufferSlices > 0);

    return bBindRenderTargets;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::SetSubpassRenderTargets()
{
    VERIFY_EXPR(m_pBoundFramebuffer);
    VERIFY_EXPR(m_pActiveRenderPass);

    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    const auto& FBDesc = m_pBoundFramebuffer->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& Subpass = RPDesc.pSubpasses[m_SubpassIndex];

    ITextureView* ppRTVs[MAX_RENDER_TARGETS] = {};
    ITextureView* pDSV                       = nullptr;
    for (Uint32 rt = 0; rt < Subpass.RenderTargetAttachmentCount; ++rt)
    {
        const auto& RTAttachmentRef = Subpass.pRenderTargetAttachments[rt];
        if (RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            VERIFY_EXPR(RTAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
            ppRTVs[rt] = FBDesc.ppAttachments[RTAttachmentRef.AttachmentIndex];
        }
    }

    if (Subpass.pDepthStencilAttachment != nullptr)
    {
        const auto& DSAttachmentRef = *Subpass.pDepthStencilAttachment;
        if (DSAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            VERIFY_EXPR(DSAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
            pDSV = FBDesc.ppAttachments[DSAttachmentRef.AttachmentIndex];
        }
    }
    bool BindRenderTargets = SetRenderTargets(Subpass.RenderTargetAttachmentCount, ppRTVs, pDSV);

    // Use framebuffer dimensions (override what was set by SetRenderTargets)
    m_FramebufferWidth  = FBDesc.Width;
    m_FramebufferHeight = FBDesc.Height;
    m_FramebufferSlices = FBDesc.NumArraySlices;

    return BindRenderTargets;
}


template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    GetRenderTargets(Uint32& NumRenderTargets, ITextureView** ppRTVs, ITextureView** ppDSV)
{
    NumRenderTargets = m_NumBoundRenderTargets;

    if (ppRTVs)
    {
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        {
            VERIFY(ppRTVs[rt] == nullptr, "Non-null pointer found in RTV array element #", rt);
            auto pBoundRTV = m_pBoundRenderTargets[rt];
            if (pBoundRTV)
                pBoundRTV->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppRTVs + rt));
            else
                ppRTVs[rt] = nullptr;
        }
        for (Uint32 rt = NumRenderTargets; rt < MAX_RENDER_TARGETS; ++rt)
        {
            VERIFY(ppRTVs[rt] == nullptr, "Non-null pointer found in RTV array element #", rt);
            ppRTVs[rt] = nullptr;
        }
    }

    if (ppDSV)
    {
        VERIFY(*ppDSV == nullptr, "Non-null DSV pointer found");
        if (m_pBoundDepthStencil)
            m_pBoundDepthStencil->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppDSV));
        else
            *ppDSV = nullptr;
    }
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::ClearStateCache()
{
    for (Uint32 stream = 0; stream < m_NumVertexStreams; ++stream)
        m_VertexStreams[stream] = VertexStreamInfo<BufferImplType>{};
#ifdef DILIGENT_DEBUG
    for (Uint32 stream = m_NumVertexStreams; stream < _countof(m_VertexStreams); ++stream)
    {
        VERIFY(m_VertexStreams[stream].pBuffer == nullptr, "Unexpected non-null buffer");
        VERIFY(m_VertexStreams[stream].Offset == 0, "Unexpected non-zero offset");
    }
#endif
    m_NumVertexStreams = 0;

    m_pPipelineState.Release();

    m_pIndexBuffer.Release();
    m_IndexDataStartOffset = 0;

    m_StencilRef = 0;

    for (int i = 0; i < 4; ++i)
        m_BlendFactors[i] = -1;

    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
        m_Viewports[vp] = Viewport();
    m_NumViewports = 0;

    for (Uint32 sr = 0; sr < m_NumScissorRects; ++sr)
        m_ScissorRects[sr] = Rect();
    m_NumScissorRects = 0;

    ResetRenderTargets();

    VERIFY(!m_pActiveRenderPass, "Clearing state cache inside an active render pass");
    m_pActiveRenderPass = nullptr;
    m_pBoundFramebuffer = nullptr;
}

template <typename BaseInterface, typename ImplementationTraits>
bool DeviceContextBase<BaseInterface, ImplementationTraits>::CheckIfBoundAsRenderTarget(TextureImplType* pTexture)
{
    if (pTexture == nullptr)
        return false;

    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (m_pBoundRenderTargets[rt] && m_pBoundRenderTargets[rt]->GetTexture() == pTexture)
        {
            return true;
        }
    }

    return false;
}

template <typename BaseInterface, typename ImplementationTraits>
bool DeviceContextBase<BaseInterface, ImplementationTraits>::CheckIfBoundAsDepthStencil(TextureImplType* pTexture)
{
    if (pTexture == nullptr)
        return false;

    return m_pBoundDepthStencil && m_pBoundDepthStencil->GetTexture() == pTexture;
}

template <typename BaseInterface, typename ImplementationTraits>
bool DeviceContextBase<BaseInterface, ImplementationTraits>::UnbindTextureFromFramebuffer(TextureImplType* pTexture, bool bShowMessage)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass.");

    if (pTexture == nullptr)
        return false;

    const auto& TexDesc = pTexture->GetDesc();

    bool bResetRenderTargets = false;
    if (TexDesc.BindFlags & BIND_RENDER_TARGET)
    {
        if (CheckIfBoundAsRenderTarget(pTexture))
        {
            if (bShowMessage)
            {
                LOG_INFO_MESSAGE("Texture '", TexDesc.Name,
                                 "' is currently bound as render target and will be unset along with all "
                                 "other render targets and depth-stencil buffer. "
                                 "Call SetRenderTargets() to reset the render targets.\n"
                                 "To silence this message, explicitly unbind the texture with "
                                 "SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE)");
            }

            bResetRenderTargets = true;
        }
    }

    if (TexDesc.BindFlags & BIND_DEPTH_STENCIL)
    {
        if (CheckIfBoundAsDepthStencil(pTexture))
        {
            if (bShowMessage)
            {
                LOG_INFO_MESSAGE("Texture '", TexDesc.Name,
                                 "' is currently bound as depth buffer and will be unset along with "
                                 "all render targets. Call SetRenderTargets() to reset the render targets.\n"
                                 "To silence this message, explicitly unbind the texture with "
                                 "SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE)");
            }

            bResetRenderTargets = true;
        }
    }

    if (bResetRenderTargets)
    {
        ResetRenderTargets();
    }

    return bResetRenderTargets;
}

template <typename BaseInterface, typename ImplementationTraits>
void DeviceContextBase<BaseInterface, ImplementationTraits>::ResetRenderTargets()
{
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
        m_pBoundRenderTargets[rt].Release();
#ifdef DILIGENT_DEBUG
    for (Uint32 rt = m_NumBoundRenderTargets; rt < _countof(m_pBoundRenderTargets); ++rt)
    {
        VERIFY(m_pBoundRenderTargets[rt] == nullptr, "Non-null render target found");
    }
#endif
    m_NumBoundRenderTargets = 0;
    m_FramebufferWidth      = 0;
    m_FramebufferHeight     = 0;
    m_FramebufferSlices     = 0;

    m_pBoundDepthStencil.Release();

    // Do not reset framebuffer here as there may potentially
    // be a subpass without any render target attachments.
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    VERIFY(m_pActiveRenderPass == nullptr, "Attempting to begin render pass while another render pass ('", m_pActiveRenderPass->GetDesc().Name, "') is active.");
    VERIFY(m_pBoundFramebuffer == nullptr, "Attempting to begin render pass while another framebuffer ('", m_pBoundFramebuffer->GetDesc().Name, "') is bound.");
    VERIFY(Attribs.pRenderPass != nullptr, "Render pass must not be null");
    VERIFY(Attribs.pFramebuffer != nullptr, "Framebuffer must not be null");
#ifdef DILIGENT_DEBUG
    {
        const auto& RPDesc = Attribs.pRenderPass->GetDesc();

        Uint32 NumRequiredClearValues = 0;
        for (Uint32 i = 0; i < RPDesc.AttachmentCount; ++i)
        {
            const auto& Attchmnt = RPDesc.pAttachments[i];
            if (Attchmnt.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
                NumRequiredClearValues = i + 1;

            const auto& FmtAttribs = GetTextureFormatAttribs(Attchmnt.Format);
            if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            {
                if (Attchmnt.StencilLoadOp == ATTACHMENT_LOAD_OP_CLEAR)
                    NumRequiredClearValues = i + 1;
            }
        }
        VERIFY(Attribs.ClearValueCount >= NumRequiredClearValues,
               "Begin render pass operation requiers at least ", NumRequiredClearValues,
               " clear values, but only ", Attribs.ClearValueCount, " are given.");
        VERIFY(Attribs.ClearValueCount == 0 || Attribs.pClearValues != nullptr,
               "pClearValues must not be null when ClearValueCount is not zero");
    }
#endif

    // Reset current render targets (in Vulkan backend, this may end current render pass).
    ResetRenderTargets();

    auto* pNewRenderPass  = ValidatedCast<RenderPassImplType>(Attribs.pRenderPass);
    auto* pNewFramebuffer = ValidatedCast<FramebufferImplType>(Attribs.pFramebuffer);
    if (Attribs.StateTransitionMode != RESOURCE_STATE_TRANSITION_MODE_NONE)
    {
        const auto& RPDesc = pNewRenderPass->GetDesc();
        const auto& FBDesc = pNewFramebuffer->GetDesc();
        VERIFY(RPDesc.AttachmentCount <= FBDesc.AttachmentCount,
               "The number of attachments (", FBDesc.AttachmentCount,
               ") in currently bound framebuffer is smaller than the number of attachments in the render pass (", RPDesc.AttachmentCount, ")");
        for (Uint32 i = 0; i < FBDesc.AttachmentCount; ++i)
        {
            auto* pView = FBDesc.ppAttachments[i];
            if (pView == nullptr)
                return;

            auto* pTex          = ValidatedCast<TextureImplType>(pView->GetTexture());
            auto  RequiredState = RPDesc.pAttachments[i].InitialState;
            if (Attribs.StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
            {
                if (pTex->IsInKnownState() && !pTex->CheckState(RequiredState))
                {
                    StateTransitionDesc Barrier{pTex, RESOURCE_STATE_UNKNOWN, RequiredState, true};
                    this->TransitionResourceStates(1, &Barrier);
                }
            }
            else if (Attribs.StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
            {
                DvpVerifyTextureState(*pTex, RequiredState, "BeginRenderPass");
            }
        }
    }

    m_pActiveRenderPass                   = pNewRenderPass;
    m_pBoundFramebuffer                   = pNewFramebuffer;
    m_SubpassIndex                        = 0;
    m_RenderPassAttachmentsTransitionMode = Attribs.StateTransitionMode;
    UpdateAttachmentStates(m_SubpassIndex);
    SetSubpassRenderTargets();
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::NextSubpass()
{
    VERIFY(m_pActiveRenderPass != nullptr, "There is no active render pass");
    VERIFY(m_SubpassIndex + 1 < m_pActiveRenderPass->GetDesc().SubpassCount, "The render pass has reached the final subpass already");
    ++m_SubpassIndex;
    UpdateAttachmentStates(m_SubpassIndex);
    SetSubpassRenderTargets();
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::UpdateAttachmentStates(Uint32 SubpassIndex)
{
    if (m_RenderPassAttachmentsTransitionMode != RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
        return;

    VERIFY_EXPR(m_pActiveRenderPass != nullptr);
    VERIFY_EXPR(m_pBoundFramebuffer != nullptr);

    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    const auto& FBDesc = m_pBoundFramebuffer->GetDesc();
    VERIFY(FBDesc.AttachmentCount == RPDesc.AttachmentCount,
           "Framebuffer attachment count (", FBDesc.AttachmentCount, ") is not consistent with the render pass attachment count (", RPDesc.AttachmentCount, ")");
    VERIFY_EXPR(SubpassIndex <= RPDesc.SubpassCount);
    for (Uint32 i = 0; i < RPDesc.AttachmentCount; ++i)
    {
        if (auto* pView = FBDesc.ppAttachments[i])
        {
            auto* pTex = ValidatedCast<TextureImplType>(pView->GetTexture());
            if (pTex->IsInKnownState())
            {
                auto CurrState = SubpassIndex < RPDesc.SubpassCount ?
                    m_pActiveRenderPass->GetAttachmentState(SubpassIndex, i) :
                    RPDesc.pAttachments[i].FinalState;
                pTex->SetState(CurrState);
            }
        }
    }
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::EndRenderPass()
{
    VERIFY(m_pActiveRenderPass != nullptr, "There is no active render pass");
    VERIFY(m_pBoundFramebuffer != nullptr, "There is no active framebuffer");
    VERIFY(m_pActiveRenderPass->GetDesc().SubpassCount == m_SubpassIndex + 1,
           "Ending render pass at subpass ", m_SubpassIndex, " before reaching the final subpass");

    UpdateAttachmentStates(m_SubpassIndex + 1);

    m_pActiveRenderPass.Release();
    m_pBoundFramebuffer.Release();
    m_SubpassIndex                        = 0;
    m_RenderPassAttachmentsTransitionMode = RESOURCE_STATE_TRANSITION_MODE_NONE;
    ResetRenderTargets();
}


template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::ClearDepthStencil(ITextureView* pView)
{
    if (pView == nullptr)
    {
        LOG_ERROR_MESSAGE("Depth-stencil view to clear must not be null");
        return false;
    }

#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& ViewDesc = pView->GetDesc();
        if (ViewDesc.ViewType != TEXTURE_VIEW_DEPTH_STENCIL)
        {
            LOG_ERROR_MESSAGE("The type (", GetTexViewTypeLiteralName(ViewDesc.ViewType), ") of the texture view '", ViewDesc.Name,
                              "' is invalid: ClearDepthStencil command expects depth-stencil view (TEXTURE_VIEW_DEPTH_STENCIL).");
            return false;
        }

        if (pView != m_pBoundDepthStencil)
        {
            if (m_pActiveRenderPass != nullptr)
            {
                LOG_ERROR_MESSAGE("Depth-stencil view '", ViewDesc.Name,
                                  "' is not bound as framebuffer attachment. ClearDepthStencil command inside a render pass "
                                  "requires depth-stencil view to be bound as a framebuffer attachment.");
                return false;
            }
            else if (m_pDevice->GetDeviceCaps().IsGLDevice())
            {
                LOG_ERROR_MESSAGE("Depth-stencil view '", ViewDesc.Name,
                                  "' is not bound to the device context. ClearDepthStencil command requires "
                                  "depth-stencil view be bound to the device contex in OpenGL backend");
                return false;
            }
            else
            {
                LOG_WARNING_MESSAGE("Depth-stencil view '", ViewDesc.Name,
                                    "' is not bound to the device context. "
                                    "ClearDepthStencil command is more efficient when depth-stencil "
                                    "view is bound to the context. In OpenGL backend this is a requirement.");
            }
        }
    }
#endif

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::ClearRenderTarget(ITextureView* pView)
{
    if (pView == nullptr)
    {
        LOG_ERROR_MESSAGE("Render target view to clear must not be null");
        return false;
    }

#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& ViewDesc = pView->GetDesc();
        if (ViewDesc.ViewType != TEXTURE_VIEW_RENDER_TARGET)
        {
            LOG_ERROR_MESSAGE("The type (", GetTexViewTypeLiteralName(ViewDesc.ViewType), ") of texture view '", pView->GetDesc().Name,
                              "' is invalid: ClearRenderTarget command expects render target view (TEXTURE_VIEW_RENDER_TARGET).");
            return false;
        }

        bool RTFound = false;
        for (Uint32 i = 0; i < m_NumBoundRenderTargets && !RTFound; ++i)
        {
            RTFound = m_pBoundRenderTargets[i] == pView;
        }

        if (!RTFound)
        {
            if (m_pActiveRenderPass != nullptr)
            {
                LOG_ERROR_MESSAGE("Render target view '", ViewDesc.Name,
                                  "' is not bound as framebuffer attachment. ClearRenderTarget command inside a render pass "
                                  "requires render target view to be bound as a framebuffer attachment.");
                return false;
            }
            else if (m_pDevice->GetDeviceCaps().IsGLDevice())
            {
                LOG_ERROR_MESSAGE("Render target view '", ViewDesc.Name,
                                  "' is not bound to the device context. ClearRenderTarget command "
                                  "requires render target view to be bound to the device contex in OpenGL backend");
                return false;
            }
            else
            {
                LOG_WARNING_MESSAGE("Render target view '", ViewDesc.Name,
                                    "' is not bound to the device context. ClearRenderTarget command is more efficient "
                                    "if render target view is bound to the device context. In OpenGL backend this is a requirement.");
            }
        }
    }
#endif

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::BeginQuery(IQuery* pQuery, int)
{
    if (pQuery == nullptr)
    {
        LOG_ERROR_MESSAGE("IDeviceContext::BeginQuery: pQuery must not be null");
        return false;
    }

    if (m_bIsDeferred)
    {
        LOG_ERROR_MESSAGE("IDeviceContext::BeginQuery: Deferred contexts do not support queries");
        return false;
    }

    if (pQuery->GetDesc().Type == QUERY_TYPE_TIMESTAMP)
    {
        LOG_ERROR_MESSAGE("BeginQuery() is disabled for timestamp queries. Call EndQuery() to set the timestamp.");
        return false;
    }

    if (!ValidatedCast<QueryImplType>(pQuery)->OnBeginQuery(this))
        return false;

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::EndQuery(IQuery* pQuery, int)
{
    if (pQuery == nullptr)
    {
        LOG_ERROR_MESSAGE("IDeviceContext::EndQuery: pQuery must not be null");
        return false;
    }

    if (m_bIsDeferred)
    {
        LOG_ERROR_MESSAGE("IDeviceContext::EndQuery: Deferred contexts do not support queries");
        return false;
    }

    if (!ValidatedCast<QueryImplType>(pQuery)->OnEndQuery(this))
        return false;

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    UpdateBuffer(IBuffer* pBuffer, Uint32 Offset, Uint32 Size, const void* pData, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    VERIFY(pBuffer != nullptr, "Buffer must not be null");
    VERIFY(m_pActiveRenderPass == nullptr, "UpdateBuffer command must be used outside of render pass.");
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& BuffDesc = ValidatedCast<BufferImplType>(pBuffer)->GetDesc();
        DEV_CHECK_ERR(BuffDesc.Usage == USAGE_DEFAULT, "Unable to update buffer '", BuffDesc.Name, "': only USAGE_DEFAULT buffers can be updated with UpdateData()");
        DEV_CHECK_ERR(Offset < BuffDesc.uiSizeInBytes, "Unable to update buffer '", BuffDesc.Name, "': offset (", Offset, ") exceeds the buffer size (", BuffDesc.uiSizeInBytes, ")");
        DEV_CHECK_ERR(Size + Offset <= BuffDesc.uiSizeInBytes, "Unable to update buffer '", BuffDesc.Name, "': Update region [", Offset, ",", Size + Offset, ") is out of buffer bounds [0,", BuffDesc.uiSizeInBytes, ")");
    }
#endif
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    CopyBuffer(IBuffer*                       pSrcBuffer,
               Uint32                         SrcOffset,
               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
               IBuffer*                       pDstBuffer,
               Uint32                         DstOffset,
               Uint32                         Size,
               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    VERIFY(pSrcBuffer != nullptr, "Source buffer must not be null");
    VERIFY(pDstBuffer != nullptr, "Destination buffer must not be null");
    VERIFY(m_pActiveRenderPass == nullptr, "CopyBuffer command must be used outside of render pass.");
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& SrcBufferDesc = ValidatedCast<BufferImplType>(pSrcBuffer)->GetDesc();
        const auto& DstBufferDesc = ValidatedCast<BufferImplType>(pDstBuffer)->GetDesc();
        DEV_CHECK_ERR(DstOffset + Size <= DstBufferDesc.uiSizeInBytes, "Failed to copy buffer '", SrcBufferDesc.Name, "' to '", DstBufferDesc.Name, "': Destination range [", DstOffset, ",", DstOffset + Size, ") is out of buffer bounds [0,", DstBufferDesc.uiSizeInBytes, ")");
        DEV_CHECK_ERR(SrcOffset + Size <= SrcBufferDesc.uiSizeInBytes, "Failed to copy buffer '", SrcBufferDesc.Name, "' to '", DstBufferDesc.Name, "': Source range [", SrcOffset, ",", SrcOffset + Size, ") is out of buffer bounds [0,", SrcBufferDesc.uiSizeInBytes, ")");
    }
#endif
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData)
{
    VERIFY(pBuffer, "pBuffer must not be null");

    const auto& BuffDesc = pBuffer->GetDesc();

#ifdef DILIGENT_DEBUG
    {
        VERIFY(m_DbgMappedBuffers.find(pBuffer) == m_DbgMappedBuffers.end(), "Buffer '", BuffDesc.Name, "' has already been mapped");
        m_DbgMappedBuffers[pBuffer] = DbgMappedBufferInfo{MapType};
    }
#endif

    pMappedData = nullptr;
    switch (MapType)
    {
        case MAP_READ:
            DEV_CHECK_ERR(BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED,
                          "Only buffers with usage USAGE_STAGING or USAGE_UNIFIED can be mapped for reading");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_READ), "Buffer being mapped for reading was not created with CPU_ACCESS_READ flag");
            DEV_CHECK_ERR((MapFlags & MAP_FLAG_DISCARD) == 0, "MAP_FLAG_DISCARD is not valid when mapping buffer for reading");
            break;

        case MAP_WRITE:
            DEV_CHECK_ERR(BuffDesc.Usage == USAGE_DYNAMIC || BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED,
                          "Only buffers with usage USAGE_STAGING, USAGE_DYNAMIC or USAGE_UNIFIED can be mapped for writing");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Buffer being mapped for writing was not created with CPU_ACCESS_WRITE flag");
            break;

        case MAP_READ_WRITE:
            DEV_CHECK_ERR(BuffDesc.Usage == USAGE_STAGING || BuffDesc.Usage == USAGE_UNIFIED,
                          "Only buffers with usage USAGE_STAGING or USAGE_UNIFIED can be mapped for reading and writing");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Buffer being mapped for reading & writing was not created with CPU_ACCESS_WRITE flag");
            DEV_CHECK_ERR((BuffDesc.CPUAccessFlags & CPU_ACCESS_READ), "Buffer being mapped for reading & writing was not created with CPU_ACCESS_READ flag");
            DEV_CHECK_ERR((MapFlags & MAP_FLAG_DISCARD) == 0, "MAP_FLAG_DISCARD is not valid when mapping buffer for reading and writing");
            break;

        default: UNEXPECTED("Unknown map type");
    }

    if (BuffDesc.Usage == USAGE_DYNAMIC)
    {
        DEV_CHECK_ERR((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0 && MapType == MAP_WRITE, "Dynamic buffers can only be mapped for writing with MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE flag");
        DEV_CHECK_ERR((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE), "When mapping dynamic buffer, only one of MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE flags must be specified");
    }

    if ((MapFlags & MAP_FLAG_DISCARD) != 0)
    {
        DEV_CHECK_ERR(BuffDesc.Usage == USAGE_DYNAMIC || BuffDesc.Usage == USAGE_STAGING, "Only dynamic and staging buffers can be mapped with discard flag");
        DEV_CHECK_ERR(MapType == MAP_WRITE, "MAP_FLAG_DISCARD is only valid when mapping buffer for writing");
    }
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    VERIFY(pBuffer, "pBuffer must not be null");
#ifdef DILIGENT_DEBUG
    {
        auto MappedBufferIt = m_DbgMappedBuffers.find(pBuffer);
        VERIFY(MappedBufferIt != m_DbgMappedBuffers.end(), "Buffer '", pBuffer->GetDesc().Name, "' has not been mapped.");
        VERIFY(MappedBufferIt->second.MapType == MapType, "MapType (", MapType, ") does not match the map type that was used to map the buffer ", MappedBufferIt->second.MapType);
        m_DbgMappedBuffers.erase(MappedBufferIt);
    }
#endif
}


template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    UpdateTexture(ITexture* pTexture, Uint32 MipLevel, Uint32 Slice, const Box& DstBox, const TextureSubResData& SubresData, RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode, RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    VERIFY(pTexture != nullptr, "pTexture must not be null");
    VERIFY(m_pActiveRenderPass == nullptr, "UpdateTexture command must be used outside of render pass.");
    ValidateUpdateTextureParams(pTexture->GetDesc(), MipLevel, Slice, DstBox, SubresData);
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    VERIFY(CopyAttribs.pSrcTexture, "Src texture must not be null");
    VERIFY(CopyAttribs.pDstTexture, "Dst texture must not be null");
    VERIFY(m_pActiveRenderPass == nullptr, "CopyTexture command must be used outside of render pass.");
    ValidateCopyTextureParams(CopyAttribs);
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    MapTextureSubresource(ITexture*                 pTexture,
                          Uint32                    MipLevel,
                          Uint32                    ArraySlice,
                          MAP_TYPE                  MapType,
                          MAP_FLAGS                 MapFlags,
                          const Box*                pMapRegion,
                          MappedTextureSubresource& MappedData)
{
    VERIFY(pTexture, "pTexture must not be null");
    ValidateMapTextureParams(pTexture->GetDesc(), MipLevel, ArraySlice, MapType, MapFlags, pMapRegion);
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice)
{
    VERIFY(pTexture, "pTexture must not be null");
    DEV_CHECK_ERR(MipLevel < pTexture->GetDesc().MipLevels, "Mip level is out of range");
    DEV_CHECK_ERR(ArraySlice < pTexture->GetDesc().ArraySize, "Array slice is out of range");
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    GenerateMips(ITextureView* pTexView)
{
    VERIFY(pTexView != nullptr, "pTexView must not be null");
    VERIFY(m_pActiveRenderPass == nullptr, "GenerateMips command must be used outside of render pass.");
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& ViewDesc = pTexView->GetDesc();
        DEV_CHECK_ERR(ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE, "Shader resource view '", ViewDesc.Name,
                      "' can't be used to generate mipmaps because its type is ", GetTexViewTypeLiteralName(ViewDesc.ViewType), ". Required view type: TEXTURE_VIEW_SHADER_RESOURCE.");
        DEV_CHECK_ERR((ViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0, "Shader resource view '", ViewDesc.Name,
                      "' was not created with TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION flag and can't be used to generate mipmaps.");
    }
#endif
}


template <typename BaseInterface, typename ImplementationTraits>
void DeviceContextBase<BaseInterface, ImplementationTraits>::
    ResolveTextureSubresource(ITexture*                               pSrcTexture,
                              ITexture*                               pDstTexture,
                              const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
#ifdef DILIGENT_DEVELOPMENT
    VERIFY_EXPR(pSrcTexture != nullptr && pDstTexture != nullptr);
    const auto& SrcTexDesc = pSrcTexture->GetDesc();
    const auto& DstTexDesc = pDstTexture->GetDesc();
    DEV_CHECK_ERR(SrcTexDesc.SampleCount > 1,
                  "Source texture '", SrcTexDesc.Name, "' of a resolve operation is not multi-sampled");
    DEV_CHECK_ERR(DstTexDesc.SampleCount == 1,
                  "Destination texture '", DstTexDesc.Name, "' of a resolve operation is multi-sampled");
    auto SrcMipLevelProps = GetMipLevelProperties(SrcTexDesc, ResolveAttribs.SrcMipLevel);
    auto DstMipLevelProps = GetMipLevelProperties(DstTexDesc, ResolveAttribs.DstMipLevel);
    DEV_CHECK_ERR(SrcMipLevelProps.LogicalWidth == DstMipLevelProps.LogicalWidth && SrcMipLevelProps.LogicalHeight == DstMipLevelProps.LogicalHeight,
                  "The size (", SrcMipLevelProps.LogicalWidth, "x", SrcMipLevelProps.LogicalHeight,
                  ") of the source subresource of a resolve operation (texture '",
                  SrcTexDesc.Name, "', mip ", ResolveAttribs.SrcMipLevel, ", slice ", ResolveAttribs.SrcSlice,
                  ") does not match the size (", DstMipLevelProps.LogicalWidth, "x", DstMipLevelProps.LogicalHeight,
                  ") of the destination subresource (texture '", DstTexDesc.Name, "', mip ", ResolveAttribs.DstMipLevel, ", slice ",
                  ResolveAttribs.DstSlice, ")");

    const auto& SrcFmtAttribs     = GetTextureFormatAttribs(SrcTexDesc.Format);
    const auto& DstFmtAttribs     = GetTextureFormatAttribs(DstTexDesc.Format);
    const auto& ResolveFmtAttribs = GetTextureFormatAttribs(ResolveAttribs.Format);
    if (!SrcFmtAttribs.IsTypeless && !DstFmtAttribs.IsTypeless)
    {
        DEV_CHECK_ERR(SrcTexDesc.Format == DstTexDesc.Format,
                      "Source (", SrcFmtAttribs.Name, ") and destination (", DstFmtAttribs.Name,
                      ") texture formats of a resolve operation must match exaclty or be compatible typeless formats");
        DEV_CHECK_ERR(ResolveAttribs.Format == TEX_FORMAT_UNKNOWN || SrcTexDesc.Format == ResolveAttribs.Format, "Invalid format of a resolve operation");
    }
    if (SrcFmtAttribs.IsTypeless && DstFmtAttribs.IsTypeless)
    {
        DEV_CHECK_ERR(ResolveAttribs.Format != TEX_FORMAT_UNKNOWN,
                      "Format of a resolve operation must not be unknown when both src and dst texture formats are typeless");
    }
    if (SrcFmtAttribs.IsTypeless || DstFmtAttribs.IsTypeless)
    {
        DEV_CHECK_ERR(!ResolveFmtAttribs.IsTypeless,
                      "Format of a resolve operation must not be typeless when one of the texture formats is typeless");
    }
    VERIFY(m_pActiveRenderPass == nullptr, "ResolveTextureSubresource command must be used outside of render pass.");
#endif
}

#ifdef DILIGENT_DEVELOPMENT
template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDrawArguments(const DrawAttribs& Attribs) const
{
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) == 0)
        return true;

    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("Draw command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_GRAPHICS)
    {
        LOG_ERROR_MESSAGE("Draw command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");
        return false;
    }

    if (Attribs.NumVertices == 0)
    {
        LOG_WARNING_MESSAGE("Draw command arguments are invalid: number of vertices to draw is zero.");
    }

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDrawIndexedArguments(const DrawIndexedAttribs& Attribs) const
{
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) == 0)
        return true;

    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("DrawIndexed command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_GRAPHICS)
    {
        LOG_ERROR_MESSAGE("DrawIndexed command arguments are invalid: pipeline state '",
                          m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");
        return false;
    }

    if (Attribs.IndexType != VT_UINT16 && Attribs.IndexType != VT_UINT32)
    {
        LOG_ERROR_MESSAGE("DrawIndexed command arguments are invalid: IndexType (",
                          GetValueTypeString(Attribs.IndexType), ") must be VT_UINT16 or VT_UINT32.");
        return false;
    }

    if (!m_pIndexBuffer)
    {
        LOG_ERROR_MESSAGE("DrawIndexed command arguments are invalid: no index buffer is bound.");
        return false;
    }

    if (Attribs.NumIndices == 0)
    {
        LOG_WARNING_MESSAGE("DrawIndexed command arguments are invalid: number of indices to draw is zero.");
    }

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDrawMeshArguments(const DrawMeshAttribs& Attribs) const
{
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) == 0)
        return true;

    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("DrawMesh command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_MESH)
    {
        LOG_ERROR_MESSAGE("DrawMesh command arguments are invalid: pipeline state '",
                          m_pPipelineState->GetDesc().Name, "' is not a mesh pipeline.");
        return false;
    }

    if (Attribs.ThreadGroupCount == 0)
    {
        LOG_WARNING_MESSAGE("DrawMesh command arguments are invalid: number of groups to dispatch is zero.");
    }

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDrawIndirectArguments(const DrawIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer) const
{
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) == 0)
        return true;

    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("DrawIndirect command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_GRAPHICS)
    {

        LOG_ERROR_MESSAGE("DrawIndirect command arguments are invalid: pipeline state '",
                          m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");
        return false;
    }

    if (pAttribsBuffer != nullptr)
    {
        if ((pAttribsBuffer->GetDesc().BindFlags & BIND_INDIRECT_DRAW_ARGS) == 0)
        {
            LOG_ERROR_MESSAGE("DrawIndirect command arguments are invalid: indirect draw arguments buffer '",
                              pAttribsBuffer->GetDesc().Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
            return false;
        }
    }
    else
    {
        LOG_ERROR_MESSAGE("DrawIndirect command arguments are invalid: indirect draw arguments buffer is null.");
        return false;
    }

    if (m_pActiveRenderPass != nullptr && Attribs.IndirectAttribsBufferStateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        LOG_ERROR_MESSAGE("Resource state transitons are not allowed inside a render pass and may result in an undefined behavior. "
                          "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");
        return false;
    }

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDrawIndexedIndirectArguments(const DrawIndexedIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer) const
{
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) == 0)
        return true;

    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("DrawIndexedIndirect command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_GRAPHICS)
    {
        LOG_ERROR_MESSAGE("DrawIndexedIndirect command arguments are invalid: pipeline state '",
                          m_pPipelineState->GetDesc().Name, "' is not a graphics pipeline.");
        return false;
    }

    if (Attribs.IndexType != VT_UINT16 && Attribs.IndexType != VT_UINT32)
    {
        LOG_ERROR_MESSAGE("DrawIndexedIndirect command arguments are invalid: IndexType (",
                          GetValueTypeString(Attribs.IndexType), ") must be VT_UINT16 or VT_UINT32.");
        return false;
    }

    if (!m_pIndexBuffer)
    {
        LOG_ERROR_MESSAGE("DrawIndexedIndirect command arguments are invalid: no index buffer is bound.");
        return false;
    }

    if (pAttribsBuffer != nullptr)
    {
        if ((pAttribsBuffer->GetDesc().BindFlags & BIND_INDIRECT_DRAW_ARGS) == 0)
        {
            LOG_ERROR_MESSAGE("DrawIndexedIndirect command arguments are invalid: indirect draw arguments buffer '",
                              pAttribsBuffer->GetDesc().Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
            return false;
        }
    }
    else
    {
        LOG_ERROR_MESSAGE("DrawIndexedIndirect command arguments are invalid: indirect draw arguments buffer is null.");
        return false;
    }

    if (m_pActiveRenderPass != nullptr && Attribs.IndirectAttribsBufferStateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        LOG_ERROR_MESSAGE("Resource state transitons are not allowed inside a render pass and may result in an undefined behavior. "
                          "Do not use RESOURCE_STATE_TRANSITION_MODE_TRANSITION or end the render pass first.");
        return false;
    }

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDrawMeshIndirectArguments(const DrawMeshIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer) const
{
    if ((Attribs.Flags & DRAW_FLAG_VERIFY_DRAW_ATTRIBS) == 0)
        return true;

    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("DrawMeshIndirect command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_MESH)
    {
        LOG_ERROR_MESSAGE("DrawMeshIndirect command arguments are invalid: pipeline state '",
                          m_pPipelineState->GetDesc().Name, "' is not a mesh pipeline.");
        return false;
    }

    if (pAttribsBuffer != nullptr)
    {
        if ((pAttribsBuffer->GetDesc().BindFlags & BIND_INDIRECT_DRAW_ARGS) == 0)
        {
            LOG_ERROR_MESSAGE("DrawMeshIndirect command arguments are invalid: indirect draw arguments buffer '",
                              pAttribsBuffer->GetDesc().Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
            return false;
        }
    }
    else
    {
        LOG_ERROR_MESSAGE("DrawMeshIndirect command arguments are invalid: indirect draw arguments buffer is null.");
        return false;
    }

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline void DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyRenderTargets() const
{
    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("No pipeline state is bound");
        return;
    }

    const auto& PSODesc = m_pPipelineState->GetDesc();
    if (!PSODesc.IsAnyGraphicsPipeline())
    {
        LOG_ERROR_MESSAGE("Pipeline state '", PSODesc.Name, "' is not a graphics pipeline");
        return;
    }

    TEXTURE_FORMAT BoundRTVFormats[8] = {TEX_FORMAT_UNKNOWN};
    TEXTURE_FORMAT BoundDSVFormat     = TEX_FORMAT_UNKNOWN;

    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (auto* pRT = m_pBoundRenderTargets[rt].RawPtr())
            BoundRTVFormats[rt] = pRT->GetDesc().Format;
        else
            BoundRTVFormats[rt] = TEX_FORMAT_UNKNOWN;
    }

    BoundDSVFormat = m_pBoundDepthStencil ? m_pBoundDepthStencil->GetDesc().Format : TEX_FORMAT_UNKNOWN;

    const auto& GraphicsPipeline = m_pPipelineState->GetGraphicsPipelineDesc();
    if (GraphicsPipeline.NumRenderTargets != m_NumBoundRenderTargets)
    {
        LOG_WARNING_MESSAGE("The number of currently bound render targets (", m_NumBoundRenderTargets,
                            ") does not match the number of outputs specified by the PSO '", PSODesc.Name,
                            "' (", Uint32{GraphicsPipeline.NumRenderTargets}, ").");
    }

    if (BoundDSVFormat != GraphicsPipeline.DSVFormat)
    {
        LOG_WARNING_MESSAGE("Currently bound depth-stencil buffer format (", GetTextureFormatAttribs(BoundDSVFormat).Name,
                            ") does not match the DSV format specified by the PSO '", PSODesc.Name,
                            "' (", GetTextureFormatAttribs(GraphicsPipeline.DSVFormat).Name, ").");
    }

    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        auto BoundFmt = BoundRTVFormats[rt];
        auto PSOFmt   = GraphicsPipeline.RTVFormats[rt];
        if (BoundFmt != PSOFmt)
        {
            LOG_WARNING_MESSAGE("Render target bound to slot ", rt, " (", GetTextureFormatAttribs(BoundFmt).Name,
                                ") does not match the RTV format specified by the PSO '", PSODesc.Name,
                                "' (", GetTextureFormatAttribs(PSOFmt).Name, ").");
        }
    }
}



template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDispatchArguments(const DispatchComputeAttribs& Attribs) const
{
    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("DispatchCompute command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_COMPUTE)
    {
        LOG_ERROR_MESSAGE("DispatchCompute command arguments are invalid: pipeline state '", m_pPipelineState->GetDesc().Name,
                          "' is not a compute pipeline.");
        return false;
    }

    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("DispatchCompute command must be performed outside of render pass");
        return false;
    }

    if (Attribs.ThreadGroupCountX == 0)
        LOG_WARNING_MESSAGE("DispatchCompute command arguments are invalid: ThreadGroupCountX is zero.");

    if (Attribs.ThreadGroupCountY == 0)
        LOG_WARNING_MESSAGE("DispatchCompute command arguments are invalid: ThreadGroupCountY is zero.");

    if (Attribs.ThreadGroupCountZ == 0)
        LOG_WARNING_MESSAGE("DispatchCompute command arguments are invalid: ThreadGroupCountZ is zero.");

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
inline bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyDispatchIndirectArguments(const DispatchComputeIndirectAttribs& Attribs, const IBuffer* pAttribsBuffer) const
{
    if (!m_pPipelineState)
    {
        LOG_ERROR_MESSAGE("DispatchComputeIndirect command arguments are invalid: no pipeline state is bound.");
        return false;
    }

    if (m_pPipelineState->GetDesc().PipelineType != PIPELINE_TYPE_COMPUTE)
    {
        LOG_ERROR_MESSAGE("DispatchComputeIndirect command arguments are invalid: pipeline state '",
                          m_pPipelineState->GetDesc().Name, "' is not a compute pipeline.");
        return false;
    }

    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("DispatchComputeIndirect command must be performed outside of render pass");
        return false;
    }

    if (pAttribsBuffer != nullptr)
    {
        if ((pAttribsBuffer->GetDesc().BindFlags & BIND_INDIRECT_DRAW_ARGS) == 0)
        {
            LOG_ERROR_MESSAGE("DispatchComputeIndirect command arguments are invalid: indirect dispatch arguments buffer '",
                              pAttribsBuffer->GetDesc().Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
            return false;
        }
    }
    else
    {
        LOG_ERROR_MESSAGE("DispatchComputeIndirect command arguments are invalid: indirect dispatch arguments buffer is null.");
        return false;
    }

    return true;
}


template <typename BaseInterface, typename ImplementationTraits>
void DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyStateTransitionDesc(const StateTransitionDesc& Barrier) const
{
    DEV_CHECK_ERR((Barrier.pTexture != nullptr) ^ (Barrier.pBuffer != nullptr), "Exactly one of pTexture or pBuffer members of StateTransitionDesc must not be null");
    DEV_CHECK_ERR(Barrier.NewState != RESOURCE_STATE_UNKNOWN, "New resource state can't be unknown");
    RESOURCE_STATE OldState = RESOURCE_STATE_UNKNOWN;
    if (Barrier.pTexture)
    {
        const auto& TexDesc = Barrier.pTexture->GetDesc();

        DEV_CHECK_ERR(VerifyResourceStates(Barrier.NewState, true), "Invlaid new state specified for texture '", TexDesc.Name, "'");
        OldState = Barrier.OldState != RESOURCE_STATE_UNKNOWN ? Barrier.OldState : Barrier.pTexture->GetState();
        DEV_CHECK_ERR(OldState != RESOURCE_STATE_UNKNOWN,
                      "The state of texture '", TexDesc.Name,
                      "' is unknown to the engine and is not explicitly specified in the barrier");
        DEV_CHECK_ERR(VerifyResourceStates(OldState, true), "Invlaid old state specified for texture '", TexDesc.Name, "'");

        DEV_CHECK_ERR(Barrier.FirstMipLevel < TexDesc.MipLevels, "First mip level (", Barrier.FirstMipLevel,
                      ") specified by the barrier is out of range. Texture '",
                      TexDesc.Name, "' has only ", TexDesc.MipLevels, " mip level(s)");
        DEV_CHECK_ERR(Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || Barrier.FirstMipLevel + Barrier.MipLevelsCount <= TexDesc.MipLevels,
                      "Mip level range ", Barrier.FirstMipLevel, "..", Barrier.FirstMipLevel + Barrier.MipLevelsCount - 1,
                      " specified by the barrier is out of range. Texture '",
                      TexDesc.Name, "' has only ", TexDesc.MipLevels, " mip level(s)");

        DEV_CHECK_ERR(Barrier.FirstArraySlice < TexDesc.ArraySize, "First array slice (", Barrier.FirstArraySlice,
                      ") specified by the barrier is out of range. Array size of texture '",
                      TexDesc.Name, "' is ", TexDesc.ArraySize);
        DEV_CHECK_ERR(Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || Barrier.FirstArraySlice + Barrier.ArraySliceCount <= TexDesc.ArraySize,
                      "Array slice range ", Barrier.FirstArraySlice, "..", Barrier.FirstArraySlice + Barrier.ArraySliceCount - 1,
                      " specified by the barrier is out of range. Array size of texture '",
                      TexDesc.Name, "' is ", TexDesc.ArraySize);

        auto DevType = m_pDevice->GetDeviceCaps().DevType;
        if (DevType != RENDER_DEVICE_TYPE_D3D12 && DevType != RENDER_DEVICE_TYPE_VULKAN)
        {
            DEV_CHECK_ERR(Barrier.FirstMipLevel == 0 && (Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || Barrier.MipLevelsCount == TexDesc.MipLevels),
                          "Failed to transition texture '", TexDesc.Name, "': only whole resources can be transitioned on this device");
            DEV_CHECK_ERR(Barrier.FirstArraySlice == 0 && (Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || Barrier.ArraySliceCount == TexDesc.ArraySize),
                          "Failed to transition texture '", TexDesc.Name, "': only whole resources can be transitioned on this device");
        }
    }
    else
    {
        const auto& BuffDesc = Barrier.pBuffer->GetDesc();
        DEV_CHECK_ERR(VerifyResourceStates(Barrier.NewState, false), "Invlaid new state specified for buffer '", BuffDesc.Name, "'");
        OldState = Barrier.OldState != RESOURCE_STATE_UNKNOWN ? Barrier.OldState : Barrier.pBuffer->GetState();
        DEV_CHECK_ERR(OldState != RESOURCE_STATE_UNKNOWN, "The state of buffer '", BuffDesc.Name, "' is unknown to the engine and is not explicitly specified in the barrier");
        DEV_CHECK_ERR(VerifyResourceStates(OldState, false), "Invlaid old state specified for buffer '", BuffDesc.Name, "'");
    }

    if (OldState == RESOURCE_STATE_UNORDERED_ACCESS && Barrier.NewState == RESOURCE_STATE_UNORDERED_ACCESS)
    {
        DEV_CHECK_ERR(Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE, "For UAV barriers, transition type must be STATE_TRANSITION_TYPE_IMMEDIATE");
    }

    if (Barrier.TransitionType == STATE_TRANSITION_TYPE_BEGIN)
    {
        DEV_CHECK_ERR(!Barrier.UpdateResourceState, "Resource state can't be updated in begin-split barrier");
    }
}

template <typename BaseInterface, typename ImplementationTraits>
bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyTextureState(const TextureImplType& Texture, RESOURCE_STATE RequiredState, const char* OperationName) const
{
    if (Texture.IsInKnownState() && !Texture.CheckState(RequiredState))
    {
        LOG_ERROR_MESSAGE(OperationName, " requires texture '", Texture.GetDesc().Name, "' to be transitioned to ", GetResourceStateString(RequiredState),
                          " state. Actual texture state: ", GetResourceStateString(Texture.GetState()),
                          ". Use appropriate state transiton flags or explicitly transition the texture using IDeviceContext::TransitionResourceStates() method.");
        return false;
    }

    return true;
}

template <typename BaseInterface, typename ImplementationTraits>
bool DeviceContextBase<BaseInterface, ImplementationTraits>::
    DvpVerifyBufferState(const BufferImplType& Buffer, RESOURCE_STATE RequiredState, const char* OperationName) const
{
    if (Buffer.IsInKnownState() && !Buffer.CheckState(RequiredState))
    {
        LOG_ERROR_MESSAGE(OperationName, " requires buffer '", Buffer.GetDesc().Name, "' to be transitioned to ", GetResourceStateString(RequiredState),
                          " state. Actual buffer state: ", GetResourceStateString(Buffer.GetState()),
                          ". Use appropriate state transiton flags or explicitly transition the buffer using IDeviceContext::TransitionResourceStates() method.");
        return false;
    }

    return true;
}

#endif // DILIGENT_DEVELOPMENT

} // namespace Diligent
