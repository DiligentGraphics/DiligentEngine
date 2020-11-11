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
#include <cmath>
#include <algorithm>

#include "Tutorial09_Quads.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "imgui.h"
#include "ImGuiUtils.hpp"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial09_Quads();
}

Tutorial09_Quads::~Tutorial09_Quads()
{
    StopWorkerThreads();
}

void Tutorial09_Quads::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType,
                                                      EngineCreateInfo&  Attribs,
                                                      SwapChainDesc&     SCDesc)
{
    SampleBase::GetEngineInitializationAttribs(DeviceType, Attribs, SCDesc);
    Attribs.NumDeferredContexts = std::max(std::thread::hardware_concurrency() - 1, 2u);
#if D3D12_SUPPORTED
    if (DeviceType == RENDER_DEVICE_TYPE_D3D12)
    {
        EngineD3D12CreateInfo& EngD3D12Attribs    = static_cast<EngineD3D12CreateInfo&>(Attribs);
        EngD3D12Attribs.NumCommandsToFlushCmdList = 8192;
    }
#endif
#if VULKAN_SUPPORTED
    if (DeviceType == RENDER_DEVICE_TYPE_VULKAN)
    {
        auto& VkAttrs                       = static_cast<EngineVkCreateInfo&>(Attribs);
        VkAttrs.DynamicHeapSize             = 128 << 20;
        VkAttrs.DynamicHeapPageSize         = 2 << 20;
        VkAttrs.NumCommandsToFlushCmdBuffer = 8192;
    }
#endif
}

void Tutorial09_Quads::CreatePipelineStates(std::vector<StateTransitionDesc>& Barriers)
{
    BlendStateDesc BlendState[NumStates];
    BlendState[1].RenderTargets[0].BlendEnable = true;
    BlendState[1].RenderTargets[0].SrcBlend    = BLEND_FACTOR_SRC_ALPHA;
    BlendState[1].RenderTargets[0].DestBlend   = BLEND_FACTOR_INV_SRC_ALPHA;

    BlendState[2].RenderTargets[0].BlendEnable = true;
    BlendState[2].RenderTargets[0].SrcBlend    = BLEND_FACTOR_INV_SRC_ALPHA;
    BlendState[2].RenderTargets[0].DestBlend   = BLEND_FACTOR_SRC_ALPHA;

    BlendState[3].RenderTargets[0].BlendEnable = true;
    BlendState[3].RenderTargets[0].SrcBlend    = BLEND_FACTOR_SRC_COLOR;
    BlendState[3].RenderTargets[0].DestBlend   = BLEND_FACTOR_INV_SRC_COLOR;

    BlendState[4].RenderTargets[0].BlendEnable = true;
    BlendState[4].RenderTargets[0].SrcBlend    = BLEND_FACTOR_INV_SRC_COLOR;
    BlendState[4].RenderTargets[0].DestBlend   = BLEND_FACTOR_SRC_COLOR;

    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Quad PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    // Disable back face culling
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    // Disable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS, pVSBatched;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Quad VS";
        ShaderCI.FilePath        = "quad.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        ShaderCI.Desc.Name = "Quad VS Batched";
        ShaderCI.FilePath  = "quad_batch.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVSBatched);

        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        CreateUniformBuffer(m_pDevice, sizeof(float4x4), "Instance constants CB", &m_QuadAttribsCB);
        // Explicitly transition the buffer to RESOURCE_STATE_CONSTANT_BUFFER state
        Barriers.emplace_back(m_QuadAttribsCB, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true);
    }

    // Create pixel shaders
    RefCntAutoPtr<IShader> pPS, pPSBatched;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Quad PS";
        ShaderCI.FilePath        = "quad.psh";

        m_pDevice->CreateShader(ShaderCI, &pPS);

        ShaderCI.Desc.Name = "Quad PS Batched";
        ShaderCI.FilePath  = "quad_batch.psh";

        m_pDevice->CreateShader(ShaderCI, &pPSBatched);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // clang-format off
    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    for (int state = 0; state < NumStates; ++state)
    {
        PSOCreateInfo.GraphicsPipeline.BlendDesc = BlendState[state];
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO[0][state]);

        // Since we did not explcitly specify the type for 'QuadAttribs' variable, default
        // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
        // change and are bound directly to the pipeline state object.
        m_pPSO[0][state]->GetStaticVariableByName(SHADER_TYPE_VERTEX, "QuadAttribs")->Set(m_QuadAttribsCB);

        if (state > 0)
            VERIFY(m_pPSO[0][state]->IsCompatibleWith(m_pPSO[0][0]), "PSOs are expected to be compatible");
    }


    PSOCreateInfo.PSODesc.Name = "Batched Quads PSO";
    // clang-format off
    // Define vertex shader input layout
    // This tutorial only uses per-instance data.
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - QuadRotationAndScale
        LayoutElement{0, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
        // Attribute 1 - QuadCenter
        LayoutElement{1, 0, 2, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
        // Attribute 2 - TexArrInd
        LayoutElement{2, 0, 1, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}
    };
    // clang-format on
    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    PSOCreateInfo.pVS = pVSBatched;
    PSOCreateInfo.pPS = pPSBatched;

    for (int state = 0; state < NumStates; ++state)
    {
        PSOCreateInfo.GraphicsPipeline.BlendDesc = BlendState[state];
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO[1][state]);
#ifdef DILIGENT_DEBUG
        if (state > 0)
        {
            VERIFY(m_pPSO[1][state]->IsCompatibleWith(m_pPSO[1][0]), "PSOs are expected to be compatible");
        }
#endif
    }
}

