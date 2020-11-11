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
/// Declaration of Diligent::FixedBlockMemoryAllocator class

#include <unordered_map>
#include <mutex>
#include <unordered_set>
#include <vector>
#include <cstring>
#include <memory>
#include "../../Primitives/interface/Errors.hpp"
#include "../../Primitives/interface/MemoryAllocator.h"
#include "STDAllocator.hpp"

namespace Diligent
{

#ifdef DILIGENT_DEBUG
inline void FillWithDebugPattern(void* ptr, Uint8 Pattern, size_t NumBytes)
{
    memset(ptr, Pattern, NumBytes);
}
#else
#    define FillWithDebugPattern(...)
#endif

/// Memory allocator that allocates memory in a fixed-size chunks
class FixedBlockMemoryAllocator final : public IMemoryAllocator
{
public:
    FixedBlockMemoryAllocator(IMemoryAllocator& RawMemoryAllocator, size_t BlockSize, Uint32 NumBlocksInPage);
    ~FixedBlockMemoryAllocator();

    /// Allocates block of memory
    virtual void* Allocate(size_t Size, const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber) override final;

    /// Releases memory
    virtual void Free(void* Ptr) override final;

private:
    // clang-format off
    FixedBlockMemoryAllocator             (const FixedBlockMemoryAllocator&) = delete;
    FixedBlockMemoryAllocator             (FixedBlockMemoryAllocator&&)      = delete;
    FixedBlockMemoryAllocator& operator = (const FixedBlockMemoryAllocator&) = delete;
    FixedBlockMemoryAllocator& operator = (FixedBlockMemoryAllocator&&)      = delete;
    // clang-format on

    void CreateNewPage();

    // Memory page class is based on the fixed-size memory pool described in "Fast Efficient Fixed-Size Memory Pool"
    // by Ben Kenwright
    class MemoryPage
    {
    public:
        static constexpr Uint8 NewPageMemPattern          = 0xAA;
        static constexpr Uint8 AllocatedBlockMemPattern   = 0xAB;
        static constexpr Uint8 DeallocatedBlockMemPattern = 0xDE;
        static constexpr Uint8 InitializedBlockMemPattern = 0xCF;

        MemoryPage(FixedBlockMemoryAllocator& OwnerAllocator) :
            // clang-format off
            m_NumFreeBlocks       {OwnerAllocator.m_NumBlocksInPage},
            m_NumInitializedBlocks{0},
            m_pOwnerAllocator     {&OwnerAllocator}
        // clang-format on
        {
            auto PageSize = OwnerAllocator.m_BlockSize * OwnerAllocator.m_NumBlocksInPage;
            m_pPageStart  = reinterpret_cast<Uint8*>(
                OwnerAllocator.m_RawMemoryAllocator.Allocate(PageSize, "FixedBlockMemoryAllocator page", __FILE__, __LINE__));
            m_pNextFreeBlock = m_pPageStart;
            FillWithDebugPattern(m_pPageStart, NewPageMemPattern, PageSize);
        }

        MemoryPage(MemoryPage&& Page) noexcept :
            // clang-format off
            m_NumFreeBlocks       {Page.m_NumFreeBlocks       },
            m_NumInitializedBlocks{Page.m_NumInitializedBlocks},
            m_pPageStart          {Page.m_pPageStart          },
            m_pNextFreeBlock      {Page.m_pNextFreeBlock      },
            m_pOwnerAllocator     {Page.m_pOwnerAllocator     }
        // clang-format on
        {
            Page.m_NumFreeBlocks        = 0;
            Page.m_NumInitializedBlocks = 0;
            Page.m_pPageStart           = nullptr;
            Page.m_pNextFreeBlock       = nullptr;
            Page.m_pOwnerAllocator      = nullptr;
        }

        ~MemoryPage()
        {
            if (m_pOwnerAllocator)
                m_pOwnerAllocator->m_RawMemoryAllocator.Free(m_pPageStart);
        }

