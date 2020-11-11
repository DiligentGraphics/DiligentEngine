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
/// Declaration of Diligent::DeviceContextD3D12Impl class

#include <unordered_map>
#include <vector>

#include "DeviceContextD3D12.h"
#include "DeviceContextNextGenBase.hpp"
#include "BufferD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "QueryD3D12Impl.hpp"
#include "FramebufferD3D12Impl.hpp"
#include "RenderPassD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "D3D12DynamicHeap.hpp"

namespace Diligent
{

struct DeviceContextD3D12ImplTraits
{
    using BufferType        = BufferD3D12Impl;
    using TextureType       = TextureD3D12Impl;
    using PipelineStateType = PipelineStateD3D12Impl;
    using DeviceType        = RenderDeviceD3D12Impl;
    using ICommandQueueType = ICommandQueueD3D12;
    using QueryType         = QueryD3D12Impl;
    using FramebufferType   = FramebufferD3D12Impl;
    using RenderPassType    = RenderPassD3D12Impl;
};

/// Device context implementation in Direct3D12 backend.
class DeviceContextD3D12Impl final : public DeviceContextNextGenBase<IDeviceContextD3D12, DeviceContextD3D12ImplTraits>
{
public:
    using TDeviceContextBase = DeviceContextNextGenBase<IDeviceContextD3D12, DeviceContextD3D12ImplTraits>;

    DeviceContextD3D12Impl(IReferenceCounters*          pRefCounters,
                           RenderDeviceD3D12Impl*       pDevice,
                           bool                         bIsDeferred,
                           const EngineD3D12CreateInfo& EngineCI,
                           Uint32                       ContextId,
                           Uint32                       CommandQueueId);
    ~DeviceContextD3D12Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IDeviceContext::SetPipelineState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetPipelineState(IPipelineState* pPipelineState) override final;

    /// Implementation of IDeviceContext::TransitionShaderResources() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionShaderResources(IPipelineState*         pPipelineState,
                                                              IShaderResourceBinding* pShaderResourceBinding) override final;

    /// Implementation of IDeviceContext::CommitShaderResources() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetStencilRef() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetStencilRef(Uint32 StencilRef) override final;

    /// Implementation of IDeviceContext::SetBlendFactors() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetBlendFactors(const float* pBlendFactors = nullptr) override final;

