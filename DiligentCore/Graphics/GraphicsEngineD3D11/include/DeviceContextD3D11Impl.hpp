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
/// Declaration of Diligent::DeviceContextD3D11Impl class

#include <vector>

#include "DeviceContextD3D11.h"
#include "DeviceContextBase.hpp"
#include "BufferD3D11Impl.hpp"
#include "TextureBaseD3D11.hpp"
#include "PipelineStateD3D11Impl.hpp"
#include "QueryD3D11Impl.hpp"
#include "FramebufferD3D11Impl.hpp"
#include "RenderPassD3D11Impl.hpp"
#include "DisjointQueryPool.hpp"

#ifdef DILIGENT_DEBUG
#    define VERIFY_CONTEXT_BINDINGS
#endif

namespace Diligent
{

struct DeviceContextD3D11ImplTraits
{
    using BufferType        = BufferD3D11Impl;
    using TextureType       = TextureBaseD3D11;
    using PipelineStateType = PipelineStateD3D11Impl;
    using DeviceType        = RenderDeviceD3D11Impl;
    using QueryType         = QueryD3D11Impl;
    using FramebufferType   = FramebufferD3D11Impl;
    using RenderPassType    = RenderPassD3D11Impl;
};

/// Device context implementation in Direct3D11 backend.
class DeviceContextD3D11Impl final : public DeviceContextBase<IDeviceContextD3D11, DeviceContextD3D11ImplTraits>
{
public:
    using TDeviceContextBase = DeviceContextBase<IDeviceContextD3D11, DeviceContextD3D11ImplTraits>;

    DeviceContextD3D11Impl(IReferenceCounters*                 pRefCounters,
                           IMemoryAllocator&                   Allocator,
                           RenderDeviceD3D11Impl*              pDevice,
                           ID3D11DeviceContext*                pd3d11DeviceContext,
                           const struct EngineD3D11CreateInfo& EngineAttribs,
                           bool                                bIsDeferred);
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IDeviceContext::SetPipelineState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetPipelineState(IPipelineState* pPipelineState) override final;

    /// Implementation of IDeviceContext::TransitionShaderResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE TransitionShaderResources(IPipelineState*         pPipelineState,
                                                              IShaderResourceBinding* pShaderResourceBinding) override final;

    /// Implementation of IDeviceContext::CommitShaderResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CommitShaderResources(IShaderResourceBinding*        pShaderResourceBinding,
                                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetStencilRef() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetStencilRef(Uint32 StencilRef) override final;

    /// Implementation of IDeviceContext::SetBlendFactors() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetBlendFactors(const float* pBlendFactors = nullptr) override final;