        void* GetBlockStartAddress(Uint32 BlockIndex) const
        {
            VERIFY_EXPR(m_pOwnerAllocator != nullptr);
            VERIFY(BlockIndex >= 0 && BlockIndex < m_pOwnerAllocator->m_NumBlocksInPage, "Invalid block index");
            return reinterpret_cast<Uint8*>(m_pPageStart) + BlockIndex * m_pOwnerAllocator->m_BlockSize;
        }

#ifdef DILIGENT_DEBUG
        void dbgVerifyAddress(const void* pBlockAddr) const
        {
            size_t Delta = reinterpret_cast<const Uint8*>(pBlockAddr) - reinterpret_cast<Uint8*>(m_pPageStart);
            VERIFY(Delta % m_pOwnerAllocator->m_BlockSize == 0, "Invalid address");
            Uint32 BlockIndex = static_cast<Uint32>(Delta / m_pOwnerAllocator->m_BlockSize);
            VERIFY(BlockIndex >= 0 && BlockIndex < m_pOwnerAllocator->m_NumBlocksInPage, "Invalid block index");
        }
#else
#    define dbgVerifyAddress(...)
#endif

        void* Allocate()
        {
            VERIFY_EXPR(m_pOwnerAllocator != nullptr);

            if (m_NumFreeBlocks == 0)
            {
                VERIFY_EXPR(m_NumInitializedBlocks == m_pOwnerAllocator->m_NumBlocksInPage);
                return nullptr;
            }

            // Initialize the next block
            if (m_NumInitializedBlocks < m_pOwnerAllocator->m_NumBlocksInPage)
            {
                // Link next uninitialized block to the end of the list:

                //
                //                            ___________                      ___________
                //                           |           |                    |           |
                //                           | 0xcdcdcd  |                 -->| 0xcdcdcd  |   m_NumInitializedBlocks
                //                           |-----------|                |   |-----------|
                //                           |           |                |   |           |
                //  m_NumInitializedBlocks   | 0xcdcdcd  |      ==>        ---|           |
                //                           |-----------|                    |-----------|
                //
                //                           ~           ~                    ~           ~
                //                           |           |                    |           |
                //                       0   |           |                    |           |
                //                            -----------                      -----------
                //
                auto* pUninitializedBlock = GetBlockStartAddress(m_NumInitializedBlocks);
                FillWithDebugPattern(pUninitializedBlock, InitializedBlockMemPattern, m_pOwnerAllocator->m_BlockSize);
                void** ppNextBlock = reinterpret_cast<void**>(pUninitializedBlock);
                ++m_NumInitializedBlocks;
                if (m_NumInitializedBlocks < m_pOwnerAllocator->m_NumBlocksInPage)
                    *ppNextBlock = GetBlockStartAddress(m_NumInitializedBlocks);
                else
                    *ppNextBlock = nullptr;
            }

            void* res = m_pNextFreeBlock;
            dbgVerifyAddress(res);
            // Move pointer to the next free block
            m_pNextFreeBlock = *reinterpret_cast<void**>(m_pNextFreeBlock);
            --m_NumFreeBlocks;
            if (m_NumFreeBlocks != 0)
                dbgVerifyAddress(m_pNextFreeBlock);
            else
                VERIFY_EXPR(m_pNextFreeBlock == nullptr);

            FillWithDebugPattern(res, AllocatedBlockMemPattern, m_pOwnerAllocator->m_BlockSize);
            return res;
        }

        void DeAllocate(void* p)
        {
            VERIFY_EXPR(m_pOwnerAllocator != nullptr);

            dbgVerifyAddress(p);
            FillWithDebugPattern(p, DeallocatedBlockMemPattern, m_pOwnerAllocator->m_BlockSize);
            // Add block to the beginning of the linked list
            *reinterpret_cast<void**>(p) = m_pNextFreeBlock;
            m_pNextFreeBlock             = p;
            ++m_NumFreeBlocks;
        }

        bool HasSpace() const { return m_NumFreeBlocks > 0; }
        bool HasAllocations() const { return m_NumFreeBlocks < m_NumInitializedBlocks; }

    private:
        MemoryPage(const MemoryPage&) = delete;
        MemoryPage& operator=(const MemoryPage) = delete;
        MemoryPage& operator=(MemoryPage&&) = delete;

        Uint32                     m_NumFreeBlocks        = 0;       // Num of remaining blocks
        Uint32                     m_NumInitializedBlocks = 0;       // Num of initialized blocks
        void*                      m_pPageStart           = nullptr; // Beginning of memory pool
        void*                      m_pNextFreeBlock       = nullptr; // Num of next free block
        FixedBlockMemoryAllocator* m_pOwnerAllocator      = nullptr;
    };

