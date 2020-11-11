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

#include "VariableSizeGPUAllocationsManager.hpp"
#include "DefaultRawMemoryAllocator.hpp"
#include "PlatformDefinitions.h"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(GraphicsAccessories_VariableSizeGPUAllocationsManager, AllocateFree)
{
    auto& Allocator = DefaultRawMemoryAllocator::GetAllocator();

    using OffsetType = VariableSizeAllocationsManager::OffsetType;

    {
        VariableSizeAllocationsManager ListMgr(128, Allocator);
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), size_t{1});

        auto a1 = ListMgr.Allocate(17, 4);
        EXPECT_EQ(a1.UnalignedOffset, OffsetType{0});
        EXPECT_EQ(a1.Size, OffsetType{20});
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), size_t{1});

        auto a2 = ListMgr.Allocate(17, 8);
        EXPECT_EQ(a2.UnalignedOffset, OffsetType{20});
        EXPECT_EQ(a2.Size, OffsetType{28});

        auto a3 = ListMgr.Allocate(8, 1);
        EXPECT_EQ(a3.UnalignedOffset, OffsetType{48});
        EXPECT_EQ(a3.Size, OffsetType{8});

        auto a4 = ListMgr.Allocate(11, 8);
        EXPECT_EQ(a4.UnalignedOffset, OffsetType{56});
        EXPECT_EQ(a4.Size, OffsetType{16});

        auto a5 = ListMgr.Allocate(64, 1);
        EXPECT_FALSE(a5.IsValid());
        EXPECT_EQ(a5.Size, OffsetType{0});

        a5 = ListMgr.Allocate(16, 1);
        EXPECT_EQ(a5.UnalignedOffset, OffsetType{72});
        EXPECT_EQ(a5.Size, OffsetType{16});

        auto a6 = ListMgr.Allocate(8, 1);
        EXPECT_EQ(a6.UnalignedOffset, OffsetType{88});
        EXPECT_EQ(a6.Size, OffsetType{8});

        auto a7 = ListMgr.Allocate(16, 1);
        EXPECT_EQ(a7.UnalignedOffset, OffsetType{96});
        EXPECT_EQ(a7.Size, OffsetType{16});

        auto a8 = ListMgr.Allocate(8, 1);
        EXPECT_EQ(a8.UnalignedOffset, OffsetType{112});
        EXPECT_EQ(a8.Size, OffsetType{8});
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{1});

        auto a9 = ListMgr.Allocate(8, 1);
        EXPECT_EQ(a9.UnalignedOffset, OffsetType{120});
        EXPECT_EQ(a9.Size, OffsetType{8});
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{0});

        EXPECT_TRUE(ListMgr.IsFull());

        ListMgr.Free(std::move(a6));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{1});

        ListMgr.Free(a8.UnalignedOffset, a8.Size);
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{2});

        ListMgr.Free(std::move(a9));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{2});

        auto a10 = ListMgr.Allocate(16, 1);
        EXPECT_EQ(a10.UnalignedOffset, OffsetType{112});
        EXPECT_EQ(a10.Size, OffsetType{16});
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{1});

        ListMgr.Free(a10.UnalignedOffset, a10.Size);
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{2});

        ListMgr.Free(std::move(a7));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{1});

        ListMgr.Free(std::move(a4));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{2});

        ListMgr.Free(a2.UnalignedOffset, a2.Size);
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{3});

        ListMgr.Free(std::move(a1));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{3});

        ListMgr.Free(std::move(a3));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{2});

        ListMgr.Free(std::move(a5));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), OffsetType{1});

        EXPECT_TRUE(ListMgr.IsEmpty());
    }

    {
        VariableSizeAllocationsManager ListMgr(128, Allocator);

        auto a1 = ListMgr.Allocate(64, 1);
        EXPECT_EQ(a1.UnalignedOffset, OffsetType{0});
        EXPECT_EQ(a1.Size, OffsetType{64});
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), size_t{1});

        auto a2 = ListMgr.Allocate(128, 1);
        EXPECT_EQ(a2, VariableSizeAllocationsManager::Allocation::InvalidAllocation());

        ListMgr.Extend(128);
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), size_t{1});

        a2 = ListMgr.Allocate(128, 1);
        EXPECT_EQ(a2.UnalignedOffset, OffsetType{64});
        EXPECT_EQ(a2.Size, OffsetType{128});

        auto a3 = ListMgr.Allocate(64, 1);
        EXPECT_TRUE(ListMgr.IsFull());

        ListMgr.Extend(32);
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), size_t{1});

        auto a4 = ListMgr.Allocate(32, 1);
        EXPECT_TRUE(ListMgr.IsFull());

        ListMgr.Free(std::move(a1));
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), size_t{1});

        ListMgr.Extend(1024);
        EXPECT_EQ(ListMgr.GetNumFreeBlocks(), size_t{2});

        auto a5 = ListMgr.Allocate(512, 1);

        ListMgr.Free(std::move(a4));
        ListMgr.Free(std::move(a2));
        ListMgr.Free(std::move(a5));
        ListMgr.Free(std::move(a3));
    }
}

