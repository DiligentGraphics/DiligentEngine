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
/// Implementation of the Diligent::BufferBase template class

#include "MemoryAllocator.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

/// Sets raw memory allocator. This function must be called before any memory allocation/deallocation function
/// is called.
void SetRawAllocator(IMemoryAllocator* pRawAllocator);

/// Returns raw memory allocator
IMemoryAllocator& GetRawAllocator();

IMemoryAllocator& GetStringAllocator();

#define ALLOCATE_RAW(Allocator, Desc, Size)    (Allocator).Allocate(Size, Desc, __FILE__, __LINE__)
#define ALLOCATE(Allocator, Desc, Type, Count) reinterpret_cast<Type*>(ALLOCATE_RAW(Allocator, Desc, sizeof(Type) * (Count)))
#define FREE(Allocator, Ptr)                   Allocator.Free(Ptr)

DILIGENT_END_NAMESPACE // namespace Diligent


#if 0
inline void* operator new(size_t Size, const char* dbgDescription, const char* dbgFileName, const int dbgLineNumber)
{ 
    return Diligent::GetRawAllocator().Allocate(Size, dbgDescription, dbgFileName, dbgLineNumber); 
}

inline void* operator new[](size_t Size, const char* dbgDescription, const char* dbgFileName, const int dbgLineNumber) 
{ 
    return Diligent::GetRawAllocator().Allocate(Size, dbgDescription, dbgFileName, dbgLineNumber); 
}

// Arguments of the delete operator must exactly match the arguments of new
inline void operator delete(void* Ptr, const char* dbgDescription, const char* dbgFileName, const int dbgLineNumber)
{ 
    Diligent::GetRawAllocator().Free(Ptr); 
}

inline void operator delete[](void* Ptr, const char* dbgDescription, const char* dbgFileName, const int dbgLineNumber)
{ 
    Diligent::GetRawAllocator().Free(Ptr); 
}

#    define MemAlloc(Size, Desc) GetRawAllocator().Allocate(Size, Desc, __FILE__, __LINE__)
#    define MemFree(Ptr)         GetRawAllocator().Free(Ptr)
#    define New(Desc)            new (Desc, __FILE__, __LINE__)
#    define Delete               delete


#endif