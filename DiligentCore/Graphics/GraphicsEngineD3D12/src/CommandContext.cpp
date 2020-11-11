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
#include "d3dx12_win.h"
#include "CommandContext.hpp"
#include "TextureD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "CommandListManager.hpp"
#include "D3D12TypeConversions.hpp"


namespace Diligent
{

CommandContext::CommandContext(CommandListManager& CmdListManager) :
    // clang-format off
	m_pCurGraphicsRootSignature {nullptr},
	m_pCurPipelineState         {nullptr},
	m_pCurComputeRootSignature  {nullptr},
    m_PendingResourceBarriers   (STD_ALLOCATOR_RAW_MEM(D3D12_RESOURCE_BARRIER, GetRawAllocator(), "Allocator for vector<D3D12_RESOURCE_BARRIER>"))
// clang-format on
{
    m_PendingResourceBarriers.reserve(MaxPendingBarriers);
    CmdListManager.CreateNewCommandList(&m_pCommandList, &m_pCurrentAllocator, m_MaxInterfaceVer);
}

CommandContext::~CommandContext(void)
{
    DEV_CHECK_ERR(m_pCurrentAllocator == nullptr, "Command allocator must be released prior to destroying the command context");
}

void CommandContext::Reset(CommandListManager& CmdListManager)
{
    // We only call Reset() on previously freed contexts. The command list persists, but we need to
    // request a new allocator
    VERIFY_EXPR(m_pCommandList != nullptr);
    if (!m_pCurrentAllocator)
    {
        CmdListManager.RequestAllocator(&m_pCurrentAllocator);
        // Unlike ID3D12CommandAllocator::Reset, ID3D12GraphicsCommandList::Reset can be called while the
        // command list is still being executed. A typical pattern is to submit a command list and then
        // immediately reset it to reuse the allocated memory for another command list.
        m_pCommandList->Reset(m_pCurrentAllocator, nullptr);
    }

    m_pCurPipelineState         = nullptr;
    m_pCurGraphicsRootSignature = nullptr;
    m_pCurComputeRootSignature  = nullptr;
    m_PendingResourceBarriers.clear();
    m_BoundDescriptorHeaps = ShaderDescriptorHeaps();

    m_DynamicGPUDescriptorAllocators = nullptr;

    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
#if 0
	BindDescriptorHeaps();
#endif
}

ID3D12GraphicsCommandList* CommandContext::Close(CComPtr<ID3D12CommandAllocator>& pAllocator)
{
    FlushResourceBarriers();

    //if (m_ID.length() > 0)
    //	EngineProfiling::EndBlock(this);

    VERIFY_EXPR(m_pCurrentAllocator != nullptr);
    auto hr = m_pCommandList->Close();
    DEV_CHECK_ERR(SUCCEEDED(hr), "Failed to close the command list");

    pAllocator = std::move(m_pCurrentAllocator);
    return m_pCommandList;
}

void CommandContext::TransitionResource(ITextureD3D12* pTexture, RESOURCE_STATE NewState)
{
    VERIFY_EXPR(pTexture != nullptr);
    auto* pTexD3D12 = ValidatedCast<TextureD3D12Impl>(pTexture);
    VERIFY(pTexD3D12->IsInKnownState(), "Texture state can't be unknown");
    StateTransitionDesc TextureBarrier(pTexture, RESOURCE_STATE_UNKNOWN, NewState, true);
    TransitionResource(TextureBarrier);
}

void CommandContext::TransitionResource(IBufferD3D12* pBuffer, RESOURCE_STATE NewState)
{
    VERIFY_EXPR(pBuffer != nullptr);
    auto* pBuffD3D12 = ValidatedCast<BufferD3D12Impl>(pBuffer);
    VERIFY(pBuffD3D12->IsInKnownState(), "Buffer state can't be unknown");
    StateTransitionDesc BufferBarrier(pBuffer, RESOURCE_STATE_UNKNOWN, NewState, true);
    TransitionResource(BufferBarrier);
}

void CommandContext::InsertUAVBarrier(ID3D12Resource* pd3d12Resource)
{
    m_PendingResourceBarriers.emplace_back();
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_PendingResourceBarriers.back();
    // UAV barrier indicates that all UAV accesses (reads or writes) to a particular resource
    // must complete before any future UAV accesses (read or write) can begin.
    BarrierDesc.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    BarrierDesc.Flags         = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.UAV.pResource = pd3d12Resource;
}

static D3D12_RESOURCE_BARRIER_FLAGS TransitionTypeToD3D12ResourceBarrierFlag(STATE_TRANSITION_TYPE TransitionType)
{
    switch (TransitionType)
    {
        case STATE_TRANSITION_TYPE_IMMEDIATE:
            return D3D12_RESOURCE_BARRIER_FLAG_NONE;

        case STATE_TRANSITION_TYPE_BEGIN:
            return D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        case STATE_TRANSITION_TYPE_END:
            return D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;

        default:
            UNEXPECTED("Unexpected state transition type");
            return D3D12_RESOURCE_BARRIER_FLAG_NONE;
    }
}

void CommandContext::TransitionResource(const StateTransitionDesc& Barrier)
{
    DEV_CHECK_ERR((Barrier.pTexture != nullptr) ^ (Barrier.pBuffer != nullptr), "Exactly one of pTexture or pBuffer must not be null");

    DEV_CHECK_ERR(Barrier.NewState != RESOURCE_STATE_UNKNOWN, "New resource state can't be unknown");
    RESOURCE_STATE    OldState          = RESOURCE_STATE_UNKNOWN;
    ID3D12Resource*   pd3d12Resource    = nullptr;
    TextureD3D12Impl* pTextureD3D12Impl = nullptr;
    BufferD3D12Impl*  pBufferD3D12Impl  = nullptr;
    if (Barrier.pTexture)
    {
        pTextureD3D12Impl = ValidatedCast<TextureD3D12Impl>(Barrier.pTexture);
        pd3d12Resource    = pTextureD3D12Impl->GetD3D12Resource();
        OldState          = pTextureD3D12Impl->GetState();
    }
    else
    {
        VERIFY_EXPR(Barrier.pBuffer != nullptr);
        pBufferD3D12Impl = ValidatedCast<BufferD3D12Impl>(Barrier.pBuffer);
        pd3d12Resource   = pBufferD3D12Impl->GetD3D12Resource();
        OldState         = pBufferD3D12Impl->GetState();

#ifdef DILIGENT_DEVELOPMENT
        // Dynamic buffers wtih no SRV/UAV bind flags are suballocated in
        // the upload heap when Map() is called and must always be in
        // D3D12_RESOURCE_STATE_GENERIC_READ state
        if (pBufferD3D12Impl->GetDesc().Usage == USAGE_DYNAMIC && (pBufferD3D12Impl->GetDesc().BindFlags & (BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS)) == 0)
        {
            DEV_CHECK_ERR(pBufferD3D12Impl->GetState() == RESOURCE_STATE_GENERIC_READ, "Dynamic buffers that cannot be bound as SRV or UAV are expected to always be in D3D12_RESOURCE_STATE_GENERIC_READ state");
            VERIFY((Barrier.NewState & RESOURCE_STATE_GENERIC_READ) == Barrier.NewState, "Dynamic buffers can only transition to one of RESOURCE_STATE_GENERIC_READ states");
        }
#endif
    }

    if (OldState == RESOURCE_STATE_UNKNOWN)
    {
        DEV_CHECK_ERR(Barrier.OldState != RESOURCE_STATE_UNKNOWN, "When resource state is unknown (which means it is managed by the app), OldState member must not be RESOURCE_STATE_UNKNOWN");
        OldState = Barrier.OldState;
    }
    else
    {
        DEV_CHECK_ERR(Barrier.OldState == RESOURCE_STATE_UNKNOWN || Barrier.OldState == OldState,
                      "Resource state is known (", GetResourceStateString(OldState), ") and does not match the OldState (",
                      GetResourceStateString(Barrier.OldState), ") specified in the resource barrier. Set OldState member to "
                                                                "RESOURCE_STATE_UNKNOWN to make the engine use current resource state");
    }

    // Check if required state is already set
    if ((OldState & Barrier.NewState) != Barrier.NewState)
    {
        auto NewState = Barrier.NewState;
        // If both old state and new state are read-only states, combine the two
        if ((OldState & RESOURCE_STATE_GENERIC_READ) == OldState &&
            (NewState & RESOURCE_STATE_GENERIC_READ) == NewState)
            NewState = static_cast<RESOURCE_STATE>(OldState | NewState);

        D3D12_RESOURCE_BARRIER BarrierDesc;
        BarrierDesc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Flags                  = TransitionTypeToD3D12ResourceBarrierFlag(Barrier.TransitionType);
        BarrierDesc.Transition.pResource   = pd3d12Resource;
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = ResourceStateFlagsToD3D12ResourceStates(OldState);
        BarrierDesc.Transition.StateAfter  = ResourceStateFlagsToD3D12ResourceStates(NewState);

        // Note that RESOURCE_STATE_UNDEFINED != RESOURCE_STATE_PRESENT, but
        // D3D12_RESOURCE_STATE_COMMON == D3D12_RESOURCE_STATE_PRESENT
        if (BarrierDesc.Transition.StateBefore != BarrierDesc.Transition.StateAfter)
        {
            if (pTextureD3D12Impl)
            {
                const auto& TexDesc = pTextureD3D12Impl->GetDesc();
                VERIFY(Barrier.FirstMipLevel < TexDesc.MipLevels, "First mip level is out of range");
                VERIFY(Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || Barrier.FirstMipLevel + Barrier.MipLevelsCount <= TexDesc.MipLevels,
                       "Invalid mip level range");
                VERIFY(Barrier.FirstArraySlice < TexDesc.ArraySize, "First array slice is out of range");
                VERIFY(Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || Barrier.FirstArraySlice + Barrier.ArraySliceCount <= TexDesc.ArraySize,
                       "Invalid array slice range");

                if (Barrier.FirstMipLevel == 0 && (Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || Barrier.MipLevelsCount == TexDesc.MipLevels) &&
                    Barrier.FirstArraySlice == 0 && (Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || Barrier.ArraySliceCount == TexDesc.ArraySize))
                {
                    BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    m_PendingResourceBarriers.emplace_back(BarrierDesc);
                }
                else
                {
                    Uint32 EndMip   = Barrier.MipLevelsCount == REMAINING_MIP_LEVELS ? TexDesc.MipLevels : Barrier.FirstMipLevel + Barrier.MipLevelsCount;
                    Uint32 EndSlice = Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES ? TexDesc.ArraySize : Barrier.FirstArraySlice + Barrier.ArraySliceCount;
                    for (Uint32 mip = Barrier.FirstMipLevel; mip < EndMip; ++mip)
                    {
                        for (Uint32 slice = Barrier.FirstArraySlice; slice < EndSlice; ++slice)
                        {
                            BarrierDesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, TexDesc.MipLevels, TexDesc.ArraySize);
                            m_PendingResourceBarriers.emplace_back(BarrierDesc);
                        }
                    }
                }
            }
            else
            {
                VERIFY_EXPR(pBufferD3D12Impl);
                m_PendingResourceBarriers.emplace_back(BarrierDesc);
            }
        }

        if (pTextureD3D12Impl)
        {
            VERIFY(!Barrier.UpdateResourceState || (Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE || Barrier.TransitionType == STATE_TRANSITION_TYPE_END),
                   "Texture state can't be updated in begin-split barrier");
            if (Barrier.UpdateResourceState)
            {
                pTextureD3D12Impl->SetState(NewState);
            }
        }
        else
        {
            VERIFY_EXPR(pBufferD3D12Impl);

            VERIFY(!Barrier.UpdateResourceState || (Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE || Barrier.TransitionType == STATE_TRANSITION_TYPE_END),
                   "Buffer state can't be updated in begin-split barrier");
            if (Barrier.UpdateResourceState)
            {
                pBufferD3D12Impl->SetState(NewState);
            }

            if (pBufferD3D12Impl->GetDesc().Usage == USAGE_DYNAMIC && (pBufferD3D12Impl->GetDesc().BindFlags & (BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS)) == 0)
                VERIFY(pBufferD3D12Impl->GetState() == RESOURCE_STATE_GENERIC_READ,
                       "Dynamic buffers without SRV/UAV bind flag are expected to never "
                       "transition from RESOURCE_STATE_GENERIC_READ state");
        }
    }

    if (OldState == RESOURCE_STATE_UNORDERED_ACCESS && Barrier.NewState == RESOURCE_STATE_UNORDERED_ACCESS)
    {
        DEV_CHECK_ERR(Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE, "UAV barriers must not be split");
        InsertUAVBarrier(pd3d12Resource);
    }

    if (m_PendingResourceBarriers.size() >= MaxPendingBarriers)
        FlushResourceBarriers();
}

void CommandContext::InsertAliasBarrier(D3D12ResourceBase& Before, D3D12ResourceBase& After, bool FlushImmediate)
{
    m_PendingResourceBarriers.emplace_back();
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_PendingResourceBarriers.back();

    BarrierDesc.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    BarrierDesc.Flags                    = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.Aliasing.pResourceBefore = Before.GetD3D12Resource();
    BarrierDesc.Aliasing.pResourceAfter  = After.GetD3D12Resource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

} // namespace Diligent
