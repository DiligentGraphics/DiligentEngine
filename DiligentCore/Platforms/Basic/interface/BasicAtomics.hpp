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

#include <atomic>

struct BasicAtomics
{
    using Long        = long;
    using AtomicLong  = std::atomic<Long>;
    using Int64       = int64_t;
    using AtomicInt64 = std::atomic<Int64>;

    // The function returns the resulting INCREMENTED value.
    template <typename Type>
    static inline Type AtomicIncrement(std::atomic<Type>& Val)
    {
        return ++Val;
    }

    // The function returns the resulting DECREMENTED value.
    template <typename Type>
    static inline Type AtomicDecrement(std::atomic<Type>& Val)
    {
        return --Val;
    }

    // The function compares the Destination value with the Comparand value. If the Destination value is equal
    // to the Comparand value, the Exchange value is stored in the address specified by Destination.
    // Otherwise, no operation is performed.
    // The function returns the initial value of the Destination parameter
    template <typename Type>
    static inline Type AtomicCompareExchange(std::atomic<Type>& Destination, Type Exchange, Type Comparand)
    {
        Destination.compare_exchange_strong(Comparand, Exchange);
        return Comparand;
    }

    template <typename Type>
    static inline Type AtomicAdd(std::atomic<Type>& Destination, Type Val)
    {
        return std::atomic_fetch_add(&Destination, Val);
    }
};
