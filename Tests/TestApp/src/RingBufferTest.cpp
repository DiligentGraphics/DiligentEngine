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
        RingBuffer RB(1023, Allocator);

        auto Offset = RB.Allocate(120, 16);
        //
        //  O          h
        //  |          |                                      |
        //  0         128
        VERIFY_EXPR(Offset == 0);


        Offset = RB.Allocate(10, 1);
        //
        //  t          O   h
        //  |          |   |                                  |
        //  0         128 138
        VERIFY_EXPR(Offset == 128);

        Offset = RB.Allocate(10, 32);
        //
        //  t                  O   h 
        //  |                  |   |                          |
        //  0         128 138 160 192
        VERIFY_EXPR(Offset == 160);

        Offset = RB.Allocate(17, 1);
        //
        //  t                      O   h
        //  |                      |   |                      |
        //  0         128 138 160 192 209
        VERIFY_EXPR(Offset == 192);

        Offset = RB.Allocate(65, 64);
        //                          
        //  t                               O    h   
        //  |                               |    |            |
        //  0         128 138 160 192 209  256  384
        VERIFY_EXPR(Offset == 256);

        RB.FinishCurrentFrame(1);
        //                          
        //  t          h1                            
        //  |          |                                      |
        //  0         384                          


        Offset = RB.Allocate(100, 256);
        //                          
        //  t          h1    O      h                
        //  |          |     |      |                         |
        //  0         384   512    768             
        VERIFY_EXPR(Offset == 512);


        Offset = RB.Allocate(127, 1);
        //                          
        //  t          h1           O         h       
        //  |          |            |         |               |
        //  0         384   512    768      895 = 1023-128   
        VERIFY_EXPR(Offset == 768);

        Offset = RB.Allocate(128, 2);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);

        Offset = RB.Allocate(128, 1);
        //                          
        //  t          h1                      O              h
        //  |          |                       |              |
        //  0         384   512    768        895           1023
        VERIFY_EXPR(Offset == 895);

        RB.FinishCurrentFrame(2);
        //                          
        //  t          h1                                     h2,h
        //  |          |                                      |
        //  0         384                                   1023
        VERIFY_EXPR(RB.IsFull());

        Offset = RB.Allocate(1, 1);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);

        RingBuffer RB1(std::move(RB));
        VERIFY_EXPR(RB.IsEmpty());
        RB1.ReleaseCompletedFrames(1);
        //                          
        //             t                                      h2
        //  |          |                                      |
        //  0         384                                   1023

        RB1.ReleaseCompletedFrames(2);
        //                          
        //                                                   h,t
        //  |                                                 |
        //  0                                               1023
        VERIFY_EXPR(RB1.IsEmpty());
        VERIFY_EXPR(RB1.GetUsedSize() == 0);

        Offset = RB1.Allocate(256,1);
        //                          
        //  O        h                                        t
        //  |        |                                        |
        //  0       256                                     1023
        VERIFY_EXPR(Offset == 0);


        Offset = RB1.Allocate(256, 16);
        RB1.ReleaseCompletedFrames(0);
        //                          
        //           O      h                                 t
        //  |        |      |                                 |
        //  0       256    512                              1023
        VERIFY_EXPR(Offset == 256);
        RB1.FinishCurrentFrame(2);
        RB1.FinishCurrentFrame(3); // ignored
        //                          
        //                  h2                                t
        //  |               |                                 |
        //  0              512                              1023
        RB1.ReleaseCompletedFrames(2);
        //                          
        // h,t                                                 
        //  |                                                 |
        //  0                                               1023
        
        RB1.ReleaseCompletedFrames(3);

        VERIFY_EXPR(RB1.GetUsedSize() == 0);
        VERIFY_EXPR(RB1.IsEmpty());

        Offset = RB1.Allocate(512,1);
        //                          
        //  O               h                                  
        //  |               |                                 |
        //  0              512                              1023
        VERIFY_EXPR(Offset == 0);

        RB1.FinishCurrentFrame(4);
        RB1.FinishCurrentFrame(5);
        //                          
        //  t               h4   
        //  |               |                                 |
        //  0              512                              1023

        Offset = RB1.Allocate(129,1);
        //                          
        //  t              h4,O       h
        //  |               |         |                       |
        //  0              512       641                    1023
        VERIFY_EXPR(Offset == 512);

        RB1.ReleaseCompletedFrames(4);
        //                          
        //                  t         h
        //  |               |         |                       |
        //  0              512       641                    1023


        Offset = RB1.Allocate(128,128);
        //                          
        //                  t                  O      h        
        //  |               |                  |      |       |
        //  0              512       641      768    896    1023
        VERIFY_EXPR(Offset == 768);

        Offset = RB1.Allocate(513,64);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);

        Offset = RB1.Allocate(513,1);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);

        Offset = RB1.Allocate(255,1);
        //                          
        //  O     h         t                                  
        //  |     |         |                                 |
        //  0    255       512                              1023
        VERIFY_EXPR(Offset == 0);

        RB1.FinishCurrentFrame(6);
        //                          
        //  O    h,h6       t                                  
        //  |     |         |                                 |
        //  0    255       512                              1023


        Offset = RB1.Allocate(256, 2);
        //                          
        //       h6   O    t,h                                
        //  |     |   |     |                                 |
        //  0    255 256   512                              1023
        VERIFY_EXPR(Offset == 256);

        VERIFY_EXPR(RB1.IsFull());
        Offset = RB1.Allocate(1,1);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);
        RB1.ReleaseCompletedFrames(6);
        //                          
        //        t         h                                
        //  |     |         |                                 |
        //  0    255       512                              1023

        Offset = RB1.Allocate(511, 1);
        //                          
        //        t         O                                 h
        //  |     |         |                                 |
        //  0    255       512                              1023
        VERIFY_EXPR(Offset == 512);

        Offset = RB1.Allocate(191,1);
        //                          
        //  O       h          t                             
        //  |       |          |                              |
        //  0      191        255                           1023
        VERIFY_EXPR(Offset == 0);

        Offset = RB1.Allocate(64,2);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);

        Offset = RB1.Allocate(64,1);
        //                          
        //          O         t,h                           
        //  |       |          |                              |
        //  0      191        255                           1023
        VERIFY_EXPR(Offset == 191);

        Offset = RB1.Allocate(1,1);
        VERIFY_EXPR(Offset == RingBuffer::InvalidOffset);

        RB1.FinishCurrentFrame(7);
        RB1.ReleaseCompletedFrames(7);
    }

    {
        RingBuffer RB(1024, Allocator);
        auto offset = RB.Allocate(512, 1);
        RB.FinishCurrentFrame(0);
        RB.FinishCurrentFrame(1);
        RB.ReleaseCompletedFrames(1);
        RB.FinishCurrentFrame(2);
        RB.FinishCurrentFrame(3);
        offset = RB.Allocate(512, 1);
        RB.FinishCurrentFrame(4);
        RB.ReleaseCompletedFrames(2);
        RB.ReleaseCompletedFrames(3);
        RB.ReleaseCompletedFrames(4);
        offset = RB.Allocate(512, 1);
        RB.FinishCurrentFrame(5);
        RB.ReleaseCompletedFrames(5);
    }

    SetStatus(TestResult::Succeeded);
}
