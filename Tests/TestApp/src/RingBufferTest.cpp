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
#include "RingBuffer.h"
#include "DefaultRawMemoryAllocator.h"
#include "DebugUtilities.h"
#include "UnitTestBase.h"

using namespace Diligent;

class RingBufferTest : public UnitTestBase
{
public:
    RingBufferTest();
};

static RingBufferTest TheRingBufferTest;

RingBufferTest::RingBufferTest() :
    UnitTestBase("Ring buffer test")
{
    auto &Allocator = DefaultRawMemoryAllocator::GetAllocator();
    {
        RingBuffer RB(1024, Allocator);

        auto Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == 0);

        Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == 256);

        RB.FinishCurrentFrame(0);

        Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == 512);

        Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == 768);

        RB.FinishCurrentFrame(1);
        VERIFY_EXPR(RB.IsFull());

        Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);

        RingBuffer RB1(std::move(RB));
        RB1.ReleaseCompletedFrames(2);
        VERIFY_EXPR(RB1.IsEmpty());

        VERIFY_EXPR(RB1.GetUsedSize() == 0);
        
        Offset = RB1.Allocate(256);
        VERIFY_EXPR(Offset == 0);
        
        Offset = RB1.Allocate(256);
        VERIFY_EXPR(Offset == 256);
        RB1.FinishCurrentFrame(2);
        RB1.ReleaseCompletedFrames(3);
        
        VERIFY_EXPR(RB1.GetUsedSize() == 0);
        VERIFY_EXPR(RB1.IsEmpty());

        Offset = RB1.Allocate(256);
        VERIFY_EXPR(Offset == 512);

        Offset = RB1.Allocate(512);
        VERIFY_EXPR(Offset == 0);

        VERIFY_EXPR(RB1.IsFull());
        Offset = RB1.Allocate(1);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);
        RB1.FinishCurrentFrame(3);

        RB = std::move(RB1);
        RB.ReleaseCompletedFrames(4);
        VERIFY_EXPR(RB.GetUsedSize() == 0);
        VERIFY_EXPR(RB.IsEmpty());

        Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == 512);
        Offset = RB.Allocate(512+1);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);
        Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == 768);
        Offset = RB.Allocate(256);
        VERIFY_EXPR(Offset == 0);
        Offset = RB.Allocate(256+1);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);
        RB.FinishCurrentFrame(5);
        RB.ReleaseCompletedFrames(6);
    }

    {
        RingBuffer RB(1024, Allocator);
        auto offset = RB.Allocate(512);
        RB.FinishCurrentFrame(0);
        RB.FinishCurrentFrame(1);
        RB.ReleaseCompletedFrames(2);
        RB.FinishCurrentFrame(2);
        RB.FinishCurrentFrame(3);
        offset = RB.Allocate(512);
        RB.FinishCurrentFrame(4);
        RB.ReleaseCompletedFrames(3);
        RB.ReleaseCompletedFrames(4);
        RB.ReleaseCompletedFrames(5);
        offset = RB.Allocate(512);
        RB.FinishCurrentFrame(5);
        RB.ReleaseCompletedFrames(6);
    }
    
    SetStatus(TestResult::Succeeded);
}
