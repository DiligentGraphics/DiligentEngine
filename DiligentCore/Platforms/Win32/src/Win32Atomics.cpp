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

#include "Win32Atomics.hpp"

#include <Windows.h>

// The function returns the resulting INCREMENTED value.
WindowsAtomics::Long WindowsAtomics::AtomicIncrement(AtomicLong& Val)
{
    return InterlockedIncrement(&Val);
}

WindowsAtomics::Int64 WindowsAtomics::AtomicIncrement(AtomicInt64& Val)
{
    return InterlockedIncrement64(&Val);
}

// The function returns the resulting DECREMENTED value.
WindowsAtomics::Long WindowsAtomics::AtomicDecrement(AtomicLong& Val)
{
    return InterlockedDecrement(&Val);
}

WindowsAtomics::Int64 WindowsAtomics::AtomicDecrement(AtomicInt64& Val)
{
    return InterlockedDecrement64(&Val);
}

WindowsAtomics::Long WindowsAtomics::AtomicCompareExchange(AtomicLong& Destination, Long Exchange, Long Comparand)
{
    return InterlockedCompareExchange(&Destination, Exchange, Comparand);
}

WindowsAtomics::Long WindowsAtomics::AtomicAdd(AtomicLong& Destination, Long Val)
{
    return InterlockedAdd(&Destination, Val);
}

WindowsAtomics::Int64 WindowsAtomics::AtomicAdd(AtomicInt64& Destination, Int64 Val)
{
    return InterlockedAdd64(&Destination, Val);
}
