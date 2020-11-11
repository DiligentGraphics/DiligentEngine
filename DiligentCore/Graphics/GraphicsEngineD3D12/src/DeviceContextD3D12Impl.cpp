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
#include <sstream>
#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "CommandContext.hpp"
#include "TextureD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "FenceD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"
#include "d3dx12_win.h"
#include "D3D12DynamicHeap.hpp"
#include "CommandListD3D12Impl.hpp"
#include "DXGITypeConversions.hpp"

namespace Diligent
{

static std::string GetContextObjectName(const char* Object, bool bIsDeferred, Uint32 ContextId)
{
    std::stringstream ss;
    ss << Object;
    if (bIsDeferred)
        ss << " of deferred context #" << ContextId;
    else
        ss << " of immediate context";
    return ss.str();
}

DeviceContextD3D12Impl::DeviceContextD3D12Impl(IReferenceCounters*          pRefCounters,
                                               RenderDeviceD3D12Impl*       pDeviceD3D12Impl,
                                               bool                         bIsDeferred,
                                               const EngineD3D12CreateInfo& EngineCI,
                                               Uint32                       ContextId,
                                               Uint32                       CommandQueueId) :
    // clang-format off
    TDeviceContextBase
    {
        pRefCounters,
        pDeviceD3D12Impl,
        ContextId,
        CommandQueueId,
        bIsDeferred ? std::numeric_limits<decltype(m_NumCommandsToFlush)>::max() : EngineCI.NumCommandsToFlushCmdList,
        bIsDeferred
    },
    m_DynamicHeap
    {
        pDeviceD3D12Impl->GetDynamicMemoryManager(),
        GetContextObjectName("Dynamic heap", bIsDeferred, ContextId),
        EngineCI.DynamicHeapPageSize
    },
    m_DynamicGPUDescriptorAllocator
    {
        {
            GetRawAllocator(),
            pDeviceD3D12Impl->GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
            EngineCI.DynamicDescriptorAllocationChunkSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV],
            GetContextObjectName("CBV_SRV_UAV dynamic descriptor allocator", bIsDeferred, ContextId)
        },
        {
            GetRawAllocator(),
            pDeviceD3D12Impl->GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
            EngineCI.DynamicDescriptorAllocationChunkSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER],
            GetContextObjectName("SAMPLER     dynamic descriptor allocator", bIsDeferred, ContextId)
        }
    },
    m_CmdListAllocator{GetRawAllocator(), sizeof(CommandListD3D12Impl), 64}
// clang-format on
{
    RequestCommandContext(pDeviceD3D12Impl);

    auto* pd3d12Device = pDeviceD3D12Impl->GetD3D12Device();

    D3D12_COMMAND_SIGNATURE_DESC CmdSignatureDesc = {};
    D3D12_INDIRECT_ARGUMENT_DESC IndirectArg      = {};

    CmdSignatureDesc.NodeMask         = 0;
    CmdSignatureDesc.NumArgumentDescs = 1;
    CmdSignatureDesc.pArgumentDescs   = &IndirectArg;

    CmdSignatureDesc.ByteStride = sizeof(UINT) * 4;
    IndirectArg.Type            = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
    auto hr                     = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, __uuidof(m_pDrawIndirectSignature), reinterpret_cast<void**>(static_cast<ID3D12CommandSignature**>(&m_pDrawIndirectSignature)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create indirect draw command signature");

    CmdSignatureDesc.ByteStride = sizeof(UINT) * 5;
    IndirectArg.Type            = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    hr                          = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, __uuidof(m_pDrawIndexedIndirectSignature), reinterpret_cast<void**>(static_cast<ID3D12CommandSignature**>(&m_pDrawIndexedIndirectSignature)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create draw indexed indirect command signature");

    CmdSignatureDesc.ByteStride = sizeof(UINT) * 3;
    IndirectArg.Type            = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    hr                          = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, __uuidof(m_pDispatchIndirectSignature), reinterpret_cast<void**>(static_cast<ID3D12CommandSignature**>(&m_pDispatchIndirectSignature)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create dispatch indirect command signature");

#ifdef D3D12_H_HAS_MESH_SHADER
    CmdSignatureDesc.ByteStride = sizeof(UINT) * 3;
    IndirectArg.Type            = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
    hr                          = pd3d12Device->CreateCommandSignature(&CmdSignatureDesc, nullptr, __uuidof(m_pDrawMeshIndirectSignature), reinterpret_cast<void**>(static_cast<ID3D12CommandSignature**>(&m_pDrawMeshIndirectSignature)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create draw mesh indirect command signature");
#endif
}

DeviceContextD3D12Impl::~DeviceContextD3D12Impl()
{
    if (m_State.NumCommands != 0)
    {
        if (m_bIsDeferred)
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in deferred context #",
                              m_ContextId,
                              " being destroyed, which indicates that FinishCommandList() has not been called."
                              " This may cause synchronization issues.");
        }
        else
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in the immediate context being destroyed, "
                              "which indicates the context has not been Flush()'ed.",
                              " This may cause synchronization issues.");
        }
    }

    if (m_bIsDeferred)
    {
        if (m_CurrCmdCtx)
        {
            // The command context has never been executed, so it can be disposed without going through release queue
            m_pDevice->DisposeCommandContext(std::move(m_CurrCmdCtx));
        }
    }
    else
    {
        Flush(false);
    }

    // For deferred contexts, m_SubmittedBuffersCmdQueueMask is reset to 0 after every call to FinishFrame().
    // In this case there are no resources to release, so there will be no issues.
    FinishFrame();

    // Note: as dynamic pages are returned to the global dynamic memory manager hosted by the render device,
    // the dynamic heap can be destroyed before all pages are actually returned to the global manager.
    DEV_CHECK_ERR(m_DynamicHeap.GetAllocatedPagesCount() == 0, "All dynamic pages must have been released by now.");

    for (size_t i = 0; i < _countof(m_DynamicGPUDescriptorAllocator); ++i)
    {
        // Note: as dynamic decriptor suballocations are returned to the global GPU descriptor heap that
        // is hosted by the render device, the descriptor allocator can be destroyed before all suballocations
        // are actually returned to the global heap.
        DEV_CHECK_ERR(m_DynamicGPUDescriptorAllocator[i].GetSuballocationCount() == 0, "All dynamic suballocations must have been released");
    }
}

IMPLEMENT_QUERY_INTERFACE(DeviceContextD3D12Impl, IID_DeviceContextD3D12, TDeviceContextBase)

void DeviceContextD3D12Impl::SetPipelineState(IPipelineState* pPipelineState)
{
    auto* pPipelineStateD3D12 = ValidatedCast<PipelineStateD3D12Impl>(pPipelineState);
    if (PipelineStateD3D12Impl::IsSameObject(m_pPipelineState, pPipelineStateD3D12))
        return;

    // Never flush deferred context!
    // For the query types which support both BeginQuery and EndQuery (all except for timestamp),
    // a query for a given element must not span command list boundaries.
    if (!m_bIsDeferred && m_State.NumCommands >= m_NumCommandsToFlush && m_ActiveQueriesCounter == 0)
    {
        Flush(true);
    }

    const auto& PSODesc = pPipelineStateD3D12->GetDesc();

    bool CommitStates  = false;
    bool CommitScissor = false;
    if (!m_pPipelineState)
    {
        // If no pipeline state is bound, we are working with the fresh command
        // list. We have to commit the states set in the context that are not
        // committed by the draw command (render targets, viewports, scissor rects, etc.)
        CommitStates = true;
    }
    else
    {
        const auto& OldPSODesc = m_pPipelineState->GetDesc();
        // Commit all graphics states when switching from compute pipeline
        // This is necessary because if the command list had been flushed
        // and the first PSO set on the command list was a compute pipeline,
        // the states would otherwise never be committed (since m_pPipelineState != nullptr)
        CommitStates = !OldPSODesc.IsAnyGraphicsPipeline();
        // We also need to update scissor rect if ScissorEnable state has changed
        if (OldPSODesc.IsAnyGraphicsPipeline() && PSODesc.IsAnyGraphicsPipeline())
            CommitScissor = m_pPipelineState->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable != pPipelineStateD3D12->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable;
    }

    TDeviceContextBase::SetPipelineState(pPipelineStateD3D12, 0 /*Dummy*/);

    auto& CmdCtx    = GetCmdContext();
    auto* pd3d12PSO = pPipelineStateD3D12->GetD3D12PipelineState();

    switch (PSODesc.PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
        case PIPELINE_TYPE_MESH:
        {
            auto& GraphicsPipeline = pPipelineStateD3D12->GetGraphicsPipelineDesc();
            auto& GraphicsCtx      = CmdCtx.AsGraphicsContext();
            GraphicsCtx.SetPipelineState(pd3d12PSO);

            if (PSODesc.PipelineType == PIPELINE_TYPE_GRAPHICS)
            {
                auto D3D12Topology = TopologyToD3D12Topology(GraphicsPipeline.PrimitiveTopology);
                GraphicsCtx.SetPrimitiveTopology(D3D12Topology);
            }

            if (CommitStates)
            {
                GraphicsCtx.SetStencilRef(m_StencilRef);
                GraphicsCtx.SetBlendFactor(m_BlendFactors);
                if (GraphicsPipeline.pRenderPass == nullptr)
                {
                    CommitRenderTargets(RESOURCE_STATE_TRANSITION_MODE_VERIFY);
                }
                CommitViewports();
            }

            if (CommitStates || CommitScissor)
            {
                CommitScissorRects(GraphicsCtx, GraphicsPipeline.RasterizerDesc.ScissorEnable);
            }
            break;
        }

        case PIPELINE_TYPE_COMPUTE:
        {
            CmdCtx.AsComputeContext().SetPipelineState(pd3d12PSO);
            break;
        }

        default:
            UNEXPECTED("unknown pipeline type");
    }

    m_State.pCommittedResourceCache = nullptr;
    m_State.bRootViewsCommitted     = false;
}

void DeviceContextD3D12Impl::TransitionShaderResources(IPipelineState* pPipelineState, IShaderResourceBinding* pShaderResourceBinding)
{
    DEV_CHECK_ERR(pPipelineState != nullptr, "Pipeline state must mot be null");
    if (m_pActiveRenderPass)
    {
        LOG_ERROR_MESSAGE("State transitions are not allowed inside a render pass.");
        return;
    }

    auto& Ctx = GetCmdContext();

    auto* pPipelineStateD3D12 = ValidatedCast<PipelineStateD3D12Impl>(pPipelineState);

    PipelineStateD3D12Impl::CommitAndTransitionResourcesAttribs Attribs;
    Attribs.CtxId                  = m_ContextId;
    Attribs.pShaderResourceBinding = pShaderResourceBinding;
    Attribs.CommitResources        = false;
    Attribs.TransitionResources    = true;
    Attribs.ValidateStates         = false;
    pPipelineStateD3D12->CommitAndTransitionShaderResources(this, Ctx, Attribs);
}

void DeviceContextD3D12Impl::CommitShaderResources(IShaderResourceBinding* pShaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (!DeviceContextBase::CommitShaderResources(pShaderResourceBinding, StateTransitionMode, 0 /*Dummy*/))
        return;

    auto& Ctx = GetCmdContext();

    PipelineStateD3D12Impl::CommitAndTransitionResourcesAttribs Attribs;
    Attribs.CtxId                   = m_ContextId;
    Attribs.pShaderResourceBinding  = pShaderResourceBinding;
    Attribs.CommitResources         = true;
    Attribs.TransitionResources     = StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Attribs.ValidateStates          = StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY;
    m_State.pCommittedResourceCache = m_pPipelineState->CommitAndTransitionShaderResources(this, Ctx, Attribs);
    m_State.bRootViewsCommitted     = false;
}

void DeviceContextD3D12Impl::SetStencilRef(Uint32 StencilRef)
{
    if (TDeviceContextBase::SetStencilRef(StencilRef, 0))
    {
        GetCmdContext().AsGraphicsContext().SetStencilRef(m_StencilRef);
    }
}

void DeviceContextD3D12Impl::SetBlendFactors(const float* pBlendFactors)
{
    if (TDeviceContextBase::SetBlendFactors(pBlendFactors, 0))
    {
        GetCmdContext().AsGraphicsContext().SetBlendFactor(m_BlendFactors);
    }
}

void DeviceContextD3D12Impl::CommitD3D12IndexBuffer(GraphicsContext& GraphCtx, VALUE_TYPE IndexType)
{
    VERIFY(m_pIndexBuffer != nullptr, "Index buffer is not set up for indexed draw command");

    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = m_pIndexBuffer->GetGPUAddress(m_ContextId, this) + m_IndexDataStartOffset;
    if (IndexType == VT_UINT32)
        IBView.Format = DXGI_FORMAT_R32_UINT;
    else
    {
        DEV_CHECK_ERR(IndexType == VT_UINT16, "Unsupported index format. Only R16_UINT and R32_UINT are allowed.");
        IBView.Format = DXGI_FORMAT_R16_UINT;
    }
    // Note that for a dynamic buffer, what we use here is the size of the buffer itself, not the upload heap buffer!
    IBView.SizeInBytes = m_pIndexBuffer->GetDesc().uiSizeInBytes - m_IndexDataStartOffset;

    // Device context keeps strong reference to bound index buffer.
    // When the buffer is unbound, the reference to the D3D12 resource
    // is added to the context. There is no need to add reference here
    //auto &GraphicsCtx = GetCmdContext().AsGraphicsContext();
    //auto *pd3d12Resource = pBuffD3D12->GetD3D12Buffer();
    //GraphicsCtx.AddReferencedObject(pd3d12Resource);

    bool IsDynamic = m_pIndexBuffer->GetDesc().Usage == USAGE_DYNAMIC;
#ifdef DILIGENT_DEVELOPMENT
    if (IsDynamic)
        m_pIndexBuffer->DvpVerifyDynamicAllocation(this);
#endif

    Uint64 BuffDataStartByteOffset;
    auto*  pd3d12Buff = m_pIndexBuffer->GetD3D12Buffer(BuffDataStartByteOffset, this);

    // clang-format off
    if (IsDynamic ||
        m_State.CommittedD3D12IndexBuffer          != pd3d12Buff ||
        m_State.CommittedIBFormat                  != IndexType ||
        m_State.CommittedD3D12IndexDataStartOffset != m_IndexDataStartOffset + BuffDataStartByteOffset)
    // clang-format on
    {
        m_State.CommittedD3D12IndexBuffer          = pd3d12Buff;
        m_State.CommittedIBFormat                  = IndexType;
        m_State.CommittedD3D12IndexDataStartOffset = m_IndexDataStartOffset + BuffDataStartByteOffset;
        GraphCtx.SetIndexBuffer(IBView);
    }

    // GPU virtual address of a dynamic index buffer can change every time
    // a draw command is invoked
    m_State.bCommittedD3D12IBUpToDate = !IsDynamic;
}

void DeviceContextD3D12Impl::CommitD3D12VertexBuffers(GraphicsContext& GraphCtx)
{
    // Do not initialize array with zeroes for performance reasons
    D3D12_VERTEX_BUFFER_VIEW VBViews[MAX_BUFFER_SLOTS]; // = {}
    VERIFY(m_NumVertexStreams <= MAX_BUFFER_SLOTS, "Too many buffers are being set");
    DEV_CHECK_ERR(m_NumVertexStreams >= m_pPipelineState->GetNumBufferSlotsUsed(), "Currently bound pipeline state '", m_pPipelineState->GetDesc().Name, "' expects ", m_pPipelineState->GetNumBufferSlotsUsed(), " input buffer slots, but only ", m_NumVertexStreams, " is bound");
    bool DynamicBufferPresent = false;
    for (UINT Buff = 0; Buff < m_NumVertexStreams; ++Buff)
    {
        auto& CurrStream = m_VertexStreams[Buff];
        auto& VBView     = VBViews[Buff];
        if (auto* pBufferD3D12 = CurrStream.pBuffer.RawPtr())
        {
            if (pBufferD3D12->GetDesc().Usage == USAGE_DYNAMIC)
            {
                DynamicBufferPresent = true;
#ifdef DILIGENT_DEVELOPMENT
                pBufferD3D12->DvpVerifyDynamicAllocation(this);
#endif
            }

            // Device context keeps strong references to all vertex buffers.
            // When a buffer is unbound, a reference to D3D12 resource is added to the context,
            // so there is no need to reference the resource here
            //GraphicsCtx.AddReferencedObject(pd3d12Resource);

            VBView.BufferLocation = pBufferD3D12->GetGPUAddress(m_ContextId, this) + CurrStream.Offset;
            VBView.StrideInBytes  = m_pPipelineState->GetBufferStride(Buff);
            // Note that for a dynamic buffer, what we use here is the size of the buffer itself, not the upload heap buffer!
            VBView.SizeInBytes = pBufferD3D12->GetDesc().uiSizeInBytes - CurrStream.Offset;
        }
        else
        {
            VBView = D3D12_VERTEX_BUFFER_VIEW{};
        }
    }

    GraphCtx.FlushResourceBarriers();
    GraphCtx.SetVertexBuffers(0, m_NumVertexStreams, VBViews);

    // GPU virtual address of a dynamic vertex buffer can change every time
    // a draw command is invoked
    m_State.bCommittedD3D12VBsUpToDate = !DynamicBufferPresent;
}

void DeviceContextD3D12Impl::PrepareForDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_RENDER_TARGETS) != 0)
        DvpVerifyRenderTargets();
