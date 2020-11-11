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

#include "StreamingBuffer.hpp"
#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(StreamingBufferTest, MapUnamp)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    StreamingBufferCreateInfo CI;
    CI.pDevice = pDevice;

    CI.BuffDesc.Name           = "Test streaming buffer";
    CI.BuffDesc.BindFlags      = BIND_VERTEX_BUFFER;
    CI.BuffDesc.Usage          = USAGE_DYNAMIC;
    CI.BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    CI.BuffDesc.uiSizeInBytes  = 1024;

    StreamingBuffer StreamBuff{CI};
    ASSERT_TRUE(StreamBuff.GetBuffer() != nullptr);

    {
        auto Offset = StreamBuff.Map(pContext, pDevice, 256);
        EXPECT_EQ(Offset, Uint32{0});
        EXPECT_NE(StreamBuff.GetMappedCPUAddress(), nullptr);
        StreamBuff.Unmap();
        EXPECT_EQ(StreamBuff.GetMappedCPUAddress(), nullptr);
    }

    {
        static constexpr Uint32 DataSize       = 512;
        static constexpr Uint8  Data[DataSize] = {};

        auto Offset = StreamBuff.Update(pContext, pDevice, Data, DataSize);
        EXPECT_EQ(Offset, Uint32{256});
    }

    {
        auto Offset = StreamBuff.Map(pContext, pDevice, 768);
        EXPECT_EQ(Offset, Uint32{0});
        StreamBuff.Unmap();
    }

    {
        auto Offset = StreamBuff.Map(pContext, pDevice, 1536);
        EXPECT_EQ(Offset, Uint32{0});
        StreamBuff.Unmap();
    }

    {
        auto Offset = StreamBuff.Map(pContext, pDevice, 256);
        EXPECT_EQ(Offset, Uint32{1536});
        StreamBuff.Unmap();
    }

    StreamBuff.Flush();

    {
        auto Offset = StreamBuff.Map(pContext, pDevice, 64);
        EXPECT_EQ(Offset, Uint32{0});
        StreamBuff.Unmap();
    }

    StreamBuff.Reset();
}

} // namespace