    /// Implementation of IDeviceContext::SetVertexBuffers() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                     Uint32                         NumBuffersSet,
                                                     IBuffer**                      ppBuffers,
                                                     Uint32*                        pOffsets,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                     SET_VERTEX_BUFFERS_FLAGS       Flags) override final;

    /// Implementation of IDeviceContext::InvalidateState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE InvalidateState() override final;

    /// Implementation of IDeviceContext::SetIndexBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                                   Uint32                         ByteOffset,
                                                   RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetViewports() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetViewports(Uint32          NumViewports,
                                                 const Viewport* pViewports,
                                                 Uint32          RTWidth,
                                                 Uint32          RTHeight) override final;

    /// Implementation of IDeviceContext::SetScissorRects() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SetScissorRects(Uint32      NumRects,
                                                    const Rect* pRects,
                                                    Uint32      RTWidth,
                                                    Uint32      RTHeight) override final;

    /// Implementation of IDeviceContext::SetRenderTargets() in Direct3D12 backend.
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
    /// Implementation of IDeviceContext::Draw() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Draw               (const DrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexed() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexed        (const DrawIndexedAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndirect       (const DrawIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    /// Implementation of IDeviceContext::DrawIndexedIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    /// Implementation of IDeviceContext::DrawMesh() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawMesh           (const DrawMeshAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMeshIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DrawMeshIndirect   (const DrawMeshIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    

    /// Implementation of IDeviceContext::DispatchCompute() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DispatchCompute        (const DispatchComputeAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DispatchComputeIndirect() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    // clang-format on

    /// Implementation of IDeviceContext::ClearDepthStencil() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ClearDepthStencil(ITextureView*                  pView,
                                                      CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                                      float                          fDepth,
                                                      Uint8                          Stencil,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::ClearRenderTarget() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ClearRenderTarget(ITextureView*                  pView,
                                                      const float*                   RGBA,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::UpdateBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint32                         Offset,
                                                 Uint32                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint32                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint32                         DstOffset,
                                               Uint32                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override final;

    /// Implementation of IDeviceContext::MapBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer*  pBuffer,
                                              MAP_TYPE  MapType,
                                              MAP_FLAGS MapFlags,
                                              PVoid&    pMappedData) override final;

    /// Implementation of IDeviceContext::UnmapBuffer() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override final;

    /// Implementation of IDeviceContext::UpdateTexture() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyTexture() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override final;

    /// Implementation of IDeviceContext::MapTextureSubresource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override final;

    /// Implementation of IDeviceContext::UnmapTextureSubresource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice) override final;

    /// Implementation of IDeviceContext::FinishFrame() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE FinishFrame() override final;

    /// Implementation of IDeviceContext::TransitionResourceStates() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionResourceStates(Uint32 BarrierCount, StateTransitionDesc* pResourceBarriers) override final;

    /// Implementation of IDeviceContext::ResolveTextureSubresource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override final;

    /// Implementation of IDeviceContext::FinishCommandList() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE FinishCommandList(class ICommandList** ppCommandList) override final;

    /// Implementation of IDeviceContext::ExecuteCommandList() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE ExecuteCommandList(class ICommandList* pCommandList) override final;

    /// Implementation of IDeviceContext::SignalFence() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE SignalFence(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::WaitForFence() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE WaitForFence(IFence* pFence, Uint64 Value, bool FlushContext) override final;

    /// Implementation of IDeviceContext::WaitForIdle() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of IDeviceContext::BeginQuery() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE BeginQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::EndQuery() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE EndQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::Flush() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Flush() override final;

    /// Implementation of IDeviceContext::TransitionTextureState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionTextureState(ITexture* pTexture, D3D12_RESOURCE_STATES State) override final;

    /// Implementation of IDeviceContext::TransitionBufferState() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE TransitionBufferState(IBuffer* pBuffer, D3D12_RESOURCE_STATES State) override final;

    /// Implementation of IDeviceContextD3D12::ID3D12GraphicsCommandList() in Direct3D12 backend.
    virtual ID3D12GraphicsCommandList* DILIGENT_CALL_TYPE GetD3D12CommandList() override final;

    void UpdateBufferRegion(class BufferD3D12Impl*         pBuffD3D12,
                            D3D12DynamicAllocation&        Allocation,
                            Uint64                         DstOffset,
                            Uint64                         NumBytes,
                            RESOURCE_STATE_TRANSITION_MODE StateTransitionMode);

    void CopyTextureRegion(class TextureD3D12Impl*        pSrcTexture,
                           Uint32                         SrcSubResIndex,
                           const D3D12_BOX*               pD3D12SrcBox,
                           RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode,
                           class TextureD3D12Impl*        pDstTexture,
                           Uint32                         DstSubResIndex,
                           Uint32                         DstX,
                           Uint32                         DstY,
                           Uint32                         DstZ,
                           RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode);

    void CopyTextureRegion(IBuffer*                       pSrcBuffer,
                           Uint32                         SrcOffset,
                           Uint32                         SrcStride,
                           Uint32                         SrcDepthStride,
                           class TextureD3D12Impl&        TextureD3D12,
                           Uint32                         DstSubResIndex,
                           const Box&                     DstBox,
                           RESOURCE_STATE_TRANSITION_MODE BufferTransitionMode,
                           RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode);
    void CopyTextureRegion(ID3D12Resource*                pd3d12Buffer,
                           Uint32                         SrcOffset,
                           Uint32                         SrcStride,
                           Uint32                         SrcDepthStride,
                           Uint32                         BufferSize,
                           class TextureD3D12Impl&        TextureD3D12,
                           Uint32                         DstSubResIndex,
                           const Box&                     DstBox,
                           RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode);

    void UpdateTextureRegion(const void*                    pSrcData,
                             Uint32                         SrcStride,
                             Uint32                         SrcDepthStride,
                             class TextureD3D12Impl&        TextureD3D12,
                             Uint32                         DstSubResIndex,
                             const Box&                     DstBox,
                             RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode);

    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTexView) override final;

    D3D12DynamicAllocation AllocateDynamicSpace(size_t NumBytes, size_t Alignment);

    Uint32 GetContextId() const { return m_ContextId; }

    size_t GetNumCommandsInCtx() const { return m_State.NumCommands; }

    Int64 GetCurrentFrameNumber() const { return m_ContextFrameNumber; }

