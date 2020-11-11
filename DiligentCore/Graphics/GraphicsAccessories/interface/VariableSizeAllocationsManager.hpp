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

// Helper class that handles free memory block management to accommodate variable-size allocation requests
// See http://diligentgraphics.com/diligent-engine/architecture/d3d12/variable-size-memory-allocations-manager/

#pragma once

#include <map>
#include <algorithm>

#include "../../../Primitives/interface/MemoryAllocator.h"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "../../../Common/interface/Align.hpp"
#include "../../../Common/interface/STDAllocator.hpp"

namespace Diligent
{
// The class handles free memory block management to accommodate variable-size allocation requests.
// It keeps track of free blocks only and does not record allocation sizes. The class uses two ordered maps
// to facilitate operations. The first map keeps blocks sorted by their offsets. The second multimap keeps blocks
// sorted by their sizes. The elements of the two maps reference each other, which enables efficient block
// insertion, removal and merging.
//
//   8                 32                       64                           104
//   |<---16--->|       |<-----24------>|        |<---16--->|                 |<-----32----->|
//
//
//        m_FreeBlocksBySize      m_FreeBlocksByOffset
//           size->offset            offset->size
//
//                16 ------------------>  8  ---------->  {size = 16, &m_FreeBlocksBySize[0]}
//
//                16 ------.   .-------> 32  ---------->  {size = 24, &m_FreeBlocksBySize[2]}
//                          '.'
//                24 -------' '--------> 64  ---------->  {size = 16, &m_FreeBlocksBySize[1]}
//
//                32 ------------------> 104 ---------->  {size = 32, &m_FreeBlocksBySize[3]}
//
class VariableSizeAllocationsManager
{
public:
    using OffsetType = size_t;

private:
    struct FreeBlockInfo;

    // Type of the map that keeps memory blocks sorted by their offsets
    using TFreeBlocksByOffsetMap =
        std::map<OffsetType,
                 FreeBlockInfo,
                 std::less<OffsetType>,                                         // Standard ordering
                 STDAllocatorRawMem<std::pair<const OffsetType, FreeBlockInfo>> // Raw memory allocator
                 >;

    // Type of the map that keeps memory blocks sorted by their sizes
    using TFreeBlocksBySizeMap =
        std::multimap<OffsetType,
                      TFreeBlocksByOffsetMap::iterator,
                      std::less<OffsetType>,                                                            // Standard ordering
                      STDAllocatorRawMem<std::pair<const OffsetType, TFreeBlocksByOffsetMap::iterator>> // Raw memory allocator
                      >;

    struct FreeBlockInfo
    {
        // Block size (no reserved space for the size of the allocation)
        OffsetType Size;

        // Iterator referencing this block in the multimap sorted by the block size
        TFreeBlocksBySizeMap::iterator OrderBySizeIt;

        FreeBlockInfo(OffsetType _Size) :
            Size(_Size) {}
    };

public:
    VariableSizeAllocationsManager(OffsetType MaxSize, IMemoryAllocator& Allocator) :
        m_FreeBlocksByOffset(STD_ALLOCATOR_RAW_MEM(TFreeBlocksByOffsetMap::value_type, Allocator, "Allocator for map<OffsetType, FreeBlockInfo>")),
        m_FreeBlocksBySize(STD_ALLOCATOR_RAW_MEM(TFreeBlocksBySizeMap::value_type, Allocator, "Allocator for multimap<OffsetType, TFreeBlocksByOffsetMap::iterator>")),
        m_MaxSize(MaxSize),
        m_FreeSize(MaxSize)
    {
        // Insert single maximum-size block
        AddNewBlock(0, m_MaxSize);
        ResetCurrAlignment();

#ifdef DILIGENT_DEBUG
        DbgVerifyList();
#endif
    }

    ~VariableSizeAllocationsManager()
    {
#ifdef DILIGENT_DEBUG
        if (!m_FreeBlocksByOffset.empty() || !m_FreeBlocksBySize.empty())
        {
            VERIFY(m_FreeBlocksByOffset.size() == 1, "Single free block is expected");
            VERIFY(m_FreeBlocksByOffset.begin()->first == 0, "Head chunk offset is expected to be 0");
            VERIFY(m_FreeBlocksByOffset.begin()->second.Size == m_MaxSize, "Head chunk size is expected to be ", m_MaxSize);
            VERIFY_EXPR(m_FreeBlocksByOffset.begin()->second.OrderBySizeIt == m_FreeBlocksBySize.begin());
            VERIFY(m_FreeBlocksBySize.size() == m_FreeBlocksByOffset.size(), "Sizes of the two maps must be equal");

            VERIFY(m_FreeBlocksBySize.size() == 1, "Single free block is expected");
            VERIFY(m_FreeBlocksBySize.begin()->first == m_MaxSize, "Head chunk size is expected to be ", m_MaxSize);
            VERIFY(m_FreeBlocksBySize.begin()->second == m_FreeBlocksByOffset.begin(), "Incorrect first block");
        }
#endif
    }

