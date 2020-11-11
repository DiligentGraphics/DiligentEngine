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

#include "FastRand.hpp"

#include <vector>
#include <unordered_map>

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_FastRand, Distribution)
{
    static_assert(FastRand::Max < 32768, "FastRand max value is probably to0 large for this test");
    std::vector<int> Counter(FastRand::Max + 1);

    FastRand Rnd(1);
    for (unsigned i = 0; i < FastRand::Max / 4; ++i)
    {
        auto x = Rnd();
        ++Counter[x];
        EXPECT_LE(Counter[x], 4) << x << " occured total " << Counter[x] << " times";
    }
}

TEST(Common_FastRand, FastRandFloat)
{
    std::unordered_map<float, int> Counter;

    FastRandFloat Rnd(1, -1.f, +1.f);
    for (unsigned i = 0; i < FastRand::Max / 4; ++i)
    {
        auto x = Rnd();
        EXPECT_GE(x, -1.f);
        EXPECT_LE(x, 1.f);

        ++Counter[x];
        EXPECT_LE(Counter[x], 4) << x << " occured total " << Counter[x] << " times";
    }
}

TEST(Common_FastRand, FastRandInt)
{
    constexpr int Min = -128;
    constexpr int Max = +256;

    std::vector<int> Counter(Max - Min + 1);

    FastRandInt Rnd(1, Min, Max);
    for (unsigned i = 0; i < (Max - Min) / 4; ++i)
    {
        auto x = Rnd();
        EXPECT_GE(x, Min);
        EXPECT_LE(x, Max);

        ++Counter[x - Min];
        EXPECT_LE(Counter[x - Min], 2) << x << " occured total " << Counter[x - Min] << " times";
    }
}

} // namespace
