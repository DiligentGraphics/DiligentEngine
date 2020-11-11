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
/// Defines Diligent::DefaultRawMemoryAllocator class
#include <limits>

#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/MemoryAllocator.h"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

template <typename T>
typename std::enable_if<std::is_destructible<T>::value, void>::type Destruct(T* ptr)
{
    ptr->~T();
}

template <typename T>
typename std::enable_if<!std::is_destructible<T>::value, void>::type Destruct(T* ptr)
{
}

template <typename T, typename AllocatorType>
struct STDAllocator
{
    using value_type      = T;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    STDAllocator(AllocatorType& Allocator, const Char* Description, const Char* FileName, const Int32 LineNumber) noexcept :
        // clang-format off
        m_Allocator     {Allocator}
#ifdef DILIGENT_DEVELOPMENT
      , m_dvpDescription{Description}
      , m_dvpFileName   {FileName   }
      , m_dvpLineNumber {LineNumber }
#endif
    // clang-format on
    {
    }

    template <class U>
    STDAllocator(const STDAllocator<U, AllocatorType>& other) noexcept :
        // clang-format off
        m_Allocator     {other.m_Allocator}
#ifdef DILIGENT_DEVELOPMENT
      , m_dvpDescription{other.m_dvpDescription}
      , m_dvpFileName   {other.m_dvpFileName   }
      , m_dvpLineNumber {other.m_dvpLineNumber }
#endif
    // clang-format on
    {
    }

    template <class U>
    STDAllocator(STDAllocator<U, AllocatorType>&& other) noexcept :
        // clang-format off
        m_Allocator     {other.m_Allocator}
#ifdef DILIGENT_DEVELOPMENT
      , m_dvpDescription{other.m_dvpDescription}
      , m_dvpFileName   {other.m_dvpFileName   }
      , m_dvpLineNumber {other.m_dvpLineNumber }
#endif
    // clang-format on
    {
    }

    template <class U>
    STDAllocator& operator=(STDAllocator<U, AllocatorType>&& other) noexcept
    {
        // Android build requires this operator to be defined - I have no idea why.
        // There is no default constructor to create null allocator, so all fields must be
        // initialized.
        DEV_CHECK_ERR(&m_Allocator == &other.m_Allocator, "Inconsistent allocators");
#ifdef DILIGENT_DEVELOPMENT
        DEV_CHECK_ERR(m_dvpDescription == other.m_dvpDescription, "Incosistent allocator descriptions");
        DEV_CHECK_ERR(m_dvpFileName == other.m_dvpFileName, "Incosistent allocator file names");
        DEV_CHECK_ERR(m_dvpLineNumber == other.m_dvpLineNumber, "Incosistent allocator line numbers");
#endif
        return *this;
    }

    template <class U> struct rebind
    {
        typedef STDAllocator<U, AllocatorType> other;
    };

    T* allocate(std::size_t count)
    {
#ifndef DILIGENT_DEVELOPMENT
        static constexpr const char* m_dvpDescription = "<Unavailable in release build>";
        static constexpr const char* m_dvpFileName    = "<Unavailable in release build>";
        static constexpr Int32       m_dvpLineNumber  = -1;
#endif
        return reinterpret_cast<T*>(m_Allocator.Allocate(count * sizeof(T), m_dvpDescription, m_dvpFileName, m_dvpLineNumber));
    }

    pointer       address(reference r) { return &r; }
    const_pointer address(const_reference r) { return &r; }

    void deallocate(T* p, std::size_t count)
    {
        m_Allocator.Free(p);
    }

    inline size_type max_size() const
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    //    construction/destruction
    template <class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        ::new (p) U(std::forward<Args>(args)...);
    }

    inline void destroy(pointer p)
    {
        p->~T();
    }

    AllocatorType& m_Allocator;
#ifdef DILIGENT_DEVELOPMENT
    const Char* const m_dvpDescription;
    const Char* const m_dvpFileName;
    Int32 const       m_dvpLineNumber;
#endif
};

#define STD_ALLOCATOR(Type, AllocatorType, Allocator, Description) STDAllocator<Type, AllocatorType>(Allocator, Description, __FILE__, __LINE__)

template <class T, class U, class A>
bool operator==(const STDAllocator<T, A>& left, const STDAllocator<U, A>& right)
{
    return &left.m_Allocator == &right.m_Allocator;
}

template <class T, class U, class A>
bool operator!=(const STDAllocator<T, A>& left, const STDAllocator<U, A>& right)
{
    return !(left == right);
}

template <class T> using STDAllocatorRawMem = STDAllocator<T, IMemoryAllocator>;
#define STD_ALLOCATOR_RAW_MEM(Type, Allocator, Description) STDAllocatorRawMem<Type>(Allocator, Description, __FILE__, __LINE__)

template <class T, typename AllocatorType>
struct STDDeleter
{
    STDDeleter() noexcept {}

    STDDeleter(AllocatorType& Allocator) noexcept :
        m_Allocator{&Allocator}
    {}

    STDDeleter(const STDDeleter&) = default;
    STDDeleter& operator=(const STDDeleter&) = default;

    STDDeleter(STDDeleter&& rhs) noexcept :
        m_Allocator{rhs.m_Allocator}
    {
        rhs.m_Allocator = nullptr;
    }

    STDDeleter& operator=(STDDeleter&& rhs) noexcept
    {
        m_Allocator     = rhs.m_Allocator;
        rhs.m_Allocator = nullptr;
        return *this;
    }

    void operator()(T* ptr) noexcept
    {
        VERIFY(m_Allocator != nullptr, "The deleter has been moved away or never initialized, and can't be used");
        Destruct(ptr);
        m_Allocator->Free(ptr);
    }

private:
    AllocatorType* m_Allocator = nullptr;
};
template <class T> using STDDeleterRawMem = STDDeleter<T, IMemoryAllocator>;

} // namespace Diligent
