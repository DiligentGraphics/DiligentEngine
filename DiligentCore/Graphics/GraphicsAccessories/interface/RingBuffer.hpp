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
/// Implementation of Diligent::RingBuffer class


#include <deque>
#include "../../../Primitives/interface/MemoryAllocator.h"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "../../../Common/interface/Align.hpp"
#include "../../../Common/interface/STDAllocator.hpp"

namespace Diligent
{
/// Implementation of a ring buffer. The class is not thread-safe.
class RingBuffer
{
public:
    using OffsetType = size_t;
    struct FrameHeadAttribs
    {
        // clang-format off
        FrameHeadAttribs(Uint64 fv, OffsetType off, OffsetType sz) noexcept :
            FenceValue{fv },
            Offset    {off},
            Size      {sz }
        {}
        // clang-format on

        // Fence value associated with the command list in which
        // the allocation could have been referenced last time
        Uint64     FenceValue;
        OffsetType Offset;
        OffsetType Size;
    };
    static constexpr const OffsetType InvalidOffset = static_cast<OffsetType>(-1);

    RingBuffer(OffsetType MaxSize, IMemoryAllocator& Allocator) noexcept :
        m_CompletedFrameHeads(STD_ALLOCATOR_RAW_MEM(FrameHeadAttribs, Allocator, "Allocator for deque<FrameHeadAttribs>")),
        m_MaxSize{MaxSize}
    {}

    // clang-format off
    RingBuffer(RingBuffer&& rhs) noexcept :
        m_CompletedFrameHeads{std::move(rhs.m_CompletedFrameHeads)},
        m_Tail           {rhs.m_Tail         },
        m_Head           {rhs.m_Head         },
        m_MaxSize        {rhs.m_MaxSize      },
        m_UsedSize       {rhs.m_UsedSize     },
        m_CurrFrameSize  {rhs.m_CurrFrameSize}
    // clang-format on
    {
        rhs.m_Tail          = 0;
        rhs.m_Head          = 0;
        rhs.m_MaxSize       = 0;
        rhs.m_UsedSize      = 0;
        rhs.m_CurrFrameSize = 0;
    }

    RingBuffer& operator=(RingBuffer&& rhs) noexcept
    {
        m_CompletedFrameHeads = std::move(rhs.m_CompletedFrameHeads);
        m_Tail                = rhs.m_Tail;
        m_Head                = rhs.m_Head;
        m_MaxSize             = rhs.m_MaxSize;
        m_UsedSize            = rhs.m_UsedSize;
        m_CurrFrameSize       = rhs.m_CurrFrameSize;

        rhs.m_MaxSize       = 0;
        rhs.m_Tail          = 0;
        rhs.m_Head          = 0;
        rhs.m_UsedSize      = 0;
        rhs.m_CurrFrameSize = 0;

        return *this;
    }

    // clang-format off
    RingBuffer             (const RingBuffer&) = delete;
    RingBuffer& operator = (const RingBuffer&) = delete;
    // clang-format on

    ~RingBuffer()
    {
        VERIFY(m_UsedSize == 0, "All space in the ring buffer must be released");
    }