void Tutorial09_Quads::LoadTextures(std::vector<StateTransitionDesc>& Barriers)
{
    RefCntAutoPtr<ITexture> pTexArray;
    for (int tex = 0; tex < NumTextures; ++tex)
    {
        // Load current texture
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = true;
        RefCntAutoPtr<ITexture> SrcTex;

        std::stringstream FileNameSS;
        FileNameSS << "DGLogo" << tex << ".png";
        auto FileName = FileNameSS.str();
        CreateTextureFromFile(FileName.c_str(), loadInfo, m_pDevice, &SrcTex);
        // Get shader resource view from the texture
        m_TextureSRV[tex] = SrcTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        const auto& TexDesc = SrcTex->GetDesc();
        if (pTexArray == nullptr)
        {
            //	Create texture array
            auto TexArrDesc      = TexDesc;
            TexArrDesc.ArraySize = NumTextures;
            TexArrDesc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
            TexArrDesc.Usage     = USAGE_DEFAULT;
            TexArrDesc.BindFlags = BIND_SHADER_RESOURCE;
            m_pDevice->CreateTexture(TexArrDesc, nullptr, &pTexArray);
        }
        // Copy current texture into the texture array
        for (Uint32 mip = 0; mip < TexDesc.MipLevels; ++mip)
        {
            CopyTextureAttribs CopyAttribs(SrcTex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pTexArray, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            CopyAttribs.SrcMipLevel = mip;
            CopyAttribs.DstMipLevel = mip;
            CopyAttribs.DstSlice    = tex;
            m_pImmediateContext->CopyTexture(CopyAttribs);
        }
        // Transition textures to shader resource state
        Barriers.emplace_back(SrcTex, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true);
    }
    m_TexArraySRV = pTexArray->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    // Transition all textures to shader resource state
    Barriers.emplace_back(pTexArray, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true);

    // Set texture SRV in the SRB
    for (int tex = 0; tex < NumTextures; ++tex)
    {
        // Create one Shader Resource Binding for every texture
        // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
        m_pPSO[0][0]->CreateShaderResourceBinding(&m_SRB[tex], true);
        m_SRB[tex]->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV[tex]);
    }

    m_pPSO[1][0]->CreateShaderResourceBinding(&m_BatchSRB, true);
    m_BatchSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TexArraySRV);
}

