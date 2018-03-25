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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "AllocatorTest.h"
#include "Errors.h"
#include "DefaultRawMemoryAllocator.h"
#include "FixedBlockMemoryAllocator.h"

using namespace Diligent;

AllocatorTest TheTest;

AllocatorTest::AllocatorTest() :
    UnitTestBase("Allocator test")
{
    const Uint32 AllocSize = 32;
    const Uint32 NumAllocationsPerPage = 16;
    FixedBlockMemoryAllocator TestAllocator(DefaultRawMemoryAllocator::GetAllocator(), AllocSize, NumAllocationsPerPage);
    void* Allocations[NumAllocationsPerPage][2]={};
    for(int p=0; p < 2; ++p)
    {
        for (int a = 1; a < NumAllocationsPerPage; ++a)
        {
            for (int i = 0; i < a; ++i)
                Allocations[i][p] = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);

            for (int i = a-1; i >= 0; --i)
                TestAllocator.Free(Allocations[i][p]);

            for (int i = 0; i < a; ++i)
            {
                auto *NewAlloc = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);
                VERIFY_EXPR(Allocations[i][p] == NewAlloc);
            }

            for (int i = a-1; i >= 0; --i)
                TestAllocator.Free(Allocations[i][p]);
        }
        for (int i = 0; i < NumAllocationsPerPage; ++i)
            Allocations[i][p] = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);
    }

    for(int p=0; p < 2; ++p)
        for (int i = 0; i < NumAllocationsPerPage; ++i)
            TestAllocator.Free( Allocations[i][p] );

    for(int p=0; p < 2; ++p)
        for (int i = 0; i < NumAllocationsPerPage; ++i)
            Allocations[i][p] = TestAllocator.Allocate(AllocSize, "Fixed block allocator test", __FILE__, __LINE__);

    for(int p=0; p < 2; ++p)
        for (int s = 0; s < 5; ++s)
            for (int i = s; i < NumAllocationsPerPage; i+=5)
               TestAllocator.Free( Allocations[i][p] );

    // Double free
    //TestAllocator.Free( Allocations[0][0] );
    SetStatus(TestResult::Succeeded);
}