    /// Implementation of IDeviceContext::SetVertexBuffers() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetVertexBuffers(Uint32                         StartSlot,
                                                     Uint32                         NumBuffersSet,
                                                     IBuffer**                      ppBuffers,
                                                     Uint32*                        pOffsets,
                                                     RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                                     SET_VERTEX_BUFFERS_FLAGS       Flags) override final;

    /// Implementation of IDeviceContext::InvalidateState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE InvalidateState() override final;

    /// Implementation of IDeviceContext::SetIndexBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetIndexBuffer(IBuffer*                       pIndexBuffer,
                                                   Uint32                         ByteOffset,
                                                   RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::SetViewports() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetViewports(Uint32          NumViewports,
                                                 const Viewport* pViewports,
                                                 Uint32          RTWidth,
                                                 Uint32          RTHeight) override final;

    /// Implementation of IDeviceContext::SetScissorRects() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SetScissorRects(Uint32      NumRects,
                                                    const Rect* pRects,
                                                    Uint32      RTWidth,
                                                    Uint32      RTHeight) override final;

    /// Implementation of IDeviceContext::SetRenderTargets() in Direct3D11 backend.
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

    /// Implementation of IDeviceContext::Draw() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE Draw(const DrawAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndexed() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexed(const DrawIndexedAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndirect(const DrawIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    /// Implementation of IDeviceContext::DrawIndexedIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;
    /// Implementation of IDeviceContext::DrawMesh() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawMesh(const DrawMeshAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DrawMeshIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;

    /// Implementation of IDeviceContext::DispatchCompute() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DispatchCompute(const DispatchComputeAttribs& Attribs) override final;
    /// Implementation of IDeviceContext::DispatchComputeIndirect() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, IBuffer* pAttribsBuffer) override final;

    /// Implementation of IDeviceContext::ClearDepthStencil() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ClearDepthStencil(ITextureView*                  pView,
                                                      CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                                      float                          fDepth,
                                                      Uint8                          Stencil,
                                                      RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::ClearRenderTarget() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ClearRenderTarget(ITextureView* pView, const float* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::UpdateBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UpdateBuffer(IBuffer*                       pBuffer,
                                                 Uint32                         Offset,
                                                 Uint32                         Size,
                                                 const void*                    pData,
                                                 RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CopyBuffer(IBuffer*                       pSrcBuffer,
                                               Uint32                         SrcOffset,
                                               RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                               IBuffer*                       pDstBuffer,
                                               Uint32                         DstOffset,
                                               Uint32                         Size,
                                               RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode) override final;

    /// Implementation of IDeviceContext::MapBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData) override final;

    /// Implementation of IDeviceContext::UnmapBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType) override final;

    /// Implementation of IDeviceContext::UpdateTexture() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UpdateTexture(ITexture*                      pTexture,
                                                  Uint32                         MipLevel,
                                                  Uint32                         Slice,
                                                  const Box&                     DstBox,
                                                  const TextureSubResData&       SubresData,
                                                  RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                                  RESOURCE_STATE_TRANSITION_MODE StateTransitionMode) override final;

    /// Implementation of IDeviceContext::CopyTexture() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CopyTexture(const CopyTextureAttribs& CopyAttribs) override final;

    /// Implementation of IDeviceContext::MapTextureSubresource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE MapTextureSubresource(ITexture*                 pTexture,
                                                          Uint32                    MipLevel,
                                                          Uint32                    ArraySlice,
                                                          MAP_TYPE                  MapType,
                                                          MAP_FLAGS                 MapFlags,
                                                          const Box*                pMapRegion,
                                                          MappedTextureSubresource& MappedData) override final;

    /// Implementation of IDeviceContext::UnmapTextureSubresource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice) override final;

    /// Implementation of IDeviceContext::GenerateMips() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE GenerateMips(ITextureView* pTextureView) override final;

    /// Implementation of IDeviceContext::FinishFrame() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE FinishFrame() override final;

    /// Implementation of IDeviceContext::TransitionResourceStates() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE TransitionResourceStates(Uint32 BarrierCount, StateTransitionDesc* pResourceBarriers) override final;

    /// Implementation of IDeviceContext::ResolveTextureSubresource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                              ITexture*                               pDstTexture,
                                                              const ResolveTextureSubresourceAttribs& ResolveAttribs) override final;

    /// Implementation of IDeviceContext::FinishCommandList() in Direct3D11 backend.
    void DILIGENT_CALL_TYPE FinishCommandList(class ICommandList** ppCommandList) override final;

    /// Implementation of IDeviceContext::ExecuteCommandList() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ExecuteCommandList(class ICommandList* pCommandList) override final;

    /// Implementation of IDeviceContext::SignalFence() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE SignalFence(IFence* pFence, Uint64 Value) override final;

    /// Implementation of IDeviceContext::WaitForFence() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE WaitForFence(IFence* pFence, Uint64 Value, bool FlushContext) override final;

    /// Implementation of IDeviceContext::WaitForIdle() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of IDeviceContext::BeginQuery() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BeginQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::EndQuery() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE EndQuery(IQuery* pQuery) override final;

    /// Implementation of IDeviceContext::Flush() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE Flush() override final;

    /// Implementation of IDeviceContextD3D11::GetD3D11DeviceContext().
    virtual ID3D11DeviceContext* DILIGENT_CALL_TYPE GetD3D11DeviceContext() override final { return m_pd3d11DeviceContext; }

    void CommitRenderTargets();

    /// Clears committed shader resource cache. This function
    /// is called once per frame (before present) to release all
    /// outstanding objects that are only kept alive by references
    /// in the cache. The function does not release cached vertex and
    /// index buffers, input layout, depth-stencil, rasterizer, and blend
    /// states.
    void ReleaseCommittedShaderResources();

    /// Unbinds all render targets. Used when resizing the swap chain.
    virtual void ResetRenderTargets() override final;

    /// Number of different shader types (Vertex, Pixel, Geometry, Domain, Hull, Compute)
    static constexpr int NumShaderTypes = 6;