#endif

    if (!m_State.bCommittedD3D12VBsUpToDate && m_pPipelineState->GetNumBufferSlotsUsed() > 0)
    {
        CommitD3D12VertexBuffers(GraphCtx);
    }

#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_STATES) != 0)
    {
        for (Uint32 Buff = 0; Buff < m_NumVertexStreams; ++Buff)
        {
            const auto& CurrStream   = m_VertexStreams[Buff];
            const auto* pBufferD3D12 = CurrStream.pBuffer.RawPtr();
            if (pBufferD3D12 != nullptr)
            {
                DvpVerifyBufferState(*pBufferD3D12, RESOURCE_STATE_VERTEX_BUFFER, "Using vertex buffers (DeviceContextD3D12Impl::Draw())");
            }
        }
    }
#endif

    GraphCtx.SetRootSignature(m_pPipelineState->GetD3D12RootSignature());

    if (m_State.pCommittedResourceCache != nullptr)
    {
        if (m_State.pCommittedResourceCache->GetNumDynamicCBsBound() > 0)
        {
            if (!m_State.bRootViewsCommitted || (Flags & DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT) == 0)
            {
                // Only process dynamic buffers. Non-dynamic buffers are committed by CommitShaderResources
                m_pPipelineState->GetRootSignature()
                    .CommitRootViews(*m_State.pCommittedResourceCache,
                                     GraphCtx,
                                     false, // IsCompute
                                     m_ContextId,
                                     this,
                                     true,  // CommitViews
                                     true,  // ProcessDynamicBuffers
                                     false, // ProcessNonDynamicBuffers
                                     false, // TransitionStates
                                     false  // ValidateStates
                    );
                m_State.bRootViewsCommitted = true;
            }
        }
    }
#ifdef DILIGENT_DEVELOPMENT
    else
    {
        if (m_pPipelineState->ContainsShaderResources())
            LOG_ERROR_MESSAGE("Pipeline state '", m_pPipelineState->GetDesc().Name, "' contains shader resources, but IDeviceContext::CommitShaderResources() was not called with non-null SRB");
    }
#endif
}

void DeviceContextD3D12Impl::PrepareForIndexedDraw(GraphicsContext& GraphCtx, DRAW_FLAGS Flags, VALUE_TYPE IndexType)
{
    PrepareForDraw(GraphCtx, Flags);
    if (m_State.CommittedIBFormat != IndexType)
        m_State.bCommittedD3D12IBUpToDate = false;
    if (!m_State.bCommittedD3D12IBUpToDate)
    {
        CommitD3D12IndexBuffer(GraphCtx, IndexType);
    }
#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_STATES) != 0)
    {
        DvpVerifyBufferState(*m_pIndexBuffer, RESOURCE_STATE_INDEX_BUFFER, "Indexed draw (DeviceContextD3D12Impl::Draw())");
    }
#endif
}

