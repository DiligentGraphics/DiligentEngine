/*     Copyright 2015-2017 Egor Yusov
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

        auto o1 = ListMgr.Allocate(16);
        VERIFY_EXPR(o1 == 0);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        auto o2 = ListMgr.Allocate(32);
        VERIFY_EXPR(o2 == 16);

        auto o3 = ListMgr.Allocate(8);
        VERIFY_EXPR(o3 == 48);

        auto o4 = ListMgr.Allocate(16);
        VERIFY_EXPR(o4 == 56);

        auto o5 = ListMgr.Allocate(64);
        VERIFY_EXPR(o5 == VariableSizeAllocationsManager::InvalidOffset);

        o5 = ListMgr.Allocate(16);
        VERIFY_EXPR(o5 == 72);

        auto o6 = ListMgr.Allocate(8);
        VERIFY_EXPR(o6 == 88);

        auto o7 = ListMgr.Allocate(16);
        VERIFY_EXPR(o7 == 96);

        auto o8 = ListMgr.Allocate(8);
        VERIFY_EXPR(o8 == 112);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        auto o9 = ListMgr.Allocate(8);
        VERIFY_EXPR(o9 == 120);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 0);

        VERIFY_EXPR(ListMgr.IsFull());

        ListMgr.Free(o6, 8);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        ListMgr.Free(o8, 8);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(o9, 8);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        auto o10 = ListMgr.Allocate(16);
        VERIFY_EXPR(o10 == o8);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        ListMgr.Free(o10, 16);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(o7, 16);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 1);

        ListMgr.Free(o4, 16);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(o2, 32);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 3);

        ListMgr.Free(o1, 16);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 3);

        ListMgr.Free(o3, 8);
        VERIFY_EXPR(ListMgr.DbgGetNumFreeBlocks() == 2);

        ListMgr.Free(o5, 16);
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
            VariableSizeAllocationsManager::OffsetType allocs[NumAllocs];
            for(size_t a=0; a < NumAllocs; ++a)
            {
                allocs[a] = ListMgr.Allocate(4);
                VERIFY_EXPR(allocs[a] == a*4);
            }
            for(size_t a=0; a < NumAllocs; ++a)
            {
                ListMgr.Free(allocs[ReleaseOrder[a]], 4);
            }
        } while(std::next_permutation(std::begin(ReleaseOrder), std::end(ReleaseOrder)));        
        VERIFY_EXPR(NumPerms == 720);
    }

    {
        VariableSizeGPUAllocationsManager ListMgr(128, Allocator);
        VariableSizeGPUAllocationsManager::OffsetType off[16];
        for(size_t o=0; o < _countof(off); ++o)
            off[o] = ListMgr.Allocate(8);
        VERIFY_EXPR(ListMgr.IsFull());

        ListMgr.Free(off[1], 8, 0);
        ListMgr.Free(off[5], 8, 0);
        ListMgr.Free(off[4], 8, 0);
        ListMgr.Free(off[3], 8, 0);

        ListMgr.Free(off[10], 8, 1);
        ListMgr.Free(off[13], 8, 1);
        ListMgr.Free(off[2], 8, 1);
        ListMgr.Free(off[8], 8, 1);

        ListMgr.ReleaseStaleAllocations(1);

        ListMgr.Free(off[14], 8, 2);
        ListMgr.Free(off[7], 8, 2);
        ListMgr.Free(off[0], 8, 2);
        ListMgr.Free(off[9], 8, 2);

        ListMgr.ReleaseStaleAllocations(2);

        ListMgr.Free(off[12], 8, 1);
        ListMgr.Free(off[15], 8, 1);
        ListMgr.Free(off[6], 8, 1);
        ListMgr.Free(off[11], 8, 1);

        ListMgr.ReleaseStaleAllocations(3);
    }
    
    SetStatus(TestResult::Succeeded);
}
