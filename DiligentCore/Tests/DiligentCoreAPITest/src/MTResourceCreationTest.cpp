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

#include <thread>
#include <atomic>
#include <algorithm>

#include "TestingEnvironment.hpp"
#include "ThreadSignal.hpp"
#if D3D12_SUPPORTED
#    include "D3D12/D3D12DebugLayerSetNameBugWorkaround.hpp"
#endif

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static const char g_ShaderSource[] = R"(
void VSMain(out float4 pos : SV_POSITION)
{
	pos = float4(0.0, 0.0, 0.0, 0.0);
}
                                       
void PSMain(out float4 col : SV_TARGET)
{
	col = float4(0.0, 0.0, 0.0, 0.0);
}
)";


class MultithreadedResourceCreationTest : public ::testing::Test
{
protected:
    ~MultithreadedResourceCreationTest();

    static void WorkerThreadFunc(MultithreadedResourceCreationTest* This, size_t ThreadNum);

    void StartWorkerThreadsAndWait(int SignalIdx);
    void WaitSiblingWorkerThreads(int SignalIdx);

    std::vector<std::thread> m_Threads;

    ThreadingTools::Signal m_WorkerThreadSignal[2];
    ThreadingTools::Signal m_MainThreadSignal;

    std::mutex      m_NumThreadsCompletedMtx;
    std::atomic_int m_NumThreadsCompleted[2];
    std::atomic_int m_NumThreadsReady;

    static const int NumBuffersToCreate  = 10;
    static const int NumTexturesToCreate = 5;
    static const int NumPSOToCreate      = 3;

#ifdef DILIGENT_DEBUG
    static const int NumIterations = 10;
#else
    static const int NumIterations = 30;
#endif
};

MultithreadedResourceCreationTest::~MultithreadedResourceCreationTest()
{
    LOG_INFO_MESSAGE("Created ", int{NumBuffersToCreate}, " buffers, ", int{NumTexturesToCreate}, " textures, and ",
                     int{NumPSOToCreate}, " PSO in ", int{NumIterations}, " iterations by each of ", m_Threads.size(), " threads");
}

void MultithreadedResourceCreationTest::WaitSiblingWorkerThreads(int SignalIdx)
{
    auto NumThreads = static_cast<int>(m_Threads.size());
    if (++m_NumThreadsCompleted[SignalIdx] == NumThreads)
    {
        ASSERT_FALSE(m_WorkerThreadSignal[1 - SignalIdx].IsTriggered());
        m_MainThreadSignal.Trigger();
    }
    else
    {
        while (m_NumThreadsCompleted[SignalIdx] < NumThreads)
            std::this_thread::yield();
    }
}

void MultithreadedResourceCreationTest::StartWorkerThreadsAndWait(int SignalIdx)
{
    m_NumThreadsCompleted[SignalIdx] = 0;
    m_WorkerThreadSignal[SignalIdx].Trigger(true);

    m_MainThreadSignal.Wait(true, 1);
}

