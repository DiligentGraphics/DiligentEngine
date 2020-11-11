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
#include <algorithm>
#include "FixedBlockMemoryAllocator.hpp"
#include "Align.hpp"

namespace Diligent
{

static size_t AdjustBlockSize(size_t BlockSize)
{
    return Align(std::max(BlockSize, size_t{1}), sizeof(void*));
}

FixedBlockMemoryAllocator::FixedBlockMemoryAllocator(IMemoryAllocator& RawMemoryAllocator,
                                                     size_t            BlockSize,
                                                     Uint32            NumBlocksInPage) :
    // clang-format off
    m_PagePool          (STD_ALLOCATOR_RAW_MEM(MemoryPage, RawMemoryAllocator, "Allocator for vector<MemoryPage>")),
    m_AvailablePages    (STD_ALLOCATOR_RAW_MEM(size_t, RawMemoryAllocator, "Allocator for unordered_set<size_t>") ),
    m_AddrToPageId      (STD_ALLOCATOR_RAW_MEM(AddrToPageIdMapElem, RawMemoryAllocator, "Allocator for unordered_map<void*, size_t>")),
    m_RawMemoryAllocator{RawMemoryAllocator        },
    m_BlockSize         {AdjustBlockSize(BlockSize)},
    m_NumBlocksInPage   {NumBlocksInPage           }
// clang-format on
{
    VERIFY_EXPR(BlockSize > 0);

    // Allocate one page
    CreateNewPage();
}

FixedBlockMemoryAllocator::~FixedBlockMemoryAllocator()
{
#ifdef DILIGENT_DEBUG
    for (size_t p = 0; p < m_PagePool.size(); ++p)
    {
        VERIFY(!m_PagePool[p].HasAllocations(), "Memory leak detected: memory page has allocated block");
        VERIFY(m_AvailablePages.find(p) != m_AvailablePages.end(), "Memory page is not in the available page pool");
    }
#endif
}

void FixedBlockMemoryAllocator::CreateNewPage()
{
    m_PagePool.emplace_back(*this);
    m_AvailablePages.insert(m_PagePool.size() - 1);
    m_AddrToPageId.reserve(m_PagePool.size() * m_NumBlocksInPage);
}

void* FixedBlockMemoryAllocator::Allocate(size_t Size, const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber)
{
    VERIFY_EXPR(Size > 0);

    Size = AdjustBlockSize(Size);
    VERIFY(m_BlockSize == Size, "Requested size (", Size, ") does not match the block size (", m_BlockSize, ")");

    std::lock_guard<std::mutex> LockGuard(m_Mutex);

    if (m_AvailablePages.empty())
    {
        CreateNewPage();
    }

    auto  PageId = *m_AvailablePages.begin();
    auto& Page   = m_PagePool[PageId];
    auto* Ptr    = Page.Allocate();
    m_AddrToPageId.insert(std::make_pair(Ptr, PageId));
    if (!Page.HasSpace())
    {
        m_AvailablePages.erase(m_AvailablePages.begin());
    }

    return Ptr;
}

void FixedBlockMemoryAllocator::Free(void* Ptr)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);
    auto                        PageIdIt = m_AddrToPageId.find(Ptr);
    if (PageIdIt != m_AddrToPageId.end())
    {
        auto PageId = PageIdIt->second;
        VERIFY_EXPR(PageId >= 0 && PageId < m_PagePool.size());
        m_PagePool[PageId].DeAllocate(Ptr);
        m_AvailablePages.insert(PageId);
        m_AddrToPageId.erase(PageIdIt);
        if (m_AvailablePages.size() > 1 && !m_PagePool[PageId].HasAllocations())
        {
            // In current implementation pages are never released!
            // Note that if we delete a page, all indices past it will be invalid

            //m_PagePool.erase(m_PagePool.begin() + PageId);
            //m_AvailablePages.erase(PageId);
        }
    }
    else
    {
        UNEXPECTED("Address not found in the allocations list - double freeing memory?");
    }
}

} // namespace Diligent
