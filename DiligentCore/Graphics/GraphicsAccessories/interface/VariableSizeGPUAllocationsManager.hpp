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


// An GPU-tailored extension of the basic variable-size allocations manager
// See http://diligentgraphics.com/diligent-engine/architecture/d3d12/variable-size-memory-allocations-manager/

#pragma once

#include <deque>
#include "VariableSizeAllocationsManager.hpp"

namespace Diligent
{
// Class extends basic variable-size memory block allocator by deferring deallocation
// of freed blocks untill the corresponding frame is completed
class VariableSizeGPUAllocationsManager : public VariableSizeAllocationsManager
{
private:
    struct StaleAllocationAttribs
    {
        OffsetType Offset;
        OffsetType Size;
        Uint64     FenceValue;
        StaleAllocationAttribs(OffsetType _Offset, OffsetType _Size, Uint64 _FenceValue) :
            Offset{_Offset}, Size{_Size}, FenceValue{_FenceValue}
        {}
    };

public:
    VariableSizeGPUAllocationsManager(OffsetType MaxSize, IMemoryAllocator& Allocator) :
        VariableSizeAllocationsManager{MaxSize, Allocator},
        m_StaleAllocations{0, StaleAllocationAttribs(0, 0, 0), STD_ALLOCATOR_RAW_MEM(StaleAllocationAttribs, Allocator, "Allocator for deque<StaleAllocationAttribs>")}
    {}

    ~VariableSizeGPUAllocationsManager()
    {
        VERIFY(m_StaleAllocations.empty(), "Not all stale allocations released");
        VERIFY(m_StaleAllocationsSize == 0, "Not all stale allocations released");
    }

    // = default causes compiler error when instantiating std::vector::emplace_back() in Visual Studio 2015 (Version 14.0.23107.0 D14REL)
    VariableSizeGPUAllocationsManager(VariableSizeGPUAllocationsManager&& rhs) noexcept :
        VariableSizeAllocationsManager(std::move(rhs)),
        m_StaleAllocations(std::move(rhs.m_StaleAllocations)),
        m_StaleAllocationsSize(rhs.m_StaleAllocationsSize)
    {
        rhs.m_StaleAllocationsSize = 0;
    }

    // clang-format off
	VariableSizeGPUAllocationsManager& operator = (VariableSizeGPUAllocationsManager&& rhs) = delete;
    VariableSizeGPUAllocationsManager(const VariableSizeGPUAllocationsManager&) = delete;
    VariableSizeGPUAllocationsManager& operator = (const VariableSizeGPUAllocationsManager&) = delete;
    // clang-format on

    void Free(VariableSizeAllocationsManager::Allocation&& allocation, Uint64 FenceValue)
    {
        Free(allocation.UnalignedOffset, allocation.Size, FenceValue);
        allocation = VariableSizeAllocationsManager::Allocation{};
    }

    void Free(OffsetType Offset, OffsetType Size, Uint64 FenceValue)
    {
        // Do not release the block immediately, but add
        // it to the queue instead
        m_StaleAllocations.emplace_back(Offset, Size, FenceValue);
        m_StaleAllocationsSize += Size;
    }

    // Releases stale allocation from completed command lists
    // The method takes the last known completed fence value N
    // and releases all allocations whose associated fence value
    // is at most N (n <= N)
    void ReleaseStaleAllocations(Uint64 LastCompletedFenceValue)
    {
        // Free all allocations from the beginning of the queue that belong to completed command lists
        while (!m_StaleAllocations.empty() && m_StaleAllocations.front().FenceValue <= LastCompletedFenceValue)
        {
            auto& OldestAllocation = m_StaleAllocations.front();
            VariableSizeAllocationsManager::Free(OldestAllocation.Offset, OldestAllocation.Size);
            m_StaleAllocationsSize -= OldestAllocation.Size;
            m_StaleAllocations.pop_front();
        }
    }

    size_t GetStaleAllocationsSize() const { return m_StaleAllocationsSize; }

private:
    std::deque<StaleAllocationAttribs, STDAllocatorRawMem<StaleAllocationAttribs>> m_StaleAllocations;
    size_t                                                                         m_StaleAllocationsSize = 0;
};
} // namespace Diligent
