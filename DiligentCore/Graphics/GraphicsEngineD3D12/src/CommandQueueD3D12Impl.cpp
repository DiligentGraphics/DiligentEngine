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
#include "CommandQueueD3D12Impl.hpp"

namespace Diligent
{

CommandQueueD3D12Impl::CommandQueueD3D12Impl(IReferenceCounters* pRefCounters,
                                             ID3D12CommandQueue* pd3d12NativeCmdQueue,
                                             ID3D12Fence*        pd3d12Fence) :
    // clang-format off
    TBase{pRefCounters},
    m_pd3d12CmdQueue       {pd3d12NativeCmdQueue},
    m_d3d12Fence           {pd3d12Fence         },
    m_NextFenceValue       {1                   },
    m_WaitForGPUEventHandle{CreateEvent(nullptr, false, false, nullptr)}
// clang-format on
{
    VERIFY_EXPR(m_WaitForGPUEventHandle != INVALID_HANDLE_VALUE);
    m_d3d12Fence->Signal(0);
}

CommandQueueD3D12Impl::~CommandQueueD3D12Impl()
{
    CloseHandle(m_WaitForGPUEventHandle);
}

IMPLEMENT_QUERY_INTERFACE(CommandQueueD3D12Impl, IID_CommandQueueD3D12, TBase)

Uint64 CommandQueueD3D12Impl::Submit(ID3D12GraphicsCommandList* commandList)
{
    std::lock_guard<std::mutex> Lock{m_QueueMtx};

    auto FenceValue = m_NextFenceValue;
    // Increment the value before submitting the list
    Atomics::AtomicIncrement(m_NextFenceValue);

    if (commandList != nullptr)
    {
        ID3D12CommandList* const ppCmdLists[] = {commandList};
        m_pd3d12CmdQueue->ExecuteCommandLists(1, ppCmdLists);
    }

    // Signal the fence. This must be done atomically with command list submission.
    m_pd3d12CmdQueue->Signal(m_d3d12Fence, FenceValue);

    return FenceValue;
}

Uint64 CommandQueueD3D12Impl::WaitForIdle()
{
    std::lock_guard<std::mutex> Lock{m_QueueMtx};

    Uint64 LastSignaledFenceValue = m_NextFenceValue;
    Atomics::AtomicIncrement(m_NextFenceValue);

    m_pd3d12CmdQueue->Signal(m_d3d12Fence, LastSignaledFenceValue);

    if (GetCompletedFenceValue() < LastSignaledFenceValue)
    {
        m_d3d12Fence->SetEventOnCompletion(LastSignaledFenceValue, m_WaitForGPUEventHandle);
        WaitForSingleObject(m_WaitForGPUEventHandle, INFINITE);
        VERIFY(GetCompletedFenceValue() == LastSignaledFenceValue, "Unexpected signaled fence value");
    }
    return LastSignaledFenceValue;
}

Uint64 CommandQueueD3D12Impl::GetCompletedFenceValue()
{
    auto CompletedFenceValue = m_d3d12Fence->GetCompletedValue();
    if (CompletedFenceValue > m_LastCompletedFenceValue)
        m_LastCompletedFenceValue = CompletedFenceValue;
    return m_LastCompletedFenceValue;
}

void CommandQueueD3D12Impl::SignalFence(ID3D12Fence* pFence, Uint64 Value)
{
    std::lock_guard<std::mutex> Lock{m_QueueMtx};
    m_pd3d12CmdQueue->Signal(pFence, Value);
}

} // namespace Diligent
