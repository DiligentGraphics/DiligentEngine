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
/// Defines Diligent::LinearAllocator class

#include <vector>

#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/MemoryAllocator.h"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "Align.hpp"

namespace Diligent
{

/// Implementation of a linear allocator on a fixed-size memory page
class LinearAllocator
{
public:
    // clang-format off
    LinearAllocator           (const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator& operator=(LinearAllocator&&)      = delete;
    // clang-format on

    explicit LinearAllocator(IMemoryAllocator& Allocator) noexcept :
        m_pAllocator{&Allocator}
    {}

    LinearAllocator(LinearAllocator&& Other) noexcept :
        // clang-format off
        m_pDataStart   {Other.m_pDataStart   },
        m_pCurrPtr     {Other.m_pCurrPtr     },
        m_ReservedSize {Other.m_ReservedSize },
        m_CurrAlignment{Other.m_CurrAlignment},
        m_pAllocator   {Other.m_pAllocator   }
    // clang-format on
    {
        Other.Reset();
    }

    ~LinearAllocator()
    {
        Free();
    }

    void Free()
    {
        if (m_pDataStart != nullptr && m_pAllocator != nullptr)
        {
            m_pAllocator->Free(m_pDataStart);
        }
        Reset();
    }

    void* Release()
    {
        void* Ptr = m_pDataStart;
        Reset();
        return Ptr;
    }

    void* ReleaseOwnership() noexcept
    {
        m_pAllocator = nullptr;
        return GetDataPtr();
    }

    void* GetDataPtr() const noexcept
    {
        return m_pDataStart;
    }

    void AddSpace(size_t size, size_t alignment) noexcept
    {
        VERIFY(m_pDataStart == nullptr, "Memory has already been allocated");
        VERIFY(IsPowerOfTwo(alignment), "Alignment is not a power of two!");

        if (size == 0)
            return;

        if (m_CurrAlignment == 0)
        {
            VERIFY(m_ReservedSize == 0, "This is expected to be a very first time the space is added");
            m_CurrAlignment = sizeof(void*);
        }

        if (alignment > m_CurrAlignment)
        {
            // Reserve extra space that may be needed for alignment
            m_ReservedSize += alignment - m_CurrAlignment;
        }
        m_CurrAlignment = alignment;

        size = Align(size, alignment);
        m_ReservedSize += size;

#if DILIGENT_DEBUG
        m_DbgAllocations.emplace_back(size, alignment, m_ReservedSize);
#endif
    }

    template <typename T>
    void AddSpace(size_t count = 1) noexcept
    {
        AddSpace(sizeof(T) * count, alignof(T));
    }

    void AddSpaceForString(const Char* str) noexcept
    {
        VERIFY_EXPR(str != nullptr);
        AddSpace(strlen(str) + 1, 1);
    }

    void AddSpaceForString(const String& str) noexcept
    {
        AddSpaceForString(str.c_str());
    }

    void Reserve(size_t size)
    {
        VERIFY(m_pDataStart == nullptr, "Memory has already been allocated");
        VERIFY(m_ReservedSize == 0, "Space has been added to the allocator and will be overriden");
        m_ReservedSize = size;
        Reserve();
    }

    void Reserve()
    {
        VERIFY(m_pDataStart == nullptr, "Memory has already been allocated");
        VERIFY(m_pAllocator != nullptr, "Allocator must not be null");
        // Make sure the data size is at least sizeof(void*)-aligned
        m_ReservedSize = Align(m_ReservedSize, sizeof(void*));
        if (m_ReservedSize > 0)
        {
            m_pDataStart = reinterpret_cast<uint8_t*>(m_pAllocator->Allocate(m_ReservedSize, "Raw memory for linear allocator", __FILE__, __LINE__));
            VERIFY(m_pDataStart == Align(m_pDataStart, sizeof(void*)), "Memory pointer must be at least sizeof(void*)-aligned");

            m_pCurrPtr = m_pDataStart;
        }
        m_CurrAlignment = sizeof(void*);
    }

    void* Allocate(size_t size, size_t alignment)
    {
        VERIFY(size == 0 || m_pDataStart != nullptr, "Memory has not been allocated");
        VERIFY(IsPowerOfTwo(alignment), "Alignment is not a power of two!");

        if (size == 0)
            return nullptr;

        size = Align(size, alignment);

#if DILIGENT_DEBUG
        VERIFY(m_DbgCurrAllocation < m_DbgAllocations.size(), "Allocation number exceed the number of allocations that were originally reserved.");
        const auto& CurrAllocation = m_DbgAllocations[m_DbgCurrAllocation++];
        VERIFY(CurrAllocation.size == size, "Allocation size (", size, ") does not match the initially requested size (", CurrAllocation.size, ")");
        VERIFY(CurrAllocation.alignment == alignment, "Allocation alignment (", alignment, ") does not match the initially requested alignment (", CurrAllocation.alignment, ")");
#endif

        VERIFY(Align(m_pCurrPtr, m_CurrAlignment) == m_pCurrPtr, "Current pointer is not aligned as expected");
        m_pCurrPtr      = Align(m_pCurrPtr, alignment);
        m_CurrAlignment = alignment;

        VERIFY(m_pCurrPtr + size <= m_pDataStart + CurrAllocation.reserved_size,
               "Allocation size exceeds the initially reserved space. This is likely a bug.");

        auto* ptr = m_pCurrPtr;
        m_pCurrPtr += size;

        VERIFY(m_pCurrPtr <= m_pDataStart + m_ReservedSize, "Allocation size exceeds the reserved space");

        return ptr;
    }

    template <typename T>
    T* Allocate(size_t count = 1)
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
    }