void Tutorial09_Quads::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::InputInt("Num Quads", &m_NumQuads, 100, 1000, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            m_NumQuads = std::min(std::max(m_NumQuads, 1), 100000);
            InitializeQuads();
        }
        if (ImGui::InputInt("Batch Size", &m_BatchSize, 1, 5))
        {
            m_BatchSize = std::min(std::max(m_BatchSize, 1), 100);
            CreateInstanceBuffer();
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


void Tutorial09_Quads::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    m_MaxThreads       = static_cast<int>(m_pDeferredContexts.size());
    m_NumWorkerThreads = std::min(7, m_MaxThreads);

    std::vector<StateTransitionDesc> Barriers;
    CreatePipelineStates(Barriers);
    LoadTextures(Barriers);
    m_pImmediateContext->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());

    InitializeQuads();

    if (m_BatchSize > 1)
        CreateInstanceBuffer();

    StartWorkerThreads(m_NumWorkerThreads);
}

void Tutorial09_Quads::InitializeQuads()
{
    m_Quads.resize(m_NumQuads);

    std::mt19937 gen; // Standard mersenne_twister_engine. Use default seed
                      // to generate consistent distribution.

    std::uniform_real_distribution<float> scale_distr(0.01f, 0.05f);
    std::uniform_real_distribution<float> pos_distr(-0.95f, +0.95f);
    std::uniform_real_distribution<float> move_dir_distr(-0.1f, +0.1f);
    std::uniform_real_distribution<float> angle_distr(-PI_F, +PI_F);
    std::uniform_real_distribution<float> rot_distr(-PI_F * 0.5f, +PI_F * 0.5f);
    std::uniform_int_distribution<Int32>  tex_distr(0, NumTextures - 1);
    std::uniform_int_distribution<Int32>  state_distr(0, NumStates - 1);

    for (int quad = 0; quad < m_NumQuads; ++quad)
    {
        auto& CurrInst     = m_Quads[quad];
        CurrInst.Size      = scale_distr(gen);
        CurrInst.Angle     = angle_distr(gen);
        CurrInst.Pos.x     = pos_distr(gen);
        CurrInst.Pos.y     = pos_distr(gen);
        CurrInst.MoveDir.x = move_dir_distr(gen);
        CurrInst.MoveDir.y = move_dir_distr(gen);
        CurrInst.RotSpeed  = rot_distr(gen);
        // Texture array index
        CurrInst.TextureInd = tex_distr(gen);
        CurrInst.StateInd   = state_distr(gen);
    }
}

void Tutorial09_Quads::UpdateQuads(float elapsedTime)
{
    std::mt19937 gen; // Standard mersenne_twister_engine. Use default seed
                      // to generate consistent distribution.

    std::uniform_real_distribution<float> rot_distr(-PI_F * 0.5f, +PI_F * 0.5f);
    for (int quad = 0; quad < m_NumQuads; ++quad)
    {
        auto& CurrInst = m_Quads[quad];
        CurrInst.Angle += CurrInst.RotSpeed * elapsedTime;
        if (std::abs(CurrInst.Pos.x + CurrInst.MoveDir.x * elapsedTime) > 0.95)
        {
            CurrInst.MoveDir.x *= -1.f;
            CurrInst.RotSpeed = rot_distr(gen);
        }
        CurrInst.Pos.x += CurrInst.MoveDir.x * elapsedTime;
        if (std::abs(CurrInst.Pos.y + CurrInst.MoveDir.y * elapsedTime) > 0.95)
        {
            CurrInst.MoveDir.y *= -1.f;
            CurrInst.RotSpeed = rot_distr(gen);
        }
        CurrInst.Pos.y += CurrInst.MoveDir.y * elapsedTime;
    }
}

void Tutorial09_Quads::StartWorkerThreads(size_t NumThreads)
{
    m_WorkerThreads.resize(NumThreads);
    for (Uint32 t = 0; t < m_WorkerThreads.size(); ++t)
    {
        m_WorkerThreads[t] = std::thread(WorkerThreadFunc, this, t);
    }
    m_CmdLists.resize(NumThreads);
}