    OffsetType Allocate(OffsetType Size, OffsetType Alignment)
    {
        VERIFY_EXPR(Size > 0);
        VERIFY(IsPowerOfTwo(Alignment), "Alignment (", Alignment, ") must be power of 2");
        Size = Align(Size, Alignment);

        if (m_UsedSize + Size > m_MaxSize)
        {
            return InvalidOffset;
        }

        auto AlignedHead = Align(m_Head, Alignment);
        if (m_Head >= m_Tail)
        {
            //                                         AlignedHead
            //                     Tail          Head  |            MaxSize
            //                     |                |  |            |
            //  [                  xxxxxxxxxxxxxxxxx...             ]
            //
            //
            if (AlignedHead + Size <= m_MaxSize)
            {
                auto Offset       = AlignedHead;
                auto AdjustedSize = Size + (AlignedHead - m_Head);
                m_Head += AdjustedSize;
                m_UsedSize += AdjustedSize;
                m_CurrFrameSize += AdjustedSize;
                return Offset;
            }
            else if (Size <= m_Tail)
            {
                // Allocate from the beginning of the buffer
                //
                //
                // Offset              Tail          Head               MaxSize
                //  |                  |                |<---AddSize--->|
                //  [                  xxxxxxxxxxxxxxxxx++++++++++++++++]
                //
                OffsetType AddSize = (m_MaxSize - m_Head) + Size;
                m_UsedSize += AddSize;
                m_CurrFrameSize += AddSize;
                m_Head = Size;
                return 0;
            }
        }
        else if (AlignedHead + Size <= m_Tail)
        {
            //          AlignedHead
            //    Head  |              Tail
            //       |  |              |
            //  [xxxx...               xxxxxxxxxxxxxxxxxxxxxxxxxx]
            //
            auto Offset       = AlignedHead;
            auto AdjustedSize = Size + (AlignedHead - m_Head);
            m_Head += AdjustedSize;
            m_UsedSize += AdjustedSize;
            m_CurrFrameSize += AdjustedSize;
            return Offset;
        }

        return InvalidOffset;
    }

    // FenceValue is the fence value associated with the command list in which the head
    // could have been referenced last time
    // See http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-resource-lifetimes/
    void FinishCurrentFrame(Uint64 FenceValue)
    {
#ifdef DILIGENT_DEBUG
        if (!m_CompletedFrameHeads.empty())
            VERIFY(FenceValue >= m_CompletedFrameHeads.back().FenceValue, "Current frame fence value (", FenceValue, ") is lower than the fence value of the previous frame (", m_CompletedFrameHeads.back().FenceValue, ")");
#endif
        // Ignore zero-size frames
        if (m_CurrFrameSize != 0)
        {
            m_CompletedFrameHeads.emplace_back(FenceValue, m_Head, m_CurrFrameSize);
            m_CurrFrameSize = 0;
        }
    }

    // CompletedFenceValue indicates GPU progress
    // See http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-resource-lifetimes/
    void ReleaseCompletedFrames(Uint64 CompletedFenceValue)
    {
        // We can release all heads whose associated fence value is less than or equal to CompletedFenceValue
        while (!m_CompletedFrameHeads.empty() && m_CompletedFrameHeads.front().FenceValue <= CompletedFenceValue)
        {
            const auto& OldestFrameHead = m_CompletedFrameHeads.front();
            VERIFY_EXPR(OldestFrameHead.Size <= m_UsedSize);
            m_UsedSize -= OldestFrameHead.Size;
            m_Tail = OldestFrameHead.Offset;
            m_CompletedFrameHeads.pop_front();
        }

        if (IsEmpty())
        {
#ifdef DILIGENT_DEBUG
            VERIFY(m_CompletedFrameHeads.empty(), "Zero-size heads are not added to the list, and since the buffer is empty, there must be no heads in the list");
            for (const auto& head : m_CompletedFrameHeads)
                VERIFY(head.Size == 0, "Non zero-size head found");
#endif
            m_CompletedFrameHeads.clear();

            //       t,h                 t,h
            //  |     |     |   ====>     |           |
            m_Tail = m_Head = 0;
        }
    }

    // clang-format off
    OffsetType GetMaxSize() const { return m_MaxSize; }
    bool       IsFull()     const { return m_UsedSize==m_MaxSize; };
    bool       IsEmpty()    const { return m_UsedSize==0; };
    OffsetType GetUsedSize()const { return m_UsedSize; }
    // clang-format on

private:
    std::deque<FrameHeadAttribs, STDAllocatorRawMem<FrameHeadAttribs>> m_CompletedFrameHeads;

    OffsetType m_Tail          = 0;
    OffsetType m_Head          = 0;
    OffsetType m_MaxSize       = 0;
    OffsetType m_UsedSize      = 0;
    OffsetType m_CurrFrameSize = 0;
};
} // namespace Diligent