void DeviceContextD3D12Impl::Draw(const DrawAttribs& Attribs)
{
    if (!DvpVerifyDrawArguments(Attribs))
        return;

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForDraw(GraphCtx, Attribs.Flags);
    GraphCtx.Draw(Attribs.NumVertices, Attribs.NumInstances, Attribs.StartVertexLocation, Attribs.FirstInstanceLocation);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::DrawIndexed(const DrawIndexedAttribs& Attribs)
{
    if (!DvpVerifyDrawIndexedArguments(Attribs))
        return;

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForIndexedDraw(GraphCtx, Attribs.Flags, Attribs.IndexType);
    GraphCtx.DrawIndexed(Attribs.NumIndices, Attribs.NumInstances, Attribs.FirstIndexLocation, Attribs.BaseVertex, Attribs.FirstInstanceLocation);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::PrepareDrawIndirectBuffer(GraphicsContext&               GraphCtx,
                                                       IBuffer*                       pAttribsBuffer,
                                                       RESOURCE_STATE_TRANSITION_MODE BufferStateTransitionMode,
                                                       ID3D12Resource*&               pd3d12ArgsBuff,
                                                       Uint64&                        BuffDataStartByteOffset)
{
    DEV_CHECK_ERR(pAttribsBuffer != nullptr, "Indirect draw attribs buffer must not be null");

    auto* pIndirectDrawAttribsD3D12 = ValidatedCast<BufferD3D12Impl>(pAttribsBuffer);
#ifdef DILIGENT_DEVELOPMENT
    if (pIndirectDrawAttribsD3D12->GetDesc().Usage == USAGE_DYNAMIC)
        pIndirectDrawAttribsD3D12->DvpVerifyDynamicAllocation(this);
#endif

    TransitionOrVerifyBufferState(GraphCtx, *pIndirectDrawAttribsD3D12, BufferStateTransitionMode,
                                  RESOURCE_STATE_INDIRECT_ARGUMENT,
                                  "Indirect draw (DeviceContextD3D12Impl::PrepareDrawIndirectBuffer)");

    pd3d12ArgsBuff = pIndirectDrawAttribsD3D12->GetD3D12Buffer(BuffDataStartByteOffset, this);
}

void DeviceContextD3D12Impl::DrawIndirect(const DrawIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDrawIndirectArguments(Attribs, pAttribsBuffer))
        return;

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForDraw(GraphCtx, Attribs.Flags);

    ID3D12Resource* pd3d12ArgsBuff;
    Uint64          BuffDataStartByteOffset;
    PrepareDrawIndirectBuffer(GraphCtx, pAttribsBuffer, Attribs.IndirectAttribsBufferStateTransitionMode, pd3d12ArgsBuff, BuffDataStartByteOffset);

    GraphCtx.ExecuteIndirect(m_pDrawIndirectSignature, pd3d12ArgsBuff, Attribs.IndirectDrawArgsOffset + BuffDataStartByteOffset);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDrawIndexedIndirectArguments(Attribs, pAttribsBuffer))
        return;

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForIndexedDraw(GraphCtx, Attribs.Flags, Attribs.IndexType);

    ID3D12Resource* pd3d12ArgsBuff;
    Uint64          BuffDataStartByteOffset;
    PrepareDrawIndirectBuffer(GraphCtx, pAttribsBuffer, Attribs.IndirectAttribsBufferStateTransitionMode, pd3d12ArgsBuff, BuffDataStartByteOffset);

    GraphCtx.ExecuteIndirect(m_pDrawIndexedIndirectSignature, pd3d12ArgsBuff, Attribs.IndirectDrawArgsOffset + BuffDataStartByteOffset);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::DrawMesh(const DrawMeshAttribs& Attribs)
{
    if (!DvpVerifyDrawMeshArguments(Attribs))
        return;

    auto& GraphCtx = GetCmdContext().AsGraphicsContext6();
    PrepareForDraw(GraphCtx, Attribs.Flags);

    GraphCtx.DrawMesh(Attribs.ThreadGroupCount, 1, 1);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDrawMeshIndirectArguments(Attribs, pAttribsBuffer))
        return;

    auto& GraphCtx = GetCmdContext().AsGraphicsContext();
    PrepareForDraw(GraphCtx, Attribs.Flags);

    ID3D12Resource* pd3d12ArgsBuff;
    Uint64          BuffDataStartByteOffset;
    PrepareDrawIndirectBuffer(GraphCtx, pAttribsBuffer, Attribs.IndirectAttribsBufferStateTransitionMode, pd3d12ArgsBuff, BuffDataStartByteOffset);

    GraphCtx.ExecuteIndirect(m_pDrawMeshIndirectSignature, pd3d12ArgsBuff, Attribs.IndirectDrawArgsOffset + BuffDataStartByteOffset);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::PrepareForDispatchCompute(ComputeContext& ComputeCtx)
{
    ComputeCtx.SetRootSignature(m_pPipelineState->GetD3D12RootSignature());
    if (m_State.pCommittedResourceCache != nullptr)
    {
        if (m_State.pCommittedResourceCache->GetNumDynamicCBsBound() > 0)
        {
            // Only process dynamic buffers. Non-dynamic buffers are committed by CommitShaderResources
            m_pPipelineState->GetRootSignature()
                .CommitRootViews(*m_State.pCommittedResourceCache,
                                 ComputeCtx,
                                 true, // IsCompute
                                 m_ContextId,
                                 this,
                                 true,  // CommitViews
                                 true,  // ProcessDynamicBuffers
                                 false, // ProcessNonDynamicBuffers
                                 false, // TransitionStates
                                 false  // ValidateStates
                );
        }
    }
#ifdef DILIGENT_DEVELOPMENT
    else
    {
        if (m_pPipelineState->ContainsShaderResources())
            LOG_ERROR_MESSAGE("Pipeline state '", m_pPipelineState->GetDesc().Name, "' contains shader resources, but IDeviceContext::CommitShaderResources() was not called with non-null SRB");
    }
#endif
}

void DeviceContextD3D12Impl::DispatchCompute(const DispatchComputeAttribs& Attribs)
{
    if (!DvpVerifyDispatchArguments(Attribs))
        return;

    auto& ComputeCtx = GetCmdContext().AsComputeContext();
    PrepareForDispatchCompute(ComputeCtx);
    ComputeCtx.Dispatch(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDispatchIndirectArguments(Attribs, pAttribsBuffer))
        return;

    auto& ComputeCtx = GetCmdContext().AsComputeContext();
    PrepareForDispatchCompute(ComputeCtx);

    auto* pBufferD3D12 = ValidatedCast<BufferD3D12Impl>(pAttribsBuffer);

#ifdef DILIGENT_DEVELOPMENT
    if (pBufferD3D12->GetDesc().Usage == USAGE_DYNAMIC)
        pBufferD3D12->DvpVerifyDynamicAllocation(this);
#endif

    TransitionOrVerifyBufferState(ComputeCtx, *pBufferD3D12, Attribs.IndirectAttribsBufferStateTransitionMode,
                                  RESOURCE_STATE_INDIRECT_ARGUMENT, "Indirect dispatch (DeviceContextD3D12Impl::DispatchComputeIndirect)");
    Uint64          BuffDataStartByteOffset;
    ID3D12Resource* pd3d12ArgsBuff = pBufferD3D12->GetD3D12Buffer(BuffDataStartByteOffset, this);
    ComputeCtx.ExecuteIndirect(m_pDispatchIndirectSignature, pd3d12ArgsBuff, Attribs.DispatchArgsByteOffset + BuffDataStartByteOffset);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::ClearDepthStencil(ITextureView*                  pView,
                                               CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                               float                          fDepth,
                                               Uint8                          Stencil,
                                               RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Direct3D12 does not allow depth-stencil clears inside a render pass");
        return;
    }

    if (!TDeviceContextBase::ClearDepthStencil(pView))
        return;

    VERIFY_EXPR(pView != nullptr);

    auto* pViewD3D12    = ValidatedCast<ITextureViewD3D12>(pView);
    auto* pTextureD3D12 = ValidatedCast<TextureD3D12Impl>(pViewD3D12->GetTexture());
    auto& CmdCtx        = GetCmdContext();
    TransitionOrVerifyTextureState(CmdCtx, *pTextureD3D12, StateTransitionMode, RESOURCE_STATE_DEPTH_WRITE, "Clearing depth-stencil buffer (DeviceContextD3D12Impl::ClearDepthStencil)");

    D3D12_CLEAR_FLAGS d3d12ClearFlags = (D3D12_CLEAR_FLAGS)0;
    if (ClearFlags & CLEAR_DEPTH_FLAG) d3d12ClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
    if (ClearFlags & CLEAR_STENCIL_FLAG) d3d12ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;

    // The full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied??
    GetCmdContext().AsGraphicsContext().ClearDepthStencil(pViewD3D12->GetCPUDescriptorHandle(), d3d12ClearFlags, fDepth, Stencil);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::ClearRenderTarget(ITextureView* pView, const float* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Direct3D12 does not allow render target clears inside a render pass");
        return;
    }

    if (!TDeviceContextBase::ClearRenderTarget(pView))
        return;

    VERIFY_EXPR(pView != nullptr);

    auto pViewD3D12 = ValidatedCast<ITextureViewD3D12>(pView);

    static constexpr float Zero[4] = {0.f, 0.f, 0.f, 0.f};
    if (RGBA == nullptr)
        RGBA = Zero;

    auto* pTextureD3D12 = ValidatedCast<TextureD3D12Impl>(pViewD3D12->GetTexture());
    auto& CmdCtx        = GetCmdContext();
    TransitionOrVerifyTextureState(CmdCtx, *pTextureD3D12, StateTransitionMode, RESOURCE_STATE_RENDER_TARGET, "Clearing render target (DeviceContextD3D12Impl::ClearRenderTarget)");

    // The full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied??
    CmdCtx.AsGraphicsContext().ClearRenderTarget(pViewD3D12->GetCPUDescriptorHandle(), RGBA);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::RequestCommandContext(RenderDeviceD3D12Impl* pDeviceD3D12Impl)
{
    m_CurrCmdCtx = pDeviceD3D12Impl->AllocateCommandContext();
    m_CurrCmdCtx->SetDynamicGPUDescriptorAllocators(m_DynamicGPUDescriptorAllocator);
}

void DeviceContextD3D12Impl::Flush(bool RequestNewCmdCtx)
{
    if (m_CurrCmdCtx)
    {
        VERIFY(!m_bIsDeferred, "Deferred contexts cannot execute command lists directly");
        if (m_State.NumCommands != 0)
        {
            m_CurrCmdCtx->FlushResourceBarriers();
            m_pDevice->CloseAndExecuteCommandContext(m_CommandQueueId, std::move(m_CurrCmdCtx), true, &m_PendingFences);
            m_PendingFences.clear();
        }
        else
            m_pDevice->DisposeCommandContext(std::move(m_CurrCmdCtx));
    }

    if (m_ActiveQueriesCounter > 0)
    {
        LOG_ERROR_MESSAGE("Flushing device context that has ", m_ActiveQueriesCounter,
                          " active queries. Direct3D12 requires that queries are begun and ended in the same command list");
    }

    // If there is no command list to submit, but there are pending fences, we need to signal them now
    if (!m_PendingFences.empty())
    {
        m_pDevice->SignalFences(m_CommandQueueId, m_PendingFences);
        m_PendingFences.clear();
    }

    if (RequestNewCmdCtx)
        RequestCommandContext(m_pDevice);

    m_State = State{};

    // Setting pipeline state to null makes sure that render targets and other
    // states will be restored in the command list next time a PSO is bound.
    m_pPipelineState = nullptr;
}

void DeviceContextD3D12Impl::Flush()
{
    if (m_bIsDeferred)
    {
        LOG_ERROR_MESSAGE("Flush() should only be called for immediate contexts");
        return;
    }

    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Flushing device context inside an active render pass.");
    }

    Flush(true);
}

void DeviceContextD3D12Impl::FinishFrame()
{
#ifdef DILIGENT_DEBUG
    for (const auto& MappedBuffIt : m_DbgMappedBuffers)
    {
        const auto& BuffDesc = MappedBuffIt.first->GetDesc();
        if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            LOG_WARNING_MESSAGE("Dynamic buffer '", BuffDesc.Name,
                                "' is still mapped when finishing the frame. The contents of the buffer and "
                                "mapped address will become invalid");
        }
    }
#endif
    if (GetNumCommandsInCtx() != 0)
    {
        if (m_bIsDeferred)
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in deferred device context #",
                              m_ContextId,
                              " when finishing the frame. This is an error and may cause unpredicted behaviour. "
                              "Close all deferred contexts and execute them before finishing the frame");
        }
        else
        {
            LOG_ERROR_MESSAGE("There are outstanding commands in the immediate device context when finishing the frame. "
                              "This is an error and may cause unpredicted behaviour. Call Flush() to submit all commands"
                              " for execution before finishing the frame");
        }
    }

    if (m_ActiveQueriesCounter > 0)
    {
        LOG_ERROR_MESSAGE("There are ", m_ActiveQueriesCounter,
                          " active queries in the device context when finishing the frame. "
                          "All queries must be ended before the frame is finished.");
    }

    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Finishing frame inside an active render pass.");
    }

    VERIFY_EXPR(m_bIsDeferred || m_SubmittedBuffersCmdQueueMask == (Uint64{1} << m_CommandQueueId));

    // Released pages are returned to the global dynamic memory manager hosted by render device.
    m_DynamicHeap.ReleaseAllocatedPages(m_SubmittedBuffersCmdQueueMask);

    // Dynamic GPU descriptor allocations are returned to the global GPU descriptor heap
    // hosted by the render device.
    for (size_t i = 0; i < _countof(m_DynamicGPUDescriptorAllocator); ++i)
        m_DynamicGPUDescriptorAllocator[i].ReleaseAllocations(m_SubmittedBuffersCmdQueueMask);

    EndFrame();
}

void DeviceContextD3D12Impl::SetVertexBuffers(Uint32                         StartSlot,
                                              Uint32                         NumBuffersSet,
                                              IBuffer**                      ppBuffers,
                                              Uint32*                        pOffsets,
                                              RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                              SET_VERTEX_BUFFERS_FLAGS       Flags)
{
    TDeviceContextBase::SetVertexBuffers(StartSlot, NumBuffersSet, ppBuffers, pOffsets, StateTransitionMode, Flags);

    auto& CmdCtx = GetCmdContext();
    for (Uint32 Buff = 0; Buff < m_NumVertexStreams; ++Buff)
    {
        auto& CurrStream = m_VertexStreams[Buff];
        if (auto* pBufferD3D12 = CurrStream.pBuffer.RawPtr())
            TransitionOrVerifyBufferState(CmdCtx, *pBufferD3D12, StateTransitionMode, RESOURCE_STATE_VERTEX_BUFFER, "Setting vertex buffers (DeviceContextD3D12Impl::SetVertexBuffers)");
    }

    m_State.bCommittedD3D12VBsUpToDate = false;
}

void DeviceContextD3D12Impl::InvalidateState()
{
    if (m_State.NumCommands != 0)
        LOG_WARNING_MESSAGE("Invalidating context that has outstanding commands in it. Call Flush() to submit commands for execution");

    TDeviceContextBase::InvalidateState();
    m_State = State{};
}

void DeviceContextD3D12Impl::SetIndexBuffer(IBuffer* pIndexBuffer, Uint32 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::SetIndexBuffer(pIndexBuffer, ByteOffset, StateTransitionMode);
    if (m_pIndexBuffer)
    {
        auto& CmdCtx = GetCmdContext();
        TransitionOrVerifyBufferState(CmdCtx, *m_pIndexBuffer, StateTransitionMode, RESOURCE_STATE_INDEX_BUFFER, "Setting index buffer (DeviceContextD3D12Impl::SetIndexBuffer)");
    }
    m_State.bCommittedD3D12IBUpToDate = false;
}

void DeviceContextD3D12Impl::CommitViewports()
{
    static_assert(MAX_VIEWPORTS >= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "MaxViewports constant must be greater than D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE");
    D3D12_VIEWPORT d3d12Viewports[MAX_VIEWPORTS]; // Do not waste time initializing array to zero

    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
    {
        d3d12Viewports[vp].TopLeftX = m_Viewports[vp].TopLeftX;
        d3d12Viewports[vp].TopLeftY = m_Viewports[vp].TopLeftY;
        d3d12Viewports[vp].Width    = m_Viewports[vp].Width;
        d3d12Viewports[vp].Height   = m_Viewports[vp].Height;
        d3d12Viewports[vp].MinDepth = m_Viewports[vp].MinDepth;
        d3d12Viewports[vp].MaxDepth = m_Viewports[vp].MaxDepth;
    }
    // All viewports must be set atomically as one operation.
    // Any viewports not defined by the call are disabled.
    GetCmdContext().AsGraphicsContext().SetViewports(m_NumViewports, d3d12Viewports);
}

void DeviceContextD3D12Impl::SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32 RTWidth, Uint32 RTHeight)
{
    static_assert(MAX_VIEWPORTS >= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "MaxViewports constant must be greater than D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE");
    TDeviceContextBase::SetViewports(NumViewports, pViewports, RTWidth, RTHeight);
    VERIFY(NumViewports == m_NumViewports, "Unexpected number of viewports");

    CommitViewports();
}


constexpr LONG   MaxD3D12TexDim       = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
constexpr Uint32 MaxD3D12ScissorRects = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
// clang-format off
static constexpr RECT MaxD3D12TexSizeRects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] =
{
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},

    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},

    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},

    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim},
    {0, 0, MaxD3D12TexDim, MaxD3D12TexDim}
};
// clang-format on

