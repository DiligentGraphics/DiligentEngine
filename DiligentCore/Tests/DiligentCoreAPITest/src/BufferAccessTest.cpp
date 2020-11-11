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

#include <sstream>

#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

// clang-format off
static constexpr float TestBufferData[] =
{
    0,  1,  2,  3,
    4,  5,  6,  7,
    8,  9, 10, 11,
    12, 13, 14, 15
};
// clang-format on

void VerifyBufferData(IBuffer* pBuffer)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    BufferDesc BuffDesc;
    BuffDesc.Name           = "Test staging buffer";
    BuffDesc.Usage          = USAGE_STAGING;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_READ;
    BuffDesc.uiSizeInBytes  = sizeof(TestBufferData);
    BuffDesc.BindFlags      = BIND_NONE;

    RefCntAutoPtr<IBuffer> pStagingBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pStagingBuffer);
    ASSERT_NE(pStagingBuffer, nullptr) << "Buffer desc:\n"
                                       << BuffDesc;

    pContext->CopyBuffer(pBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                         pStagingBuffer, 0, BuffDesc.uiSizeInBytes, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->WaitForIdle();

    void* pBufferData = nullptr;
    pContext->MapBuffer(pStagingBuffer, MAP_READ, MAP_FLAG_DO_NOT_WAIT, pBufferData);
    ASSERT_NE(pBufferData, nullptr);
    if (memcmp(pBufferData, TestBufferData, sizeof(TestBufferData)) != 0)
    {
        ADD_FAILURE() << "Buffer data does not match reference values";
    }
    pContext->UnmapBuffer(pStagingBuffer, MAP_READ);
}

TEST(BufferAccessTest, Initialization)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    BufferDesc BuffDesc;
    BuffDesc.Name          = "Test immutable buffer";
    BuffDesc.Usage         = USAGE_IMMUTABLE;
    BuffDesc.uiSizeInBytes = sizeof(TestBufferData);
    BuffDesc.BindFlags     = BIND_UNIFORM_BUFFER;

    BufferData InitData;
    InitData.pData    = TestBufferData;
    InitData.DataSize = BuffDesc.uiSizeInBytes;
    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, &InitData, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << "Buffer desc:\n"
                                << BuffDesc;

    VerifyBufferData(pBuffer);
}

TEST(BufferAccessTest, UpdateBufferData)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    BufferDesc BuffDesc;
    BuffDesc.Name          = "Test default buffer";
    BuffDesc.Usage         = USAGE_DEFAULT;
    BuffDesc.uiSizeInBytes = sizeof(TestBufferData);
    BuffDesc.BindFlags     = BIND_UNIFORM_BUFFER;

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << "Buffer desc:\n"
                                << BuffDesc;
    pContext->UpdateBuffer(pBuffer, 0, BuffDesc.uiSizeInBytes, TestBufferData, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    VerifyBufferData(pBuffer);
}

TEST(BufferAccessTest, MapWriteDiscard)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    BufferDesc BuffDesc;
    BuffDesc.Name           = "Test dynamic buffer";
    BuffDesc.Usage          = USAGE_DYNAMIC;
    BuffDesc.uiSizeInBytes  = sizeof(TestBufferData);
    BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << "Buffer desc:\n"
                                << BuffDesc;
    void* pData = nullptr;
    pContext->MapBuffer(pBuffer, MAP_WRITE, MAP_FLAG_DISCARD, pData);
    ASSERT_NE(pData, nullptr);
    memcpy(pData, TestBufferData, sizeof(TestBufferData));
    pContext->UnmapBuffer(pBuffer, MAP_WRITE);

    VerifyBufferData(pBuffer);
}

TEST(BufferAccessTest, CopyFromStaging)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    BufferDesc BuffDesc;
    BuffDesc.Name          = "Test default buffer";
    BuffDesc.Usage         = USAGE_DEFAULT;
    BuffDesc.uiSizeInBytes = sizeof(TestBufferData);
    BuffDesc.BindFlags     = BIND_UNIFORM_BUFFER;

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << "Buffer desc:\n"
                                << BuffDesc;

    BuffDesc.Name           = "Staging writable buffer";
    BuffDesc.Usage          = USAGE_STAGING;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    BuffDesc.BindFlags      = BIND_NONE;
    RefCntAutoPtr<IBuffer> pStagingBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pStagingBuffer);
    ASSERT_NE(pStagingBuffer, nullptr) << "Buffer desc:\n"
                                       << BuffDesc;

    void* pData = nullptr;
    pContext->MapBuffer(pStagingBuffer, MAP_WRITE, MAP_FLAG_NONE, pData);
    ASSERT_NE(pData, nullptr);
    memcpy(pData, TestBufferData, sizeof(TestBufferData));
    pContext->UnmapBuffer(pStagingBuffer, MAP_WRITE);

    pContext->CopyBuffer(pStagingBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                         pBuffer, 0, BuffDesc.uiSizeInBytes, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    VerifyBufferData(pBuffer);
}

} // namespace