    std::vector<MemoryPage, STDAllocatorRawMem<MemoryPage>>                                          m_PagePool;
    std::unordered_set<size_t, std::hash<size_t>, std::equal_to<size_t>, STDAllocatorRawMem<size_t>> m_AvailablePages;

    using AddrToPageIdMapElem = std::pair<void* const, size_t>;
    std::unordered_map<void*, size_t, std::hash<void*>, std::equal_to<void*>, STDAllocatorRawMem<AddrToPageIdMapElem>> m_AddrToPageId;

    std::mutex m_Mutex;

    IMemoryAllocator& m_RawMemoryAllocator;
    const size_t      m_BlockSize;
    const Uint32      m_NumBlocksInPage;
};

IMemoryAllocator& GetRawAllocator();

template <typename ObjectType>
class ObjectPool
{
public:
    static void SetRawAllocator(IMemoryAllocator& Allocator)
    {
#ifdef DILIGENT_DEBUG
        if (m_bPoolInitialized && m_pRawAllocator != &Allocator)
        {
            LOG_WARNING_MESSAGE("Setting pool raw allocator after the pool has been initialized has no effect");
        }
#endif
        m_pRawAllocator = &Allocator;
    }
    static void SetPageSize(Uint32 NumAllocationsInPage)
    {
#ifdef DILIGENT_DEBUG
        if (m_bPoolInitialized && m_NumAllocationsInPage != NumAllocationsInPage)
        {
            LOG_WARNING_MESSAGE("Setting pool page size after the pool has been initialized has no effect");
        }
#endif
        m_NumAllocationsInPage = NumAllocationsInPage;
    }
    static ObjectPool& GetPool()
    {
        static ObjectPool ThePool;
#ifdef DILIGENT_DEBUG
        m_bPoolInitialized = true;
#endif
        return ThePool;
    }

    template <typename... CtorArgTypes>
    ObjectType* NewObject(const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber, CtorArgTypes&&... CtorArgs)
    {
        void* pRawMem = m_FixedBlockAlloctor.Allocate(sizeof(ObjectType), dbgDescription, dbgFileName, dbgLineNumber);
        try
        {
            return new (pRawMem) ObjectType(std::forward<CtorArgTypes>(CtorArgs)...);
        }
        catch (...)
        {
            m_FixedBlockAlloctor.Free(pRawMem);
            return nullptr;
        }
    }

    void Destroy(ObjectType* pObj)
    {
        if (pObj != nullptr)
        {
            pObj->~ObjectType();
            m_FixedBlockAlloctor.Free(pObj);
        }
    }

private:
    static Uint32            m_NumAllocationsInPage;
    static IMemoryAllocator* m_pRawAllocator;

    ObjectPool() :
        m_FixedBlockAlloctor(m_pRawAllocator ? *m_pRawAllocator : GetRawAllocator(), sizeof(ObjectType), m_NumAllocationsInPage)
    {}
#ifdef DILIGENT_DEBUG
    static bool m_bPoolInitialized;
#endif
    FixedBlockMemoryAllocator m_FixedBlockAlloctor;
};
template <typename ObjectType>
Uint32 ObjectPool<ObjectType>::m_NumAllocationsInPage = 64;

template <typename ObjectType>
IMemoryAllocator* ObjectPool<ObjectType>::m_pRawAllocator = nullptr;

#ifdef DILIGENT_DEBUG
template <typename ObjectType>
bool ObjectPool<ObjectType>::m_bPoolInitialized = false;
#endif

#define SET_POOL_RAW_ALLOCATOR(ObjectType, Allocator)        ObjectPool<ObjectType>::SetRawAllocator(Allocator)
#define SET_POOL_PAGE_SIZE(ObjectType, NumAllocationsInPage) ObjectPool<ObjectType>::SetPageSize(NumAllocationsInPage)
#define NEW_POOL_OBJECT(ObjectType, Desc, ...)               ObjectPool<ObjectType>::GetPool().NewObject(Desc, __FILE__, __LINE__, ##__VA_ARGS__)
#define DESTROY_POOL_OBJECT(pObject)                         ObjectPool<std::remove_reference<decltype(*pObject)>::type>::GetPool().Destroy(pObject)

} // namespace Diligent