void Tutorial09_Quads::StopWorkerThreads()
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

void Tutorial09_Quads::WorkerThreadFunc(Tutorial09_Quads* pThis, Uint32 ThreadNum)
{
    // Every thread should use its own deferred context
    IDeviceContext* pDeferredCtx = pThis->m_pDeferredContexts[ThreadNum];

    const int NumWorkerThreads = static_cast<int>(pThis->m_WorkerThreads.size());
    VERIFY_EXPR(NumWorkerThreads > 0);
    for (;;)
    {
        // Wait for the signal
        auto SignaledValue = pThis->m_RenderSubsetSignal.Wait(true, NumWorkerThreads);
        if (SignaledValue < 0)
            return;

        // Render current subset using the deferred context
        if (pThis->m_BatchSize > 1)
            pThis->RenderSubset<true>(pDeferredCtx, 1 + ThreadNum);
        else
            pThis->RenderSubset<false>(pDeferredCtx, 1 + ThreadNum);

        // Finish command list
        RefCntAutoPtr<ICommandList> pCmdList;
        pDeferredCtx->FinishCommandList(&pCmdList);
        pThis->m_CmdLists[ThreadNum] = pCmdList;

        {
            std::lock_guard<std::mutex> Lock{pThis->m_NumThreadsCompletedMtx};
            // Increment the number of completed threads
            ++pThis->m_NumThreadsCompleted;
            if (pThis->m_NumThreadsCompleted == NumWorkerThreads)
                pThis->m_ExecuteCommandListsSignal.Trigger();
        }

        pThis->m_GotoNextFrameSignal.Wait(true, NumWorkerThreads);

        // Call FinishFrame() to release dynamic resources allocated by deferred contexts
        // IMPORTANT: we must wait until the command lists are submitted for execution
        // because FinishFrame() invalidates all dynamic resources
        pDeferredCtx->FinishFrame();

        ++pThis->m_NumThreadsReady;
        // We must wait until all threads reach this point, because
        // m_GotoNextFrameSignal must be unsignaled before we proceed to
        // RenderSubsetSignal to avoid one thread go through the loop twice in
        // a row
        while (pThis->m_NumThreadsReady < NumWorkerThreads)
            std::this_thread::yield();
        VERIFY_EXPR(!pThis->m_GotoNextFrameSignal.IsTriggered());
    }
}

