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

#include <random>
#include <string>
#include <algorithm>

#include "Tutorial06_Multithreading.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "../../Common/src/TexturedCube.hpp"
#include "imgui.h"
#include "ImGuiUtils.hpp"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial06_Multithreading();
}

Tutorial06_Multithreading::~Tutorial06_Multithreading()
{
    StopWorkerThreads();
}

void Tutorial06_Multithreading::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType,
                                                               EngineCreateInfo&  Attribs,
                                                               SwapChainDesc&     SCDesc)
{
    SampleBase::GetEngineInitializationAttribs(DeviceType, Attribs, SCDesc);
    Attribs.NumDeferredContexts = std::max(std::thread::hardware_concurrency() - 1, 2u);
#if VULKAN_SUPPORTED
    if (DeviceType == RENDER_DEVICE_TYPE_VULKAN)
    {
        auto& VkAttrs           = static_cast<EngineVkCreateInfo&>(Attribs);
        VkAttrs.DynamicHeapSize = 26 << 20; // Enough space for 32x32x32x256 bytes allocations for 3 frames
    }
#endif
}

void Tutorial06_Multithreading::CreatePipelineState(std::vector<StateTransitionDesc>& Barriers)
{
    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

    m_pPSO = TexturedCube::CreatePipelineState(m_pDevice,
                                               m_pSwapChain->GetDesc().ColorBufferFormat,
                                               m_pSwapChain->GetDesc().DepthBufferFormat,
                                               pShaderSourceFactory,
                                               "cube.vsh",
                                               "cube.psh");

    // Create dynamic uniform buffer that will store our transformation matrix
    // Dynamic buffers can be frequently updated by the CPU
    CreateUniformBuffer(m_pDevice, sizeof(float4x4) * 2, "VS constants CB", &m_VSConstants);
    CreateUniformBuffer(m_pDevice, sizeof(float4x4), "Instance constants CB", &m_InstanceConstants);
    // Explicitly transition the buffers to RESOURCE_STATE_CONSTANT_BUFFER state
    Barriers.emplace_back(m_VSConstants, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true);
    Barriers.emplace_back(m_InstanceConstants, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true);

    // Since we did not explcitly specify the type for 'Constants' and 'InstanceData' variables,
    // default type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
    // never change and are bound directly to the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "InstanceData")->Set(m_InstanceConstants);
}

void Tutorial06_Multithreading::LoadTextures(std::vector<StateTransitionDesc>& Barriers)
{
    // Load textures
    for (int tex = 0; tex < NumTextures; ++tex)
    {
        // Load current texture
        std::stringstream FileNameSS;
        FileNameSS << "DGLogo" << tex << ".png";
        auto FileName = FileNameSS.str();

        RefCntAutoPtr<ITexture> SrcTex = TexturedCube::LoadTexture(m_pDevice, FileName.c_str());
        // Get shader resource view from the texture
        m_TextureSRV[tex] = SrcTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        // Transition textures to shader resource state
        Barriers.emplace_back(SrcTex, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true);
    }

    // Set texture SRV in the SRB
    for (int tex = 0; tex < NumTextures; ++tex)
    {
        // Create one Shader Resource Binding for every texture
        // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
        m_pPSO->CreateShaderResourceBinding(&m_SRB[tex], true);
        m_SRB[tex]->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV[tex]);
    }
}

void Tutorial06_Multithreading::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::SliderInt("Grid Size", &m_GridSize, 1, 32))
        {
            PopulateInstanceData();
        }
        {
            ImGui::ScopedDisabler Disable(m_MaxThreads == 0);
            if (ImGui::SliderInt("Worker Threads", &m_NumWorkerThreads, 0, m_MaxThreads))
            {
                StopWorkerThreads();
                StartWorkerThreads(m_NumWorkerThreads);
            }
        }
    }

    ImGui::End();
}

void Tutorial06_Multithreading::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    m_MaxThreads       = static_cast<int>(m_pDeferredContexts.size());
    m_NumWorkerThreads = std::min(4, m_MaxThreads);

    std::vector<StateTransitionDesc> Barriers;

    CreatePipelineState(Barriers);

    // Load textured cube
    m_CubeVertexBuffer = TexturedCube::CreateVertexBuffer(m_pDevice);
    m_CubeIndexBuffer  = TexturedCube::CreateIndexBuffer(m_pDevice);
    // Explicitly transition vertex and index buffers to required states
    Barriers.emplace_back(m_CubeVertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    Barriers.emplace_back(m_CubeIndexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, true);
    LoadTextures(Barriers);

    // Execute all barriers
    m_pImmediateContext->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());

    PopulateInstanceData();

    StartWorkerThreads(m_NumWorkerThreads);
}