void DeviceContextD3D12Impl::CommitScissorRects(GraphicsContext& GraphCtx, bool ScissorEnable)
{
    if (ScissorEnable)
    {
        // Commit currently set scissor rectangles
        D3D12_RECT d3d12ScissorRects[MaxD3D12ScissorRects]; // Do not waste time initializing array with zeroes
        for (Uint32 sr = 0; sr < m_NumScissorRects; ++sr)
        {
            d3d12ScissorRects[sr].left   = m_ScissorRects[sr].left;
            d3d12ScissorRects[sr].top    = m_ScissorRects[sr].top;
            d3d12ScissorRects[sr].right  = m_ScissorRects[sr].right;
            d3d12ScissorRects[sr].bottom = m_ScissorRects[sr].bottom;
        }
        GraphCtx.SetScissorRects(m_NumScissorRects, d3d12ScissorRects);
    }
    else
    {
        // Disable scissor rectangles
        static_assert(_countof(MaxD3D12TexSizeRects) == MaxD3D12ScissorRects, "Unexpected array size");
        GraphCtx.SetScissorRects(MaxD3D12ScissorRects, MaxD3D12TexSizeRects);
    }
}

void DeviceContextD3D12Impl::SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32 RTWidth, Uint32 RTHeight)
{
    const Uint32 MaxScissorRects = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    VERIFY(NumRects < MaxScissorRects, "Too many scissor rects are being set");
    NumRects = std::min(NumRects, MaxScissorRects);

    TDeviceContextBase::SetScissorRects(NumRects, pRects, RTWidth, RTHeight);

    // Only commit scissor rects if scissor test is enabled in the rasterizer state.
    // If scissor is currently disabled, or no PSO is bound, scissor rects will be committed by
    // the SetPipelineState() when a PSO with enabled scissor test is set.
    if (m_pPipelineState)
    {
        const auto& PSODesc = m_pPipelineState->GetDesc();
        if (PSODesc.IsAnyGraphicsPipeline() && m_pPipelineState->GetGraphicsPipelineDesc().RasterizerDesc.ScissorEnable)
        {
            VERIFY(NumRects == m_NumScissorRects, "Unexpected number of scissor rects");
            auto& Ctx = GetCmdContext().AsGraphicsContext();
            CommitScissorRects(Ctx, true);
        }
    }
}

void DeviceContextD3D12Impl::CommitRenderTargets(RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    VERIFY(m_pActiveRenderPass == nullptr, "This method must not be called inside a render pass");

    const Uint32 MaxD3D12RTs      = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    Uint32       NumRenderTargets = m_NumBoundRenderTargets;
    VERIFY(NumRenderTargets <= MaxD3D12RTs, "D3D12 only allows 8 simultaneous render targets");
    NumRenderTargets = std::min(MaxD3D12RTs, NumRenderTargets);

    ITextureViewD3D12* ppRTVs[MaxD3D12RTs]; // Do not initialize with zeroes!
    ITextureViewD3D12* pDSV = nullptr;

    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        ppRTVs[rt] = m_pBoundRenderTargets[rt].RawPtr();
    pDSV = m_pBoundDepthStencil.RawPtr();

    auto& CmdCtx = GetCmdContext();

    D3D12_CPU_DESCRIPTOR_HANDLE RTVHandles[8]; // Do not waste time initializing array to zero
    D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};
    for (UINT i = 0; i < NumRenderTargets; ++i)
    {
        if (auto* pRTV = ppRTVs[i])
        {
            auto* pTexture = ValidatedCast<TextureD3D12Impl>(pRTV->GetTexture());
            TransitionOrVerifyTextureState(CmdCtx, *pTexture, StateTransitionMode, RESOURCE_STATE_RENDER_TARGET, "Setting render targets (DeviceContextD3D12Impl::CommitRenderTargets)");
            RTVHandles[i] = pRTV->GetCPUDescriptorHandle();
            VERIFY_EXPR(RTVHandles[i].ptr != 0);
        }
    }

    if (pDSV)
    {
        auto* pTexture = ValidatedCast<TextureD3D12Impl>(pDSV->GetTexture());
        //if (bReadOnlyDepth)
        //{
        //	TransitionResource(*pTexture, D3D12_RESOURCE_STATE_DEPTH_READ);
        //	m_pCommandList->OMSetRenderTargets( NumRTVs, RTVHandles, FALSE, &DSV->GetDSV_DepthReadOnly() );
        //}
        //else
        {
            TransitionOrVerifyTextureState(CmdCtx, *pTexture, StateTransitionMode, RESOURCE_STATE_DEPTH_WRITE, "Setting depth-stencil buffer (DeviceContextD3D12Impl::CommitRenderTargets)");
            DSVHandle = pDSV->GetCPUDescriptorHandle();
            VERIFY_EXPR(DSVHandle.ptr != 0);
        }
    }

    if (NumRenderTargets > 0 || DSVHandle.ptr != 0)
    {
        // No need to flush resource barriers as this is a CPU-side command
        CmdCtx.AsGraphicsContext().GetCommandList()->OMSetRenderTargets(NumRenderTargets, RTVHandles, FALSE, DSVHandle.ptr != 0 ? &DSVHandle : nullptr);
    }
}

void DeviceContextD3D12Impl::SetRenderTargets(Uint32                         NumRenderTargets,
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
        CommitRenderTargets(StateTransitionMode);

        // Set the viewport to match the render target size
        SetViewports(1, nullptr, 0, 0);
    }
}

void DeviceContextD3D12Impl::TransitionSubpassAttachments(Uint32 NextSubpass)
{
    VERIFY_EXPR(m_pActiveRenderPass);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& FBDesc = m_pBoundFramebuffer->GetDesc();
    VERIFY_EXPR(RPDesc.AttachmentCount == FBDesc.AttachmentCount);
    for (Uint32 att = 0; att < RPDesc.AttachmentCount; ++att)
    {
        const auto& AttDesc  = RPDesc.pAttachments[att];
        auto        OldState = NextSubpass > 0 ? m_pActiveRenderPass->GetAttachmentState(NextSubpass - 1, att) : AttDesc.InitialState;
        auto        NewState = NextSubpass < RPDesc.SubpassCount ? m_pActiveRenderPass->GetAttachmentState(NextSubpass, att) : AttDesc.FinalState;
        if (OldState != NewState)
        {
            auto& CmdCtx = GetCmdContext();

            auto* pViewD3D12 = ValidatedCast<TextureViewD3D12Impl>(FBDesc.ppAttachments[att]);
            if (pViewD3D12 == nullptr)
                continue;

            auto* pTexD3D12 = pViewD3D12->GetTexture<TextureD3D12Impl>();

            const auto& ViewDesc = pViewD3D12->GetDesc();
            const auto& TexDesc  = pTexD3D12->GetDesc();

            D3D12_RESOURCE_BARRIER BarrierDesc;
            BarrierDesc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            BarrierDesc.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            BarrierDesc.Transition.pResource   = pTexD3D12->GetD3D12Resource();
            BarrierDesc.Transition.StateBefore = ResourceStateFlagsToD3D12ResourceStates(OldState);
            BarrierDesc.Transition.StateAfter  = ResourceStateFlagsToD3D12ResourceStates(NewState);
            for (Uint32 mip = ViewDesc.MostDetailedMip; mip < ViewDesc.MostDetailedMip + ViewDesc.NumDepthSlices; ++mip)
            {
                for (Uint32 slice = ViewDesc.FirstArraySlice; slice < ViewDesc.FirstArraySlice + ViewDesc.NumArraySlices; ++slice)
                {
                    BarrierDesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, TexDesc.MipLevels, TexDesc.ArraySize);
                    CmdCtx.ResourceBarrier(BarrierDesc);
                }
            }
        }
    }
}

void DeviceContextD3D12Impl::CommitSubpassRenderTargets()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_pBoundFramebuffer);
    const auto& FBDesc = m_pBoundFramebuffer->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& Subpass = RPDesc.pSubpasses[m_SubpassIndex];
    VERIFY(Subpass.RenderTargetAttachmentCount == m_NumBoundRenderTargets,
           "The number of currently bound render targets (", m_NumBoundRenderTargets,
           ") is not consistent with the number of redner target attachments (", Subpass.RenderTargetAttachmentCount,
           ") in current subpass");

    D3D12_RENDER_PASS_RENDER_TARGET_DESC RenderPassRTs[MAX_RENDER_TARGETS];
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        const auto& RTRef = Subpass.pRenderTargetAttachments[rt];
        if (RTRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            TextureViewD3D12Impl* pRTV = m_pBoundRenderTargets[rt];
            VERIFY(pRTV == FBDesc.ppAttachments[RTRef.AttachmentIndex],
                   "Render target bound in the device context at slot ", rt, " is not consistent with the corresponding framebuffer attachment");
            const auto  FirstLastUse     = m_pActiveRenderPass->GetAttachmentFirstLastUse(RTRef.AttachmentIndex);
            const auto& RTAttachmentDesc = RPDesc.pAttachments[RTRef.AttachmentIndex];

            auto& RPRT = RenderPassRTs[rt];
            RPRT       = D3D12_RENDER_PASS_RENDER_TARGET_DESC{};

            RPRT.cpuDescriptor = pRTV->GetCPUDescriptorHandle();
            if (FirstLastUse.first == m_SubpassIndex)
            {
                // This is the first use of this attachment - use LoadOp
                RPRT.BeginningAccess.Type = AttachmentLoadOpToD3D12BeginningAccessType(RTAttachmentDesc.LoadOp);
            }
            else
            {
                // Preserve the attachment contents
                RPRT.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            }

            if (RPRT.BeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
            {
                RPRT.BeginningAccess.Clear.ClearValue.Format = TexFormatToDXGI_Format(RTAttachmentDesc.Format);

                const auto ClearColor = m_AttachmentClearValues[RTRef.AttachmentIndex].Color;
                for (Uint32 i = 0; i < 4; ++i)
                    RPRT.BeginningAccess.Clear.ClearValue.Color[i] = ClearColor[i];
            }

            if (FirstLastUse.second == m_SubpassIndex)
            {
                // This is the last use of this attachment - use StoreOp or resolve parameters
                if (Subpass.pResolveAttachments != nullptr && Subpass.pResolveAttachments[rt].AttachmentIndex != ATTACHMENT_UNUSED)
                {
                    VERIFY_EXPR(Subpass.pResolveAttachments[rt].AttachmentIndex < RPDesc.AttachmentCount);
                    auto* pDstView     = FBDesc.ppAttachments[Subpass.pResolveAttachments[rt].AttachmentIndex];
                    auto* pSrcTexD3D12 = pRTV->GetTexture<TextureD3D12Impl>();
                    auto* pDstTexD3D12 = ValidatedCast<TextureViewD3D12Impl>(pDstView)->GetTexture<TextureD3D12Impl>();

                    const auto& SrcRTVDesc  = pRTV->GetDesc();
                    const auto& DstViewDesc = pDstView->GetDesc();
                    const auto& SrcTexDesc  = pSrcTexD3D12->GetDesc();
                    const auto& DstTexDesc  = pDstTexD3D12->GetDesc();

                    VERIFY_EXPR(SrcRTVDesc.NumArraySlices == 1);
                    Uint32 SubresourceCount = SrcRTVDesc.NumArraySlices;
                    m_AttachmentResolveInfo.resize(SubresourceCount);
                    const auto MipProps = GetMipLevelProperties(SrcTexDesc, SrcRTVDesc.MostDetailedMip);
                    for (Uint32 slice = 0; slice < SrcRTVDesc.NumArraySlices; ++slice)
                    {
                        auto& ARI = m_AttachmentResolveInfo[slice];

                        ARI.SrcSubresource = D3D12CalcSubresource(SrcRTVDesc.MostDetailedMip, SrcRTVDesc.FirstArraySlice + slice, 0, SrcTexDesc.MipLevels, SrcTexDesc.ArraySize);
                        ARI.DstSubresource = D3D12CalcSubresource(DstViewDesc.MostDetailedMip, DstViewDesc.FirstArraySlice + slice, 0, DstTexDesc.MipLevels, DstTexDesc.ArraySize);
                        ARI.DstX           = 0;
                        ARI.DstY           = 0;
                        ARI.SrcRect.left   = 0;
                        ARI.SrcRect.top    = 0;
                        ARI.SrcRect.right  = MipProps.LogicalWidth;
                        ARI.SrcRect.bottom = MipProps.LogicalHeight;
                    }

                    // The resolve source is left in its initial resource state at the time the render pass ends.
                    // A resolve operation submitted by a render pass doesn't implicitly change the state of any resource.
                    // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_render_pass_ending_access_type
                    RPRT.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;

                    auto& ResolveParams            = RPRT.EndingAccess.Resolve;
                    ResolveParams.pSrcResource     = pSrcTexD3D12->GetD3D12Resource();
                    ResolveParams.pDstResource     = pDstTexD3D12->GetD3D12Resource();
                    ResolveParams.SubresourceCount = SubresourceCount;
                    // This pointer is directly referenced by the command list, and the memory for this array
                    // must remain alive and intact until EndRenderPass is called.
                    ResolveParams.pSubresourceParameters = m_AttachmentResolveInfo.data();
                    ResolveParams.Format                 = TexFormatToDXGI_Format(RTAttachmentDesc.Format);
                    ResolveParams.ResolveMode            = D3D12_RESOLVE_MODE_AVERAGE;
                    ResolveParams.PreserveResolveSource  = RTAttachmentDesc.StoreOp == ATTACHMENT_STORE_OP_STORE;
                }
                else
                {
                    RPRT.EndingAccess.Type = AttachmentStoreOpToD3D12EndingAccessType(RTAttachmentDesc.StoreOp);
                }
            }
            else
            {
                // The attachment will be used in subsequent subpasses - preserve its contents
                RPRT.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            }
        }
        else
        {
            // Attachment is not used
            RenderPassRTs[rt].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
            RenderPassRTs[rt].EndingAccess.Type    = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
            continue;
        }
    }

    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC RenderPassDS;
    if (m_pBoundDepthStencil)
    {
        RenderPassDS = D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{};

        const auto& DSAttachmentRef = *Subpass.pDepthStencilAttachment;
        VERIFY_EXPR(Subpass.pDepthStencilAttachment != nullptr && DSAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED);
        VERIFY(m_pBoundDepthStencil == FBDesc.ppAttachments[DSAttachmentRef.AttachmentIndex],
               "Depth-stencil bufer in the device context is inconsistent with the framebuffer");
        const auto  FirstLastUse     = m_pActiveRenderPass->GetAttachmentFirstLastUse(DSAttachmentRef.AttachmentIndex);
        const auto& DSAttachmentDesc = RPDesc.pAttachments[DSAttachmentRef.AttachmentIndex];

        RenderPassDS.cpuDescriptor = m_pBoundDepthStencil->GetCPUDescriptorHandle();
        if (FirstLastUse.first == m_SubpassIndex)
        {
            RenderPassDS.DepthBeginningAccess.Type   = AttachmentLoadOpToD3D12BeginningAccessType(DSAttachmentDesc.LoadOp);
            RenderPassDS.StencilBeginningAccess.Type = AttachmentLoadOpToD3D12BeginningAccessType(DSAttachmentDesc.StencilLoadOp);
        }
        else
        {
            RenderPassDS.DepthBeginningAccess.Type   = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            RenderPassDS.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        }

        if (RenderPassDS.DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
        {
            RenderPassDS.DepthBeginningAccess.Clear.ClearValue.Format = TexFormatToDXGI_Format(DSAttachmentDesc.Format);
            RenderPassDS.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth =
                m_AttachmentClearValues[DSAttachmentRef.AttachmentIndex].DepthStencil.Depth;
        }

        if (RenderPassDS.StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
        {
            RenderPassDS.StencilBeginningAccess.Clear.ClearValue.Format = TexFormatToDXGI_Format(DSAttachmentDesc.Format);
            RenderPassDS.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil =
                m_AttachmentClearValues[DSAttachmentRef.AttachmentIndex].DepthStencil.Stencil;
        }

        if (FirstLastUse.second == m_SubpassIndex)
        {
            RenderPassDS.DepthEndingAccess.Type   = AttachmentStoreOpToD3D12EndingAccessType(DSAttachmentDesc.StoreOp);
            RenderPassDS.StencilEndingAccess.Type = AttachmentStoreOpToD3D12EndingAccessType(DSAttachmentDesc.StencilStoreOp);
        }
        else
        {
            RenderPassDS.DepthEndingAccess.Type   = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            RenderPassDS.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }
    }

    auto& CmdCtx = GetCmdContext();
    CmdCtx.AsGraphicsContext4().BeginRenderPass(
        Subpass.RenderTargetAttachmentCount,
        RenderPassRTs,
        m_pBoundDepthStencil ? &RenderPassDS : nullptr,
        D3D12_RENDER_PASS_FLAG_NONE);

    // Set the viewport to match the framebuffer size
    SetViewports(1, nullptr, 0, 0);
}

void DeviceContextD3D12Impl::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    TDeviceContextBase::BeginRenderPass(Attribs);

    m_AttachmentClearValues.resize(Attribs.ClearValueCount);
    for (Uint32 i = 0; i < Attribs.ClearValueCount; ++i)
        m_AttachmentClearValues[i] = Attribs.pClearValues[i];

    TransitionSubpassAttachments(m_SubpassIndex);
    CommitSubpassRenderTargets();
}