private:
    void CommitD3D12IndexBuffer(GraphicsContext& GraphCtx, VALUE_TYPE IndexType);
    void CommitD3D12VertexBuffers(class GraphicsContext& GraphCtx);
    void CommitRenderTargets(RESOURCE_STATE_TRANSITION_MODE StateTransitionMode);
    void CommitViewports();
    void CommitScissorRects(class GraphicsContext& GraphCtx, bool ScissorEnable);
    void TransitionSubpassAttachments(Uint32 NextSubpass);
    void CommitSubpassRenderTargets();
    void Flush(bool RequestNewCmdCtx);

    __forceinline void RequestCommandContext(RenderDeviceD3D12Impl* pDeviceD3D12Impl);

    __forceinline void TransitionOrVerifyBufferState(CommandContext&                CmdCtx,
                                                     BufferD3D12Impl&               Buffer,
                                                     RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                     RESOURCE_STATE                 RequiredState,
                                                     const char*                    OperationName);
    __forceinline void TransitionOrVerifyTextureState(CommandContext&                CmdCtx,
                                                      TextureD3D12Impl&              Texture,
                                                      RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                      RESOURCE_STATE                 RequiredState,
                                                      const char*                    OperationName);

    __forceinline void PrepareForDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags);

    __forceinline void PrepareForIndexedDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags, VALUE_TYPE IndexType);

    __forceinline void PrepareForDispatchCompute(ComputeContext& GraphCtx);

    __forceinline void PrepareDrawIndirectBuffer(GraphicsContext&               GraphCtx,
                                                 IBuffer*                       pAttribsBuffer,
                                                 RESOURCE_STATE_TRANSITION_MODE BufferStateTransitionMode,
                                                 ID3D12Resource*&               pd3d12ArgsBuff,
                                                 Uint64&                        BuffDataStartByteOffset);

    struct TextureUploadSpace
    {
        D3D12DynamicAllocation Allocation;
        Uint32                 AlignedOffset = 0;
        Uint32                 Stride        = 0;
        Uint32                 DepthStride   = 0;
        Uint32                 RowSize       = 0;
        Uint32                 RowCount      = 0;
        Box                    Region;
    };
    TextureUploadSpace AllocateTextureUploadSpace(TEXTURE_FORMAT TexFmt,
                                                  const Box&     Region);


    friend class SwapChainD3D12Impl;
    inline class CommandContext& GetCmdContext()
    {
        // Make sure that the number of commands in the context is at least one,
        // so that the context cannot be disposed by Flush()
        m_State.NumCommands = m_State.NumCommands != 0 ? m_State.NumCommands : 1;
        return *m_CurrCmdCtx;
    }
    std::unique_ptr<CommandContext, STDDeleterRawMem<CommandContext>> m_CurrCmdCtx;

    struct State
    {
        size_t NumCommands = 0;

        CComPtr<ID3D12Resource> CommittedD3D12IndexBuffer;
        VALUE_TYPE              CommittedIBFormat                  = VT_UNDEFINED;
        Uint64                  CommittedD3D12IndexDataStartOffset = 0;

        // Indicates if currently committed D3D12 vertex buffers are up to date
        bool bCommittedD3D12VBsUpToDate = false;

        // Indicates if currently committed D3D11 index buffer is up to date
        bool bCommittedD3D12IBUpToDate = false;

        // Indicates if root views have been committed since the time SRB
        // has been committed.
        bool bRootViewsCommitted = false;

        class ShaderResourceCacheD3D12* pCommittedResourceCache = nullptr;
    } m_State;

    CComPtr<ID3D12CommandSignature> m_pDrawIndirectSignature;
    CComPtr<ID3D12CommandSignature> m_pDrawIndexedIndirectSignature;
    CComPtr<ID3D12CommandSignature> m_pDispatchIndirectSignature;
    CComPtr<ID3D12CommandSignature> m_pDrawMeshIndirectSignature;

    D3D12DynamicHeap m_DynamicHeap;

    // Every context must use its own allocator that maintains individual list of retired descriptor heaps to
    // avoid interference with other command contexts
    // The allocations in heaps are discarded at the end of the frame.
    DynamicSuballocationsManager m_DynamicGPUDescriptorAllocator[2];

    FixedBlockMemoryAllocator m_CmdListAllocator;

    std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>> m_PendingFences;

    struct MappedTextureKey
    {
        TextureD3D12Impl* const Texture;
        UINT const              Subresource;

        bool operator==(const MappedTextureKey& rhs) const
        {
            return Texture == rhs.Texture && Subresource == rhs.Subresource;
        }
        struct Hasher
        {
            size_t operator()(const MappedTextureKey& Key) const
            {
                return ComputeHash(Key.Texture, Key.Subresource);
            }
        };
    };
    std::unordered_map<MappedTextureKey, TextureUploadSpace, MappedTextureKey::Hasher> m_MappedTextures;

    Int32 m_ActiveQueriesCounter = 0;

    std::vector<OptimizedClearValue> m_AttachmentClearValues;

    std::vector<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS> m_AttachmentResolveInfo;
};

} // namespace Diligent