void Tutorial06_Multithreading::PopulateInstanceData()
{
    m_InstanceData.resize(m_GridSize * m_GridSize * m_GridSize);
    // Populate instance data buffer
    float fGridSize = static_cast<float>(m_GridSize);

    std::mt19937 gen; // Standard mersenne_twister_engine. Use default seed
                      // to generate consistent distribution.

    std::uniform_real_distribution<float> scale_distr(0.3f, 1.0f);
    std::uniform_real_distribution<float> offset_distr(-0.15f, +0.15f);
    std::uniform_real_distribution<float> rot_distr(-PI_F, +PI_F);
    std::uniform_int_distribution<Int32>  tex_distr(0, NumTextures - 1);

    float BaseScale = 0.6f / fGridSize;
    int   instId    = 0;
    for (int x = 0; x < m_GridSize; ++x)
    {
        for (int y = 0; y < m_GridSize; ++y)
        {
            for (int z = 0; z < m_GridSize; ++z)
            {
                // Add random offset from central position in the grid
                float xOffset = 2.f * (x + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                float yOffset = 2.f * (y + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                float zOffset = 2.f * (z + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                // Random scale
                float scale = BaseScale * scale_distr(gen);
                // Random rotation
                float4x4 rotation = float4x4::RotationX(rot_distr(gen)) * float4x4::RotationY(rot_distr(gen)) * float4x4::RotationZ(rot_distr(gen));
                // Combine rotation, scale and translation
                float4x4 matrix   = rotation * float4x4::Scale(scale, scale, scale) * float4x4::Translation(xOffset, yOffset, zOffset);
                auto&    CurrInst = m_InstanceData[instId++];
                CurrInst.Matrix   = matrix;
                // Texture array index
                CurrInst.TextureInd = tex_distr(gen);
            }
        }
    }
}

void Tutorial06_Multithreading::StartWorkerThreads(size_t NumThreads)
{
    m_WorkerThreads.resize(NumThreads);
    for (Uint32 t = 0; t < m_WorkerThreads.size(); ++t)
    {
        m_WorkerThreads[t] = std::thread(WorkerThreadFunc, this, t);
    }
    m_CmdLists.resize(NumThreads);
}

void Tutorial06_Multithreading::StopWorkerThreads()
{
    m_RenderSubsetSignal.Trigger(true, -1);

    for (auto& thread : m_WorkerThreads)
    {
        thread.join();
    }
    m_RenderSubsetSignal.Reset();
    m_WorkerThreads.clear();
    m_CmdLists.clear();
}

void Tutorial06_Multithreading::WorkerThreadFunc(Tutorial06_Multithreading* pThis, Uint32 ThreadNum)
{
    // Every thread should use its own deferred context
    IDeviceContext* pDeferredCtx     = pThis->m_pDeferredContexts[ThreadNum];
    const int       NumWorkerThreads = static_cast<int>(pThis->m_WorkerThreads.size());
    for (;;)
    {
        // Wait for the signal
        auto SignaledValue = pThis->m_RenderSubsetSignal.Wait(true, NumWorkerThreads);
        if (SignaledValue < 0)
            return;

        // Render current subset using the deferred context
        pThis->RenderSubset(pDeferredCtx, 1 + ThreadNum);

        // Finish command list
        RefCntAutoPtr<ICommandList> pCmdList;
        pDeferredCtx->FinishCommandList(&pCmdList);
        pThis->m_CmdLists[ThreadNum] = pCmdList;

        {
            std::lock_guard<std::mutex> Lock(pThis->m_NumThreadsCompletedMtx);
            // Increment the number of completed threads
            ++pThis->m_NumThreadsCompleted;
            if (pThis->m_NumThreadsCompleted == NumWorkerThreads)
                pThis->m_ExecuteCommandListsSignal.Trigger();
        }

        pThis->m_GotoNextFrameSignal.Wait(true, NumWorkerThreads);

        // Call FinishFrame() to release dynamic resources allocated by deferred contexts
        // IMPORTANT: we must wait until the command lists are submitted for execution
        // because FinishFrame() invalidates all dynamic resources.
        pDeferredCtx->FinishFrame();

        ++pThis->m_NumThreadsReady;
        // We must wait until all threads reach this point, because
        // m_GotoNextFrameSignal must be unsignaled before we proceed to
        // RenderSubsetSignal to avoid one thread going through the loop twice in
        // a row.
        while (pThis->m_NumThreadsReady < NumWorkerThreads)
            std::this_thread::yield();
        VERIFY_EXPR(!pThis->m_GotoNextFrameSignal.IsTriggered());
    }
}

void Tutorial06_Multithreading::RenderSubset(IDeviceContext* pCtx, Uint32 Subset)
{
    // Deferred contexts start in default state. We must bind everything to the context.
    // Render targets are set and transitioned to correct states by the main thread, here we only verify the states.
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    pCtx->SetRenderTargets(1, &pRTV, m_pSwapChain->GetDepthBufferDSV(), RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    {
        // Map the buffer and write current world-view-projection matrix

        // Since this is a dynamic buffer, it must be mapped in every context before
        // it can be used even though the matrices are the same.
        MapHelper<float4x4> CBConstants(pCtx, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        CBConstants[0] = m_ViewProjMatrix.Transpose();
        CBConstants[1] = m_RotationMatrix.Transpose();
    }

    // Bind vertex and index buffers. This must be done for every context
    Uint32   offsets[] = {0, 0};
    IBuffer* pBuffs[]  = {m_CubeVertexBuffer};
    pCtx->SetVertexBuffers(0, _countof(pBuffs), pBuffs, offsets, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_RESET);
    pCtx->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    DrawIndexedAttribs DrawAttrs;     // This is an indexed draw call
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = 36;
    DrawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;

    // Set the pipeline state
    pCtx->SetPipelineState(m_pPSO);
    Uint32 NumSubsets   = Uint32{1} + static_cast<Uint32>(m_WorkerThreads.size());
    Uint32 NumInstances = static_cast<Uint32>(m_InstanceData.size());
    Uint32 SusbsetSize  = NumInstances / NumSubsets;
    Uint32 StartInst    = SusbsetSize * Subset;
    Uint32 EndInst      = (Subset < NumSubsets - 1) ? SusbsetSize * (Subset + 1) : NumInstances;
    for (size_t inst = StartInst; inst < EndInst; ++inst)
    {
        const auto& CurrInstData = m_InstanceData[inst];
        // Shader resources have been explicitly transitioned to correct states, so
        // RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode is not needed.
        // Instead, we use RESOURCE_STATE_TRANSITION_MODE_VERIFY mode to
        // verify that all resources are in correct states. This mode only has effect
        // in debug and development builds.
        pCtx->CommitShaderResources(m_SRB[CurrInstData.TextureInd], RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        {
            // Map the buffer and write current world-view-projection matrix
            MapHelper<float4x4> InstData(pCtx, m_InstanceConstants, MAP_WRITE, MAP_FLAG_DISCARD);
            if (InstData == nullptr)
            {
                LOG_ERROR_MESSAGE("Failed to map instance data buffer");
                break;
            }
            *InstData = CurrInstData.Matrix.Transpose();
        }

        pCtx->DrawIndexed(DrawAttrs);
    }
}

// Render a frame
void Tutorial06_Multithreading::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    if (!m_WorkerThreads.empty())
    {
        m_NumThreadsCompleted = 0;
        m_RenderSubsetSignal.Trigger(true);
    }

    RenderSubset(m_pImmediateContext, 0);

    if (!m_WorkerThreads.empty())
    {
        m_ExecuteCommandListsSignal.Wait(true, 1);

        for (auto& cmdList : m_CmdLists)
        {
            m_pImmediateContext->ExecuteCommandList(cmdList);
            // Release command lists now to release all outstanding references
            // In d3d11 mode, command lists hold references to the swap chain's back buffer
            // that cause swap chain resize to fail
            cmdList.Release();
        }

        m_NumThreadsReady = 0;
        m_GotoNextFrameSignal.Trigger(true);
    }
}

void Tutorial06_Multithreading::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    // Set the cube view matrix
    float4x4 View = float4x4::RotationX(-0.6f) * float4x4::Translation(0.f, 0.f, 4.0f);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    // Compute view-projection matrix
    m_ViewProjMatrix = View * SrfPreTransform * Proj;

    // Global rotation matrix
    m_RotationMatrix = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f) * float4x4::RotationX(-static_cast<float>(CurrTime) * 0.25f);
}

} // namespace Diligent
