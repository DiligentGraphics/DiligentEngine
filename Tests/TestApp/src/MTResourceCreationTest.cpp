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
#include "MTResourceCreationTest.h"
#include "Errors.h"

using namespace Diligent;

static const char g_ShaderSource[] = 
"void VSMain(out float4 pos : SV_POSITION) \n"
"{                                      \n"
"	pos = float4(0.0, 0.0, 0.0, 0.0);   \n"
"}                                      \n"
"                                       \n"
"void PSMain(out float4 col : SV_TARGET)\n"
"{                                      \n"
"	col = float4(0.0, 0.0, 0.0, 0.0);   \n"
"}                                      \n"
;

MTResourceCreationTest::MTResourceCreationTest(IRenderDevice *pDevice, IDeviceContext *pContext, Uint32 NumThreads) :
    UnitTestBase("Multithreaded resource creation"),
    m_pDevice(pDevice),
    m_pContext(pContext)
{
    m_NumThreadsCompleted = 0;
    m_NumBuffersCreated = 0;
    m_NumTexturesCreated = 0;
    m_NumPSOCreated = 0;

    auto &DevCaps = m_pDevice->GetDeviceCaps();
    if (DevCaps.bMultithreadedResourceCreationSupported)
    {
        m_Threads.resize(NumThreads);
    }
    else
    {
        LOG_WARNING_MESSAGE("Multithreaded resource creation is not supported on this device");
    }
}

void MTResourceCreationTest::WaitForThreadStart(int VarId)
{
    std::unique_lock<std::mutex> lk(m_Mtx);
    m_CondVar.wait(lk, [&]{return m_bReleaseThread[VarId];});
}

void MTResourceCreationTest::WaitThreads(int VarId)
{
    while((size_t)m_NumThreadsCompleted < m_Threads.size())
        std::this_thread::yield();
    m_bReleaseThread[VarId] = false;
    VERIFY_EXPR(m_NumThreadsCompleted == m_Threads.size());
}

void MTResourceCreationTest::StartThreads(int VarId)
{
    {
        std::unique_lock<std::mutex> lk(m_Mtx);
        m_bReleaseThread[VarId] = true;
        m_NumThreadsCompleted = 0;
    }
    m_CondVar.notify_all();
}

static Uint8 RawBufferData[1024];
static Uint8 RawTextureData[1024*1024*4];

