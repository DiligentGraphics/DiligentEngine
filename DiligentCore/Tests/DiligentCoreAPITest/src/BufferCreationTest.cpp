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

#if D3D11_SUPPORTED
#    include "D3D11/CreateObjFromNativeResD3D11.hpp"
#endif

#if D3D12_SUPPORTED
#    include "D3D12/CreateObjFromNativeResD3D12.hpp"
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#    include "GL/CreateObjFromNativeResGL.hpp"
#endif

#if VULKAN_SUPPORTED
#    include "Vulkan/CreateObjFromNativeResVK.hpp"
#endif

#include "GraphicsAccessories.hpp"

#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

extern "C"
{
    int TestBufferCInterface(void* pBuffer);
    int TestBufferViewCInterface(void* pView);
}

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

class BufferCreationTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        const auto DevCaps = pDevice->GetDeviceCaps();
        switch (DevCaps.DevType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D11(pDevice));
                break;

#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResD3D12(pDevice));
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResGL(pDevice));
                break;
#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                pCreateObjFromNativeRes.reset(new TestCreateObjFromNativeResVK(pDevice));
                break;
#endif

            default: UNEXPECTED("Unexpected device type");
        }
    }

    static void TearDownTestSuite()
    {
        pCreateObjFromNativeRes.reset();
        TestingEnvironment::GetInstance()->Reset();
    }

    static std::unique_ptr<CreateObjFromNativeResTestBase> pCreateObjFromNativeRes;
};

std::unique_ptr<CreateObjFromNativeResTestBase> BufferCreationTest::pCreateObjFromNativeRes;

TEST_F(BufferCreationTest, CreateVertexBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    BufferDesc BuffDesc;
    BuffDesc.Name          = "Vertex buffer";
    BuffDesc.uiSizeInBytes = 256;
    BuffDesc.BindFlags     = BIND_VERTEX_BUFFER;

    BufferData InitData;
    InitData.DataSize = BuffDesc.uiSizeInBytes;
    std::vector<Uint8> DummyData(InitData.DataSize);
    InitData.pData = DummyData.data();
    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, &InitData, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

    pCreateObjFromNativeRes->CreateBuffer(pBuffer);
}

TEST_F(BufferCreationTest, CreateIndexBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    BufferDesc BuffDesc;
    BuffDesc.Name          = "Index";
    BuffDesc.uiSizeInBytes = 256;
    BuffDesc.BindFlags     = BIND_VERTEX_BUFFER;

    BufferData NullData;

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, &NullData, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

    pCreateObjFromNativeRes->CreateBuffer(pBuffer);
}

TEST_F(BufferCreationTest, CreateFormattedBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& DevCaps = pDevice->GetDeviceCaps();
    if (!(DevCaps.Features.ComputeShaders && DevCaps.Features.IndirectRendering))
    {
        GTEST_SKIP();
    }

    BufferDesc BuffDesc;
    BuffDesc.Name              = "Formatted buffer";
    BuffDesc.uiSizeInBytes     = 256;
    BuffDesc.BindFlags         = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    BuffDesc.Mode              = BUFFER_MODE_FORMATTED;
    BuffDesc.ElementByteStride = 16;

    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

    BufferViewDesc ViewDesc;
    ViewDesc.ViewType             = BUFFER_VIEW_SHADER_RESOURCE;
    ViewDesc.ByteOffset           = 64;
    ViewDesc.Format.NumComponents = 4;
    ViewDesc.Format.ValueType     = VT_FLOAT32;
    ViewDesc.Format.IsNormalized  = false;
    RefCntAutoPtr<IBufferView> pBufferSRV;
    pBuffer->CreateView(ViewDesc, &pBufferSRV);
    EXPECT_NE(pBufferSRV, nullptr) << GetObjectDescString(BuffDesc);

    EXPECT_EQ(TestBufferViewCInterface(pBufferSRV.RawPtr()), 0);

    ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
    RefCntAutoPtr<IBufferView> pBufferUAV;
    pBuffer->CreateView(ViewDesc, &pBufferUAV);
    EXPECT_NE(pBufferUAV, nullptr) << GetObjectDescString(BuffDesc);

    pCreateObjFromNativeRes->CreateBuffer(pBuffer);

    EXPECT_EQ(TestBufferCInterface(pBuffer.RawPtr()), 0);
}

TEST_F(BufferCreationTest, CreateStructuredBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& DevCaps = pDevice->GetDeviceCaps();
    if (!DevCaps.Features.ComputeShaders)
    {
        GTEST_SKIP();
    }

    BufferDesc BuffDesc;
    BuffDesc.Name              = "Structured buffer";
    BuffDesc.uiSizeInBytes     = 256;
    BuffDesc.BindFlags         = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
    BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
    BuffDesc.ElementByteStride = 16;
    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

    pCreateObjFromNativeRes->CreateBuffer(pBuffer);
}

TEST_F(BufferCreationTest, CreateUniformBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    BufferDesc BuffDesc;
    BuffDesc.Name          = "Uniform buffer";
    BuffDesc.uiSizeInBytes = 256;
    BuffDesc.BindFlags     = BIND_UNIFORM_BUFFER;
    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

    pCreateObjFromNativeRes->CreateBuffer(pBuffer);
}