    template <typename T, typename... Args>
    T* Construct(Args&&... args)
    {
        T* Ptr = Allocate<T>();
        new (Ptr) T{std::forward<Args>(args)...};
        return Ptr;
    }

    template <typename T, typename... Args>
    T* ConstructArray(size_t count, const Args&... args)
    {
        T* Ptr = Allocate<T>(count);
        for (size_t i = 0; i < count; ++i)
        {
            new (Ptr + i) T{args...};
        }
        return Ptr;
    }

    template <typename T>
    T* Copy(const T& Src)
    {
        return Construct<T>(Src);
    }

    template <typename T>
    T* CopyArray(const T* Src, size_t count)
    {
        T* Dst = Allocate<T>(count);
        for (size_t i = 0; i < count; ++i)
        {
            new (Dst + i) T{Src[i]};
        }
        return Dst;
    }

    Char* CopyString(const char* Str)
    {
        if (Str == nullptr)
            return nullptr;

        auto* Ptr = reinterpret_cast<Char*>(Allocate(strlen(Str) + 1, 1));
        Char* Dst = Ptr;

        const auto* pDataEnd = reinterpret_cast<Char*>(m_pDataStart) + m_ReservedSize;
        while (*Str != 0 && Dst < pDataEnd)
        {
            *(Dst++) = *(Str++);
        }
        if (Dst < pDataEnd)
            *(Dst++) = 0;
        else
            UNEXPECTED("Not enough space reserved for the string");

        VERIFY_EXPR(reinterpret_cast<Char*>(m_pCurrPtr) == Dst);
        return Ptr;
    }

    Char* CopyString(const std::string& Str)
    {
        return CopyString(Str.c_str());
    }

    size_t GetCurrentSize() const
    {
        return static_cast<size_t>(m_pCurrPtr - m_pDataStart);
    }

    size_t GetReservedSize() const
    {
        return m_ReservedSize;
    }

private:
    void Reset()
    {
        m_pDataStart    = nullptr;
        m_pCurrPtr      = nullptr;
        m_ReservedSize  = 0;
        m_CurrAlignment = 0;
        m_pAllocator    = nullptr;
    }

    uint8_t*          m_pDataStart    = nullptr;
    uint8_t*          m_pCurrPtr      = nullptr;
    size_t            m_ReservedSize  = 0;
    size_t            m_CurrAlignment = 0;
    IMemoryAllocator* m_pAllocator    = nullptr;

#if DILIGENT_DEBUG
    size_t m_DbgCurrAllocation = 0;
    struct DbgAllocationInfo
    {
        const size_t size;
        const size_t alignment;
        const size_t reserved_size;

        DbgAllocationInfo(size_t _size, size_t _alignment, size_t _reserved_size) :
            size{_size},
            alignment{_alignment},
            reserved_size{_reserved_size}
        {
        }
    };
    std::vector<DbgAllocationInfo> m_DbgAllocations;
#endif
};

} // namespace Diligent
