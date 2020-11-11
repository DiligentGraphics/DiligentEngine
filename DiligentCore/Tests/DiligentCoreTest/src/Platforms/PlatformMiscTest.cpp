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

#include "PlatformMisc.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Platforms_PlatformMisc, GetMsbLsb)
{
    EXPECT_EQ(PlatformMisc::GetMSB(Uint32{0}), Uint32{32});
    for (Uint32 i = 0; i < 32; ++i)
    {
        auto MSB = PlatformMisc::GetMSB((Uint32{1} << i) | 1);
        EXPECT_EQ(MSB, i);
    }

    EXPECT_EQ(PlatformMisc::GetMSB(Uint64{0}), Uint64{64});
    for (Uint32 i = 0; i < 64; ++i)
    {
        auto MSB = PlatformMisc::GetMSB((Uint64{1} << i) | 1);
        EXPECT_EQ(MSB, i);
    }

    EXPECT_EQ(PlatformMisc::GetLSB(Uint32{0}), Uint32{32});
    for (Uint32 i = 0; i < 32; ++i)
    {
        auto LSB = PlatformMisc::GetLSB((Uint32{1} << i) | (Uint32{1} << 31));
        EXPECT_EQ(LSB, i);
    }

    EXPECT_EQ(PlatformMisc::GetLSB(Uint64{0}), Uint64{64});
    for (Uint32 i = 0; i < 64; ++i)
    {
        auto LSB = PlatformMisc::GetLSB((Uint64{1} << i) | (Uint64{1} << 63));
        EXPECT_EQ(LSB, i);
    }
}

TEST(Platforms_PlatformMisc, CountOneBits)
{
    EXPECT_EQ(PlatformMisc::CountOneBits(Uint32{0}), Uint32{0});
    EXPECT_EQ(PlatformMisc::CountOneBits(Uint64{0}), Uint64{0});
    EXPECT_EQ(PlatformMisc::CountOneBits(Uint32{1}), Uint32{1});
    EXPECT_EQ(PlatformMisc::CountOneBits(Uint64{1}), Uint64{1});
    EXPECT_EQ(PlatformMisc::CountOneBits(Uint32{7}), Uint32{3});
    EXPECT_EQ(PlatformMisc::CountOneBits(Uint64{7}), Uint64{3});
    EXPECT_EQ(PlatformMisc::CountOneBits((Uint32{1} << 31) | (Uint32{1} << 15)), Uint32{2});
    EXPECT_EQ(PlatformMisc::CountOneBits((Uint64{1} << 63) | (Uint32{1} << 31)), Uint64{2});
    EXPECT_EQ(PlatformMisc::CountOneBits((Uint32{1} << 31) - 1), Uint32{31});
    EXPECT_EQ(PlatformMisc::CountOneBits((Uint64{1} << 63) - 1), Uint64{63});
}

} // namespace