TEST_F(BufferCreationTest, CreateRawBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    BufferDesc BuffDesc;
    BuffDesc.Name              = "Raw buffer";
    BuffDesc.uiSizeInBytes     = 256;
    BuffDesc.BindFlags         = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    BuffDesc.Mode              = BUFFER_MODE_RAW;
    BuffDesc.ElementByteStride = 16;
    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
    ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

    BufferViewDesc ViewDesc;
    ViewDesc.ViewType             = BUFFER_VIEW_UNORDERED_ACCESS;
    ViewDesc.ByteOffset           = 64;
    ViewDesc.Format.NumComponents = 4;
    ViewDesc.Format.ValueType     = VT_FLOAT32;
    RefCntAutoPtr<IBufferView> pBufferUAV;
    pBuffer->CreateView(ViewDesc, &pBufferUAV);
    ASSERT_NE(pBufferUAV, nullptr) << GetObjectDescString(BuffDesc);

    ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
    RefCntAutoPtr<IBufferView> pBufferSRV;
    pBuffer->CreateView(ViewDesc, &pBufferSRV);
    ASSERT_NE(pBufferSRV, nullptr) << GetObjectDescString(BuffDesc);

    pCreateObjFromNativeRes->CreateBuffer(pBuffer);
}

TEST_F(BufferCreationTest, CreateStagingBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    auto* pCtx    = pEnv->GetDeviceContext();

    BufferDesc BuffDesc;
    BuffDesc.Name           = "Staging buffer";
    BuffDesc.Usage          = USAGE_STAGING;
    BuffDesc.uiSizeInBytes  = 256;
    BuffDesc.BindFlags      = BIND_NONE;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_READ;

    {
        RefCntAutoPtr<IBuffer> pReadBuffer;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pReadBuffer);
        ASSERT_NE(pReadBuffer, nullptr) << GetObjectDescString(BuffDesc);

        void* pMappedData = nullptr;
        pCtx->MapBuffer(pReadBuffer, MAP_READ, MAP_FLAG_DO_NOT_WAIT, pMappedData);
        EXPECT_NE(pMappedData, nullptr);
        pCtx->UnmapBuffer(pReadBuffer, MAP_READ);
    }

    {
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        RefCntAutoPtr<IBuffer> pWriteBuffer;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pWriteBuffer);
        ASSERT_NE(pWriteBuffer, nullptr) << GetObjectDescString(BuffDesc);

        void* pMappedData = nullptr;
        pCtx->MapBuffer(pWriteBuffer, MAP_WRITE, MAP_FLAG_NONE, pMappedData);
        EXPECT_NE(pMappedData, nullptr);
        pCtx->UnmapBuffer(pWriteBuffer, MAP_WRITE);
    }
}


TEST_F(BufferCreationTest, CreateDynamicBuffer)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    auto* pCtx    = pEnv->GetDeviceContext();

    BufferDesc BuffDesc;
    BuffDesc.Name           = "Dynamic vertex buffer";
    BuffDesc.Usage          = USAGE_DYNAMIC;
    BuffDesc.uiSizeInBytes  = 256;
    BuffDesc.BindFlags      = BIND_VERTEX_BUFFER;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    {
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
        ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

        void* pMappedData = nullptr;
        pCtx->MapBuffer(pBuffer, MAP_WRITE, MAP_FLAG_DISCARD, pMappedData);
        EXPECT_NE(pMappedData, nullptr);
        pCtx->UnmapBuffer(pBuffer, MAP_WRITE);
    }

    BuffDesc.Name              = "Dynamic structured buffer";
    BuffDesc.BindFlags         = BIND_SHADER_RESOURCE;
    BuffDesc.Mode              = BUFFER_MODE_STRUCTURED;
    BuffDesc.ElementByteStride = 16;

    {
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
        ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

        void* pMappedData = nullptr;
        pCtx->MapBuffer(pBuffer, MAP_WRITE, MAP_FLAG_DISCARD, pMappedData);
        EXPECT_NE(pMappedData, nullptr);
        pCtx->UnmapBuffer(pBuffer, MAP_WRITE);
    }
}

TEST_F(BufferCreationTest, CreateUnifiedBuffer)
{
    auto*       pEnv        = TestingEnvironment::GetInstance();
    auto*       pDevice     = pEnv->GetDevice();
    auto*       pCtx        = pEnv->GetDeviceContext();
    const auto& AdapterInfo = pDevice->GetDeviceCaps().AdapterInfo;
    if (AdapterInfo.UnifiedMemory == 0)
    {
        GTEST_SKIP() << "Unified memory is not available on this device";
    }

    BufferDesc BuffDesc;
    BuffDesc.Name           = "Unified vertex buffer";
    BuffDesc.Usage          = USAGE_UNIFIED;
    BuffDesc.uiSizeInBytes  = 256;
    BuffDesc.BindFlags      = BIND_VERTEX_BUFFER;
    BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    if (AdapterInfo.UnifiedMemoryCPUAccess & CPU_ACCESS_WRITE)
    {
        BufferData InitData;
        InitData.DataSize = BuffDesc.uiSizeInBytes;
        std::vector<Uint8> DummyData(InitData.DataSize);
        InitData.pData = DummyData.data();
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, &InitData, &pBuffer);
        ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);
    }
    else
    {
        LOG_INFO_MESSAGE("Unified memory on this device does not support write access");
    }

    if (AdapterInfo.UnifiedMemoryCPUAccess & CPU_ACCESS_READ)
    {
        BuffDesc.BindFlags      = BIND_NONE;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_READ;
        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, nullptr, &pBuffer);
        ASSERT_NE(pBuffer, nullptr) << GetObjectDescString(BuffDesc);

        void* pMappedData = nullptr;
        pCtx->MapBuffer(pBuffer, MAP_READ, MAP_FLAG_DO_NOT_WAIT, pMappedData);
        EXPECT_NE(pMappedData, nullptr);
        pCtx->UnmapBuffer(pBuffer, MAP_READ);
    }
    else
    {
        LOG_INFO_MESSAGE("Unified memory on this device does not read access");
    }
}

} // namespace