    // clang-format off
    VariableSizeAllocationsManager(VariableSizeAllocationsManager&& rhs) noexcept :
        m_FreeBlocksByOffset {std::move(rhs.m_FreeBlocksByOffset)},
        m_FreeBlocksBySize   {std::move(rhs.m_FreeBlocksBySize)  },
        m_MaxSize            {rhs.m_MaxSize      },
        m_FreeSize           {rhs.m_FreeSize     },
        m_CurrAlignment      {rhs.m_CurrAlignment}
    {
        // clang-format on
        rhs.m_MaxSize       = 0;
        rhs.m_FreeSize      = 0;
        rhs.m_CurrAlignment = 0;
    }

    // clang-format off
    VariableSizeAllocationsManager& operator = (VariableSizeAllocationsManager&& rhs) = default;
    VariableSizeAllocationsManager             (const VariableSizeAllocationsManager&) = delete;
    VariableSizeAllocationsManager& operator = (const VariableSizeAllocationsManager&) = delete;
    // clang-format on

    // Offset returned by Allocate() may not be aligned, but the size of the allocation
    // is sufficient to properly align it
    struct Allocation
    {
        // clang-format off
        Allocation(OffsetType offset, OffsetType size) :
            UnalignedOffset{offset},
            Size           {size  }
        {}
        // clang-format on

        Allocation() {}

        static constexpr OffsetType InvalidOffset = static_cast<OffsetType>(-1);
        static Allocation           InvalidAllocation()
        {
            return Allocation{InvalidOffset, 0};
        }

        bool IsValid() const
        {
            return UnalignedOffset != InvalidAllocation().UnalignedOffset;
        }

        bool operator==(const Allocation& rhs) const
        {
            return UnalignedOffset == rhs.UnalignedOffset &&
                Size == rhs.Size;
        }

        OffsetType UnalignedOffset = InvalidOffset;
        OffsetType Size            = 0;
    };

    Allocation Allocate(OffsetType Size, OffsetType Alignment)
    {
        VERIFY_EXPR(Size > 0);
        VERIFY(IsPowerOfTwo(Alignment), "Alignment (", Alignment, ") must be power of 2");
        Size = Align(Size, Alignment);
        if (m_FreeSize < Size)
            return Allocation::InvalidAllocation();

        auto AlignmentReserve = (Alignment > m_CurrAlignment) ? Alignment - m_CurrAlignment : 0;
        // Get the first block that is large enough to encompass Size + AlignmentReserve bytes
        // lower_bound() returns an iterator pointing to the first element that
        // is not less (i.e. >= ) than key
        auto SmallestBlockItIt = m_FreeBlocksBySize.lower_bound(Size + AlignmentReserve);
        if (SmallestBlockItIt == m_FreeBlocksBySize.end())
            return Allocation::InvalidAllocation();

        auto SmallestBlockIt = SmallestBlockItIt->second;
        VERIFY_EXPR(Size + AlignmentReserve <= SmallestBlockIt->second.Size);
        VERIFY_EXPR(SmallestBlockIt->second.Size == SmallestBlockItIt->first);

        //     SmallestBlockIt.Offset
        //        |                                  |
        //        |<------SmallestBlockIt.Size------>|
        //        |<------Size------>|<---NewSize--->|
        //        |                  |
        //      Offset              NewOffset
        //
        auto Offset = SmallestBlockIt->first;
        VERIFY_EXPR(Offset % m_CurrAlignment == 0);
        auto AlignedOffset = Align(Offset, Alignment);
        auto AdjustedSize  = Size + (AlignedOffset - Offset);
        VERIFY_EXPR(AdjustedSize <= Size + AlignmentReserve);
        auto NewOffset = Offset + AdjustedSize;
        auto NewSize   = SmallestBlockIt->second.Size - AdjustedSize;
        VERIFY_EXPR(SmallestBlockItIt == SmallestBlockIt->second.OrderBySizeIt);
        m_FreeBlocksBySize.erase(SmallestBlockItIt);
        m_FreeBlocksByOffset.erase(SmallestBlockIt);
        if (NewSize > 0)
        {
            AddNewBlock(NewOffset, NewSize);
        }

        m_FreeSize -= AdjustedSize;

        if ((Size & (m_CurrAlignment - 1)) != 0)
        {
            if (IsPowerOfTwo(Size))
            {
                VERIFY_EXPR(Size >= Alignment && Size < m_CurrAlignment);
                m_CurrAlignment = Size;
            }
            else
            {
                m_CurrAlignment = std::min(m_CurrAlignment, Alignment);
            }
        }

#ifdef DILIGENT_DEBUG
        DbgVerifyList();
#endif
        return Allocation{Offset, AdjustedSize};
    }