void MTResourceCreationTest::ThreadWorkerFunc(bool bIsMasterThread)
{
    while (!m_bStopThreadsFlagInternal)
    {
        if (bIsMasterThread)
            StartThreads(0);
        else
            WaitForThreadStart(0);
        
        RefCntAutoPtr<IBuffer> pBuffer1, pBuffer2,pBuffer3,pBuffer4;
        RefCntAutoPtr<IBufferView> pBufferSRV, pBufferUAV;
        {
            BufferDesc BuffDesc;
            BuffDesc.Usage = USAGE_DEFAULT;
            BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
            BuffDesc.Name = "MT test buffer";
            BuffDesc.uiSizeInBytes = sizeof(RawBufferData);

            BufferData BuffData;
            BuffData.DataSize = BuffDesc.uiSizeInBytes;
            BuffData.pData = RawBufferData;

            m_pDevice->CreateBuffer(BuffDesc, BuffData, &pBuffer1);

            BuffDesc.Mode = BUFFER_MODE_FORMATTED;
            BuffDesc.ElementByteStride = 16;
            BuffDesc.BindFlags = BIND_SHADER_RESOURCE|BIND_UNORDERED_ACCESS;
            m_pDevice->CreateBuffer(BuffDesc, BuffData, &pBuffer2);

            BufferViewDesc ViewDesc;
            ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
            ViewDesc.ByteOffset = 16;
            ViewDesc.Format.NumComponents = 4;
            ViewDesc.Format.IsNormalized = False;
            ViewDesc.Format.ValueType = VT_FLOAT32;
            pBuffer2->CreateView(ViewDesc, &pBufferSRV);

            BuffDesc.BindFlags = BIND_VERTEX_BUFFER | BIND_UNORDERED_ACCESS;
            m_pDevice->CreateBuffer(BuffDesc, BuffData, &pBuffer3);
            ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
            pBuffer3->CreateView(ViewDesc, &pBufferUAV);

            BuffDesc.Mode = BUFFER_MODE_RAW;
            BuffDesc.BindFlags = BIND_INDEX_BUFFER|BIND_UNORDERED_ACCESS;
            m_pDevice->CreateBuffer(BuffDesc, BuffData, &pBuffer4);
            
            m_NumBuffersCreated += 4;

            ++m_NumThreadsCompleted;
        }
        
        if (bIsMasterThread)
        {
            WaitThreads(0);

            // This is the sync point. All threads must reach this point before
            // master thread can proceed. Othre threads are now waiting for StartThreads(1);
            // When we set m_bStopThreadsFlagInternal here,
            // we make sure all threads will exit simultaneously
            m_bStopThreadsFlagInternal = m_bStopThreadsFlag;

            StartThreads(1);
        }
        else
            WaitForThreadStart(1);

        RefCntAutoPtr<ITexture> pTexture;
        {
            TextureDesc TexDesc;
            TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
            TexDesc.Type = RESOURCE_DIM_TEX_2D;
            TexDesc.Width = 1024;
            TexDesc.Height = 1024;
            TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
            TexDesc.MipLevels = 1;

            
            TextureSubResData SubResData;
            SubResData.pData = RawTextureData;
            SubResData.Stride = TexDesc.Width*4;

            TextureData TexData;
            TexData.NumSubresources = 1;
            TexData.pSubResources = &SubResData;

            m_pDevice->CreateTexture(TexDesc, TexData, &pTexture);
            ++m_NumTexturesCreated;

            ++m_NumThreadsCompleted;
        }


        if (bIsMasterThread)
        {
            WaitThreads(1);
            StartThreads(0);
        }
        else
            WaitForThreadStart(0);


        RefCntAutoPtr<IShader> pTrivialVS, pTrivialPS;
        RefCntAutoPtr<IPipelineState> pPSO;
        {
            ShaderCreationAttribs Attrs;
            Attrs.Source = g_ShaderSource;
            Attrs.EntryPoint = "VSMain";
            Attrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
            Attrs.Desc.Name = "TrivialVS (MTResourceCreationTest)";
            Attrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            Attrs.UseCombinedTextureSamplers = true;
            m_pDevice->CreateShader(Attrs, &pTrivialVS);

            Attrs.EntryPoint = "PSMain";
            Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
            Attrs.Desc.Name = "TrivialPS (MTResourceCreationTest)";
            m_pDevice->CreateShader(Attrs, &pTrivialPS);

            PipelineStateDesc PSODesc;
            PSODesc.GraphicsPipeline.pVS = pTrivialVS;
            PSODesc.GraphicsPipeline.pPS = pTrivialPS;
            PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            PSODesc.GraphicsPipeline.NumRenderTargets = 1;
            PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
            PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;
            
            m_pDevice->CreatePipelineState(PSODesc, &pPSO);
            ++m_NumPSOCreated;

            ++m_NumThreadsCompleted;
        }

        if (bIsMasterThread)
        {
            WaitThreads(0);
            StartThreads(1);
        }
        else
            WaitForThreadStart(1);


        {

            ++m_NumThreadsCompleted;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (bIsMasterThread)
            WaitThreads(1);
    }
}

void MTResourceCreationTest::StartThreads()
{
    for(size_t t=0; t < m_Threads.size(); ++t)
        m_Threads[t] = std::thread(&MTResourceCreationTest::ThreadWorkerFunc, this, t == 0);
}

void MTResourceCreationTest::StopThreads()
{
    m_bStopThreadsFlag = true;
    for(auto &t : m_Threads)
        t.join();
    
    std::stringstream infoss;
    infoss << "Buffers created: " << m_NumBuffersCreated << " Textures created: " << m_NumTexturesCreated << " PSO Created: " << m_NumPSOCreated;
    SetStatus(m_Threads.empty() ? TestResult::Skipped : TestResult::Succeeded, infoss.str().c_str());
}