template <bool UseBatch>
void Tutorial09_Quads::RenderSubset(IDeviceContext* pCtx, Uint32 Subset)
{
    // Deferred contexts start in default state. We must bind everything to the context
    // Render targets are set and transitioned to correct states by the main thread, here we only verify states
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    pCtx->SetRenderTargets(1, &pRTV, m_pSwapChain->GetDepthBufferDSV(), RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    if (UseBatch)
    {
        Uint32   offsets[] = {0};
        IBuffer* pBuffs[]  = {m_BatchDataBuffer};
        pCtx->SetVertexBuffers(0, _countof(pBuffs), pBuffs, offsets, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_RESET);
    }
    DrawAttribs DrawAttrs;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    DrawAttrs.NumVertices = 4;

    Uint32       NumSubsets   = Uint32{1} + static_cast<Uint32>(m_WorkerThreads.size());
    const Uint32 TotalQuads   = static_cast<Uint32>(m_Quads.size());
    const Uint32 TotalBatches = (TotalQuads + m_BatchSize - 1) / m_BatchSize;
    const Uint32 SusbsetSize  = TotalBatches / NumSubsets;
    const Uint32 StartBatch   = SusbsetSize * Subset;
    const Uint32 EndBatch     = (Subset < NumSubsets - 1) ? SusbsetSize * (Subset + 1) : TotalBatches;
    for (Uint32 batch = StartBatch; batch < EndBatch; ++batch)
    {
        const Uint32 StartInst = batch * m_BatchSize;
        const Uint32 EndInst   = std::min(StartInst + static_cast<Uint32>(m_BatchSize), static_cast<Uint32>(m_NumQuads));

        // Set the pipeline state
        auto StateInd = m_Quads[StartInst].StateInd;
        pCtx->SetPipelineState(m_pPSO[UseBatch ? 1 : 0][StateInd]);

        MapHelper<InstanceData> BatchData;
        if (UseBatch)
        {
            pCtx->CommitShaderResources(m_BatchSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
            BatchData.Map(pCtx, m_BatchDataBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        }

        for (Uint32 inst = StartInst; inst < EndInst; ++inst)
        {
            const auto& CurrInstData = m_Quads[inst];
            // Shader resources have been explicitly transitioned to correct states, so
            // RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode is not needed.
            // Instead, we use RESOURCE_STATE_TRANSITION_MODE_VERIFY mode to
            // verify that all resources are in correct states. This mode only has effect
            // in debug and development builds
            if (!UseBatch)
                pCtx->CommitShaderResources(m_SRB[CurrInstData.TextureInd], RESOURCE_STATE_TRANSITION_MODE_VERIFY);

            {
                // clang-format off
                float2x2 ScaleMatr
                {
                    CurrInstData.Size,               0.f,
                    0.f,               CurrInstData.Size
                };
                // clang-format on
                float    sinAngle = sinf(CurrInstData.Angle);
                float    cosAngle = cosf(CurrInstData.Angle);
                float2x2 RotMatr(cosAngle, -sinAngle,
                                 sinAngle, cosAngle);
                auto     Matr = ScaleMatr * RotMatr;

                float4 QuadRotationAndScale(Matr.m00, Matr.m10, Matr.m01, Matr.m11);

                if (UseBatch)
                {
                    auto& CurrQuad                = BatchData[inst - StartInst];
                    CurrQuad.QuadRotationAndScale = QuadRotationAndScale;
                    CurrQuad.QuadCenter           = CurrInstData.Pos;
                    CurrQuad.TexArrInd            = static_cast<float>(CurrInstData.TextureInd);
                }
                else
                {
                    struct QuadAttribs
                    {
                        float4 g_QuadRotationAndScale;
                        float4 g_QuadCenter;
                    };

                    // Map the buffer and write current world-view-projection matrix
                    MapHelper<QuadAttribs> InstData(pCtx, m_QuadAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);

                    InstData->g_QuadRotationAndScale = QuadRotationAndScale;
                    InstData->g_QuadCenter.x         = CurrInstData.Pos.x;
                    InstData->g_QuadCenter.y         = CurrInstData.Pos.y;
                }
            }
        }

        if (UseBatch)
            BatchData.Unmap();

        DrawAttrs.NumInstances = EndInst - StartInst;
        pCtx->Draw(DrawAttrs);
    }
}

// Render a frame
void Tutorial09_Quads::Render()
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

    if (m_BatchSize > 1)
        RenderSubset<true>(m_pImmediateContext, 0);
    else
        RenderSubset<false>(m_pImmediateContext, 0);

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

void Tutorial09_Quads::CreateInstanceBuffer()
{
    // Create instance data buffer that will store transformation matrices
    BufferDesc InstBuffDesc;
    InstBuffDesc.Name = "Batch data buffer";
    // Use default usage as this buffer will only be updated when grid size changes
    InstBuffDesc.Usage          = USAGE_DYNAMIC;
    InstBuffDesc.BindFlags      = BIND_VERTEX_BUFFER;
    InstBuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    InstBuffDesc.uiSizeInBytes  = sizeof(InstanceData) * m_BatchSize;
    m_BatchDataBuffer.Release();
    m_pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_BatchDataBuffer);
    StateTransitionDesc Barrier(m_BatchDataBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    m_pImmediateContext->TransitionResourceStates(1, &Barrier);
}

void Tutorial09_Quads::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    UpdateQuads(static_cast<float>(ElapsedTime));
}

} // namespace Diligent