    void Free(Allocation&& allocation)
    {
        Free(allocation.UnalignedOffset, allocation.Size);
        allocation = Allocation{};
    }

    void Free(OffsetType Offset, OffsetType Size)
    {
        VERIFY_EXPR(Offset + Size <= m_MaxSize);

        // Find the first element whose offset is greater than the specified offset.
        // upper_bound() returns an iterator pointing to the first element in the
        // container whose key is considered to go after k.
        auto NextBlockIt = m_FreeBlocksByOffset.upper_bound(Offset);
#ifdef DILIGENT_DEBUG
        {
            auto LowBnd = m_FreeBlocksByOffset.lower_bound(Offset); // First element whose offset is  >=
            // Since zero-size allocations are not allowed, lower bound must always be equal to the upper bound
            VERIFY_EXPR(LowBnd == NextBlockIt);
        }
#endif
        // Block being deallocated must not overlap with the next block
        VERIFY_EXPR(NextBlockIt == m_FreeBlocksByOffset.end() || Offset + Size <= NextBlockIt->first);
        auto PrevBlockIt = NextBlockIt;
        if (PrevBlockIt != m_FreeBlocksByOffset.begin())
        {
            --PrevBlockIt;
            // Block being deallocated must not overlap with the previous block
            VERIFY_EXPR(Offset >= PrevBlockIt->first + PrevBlockIt->second.Size);
        }
        else
            PrevBlockIt = m_FreeBlocksByOffset.end();

        OffsetType NewSize, NewOffset;
        if (PrevBlockIt != m_FreeBlocksByOffset.end() && Offset == PrevBlockIt->first + PrevBlockIt->second.Size)
        {
            //  PrevBlock.Offset             Offset
            //       |                          |
            //       |<-----PrevBlock.Size----->|<------Size-------->|
            //
            NewSize   = PrevBlockIt->second.Size + Size;
            NewOffset = PrevBlockIt->first;

            if (NextBlockIt != m_FreeBlocksByOffset.end() && Offset + Size == NextBlockIt->first)
            {
                //   PrevBlock.Offset           Offset            NextBlock.Offset
                //     |                          |                    |
                //     |<-----PrevBlock.Size----->|<------Size-------->|<-----NextBlock.Size----->|
                //
                NewSize += NextBlockIt->second.Size;
                m_FreeBlocksBySize.erase(PrevBlockIt->second.OrderBySizeIt);
                m_FreeBlocksBySize.erase(NextBlockIt->second.OrderBySizeIt);
                // Delete the range of two blocks
                ++NextBlockIt;
                m_FreeBlocksByOffset.erase(PrevBlockIt, NextBlockIt);
            }
            else
            {
                //   PrevBlock.Offset           Offset                     NextBlock.Offset
                //     |                          |                             |
                //     |<-----PrevBlock.Size----->|<------Size-------->| ~ ~ ~  |<-----NextBlock.Size----->|
                //
                m_FreeBlocksBySize.erase(PrevBlockIt->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(PrevBlockIt);
            }
        }
        else if (NextBlockIt != m_FreeBlocksByOffset.end() && Offset + Size == NextBlockIt->first)
        {
            //   PrevBlock.Offset                   Offset            NextBlock.Offset
            //     |                                  |                    |
            //     |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->|<-----NextBlock.Size----->|
            //
            NewSize   = Size + NextBlockIt->second.Size;
            NewOffset = Offset;
            m_FreeBlocksBySize.erase(NextBlockIt->second.OrderBySizeIt);
            m_FreeBlocksByOffset.erase(NextBlockIt);
        }
        else
        {
            //   PrevBlock.Offset                   Offset                     NextBlock.Offset
            //     |                                  |                            |
            //     |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->| ~ ~ ~ |<-----NextBlock.Size----->|
            //
            NewSize   = Size;
            NewOffset = Offset;
        }

        AddNewBlock(NewOffset, NewSize);

        m_FreeSize += Size;
        if (IsEmpty())
        {
            // Reset current alignment
            VERIFY_EXPR(GetNumFreeBlocks() == 1);
            ResetCurrAlignment();
        }

#ifdef DILIGENT_DEBUG
        DbgVerifyList();
#endif
    }

    // clang-format off
    bool IsFull() const{ return m_FreeSize==0; };
    bool IsEmpty()const{ return m_FreeSize==m_MaxSize; };
    OffsetType GetMaxSize() const{return m_MaxSize;}
    OffsetType GetFreeSize()const{return m_FreeSize;}
    OffsetType GetUsedSize()const{return m_MaxSize - m_FreeSize;}
    // clang-format on

    size_t GetNumFreeBlocks() const
    {
        return m_FreeBlocksByOffset.size();
    }

    void Extend(size_t ExtraSize)
    {
        size_t NewBlockOffset = m_MaxSize;
        size_t NewBlockSize   = ExtraSize;

        if (!m_FreeBlocksByOffset.empty())
        {
            auto LastBlockIt = m_FreeBlocksByOffset.end();
            --LastBlockIt;

            const auto LastBlockOffset = LastBlockIt->first;
            const auto LastBlockSize   = LastBlockIt->second.Size;
            if (LastBlockOffset + LastBlockSize == m_MaxSize)
            {
                // Extend the last block
                NewBlockOffset = LastBlockOffset;
                NewBlockSize += LastBlockSize;

                VERIFY_EXPR(LastBlockIt->second.OrderBySizeIt->first == LastBlockSize &&
                            LastBlockIt->second.OrderBySizeIt->second == LastBlockIt);
                m_FreeBlocksBySize.erase(LastBlockIt->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(LastBlockIt);
            }
        }

        AddNewBlock(NewBlockOffset, NewBlockSize);

        m_MaxSize += ExtraSize;
        m_FreeSize += ExtraSize;

#ifdef DILIGENT_DEBUG
        DbgVerifyList();
#endif
    }

private:
    void AddNewBlock(OffsetType Offset, OffsetType Size)
    {
        auto NewBlockIt = m_FreeBlocksByOffset.emplace(Offset, Size);
        VERIFY_EXPR(NewBlockIt.second);
        auto OrderIt                           = m_FreeBlocksBySize.emplace(Size, NewBlockIt.first);
        NewBlockIt.first->second.OrderBySizeIt = OrderIt;
    }

    void ResetCurrAlignment()
    {
        for (m_CurrAlignment = 1; m_CurrAlignment * 2 <= m_MaxSize; m_CurrAlignment *= 2)
        {}
    }

#ifdef DILIGENT_DEBUG
    void DbgVerifyList()
    {
        OffsetType TotalFreeSize = 0;

        VERIFY_EXPR(IsPowerOfTwo(m_CurrAlignment));
        auto BlockIt     = m_FreeBlocksByOffset.begin();
        auto PrevBlockIt = m_FreeBlocksByOffset.end();
        VERIFY_EXPR(m_FreeBlocksByOffset.size() == m_FreeBlocksBySize.size());
        while (BlockIt != m_FreeBlocksByOffset.end())
        {
            VERIFY_EXPR(BlockIt->first >= 0 && BlockIt->first + BlockIt->second.Size <= m_MaxSize);
            VERIFY((BlockIt->first & (m_CurrAlignment - 1)) == 0, "Block offset (", BlockIt->first, ") is not ", m_CurrAlignment, "-aligned");
            if (BlockIt->first + BlockIt->second.Size < m_MaxSize)
                VERIFY((BlockIt->second.Size & (m_CurrAlignment - 1)) == 0, "All block sizes except for the last one must be ", m_CurrAlignment, "-aligned");
            VERIFY_EXPR(BlockIt == BlockIt->second.OrderBySizeIt->second);
            VERIFY_EXPR(BlockIt->second.Size == BlockIt->second.OrderBySizeIt->first);
            //   PrevBlock.Offset                   BlockIt.first
            //     |                                  |
            // ~ ~ |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->| ~ ~ ~
            //
            VERIFY(PrevBlockIt == m_FreeBlocksByOffset.end() || BlockIt->first > PrevBlockIt->first + PrevBlockIt->second.Size, "Unmerged adjacent or overlapping blocks detected");
            TotalFreeSize += BlockIt->second.Size;

            PrevBlockIt = BlockIt;
            ++BlockIt;
        }

        auto OrderIt = m_FreeBlocksBySize.begin();
        while (OrderIt != m_FreeBlocksBySize.end())
        {
            VERIFY_EXPR(OrderIt->first == OrderIt->second->second.Size);
            ++OrderIt;
        }

        VERIFY_EXPR(TotalFreeSize == m_FreeSize);
    }
#endif

    TFreeBlocksByOffsetMap m_FreeBlocksByOffset;
    TFreeBlocksBySizeMap   m_FreeBlocksBySize;

    OffsetType m_MaxSize       = 0;
    OffsetType m_FreeSize      = 0;
    OffsetType m_CurrAlignment = 0;
    // When adding new members, do not forget to update move ctor
};
} // namespace Diligent
