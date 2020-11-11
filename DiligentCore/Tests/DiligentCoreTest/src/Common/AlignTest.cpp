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

#include <array>

#include "Align.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_Align, IsPowerOfTwo)
{
    for (Uint32 i = 0; i < 256; ++i)
    {
        bool IsPw2 = (i == 1 || i == 2 || i == 4 || i == 8 || i == 16 || i == 32 || i == 64 || i == 128);
        EXPECT_EQ(IsPowerOfTwo(static_cast<Uint8>(i)), IsPw2);
        EXPECT_EQ(IsPowerOfTwo(static_cast<Uint16>(i)), IsPw2);
        EXPECT_EQ(IsPowerOfTwo(i), IsPw2);
    }

    for (Uint32 bit = 0; bit < 32; ++bit)
    {
        auto Pw2 = Uint32{1} << bit;
        EXPECT_TRUE(IsPowerOfTwo(Pw2));
        EXPECT_EQ(IsPowerOfTwo(Pw2 + 1), Pw2 == 1);
        EXPECT_EQ(IsPowerOfTwo(Pw2 - 1), Pw2 == 2);
    }

    for (Uint64 bit = 0; bit < 64; ++bit)
    {
        auto Pw2 = Uint64{1} << bit;
        EXPECT_TRUE(IsPowerOfTwo(Pw2));
        EXPECT_EQ(IsPowerOfTwo(Pw2 + 1), Pw2 == 1);
        EXPECT_EQ(IsPowerOfTwo(Pw2 - 1), Pw2 == 2);
    }
}

TEST(Common_Align, Align)
{
    EXPECT_EQ(Align(Uint8{0}, Uint8{16}), Uint8{0});
    EXPECT_EQ(Align(Uint8{1}, Uint8{16}), Uint8{16});
    EXPECT_EQ(Align(Uint8{15}, Uint8{16}), Uint8{16});
    EXPECT_EQ(Align(Uint8{16}, Uint8{16}), Uint8{16});
    EXPECT_EQ(Align(Uint8{17}, Uint8{16}), Uint8{32});

    for (Uint32 i = 0; i < 1024; ++i)
    {
        constexpr Uint32 Alignment = 16;

        auto Aligned = (i / Alignment) * Alignment;
        if (Aligned < i) Aligned += Alignment;

        EXPECT_EQ(Align(i, Alignment), Aligned);
    }

    EXPECT_EQ(Align((Uint64{1} << 63) + 1, Uint64{1024}), (Uint64{1} << 63) + 1024);
}

TEST(Common_Align, AlignDown)
{
    EXPECT_EQ(AlignDown(Uint8{0}, Uint8{16}), Uint8{0});
    EXPECT_EQ(AlignDown(Uint8{1}, Uint8{16}), Uint8{0});
    EXPECT_EQ(AlignDown(Uint8{15}, Uint8{16}), Uint8{0});
    EXPECT_EQ(AlignDown(Uint8{16}, Uint8{16}), Uint8{16});
    EXPECT_EQ(AlignDown(Uint8{17}, Uint8{16}), Uint8{16});

    for (Uint32 i = 0; i < 1024; ++i)
    {
        constexpr Uint32 Alignment = 16;

        auto Aligned = (i / Alignment) * Alignment;

        EXPECT_EQ(AlignDown(i, Alignment), Aligned);
    }

    EXPECT_EQ(AlignDown((Uint64{1} << 63) + 1023, Uint64{1024}), (Uint64{1} << 63));
}

TEST(Common_Align, AlignPtr)
{
    EXPECT_EQ(Align((void*)0x1000, size_t{16}), (void*)0x1000);
    EXPECT_EQ(Align((void*)0x1001, size_t{16}), (void*)0x1010);
    EXPECT_EQ(Align((void*)0x100F, size_t{16}), (void*)0x1010);
    EXPECT_EQ(Align((void*)0x1010, size_t{16}), (void*)0x1010);
    EXPECT_EQ(Align((void*)0x1011, size_t{16}), (void*)0x1020);

    for (uintptr_t i = 0; i < 1024; ++i)
    {
        constexpr uintptr_t Alignment = 16;

        auto Aligned = (i / Alignment) * Alignment;
        if (Aligned < i) Aligned += Alignment;

        EXPECT_EQ(Align((void*)(0x1000 + i), (size_t)Alignment), (void*)(0x1000 + Aligned));
    }
}

TEST(Common_Align, AlignDownPtr)
{
    EXPECT_EQ(AlignDown((void*)0x1000, size_t{16}), (void*)0x1000);
    EXPECT_EQ(AlignDown((void*)0x1001, size_t{16}), (void*)0x1000);
    EXPECT_EQ(AlignDown((void*)0x100F, size_t{16}), (void*)0x1000);
    EXPECT_EQ(AlignDown((void*)0x1010, size_t{16}), (void*)0x1010);
    EXPECT_EQ(AlignDown((void*)0x1011, size_t{16}), (void*)0x1010);

    for (uintptr_t i = 0; i < 1024; ++i)
    {
        constexpr uintptr_t Alignment = 16;

        auto Aligned = (i / Alignment) * Alignment;

        EXPECT_EQ(AlignDown((void*)(0x1000 + i), (size_t)Alignment), (void*)(0x1000 + Aligned));
    }
}

} // namespace
