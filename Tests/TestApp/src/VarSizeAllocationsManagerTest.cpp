/*     Copyright 2015-2019 Egor Yusov
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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "VariableSizeGPUAllocationsManager.h"
#include "DefaultRawMemoryAllocator.h"
#include "DebugUtilities.h"
#include "UnitTestBase.h"

using namespace Diligent;

class VariableSizeAllocationsManagerTest : public UnitTestBase
{
public:
    VariableSizeAllocationsManagerTest();
};

static VariableSizeAllocationsManagerTest TheVariableSizeAllocationsManagerTest;

VariableSizeAllocationsManagerTest::VariableSizeAllocationsManagerTest() :
    UnitTestBase("Variable size allocation manager test")
{
    auto &Allocator = DefaultRawMemoryAllocator::GetAllocator();

    {
        VariableSizeAllocationsManager ListMgr(128, Allocator);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        auto a1 = ListMgr.Allocate(17, 4);
        VERIFY_EXPR(a1.UnalignedOffset == 0 && a1.Size == 20);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        auto a2 = ListMgr.Allocate(17, 8);
        VERIFY_EXPR(a2.UnalignedOffset == 20 && a2.Size == 28);

        auto a3 = ListMgr.Allocate(8, 1);
        VERIFY_EXPR(a3.UnalignedOffset == 48 && a3.Size == 8);

        auto a4 = ListMgr.Allocate(11, 8);
        VERIFY_EXPR(a4.UnalignedOffset == 56 && a4.Size == 16);

        auto a5 = ListMgr.Allocate(64, 1);
        VERIFY_EXPR(!a5.IsValid() && a5.Size == 0);

        a5 = ListMgr.Allocate(16, 1);
        VERIFY_EXPR(a5.UnalignedOffset == 72 && a5.Size == 16);

        auto a6 = ListMgr.Allocate(8, 1);
        VERIFY_EXPR(a6.UnalignedOffset == 88 && a6.Size == 8);

        auto a7 = ListMgr.Allocate(16, 1);
        VERIFY_EXPR(a7.UnalignedOffset == 96 && a7.Size == 16);

        auto a8 = ListMgr.Allocate(8, 1);
        VERIFY_EXPR(a8.UnalignedOffset == 112 && a8.Size == 8);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        auto a9 = ListMgr.Allocate(8, 1);
        VERIFY_EXPR(a9.UnalignedOffset == 120 && a9.Size == 8);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 0);

        VERIFY_EXPR(ListMgr.IsFull());

        ListMgr.Free(std::move(a6));
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        ListMgr.Free(a8.UnalignedOffset, a8.Size);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(std::move(a9));
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        auto a10 = ListMgr.Allocate(16, 1);
        VERIFY_EXPR(a10.UnalignedOffset == 112 && a10.Size == 16);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        ListMgr.Free(a10.UnalignedOffset, a10.Size);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(std::move(a7));
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        ListMgr.Free(std::move(a4));
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(a2.UnalignedOffset, a2.Size);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 3);

        ListMgr.Free(std::move(a1));
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 3);

        ListMgr.Free(std::move(a3));
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(std::move(a5));
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        VERIFY_EXPR(ListMgr.IsEmpty());
    }

    {
        const auto NumAllocs = 6;
        int NumPerms = 0;
        size_t ReleaseOrder[NumAllocs];
        for(size_t a=0; a < NumAllocs; ++a)
            ReleaseOrder[a] = a;
        do
        {
            ++NumPerms;
            VariableSizeAllocationsManager ListMgr(NumAllocs*4, Allocator);
            VariableSizeAllocationsManager::Allocation allocs[NumAllocs];
            for(size_t a=0; a < NumAllocs; ++a)
            {
                allocs[a] = ListMgr.Allocate(4, 1);
                VERIFY_EXPR(allocs[a].UnalignedOffset == a*4 && allocs[a].Size == 4);
            }
            for(size_t a=0; a < NumAllocs; ++a)
            {
                ListMgr.Free(std::move(allocs[ReleaseOrder[a]]));
            }
        } while(std::next_permutation(std::begin(ReleaseOrder), std::end(ReleaseOrder)));        
        VERIFY_EXPR(NumPerms == 720);
    }

    {
        VariableSizeGPUAllocationsManager ListMgr(128, Allocator);
        VariableSizeGPUAllocationsManager::Allocation al[16];
        for(size_t o=0; o < _countof(al); ++o)
            al[o] = ListMgr.Allocate(8, 4);
        VERIFY_EXPR(ListMgr.IsFull());

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

    SetStatus(TestResult::Succeeded);
}
