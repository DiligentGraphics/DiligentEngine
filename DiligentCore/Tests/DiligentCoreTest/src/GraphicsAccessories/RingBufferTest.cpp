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

#include "RingBuffer.hpp"
#include "DefaultRawMemoryAllocator.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(GraphicsAccessories_RingBuffer, AllocDealloc)
{
    // Need to define local variable to avoid vexing linker errors
    const auto InvalidOffset = RingBuffer::InvalidOffset;
    using OffsetType         = RingBuffer::OffsetType;

    auto& Allocator = DefaultRawMemoryAllocator::GetAllocator();
    {
        RingBuffer RB(1023, Allocator);

        auto Offset = RB.Allocate(120, 16);
        //
        //  O          h
        //  |          |                                      |
        //  0         128
        EXPECT_EQ(Offset, OffsetType{0});


        Offset = RB.Allocate(10, 1);
        //
        //  t          O   h
        //  |          |   |                                  |
        //  0         128 138
        EXPECT_EQ(Offset, OffsetType{128});

        Offset = RB.Allocate(10, 32);
        //
        //  t                  O   h
        //  |                  |   |                          |
        //  0         128 138 160 192
        EXPECT_EQ(Offset, OffsetType{160});

        Offset = RB.Allocate(17, 1);
        //
        //  t                      O   h
        //  |                      |   |                      |
        //  0         128 138 160 192 209
        EXPECT_EQ(Offset, OffsetType{192});

        Offset = RB.Allocate(65, 64);
        //
        //  t                               O    h
        //  |                               |    |            |
        //  0         128 138 160 192 209  256  384
        EXPECT_EQ(Offset, OffsetType{256});

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
        EXPECT_EQ(Offset, OffsetType{512});


        Offset = RB.Allocate(127, 1);
        //
        //  t          h1           O         h
        //  |          |            |         |               |
        //  0         384   512    768      895 = 1023-128
        EXPECT_EQ(Offset, OffsetType{768});

        Offset = RB.Allocate(128, 2);
        EXPECT_EQ(Offset, InvalidOffset);

        Offset = RB.Allocate(128, 1);
        //
        //  t          h1                      O              h
        //  |          |                       |              |
        //  0         384   512    768        895           1023
        EXPECT_EQ(Offset, OffsetType{895});

        RB.FinishCurrentFrame(2);
        //
        //  t          h1                                     h2,h
        //  |          |                                      |
        //  0         384                                   1023
        EXPECT_TRUE(RB.IsFull());

        Offset = RB.Allocate(1, 1);
        EXPECT_EQ(Offset, InvalidOffset);

        RingBuffer RB1(std::move(RB));
        EXPECT_TRUE(RB.IsEmpty());
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
        EXPECT_TRUE(RB1.IsEmpty());
        EXPECT_EQ(RB1.GetUsedSize(), OffsetType{0});

        Offset = RB1.Allocate(256, 1);
        //
        //  O        h                                        t
        //  |        |                                        |
        //  0       256                                     1023
        EXPECT_EQ(Offset, OffsetType{0});


        Offset = RB1.Allocate(256, 16);
        RB1.ReleaseCompletedFrames(0);
        //
        //           O      h                                 t
        //  |        |      |                                 |
        //  0       256    512                              1023
        EXPECT_EQ(Offset, OffsetType{256});
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

        EXPECT_EQ(RB1.GetUsedSize(), OffsetType{0});
        EXPECT_TRUE(RB1.IsEmpty());

        Offset = RB1.Allocate(512, 1);
        //
        //  O               h
        //  |               |                                 |
        //  0              512                              1023
        EXPECT_EQ(Offset, OffsetType{0});

        RB1.FinishCurrentFrame(4);
        RB1.FinishCurrentFrame(5);
        //
        //  t               h4
        //  |               |                                 |
        //  0              512                              1023

        Offset = RB1.Allocate(129, 1);
        //
        //  t              h4,O       h
        //  |               |         |                       |
        //  0              512       641                    1023
        EXPECT_EQ(Offset, OffsetType{512});

        RB1.ReleaseCompletedFrames(4);
        //
        //                  t         h
        //  |               |         |                       |
        //  0              512       641                    1023


        Offset = RB1.Allocate(128, 128);
        //
        //                  t                  O      h
        //  |               |                  |      |       |
        //  0              512       641      768    896    1023
        EXPECT_EQ(Offset, OffsetType{768});

        Offset = RB1.Allocate(513, 64);
        EXPECT_EQ(Offset, InvalidOffset);

        Offset = RB1.Allocate(513, 1);
        EXPECT_EQ(Offset, InvalidOffset);

        Offset = RB1.Allocate(255, 1);
        //
        //  O     h         t
        //  |     |         |                                 |
        //  0    255       512                              1023
        EXPECT_EQ(Offset, OffsetType{0});

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
        EXPECT_EQ(Offset, OffsetType{256});

        EXPECT_TRUE(RB1.IsFull());
        Offset = RB1.Allocate(1, 1);
        EXPECT_EQ(Offset, InvalidOffset);
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
        EXPECT_EQ(Offset, OffsetType{512});

        Offset = RB1.Allocate(191, 1);
        //
        //  O       h          t
        //  |       |          |                              |
        //  0      191        255                           1023
        EXPECT_EQ(Offset, OffsetType{0});

        Offset = RB1.Allocate(64, 2);
        EXPECT_EQ(Offset, InvalidOffset);

        Offset = RB1.Allocate(64, 1);
        //
        //          O         t,h
        //  |       |          |                              |
        //  0      191        255                           1023
        EXPECT_EQ(Offset, OffsetType{191});

        Offset = RB1.Allocate(1, 1);
        EXPECT_EQ(Offset, InvalidOffset);

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
}

} // namespace
