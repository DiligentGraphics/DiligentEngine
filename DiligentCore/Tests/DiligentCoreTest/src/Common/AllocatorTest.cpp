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

#include "DefaultRawMemoryAllocator.hpp"
#include "FixedBlockMemoryAllocator.hpp"
#include "LinearAllocator.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_FixedBlockMemoryAllocator, AllocDealloc)
{
    constexpr Uint32 AllocSize             = 32;
    constexpr Uint32 NumAllocationsPerPage = 16;

    FixedBlockMemoryAllocator TestAllocator(DefaultRawMemoryAllocator::GetAllocator(), AllocSize, NumAllocationsPerPage);

    void* Allocations[NumAllocationsPerPage][2] = {};
    for (int p = 0; p < 2; ++p)
    {
        for (int a = 1; a < NumAllocationsPerPage; ++a)
        {
            for (int i = 0; i < a; ++i)
                Allocations[i][p] = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);

            for (int i = a - 1; i >= 0; --i)
                TestAllocator.Free(Allocations[i][p]);

            for (int i = 0; i < a; ++i)
            {
                auto* NewAlloc = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);
                EXPECT_EQ(Allocations[i][p], NewAlloc);
            }

            for (int i = a - 1; i >= 0; --i)
                TestAllocator.Free(Allocations[i][p]);
        }
        for (int i = 0; i < NumAllocationsPerPage; ++i)
            Allocations[i][p] = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);
    }

    for (int p = 0; p < 2; ++p)
        for (int i = 0; i < NumAllocationsPerPage; ++i)
            TestAllocator.Free(Allocations[i][p]);

    for (int p = 0; p < 2; ++p)
        for (int i = 0; i < NumAllocationsPerPage; ++i)
            Allocations[i][p] = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);

    for (int p = 0; p < 2; ++p)
        for (int s = 0; s < 5; ++s)
            for (int i = s; i < NumAllocationsPerPage; i += 5)
                TestAllocator.Free(Allocations[i][p]);
}

TEST(Common_FixedBlockMemoryAllocator, SmallObject)
{
    constexpr Uint32 AllocSize             = 4;
    constexpr Uint32 NumAllocationsPerPage = 1;

    FixedBlockMemoryAllocator TestAllocator(DefaultRawMemoryAllocator::GetAllocator(), AllocSize, NumAllocationsPerPage);

    {
        void* pRawMem0 = TestAllocator.Allocate(AllocSize, "Small object allocation test", __FILE__, __LINE__);
        TestAllocator.Free(pRawMem0);
    }

    {
        void* pRawMem0 = TestAllocator.Allocate(AllocSize, "Small object allocation test", __FILE__, __LINE__);
        void* pRawMem1 = TestAllocator.Allocate(AllocSize, "Small object allocation test", __FILE__, __LINE__);
        TestAllocator.Free(pRawMem0);
        TestAllocator.Free(pRawMem1);
    }
}

TEST(Common_FixedBlockMemoryAllocator, UnalignedSize)
{
    constexpr Uint32 AllocSize             = 10;
    constexpr Uint32 NumAllocationsPerPage = 1;

    FixedBlockMemoryAllocator TestAllocator(DefaultRawMemoryAllocator::GetAllocator(), AllocSize, NumAllocationsPerPage);

    {
        void* pRawMem0 = TestAllocator.Allocate(AllocSize, "Unaligned-size object allocation test", __FILE__, __LINE__);
        TestAllocator.Free(pRawMem0);
    }

    {
        void* pRawMem0 = TestAllocator.Allocate(AllocSize, "Unaligned-size object allocation test", __FILE__, __LINE__);
        void* pRawMem1 = TestAllocator.Allocate(AllocSize, "Unaligned-size object allocation test", __FILE__, __LINE__);
        TestAllocator.Free(pRawMem0);
        TestAllocator.Free(pRawMem1);
    }
}

TEST(Common_LinearAllocator, EmptyAllocator)
{
    LinearAllocator Allocator{DefaultRawMemoryAllocator::GetAllocator()};
    Allocator.AddSpace(0, 16);
    Allocator.Reserve();
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{0});
    auto* pNull = Allocator.Allocate(0, 16);
    EXPECT_EQ(pNull, nullptr);
}

TEST(Common_LinearAllocator, LargeAlignment)
{
    LinearAllocator Allocator{DefaultRawMemoryAllocator::GetAllocator()};
    Allocator.AddSpace(32, 8192);
    Allocator.Reserve();
    auto* Ptr = Allocator.Allocate(32, 8192);
    EXPECT_EQ(Ptr, Align(Ptr, 8192));
}

TEST(Common_LinearAllocator, ObjectConstruction)
{
    LinearAllocator Allocator{DefaultRawMemoryAllocator::GetAllocator()};

    struct alignas(1024) TObj1k
    {
        char data[1024];
    };

    const std::string SrcStr = "123456789";

    Allocator.AddSpace<uint8_t>();
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{1});
    Allocator.AddSpace<uint16_t>();
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{1 + 1 + 2});
    Allocator.AddSpace(0, 16);
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{4});
    Allocator.AddSpaceForString(SrcStr);
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{4 + 10});
    Allocator.AddSpace<uint32_t>(5);
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{14 + 3 + 5 * 4});
    Allocator.AddSpace<uint64_t>(3);
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{37 + 4 + 3 * 8});
    Allocator.AddSpace(0, 16);
    EXPECT_EQ(Allocator.GetReservedSize(), size_t{65});
    Allocator.AddSpace<TObj1k>(4);

    Allocator.Reserve();

    {
        auto* pUI8 = Allocator.Construct<uint8_t>(uint8_t{15});
        EXPECT_EQ(pUI8, Align(pUI8, alignof(uint8_t)));
        EXPECT_EQ(*pUI8, uint8_t{15});
    }

    {
        auto* pUI16 = Allocator.Copy(uint16_t{31});
        EXPECT_EQ(pUI16, Align(pUI16, alignof(uint16_t)));
        EXPECT_EQ(*pUI16, uint16_t{31});
    }

    {
        auto* pNull = Allocator.Allocate(0, 16);
        EXPECT_EQ(pNull, nullptr);
    }

    {
        auto* DstStr = Allocator.CopyString(SrcStr.c_str());
        EXPECT_STREQ(DstStr, SrcStr.c_str());
    }

    {
        auto* pUI32 = Allocator.ConstructArray<uint32_t>(5, 100u);
        EXPECT_EQ(pUI32, Align(pUI32, alignof(uint32_t)));
        for (size_t i = 0; i < 5; ++i)
            EXPECT_EQ(pUI32[i], 100u);
    }

    {
        std::array<uint64_t, 3> RefArray = {11, 120, 1300};

        auto* pUI64 = Allocator.CopyArray<uint64_t>(RefArray.data(), RefArray.size());
        EXPECT_EQ(pUI64, Align(pUI64, alignof(uint64_t)));
        for (size_t i = 0; i < RefArray.size(); ++i)
            EXPECT_EQ(pUI64[i], RefArray[i]);
    }

    {
        auto* pNull = Allocator.Allocate(0, 16);
        EXPECT_EQ(pNull, nullptr);
    }

    {
        auto* pObj = Allocator.Allocate<TObj1k>(4);
        EXPECT_EQ(pObj, Align(pObj, alignof(TObj1k)));
    }
}

} // namespace