private:
    /// Commits d3d11 index buffer to d3d11 device context.
    void CommitD3D11IndexBuffer(VALUE_TYPE IndexType);

    /// Commits d3d11 vertex buffers to d3d11 device context.
    void CommitD3D11VertexBuffers(class PipelineStateD3D11Impl* pPipelineStateD3D11);

    /// Helper template function used to facilitate resource unbinding
    template <typename TD3D11ResourceViewType,
              typename TSetD3D11View,
              size_t NumSlots>
    void UnbindResourceView(TD3D11ResourceViewType CommittedD3D11ViewsArr[][NumSlots],
                            ID3D11Resource*        CommittedD3D11ResourcesArr[][NumSlots],
                            Uint8                  NumCommittedResourcesArr[],
                            ID3D11Resource*        pd3d11ResToUndind,
                            TSetD3D11View          SetD3D11ViewMethods[]);

    /// Unbinds a texture from shader resource view slots.
    /// \note The function only unbinds the texture from d3d11 device
    ///       context. All shader bindings are retained.
    void UnbindTextureFromInput(TextureBaseD3D11* pTexture, ID3D11Resource* pd3d11Resource);

    /// Unbinds a buffer from input (shader resource views slots, index
    /// and vertex buffer slots).
    /// \note The function only unbinds the buffer from d3d11 device
    ///       context. All shader bindings are retained.
    void UnbindBufferFromInput(BufferD3D11Impl* pBuffer, ID3D11Resource* pd3d11Buffer);

    /// Unbinds a resource from UAV slots.
    /// \note The function only unbinds the resource from d3d11 device
    ///       context. All shader bindings are retained.
    void UnbindResourceFromUAV(IDeviceObject* pResource, ID3D11Resource* pd3d11Resource);

    /// Unbinds a texture from render target slots.
    void UnbindTextureFromRenderTarget(TextureBaseD3D11* pResource);

    /// Unbinds a texture from depth-stencil.
    void UnbindTextureFromDepthStencil(TextureBaseD3D11* pTexD3D11);

    /// Prepares for a draw command
    __forceinline void PrepareForDraw(DRAW_FLAGS Flags);

    /// Prepares for an indexed draw command
    __forceinline void PrepareForIndexedDraw(DRAW_FLAGS Flags, VALUE_TYPE IndexType);

    /// Performs operations required to begin current subpass (e.g. bind render targets)
    void BeginSubpass();
    /// Ends current subpass
    void EndSubpass();

    template <bool TransitionResources,
              bool CommitResources>
    void TransitionAndCommitShaderResources(IPipelineState* pPSO, IShaderResourceBinding* pShaderResourceBinding, bool VerifyStates);

    void ClearStateCache();

    std::shared_ptr<DisjointQueryPool::DisjointQueryWrapper> BeginDisjointQuery();

    CComPtr<ID3D11DeviceContext> m_pd3d11DeviceContext; ///< D3D11 device context

    // clang-format off

    /// An array of D3D11 constant buffers committed to D3D11 device context,
    /// for each shader type. The context addref's all bound resources, so we do
    /// not need to keep strong references.
    ID3D11Buffer*              m_CommittedD3D11CBs     [NumShaderTypes][D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    
    /// An array of D3D11 shader resource views committed to D3D11 device context,
    /// for each shader type. The context addref's all bound resources, so we do 
    /// not need to keep strong references.
    ID3D11ShaderResourceView*  m_CommittedD3D11SRVs    [NumShaderTypes][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    
    /// An array of D3D11 samplers committed to D3D11 device context,
    /// for each shader type. The context addref's all bound resources, so we do 
    /// not need to keep strong references.
    ID3D11SamplerState*        m_CommittedD3D11Samplers[NumShaderTypes][D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};
    
    /// An array of D3D11 UAVs committed to D3D11 device context,
    /// for each shader type. The context addref's all bound resources, so we do 
    /// not need to keep strong references.
    ID3D11UnorderedAccessView* m_CommittedD3D11UAVs    [NumShaderTypes][D3D11_PS_CS_UAV_REGISTER_COUNT] = {};

    /// An array of D3D11 resources commited as SRV to D3D11 device context,
    /// for each shader type. The context addref's all bound resources, so we do 
    /// not need to keep strong references.
    ID3D11Resource*  m_CommittedD3D11SRVResources      [NumShaderTypes][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};

    /// An array of D3D11 resources commited as UAV to D3D11 device context,
    /// for each shader type. The context addref's all bound resources, so we do 
    /// not need to keep strong references.
    ID3D11Resource*  m_CommittedD3D11UAVResources      [NumShaderTypes][D3D11_PS_CS_UAV_REGISTER_COUNT] = {};

    Uint8 m_NumCommittedCBs     [NumShaderTypes] = {};
    Uint8 m_NumCommittedSRVs    [NumShaderTypes] = {};
    Uint8 m_NumCommittedSamplers[NumShaderTypes] = {};
    Uint8 m_NumCommittedUAVs    [NumShaderTypes] = {};

    // clang-format on

    /// An array of D3D11 vertex buffers committed to D3D device context.
    /// There is no need to keep strong references because D3D11 device context
    /// already does. Buffers cannot be destroyed while bound to the context.
    /// We only mirror all bindings.
    ID3D11Buffer* m_CommittedD3D11VertexBuffers[MAX_BUFFER_SLOTS] = {};
    /// An array of strides of committed vertex buffers
    UINT m_CommittedD3D11VBStrides[MAX_BUFFER_SLOTS] = {};
    /// An array of offsets of committed vertex buffers
    UINT m_CommittedD3D11VBOffsets[MAX_BUFFER_SLOTS] = {};
    /// Number committed vertex buffers
    UINT m_NumCommittedD3D11VBs = 0;
    /// Flag indicating if currently committed D3D11 vertex buffers are up to date
    bool m_bCommittedD3D11VBsUpToDate = false;

    /// D3D11 input layout committed to device context.
    /// The context keeps the layout alive, so there is no need
    /// to keep strong reference.
    ID3D11InputLayout* m_CommittedD3D11InputLayout = nullptr;

    /// Strong reference to D3D11 buffer committed as index buffer
    /// to D3D device context.
    CComPtr<ID3D11Buffer> m_CommittedD3D11IndexBuffer;
    /// Format of the committed D3D11 index buffer
    VALUE_TYPE m_CommittedIBFormat = VT_UNDEFINED;
    /// Offset of the committed D3D11 index buffer
    Uint32 m_CommittedD3D11IndexDataStartOffset = 0;
    /// Flag indicating if currently committed D3D11 index buffer is up to date
    bool m_bCommittedD3D11IBUpToDate = false;

    D3D11_PRIMITIVE_TOPOLOGY m_CommittedD3D11PrimTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    PRIMITIVE_TOPOLOGY       m_CommittedPrimitiveTopology = PRIMITIVE_TOPOLOGY_UNDEFINED;

    /// Strong references to committed D3D11 shaders
    CComPtr<ID3D11DeviceChild> m_CommittedD3DShaders[NumShaderTypes];

    const Uint32 m_DebugFlags;

    FixedBlockMemoryAllocator m_CmdListAllocator;

    DisjointQueryPool                                        m_DisjointQueryPool;
    std::shared_ptr<DisjointQueryPool::DisjointQueryWrapper> m_ActiveDisjointQuery;

    std::vector<OptimizedClearValue> m_AttachmentClearValues;

#ifdef VERIFY_CONTEXT_BINDINGS

    /// Helper template function used to facilitate context verification
    template <UINT MaxResources, typename TD3D11ResourceType, typename TGetD3D11ResourcesType>
    void dbgVerifyCommittedResources(TD3D11ResourceType     CommittedD3D11ResourcesArr[][MaxResources],
                                     Uint8                  NumCommittedResourcesArr[],
                                     TGetD3D11ResourcesType GetD3D11ResMethods[],
                                     const Char*            ResourceName,
                                     SHADER_TYPE            ShaderType);

    /// Helper template function used to facilitate validation of SRV and UAV consistency with D3D11 resources
    template <UINT MaxResources, typename TD3D11ViewType>
    void dbgVerifyViewConsistency(TD3D11ViewType  CommittedD3D11ViewArr[][MaxResources],
                                  ID3D11Resource* CommittedD3D11ResourcesArr[][MaxResources],
                                  Uint8           NumCommittedResourcesArr[],
                                  const Char*     ResourceName,
                                  SHADER_TYPE     ShaderType);

    /// Debug function that verifies that SRVs cached in m_CommittedD3D11SRVs
    /// array comply with resources actually committed to D3D11 device context
    void dbgVerifyCommittedSRVs(SHADER_TYPE ShaderType = SHADER_TYPE_UNKNOWN);

    /// Debug function that verifies that UAVs cached in m_CommittedD3D11UAVs
    /// array comply with resources actually committed to D3D11 device context
    void dbgVerifyCommittedUAVs(SHADER_TYPE ShaderType = SHADER_TYPE_UNKNOWN);

    /// Debug function that verifies that samplers cached in m_CommittedD3D11Samplers
    /// array comply with resources actually committed to D3D11 device context
    void dbgVerifyCommittedSamplers(SHADER_TYPE ShaderType = SHADER_TYPE_UNKNOWN);

    /// Debug function that verifies that constant buffers cached in m_CommittedD3D11CBs
    /// array comply with buffers actually committed to D3D11 device context
    void dbgVerifyCommittedCBs(SHADER_TYPE ShaderType = SHADER_TYPE_UNKNOWN);

    /// Debug function that verifies that index buffer cached in
    /// m_CommittedD3D11IndexBuffer is the buffer actually committed to D3D11
    /// device context
    void dbgVerifyCommittedIndexBuffer();

    /// Debug function that verifies that vertex buffers cached in
    /// m_CommittedD3D11VertexBuffers are the buffers actually committed to D3D11
    /// device context
    void dbgVerifyCommittedVertexBuffers();

    /// Debug function that verifies that shaders cached in
    /// m_CommittedD3DShaders are the shaders actually committed to D3D11
    /// device context
    void dbgVerifyCommittedShaders();

#else
#    define dbgVerifyRenderTargetFormats(...)
#    define dbgVerifyCommittedSRVs(...)
#    define dbgVerifyCommittedUAVs(...)
#    define dbgVerifyCommittedSamplers(...)
#    define dbgVerifyCommittedCBs(...)
#    define dbgVerifyCommittedIndexBuffer(...)
#    define dbgVerifyCommittedVertexBuffers(...)
#    define dbgVerifyCommittedShaders(...)
#endif // VERIFY_CONTEXT_BINDINGS
};

} // namespace Diligent
