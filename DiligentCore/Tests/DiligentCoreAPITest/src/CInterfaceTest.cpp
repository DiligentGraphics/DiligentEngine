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

#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

extern "C"
{
    int TestRenderDeviceCInterface_Misc(void* pRenderDevice);
    int TestRenderDeviceCInterface_CreateBuffer(void* pRenderDevice);
    int TestRenderDeviceCInterface_CreateShader(void* pRenderDevice);
    int TestRenderDeviceCInterface_CreateTexture(void* pRenderDevice);
    int TestRenderDeviceCInterface_CreateSampler(void* pRenderDevice);
    int TestRenderDeviceCInterface_CreateResourceMapping(void* pRenderDevice);
    int TestRenderDeviceCInterface_CreateFence(void* pRenderDevice);
    int TestRenderDeviceCInterface_CreateQuery(void* pRenderDevice);
}

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(RenderDevice_CInterface, Misc)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_Misc(pDevice), 0);
}


TEST(RenderDevice_CInterface, CreateBuffer)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_CreateBuffer(pDevice), 0);
}

TEST(RenderDevice_CInterface, CreateShader)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_CreateShader(pDevice), 0);
}

TEST(RenderDevice_CInterface, CreateTexture)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_CreateTexture(pDevice), 0);
}

TEST(RenderDevice_CInterface, CreateSampler)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_CreateSampler(pDevice), 0);
}

TEST(RenderDevice_CInterface, CreateResourceMapping)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_CreateResourceMapping(pDevice), 0);
}

TEST(RenderDevice_CInterface, CreateFence)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_CreateFence(pDevice), 0);
}

TEST(RenderDevice_CInterface, CreateQuery)
{
    auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();
    EXPECT_EQ(TestRenderDeviceCInterface_CreateQuery(pDevice), 0);
}

} // namespace
