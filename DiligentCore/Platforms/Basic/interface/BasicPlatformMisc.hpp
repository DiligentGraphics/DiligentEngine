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

#include "../../../Primitives/interface/BasicTypes.h"

struct BasicPlatformMisc
{
    template <typename Type>
    static Diligent::Uint32 GetMSB(Type Val)
    {
        if (Val == 0) return sizeof(Type) * 8;

        Diligent::Uint32 MSB = sizeof(Type) * 8 - 1;
        while (!(Val & (Type{1} << MSB)))
            --MSB;

        return MSB;
    }

    template <typename Type>
    static Diligent::Uint32 GetLSB(Type Val)
    {
        if (Val == 0) return sizeof(Type) * 8;

        Diligent::Uint32 LSB = 0;
        while (!(Val & (Type{1} << LSB)))
            ++LSB;

        return LSB;
    }

    template <typename Type>
    static Diligent::Uint32 CountOneBits(Type Val)
    {
        Diligent::Uint32 bits = 0;
        while (Val != 0)
        {
            Val &= (Val - 1);
            ++bits;
        }
        return bits;
    }
};