TEST(GraphicsAccessories_VariableSizeGPUAllocationsManager, FreeOrder)
{
    auto& Allocator  = DefaultRawMemoryAllocator::GetAllocator();
    using OffsetType = VariableSizeAllocationsManager::OffsetType;

    {
        const auto NumAllocs = 6;
        int        NumPerms  = 0;
        size_t     ReleaseOrder[NumAllocs];
        for (size_t a = 0; a < NumAllocs; ++a)
            ReleaseOrder[a] = a;
        do
        {
            ++NumPerms;
            VariableSizeAllocationsManager ListMgr(NumAllocs * 4, Allocator);

            VariableSizeAllocationsManager::Allocation allocs[NumAllocs];
            for (size_t a = 0; a < NumAllocs; ++a)
            {
                allocs[a] = ListMgr.Allocate(4, 1);
                EXPECT_EQ(allocs[a].UnalignedOffset, a * 4);
                EXPECT_EQ(allocs[a].Size, OffsetType{4});
            }
            for (size_t a = 0; a < NumAllocs; ++a)
            {
                ListMgr.Free(std::move(allocs[ReleaseOrder[a]]));
            }
        } while (std::next_permutation(std::begin(ReleaseOrder), std::end(ReleaseOrder)));
        EXPECT_EQ(NumPerms, 720);
    }
}

TEST(GraphicsAccessories_VariableSizeGPUAllocationsManager, Free)
{
    auto& Allocator = DefaultRawMemoryAllocator::GetAllocator();
    {
        VariableSizeGPUAllocationsManager ListMgr(128, Allocator);

        VariableSizeGPUAllocationsManager::Allocation al[16];
        for (size_t o = 0; o < _countof(al); ++o)
            al[o] = ListMgr.Allocate(8, 4);
        EXPECT_TRUE(ListMgr.IsFull());

        ListMgr.Free(std::move(al[1]), 0);
        ListMgr.Free(std::move(al[5]), 0);
        ListMgr.Free(std::move(al[4]), 0);
        ListMgr.Free(std::move(al[3]), 0);

        ListMgr.Free(al[10].UnalignedOffset, al[10].Size, 1);
        ListMgr.Free(al[13].UnalignedOffset, al[13].Size, 1);
        ListMgr.Free(al[2].UnalignedOffset, al[2].Size, 1);
        ListMgr.Free(al[8].UnalignedOffset, al[8].Size, 1);

        ListMgr.ReleaseStaleAllocations(1);

        ListMgr.Free(std::move(al[14]), 2);
        ListMgr.Free(std::move(al[7]), 2);
        ListMgr.Free(std::move(al[0]), 2);
        ListMgr.Free(std::move(al[9]), 2);

        ListMgr.ReleaseStaleAllocations(2);

        ListMgr.Free(std::move(al[12]), 1);
        ListMgr.Free(std::move(al[15]), 1);
        ListMgr.Free(std::move(al[6]), 1);
        ListMgr.Free(std::move(al[11]), 1);

        ListMgr.ReleaseStaleAllocations(3);
    }
}

} // namespace