void MultithreadedResourceCreationTest::WorkerThreadFunc(MultithreadedResourceCreationTest* This, size_t ThreadNum)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const int NumThreads = static_cast<int>(This->m_Threads.size());

    constexpr Uint32 TexWidth  = 512;
    constexpr Uint32 TexHeight = 512;

    std::vector<Uint8> RawBufferData(1024);
    std::vector<Uint8> RawTextureData(TexWidth * TexHeight * 4);

    int SignalIdx = 0;
    while (true)
    {
        auto SignaledValue = This->m_WorkerThreadSignal[SignalIdx].Wait(true, NumThreads);
        if (SignaledValue < 0)
        {
            return;
        }

        for (Uint32 i = 0; i < NumBuffersToCreate; ++i)
        {
            BufferDesc BuffDesc;
            BuffDesc.Name          = "MT creation test uniform buffer";
            BuffDesc.Usage         = USAGE_DEFAULT;
            BuffDesc.BindFlags     = BIND_UNIFORM_BUFFER;
            BuffDesc.uiSizeInBytes = static_cast<Uint32>(RawBufferData.size());

            BufferData BuffData;
            BuffData.DataSize = BuffDesc.uiSizeInBytes;
            BuffData.pData    = RawBufferData.data();

            RefCntAutoPtr<IBuffer> pBuffer1;
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer1);
            EXPECT_NE(pBuffer1, nullptr) << "Failed to create the following buffer:\n"
                                         << BuffDesc;

            BuffDesc.Name              = "MT creation test formatted buffer";
            BuffDesc.Mode              = BUFFER_MODE_FORMATTED;
            BuffDesc.ElementByteStride = 16;
            BuffDesc.BindFlags         = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
            RefCntAutoPtr<IBuffer> pBuffer2;
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer2);
            EXPECT_NE(pBuffer2, nullptr) << "Failed to create the following buffer:\n"
                                         << BuffDesc;

            BufferViewDesc ViewDesc;
            ViewDesc.ViewType             = BUFFER_VIEW_SHADER_RESOURCE;
            ViewDesc.ByteOffset           = 64;
            ViewDesc.Format.NumComponents = 4;
            ViewDesc.Format.IsNormalized  = False;
            ViewDesc.Format.ValueType     = VT_FLOAT32;
            RefCntAutoPtr<IBufferView> pBufferSRV;
            if (pBuffer2)
            {
                pBuffer2->CreateView(ViewDesc, &pBufferSRV);
                EXPECT_NE(pBufferSRV, nullptr) << "Failed to create SRV for the following buffer:\n"
                                               << BuffDesc;
            }

            BuffDesc.Name      = "MT creation test vertex buffer";
            BuffDesc.BindFlags = BIND_VERTEX_BUFFER | BIND_UNORDERED_ACCESS;
            RefCntAutoPtr<IBuffer> pBuffer3;
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer3);
            EXPECT_NE(pBuffer3, nullptr) << "Failed to create the following buffer:\n"
                                         << BuffDesc;
            ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
            RefCntAutoPtr<IBufferView> pBufferUAV;
            if (pBuffer3)
            {
                pBuffer3->CreateView(ViewDesc, &pBufferUAV);
                EXPECT_NE(pBufferUAV, nullptr) << "Failed to create UAV for the following buffer:\n"
                                               << BuffDesc;
            }

            BuffDesc.Name      = "MT creation test raw buffer";
            BuffDesc.Mode      = BUFFER_MODE_RAW;
            BuffDesc.BindFlags = BIND_INDEX_BUFFER | BIND_UNORDERED_ACCESS;
            RefCntAutoPtr<IBuffer> pBuffer4;
            pDevice->CreateBuffer(BuffDesc, &BuffData, &pBuffer4);
            EXPECT_NE(pBuffer4, nullptr) << "Failed to create the following buffer:\n"
                                         << BuffDesc;
        }

        for (Uint32 i = 0; i < NumTexturesToCreate; ++i)
        {
            RefCntAutoPtr<ITexture> pTexture;

            TextureDesc TexDesc;
            TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
            TexDesc.Type      = RESOURCE_DIM_TEX_2D;
            TexDesc.Width     = TexWidth;
            TexDesc.Height    = TexHeight;
            TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
            TexDesc.MipLevels = 1;


            TextureSubResData SubResData;
            SubResData.pData  = RawTextureData.data();
            SubResData.Stride = TexDesc.Width * 4;

            TextureData TexData;
            TexData.NumSubresources = 1;
            TexData.pSubResources   = &SubResData;

            pDevice->CreateTexture(TexDesc, &TexData, &pTexture);
            EXPECT_NE(pTexture, nullptr) << "Failed to create the following texture:\n"
                                         << TexDesc;
        }

        for (Uint32 i = 0; i < NumPSOToCreate; ++i)
        {
            RefCntAutoPtr<IShader>        pTrivialVS, pTrivialPS;
            RefCntAutoPtr<IPipelineState> pPSO;
            {
                ShaderCreateInfo Attrs;
                Attrs.Source                     = g_ShaderSource;
                Attrs.EntryPoint                 = "VSMain";
                Attrs.Desc.ShaderType            = SHADER_TYPE_VERTEX;
                Attrs.Desc.Name                  = "TrivialVS (MTResourceCreationTest)";
                Attrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
                Attrs.ShaderCompiler             = pEnv->GetDefaultCompiler(Attrs.SourceLanguage);
                Attrs.UseCombinedTextureSamplers = true;
                pDevice->CreateShader(Attrs, &pTrivialVS);
                if (!pTrivialVS)
                {
                    ADD_FAILURE() << "Failed to create trivial VS";
                    continue;
                }

                Attrs.EntryPoint      = "PSMain";
                Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
                Attrs.Desc.Name       = "TrivialPS (MTResourceCreationTest)";
                pDevice->CreateShader(Attrs, &pTrivialPS);
                if (!pTrivialPS)
                {
                    ADD_FAILURE() << "Failed to create trivial PS";
                    continue;
                }


                GraphicsPipelineStateCreateInfo PSOCreateInfo;

                auto& PSODesc          = PSOCreateInfo.PSODesc;
                auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

                PSODesc.Name                       = "MT creation test PSO";
                PSOCreateInfo.pVS                  = pTrivialVS;
                PSOCreateInfo.pPS                  = pTrivialPS;
                GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                GraphicsPipeline.NumRenderTargets  = 1;
                GraphicsPipeline.RTVFormats[0]     = TEX_FORMAT_RGBA8_UNORM;
                GraphicsPipeline.DSVFormat         = TEX_FORMAT_D32_FLOAT;

                pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
                EXPECT_NE(pPSO, nullptr) << "Failed to create test PSO";
            }
        }

        This->WaitSiblingWorkerThreads(SignalIdx);
        SignalIdx = 1 - SignalIdx;
    }
}


TEST_F(MultithreadedResourceCreationTest, CreateResources)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (pDevice->GetDeviceCaps().IsGLDevice())
    {
        GTEST_SKIP() << "Multithreading resource creation is not supported in OpenGL";
    }

#if D3D12_SUPPORTED
    // There is a bug in D3D12 debug layer as of build version 10.0.18362: SetName() method
    // is not protected by a mutex internally. This, in combination with the fact that root signatures are
    // de-duplicated by D3D12 runtime results in a race condition when SetName is called and causes random crashes.

    // As a workaround, we create the root signature ahead of time and reserve enough space for the name
    // to avoid memory allocation.

    D3D12DebugLayerSetNameBugWorkaround D3D12DebugLayerBugWorkaround(pDevice);
#endif

    TestingEnvironment::ScopedReleaseResources AutoResetEnvironment;

    auto numCores = std::thread::hardware_concurrency();
    m_Threads.resize(std::max(numCores, 4u));
    for (auto& t : m_Threads)
        t = std::thread(WorkerThreadFunc, this, &t - m_Threads.data());

    int SignalIdx = 0;
    for (Uint32 iter = 0; iter < NumIterations; ++iter)
    {
        StartWorkerThreadsAndWait(SignalIdx);
        pEnv->ReleaseResources();
        SignalIdx = 1 - SignalIdx;
    }

    m_WorkerThreadSignal[SignalIdx].Trigger(true, -1);
    for (auto& t : m_Threads)
        t.join();
}

} // namespace