void DeviceContextD3D12Impl::NextSubpass()
{
    auto& CmdCtx = GetCmdContext();
    CmdCtx.AsGraphicsContext4().EndRenderPass();
    TDeviceContextBase::NextSubpass();
    TransitionSubpassAttachments(m_SubpassIndex);
    CommitSubpassRenderTargets();
}

void DeviceContextD3D12Impl::EndRenderPass()
{
    auto& CmdCtx = GetCmdContext();
    CmdCtx.AsGraphicsContext4().EndRenderPass();
    TransitionSubpassAttachments(m_SubpassIndex + 1);
    TDeviceContextBase::EndRenderPass();
}

D3D12DynamicAllocation DeviceContextD3D12Impl::AllocateDynamicSpace(size_t NumBytes, size_t Alignment)
{
    return m_DynamicHeap.Allocate(NumBytes, Alignment, m_ContextFrameNumber);
}

void DeviceContextD3D12Impl::UpdateBufferRegion(BufferD3D12Impl*               pBuffD3D12,
                                                D3D12DynamicAllocation&        Allocation,
                                                Uint64                         DstOffset,
                                                Uint64                         NumBytes,
                                                RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    auto& CmdCtx = GetCmdContext();
    VERIFY_EXPR(static_cast<size_t>(NumBytes) == NumBytes);
    TransitionOrVerifyBufferState(CmdCtx, *pBuffD3D12, StateTransitionMode, RESOURCE_STATE_COPY_DEST, "Updating buffer (DeviceContextD3D12Impl::UpdateBufferRegion)");
    Uint64 DstBuffDataStartByteOffset;
    auto*  pd3d12Buff = pBuffD3D12->GetD3D12Buffer(DstBuffDataStartByteOffset, this);
    VERIFY(DstBuffDataStartByteOffset == 0, "Dst buffer must not be suballocated");
    CmdCtx.FlushResourceBarriers();
    CmdCtx.GetCommandList()->CopyBufferRegion(pd3d12Buff, DstOffset + DstBuffDataStartByteOffset, Allocation.pBuffer, Allocation.Offset, NumBytes);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::UpdateBuffer(IBuffer*                       pBuffer,
                                          Uint32                         Offset,
                                          Uint32                         Size,
                                          const void*                    pData,
                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::UpdateBuffer(pBuffer, Offset, Size, pData, StateTransitionMode);

    // We must use cmd context from the device context provided, otherwise there will
    // be resource barrier issues in the cmd list in the device context
    auto* pBuffD3D12 = ValidatedCast<BufferD3D12Impl>(pBuffer);
    VERIFY(pBuffD3D12->GetDesc().Usage != USAGE_DYNAMIC, "Dynamic buffers must be updated via Map()");
    constexpr size_t DefaultAlginment = 16;
    auto             TmpSpace         = m_DynamicHeap.Allocate(Size, DefaultAlginment, m_ContextFrameNumber);
    memcpy(TmpSpace.CPUAddress, pData, Size);
    UpdateBufferRegion(pBuffD3D12, TmpSpace, Offset, Size, StateTransitionMode);
}

void DeviceContextD3D12Impl::CopyBuffer(IBuffer*                       pSrcBuffer,
                                        Uint32                         SrcOffset,
                                        RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                        IBuffer*                       pDstBuffer,
                                        Uint32                         DstOffset,
                                        Uint32                         Size,
                                        RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    TDeviceContextBase::CopyBuffer(pSrcBuffer, SrcOffset, SrcBufferTransitionMode, pDstBuffer, DstOffset, Size, DstBufferTransitionMode);

    auto* pSrcBuffD3D12 = ValidatedCast<BufferD3D12Impl>(pSrcBuffer);
    auto* pDstBuffD3D12 = ValidatedCast<BufferD3D12Impl>(pDstBuffer);

    VERIFY(pDstBuffD3D12->GetDesc().Usage != USAGE_DYNAMIC, "Dynamic buffers cannot be copy destinations");

    auto& CmdCtx = GetCmdContext();
    TransitionOrVerifyBufferState(CmdCtx, *pSrcBuffD3D12, SrcBufferTransitionMode, RESOURCE_STATE_COPY_SOURCE, "Using resource as copy source (DeviceContextD3D12Impl::CopyBuffer)");
    TransitionOrVerifyBufferState(CmdCtx, *pDstBuffD3D12, DstBufferTransitionMode, RESOURCE_STATE_COPY_DEST, "Using resource as copy destination (DeviceContextD3D12Impl::CopyBuffer)");

    Uint64 DstDataStartByteOffset;
    auto*  pd3d12DstBuff = pDstBuffD3D12->GetD3D12Buffer(DstDataStartByteOffset, this);
    VERIFY(DstDataStartByteOffset == 0, "Dst buffer must not be suballocated");

    Uint64 SrcDataStartByteOffset;
    auto*  pd3d12SrcBuff = pSrcBuffD3D12->GetD3D12Buffer(SrcDataStartByteOffset, this);
    CmdCtx.FlushResourceBarriers();
    CmdCtx.GetCommandList()->CopyBufferRegion(pd3d12DstBuff, DstOffset + DstDataStartByteOffset, pd3d12SrcBuff, SrcOffset + SrcDataStartByteOffset, Size);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData)
{
    TDeviceContextBase::MapBuffer(pBuffer, MapType, MapFlags, pMappedData);
    auto*       pBufferD3D12   = ValidatedCast<BufferD3D12Impl>(pBuffer);
    const auto& BuffDesc       = pBufferD3D12->GetDesc();
    auto*       pd3d12Resource = pBufferD3D12->m_pd3d12Resource.p;

    if (MapType == MAP_READ)
    {
        DEV_CHECK_ERR(BuffDesc.Usage == USAGE_STAGING, "Buffer must be created as USAGE_STAGING to be mapped for reading");
        DEV_CHECK_ERR(pd3d12Resource != nullptr, "USAGE_STAGING buffer must intialize D3D12 resource");

        if ((MapFlags & MAP_FLAG_DO_NOT_WAIT) == 0)
        {
            LOG_WARNING_MESSAGE("D3D12 backend never waits for GPU when mapping staging buffers for reading. "
                                "Applications must use fences or other synchronization methods to explicitly synchronize "
                                "access and use MAP_FLAG_DO_NOT_WAIT flag.");
        }

        D3D12_RANGE MapRange;
        MapRange.Begin = 0;
        MapRange.End   = BuffDesc.uiSizeInBytes;
        pd3d12Resource->Map(0, &MapRange, &pMappedData);
    }
    else if (MapType == MAP_WRITE)
    {
        if (BuffDesc.Usage == USAGE_STAGING)
        {
            DEV_CHECK_ERR(pd3d12Resource != nullptr, "USAGE_STAGING buffer mapped for writing must intialize D3D12 resource");
            if (MapFlags & MAP_FLAG_DISCARD)
            {
            }
            pd3d12Resource->Map(0, nullptr, &pMappedData);
        }
        else if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            DEV_CHECK_ERR((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0, "D3D12 buffer must be mapped for writing with MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE flag");
            auto& DynamicData = pBufferD3D12->m_DynamicData[m_ContextId];
            if ((MapFlags & MAP_FLAG_DISCARD) != 0 || DynamicData.CPUAddress == nullptr)
            {
                size_t Alignment = (BuffDesc.BindFlags & BIND_UNIFORM_BUFFER) ? D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT : 16;
                DynamicData      = AllocateDynamicSpace(BuffDesc.uiSizeInBytes, Alignment);
            }
            else
            {
                VERIFY_EXPR(MapFlags & MAP_FLAG_NO_OVERWRITE);

                if (pd3d12Resource != nullptr)
                {
                    LOG_ERROR("Formatted or structured buffers require actual Direct3D12 backing resource and cannot be suballocated "
                              "from dynamic heap. In current implementation, the entire contents of the backing buffer is updated when the buffer is unmapped. "
                              "As a consequence, the buffer cannot be mapped with MAP_FLAG_NO_OVERWRITE flag because updating the whole "
                              "buffer will overwrite regions that may still be in use by the GPU.");
                    return;
                }

                // Reuse previously mapped region
            }
            pMappedData = DynamicData.CPUAddress;
        }
        else
        {
            LOG_ERROR("Only USAGE_DYNAMIC and USAGE_STAGING D3D12 buffers can be mapped for writing");
        }
    }
    else if (MapType == MAP_READ_WRITE)
    {
        LOG_ERROR("MAP_READ_WRITE is not supported in D3D12");
    }
    else
    {
        LOG_ERROR("Only MAP_WRITE_DISCARD and MAP_READ are currently implemented in D3D12");
    }
}

void DeviceContextD3D12Impl::UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    TDeviceContextBase::UnmapBuffer(pBuffer, MapType);
    auto*       pBufferD3D12   = ValidatedCast<BufferD3D12Impl>(pBuffer);
    const auto& BuffDesc       = pBufferD3D12->GetDesc();
    auto*       pd3d12Resource = pBufferD3D12->m_pd3d12Resource.p;
    if (MapType == MAP_READ)
    {
        D3D12_RANGE MapRange;
        // It is valid to specify the CPU didn't write any data by passing a range where End is less than or equal to Begin.
        MapRange.Begin = 1;
        MapRange.End   = 0;
        pd3d12Resource->Unmap(0, &MapRange);
    }
    else if (MapType == MAP_WRITE)
    {
        if (BuffDesc.Usage == USAGE_STAGING)
        {
            VERIFY(pd3d12Resource != nullptr, "USAGE_STAGING buffer mapped for writing must intialize D3D12 resource");
            pd3d12Resource->Unmap(0, nullptr);
        }
        else if (BuffDesc.Usage == USAGE_DYNAMIC)
        {
            // Copy data into the resource
            if (pd3d12Resource)
            {
                UpdateBufferRegion(pBufferD3D12, pBufferD3D12->m_DynamicData[m_ContextId], 0, BuffDesc.uiSizeInBytes, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }
        }
    }
}

void DeviceContextD3D12Impl::UpdateTexture(ITexture*                      pTexture,
                                           Uint32                         MipLevel,
                                           Uint32                         Slice,
                                           const Box&                     DstBox,
                                           const TextureSubResData&       SubresData,
                                           RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                           RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    TDeviceContextBase::UpdateTexture(pTexture, MipLevel, Slice, DstBox, SubresData, SrcBufferTransitionMode, TextureTransitionMode);

    auto*       pTexD3D12 = ValidatedCast<TextureD3D12Impl>(pTexture);
    const auto& Desc      = pTexD3D12->GetDesc();
    // OpenGL backend uses UpdateData() to initialize textures, so we can't check the usage in ValidateUpdateTextureParams()
    DEV_CHECK_ERR(Desc.Usage == USAGE_DEFAULT, "Only USAGE_DEFAULT textures should be updated with UpdateData()");

    Box         BlockAlignedBox;
    const auto& FmtAttribs = GetTextureFormatAttribs(Desc.Format);
    const Box*  pBox       = nullptr;
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Align update region by the compressed block size

        VERIFY((DstBox.MinX % FmtAttribs.BlockWidth) == 0, "Update region min X coordinate (", DstBox.MinX, ") must be multiple of a compressed block width (", Uint32{FmtAttribs.BlockWidth}, ")");
        BlockAlignedBox.MinX = DstBox.MinX;
        VERIFY((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0, "Compressed block width (", Uint32{FmtAttribs.BlockWidth}, ") is expected to be power of 2");
        BlockAlignedBox.MaxX = (DstBox.MaxX + FmtAttribs.BlockWidth - 1) & ~(FmtAttribs.BlockWidth - 1);

        VERIFY((DstBox.MinY % FmtAttribs.BlockHeight) == 0, "Update region min Y coordinate (", DstBox.MinY, ") must be multiple of a compressed block height (", Uint32{FmtAttribs.BlockHeight}, ")");
        BlockAlignedBox.MinY = DstBox.MinY;
        VERIFY((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0, "Compressed block height (", Uint32{FmtAttribs.BlockHeight}, ") is expected to be power of 2");
        BlockAlignedBox.MaxY = (DstBox.MaxY + FmtAttribs.BlockHeight - 1) & ~(FmtAttribs.BlockHeight - 1);

        BlockAlignedBox.MinZ = DstBox.MinZ;
        BlockAlignedBox.MaxZ = DstBox.MaxZ;

        pBox = &BlockAlignedBox;
    }
    else
    {
        pBox = &DstBox;
    }
    auto DstSubResIndex = D3D12CalcSubresource(MipLevel, Slice, 0, Desc.MipLevels, Desc.ArraySize);
    if (SubresData.pSrcBuffer == nullptr)
    {
        UpdateTextureRegion(SubresData.pData, SubresData.Stride, SubresData.DepthStride,
                            *pTexD3D12, DstSubResIndex, *pBox, TextureTransitionMode);
    }
    else
    {
        CopyTextureRegion(SubresData.pSrcBuffer, 0, SubresData.Stride, SubresData.DepthStride,
                          *pTexD3D12, DstSubResIndex, *pBox,
                          SrcBufferTransitionMode, TextureTransitionMode);
    }
}

void DeviceContextD3D12Impl::CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    TDeviceContextBase::CopyTexture(CopyAttribs);

    auto* pSrcTexD3D12 = ValidatedCast<TextureD3D12Impl>(CopyAttribs.pSrcTexture);
    auto* pDstTexD3D12 = ValidatedCast<TextureD3D12Impl>(CopyAttribs.pDstTexture);

    const auto& SrcTexDesc = pSrcTexD3D12->GetDesc();
    const auto& DstTexDesc = pDstTexD3D12->GetDesc();

    D3D12_BOX D3D12SrcBox, *pD3D12SrcBox = nullptr;
    if (const auto* pSrcBox = CopyAttribs.pSrcBox)
    {
        D3D12SrcBox.left   = pSrcBox->MinX;
        D3D12SrcBox.right  = pSrcBox->MaxX;
        D3D12SrcBox.top    = pSrcBox->MinY;
        D3D12SrcBox.bottom = pSrcBox->MaxY;
        D3D12SrcBox.front  = pSrcBox->MinZ;
        D3D12SrcBox.back   = pSrcBox->MaxZ;
        pD3D12SrcBox       = &D3D12SrcBox;
    }

    auto DstSubResIndex = D3D12CalcSubresource(CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, 0, DstTexDesc.MipLevels, DstTexDesc.ArraySize);
    auto SrcSubResIndex = D3D12CalcSubresource(CopyAttribs.SrcMipLevel, CopyAttribs.SrcSlice, 0, SrcTexDesc.MipLevels, SrcTexDesc.ArraySize);
    CopyTextureRegion(pSrcTexD3D12, SrcSubResIndex, pD3D12SrcBox, CopyAttribs.SrcTextureTransitionMode,
                      pDstTexD3D12, DstSubResIndex, CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ,
                      CopyAttribs.DstTextureTransitionMode);
}

void DeviceContextD3D12Impl::CopyTextureRegion(TextureD3D12Impl*              pSrcTexture,
                                               Uint32                         SrcSubResIndex,
                                               const D3D12_BOX*               pD3D12SrcBox,
                                               RESOURCE_STATE_TRANSITION_MODE SrcTextureTransitionMode,
                                               TextureD3D12Impl*              pDstTexture,
                                               Uint32                         DstSubResIndex,
                                               Uint32                         DstX,
                                               Uint32                         DstY,
                                               Uint32                         DstZ,
                                               RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode)
{
    // We must unbind the textures from framebuffer because
    // we will transition their states. If we later try to commit
    // them as render targets (e.g. from SetPipelineState()), a
    // state mismatch error will occur.
    UnbindTextureFromFramebuffer(pSrcTexture, true);
    UnbindTextureFromFramebuffer(pDstTexture, true);

    auto& CmdCtx = GetCmdContext();
    if (pSrcTexture->GetDesc().Usage == USAGE_STAGING)
    {
        DEV_CHECK_ERR((pSrcTexture->GetDesc().CPUAccessFlags & CPU_ACCESS_WRITE) != 0, "Source staging texture must be created with CPU_ACCESS_WRITE flag");
        DEV_CHECK_ERR(pSrcTexture->GetState() == RESOURCE_STATE_GENERIC_READ || !pSrcTexture->IsInKnownState(), "Staging texture must always be in RESOURCE_STATE_GENERIC_READ state");
    }
    TransitionOrVerifyTextureState(CmdCtx, *pSrcTexture, SrcTextureTransitionMode, RESOURCE_STATE_COPY_SOURCE, "Using resource as copy source (DeviceContextD3D12Impl::CopyTextureRegion)");

    if (pDstTexture->GetDesc().Usage == USAGE_STAGING)
    {
        DEV_CHECK_ERR((pDstTexture->GetDesc().CPUAccessFlags & CPU_ACCESS_READ) != 0, "Destination staging texture must be created with CPU_ACCESS_READ flag");
        DEV_CHECK_ERR(pDstTexture->GetState() == RESOURCE_STATE_COPY_DEST || !pDstTexture->IsInKnownState(), "Staging texture must always be in RESOURCE_STATE_COPY_DEST state");
    }
    TransitionOrVerifyTextureState(CmdCtx, *pDstTexture, DstTextureTransitionMode, RESOURCE_STATE_COPY_DEST, "Using resource as copy destination (DeviceContextD3D12Impl::CopyTextureRegion)");

    D3D12_TEXTURE_COPY_LOCATION DstLocation = {}, SrcLocation = {};

    SrcLocation.pResource = pSrcTexture->GetD3D12Resource();
    if (pSrcTexture->GetDesc().Usage == USAGE_STAGING)
    {
        SrcLocation.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        SrcLocation.PlacedFootprint = pSrcTexture->GetStagingFootprint(SrcSubResIndex);
    }
    else
    {
        SrcLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        SrcLocation.SubresourceIndex = SrcSubResIndex;
    }

    DstLocation.pResource = pDstTexture->GetD3D12Resource();
    if (pDstTexture->GetDesc().Usage == USAGE_STAGING)
    {
        DstLocation.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        DstLocation.PlacedFootprint = pDstTexture->GetStagingFootprint(DstSubResIndex);
    }
    else
    {
        DstLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DstLocation.SubresourceIndex = DstSubResIndex;
    }

    CmdCtx.FlushResourceBarriers();
    CmdCtx.GetCommandList()->CopyTextureRegion(&DstLocation, DstX, DstY, DstZ, &SrcLocation, pD3D12SrcBox);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::CopyTextureRegion(ID3D12Resource*                pd3d12Buffer,
                                               Uint32                         SrcOffset,
                                               Uint32                         SrcStride,
                                               Uint32                         SrcDepthStride,
                                               Uint32                         BufferSize,
                                               TextureD3D12Impl&              TextureD3D12,
                                               Uint32                         DstSubResIndex,
                                               const Box&                     DstBox,
                                               RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    const auto& TexDesc                 = TextureD3D12.GetDesc();
    auto&       CmdCtx                  = GetCmdContext();
    auto*       pCmdList                = CmdCtx.GetCommandList();
    bool        StateTransitionRequired = false;
    if (TextureTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        StateTransitionRequired = TextureD3D12.IsInKnownState() && !TextureD3D12.CheckState(RESOURCE_STATE_COPY_DEST);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TextureTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyTextureState(TextureD3D12, RESOURCE_STATE_COPY_DEST, "Using texture as copy destination (DeviceContextD3D12Impl::CopyTextureRegion)");
    }
#endif
    D3D12_RESOURCE_BARRIER BarrierDesc;
    if (StateTransitionRequired)
    {
        BarrierDesc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource   = TextureD3D12.GetD3D12Resource();
        BarrierDesc.Transition.Subresource = DstSubResIndex;
        BarrierDesc.Transition.StateBefore = ResourceStateFlagsToD3D12ResourceStates(TextureD3D12.GetState());
        BarrierDesc.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
        BarrierDesc.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        pCmdList->ResourceBarrier(1, &BarrierDesc);
    }

    D3D12_TEXTURE_COPY_LOCATION DstLocation;
    DstLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DstLocation.pResource        = TextureD3D12.GetD3D12Resource();
    DstLocation.SubresourceIndex = static_cast<UINT>(DstSubResIndex);

    D3D12_TEXTURE_COPY_LOCATION SrcLocation;
    SrcLocation.Type                              = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SrcLocation.pResource                         = pd3d12Buffer;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT& Footprint = SrcLocation.PlacedFootprint;
    Footprint.Offset                              = SrcOffset;
    Footprint.Footprint.Width                     = static_cast<UINT>(DstBox.MaxX - DstBox.MinX);
    Footprint.Footprint.Height                    = static_cast<UINT>(DstBox.MaxY - DstBox.MinY);
    Footprint.Footprint.Depth                     = static_cast<UINT>(DstBox.MaxZ - DstBox.MinZ); // Depth cannot be 0
    Footprint.Footprint.Format                    = TexFormatToDXGI_Format(TexDesc.Format);

    Footprint.Footprint.RowPitch = static_cast<UINT>(SrcStride);

#ifdef DILIGENT_DEBUG
    {
        const auto&  FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);
        const Uint32 RowCount   = std::max((Footprint.Footprint.Height / FmtAttribs.BlockHeight), 1u);
        VERIFY(BufferSize >= Footprint.Footprint.RowPitch * RowCount * Footprint.Footprint.Depth, "Buffer is not large enough");
        VERIFY(Footprint.Footprint.Depth == 1 || static_cast<UINT>(SrcDepthStride) == Footprint.Footprint.RowPitch * RowCount, "Depth stride must be equal to the size of 2D plane");
    }
#endif

    D3D12_BOX D3D12SrcBox;
    D3D12SrcBox.left   = 0;
    D3D12SrcBox.right  = Footprint.Footprint.Width;
    D3D12SrcBox.top    = 0;
    D3D12SrcBox.bottom = Footprint.Footprint.Height;
    D3D12SrcBox.front  = 0;
    D3D12SrcBox.back   = Footprint.Footprint.Depth;
    CmdCtx.GetCommandList()->CopyTextureRegion(&DstLocation,
                                               static_cast<UINT>(DstBox.MinX),
                                               static_cast<UINT>(DstBox.MinY),
                                               static_cast<UINT>(DstBox.MinZ),
                                               &SrcLocation, &D3D12SrcBox);

    ++m_State.NumCommands;

    if (StateTransitionRequired)
    {
        std::swap(BarrierDesc.Transition.StateBefore, BarrierDesc.Transition.StateAfter);
        pCmdList->ResourceBarrier(1, &BarrierDesc);
    }
}

void DeviceContextD3D12Impl::CopyTextureRegion(IBuffer*                       pSrcBuffer,
                                               Uint32                         SrcOffset,
                                               Uint32                         SrcStride,
                                               Uint32                         SrcDepthStride,
                                               class TextureD3D12Impl&        TextureD3D12,
                                               Uint32                         DstSubResIndex,
                                               const Box&                     DstBox,
                                               RESOURCE_STATE_TRANSITION_MODE BufferTransitionMode,
                                               RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    auto* pBufferD3D12 = ValidatedCast<BufferD3D12Impl>(pSrcBuffer);
    if (pBufferD3D12->GetDesc().Usage == USAGE_DYNAMIC)
        DEV_CHECK_ERR(pBufferD3D12->GetState() == RESOURCE_STATE_GENERIC_READ, "Dynamic buffer is expected to always be in RESOURCE_STATE_GENERIC_READ state");
    else
    {
        if (BufferTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
        {
            if (pBufferD3D12->IsInKnownState() && pBufferD3D12->GetState() != RESOURCE_STATE_GENERIC_READ)
                GetCmdContext().TransitionResource(pBufferD3D12, RESOURCE_STATE_GENERIC_READ);
        }
#ifdef DILIGENT_DEVELOPMENT
        else if (BufferTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
        {
            DvpVerifyBufferState(*pBufferD3D12, RESOURCE_STATE_COPY_SOURCE, "Using buffer as copy source (DeviceContextD3D12Impl::CopyTextureRegion)");
        }
#endif
    }
    GetCmdContext().FlushResourceBarriers();
    Uint64 DataStartByteOffset = 0;
    auto*  pd3d12Buffer        = pBufferD3D12->GetD3D12Buffer(DataStartByteOffset, this);
    CopyTextureRegion(pd3d12Buffer, static_cast<Uint32>(DataStartByteOffset) + SrcOffset, SrcStride, SrcDepthStride,
                      pBufferD3D12->GetDesc().uiSizeInBytes, TextureD3D12, DstSubResIndex, DstBox, TextureTransitionMode);
}

DeviceContextD3D12Impl::TextureUploadSpace DeviceContextD3D12Impl::AllocateTextureUploadSpace(TEXTURE_FORMAT TexFmt,
                                                                                              const Box&     Region)
{
    TextureUploadSpace UploadSpace;
    VERIFY_EXPR(Region.MaxX > Region.MinX && Region.MaxY > Region.MinY && Region.MaxZ > Region.MinZ);
    auto UpdateRegionWidth  = Region.MaxX - Region.MinX;
    auto UpdateRegionHeight = Region.MaxY - Region.MinY;
    auto UpdateRegionDepth  = Region.MaxZ - Region.MinZ;

    const auto& FmtAttribs = GetTextureFormatAttribs(TexFmt);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Box must be aligned by the calling function
        VERIFY_EXPR((UpdateRegionWidth % FmtAttribs.BlockWidth) == 0);
        VERIFY_EXPR((UpdateRegionHeight % FmtAttribs.BlockHeight) == 0);
        UploadSpace.RowSize  = UpdateRegionWidth / Uint32{FmtAttribs.BlockWidth} * Uint32{FmtAttribs.ComponentSize};
        UploadSpace.RowCount = UpdateRegionHeight / FmtAttribs.BlockHeight;
    }
    else
    {
        UploadSpace.RowSize  = UpdateRegionWidth * Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        UploadSpace.RowCount = UpdateRegionHeight;
    }
    // RowPitch must be a multiple of 256 (aka. D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
    UploadSpace.Stride        = (UploadSpace.RowSize + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UploadSpace.DepthStride   = UploadSpace.RowCount * UploadSpace.Stride;
    const auto MemorySize     = UpdateRegionDepth * UploadSpace.DepthStride;
    UploadSpace.Allocation    = AllocateDynamicSpace(MemorySize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    UploadSpace.AlignedOffset = (UploadSpace.Allocation.Offset + (D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);
    UploadSpace.Region        = Region;

    return UploadSpace;
}

void DeviceContextD3D12Impl::UpdateTextureRegion(const void*                    pSrcData,
                                                 Uint32                         SrcStride,
                                                 Uint32                         SrcDepthStride,
                                                 TextureD3D12Impl&              TextureD3D12,
                                                 Uint32                         DstSubResIndex,
                                                 const Box&                     DstBox,
                                                 RESOURCE_STATE_TRANSITION_MODE TextureTransitionMode)
{
    const auto& TexDesc           = TextureD3D12.GetDesc();
    auto        UploadSpace       = AllocateTextureUploadSpace(TexDesc.Format, DstBox);
    auto        UpdateRegionDepth = DstBox.MaxZ - DstBox.MinZ;
#ifdef DILIGENT_DEBUG
    {
        VERIFY(SrcStride >= UploadSpace.RowSize, "Source data stride (", SrcStride, ") is below the image row size (", UploadSpace.RowSize, ")");
        const Uint32 PlaneSize = SrcStride * UploadSpace.RowCount;
        VERIFY(UpdateRegionDepth == 1 || SrcDepthStride >= PlaneSize, "Source data depth stride (", SrcDepthStride, ") is below the image plane size (", PlaneSize, ")");
    }
#endif
    const auto AlignedOffset = UploadSpace.AlignedOffset;

    for (Uint32 DepthSlice = 0; DepthSlice < UpdateRegionDepth; ++DepthSlice)
    {
        for (Uint32 row = 0; row < UploadSpace.RowCount; ++row)
        {
            const auto* pSrcPtr =
                reinterpret_cast<const Uint8*>(pSrcData) + row * SrcStride + DepthSlice * SrcDepthStride;
            auto* pDstPtr =
                reinterpret_cast<Uint8*>(UploadSpace.Allocation.CPUAddress) + (AlignedOffset - UploadSpace.Allocation.Offset) + row * UploadSpace.Stride + DepthSlice * UploadSpace.DepthStride;

            memcpy(pDstPtr, pSrcPtr, UploadSpace.RowSize);
        }
    }
    CopyTextureRegion(UploadSpace.Allocation.pBuffer,
                      static_cast<Uint32>(AlignedOffset),
                      UploadSpace.Stride,
                      UploadSpace.DepthStride,
                      static_cast<Uint32>(UploadSpace.Allocation.Size - (AlignedOffset - UploadSpace.Allocation.Offset)),
                      TextureD3D12,
                      DstSubResIndex,
                      DstBox,
                      TextureTransitionMode);
}

void DeviceContextD3D12Impl::MapTextureSubresource(ITexture*                 pTexture,
                                                   Uint32                    MipLevel,
                                                   Uint32                    ArraySlice,
                                                   MAP_TYPE                  MapType,
                                                   MAP_FLAGS                 MapFlags,
                                                   const Box*                pMapRegion,
                                                   MappedTextureSubresource& MappedData)
{
    TDeviceContextBase::MapTextureSubresource(pTexture, MipLevel, ArraySlice, MapType, MapFlags, pMapRegion, MappedData);

    auto&       TextureD3D12 = *ValidatedCast<TextureD3D12Impl>(pTexture);
    const auto& TexDesc      = TextureD3D12.GetDesc();
    auto        Subres       = D3D12CalcSubresource(MipLevel, ArraySlice, 0, TexDesc.MipLevels, TexDesc.ArraySize);
    if (TexDesc.Usage == USAGE_DYNAMIC)
    {
        if (MapType != MAP_WRITE)
        {
            LOG_ERROR("USAGE_DEFAULT textures can only be mapped for writing");
            MappedData = MappedTextureSubresource{};
            return;
        }

        if ((MapFlags & (MAP_FLAG_DISCARD | MAP_FLAG_NO_OVERWRITE)) != 0)
            LOG_INFO_MESSAGE_ONCE("Mapping textures with flags MAP_FLAG_DISCARD or MAP_FLAG_NO_OVERWRITE has no effect in D3D12 backend");

        Box FullExtentBox;
        if (pMapRegion == nullptr)
        {
            FullExtentBox.MaxX = std::max(TexDesc.Width >> MipLevel, 1u);
            FullExtentBox.MaxY = std::max(TexDesc.Height >> MipLevel, 1u);
            if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
                FullExtentBox.MaxZ = std::max(TexDesc.Depth >> MipLevel, 1u);
            pMapRegion = &FullExtentBox;
        }

        auto UploadSpace       = AllocateTextureUploadSpace(TexDesc.Format, *pMapRegion);
        MappedData.pData       = reinterpret_cast<Uint8*>(UploadSpace.Allocation.CPUAddress) + (UploadSpace.AlignedOffset - UploadSpace.Allocation.Offset);
        MappedData.Stride      = UploadSpace.Stride;
        MappedData.DepthStride = UploadSpace.DepthStride;

        auto it = m_MappedTextures.emplace(MappedTextureKey{&TextureD3D12, Subres}, std::move(UploadSpace));
        if (!it.second)
            LOG_ERROR_MESSAGE("Mip level ", MipLevel, ", slice ", ArraySlice, " of texture '", TexDesc.Name, "' has already been mapped");
    }
    else if (TexDesc.Usage == USAGE_STAGING)
    {
        const auto& Footprint = TextureD3D12.GetStagingFootprint(Subres);

        // It is valid to specify the CPU won't read any data by passing a range where
        // End is less than or equal to Begin.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map
        D3D12_RANGE InvalidateRange = {1, 0};
        if (MapType == MAP_READ)
        {
            if ((MapFlags & MAP_FLAG_DO_NOT_WAIT) == 0)
            {
                LOG_WARNING_MESSAGE("D3D12 backend never waits for GPU when mapping staging textures for reading. "
                                    "Applications must use fences or other synchronization methods to explicitly synchronize "
                                    "access and use MAP_FLAG_DO_NOT_WAIT flag.");
            }

            DEV_CHECK_ERR((TexDesc.CPUAccessFlags & CPU_ACCESS_READ), "Texture '", TexDesc.Name, "' was not created with CPU_ACCESS_READ flag and can't be mapped for reading");
            // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
            InvalidateRange.Begin     = static_cast<SIZE_T>(Footprint.Offset);
            const auto& NextFootprint = TextureD3D12.GetStagingFootprint(Subres + 1);
            InvalidateRange.End       = static_cast<SIZE_T>(NextFootprint.Offset);
        }
        else if (MapType == MAP_WRITE)
        {
            DEV_CHECK_ERR((TexDesc.CPUAccessFlags & CPU_ACCESS_WRITE), "Texture '", TexDesc.Name, "' was not created with CPU_ACCESS_WRITE flag and can't be mapped for writing");
        }

        // Nested Map() calls are supported and are ref-counted. The first call to Map() allocates
        // a CPU virtual address range for the resource. The last call to Unmap deallocates the CPU
        // virtual address range.

        // Map() invalidates the CPU cache, when necessary, so that CPU reads to this address
        // reflect any modifications made by the GPU.
        void* pMappedDataPtr = nullptr;
        TextureD3D12.GetD3D12Resource()->Map(0, &InvalidateRange, &pMappedDataPtr);
        MappedData.pData       = reinterpret_cast<Uint8*>(pMappedDataPtr) + Footprint.Offset;
        MappedData.Stride      = static_cast<Uint32>(Footprint.Footprint.RowPitch);
        const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);
        MappedData.DepthStride = static_cast<Uint32>(Footprint.Footprint.Height / FmtAttribs.BlockHeight * Footprint.Footprint.RowPitch);
    }
    else
    {
        UNSUPPORTED(GetUsageString(TexDesc.Usage), " textures cannot currently be mapped in D3D12 back-end");
    }
}

void DeviceContextD3D12Impl::UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice)
{
    TDeviceContextBase::UnmapTextureSubresource(pTexture, MipLevel, ArraySlice);

    TextureD3D12Impl& TextureD3D12 = *ValidatedCast<TextureD3D12Impl>(pTexture);
    const auto&       TexDesc      = TextureD3D12.GetDesc();
    auto              Subres       = D3D12CalcSubresource(MipLevel, ArraySlice, 0, TexDesc.MipLevels, TexDesc.ArraySize);
    if (TexDesc.Usage == USAGE_DYNAMIC)
    {
        auto UploadSpaceIt = m_MappedTextures.find(MappedTextureKey{&TextureD3D12, Subres});
        if (UploadSpaceIt != m_MappedTextures.end())
        {
            auto& UploadSpace = UploadSpaceIt->second;
            CopyTextureRegion(UploadSpace.Allocation.pBuffer,
                              UploadSpace.AlignedOffset,
                              UploadSpace.Stride,
                              UploadSpace.DepthStride,
                              static_cast<Uint32>(UploadSpace.Allocation.Size - (UploadSpace.AlignedOffset - UploadSpace.Allocation.Offset)),
                              TextureD3D12,
                              Subres,
                              UploadSpace.Region,
                              RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            m_MappedTextures.erase(UploadSpaceIt);
        }
        else
        {
            LOG_ERROR_MESSAGE("Failed to unmap mip level ", MipLevel, ", slice ", ArraySlice, " of texture '", TexDesc.Name, "'. The texture has either been unmapped already or has not been mapped");
        }
    }
    else if (TexDesc.Usage == USAGE_STAGING)
    {
        // It is valid to specify the CPU didn't write any data by passing a range where
        // End is less than or equal to Begin.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-unmap
        D3D12_RANGE FlushRange = {1, 0};

        if (TexDesc.CPUAccessFlags == CPU_ACCESS_WRITE)
        {
            const auto& Footprint     = TextureD3D12.GetStagingFootprint(Subres);
            const auto& NextFootprint = TextureD3D12.GetStagingFootprint(Subres + 1);
            FlushRange.Begin          = static_cast<SIZE_T>(Footprint.Offset);
            FlushRange.End            = static_cast<SIZE_T>(NextFootprint.Offset);
        }

        // Map and Unmap can be called by multiple threads safely. Nested Map calls are supported
        // and are ref-counted. The first call to Map allocates a CPU virtual address range for the
        // resource. The last call to Unmap deallocates the CPU virtual address range.

        // Unmap() flushes the CPU cache, when necessary, so that GPU reads to this address reflect
        // any modifications made by the CPU.
        TextureD3D12.GetD3D12Resource()->Unmap(0, &FlushRange);
    }
    else
    {
        UNSUPPORTED(GetUsageString(TexDesc.Usage), " textures cannot currently be mapped in D3D12 back-end");
    }
}


void DeviceContextD3D12Impl::GenerateMips(ITextureView* pTexView)
{
    TDeviceContextBase::GenerateMips(pTexView);

    auto& Ctx = GetCmdContext();

    const auto& MipsGenerator = m_pDevice->GetMipsGenerator();
    MipsGenerator.GenerateMips(m_pDevice->GetD3D12Device(), ValidatedCast<TextureViewD3D12Impl>(pTexView), Ctx);
    ++m_State.NumCommands;
}

void DeviceContextD3D12Impl::FinishCommandList(ICommandList** ppCommandList)
{
    VERIFY(m_pActiveRenderPass == nullptr, "Finishing command list inside an active render pass.");

    CommandListD3D12Impl* pCmdListD3D12(NEW_RC_OBJ(m_CmdListAllocator, "CommandListD3D12Impl instance", CommandListD3D12Impl)(m_pDevice, this, std::move(m_CurrCmdCtx)));
    pCmdListD3D12->QueryInterface(IID_CommandList, reinterpret_cast<IObject**>(ppCommandList));
    Flush(true);

    InvalidateState();
}

void DeviceContextD3D12Impl::ExecuteCommandList(ICommandList* pCommandList)
{
    if (m_bIsDeferred)
    {
        LOG_ERROR_MESSAGE("Only immediate context can execute command list");
        return;
    }
    // First execute commands in this context
    Flush(true);

    InvalidateState();

    CommandListD3D12Impl* pCmdListD3D12 = ValidatedCast<CommandListD3D12Impl>(pCommandList);
    VERIFY_EXPR(m_PendingFences.empty());
    RefCntAutoPtr<DeviceContextD3D12Impl> pDeferredCtx;
    auto                                  CmdContext = pCmdListD3D12->Close(pDeferredCtx);
    m_pDevice->CloseAndExecuteCommandContext(m_CommandQueueId, std::move(CmdContext), true, nullptr);
    // Set the bit in the deferred context cmd queue mask corresponding to cmd queue of this context
    pDeferredCtx->m_SubmittedBuffersCmdQueueMask |= Uint64{1} << m_CommandQueueId;
}

void DeviceContextD3D12Impl::SignalFence(IFence* pFence, Uint64 Value)
{
    VERIFY(!m_bIsDeferred, "Fence can only be signaled from immediate context");
    m_PendingFences.emplace_back(Value, pFence);
}

void DeviceContextD3D12Impl::WaitForFence(IFence* pFence, Uint64 Value, bool FlushContext)
{
    VERIFY(!m_bIsDeferred, "Fence can only be waited from immediate context");
    if (FlushContext)
        Flush();
    auto* pFenceD3D12 = ValidatedCast<FenceD3D12Impl>(pFence);
    pFenceD3D12->WaitForCompletion(Value);
}

void DeviceContextD3D12Impl::WaitForIdle()
{
    VERIFY(!m_bIsDeferred, "Only immediate contexts can be idled");
    Flush();
    m_pDevice->IdleCommandQueue(m_CommandQueueId, true);
}

void DeviceContextD3D12Impl::BeginQuery(IQuery* pQuery)
{
    if (!TDeviceContextBase::BeginQuery(pQuery, 0))
        return;

    auto*      pQueryD3D12Impl = ValidatedCast<QueryD3D12Impl>(pQuery);
    const auto QueryType       = pQueryD3D12Impl->GetDesc().Type;
    if (QueryType != QUERY_TYPE_TIMESTAMP)
        ++m_ActiveQueriesCounter;

    auto& QueryMgr = m_pDevice->GetQueryManager();
    auto& Ctx      = GetCmdContext();
    auto  Idx      = pQueryD3D12Impl->GetQueryHeapIndex(0);
    if (QueryType != QUERY_TYPE_DURATION)
        QueryMgr.BeginQuery(Ctx, QueryType, Idx);
    else
        QueryMgr.EndQuery(Ctx, QueryType, Idx);
}

void DeviceContextD3D12Impl::EndQuery(IQuery* pQuery)
{
    if (!TDeviceContextBase::EndQuery(pQuery, 0))
        return;

    auto*      pQueryD3D12Impl = ValidatedCast<QueryD3D12Impl>(pQuery);
    const auto QueryType       = pQueryD3D12Impl->GetDesc().Type;
    if (QueryType != QUERY_TYPE_TIMESTAMP)
    {
        VERIFY(m_ActiveQueriesCounter > 0, "Active query counter is 0 which means there was a mismatch between BeginQuery() / EndQuery() calls");
        --m_ActiveQueriesCounter;
    }

    auto& QueryMgr = m_pDevice->GetQueryManager();
    auto& Ctx      = GetCmdContext();
    auto  Idx      = pQueryD3D12Impl->GetQueryHeapIndex(QueryType == QUERY_TYPE_DURATION ? 1 : 0);
    QueryMgr.EndQuery(Ctx, QueryType, Idx);
}

void DeviceContextD3D12Impl::TransitionResourceStates(Uint32 BarrierCount, StateTransitionDesc* pResourceBarriers)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");

    auto& CmdCtx = GetCmdContext();
    for (Uint32 i = 0; i < BarrierCount; ++i)
    {
#ifdef DILIGENT_DEVELOPMENT
        DvpVerifyStateTransitionDesc(pResourceBarriers[i]);
#endif
        CmdCtx.TransitionResource(pResourceBarriers[i]);
    }
}

void DeviceContextD3D12Impl::TransitionOrVerifyBufferState(CommandContext&                CmdCtx,
                                                           BufferD3D12Impl&               Buffer,
                                                           RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                           RESOURCE_STATE                 RequiredState,
                                                           const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        if (Buffer.IsInKnownState() && !Buffer.CheckState(RequiredState))
            CmdCtx.TransitionResource(&Buffer, RequiredState);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyBufferState(Buffer, RequiredState, OperationName);
    }
#endif
}

void DeviceContextD3D12Impl::TransitionOrVerifyTextureState(CommandContext&                CmdCtx,
                                                            TextureD3D12Impl&              Texture,
                                                            RESOURCE_STATE_TRANSITION_MODE TransitionMode,
                                                            RESOURCE_STATE                 RequiredState,
                                                            const char*                    OperationName)
{
    if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        if (Texture.IsInKnownState() && !Texture.CheckState(RequiredState))
            CmdCtx.TransitionResource(&Texture, RequiredState);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (TransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        DvpVerifyTextureState(Texture, RequiredState, OperationName);
    }
#endif
}

void DeviceContextD3D12Impl::TransitionTextureState(ITexture* pTexture, D3D12_RESOURCE_STATES State)
{
    VERIFY_EXPR(pTexture != nullptr);
    DEV_CHECK_ERR(pTexture->GetState() != RESOURCE_STATE_UNKNOWN, "Texture state is unknown");
    auto& CmdCtx = GetCmdContext();
    CmdCtx.TransitionResource(ValidatedCast<ITextureD3D12>(pTexture), D3D12ResourceStatesToResourceStateFlags(State));
}

void DeviceContextD3D12Impl::TransitionBufferState(IBuffer* pBuffer, D3D12_RESOURCE_STATES State)
{
    VERIFY_EXPR(pBuffer != nullptr);
    DEV_CHECK_ERR(pBuffer->GetState() != RESOURCE_STATE_UNKNOWN, "Buffer state is unknown");
    auto& CmdCtx = GetCmdContext();
    CmdCtx.TransitionResource(ValidatedCast<IBufferD3D12>(pBuffer), D3D12ResourceStatesToResourceStateFlags(State));
}

ID3D12GraphicsCommandList* DeviceContextD3D12Impl::GetD3D12CommandList()
{
    auto& CmdCtx = GetCmdContext();
    CmdCtx.FlushResourceBarriers();
    return CmdCtx.GetCommandList();
}

void DeviceContextD3D12Impl::ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                       ITexture*                               pDstTexture,
                                                       const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
    TDeviceContextBase::ResolveTextureSubresource(pSrcTexture, pDstTexture, ResolveAttribs);

    auto*       pSrcTexD3D12 = ValidatedCast<TextureD3D12Impl>(pSrcTexture);
    auto*       pDstTexD3D12 = ValidatedCast<TextureD3D12Impl>(pDstTexture);
    const auto& SrcTexDesc   = pSrcTexD3D12->GetDesc();
    const auto& DstTexDesc   = pDstTexD3D12->GetDesc();

    auto& CmdCtx = GetCmdContext();
    TransitionOrVerifyTextureState(CmdCtx, *pSrcTexD3D12, ResolveAttribs.SrcTextureTransitionMode, RESOURCE_STATE_RESOLVE_SOURCE, "Resolving multi-sampled texture (DeviceContextD3D12Impl::ResolveTextureSubresource)");
    TransitionOrVerifyTextureState(CmdCtx, *pDstTexD3D12, ResolveAttribs.DstTextureTransitionMode, RESOURCE_STATE_RESOLVE_DEST, "Resolving multi-sampled texture (DeviceContextD3D12Impl::ResolveTextureSubresource)");

    auto Format = ResolveAttribs.Format;
    if (Format == TEX_FORMAT_UNKNOWN)
    {
        const auto& SrcFmtAttribs = GetTextureFormatAttribs(SrcTexDesc.Format);
        if (!SrcFmtAttribs.IsTypeless)
        {
            Format = SrcTexDesc.Format;
        }
        else
        {
            const auto& DstFmtAttribs = GetTextureFormatAttribs(DstTexDesc.Format);
            DEV_CHECK_ERR(!DstFmtAttribs.IsTypeless, "Resolve operation format can't be typeless when both source and destination textures are typeless");
            Format = DstFmtAttribs.Format;
        }
    }

    auto DXGIFmt        = TexFormatToDXGI_Format(Format);
    auto SrcSubresIndex = D3D12CalcSubresource(ResolveAttribs.SrcMipLevel, ResolveAttribs.SrcSlice, 0, SrcTexDesc.MipLevels, SrcTexDesc.ArraySize);
    auto DstSubresIndex = D3D12CalcSubresource(ResolveAttribs.DstMipLevel, ResolveAttribs.DstSlice, 0, DstTexDesc.MipLevels, DstTexDesc.ArraySize);

    CmdCtx.ResolveSubresource(pDstTexD3D12->GetD3D12Resource(), DstSubresIndex, pSrcTexD3D12->GetD3D12Resource(), SrcSubresIndex, DXGIFmt);
}
} // namespace Diligent
