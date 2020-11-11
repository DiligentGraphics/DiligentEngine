/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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

#include "../../Basic/interface/BasicPlatformMisc.hpp"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"

struct LinuxMisc : public BasicPlatformMisc
{
    static Diligent::Uint32 GetMSB(Diligent::Uint32 Val)
    {
        if (Val == 0) return 32;

        // Returns the number of leading 0-bits in x, starting at the
        // most significant bit position. If x is 0, the result is undefined.
        auto LeadingZeros = __builtin_clz(Val);
        auto MSB          = 31 - LeadingZeros;
        VERIFY_EXPR(MSB == BasicPlatformMisc::GetMSB(Val));

        return MSB;
    }

    static Diligent::Uint32 GetLSB(Diligent::Uint32 Val)
    {
        if (Val == 0) return 32;

        // Returns the number of trailing 0-bits in x, starting at the
        // least significant bit position. If x is 0, the result is undefined.
        auto TrailingZeros = __builtin_ctz(Val);
        auto LSB           = TrailingZeros;
        VERIFY_EXPR(LSB == BasicPlatformMisc::GetLSB(Val));

        return LSB;
    }

    static Diligent::Uint32 GetMSB(Diligent::Uint64 Val)
    {
        if (Val == 0) return 64;

        // Returns the number of leading 0-bits in x, starting at the
        // most significant bit position. If x is 0, the result is undefined.
        auto LeadingZeros = __builtin_clzll(Val);
        auto MSB          = 63 - LeadingZeros;
        VERIFY_EXPR(MSB == BasicPlatformMisc::GetMSB(Val));

        return MSB;
    }

    static Diligent::Uint32 GetLSB(Diligent::Uint64 Val)
    {
        if (Val == 0) return 64;

        // Returns the number of trailing 0-bits in x, starting at the
        // least significant bit position. If x is 0, the result is undefined.
        auto TrailingZeros = __builtin_ctzll(Val);
        auto LSB           = TrailingZeros;
        VERIFY_EXPR(LSB == BasicPlatformMisc::GetLSB(Val));

        return LSB;
    }

    static Diligent::Uint32 CountOneBits(Diligent::Uint32 Val)
    {
        // Returns the number of 1-bits in x.
        auto bits = __builtin_popcount(Val);
        VERIFY_EXPR(bits == BasicPlatformMisc::CountOneBits(Val));
        return bits;
    }

    static Diligent::Uint32 CountOneBits(Diligent::Uint64 Val)
    {
        // Returns the number of 1-bits in x.
        auto bits = __builtin_popcountll(Val);
        VERIFY_EXPR(bits == BasicPlatformMisc::CountOneBits(Val));
        return bits;
    }
};
