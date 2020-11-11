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
/// Defines Diligent::StringPool class

#include <cstring>
#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/MemoryAllocator.h"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

/// Implementation of a simple fixed-size string pool
class StringPool
{
public:
    StringPool() {}

    // clang-format off
    StringPool           (const StringPool&) = delete;
    StringPool& operator=(const StringPool&) = delete;
    // clang-format on

    StringPool(StringPool&& Pool) :
        // clang-format off
        m_pBuffer     {Pool.m_pBuffer     },
        m_pCurrPtr    {Pool.m_pCurrPtr    },
        m_ReservedSize{Pool.m_ReservedSize},
        m_pAllocator  {Pool.m_pAllocator  }
    // clang-format on
    {
        Pool.m_pBuffer      = nullptr;
        Pool.m_pCurrPtr     = nullptr;
        Pool.m_ReservedSize = 0;
        Pool.m_pAllocator   = nullptr;
    }

    ~StringPool()
    {
        Release();
    }

    void Reserve(size_t Size, IMemoryAllocator& Allocator)
    {
        Release();

        VERIFY(m_ReservedSize == 0, "Pool is already initialized");
        m_pAllocator   = &Allocator;
        m_ReservedSize = Size;
        if (m_ReservedSize != 0)
        {
            m_pBuffer = reinterpret_cast<Char*>(m_pAllocator->Allocate(m_ReservedSize, "Memory for string pool", __FILE__, __LINE__));
        }
        m_pCurrPtr = m_pBuffer;
    }

    void Release()
    {
        if (m_pBuffer != nullptr && m_pAllocator != nullptr)
        {
            m_pAllocator->Free(m_pBuffer);
        }

        m_pBuffer      = nullptr;
        m_pCurrPtr     = nullptr;
        m_ReservedSize = 0;
        m_pAllocator   = nullptr;
    }

    static size_t GetRequiredReserveSize(const char* str)
    {
        return str != nullptr ? strlen(str) + 1 : 0;
    }

    static size_t GetRequiredReserveSize(const std::string& str)
    {
        return str.length() + 1;
    }

    void AssignMemory(Char* pBuffer, size_t Size)
    {
        VERIFY(m_ReservedSize == 0, "Pool is already initialized");
        m_ReservedSize = Size;
        m_pBuffer      = pBuffer;
        m_pCurrPtr     = m_pBuffer;
    }

    Char* Allocate(size_t Length)
    {
        VERIFY(m_pCurrPtr + Length <= m_pBuffer + m_ReservedSize, "Not enough space in the buffer");
        auto* Ptr = m_pCurrPtr;
        m_pCurrPtr += Length;
        return Ptr;
    }

    Char* CopyString(const String& Str)
    {
        auto  len = Str.length();
        auto* str = Allocate(len + 1);
        if (len != 0)
        {
            memcpy(str, Str.data(), len * sizeof(str[0]));
        }
        str[len] = 0;
        return str;
    }

    Char* CopyString(const char* Str)
    {
        if (Str == nullptr)
            return nullptr;

        auto* Ptr = m_pCurrPtr;
        while (*Str != 0 && m_pCurrPtr < m_pBuffer + m_ReservedSize)
        {
            *(m_pCurrPtr++) = *(Str++);
        }
        if (m_pCurrPtr < m_pBuffer + m_ReservedSize)
            *(m_pCurrPtr++) = 0;
        else
            UNEXPECTED("Not enough space reserved in the string pool");
        return Ptr;
    }


    size_t GetRemainingSize() const
    {
        VERIFY(m_pCurrPtr <= m_pBuffer + m_ReservedSize, "Buffer overflow");
        return m_ReservedSize - (m_pCurrPtr - m_pBuffer);
    }
    size_t GetUsedSize() const
    {
        VERIFY(m_pCurrPtr <= m_pBuffer + m_ReservedSize, "Buffer overflow");
        return m_pCurrPtr - m_pBuffer;
    }

private:
    Char*             m_pBuffer      = nullptr;
    Char*             m_pCurrPtr     = nullptr;
    size_t            m_ReservedSize = 0;
    IMemoryAllocator* m_pAllocator   = nullptr;
};

} // namespace Diligent
