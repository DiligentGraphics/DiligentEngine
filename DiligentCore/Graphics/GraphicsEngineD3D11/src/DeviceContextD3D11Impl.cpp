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

#include "pch.h"
#include "DeviceContextD3D11Impl.hpp"
#include "BufferD3D11Impl.hpp"
#include "ShaderD3D11Impl.hpp"
#include "Texture1D_D3D11.hpp"
#include "Texture2D_D3D11.hpp"
#include "Texture3D_D3D11.hpp"
#include "SamplerD3D11Impl.hpp"
#include "D3D11TypeConversions.hpp"
#include "TextureViewD3D11Impl.hpp"
#include "PipelineStateD3D11Impl.hpp"
#include "ShaderResourceBindingD3D11Impl.hpp"
#include "EngineD3D11Defines.h"
#include "CommandListD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "FenceD3D11Impl.hpp"
#include "QueryD3D11Impl.hpp"

namespace Diligent
{
DeviceContextD3D11Impl::DeviceContextD3D11Impl(IReferenceCounters*                 pRefCounters,
                                               IMemoryAllocator&                   Allocator,
                                               RenderDeviceD3D11Impl*              pDevice,
                                               ID3D11DeviceContext*                pd3d11DeviceContext,
                                               const struct EngineD3D11CreateInfo& EngineAttribs,
                                               bool                                bIsDeferred) :
    // clang-format off
    TDeviceContextBase
    {
        pRefCounters,
        pDevice,
        bIsDeferred
    },
    m_pd3d11DeviceContext{pd3d11DeviceContext        },
    m_DebugFlags         {EngineAttribs.DebugFlags   },
    m_CmdListAllocator   {GetRawAllocator(), sizeof(CommandListD3D11Impl), 64}
// clang-format on
{
}

IMPLEMENT_QUERY_INTERFACE(DeviceContextD3D11Impl, IID_DeviceContextD3D11, TDeviceContextBase)

void DeviceContextD3D11Impl::SetPipelineState(IPipelineState* pPipelineState)
{
    auto* pPipelineStateD3D11 = ValidatedCast<PipelineStateD3D11Impl>(pPipelineState);
    if (PipelineStateD3D11Impl::IsSameObject(m_pPipelineState, pPipelineStateD3D11))
        return;

    TDeviceContextBase::SetPipelineState(pPipelineStateD3D11, 0 /*Dummy*/);
    auto& Desc = pPipelineStateD3D11->GetDesc();
    if (Desc.PipelineType == PIPELINE_TYPE_COMPUTE)
    {
        auto* pd3d11CS = pPipelineStateD3D11->GetD3D11ComputeShader();
        if (pd3d11CS == nullptr)
        {
            LOG_ERROR("Compute shader is not set in the pipeline");
            return;
        }

#define COMMIT_SHADER(SN, ShaderName)                                       \
    do                                                                      \
    {                                                                       \
        auto* pd3d11Shader = pPipelineStateD3D11->GetD3D11##ShaderName();   \
        if (m_CommittedD3DShaders[SN##Ind] != pd3d11Shader)                 \
        {                                                                   \
            m_CommittedD3DShaders[SN##Ind] = pd3d11Shader;                  \
            m_pd3d11DeviceContext->SN##SetShader(pd3d11Shader, nullptr, 0); \
        }                                                                   \
    } while (false)

        COMMIT_SHADER(CS, ComputeShader);
    }
    else if (Desc.PipelineType == PIPELINE_TYPE_GRAPHICS)
    {
        COMMIT_SHADER(VS, VertexShader);
        COMMIT_SHADER(PS, PixelShader);
        COMMIT_SHADER(GS, GeometryShader);
        COMMIT_SHADER(HS, HullShader);
        COMMIT_SHADER(DS, DomainShader);
#undef COMMIT_SHADER

        const auto& GraphicsPipeline = pPipelineStateD3D11->GetGraphicsPipelineDesc();

        m_pd3d11DeviceContext->OMSetBlendState(pPipelineStateD3D11->GetD3D11BlendState(), m_BlendFactors, GraphicsPipeline.SampleMask);
        m_pd3d11DeviceContext->RSSetState(pPipelineStateD3D11->GetD3D11RasterizerState());
        m_pd3d11DeviceContext->OMSetDepthStencilState(pPipelineStateD3D11->GetD3D11DepthStencilState(), m_StencilRef);

        auto* pd3d11InputLayout = pPipelineStateD3D11->GetD3D11InputLayout();
        // It is safe to perform raw pointer comparison as the device context
        // keeps bound input layout alive
        if (m_CommittedD3D11InputLayout != pd3d11InputLayout)
        {
            m_pd3d11DeviceContext->IASetInputLayout(pd3d11InputLayout);
            m_CommittedD3D11InputLayout = pd3d11InputLayout;
        }

        auto PrimTopology = GraphicsPipeline.PrimitiveTopology;
        if (m_CommittedPrimitiveTopology != PrimTopology)
        {
            m_CommittedPrimitiveTopology = PrimTopology;
            m_CommittedD3D11PrimTopology = TopologyToD3D11Topology(PrimTopology);
            m_pd3d11DeviceContext->IASetPrimitiveTopology(m_CommittedD3D11PrimTopology);
        }
    }
    else
    {
        UNEXPECTED(GetPipelineTypeString(Desc.PipelineType), " pipelines '", Desc.Name, "' are not supported in OpenGL");
    }
}

// clang-format off

/// Helper macro used to create an array of device context methods to
/// set particular resource for every shader stage
#define DEFINE_D3D11CTX_FUNC_POINTERS(ArrayName, FuncName) \
    typedef decltype (&ID3D11DeviceContext::VS##FuncName) T##FuncName##Type;  \
    static const T##FuncName##Type ArrayName[] =    \
    {                                           \
        &ID3D11DeviceContext::VS##FuncName,  \
        &ID3D11DeviceContext::PS##FuncName,  \
        &ID3D11DeviceContext::GS##FuncName,  \
        &ID3D11DeviceContext::HS##FuncName,  \
        &ID3D11DeviceContext::DS##FuncName,  \
        &ID3D11DeviceContext::CS##FuncName   \
    };

    DEFINE_D3D11CTX_FUNC_POINTERS(SetCBMethods,      SetConstantBuffers)
    DEFINE_D3D11CTX_FUNC_POINTERS(SetSRVMethods,     SetShaderResources)
    DEFINE_D3D11CTX_FUNC_POINTERS(SetSamplerMethods, SetSamplers)

    typedef decltype (&ID3D11DeviceContext::CSSetUnorderedAccessViews) TSetUnorderedAccessViewsType;
    static const TSetUnorderedAccessViewsType SetUAVMethods[] =
    {
        nullptr, // VS
        reinterpret_cast<TSetUnorderedAccessViewsType>(&ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews),  // Little hack for PS
        nullptr, // GS
        nullptr, // HS
        nullptr, // DS
        &ID3D11DeviceContext::CSSetUnorderedAccessViews // CS
    };

// clang-format on

// http://diligentgraphics.com/diligent-engine/architecture/d3d11/committing-shader-resources-to-the-gpu-pipeline/
template <bool TransitionResources, bool CommitResources>
void DeviceContextD3D11Impl::TransitionAndCommitShaderResources(IPipelineState* pPSO, IShaderResourceBinding* pShaderResourceBinding, bool VerifyStates)
{
    VERIFY_EXPR(pPSO != nullptr);
    static_assert(TransitionResources || CommitResources, "At least one of TransitionResources or CommitResources flags is expected to be true");

    auto* pPipelineStateD3D11 = ValidatedCast<PipelineStateD3D11Impl>(pPSO);

    if (pShaderResourceBinding == nullptr)
    {
#ifdef DILIGENT_DEVELOPMENT
        bool ResourcesPresent = false;
        for (Uint32 s = 0; s < pPipelineStateD3D11->GetNumShaderStages(); ++s)
        {
            auto* pShaderD3D11 = pPipelineStateD3D11->GetShader(s);
            if (pShaderD3D11->GetD3D11Resources()->GetTotalResources() > 0)
                ResourcesPresent = true;
        }

        if (ResourcesPresent)
        {
            LOG_ERROR_MESSAGE("Pipeline state '", pPSO->GetDesc().Name, "' requires shader resource binding object to ",
                              (CommitResources ? "commit" : "transition"), " resources, but none is provided.");
        }
#endif
        return;
    }


    auto pShaderResBindingD3D11 = ValidatedCast<ShaderResourceBindingD3D11Impl>(pShaderResourceBinding);
#ifdef DILIGENT_DEVELOPMENT
    if (pPipelineStateD3D11->IsIncompatibleWith(pShaderResourceBinding->GetPipelineState()))
    {
        LOG_ERROR_MESSAGE("Shader resource binding does not match Pipeline State");
        return;
    }
#endif

    auto NumShaders = pShaderResBindingD3D11->GetNumActiveShaders();
    VERIFY(NumShaders == pPipelineStateD3D11->GetNumShaderStages(), "Number of active shaders in shader resource binding is not consistent with the number of shaders in the pipeline state");

#ifdef DILIGENT_DEVELOPMENT
    {
        bool StaticResourcesPresent = false;
        for (Uint32 s = 0; s < NumShaders; ++s)
        {
            const auto& StaticResLayout = pPipelineStateD3D11->GetStaticResourceLayout(s);
            if (StaticResLayout.GetTotalResourceCount() > 0)
                StaticResourcesPresent = true;
        }
        // Static resource bindings are verified in BindStaticShaderResources()
        if (StaticResourcesPresent && !pShaderResBindingD3D11->IsStaticResourcesBound())
        {
            LOG_ERROR_MESSAGE("Static resources have not been initialized in the shader resource binding object being committed for PSO '", pPSO->GetDesc().Name, "'. Please call IShaderResourceBinding::InitializeStaticResources().");
        }
    }
#endif

    bool ClearPixelShaderUAVs = m_NumCommittedUAVs[PSInd] > 0;
    // First, commit all UAVs for all shader stages. This will unbind them from input
    for (Uint32 s = 0; s < NumShaders; ++s)
    {
        const auto ShaderType    = pShaderResBindingD3D11->GetActiveShaderType(s);
        const auto ShaderTypeInd = GetShaderTypeIndex(ShaderType);

#ifdef DILIGENT_DEVELOPMENT
        auto* pShaderD3D11 = pPipelineStateD3D11->GetShader(s);
        VERIFY_EXPR(ShaderType == pShaderD3D11->GetDesc().ShaderType);
#endif

        auto& Cache   = pShaderResBindingD3D11->GetResourceCache(s);
        auto  NumUAVs = Cache.GetUAVCount();
        if (NumUAVs)
        {
            ShaderResourceCacheD3D11::CachedResource* CachedUAVResources;
            ID3D11UnorderedAccessView**               d3d11UAVs;
            Cache.GetUAVArrays(CachedUAVResources, d3d11UAVs);

            auto* CommittedD3D11UAVs   = m_CommittedD3D11UAVs[ShaderTypeInd];
            auto* CommittedD3D11UAVRes = m_CommittedD3D11UAVResources[ShaderTypeInd];

            if (ShaderTypeInd == PSInd)
                ClearPixelShaderUAVs = false;

            UINT MinSlot = UINT_MAX;
            UINT MaxSlot = 0;

            for (Uint32 uav = 0; uav < NumUAVs; ++uav)
            {
                VERIFY_EXPR(uav < Cache.GetUAVCount());
                auto& UAVRes = CachedUAVResources[uav];
                // WARNING! This code is not thread-safe. It is up to the application to make
                // sure that multiple threads do not modify the texture state simultaneously.
                if (TransitionResources)
                {
                    if (auto* pTexture = ValidatedCast<TextureBaseD3D11>(UAVRes.pTexture))
                    {
                        if (pTexture->IsInKnownState() && !pTexture->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                        {
                            if (pTexture->CheckAnyState(RESOURCE_STATE_SHADER_RESOURCE | RESOURCE_STATE_INPUT_ATTACHMENT))
                                UnbindTextureFromInput(pTexture, UAVRes.pd3d11Resource);
                            pTexture->SetState(RESOURCE_STATE_UNORDERED_ACCESS);
                        }
                    }
                    else if (auto* pBuffer = ValidatedCast<BufferD3D11Impl>(UAVRes.pBuffer))
                    {
                        if (pBuffer->IsInKnownState() && !pBuffer->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                        {
                            if ((pBuffer->GetState() & RESOURCE_STATE_GENERIC_READ) != 0)
                                UnbindBufferFromInput(pBuffer, UAVRes.pd3d11Resource);
                            pBuffer->SetState(RESOURCE_STATE_UNORDERED_ACCESS);
                        }
                    }
                }
#ifdef DILIGENT_DEVELOPMENT
                else if (VerifyStates)
                {
                    if (const auto* pTexture = ValidatedCast<TextureBaseD3D11>(UAVRes.pTexture))
                    {
                        if (pTexture->IsInKnownState() && !pTexture->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                        {
                            LOG_ERROR_MESSAGE("Texture '", pTexture->GetDesc().Name, "' has not been transitioned to Unordered Access state. Call TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode or explicitly transition the texture to required state.");
                        }
                    }
                    else if (const auto* pBuffer = ValidatedCast<BufferD3D11Impl>(UAVRes.pBuffer))
                    {
                        if (pBuffer->IsInKnownState() && !pBuffer->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                        {
                            LOG_ERROR_MESSAGE("Buffer '", pBuffer->GetDesc().Name, "' has not been transitioned to Unordered Access state. Call TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode or explicitly transition the buffer to required state.");
                        }
                    }
                }
#endif
                if (CommitResources)
                {
                    bool IsNewUAV = CommittedD3D11UAVs[uav] != d3d11UAVs[uav];
                    MinSlot       = IsNewUAV ? std::min(MinSlot, uav) : MinSlot;
                    MaxSlot       = IsNewUAV ? uav : MaxSlot;

                    CommittedD3D11UAVRes[uav] = UAVRes.pd3d11Resource;
                    CommittedD3D11UAVs[uav]   = d3d11UAVs[uav];
                }
            }

            if (CommitResources)
            {
                if (MinSlot != UINT_MAX || ShaderTypeInd == PSInd && m_NumCommittedUAVs[PSInd] != NumUAVs)
                {
                    // Something has changed
                    if (ShaderTypeInd == PSInd)
                    {
                        // Pixel shader UAVs cannot be set independently; they all need to be set at the same time.
                        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/nf-d3d11-id3d11devicecontext-omsetrendertargetsandunorderedaccessviews#remarks
                        auto StartUAVSlot = m_NumBoundRenderTargets;
                        VERIFY(NumUAVs > StartUAVSlot, "Number of UAVs must be greater than the render target count");
                        m_pd3d11DeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
                            D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
                            StartUAVSlot, NumUAVs - StartUAVSlot, CommittedD3D11UAVs + StartUAVSlot, nullptr);
                        // Clear previously bound UAVs, but do not clear lower slots as if
                        // render target count reduces, we will bind these UAVs in CommitRenderTargets()
                        for (Uint32 uav = NumUAVs; uav < m_NumCommittedUAVs[ShaderTypeInd]; ++uav)
                        {
                            CommittedD3D11UAVRes[uav] = nullptr;
                            CommittedD3D11UAVs[uav]   = nullptr;
                        }
                        m_NumCommittedUAVs[ShaderTypeInd] = static_cast<Uint8>(NumUAVs);
                    }
                    else
                    {
                        // This can only be CS
                        auto SetUAVMethod = SetUAVMethods[ShaderTypeInd];
                        (m_pd3d11DeviceContext->*SetUAVMethod)(MinSlot, MaxSlot - MinSlot + 1, CommittedD3D11UAVs + MinSlot, nullptr);
                        m_NumCommittedUAVs[ShaderTypeInd] = std::max(m_NumCommittedUAVs[ShaderTypeInd], static_cast<Uint8>(NumUAVs));
                    }
                }

#ifdef DILIGENT_DEVELOPMENT
                if ((m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE) != 0 && ShaderTypeInd == CSInd)
                {
                    dbgVerifyCommittedUAVs(pShaderD3D11->GetDesc().ShaderType);
                }
#endif
            }
        }
    }

    if (ClearPixelShaderUAVs)
    {
        // If pixel shader stage is inactive or does not use UAVs, unbind all committed UAVs.
        // This is important as UnbindPixelShaderUAV<> function may need to rebind
        // existing UAVs and the UAVs pointed to by CommittedD3D11UAVRes must be alive
        // (we do not keep strong references to d3d11 UAVs)
        auto* CommittedD3D11UAVs          = m_CommittedD3D11UAVs[PSInd];
        auto* CommittedD3D11UAVRes        = m_CommittedD3D11UAVResources[PSInd];
        auto& NumCommittedPixelShaderUAVs = m_NumCommittedUAVs[PSInd];
        for (Uint32 uav = 0; uav < NumCommittedPixelShaderUAVs; ++uav)
        {
            CommittedD3D11UAVRes[uav] = nullptr;
            CommittedD3D11UAVs[uav]   = nullptr;
        }
        m_pd3d11DeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
            D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
            0, 0, nullptr, nullptr);
        NumCommittedPixelShaderUAVs = 0;
    }

    // Commit input resources (CBs, SRVs and Samplers)
    for (Uint32 s = 0; s < NumShaders; ++s)
    {
        const auto ShaderType    = pShaderResBindingD3D11->GetActiveShaderType(s);
        const auto ShaderTypeInd = GetShaderTypeIndex(ShaderType);

#ifdef DILIGENT_DEVELOPMENT
        auto* pShaderD3D11 = pPipelineStateD3D11->GetShader(s);
        VERIFY_EXPR(ShaderType == pShaderD3D11->GetDesc().ShaderType);
#endif

        auto& Cache = pShaderResBindingD3D11->GetResourceCache(s);

        // Transition and commit Constant Buffers
        auto NumCBs = Cache.GetCBCount();
        if (NumCBs)
        {
            ShaderResourceCacheD3D11::CachedCB* CachedCBs;
            ID3D11Buffer**                      d3d11CBs;
            Cache.GetCBArrays(CachedCBs, d3d11CBs);

            auto* CommittedD3D11CBs = m_CommittedD3D11CBs[ShaderTypeInd];
            UINT  MinSlot           = UINT_MAX;
            UINT  MaxSlot           = 0;
            for (Uint32 cb = 0; cb < NumCBs; ++cb)
            {
                VERIFY_EXPR(cb < Cache.GetCBCount());

                if (TransitionResources)
                {
                    auto& CB = CachedCBs[cb];
                    if (auto* pBuff = CB.pBuff.RawPtr<BufferD3D11Impl>())
                    {
                        // WARNING! This code is not thread-safe. It is up to the application to make
                        // sure that multiple threads do not modify the buffer state simultaneously.
                        if (pBuff->IsInKnownState() && !pBuff->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
                        {
                            if (pBuff->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                            {
                                // Even though we have unbound resources from UAV, we only checked shader
                                // stages active in current PSO, so we still may need to unbind the resource
                                // from UAV (for instance, unbind resource from CS UAV when running draw command).
                                UnbindResourceFromUAV(pBuff, d3d11CBs[cb]);
                                pBuff->ClearState(RESOURCE_STATE_UNORDERED_ACCESS);
                            }
                            pBuff->AddState(RESOURCE_STATE_CONSTANT_BUFFER);
                        }
                    }
                }
#ifdef DILIGENT_DEVELOPMENT
                else if (VerifyStates)
                {
                    VERIFY_EXPR(CommitResources);
                    const auto& CB = CachedCBs[cb];
                    if (auto* pBuff = CB.pBuff.RawPtr<const BufferD3D11Impl>())
                    {
                        if (pBuff->IsInKnownState() && !pBuff->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
                        {
                            LOG_ERROR_MESSAGE("Buffer '", pBuff->GetDesc().Name, "' has not been transitioned to Constant Buffer state. Call TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode or explicitly transition the buffer to required state.");
                        }
                    }
                }
#endif

                if (CommitResources)
                {
                    bool IsNewCB          = CommittedD3D11CBs[cb] != d3d11CBs[cb];
                    MinSlot               = IsNewCB ? std::min(MinSlot, cb) : MinSlot;
                    MaxSlot               = IsNewCB ? cb : MaxSlot;
                    CommittedD3D11CBs[cb] = d3d11CBs[cb];
                }
            }

            if (CommitResources)
            {
                if (MinSlot != UINT_MAX)
                {
                    auto SetCBMethod = SetCBMethods[ShaderTypeInd];
                    (m_pd3d11DeviceContext->*SetCBMethod)(MinSlot, MaxSlot - MinSlot + 1, CommittedD3D11CBs + MinSlot);
                    m_NumCommittedCBs[ShaderTypeInd] = std::max(m_NumCommittedCBs[ShaderTypeInd], static_cast<Uint8>(NumCBs));
                }
#ifdef DILIGENT_DEVELOPMENT
                if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
                {
                    dbgVerifyCommittedCBs(pShaderD3D11->GetDesc().ShaderType);
                }
#endif
            }
        }


        // Transition and commit Shader Resource Views
        auto NumSRVs = Cache.GetSRVCount();
        if (NumSRVs)
        {
            ShaderResourceCacheD3D11::CachedResource* CachedSRVResources;
            ID3D11ShaderResourceView**                d3d11SRVs;
            Cache.GetSRVArrays(CachedSRVResources, d3d11SRVs);

            auto* CommittedD3D11SRVs   = m_CommittedD3D11SRVs[ShaderTypeInd];
            auto* CommittedD3D11SRVRes = m_CommittedD3D11SRVResources[ShaderTypeInd];

            UINT MinSlot = UINT_MAX;
            UINT MaxSlot = 0;

            for (Uint32 srv = 0; srv < NumSRVs; ++srv)
            {
                VERIFY_EXPR(srv < Cache.GetSRVCount());
                auto& SRVRes = CachedSRVResources[srv];
                // WARNING! This code is not thread-safe. It is up to the application to make
                // sure that multiple threads do not modify the texture state simultaneously.
                if (TransitionResources)
                {
                    if (auto* pTexture = ValidatedCast<TextureBaseD3D11>(SRVRes.pTexture))
                    {
                        if (pTexture->IsInKnownState() && !pTexture->CheckAnyState(RESOURCE_STATE_SHADER_RESOURCE | RESOURCE_STATE_INPUT_ATTACHMENT))
                        {
                            if (pTexture->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                            {
                                // Even though we have unbound resources from UAV, we only checked shader
                                // stages active in current PSO, so we still may need to unbind the resource
                                // from UAV (for instance, unbind resource from CS UAV when running draw command).
                                UnbindResourceFromUAV(pTexture, SRVRes.pd3d11Resource);
                                pTexture->ClearState(RESOURCE_STATE_UNORDERED_ACCESS);
                            }

                            if (pTexture->CheckState(RESOURCE_STATE_RENDER_TARGET))
                                UnbindTextureFromRenderTarget(pTexture);

                            if (pTexture->CheckState(RESOURCE_STATE_DEPTH_WRITE))
                                UnbindTextureFromDepthStencil(pTexture);

                            pTexture->SetState(RESOURCE_STATE_SHADER_RESOURCE);
                        }
                    }
                    else if (auto* pBuffer = ValidatedCast<BufferD3D11Impl>(SRVRes.pBuffer))
                    {
                        if (pBuffer->IsInKnownState() && !pBuffer->CheckState(RESOURCE_STATE_SHADER_RESOURCE))
                        {
                            if (pBuffer->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                            {
                                UnbindResourceFromUAV(pBuffer, SRVRes.pd3d11Resource);
                                pBuffer->ClearState(RESOURCE_STATE_UNORDERED_ACCESS);
                            }
                            pBuffer->AddState(RESOURCE_STATE_SHADER_RESOURCE);
                        }
                    }
                }
#ifdef DILIGENT_DEVELOPMENT
                else if (VerifyStates)
                {
                    VERIFY_EXPR(CommitResources);
                    if (const auto* pTexture = ValidatedCast<TextureBaseD3D11>(SRVRes.pTexture))
                    {
                        if (pTexture->IsInKnownState() &&
                            !(pTexture->CheckState(RESOURCE_STATE_SHADER_RESOURCE) || m_pActiveRenderPass != nullptr && pTexture->CheckState(RESOURCE_STATE_INPUT_ATTACHMENT)))
                        {
                            LOG_ERROR_MESSAGE("Texture '", pTexture->GetDesc().Name, "' has not been transitioned to Shader Resource state. Call TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode or explicitly transition the texture to required state.");
                        }
                    }
                    else if (const auto* pBuffer = ValidatedCast<BufferD3D11Impl>(SRVRes.pBuffer))
                    {
                        if (pBuffer->IsInKnownState() && !pBuffer->CheckState(RESOURCE_STATE_SHADER_RESOURCE))
                        {
                            LOG_ERROR_MESSAGE("Buffer '", pBuffer->GetDesc().Name, "' has not been transitioned to Shader Resource state. Call TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode or explicitly transition the buffer to required state.");
                        }
                    }
                }
#endif

                if (CommitResources)
                {
                    bool IsNewSRV = CommittedD3D11SRVs[srv] != d3d11SRVs[srv];
                    MinSlot       = IsNewSRV ? std::min(MinSlot, srv) : MinSlot;
                    MaxSlot       = IsNewSRV ? srv : MaxSlot;

                    CommittedD3D11SRVRes[srv] = SRVRes.pd3d11Resource;
                    CommittedD3D11SRVs[srv]   = d3d11SRVs[srv];
                }
            }

            if (CommitResources)
            {
                if (MinSlot != UINT_MAX)
                {
                    auto SetSRVMethod = SetSRVMethods[ShaderTypeInd];
                    (m_pd3d11DeviceContext->*SetSRVMethod)(MinSlot, MaxSlot - MinSlot + 1, CommittedD3D11SRVs + MinSlot);
                    m_NumCommittedSRVs[ShaderTypeInd] = std::max(m_NumCommittedSRVs[ShaderTypeInd], static_cast<Uint8>(NumSRVs));
                }
#ifdef DILIGENT_DEVELOPMENT
                if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
                {
                    dbgVerifyCommittedSRVs(pShaderD3D11->GetDesc().ShaderType);
                }
#endif
            }
        }


        // Commit samplers (no transitions for samplers)
        if (CommitResources)
        {
            auto NumSamplers = Cache.GetSamplerCount();
            if (NumSamplers)
            {
                ShaderResourceCacheD3D11::CachedSampler* CachedSamplers;
                ID3D11SamplerState**                     d3d11Samplers;
                Cache.GetSamplerArrays(CachedSamplers, d3d11Samplers);

                auto* CommittedD3D11Samplers = m_CommittedD3D11Samplers[ShaderTypeInd];
                UINT  MinSlot                = std::numeric_limits<UINT>::max();
                UINT  MaxSlot                = 0;
                for (Uint32 sam = 0; sam < NumSamplers; ++sam)
                {
                    VERIFY_EXPR(sam < Cache.GetSamplerCount());

                    bool IsNewSam = CommittedD3D11Samplers[sam] != d3d11Samplers[sam];
                    MinSlot       = IsNewSam ? std::min(MinSlot, sam) : MinSlot;
                    MaxSlot       = IsNewSam ? sam : MaxSlot;

                    CommittedD3D11Samplers[sam] = d3d11Samplers[sam];
                }

                if (MinSlot != UINT_MAX)
                {
                    auto SetSamplerMethod = SetSamplerMethods[ShaderTypeInd];
                    (m_pd3d11DeviceContext->*SetSamplerMethod)(MinSlot, MaxSlot - MinSlot + 1, CommittedD3D11Samplers + MinSlot);
                    m_NumCommittedSamplers[ShaderTypeInd] = std::max(m_NumCommittedSamplers[ShaderTypeInd], static_cast<Uint8>(NumSamplers));
                }
#ifdef DILIGENT_DEVELOPMENT
                if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
                {
                    dbgVerifyCommittedSamplers(pShaderD3D11->GetDesc().ShaderType);
                }
#endif
            }
        }



#ifdef DILIGENT_DEVELOPMENT
        if (CommitResources && (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES) != 0)
        {
            // Use full resource layout to verify that all required resources are committed
            pShaderD3D11->GetD3D11Resources()->dvpVerifyCommittedResources(
                m_CommittedD3D11CBs[ShaderTypeInd],
                m_CommittedD3D11SRVs[ShaderTypeInd],
                m_CommittedD3D11SRVResources[ShaderTypeInd],
                m_CommittedD3D11Samplers[ShaderTypeInd],
                m_CommittedD3D11UAVs[ShaderTypeInd],
                m_CommittedD3D11UAVResources[ShaderTypeInd],
                Cache);
        }
#endif
    }
}

void DeviceContextD3D11Impl::TransitionShaderResources(IPipelineState* pPipelineState, IShaderResourceBinding* pShaderResourceBinding)
{
    DEV_CHECK_ERR(pPipelineState != nullptr, "Pipeline state must not be null");
    DEV_CHECK_ERR(pShaderResourceBinding != nullptr, "Shader resource binding must not be null");
    if (m_pActiveRenderPass)
    {
        LOG_ERROR_MESSAGE("State transitions are not allowed inside a render pass.");
        return;
    }

    TransitionAndCommitShaderResources<true, false>(pPipelineState, pShaderResourceBinding, false);
}

void DeviceContextD3D11Impl::CommitShaderResources(IShaderResourceBinding* pShaderResourceBinding, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (!DeviceContextBase::CommitShaderResources(pShaderResourceBinding, StateTransitionMode, 0 /*Dummy*/))
        return;

    if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
        TransitionAndCommitShaderResources<true, true>(m_pPipelineState, pShaderResourceBinding, false);
    else
        TransitionAndCommitShaderResources<false, true>(m_pPipelineState, pShaderResourceBinding, StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY);
}

void DeviceContextD3D11Impl::SetStencilRef(Uint32 StencilRef)
{
    if (TDeviceContextBase::SetStencilRef(StencilRef, 0))
    {
        ID3D11DepthStencilState* pd3d11DSS =
            m_pPipelineState ? m_pPipelineState->GetD3D11DepthStencilState() : nullptr;
        m_pd3d11DeviceContext->OMSetDepthStencilState(pd3d11DSS, m_StencilRef);
    }
}


void DeviceContextD3D11Impl::SetBlendFactors(const float* pBlendFactors)
{
    if (TDeviceContextBase::SetBlendFactors(pBlendFactors, 0))
    {
        Uint32            SampleMask = 0xFFFFFFFF;
        ID3D11BlendState* pd3d11BS   = nullptr;
        if (m_pPipelineState && m_pPipelineState->GetDesc().IsAnyGraphicsPipeline())
        {
            SampleMask = m_pPipelineState->GetGraphicsPipelineDesc().SampleMask;
            pd3d11BS   = m_pPipelineState->GetD3D11BlendState();
        }
        m_pd3d11DeviceContext->OMSetBlendState(pd3d11BS, m_BlendFactors, SampleMask);
    }
}

void DeviceContextD3D11Impl::CommitD3D11IndexBuffer(VALUE_TYPE IndexType)
{
    if (!m_pIndexBuffer)
    {
        LOG_ERROR_MESSAGE("Index buffer is not set up for indexed draw command");
        return;
    }

    BufferD3D11Impl* pBuffD3D11 = m_pIndexBuffer.RawPtr<BufferD3D11Impl>();
    if (m_CommittedD3D11IndexBuffer != pBuffD3D11->m_pd3d11Buffer ||
        m_CommittedIBFormat != IndexType ||
        m_CommittedD3D11IndexDataStartOffset != m_IndexDataStartOffset)
    {
        DXGI_FORMAT D3D11IndexFmt = DXGI_FORMAT_UNKNOWN;
        if (IndexType == VT_UINT32)
            D3D11IndexFmt = DXGI_FORMAT_R32_UINT;
        else if (IndexType == VT_UINT16)
            D3D11IndexFmt = DXGI_FORMAT_R16_UINT;
        else
        {
            LOG_ERROR_MESSAGE("Unsupported index format. Only R16_UINT and R32_UINT are allowed.");
            return;
        }

        m_CommittedD3D11IndexBuffer          = pBuffD3D11->m_pd3d11Buffer;
        m_CommittedIBFormat                  = IndexType;
        m_CommittedD3D11IndexDataStartOffset = m_IndexDataStartOffset;
        m_pd3d11DeviceContext->IASetIndexBuffer(pBuffD3D11->m_pd3d11Buffer, D3D11IndexFmt, m_IndexDataStartOffset);
    }

    pBuffD3D11->AddState(RESOURCE_STATE_INDEX_BUFFER);
    m_bCommittedD3D11IBUpToDate = true;
}

void DeviceContextD3D11Impl::CommitD3D11VertexBuffers(PipelineStateD3D11Impl* pPipelineStateD3D11)
{
    VERIFY(m_NumVertexStreams <= MAX_BUFFER_SLOTS, "Too many buffers are being set");
    UINT NumBuffersToSet = std::max(m_NumVertexStreams, m_NumCommittedD3D11VBs);

    bool BindVBs = m_NumVertexStreams != m_NumCommittedD3D11VBs;

    for (UINT Slot = 0; Slot < m_NumVertexStreams; ++Slot)
    {
        auto&         CurrStream     = m_VertexStreams[Slot];
        auto*         pBuffD3D11Impl = CurrStream.pBuffer.RawPtr();
        ID3D11Buffer* pd3d11Buffer   = pBuffD3D11Impl ? pBuffD3D11Impl->m_pd3d11Buffer : nullptr;
        auto          Stride         = pPipelineStateD3D11->GetBufferStride(Slot);
        auto          Offset         = CurrStream.Offset;

        // It is safe to perform raw pointer check because device context keeps
        // all buffers alive.
        if (m_CommittedD3D11VertexBuffers[Slot] != pd3d11Buffer ||
            m_CommittedD3D11VBStrides[Slot] != Stride ||
            m_CommittedD3D11VBOffsets[Slot] != Offset)
        {
            BindVBs = true;

            m_CommittedD3D11VertexBuffers[Slot] = pd3d11Buffer;
            m_CommittedD3D11VBStrides[Slot]     = Stride;
            m_CommittedD3D11VBOffsets[Slot]     = Offset;

            if (pBuffD3D11Impl)
                pBuffD3D11Impl->AddState(RESOURCE_STATE_VERTEX_BUFFER);
        }
    }

    // Unbind all buffers at the end
    for (Uint32 Slot = m_NumVertexStreams; Slot < m_NumCommittedD3D11VBs; ++Slot)
    {
        m_CommittedD3D11VertexBuffers[Slot] = nullptr;
        m_CommittedD3D11VBStrides[Slot]     = 0;
        m_CommittedD3D11VBOffsets[Slot]     = 0;
    }

    m_NumCommittedD3D11VBs = m_NumVertexStreams;

    if (BindVBs)
    {
        m_pd3d11DeviceContext->IASetVertexBuffers(0, NumBuffersToSet, m_CommittedD3D11VertexBuffers, m_CommittedD3D11VBStrides, m_CommittedD3D11VBOffsets);
    }

    m_bCommittedD3D11VBsUpToDate = true;
}

void DeviceContextD3D11Impl::PrepareForDraw(DRAW_FLAGS Flags)
{
#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_RENDER_TARGETS) != 0)
        DvpVerifyRenderTargets();
#endif

    auto* pd3d11InputLayout = m_pPipelineState->GetD3D11InputLayout();
    if (pd3d11InputLayout != nullptr && !m_bCommittedD3D11VBsUpToDate)
    {
        DEV_CHECK_ERR(m_NumVertexStreams >= m_pPipelineState->GetNumBufferSlotsUsed(), "Currently bound pipeline state '", m_pPipelineState->GetDesc().Name, "' expects ", m_pPipelineState->GetNumBufferSlotsUsed(), " input buffer slots, but only ", m_NumVertexStreams, " is bound");
        CommitD3D11VertexBuffers(m_pPipelineState);
    }

#ifdef DILIGENT_DEVELOPMENT
    if ((Flags & DRAW_FLAG_VERIFY_STATES) != 0)
    {
        for (UINT Slot = 0; Slot < m_NumVertexStreams; ++Slot)
        {
            if (auto* pBuffD3D11Impl = m_VertexStreams[Slot].pBuffer.RawPtr())
            {
                if (pBuffD3D11Impl->IsInKnownState() && pBuffD3D11Impl->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                {
                    LOG_ERROR_MESSAGE("Buffer '", pBuffD3D11Impl->GetDesc().Name, "' used as vertex buffer at slot ", Slot, " is in RESOURCE_STATE_UNORDERED_ACCESS state. "
                                                                                                                            "Use appropriate transition mode or explicitly transition the buffer to RESOURCE_STATE_VERTEX_BUFFER state.");
                }
            }
        }
    }

    if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
    {
        // Verify bindings after all resources are set
        dbgVerifyCommittedSRVs();
        dbgVerifyCommittedUAVs(SHADER_TYPE_COMPUTE);
        dbgVerifyCommittedSamplers();
        dbgVerifyCommittedCBs();
        dbgVerifyCommittedVertexBuffers();
        dbgVerifyCommittedIndexBuffer();
        dbgVerifyCommittedShaders();
    }
#endif
}

void DeviceContextD3D11Impl::PrepareForIndexedDraw(DRAW_FLAGS Flags, VALUE_TYPE IndexType)
{
    if (m_CommittedIBFormat != IndexType)
        m_bCommittedD3D11IBUpToDate = false;
    if (!m_bCommittedD3D11IBUpToDate)
    {
        CommitD3D11IndexBuffer(IndexType);
    }
#ifdef DILIGENT_DEVELOPMENT
    if (Flags & DRAW_FLAG_VERIFY_STATES)
    {
        if (m_pIndexBuffer->IsInKnownState() && m_pIndexBuffer->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
        {
            LOG_ERROR_MESSAGE("Buffer '", m_pIndexBuffer->GetDesc().Name,
                              "' used as index buffer is in RESOURCE_STATE_UNORDERED_ACCESS state."
                              " Use appropriate state transition mode or explicitly transition the buffer to RESOURCE_STATE_INDEX_BUFFER state.");
        }
    }
#endif
    // We need to commit index buffer first because PrepareForDraw
    // may verify committed resources.
    PrepareForDraw(Flags);
}

void DeviceContextD3D11Impl::Draw(const DrawAttribs& Attribs)
{
    if (!DvpVerifyDrawArguments(Attribs))
        return;

    PrepareForDraw(Attribs.Flags);

    if (Attribs.NumInstances > 1 || Attribs.FirstInstanceLocation != 0)
        m_pd3d11DeviceContext->DrawInstanced(Attribs.NumVertices, Attribs.NumInstances, Attribs.StartVertexLocation, Attribs.FirstInstanceLocation);
    else
        m_pd3d11DeviceContext->Draw(Attribs.NumVertices, Attribs.StartVertexLocation);
}

void DeviceContextD3D11Impl::DrawIndexed(const DrawIndexedAttribs& Attribs)
{
    if (!DvpVerifyDrawIndexedArguments(Attribs))
        return;

    PrepareForIndexedDraw(Attribs.Flags, Attribs.IndexType);

    if (Attribs.NumInstances > 1 || Attribs.FirstInstanceLocation != 0)
        m_pd3d11DeviceContext->DrawIndexedInstanced(Attribs.NumIndices, Attribs.NumInstances, Attribs.FirstIndexLocation, Attribs.BaseVertex, Attribs.FirstInstanceLocation);
    else
        m_pd3d11DeviceContext->DrawIndexed(Attribs.NumIndices, Attribs.FirstIndexLocation, Attribs.BaseVertex);
}

void DeviceContextD3D11Impl::DrawIndirect(const DrawIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDrawIndirectArguments(Attribs, pAttribsBuffer))
        return;

    PrepareForDraw(Attribs.Flags);

    auto*         pIndirectDrawAttribsD3D11 = ValidatedCast<BufferD3D11Impl>(pAttribsBuffer);
    ID3D11Buffer* pd3d11ArgsBuff            = pIndirectDrawAttribsD3D11->m_pd3d11Buffer;
    m_pd3d11DeviceContext->DrawInstancedIndirect(pd3d11ArgsBuff, Attribs.IndirectDrawArgsOffset);
}


void DeviceContextD3D11Impl::DrawIndexedIndirect(const DrawIndexedIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDrawIndexedIndirectArguments(Attribs, pAttribsBuffer))
        return;

    PrepareForIndexedDraw(Attribs.Flags, Attribs.IndexType);

    auto*         pIndirectDrawAttribsD3D11 = ValidatedCast<BufferD3D11Impl>(pAttribsBuffer);
    ID3D11Buffer* pd3d11ArgsBuff            = pIndirectDrawAttribsD3D11->m_pd3d11Buffer;
    m_pd3d11DeviceContext->DrawIndexedInstancedIndirect(pd3d11ArgsBuff, Attribs.IndirectDrawArgsOffset);
}

void DeviceContextD3D11Impl::DrawMesh(const DrawMeshAttribs& Attribs)
{
    UNSUPPORTED("DrawMesh is not supported in DirectX 11");
}

void DeviceContextD3D11Impl::DrawMeshIndirect(const DrawMeshIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    UNSUPPORTED("DrawMeshIndirect is not supported in DirectX 11");
}

void DeviceContextD3D11Impl::DispatchCompute(const DispatchComputeAttribs& Attribs)
{
    if (!DvpVerifyDispatchArguments(Attribs))
        return;

#ifdef DILIGENT_DEVELOPMENT
    if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
    {
        // Verify bindings
        dbgVerifyCommittedSRVs();
        dbgVerifyCommittedUAVs(SHADER_TYPE_COMPUTE);
        dbgVerifyCommittedSamplers();
        dbgVerifyCommittedCBs();
        dbgVerifyCommittedShaders();
    }
#endif

    m_pd3d11DeviceContext->Dispatch(Attribs.ThreadGroupCountX, Attribs.ThreadGroupCountY, Attribs.ThreadGroupCountZ);
}

void DeviceContextD3D11Impl::DispatchComputeIndirect(const DispatchComputeIndirectAttribs& Attribs, IBuffer* pAttribsBuffer)
{
    if (!DvpVerifyDispatchIndirectArguments(Attribs, pAttribsBuffer))
        return;

#ifdef DILIGENT_DEVELOPMENT
    if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
    {
        // Verify bindings
        dbgVerifyCommittedSRVs();
        dbgVerifyCommittedUAVs(SHADER_TYPE_COMPUTE);
        dbgVerifyCommittedSamplers();
        dbgVerifyCommittedCBs();
        dbgVerifyCommittedShaders();
    }
#endif

    auto* pd3d11Buff = ValidatedCast<BufferD3D11Impl>(pAttribsBuffer)->GetD3D11Buffer();
    m_pd3d11DeviceContext->DispatchIndirect(pd3d11Buff, Attribs.DispatchArgsByteOffset);
}


void DeviceContextD3D11Impl::ClearDepthStencil(ITextureView*                  pView,
                                               CLEAR_DEPTH_STENCIL_FLAGS      ClearFlags,
                                               float                          fDepth,
                                               Uint8                          Stencil,
                                               RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (!TDeviceContextBase::ClearDepthStencil(pView))
        return;

    VERIFY_EXPR(pView != nullptr);

    auto* pViewD3D11 = ValidatedCast<TextureViewD3D11Impl>(pView);
    auto* pd3d11DSV  = static_cast<ID3D11DepthStencilView*>(pViewD3D11->GetD3D11View());

    UINT32 d3d11ClearFlags = 0;
    if (ClearFlags & CLEAR_DEPTH_FLAG) d3d11ClearFlags |= D3D11_CLEAR_DEPTH;
    if (ClearFlags & CLEAR_STENCIL_FLAG) d3d11ClearFlags |= D3D11_CLEAR_STENCIL;
    // The full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied.
    m_pd3d11DeviceContext->ClearDepthStencilView(pd3d11DSV, d3d11ClearFlags, fDepth, Stencil);
}

void DeviceContextD3D11Impl::ClearRenderTarget(ITextureView* pView, const float* RGBA, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    if (!TDeviceContextBase::ClearRenderTarget(pView))
        return;

    VERIFY_EXPR(pView != nullptr);

    auto* pViewD3D11 = ValidatedCast<TextureViewD3D11Impl>(pView);
    auto* pd3d11RTV  = static_cast<ID3D11RenderTargetView*>(pViewD3D11->GetD3D11View());

    static const float Zero[4] = {0.f, 0.f, 0.f, 0.f};
    if (RGBA == nullptr)
        RGBA = Zero;

    // The full extent of the resource view is always cleared.
    // Viewport and scissor settings are not applied.
    m_pd3d11DeviceContext->ClearRenderTargetView(pd3d11RTV, RGBA);
}

void DeviceContextD3D11Impl::Flush()
{
    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Flushing device context inside an active render pass.");
    }

    m_pd3d11DeviceContext->Flush();
}

void DeviceContextD3D11Impl::UpdateBuffer(IBuffer*                       pBuffer,
                                          Uint32                         Offset,
                                          Uint32                         Size,
                                          const void*                    pData,
                                          RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::UpdateBuffer(pBuffer, Offset, Size, pData, StateTransitionMode);

    auto* pBufferD3D11Impl = ValidatedCast<BufferD3D11Impl>(pBuffer);

    D3D11_BOX DstBox;
    DstBox.left   = Offset;
    DstBox.right  = Offset + Size;
    DstBox.top    = 0;
    DstBox.bottom = 1;
    DstBox.front  = 0;
    DstBox.back   = 1;
    auto* pDstBox = (Offset == 0 && Size == pBufferD3D11Impl->GetDesc().uiSizeInBytes) ? nullptr : &DstBox;
    m_pd3d11DeviceContext->UpdateSubresource(pBufferD3D11Impl->m_pd3d11Buffer, 0, pDstBox, pData, 0, 0);
}

void DeviceContextD3D11Impl::CopyBuffer(IBuffer*                       pSrcBuffer,
                                        Uint32                         SrcOffset,
                                        RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                        IBuffer*                       pDstBuffer,
                                        Uint32                         DstOffset,
                                        Uint32                         Size,
                                        RESOURCE_STATE_TRANSITION_MODE DstBufferTransitionMode)
{
    TDeviceContextBase::CopyBuffer(pSrcBuffer, SrcOffset, SrcBufferTransitionMode, pDstBuffer, DstOffset, Size, DstBufferTransitionMode);

    auto* pSrcBufferD3D11Impl = ValidatedCast<BufferD3D11Impl>(pSrcBuffer);
    auto* pDstBufferD3D11Impl = ValidatedCast<BufferD3D11Impl>(pDstBuffer);

    D3D11_BOX SrcBox;
    SrcBox.left   = SrcOffset;
    SrcBox.right  = SrcOffset + Size;
    SrcBox.top    = 0;
    SrcBox.bottom = 1;
    SrcBox.front  = 0;
    SrcBox.back   = 1;
    m_pd3d11DeviceContext->CopySubresourceRegion(pDstBufferD3D11Impl->m_pd3d11Buffer, 0, DstOffset, 0, 0, pSrcBufferD3D11Impl->m_pd3d11Buffer, 0, &SrcBox);
}


void DeviceContextD3D11Impl::MapBuffer(IBuffer* pBuffer, MAP_TYPE MapType, MAP_FLAGS MapFlags, PVoid& pMappedData)
{
    TDeviceContextBase::MapBuffer(pBuffer, MapType, MapFlags, pMappedData);

    auto*     pBufferD3D11  = ValidatedCast<BufferD3D11Impl>(pBuffer);
    D3D11_MAP d3d11MapType  = static_cast<D3D11_MAP>(0);
    UINT      d3d11MapFlags = 0;
    MapParamsToD3D11MapParams(MapType, MapFlags, d3d11MapType, d3d11MapFlags);

    D3D11_MAPPED_SUBRESOURCE MappedBuff;

    HRESULT hr = m_pd3d11DeviceContext->Map(pBufferD3D11->m_pd3d11Buffer, 0, d3d11MapType, d3d11MapFlags, &MappedBuff);
    if ((d3d11MapFlags & D3D11_MAP_FLAG_DO_NOT_WAIT) == 0)
    {
        DEV_CHECK_ERR(SUCCEEDED(hr), "Failed to map buffer '", pBufferD3D11->GetDesc().Name, "'");
    }
    pMappedData = SUCCEEDED(hr) ? MappedBuff.pData : nullptr;
}

void DeviceContextD3D11Impl::UnmapBuffer(IBuffer* pBuffer, MAP_TYPE MapType)
{
    TDeviceContextBase::UnmapBuffer(pBuffer, MapType);
    auto* pBufferD3D11 = ValidatedCast<BufferD3D11Impl>(pBuffer);
    m_pd3d11DeviceContext->Unmap(pBufferD3D11->m_pd3d11Buffer, 0);
}

void DeviceContextD3D11Impl::UpdateTexture(ITexture*                      pTexture,
                                           Uint32                         MipLevel,
                                           Uint32                         Slice,
                                           const Box&                     DstBox,
                                           const TextureSubResData&       SubresData,
                                           RESOURCE_STATE_TRANSITION_MODE SrcBufferTransitionMode,
                                           RESOURCE_STATE_TRANSITION_MODE DstTextureTransitionMode)
{
    TDeviceContextBase::UpdateTexture(pTexture, MipLevel, Slice, DstBox, SubresData, SrcBufferTransitionMode, DstTextureTransitionMode);

    auto*       pTexD3D11 = ValidatedCast<TextureBaseD3D11>(pTexture);
    const auto& Desc      = pTexD3D11->GetDesc();

    // OpenGL backend uses UpdateData() to initialize textures, so we can't check the usage in ValidateUpdateTextureParams()
    DEV_CHECK_ERR(Desc.Usage == USAGE_DEFAULT, "Only USAGE_DEFAULT textures should be updated with UpdateData()");

    if (SubresData.pSrcBuffer != nullptr)
    {
        LOG_ERROR("D3D11 does not support updating texture subresource from a GPU buffer");
        return;
    }

    D3D11_BOX D3D11Box;
    D3D11Box.left          = DstBox.MinX;
    D3D11Box.right         = DstBox.MaxX;
    D3D11Box.top           = DstBox.MinY;
    D3D11Box.bottom        = DstBox.MaxY;
    D3D11Box.front         = DstBox.MinZ;
    D3D11Box.back          = DstBox.MaxZ;
    const auto& FmtAttribs = GetTextureFormatAttribs(Desc.Format);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Align update region by the compressed block size
        VERIFY((D3D11Box.left % FmtAttribs.BlockWidth) == 0, "Update region min X coordinate (", D3D11Box.left, ") must be multiple of a compressed block width (", Uint32{FmtAttribs.BlockWidth}, ")");
        VERIFY((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0, "Compressed block width (", Uint32{FmtAttribs.BlockWidth}, ") is expected to be power of 2");
        D3D11Box.right = (D3D11Box.right + FmtAttribs.BlockWidth - 1) & ~(FmtAttribs.BlockWidth - 1);

        VERIFY((D3D11Box.top % FmtAttribs.BlockHeight) == 0, "Update region min Y coordinate (", D3D11Box.top, ") must be multiple of a compressed block height (", Uint32{FmtAttribs.BlockHeight}, ")");
        VERIFY((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0, "Compressed block height (", Uint32{FmtAttribs.BlockHeight}, ") is expected to be power of 2");
        D3D11Box.bottom = (D3D11Box.bottom + FmtAttribs.BlockHeight - 1) & ~(FmtAttribs.BlockHeight - 1);
    }
    auto SubresIndex = D3D11CalcSubresource(MipLevel, Slice, Desc.MipLevels);
    m_pd3d11DeviceContext->UpdateSubresource(pTexD3D11->GetD3D11Texture(), SubresIndex, &D3D11Box, SubresData.pData, SubresData.Stride, SubresData.DepthStride);
}

void DeviceContextD3D11Impl::CopyTexture(const CopyTextureAttribs& CopyAttribs)
{
    TDeviceContextBase::CopyTexture(CopyAttribs);

    auto* pSrcTexD3D11 = ValidatedCast<TextureBaseD3D11>(CopyAttribs.pSrcTexture);
    auto* pDstTexD3D11 = ValidatedCast<TextureBaseD3D11>(CopyAttribs.pDstTexture);

    D3D11_BOX D3D11SrcBox, *pD3D11SrcBox = nullptr;
    if (const auto* pSrcBox = CopyAttribs.pSrcBox)
    {
        D3D11SrcBox.left   = pSrcBox->MinX;
        D3D11SrcBox.right  = pSrcBox->MaxX;
        D3D11SrcBox.top    = pSrcBox->MinY;
        D3D11SrcBox.bottom = pSrcBox->MaxY;
        D3D11SrcBox.front  = pSrcBox->MinZ;
        D3D11SrcBox.back   = pSrcBox->MaxZ;
        pD3D11SrcBox       = &D3D11SrcBox;
    }
    auto SrcSubRes = D3D11CalcSubresource(CopyAttribs.SrcMipLevel, CopyAttribs.SrcSlice, pSrcTexD3D11->GetDesc().MipLevels);
    auto DstSubRes = D3D11CalcSubresource(CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, pDstTexD3D11->GetDesc().MipLevels);
    m_pd3d11DeviceContext->CopySubresourceRegion(pDstTexD3D11->GetD3D11Texture(), DstSubRes, CopyAttribs.DstX, CopyAttribs.DstY, CopyAttribs.DstZ,
                                                 pSrcTexD3D11->GetD3D11Texture(), SrcSubRes, pD3D11SrcBox);
}


void DeviceContextD3D11Impl::MapTextureSubresource(ITexture*                 pTexture,
                                                   Uint32                    MipLevel,
                                                   Uint32                    ArraySlice,
                                                   MAP_TYPE                  MapType,
                                                   MAP_FLAGS                 MapFlags,
                                                   const Box*                pMapRegion,
                                                   MappedTextureSubresource& MappedData)
{
    TDeviceContextBase::MapTextureSubresource(pTexture, MipLevel, ArraySlice, MapType, MapFlags, pMapRegion, MappedData);

    auto*       pTexD3D11     = ValidatedCast<TextureBaseD3D11>(pTexture);
    const auto& TexDesc       = pTexD3D11->GetDesc();
    D3D11_MAP   d3d11MapType  = static_cast<D3D11_MAP>(0);
    UINT        d3d11MapFlags = 0;
    MapParamsToD3D11MapParams(MapType, MapFlags, d3d11MapType, d3d11MapFlags);

    auto                     Subresource = D3D11CalcSubresource(MipLevel, ArraySlice, TexDesc.MipLevels);
    D3D11_MAPPED_SUBRESOURCE MappedTex;
    auto                     hr = m_pd3d11DeviceContext->Map(pTexD3D11->GetD3D11Texture(), Subresource, d3d11MapType, d3d11MapFlags, &MappedTex);
    if (FAILED(hr))
    {
        VERIFY_EXPR(hr == DXGI_ERROR_WAS_STILL_DRAWING);
        MappedData = MappedTextureSubresource();
    }
    else
    {
        MappedData.pData       = MappedTex.pData;
        MappedData.Stride      = MappedTex.RowPitch;
        MappedData.DepthStride = MappedTex.DepthPitch;
    }
}

void DeviceContextD3D11Impl::UnmapTextureSubresource(ITexture* pTexture, Uint32 MipLevel, Uint32 ArraySlice)
{
    TDeviceContextBase::UnmapTextureSubresource(pTexture, MipLevel, ArraySlice);

    auto*       pTexD3D11   = ValidatedCast<TextureBaseD3D11>(pTexture);
    const auto& TexDesc     = pTexD3D11->GetDesc();
    auto        Subresource = D3D11CalcSubresource(MipLevel, ArraySlice, TexDesc.MipLevels);
    m_pd3d11DeviceContext->Unmap(pTexD3D11->GetD3D11Texture(), Subresource);
}

void DeviceContextD3D11Impl::GenerateMips(ITextureView* pTextureView)
{
    TDeviceContextBase::GenerateMips(pTextureView);
    auto& TexViewD3D11 = *ValidatedCast<TextureViewD3D11Impl>(pTextureView);
    auto* pd3d11SRV    = static_cast<ID3D11ShaderResourceView*>(TexViewD3D11.GetD3D11View());
    m_pd3d11DeviceContext->GenerateMips(pd3d11SRV);
}

void DeviceContextD3D11Impl::FinishFrame()
{
    if (m_ActiveDisjointQuery)
    {
        m_pd3d11DeviceContext->End(m_ActiveDisjointQuery->pd3d11Query);
        m_ActiveDisjointQuery->IsEnded = true;
        m_ActiveDisjointQuery.reset();
    }
}

void DeviceContextD3D11Impl::SetVertexBuffers(Uint32                         StartSlot,
                                              Uint32                         NumBuffersSet,
                                              IBuffer**                      ppBuffers,
                                              Uint32*                        pOffsets,
                                              RESOURCE_STATE_TRANSITION_MODE StateTransitionMode,
                                              SET_VERTEX_BUFFERS_FLAGS       Flags)
{
    TDeviceContextBase::SetVertexBuffers(StartSlot, NumBuffersSet, ppBuffers, pOffsets, StateTransitionMode, Flags);
    for (Uint32 Slot = 0; Slot < m_NumVertexStreams; ++Slot)
    {
        auto& CurrStream = m_VertexStreams[Slot];
        if (auto* pBuffD3D11Impl = CurrStream.pBuffer.RawPtr())
        {
            if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
            {
                if (pBuffD3D11Impl->IsInKnownState() && pBuffD3D11Impl->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                {
                    UnbindResourceFromUAV(pBuffD3D11Impl, pBuffD3D11Impl->m_pd3d11Buffer);
                    pBuffD3D11Impl->ClearState(RESOURCE_STATE_UNORDERED_ACCESS);
                }
            }
#ifdef DILIGENT_DEVELOPMENT
            else if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
            {
                if (pBuffD3D11Impl->IsInKnownState() && pBuffD3D11Impl->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
                {
                    LOG_ERROR_MESSAGE("Buffer '", pBuffD3D11Impl->GetDesc().Name, "' used as vertex buffer at slot ", Slot, " is in RESOURCE_STATE_UNORDERED_ACCESS state. "
                                                                                                                            "Use RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode or explicitly transition the buffer to RESOURCE_STATE_VERTEX_BUFFER state.");
                }
            }
#endif
        }
    }

    m_bCommittedD3D11VBsUpToDate = false;
}

void DeviceContextD3D11Impl::SetIndexBuffer(IBuffer* pIndexBuffer, Uint32 ByteOffset, RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
    TDeviceContextBase::SetIndexBuffer(pIndexBuffer, ByteOffset, StateTransitionMode);

    if (m_pIndexBuffer)
    {
        if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
        {
            if (m_pIndexBuffer->IsInKnownState() && m_pIndexBuffer->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
            {
                UnbindResourceFromUAV(m_pIndexBuffer, m_pIndexBuffer->m_pd3d11Buffer);
                m_pIndexBuffer->ClearState(RESOURCE_STATE_UNORDERED_ACCESS);
            }
        }
#ifdef DILIGENT_DEVELOPMENT
        else if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
        {
            if (m_pIndexBuffer->IsInKnownState() && m_pIndexBuffer->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
            {
                LOG_ERROR_MESSAGE("Buffer '", m_pIndexBuffer->GetDesc().Name, "' used as index buffer is in RESOURCE_STATE_UNORDERED_ACCESS state."
                                                                              " Use RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode or explicitly transition the buffer to RESOURCE_STATE_INDEX_BUFFER state.");
            }
        }
#endif
    }

    m_bCommittedD3D11IBUpToDate = false;
}

void DeviceContextD3D11Impl::SetViewports(Uint32 NumViewports, const Viewport* pViewports, Uint32 RTWidth, Uint32 RTHeight)
{
    static_assert(MAX_VIEWPORTS >= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "MaxViewports constant must be greater than D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE");
    TDeviceContextBase::SetViewports(NumViewports, pViewports, RTWidth, RTHeight);

    D3D11_VIEWPORT d3d11Viewports[MAX_VIEWPORTS];
    VERIFY(NumViewports == m_NumViewports, "Unexpected number of viewports");
    for (Uint32 vp = 0; vp < m_NumViewports; ++vp)
    {
        d3d11Viewports[vp].TopLeftX = m_Viewports[vp].TopLeftX;
        d3d11Viewports[vp].TopLeftY = m_Viewports[vp].TopLeftY;
        d3d11Viewports[vp].Width    = m_Viewports[vp].Width;
        d3d11Viewports[vp].Height   = m_Viewports[vp].Height;
        d3d11Viewports[vp].MinDepth = m_Viewports[vp].MinDepth;
        d3d11Viewports[vp].MaxDepth = m_Viewports[vp].MaxDepth;
    }
    // All viewports must be set atomically as one operation.
    // Any viewports not defined by the call are disabled.
    m_pd3d11DeviceContext->RSSetViewports(NumViewports, d3d11Viewports);
}

void DeviceContextD3D11Impl::SetScissorRects(Uint32 NumRects, const Rect* pRects, Uint32 RTWidth, Uint32 RTHeight)
{
    static_assert(MAX_VIEWPORTS >= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "MaxViewports constant must be greater than D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE");
    TDeviceContextBase::SetScissorRects(NumRects, pRects, RTWidth, RTHeight);

    D3D11_RECT d3d11ScissorRects[MAX_VIEWPORTS];
    VERIFY(NumRects == m_NumScissorRects, "Unexpected number of scissor rects");
    for (Uint32 sr = 0; sr < NumRects; ++sr)
    {
        d3d11ScissorRects[sr].left   = m_ScissorRects[sr].left;
        d3d11ScissorRects[sr].top    = m_ScissorRects[sr].top;
        d3d11ScissorRects[sr].right  = m_ScissorRects[sr].right;
        d3d11ScissorRects[sr].bottom = m_ScissorRects[sr].bottom;
    }

    // All scissor rects must be set atomically as one operation.
    // Any scissor rects not defined by the call are disabled.
    m_pd3d11DeviceContext->RSSetScissorRects(NumRects, d3d11ScissorRects);
}

void DeviceContextD3D11Impl::CommitRenderTargets()
{
    const Uint32 MaxD3D11RTs      = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    Uint32       NumRenderTargets = m_NumBoundRenderTargets;
    VERIFY(NumRenderTargets <= MaxD3D11RTs, "D3D11 only allows 8 simultaneous render targets");
    NumRenderTargets = std::min(MaxD3D11RTs, NumRenderTargets);

    // Do not waste time setting RTVs to null
    ID3D11RenderTargetView* pd3d11RTs[MaxD3D11RTs];
    ID3D11DepthStencilView* pd3d11DSV = nullptr;

    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        auto* pViewD3D11 = m_pBoundRenderTargets[rt].RawPtr();
        pd3d11RTs[rt]    = pViewD3D11 != nullptr ? static_cast<ID3D11RenderTargetView*>(pViewD3D11->GetD3D11View()) : nullptr;
    }

    if (m_pBoundDepthStencil != nullptr)
    {
        pd3d11DSV = static_cast<ID3D11DepthStencilView*>(m_pBoundDepthStencil->GetD3D11View());
    }

    auto& NumCommittedPixelShaderUAVs = m_NumCommittedUAVs[PSInd];
    if (NumCommittedPixelShaderUAVs > 0)
    {
        m_pd3d11DeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(NumRenderTargets, NumRenderTargets > 0 ? pd3d11RTs : nullptr, pd3d11DSV,
                                                                         0, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, nullptr, nullptr);

        auto CommittedD3D11UAVs   = m_CommittedD3D11UAVs[PSInd];
        auto CommittedD3D11UAVRes = m_CommittedD3D11UAVResources[PSInd];
        for (Uint32 slot = 0; slot < NumRenderTargets; ++slot)
        {
            CommittedD3D11UAVs[slot]   = nullptr;
            CommittedD3D11UAVRes[slot] = nullptr;
        }
        if (NumRenderTargets >= NumCommittedPixelShaderUAVs)
            NumCommittedPixelShaderUAVs = 0;
    }
    else
    {
        m_pd3d11DeviceContext->OMSetRenderTargets(NumRenderTargets, NumRenderTargets > 0 ? pd3d11RTs : nullptr, pd3d11DSV);
    }
}


void UnbindView(ID3D11DeviceContext* pContext, TSetShaderResourcesType SetSRVMethod, UINT Slot)
{
    ID3D11ShaderResourceView* ppNullView[] = {nullptr};
    (pContext->*SetSRVMethod)(Slot, 1, ppNullView);
}

void UnbindView(ID3D11DeviceContext* pContext, TSetUnorderedAccessViewsType SetUAVMethod, UINT Slot)
{
    ID3D11UnorderedAccessView* ppNullView[] = {nullptr};
    (pContext->*SetUAVMethod)(Slot, 1, ppNullView, nullptr);
}

template <typename TD3D11ResourceViewType, typename TSetD3D11View>
bool UnbindPixelShaderUAV(ID3D11DeviceContext*    pDeviceCtx,
                          TD3D11ResourceViewType* CommittedD3D11Resources[],
                          Uint32                  NumCommittedSlots,
                          Uint32                  NumCommittedRenderTargets,
                          TSetD3D11View           SetD3D11ViewMethod)
{
    // For other resource view types do nothing
    return false;
}

template <>
bool UnbindPixelShaderUAV<ID3D11UnorderedAccessView, TSetUnorderedAccessViewsType>(
    ID3D11DeviceContext*         pDeviceCtx,
    ID3D11UnorderedAccessView*   CommittedD3D11UAVs[],
    Uint32                       NumCommittedUAVs,
    Uint32                       NumCommittedRenderTargets,
    TSetUnorderedAccessViewsType SetD3D11UAVMethod)
{
    if (SetD3D11UAVMethod == reinterpret_cast<TSetUnorderedAccessViewsType>(&ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews))
    {
        // Pixel shader UAVs are bound in a special way simulatneously with the render targets
        auto UAVStartSlot = NumCommittedRenderTargets;
        // UAVs cannot be set independently; they all need to be set at the same time.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/nf-d3d11-id3d11devicecontext-omsetrendertargetsandunorderedaccessviews#remarks

        // There is potential problem here: since device context does not keep strong references to
        // UAVs, there is no guarantee the objects are alive
        pDeviceCtx->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
                                                              UAVStartSlot, NumCommittedUAVs - UAVStartSlot,
                                                              CommittedD3D11UAVs + UAVStartSlot, nullptr);
        return true;
    }

    return false;
}



/// \tparam TD3D11ResourceViewType - Type of the D3D11 resource view (ID3D11ShaderResourceView or ID3D11UnorderedAccessView)
/// \tparam TSetD3D11View - Type of the D3D11 device context method used to set the D3D11 view
/// \param CommittedResourcesArr - Pointer to the array of strong references to currently bound
///                            shader resources, for each shader stage
/// \param CommittedD3D11ResourcesArr - Pointer to the array of currently bound D3D11
///                                 shader resources, for each shader stage
/// \param pd3d11ResToUndind   - D3D11 resource to unbind
/// \param SetD3D11ViewMethods - Array of pointers to device context methods used to set the view,
///                              for every shader stage
template <typename TD3D11ResourceViewType,
          typename TSetD3D11View,
          size_t NumSlots>
void DeviceContextD3D11Impl::UnbindResourceView(TD3D11ResourceViewType CommittedD3D11ViewsArr[][NumSlots],
                                                ID3D11Resource*        CommittedD3D11ResourcesArr[][NumSlots],
                                                Uint8                  NumCommittedResourcesArr[],
                                                ID3D11Resource*        pd3d11ResToUndind,
                                                TSetD3D11View          SetD3D11ViewMethods[])
{
    for (Int32 ShaderTypeInd = 0; ShaderTypeInd < NumShaderTypes; ++ShaderTypeInd)
    {
        auto* CommittedD3D11Views     = CommittedD3D11ViewsArr[ShaderTypeInd];
        auto* CommittedD3D11Resources = CommittedD3D11ResourcesArr[ShaderTypeInd];
        auto& NumCommittedSlots       = NumCommittedResourcesArr[ShaderTypeInd];

        for (Uint32 Slot = 0; Slot < NumCommittedSlots; ++Slot)
        {
            if (CommittedD3D11Resources[Slot] == pd3d11ResToUndind)
            {
                CommittedD3D11Resources[Slot] = nullptr;
                CommittedD3D11Views[Slot]     = nullptr;

                auto SetViewMethod = SetD3D11ViewMethods[ShaderTypeInd];
                VERIFY(SetViewMethod != nullptr, "No appropriate ID3D11DeviceContext method");

                // Pixel shader UAVs require special handling
                if (!UnbindPixelShaderUAV(m_pd3d11DeviceContext, CommittedD3D11Views, NumCommittedSlots, m_NumBoundRenderTargets, SetViewMethod))
                {
                    UnbindView(m_pd3d11DeviceContext, SetViewMethod, Slot);
                }
            }
        }

        // Pop null resources from the end of arrays
        while (NumCommittedSlots > 0 && CommittedD3D11Resources[NumCommittedSlots - 1] == nullptr)
        {
            VERIFY(CommittedD3D11Views[NumSlots - 1] == nullptr, "Unexpected non-null resource view");
            --NumCommittedSlots;
        }
    }
}

void DeviceContextD3D11Impl::UnbindTextureFromInput(TextureBaseD3D11* pTexture, ID3D11Resource* pd3d11Resource)
{
    VERIFY(pTexture, "Null texture provided");
    if (!pTexture) return;

    UnbindResourceView(m_CommittedD3D11SRVs, m_CommittedD3D11SRVResources, m_NumCommittedSRVs, pd3d11Resource, SetSRVMethods);
    pTexture->ClearState(RESOURCE_STATE_SHADER_RESOURCE | RESOURCE_STATE_INPUT_ATTACHMENT);
}

void DeviceContextD3D11Impl::UnbindBufferFromInput(BufferD3D11Impl* pBuffer, ID3D11Resource* pd3d11Buffer)
{
    VERIFY(pBuffer, "Null buffer provided");
    if (!pBuffer || !pBuffer->IsInKnownState()) return;

    if (pBuffer->CheckState(RESOURCE_STATE_SHADER_RESOURCE))
    {
        UnbindResourceView(m_CommittedD3D11SRVs, m_CommittedD3D11SRVResources, m_NumCommittedSRVs, pd3d11Buffer, SetSRVMethods);
        pBuffer->ClearState(RESOURCE_STATE_SHADER_RESOURCE);
    }

    if (pBuffer->CheckState(RESOURCE_STATE_INDEX_BUFFER))
    {
        auto pd3d11IndBuffer = ValidatedCast<BufferD3D11Impl>(pBuffer)->GetD3D11Buffer();
        if (pd3d11IndBuffer == m_CommittedD3D11IndexBuffer)
        {
            // Only unbind D3D11 buffer from the context!
            // m_pIndexBuffer.Release();
            m_CommittedD3D11IndexBuffer.Release();
            m_CommittedIBFormat                  = VT_UNDEFINED;
            m_CommittedD3D11IndexDataStartOffset = 0;
            m_bCommittedD3D11IBUpToDate          = false;
            m_pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, m_CommittedD3D11IndexDataStartOffset);
        }
#ifdef DILIGENT_DEVELOPMENT
        if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
        {
            dbgVerifyCommittedIndexBuffer();
        }
#endif
        pBuffer->ClearState(RESOURCE_STATE_INDEX_BUFFER);
    }

    if (pBuffer->CheckState(RESOURCE_STATE_VERTEX_BUFFER))
    {
        auto pd3d11VB = ValidatedCast<BufferD3D11Impl>(pBuffer)->GetD3D11Buffer();
        for (Uint32 Slot = 0; Slot < m_NumCommittedD3D11VBs; ++Slot)
        {
            auto& CommittedD3D11VB = m_CommittedD3D11VertexBuffers[Slot];
            if (CommittedD3D11VB == pd3d11VB)
            {
                // Unbind only D3D11 buffer
                //*VertStream = VertexStreamInfo();
                ID3D11Buffer* ppNullBuffer[]        = {nullptr};
                const UINT    Zero[]                = {0};
                m_CommittedD3D11VertexBuffers[Slot] = nullptr;
                m_CommittedD3D11VBStrides[Slot]     = 0;
                m_CommittedD3D11VBOffsets[Slot]     = 0;
                m_bCommittedD3D11VBsUpToDate        = false;
                m_pd3d11DeviceContext->IASetVertexBuffers(Slot, _countof(ppNullBuffer), ppNullBuffer, Zero, Zero);
            }
        }
#ifdef DILIGENT_DEVELOPMENT
        if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
        {
            dbgVerifyCommittedVertexBuffers();
        }
#endif
        pBuffer->ClearState(RESOURCE_STATE_VERTEX_BUFFER);
    }

    if (pBuffer->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
    {
        for (Int32 ShaderTypeInd = 0; ShaderTypeInd < NumShaderTypes; ++ShaderTypeInd)
        {
            auto* CommittedD3D11CBs = m_CommittedD3D11CBs[ShaderTypeInd];
            auto  NumSlots          = m_NumCommittedCBs[ShaderTypeInd];
            for (Uint32 Slot = 0; Slot < NumSlots; ++Slot)
            {
                if (CommittedD3D11CBs[Slot] == pd3d11Buffer)
                {
                    CommittedD3D11CBs[Slot]      = nullptr;
                    auto          SetCBMethod    = SetCBMethods[ShaderTypeInd];
                    ID3D11Buffer* ppNullBuffer[] = {nullptr};
                    (m_pd3d11DeviceContext->*SetCBMethod)(Slot, 1, ppNullBuffer);
                }
            }
        }
#ifdef DILIGENT_DEVELOPMENT
        if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
        {
            dbgVerifyCommittedCBs();
        }
#endif
        pBuffer->ClearState(RESOURCE_STATE_CONSTANT_BUFFER);
    }
}

void DeviceContextD3D11Impl::UnbindResourceFromUAV(IDeviceObject* pResource, ID3D11Resource* pd3d11Resource)
{
    VERIFY(pResource, "Null resource provided");
    if (!pResource) return;

    UnbindResourceView(m_CommittedD3D11UAVs, m_CommittedD3D11UAVResources, m_NumCommittedUAVs, pd3d11Resource, SetUAVMethods);
}

void DeviceContextD3D11Impl::UnbindTextureFromRenderTarget(TextureBaseD3D11* pTexture)
{
    VERIFY(pTexture, "Null resource provided");
    if (!pTexture) return;

    bool bCommitRenderTargets = false;
    for (Uint32 rt = 0; rt < m_NumBoundRenderTargets; ++rt)
    {
        if (auto* pTexView = m_pBoundRenderTargets[rt].RawPtr())
        {
            if (pTexView->GetTexture() == pTexture)
            {
                m_pBoundRenderTargets[rt].Release();
                bCommitRenderTargets = true;
            }
        }
    }

    if (bCommitRenderTargets)
    {
        while (m_NumBoundRenderTargets > 0 && !m_pBoundRenderTargets[m_NumBoundRenderTargets - 1])
            --m_NumBoundRenderTargets;

        CommitRenderTargets();
    }

    pTexture->ClearState(RESOURCE_STATE_RENDER_TARGET);
}

void DeviceContextD3D11Impl::UnbindTextureFromDepthStencil(TextureBaseD3D11* pTexD3D11)
{
    VERIFY(pTexD3D11, "Null resource provided");
    if (!pTexD3D11) return;

    if (m_pBoundDepthStencil && m_pBoundDepthStencil->GetTexture() == pTexD3D11)
    {
        m_pBoundDepthStencil.Release();
        CommitRenderTargets();
    }
    pTexD3D11->ClearState(RESOURCE_STATE_DEPTH_WRITE);
}

void DeviceContextD3D11Impl::ResetRenderTargets()
{
    TDeviceContextBase::ResetRenderTargets();
    m_pd3d11DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void DeviceContextD3D11Impl::SetRenderTargets(Uint32                         NumRenderTargets,
                                              ITextureView*                  ppRenderTargets[],
                                              ITextureView*                  pDepthStencil,
                                              RESOURCE_STATE_TRANSITION_MODE StateTransitionMode)
{
#ifdef DILIGENT_DEVELOPMENT
    if (m_pActiveRenderPass != nullptr)
    {
        LOG_ERROR_MESSAGE("Calling SetRenderTargets inside active render pass is invalid. End the render pass first");
        return;
    }
#endif

    if (TDeviceContextBase::SetRenderTargets(NumRenderTargets, ppRenderTargets, pDepthStencil))
    {
        for (Uint32 RT = 0; RT < NumRenderTargets; ++RT)
        {
            if (ppRenderTargets[RT])
            {
                auto* pTex = ValidatedCast<TextureBaseD3D11>(ppRenderTargets[RT]->GetTexture());
                if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
                {
                    UnbindTextureFromInput(pTex, pTex->GetD3D11Texture());
                    if (pTex->IsInKnownState())
                        pTex->SetState(RESOURCE_STATE_RENDER_TARGET);
                }
#ifdef DILIGENT_DEVELOPMENT
                else if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
                {
                    DvpVerifyTextureState(*pTex, RESOURCE_STATE_RENDER_TARGET, "Setting render targets (DeviceContextD3D11Impl::SetRenderTargets)");
                }
#endif
            }
        }

        if (pDepthStencil)
        {
            auto* pTex = ValidatedCast<TextureBaseD3D11>(pDepthStencil->GetTexture());
            if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
            {
                UnbindTextureFromInput(pTex, pTex->GetD3D11Texture());
                if (pTex->IsInKnownState())
                    pTex->SetState(RESOURCE_STATE_DEPTH_WRITE);
            }
#ifdef DILIGENT_DEVELOPMENT
            else if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
            {
                DvpVerifyTextureState(*pTex, RESOURCE_STATE_DEPTH_WRITE, "Setting depth-stencil buffer (DeviceContextD3D11Impl::SetRenderTargets)");
            }
#endif
        }

        CommitRenderTargets();

        // Set the viewport to match the render target size
        SetViewports(1, nullptr, 0, 0);
    }
}

void DeviceContextD3D11Impl::BeginSubpass()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& Subpass = RPDesc.pSubpasses[m_SubpassIndex];
    const auto& FBDesc  = m_pBoundFramebuffer->GetDesc();

    // Unbind these attachments that will be used for output by the subpass.
    // There is no need to unbind textures from output as the new subpass atachments
    // will be committed as render target/depth stencil anyway, so these that can be used for
    // input will be unbound.

    auto UnbindAttachmentFromInput = [&](const AttachmentReference& AttachmentRef) //
    {
        if (AttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
        {
            if (auto* pTexView = FBDesc.ppAttachments[AttachmentRef.AttachmentIndex])
            {
                auto* pTexD3D11 = ValidatedCast<TextureBaseD3D11>(pTexView->GetTexture());
                UnbindResourceView(m_CommittedD3D11SRVs, m_CommittedD3D11SRVResources, m_NumCommittedSRVs, pTexD3D11->GetD3D11Texture(), SetSRVMethods);
            }
        }
    };

    for (Uint32 rt = 0; rt < Subpass.RenderTargetAttachmentCount; ++rt)
    {
        UnbindAttachmentFromInput(Subpass.pRenderTargetAttachments[rt]);
        if (Subpass.pResolveAttachments != nullptr)
        {
            UnbindAttachmentFromInput(Subpass.pResolveAttachments[rt]);
        }
    }

    if (Subpass.pDepthStencilAttachment != nullptr)
    {
        UnbindAttachmentFromInput(*Subpass.pDepthStencilAttachment);
    }

    CommitRenderTargets();

    for (Uint32 rt = 0; rt < Subpass.RenderTargetAttachmentCount; ++rt)
    {
        const auto& AttachmentRef   = Subpass.pRenderTargetAttachments[rt];
        const auto  RTAttachmentIdx = AttachmentRef.AttachmentIndex;
        if (RTAttachmentIdx != ATTACHMENT_UNUSED)
        {
            const auto AttachmentFirstUse = m_pActiveRenderPass->GetAttachmentFirstLastUse(RTAttachmentIdx).first;
            if (AttachmentFirstUse == m_SubpassIndex && RPDesc.pAttachments[RTAttachmentIdx].LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
            {
                if (auto* pTexView = FBDesc.ppAttachments[RTAttachmentIdx])
                {
                    auto* const pViewD3D11 = ValidatedCast<TextureViewD3D11Impl>(pTexView);
                    auto* const pd3d11RTV  = static_cast<ID3D11RenderTargetView*>(pViewD3D11->GetD3D11View());
                    const auto& ClearValue = m_AttachmentClearValues[RTAttachmentIdx];
                    m_pd3d11DeviceContext->ClearRenderTargetView(pd3d11RTV, ClearValue.Color);
                }
            }
        }
    }

    if (Subpass.pDepthStencilAttachment)
    {
        auto DSAttachmentIdx = Subpass.pDepthStencilAttachment->AttachmentIndex;
        if (DSAttachmentIdx != ATTACHMENT_UNUSED)
        {
            const auto AttachmentFirstUse = m_pActiveRenderPass->GetAttachmentFirstLastUse(DSAttachmentIdx).first;
            if (AttachmentFirstUse == m_SubpassIndex && RPDesc.pAttachments[DSAttachmentIdx].LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
            {
                if (auto* pTexView = FBDesc.ppAttachments[DSAttachmentIdx])
                {
                    auto* const pViewD3D11 = ValidatedCast<TextureViewD3D11Impl>(pTexView);
                    auto* const pd3d11DSV  = static_cast<ID3D11DepthStencilView*>(pViewD3D11->GetD3D11View());
                    const auto& ClearValue = m_AttachmentClearValues[DSAttachmentIdx];
                    m_pd3d11DeviceContext->ClearDepthStencilView(pd3d11DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, ClearValue.DepthStencil.Depth, ClearValue.DepthStencil.Stencil);
                }
            }
        }
    }
}

void DeviceContextD3D11Impl::EndSubpass()
{
    VERIFY_EXPR(m_pActiveRenderPass);
    const auto& RPDesc = m_pActiveRenderPass->GetDesc();
    VERIFY_EXPR(m_SubpassIndex < RPDesc.SubpassCount);
    const auto& Subpass = RPDesc.pSubpasses[m_SubpassIndex];
    const auto& FBDesc  = m_pBoundFramebuffer->GetDesc();

    if (Subpass.pResolveAttachments != nullptr)
    {
        for (Uint32 rt = 0; rt < Subpass.RenderTargetAttachmentCount; ++rt)
        {
            const auto& RslvAttachmentRef = Subpass.pResolveAttachments[rt];
            if (RslvAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
            {
                const auto& RTAttachmentRef = Subpass.pRenderTargetAttachments[rt];
                VERIFY_EXPR(RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED);
                auto* pSrcView     = FBDesc.ppAttachments[RTAttachmentRef.AttachmentIndex];
                auto* pDstView     = FBDesc.ppAttachments[RslvAttachmentRef.AttachmentIndex];
                auto* pSrcTexD3D11 = ValidatedCast<TextureBaseD3D11>(pSrcView->GetTexture());
                auto* pDstTexD3D11 = ValidatedCast<TextureBaseD3D11>(pDstView->GetTexture());

                const auto& SrcViewDesc = pSrcView->GetDesc();
                const auto& DstViewDesc = pDstView->GetDesc();
                const auto& SrcTexDesc  = pSrcTexD3D11->GetDesc();
                const auto& DstTexDesc  = pDstTexD3D11->GetDesc();

                auto DXGIFmt        = TexFormatToDXGI_Format(RPDesc.pAttachments[RTAttachmentRef.AttachmentIndex].Format);
                auto SrcSubresIndex = D3D11CalcSubresource(SrcViewDesc.MostDetailedMip, SrcViewDesc.FirstArraySlice, SrcTexDesc.MipLevels);
                auto DstSubresIndex = D3D11CalcSubresource(DstViewDesc.MostDetailedMip, DstViewDesc.FirstArraySlice, DstTexDesc.MipLevels);
                m_pd3d11DeviceContext->ResolveSubresource(pDstTexD3D11->GetD3D11Texture(), DstSubresIndex, pSrcTexD3D11->GetD3D11Texture(), SrcSubresIndex, DXGIFmt);
            }
        }
    }
}

void DeviceContextD3D11Impl::BeginRenderPass(const BeginRenderPassAttribs& Attribs)
{
    TDeviceContextBase::BeginRenderPass(Attribs);
    // BeginRenderPass() transitions resources to required states

    m_AttachmentClearValues.resize(Attribs.ClearValueCount);
    for (Uint32 i = 0; i < Attribs.ClearValueCount; ++i)
        m_AttachmentClearValues[i] = Attribs.pClearValues[i];

    BeginSubpass();

    // Set the viewport to match the framebuffer size
    SetViewports(1, nullptr, 0, 0);
}

void DeviceContextD3D11Impl::NextSubpass()
{
    EndSubpass();

    TDeviceContextBase::NextSubpass();

    BeginSubpass();
}

void DeviceContextD3D11Impl::EndRenderPass()
{
    EndSubpass();
    TDeviceContextBase::EndRenderPass();
    m_AttachmentClearValues.clear();
}


template <typename TD3D11ResourceType, typename TSetD3D11ResMethodType>
void SetD3D11ResourcesHelper(ID3D11DeviceContext*   pDeviceCtx,
                             TSetD3D11ResMethodType SetD3D11ResMethod,
                             UINT                   StartSlot,
                             UINT                   NumSlots,
                             TD3D11ResourceType**   ppResources)
{
    (pDeviceCtx->*SetD3D11ResMethod)(StartSlot, NumSlots, ppResources);
}

template <>
void SetD3D11ResourcesHelper(ID3D11DeviceContext*         pDeviceCtx,
                             TSetUnorderedAccessViewsType SetD3D11UAVMethod,
                             UINT                         StartSlot,
                             UINT                         NumSlots,
                             ID3D11UnorderedAccessView**  ppUAVs)
{
    (pDeviceCtx->*SetD3D11UAVMethod)(StartSlot, NumSlots, ppUAVs, nullptr);
}

template <typename TD3D11ResourceType, typename TSetD3D11ResMethodType>
void ReleaseCommittedShaderResourcesHelper(TD3D11ResourceType     CommittedD3D11Res[],
                                           Uint8                  NumCommittedResources,
                                           TSetD3D11ResMethodType SetD3D11ResMethod,
                                           ID3D11DeviceContext*   pDeviceCtx)
{
    if (NumCommittedResources > 0)
    {
        memset(CommittedD3D11Res, 0, NumCommittedResources * sizeof(CommittedD3D11Res[0]));
        SetD3D11ResourcesHelper(pDeviceCtx, SetD3D11ResMethod, 0, NumCommittedResources, CommittedD3D11Res);
    }
}

void ReleaseCommittedPSUAVs(ID3D11UnorderedAccessView* CommittedD3D11UAVs[],
                            Uint8                      NumCommittedResources,
                            ID3D11DeviceContext*       pDeviceCtx)
{
    if (NumCommittedResources > 0)
    {
        memset(CommittedD3D11UAVs, 0, NumCommittedResources * sizeof(CommittedD3D11UAVs[0]));
        pDeviceCtx->OMSetRenderTargetsAndUnorderedAccessViews(
            D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
            0, 0, nullptr, nullptr);
    }
}

void DeviceContextD3D11Impl::ReleaseCommittedShaderResources()
{
    for (int ShaderType = 0; ShaderType < NumShaderTypes; ++ShaderType)
    {
        // clang-format off
        ReleaseCommittedShaderResourcesHelper(m_CommittedD3D11CBs[ShaderType],      m_NumCommittedCBs[ShaderType],      SetCBMethods[ShaderType],      m_pd3d11DeviceContext);
        ReleaseCommittedShaderResourcesHelper(m_CommittedD3D11SRVs[ShaderType],     m_NumCommittedSRVs[ShaderType],     SetSRVMethods[ShaderType],     m_pd3d11DeviceContext);
        ReleaseCommittedShaderResourcesHelper(m_CommittedD3D11Samplers[ShaderType], m_NumCommittedSamplers[ShaderType], SetSamplerMethods[ShaderType], m_pd3d11DeviceContext);
        // clang-format on

        if (ShaderType == PSInd)
            ReleaseCommittedPSUAVs(m_CommittedD3D11UAVs[ShaderType], m_NumCommittedUAVs[ShaderType], m_pd3d11DeviceContext);
        else
            ReleaseCommittedShaderResourcesHelper(m_CommittedD3D11UAVs[ShaderType], m_NumCommittedUAVs[ShaderType], SetUAVMethods[ShaderType], m_pd3d11DeviceContext);

        memset(m_CommittedD3D11SRVResources[ShaderType], 0, sizeof(m_CommittedD3D11SRVResources[ShaderType][0]) * m_NumCommittedSRVs[ShaderType]);
        memset(m_CommittedD3D11UAVResources[ShaderType], 0, sizeof(m_CommittedD3D11UAVResources[ShaderType][0]) * m_NumCommittedUAVs[ShaderType]);
        m_NumCommittedCBs[ShaderType]      = 0;
        m_NumCommittedSRVs[ShaderType]     = 0;
        m_NumCommittedSamplers[ShaderType] = 0;
        m_NumCommittedUAVs[ShaderType]     = 0;
    }

#ifdef DILIGENT_DEVELOPMENT
    if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
    {
        dbgVerifyCommittedSRVs();
        dbgVerifyCommittedUAVs(SHADER_TYPE_COMPUTE);
        dbgVerifyCommittedSamplers();
        dbgVerifyCommittedCBs();
    }
#endif
    // We do not unbind vertex buffers and index buffer as this can explicitly
    // be done by the user
}


void DeviceContextD3D11Impl::FinishCommandList(ICommandList** ppCommandList)
{
    VERIFY(m_pActiveRenderPass == nullptr, "Finishing command list inside an active render pass.");

    CComPtr<ID3D11CommandList> pd3d11CmdList;
    m_pd3d11DeviceContext->FinishCommandList(
        FALSE, // A Boolean flag that determines whether the runtime saves deferred context state before it
               // executes FinishCommandList and restores it afterwards.
               // * TRUE indicates that the runtime needs to save and restore the state.
               // * FALSE indicates that the runtime will not save or restore any state.
               //   In this case, the deferred context will return to its default state
               //   after the call to FinishCommandList() completes as if
               //   ID3D11DeviceContext::ClearState() was called.
        &pd3d11CmdList);

    CommandListD3D11Impl* pCmdListD3D11(NEW_RC_OBJ(m_CmdListAllocator, "CommandListD3D11Impl instance", CommandListD3D11Impl)(m_pDevice, pd3d11CmdList));
    pCmdListD3D11->QueryInterface(IID_CommandList, reinterpret_cast<IObject**>(ppCommandList));

    // Device context is now in default state
    InvalidateState();

#ifdef DILIGENT_DEVELOPMENT
    if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
    {
        // Verify bindings
        dbgVerifyCommittedSRVs();
        dbgVerifyCommittedUAVs(SHADER_TYPE_COMPUTE);
        dbgVerifyCommittedSamplers();
        dbgVerifyCommittedCBs();
        dbgVerifyCommittedVertexBuffers();
        dbgVerifyCommittedIndexBuffer();
        dbgVerifyCommittedShaders();
    }
#endif
}

void DeviceContextD3D11Impl::ExecuteCommandList(ICommandList* pCommandList)
{
    if (m_bIsDeferred)
    {
        LOG_ERROR("Only immediate context can execute command list");
        return;
    }

    CommandListD3D11Impl* pCmdListD3D11 = ValidatedCast<CommandListD3D11Impl>(pCommandList);
    auto*                 pd3d11CmdList = pCmdListD3D11->GetD3D11CommandList();
    m_pd3d11DeviceContext->ExecuteCommandList(pd3d11CmdList,
                                              FALSE // A Boolean flag that determines whether the target context state is
                                                    // saved prior to and restored after the execution of a command list.
                                                    // * TRUE indicates that the runtime needs to save and restore the state.
                                                    // * FALSE indicate that no state shall be saved or restored, which causes the
                                                    //   target context to return to its default state after the command list executes as if
                                                    //   ID3D11DeviceContext::ClearState() was called.
    );

    // Device context is now in default state
    InvalidateState();

#ifdef DILIGENT_DEVELOPMENT
    if (m_DebugFlags & D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE)
    {
        // Verify bindings
        dbgVerifyCommittedSRVs();
        dbgVerifyCommittedUAVs(SHADER_TYPE_COMPUTE);
        dbgVerifyCommittedSamplers();
        dbgVerifyCommittedCBs();
        dbgVerifyCommittedVertexBuffers();
        dbgVerifyCommittedIndexBuffer();
        dbgVerifyCommittedShaders();
    }
#endif
}


static CComPtr<ID3D11Query> CreateD3D11QueryEvent(ID3D11Device* pd3d11Device)
{
    D3D11_QUERY_DESC QueryDesc = {};
    QueryDesc.Query            = D3D11_QUERY_EVENT; // Determines whether or not the GPU is finished processing commands.
                                                    // When the GPU is finished processing commands ID3D11DeviceContext::GetData will
                                                    // return S_OK, and pData will point to a BOOL with a value of TRUE. When using this
                                                    // type of query, ID3D11DeviceContext::Begin is disabled.
    QueryDesc.MiscFlags = 0;

    CComPtr<ID3D11Query> pd3d11Query;
    auto                 hr = pd3d11Device->CreateQuery(&QueryDesc, &pd3d11Query);
    DEV_CHECK_ERR(SUCCEEDED(hr), "Failed to create D3D11 query");
    VERIFY_EXPR(pd3d11Query);
    return pd3d11Query;
}

void DeviceContextD3D11Impl::SignalFence(IFence* pFence, Uint64 Value)
{
    VERIFY(!m_bIsDeferred, "Fence can only be signaled from immediate context");
    auto*                pd3d11Device = m_pDevice->GetD3D11Device();
    CComPtr<ID3D11Query> pd3d11Query  = CreateD3D11QueryEvent(pd3d11Device);
    m_pd3d11DeviceContext->End(pd3d11Query);
    auto* pFenceD3D11Impl = ValidatedCast<FenceD3D11Impl>(pFence);
    pFenceD3D11Impl->AddPendingQuery(m_pd3d11DeviceContext, std::move(pd3d11Query), Value);
}

void DeviceContextD3D11Impl::WaitForFence(IFence* pFence, Uint64 Value, bool FlushContext)
{
    VERIFY(!m_bIsDeferred, "Fence can only be waited from immediate context");
    if (FlushContext)
        Flush();
    auto* pFenceD3D11Impl = ValidatedCast<FenceD3D11Impl>(pFence);
    pFenceD3D11Impl->Wait(Value, FlushContext);
}

void DeviceContextD3D11Impl::WaitForIdle()
{
    VERIFY(!m_bIsDeferred, "Only immediate contexts can be idled");
    Flush();
    auto*                pd3d11Device = m_pDevice->GetD3D11Device();
    CComPtr<ID3D11Query> pd3d11Query  = CreateD3D11QueryEvent(pd3d11Device);
    m_pd3d11DeviceContext->End(pd3d11Query);
    BOOL Data;
    while (m_pd3d11DeviceContext->GetData(pd3d11Query, &Data, sizeof(Data), 0) != S_OK)
        std::this_thread::yield();
}

std::shared_ptr<DisjointQueryPool::DisjointQueryWrapper> DeviceContextD3D11Impl::BeginDisjointQuery()
{
    if (!m_ActiveDisjointQuery)
    {
        m_ActiveDisjointQuery = m_DisjointQueryPool.GetDisjointQuery(m_pDevice->GetD3D11Device());
        // Disjoint timestamp queries should only be invoked once per frame or less.
        m_pd3d11DeviceContext->Begin(m_ActiveDisjointQuery->pd3d11Query);
        m_ActiveDisjointQuery->IsEnded = false;
    }
    return m_ActiveDisjointQuery;
}

void DeviceContextD3D11Impl::BeginQuery(IQuery* pQuery)
{
    if (!TDeviceContextBase::BeginQuery(pQuery, 0))
        return;

    auto* const pQueryD3D11Impl = ValidatedCast<QueryD3D11Impl>(pQuery);
    if (pQueryD3D11Impl->GetDesc().Type == QUERY_TYPE_DURATION)
    {
        pQueryD3D11Impl->SetDisjointQuery(BeginDisjointQuery());
        m_pd3d11DeviceContext->End(pQueryD3D11Impl->GetD3D11Query(0));
    }
    else
    {
        m_pd3d11DeviceContext->Begin(pQueryD3D11Impl->GetD3D11Query(0));
    }
}

void DeviceContextD3D11Impl::EndQuery(IQuery* pQuery)
{
    if (!TDeviceContextBase::EndQuery(pQuery, 0))
        return;

    auto* const pQueryD3D11Impl = ValidatedCast<QueryD3D11Impl>(pQuery);

    const auto QueryType = pQueryD3D11Impl->GetDesc().Type;
    VERIFY(QueryType != QUERY_TYPE_DURATION || m_ActiveDisjointQuery,
           "There is no active disjoint query. Did you forget to call BeginQuery for the duration query?");
    if (QueryType == QUERY_TYPE_TIMESTAMP)
    {
        pQueryD3D11Impl->SetDisjointQuery(BeginDisjointQuery());
    }
    m_pd3d11DeviceContext->End(pQueryD3D11Impl->GetD3D11Query(QueryType == QUERY_TYPE_DURATION ? 1 : 0));
}

void DeviceContextD3D11Impl::ClearStateCache()
{
    TDeviceContextBase::ClearStateCache();

    for (int ShaderType = 0; ShaderType < NumShaderTypes; ++ShaderType)
    {
        // clang-format off
        memset(m_CommittedD3D11CBs[ShaderType],          0, sizeof(m_CommittedD3D11CBs[ShaderType][0])          * m_NumCommittedCBs[ShaderType]);
        memset(m_CommittedD3D11SRVs[ShaderType],         0, sizeof(m_CommittedD3D11SRVs[ShaderType][0])         * m_NumCommittedSRVs[ShaderType]);
        memset(m_CommittedD3D11Samplers[ShaderType],     0, sizeof(m_CommittedD3D11Samplers[ShaderType][0])     * m_NumCommittedSamplers[ShaderType]);
        memset(m_CommittedD3D11UAVs[ShaderType],         0, sizeof(m_CommittedD3D11UAVs[ShaderType][0])         * m_NumCommittedUAVs[ShaderType]);
        memset(m_CommittedD3D11SRVResources[ShaderType], 0, sizeof(m_CommittedD3D11SRVResources[ShaderType][0]) * m_NumCommittedSRVs[ShaderType]);
        memset(m_CommittedD3D11UAVResources[ShaderType], 0, sizeof(m_CommittedD3D11UAVResources[ShaderType][0]) * m_NumCommittedUAVs[ShaderType]);
        // clang-format on

        m_NumCommittedCBs[ShaderType]      = 0;
        m_NumCommittedSRVs[ShaderType]     = 0;
        m_NumCommittedSamplers[ShaderType] = 0;
        m_NumCommittedUAVs[ShaderType]     = 0;

        m_CommittedD3DShaders[ShaderType].Release();
    }

    for (Uint32 vb = 0; vb < m_NumCommittedD3D11VBs; ++vb)
    {
        m_CommittedD3D11VertexBuffers[vb] = nullptr;
        m_CommittedD3D11VBStrides[vb]     = 0;
        m_CommittedD3D11VBOffsets[vb]     = 0;
    }
    m_NumCommittedD3D11VBs       = 0;
    m_bCommittedD3D11VBsUpToDate = false;

    m_CommittedD3D11InputLayout = nullptr;

    m_CommittedD3D11IndexBuffer.Release();
    m_CommittedIBFormat                  = VT_UNDEFINED;
    m_CommittedD3D11IndexDataStartOffset = 0;
    m_bCommittedD3D11IBUpToDate          = false;

    m_CommittedD3D11PrimTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_CommittedPrimitiveTopology = PRIMITIVE_TOPOLOGY_UNDEFINED;
}

void DeviceContextD3D11Impl::InvalidateState()
{
    TDeviceContextBase::InvalidateState();

    ReleaseCommittedShaderResources();
    for (int ShaderType = 0; ShaderType < NumShaderTypes; ++ShaderType)
        m_CommittedD3DShaders[ShaderType].Release();
    m_pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->CSSetShader(nullptr, nullptr, 0);
    ID3D11RenderTargetView* d3d11NullRTV[] = {nullptr};
    m_pd3d11DeviceContext->OMSetRenderTargets(1, d3d11NullRTV, nullptr);

    if (m_NumCommittedD3D11VBs > 0)
    {
        for (Uint32 vb = 0; vb < m_NumCommittedD3D11VBs; ++vb)
        {
            m_CommittedD3D11VertexBuffers[vb] = nullptr;
            m_CommittedD3D11VBStrides[vb]     = 0;
            m_CommittedD3D11VBOffsets[vb]     = 0;
        }
        m_pd3d11DeviceContext->IASetVertexBuffers(0, m_NumCommittedD3D11VBs, m_CommittedD3D11VertexBuffers, m_CommittedD3D11VBStrides, m_CommittedD3D11VBOffsets);
        m_NumCommittedD3D11VBs = 0;
    }

    m_bCommittedD3D11VBsUpToDate = false;

    if (m_CommittedD3D11InputLayout != nullptr)
    {
        m_pd3d11DeviceContext->IASetInputLayout(nullptr);
        m_CommittedD3D11InputLayout = nullptr;
    }

    if (m_CommittedD3D11IndexBuffer)
    {
        m_pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
        m_CommittedD3D11IndexBuffer.Release();
    }

    m_CommittedIBFormat                  = VT_UNDEFINED;
    m_CommittedD3D11IndexDataStartOffset = 0;
    m_bCommittedD3D11IBUpToDate          = false;

    m_CommittedD3D11PrimTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_CommittedPrimitiveTopology = PRIMITIVE_TOPOLOGY_UNDEFINED;
}

void DeviceContextD3D11Impl::TransitionResourceStates(Uint32 BarrierCount, StateTransitionDesc* pResourceBarriers)
{
    VERIFY(m_pActiveRenderPass == nullptr, "State transitions are not allowed inside a render pass");

    for (Uint32 i = 0; i < BarrierCount; ++i)
    {
        const auto& Barrier = pResourceBarriers[i];
#ifdef DILIGENT_DEVELOPMENT
        DvpVerifyStateTransitionDesc(Barrier);
#endif
        DEV_CHECK_ERR((Barrier.pTexture != nullptr) ^ (Barrier.pBuffer != nullptr), "Exactly one of pTexture or pBuffer must not be null");
        DEV_CHECK_ERR(Barrier.NewState != RESOURCE_STATE_UNKNOWN, "New resource state can't be unknown");

        if (Barrier.TransitionType == STATE_TRANSITION_TYPE_BEGIN)
        {
            // Skip begin-split barriers
            VERIFY(!Barrier.UpdateResourceState, "Resource state can't be updated in begin-split barrier");
            continue;
        }
        VERIFY(Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE || Barrier.TransitionType == STATE_TRANSITION_TYPE_END, "Unexpected barrier type");

        if (Barrier.pTexture)
        {
            auto* pTextureD3D11Impl = ValidatedCast<TextureBaseD3D11>(Barrier.pTexture);
            auto  OldState          = Barrier.OldState;
            if (OldState == RESOURCE_STATE_UNKNOWN)
            {
                if (pTextureD3D11Impl->IsInKnownState())
                {
                    OldState = pTextureD3D11Impl->GetState();
                }
                else
                {
                    LOG_ERROR_MESSAGE("Failed to transition the state of texture '", pTextureD3D11Impl->GetDesc().Name, "' because the buffer state is unknown and is not explicitly specified");
                    continue;
                }
            }
            else
            {
                if (pTextureD3D11Impl->IsInKnownState() && pTextureD3D11Impl->GetState() != OldState)
                {
                    LOG_ERROR_MESSAGE("The state ", GetResourceStateString(pTextureD3D11Impl->GetState()), " of texture '",
                                      pTextureD3D11Impl->GetDesc().Name, "' does not match the old state ", GetResourceStateString(OldState),
                                      " specified by the barrier");
                }
            }

            if ((Barrier.NewState & RESOURCE_STATE_UNORDERED_ACCESS) != 0)
            {
                DEV_CHECK_ERR((Barrier.NewState & (RESOURCE_STATE_GENERIC_READ | RESOURCE_STATE_INPUT_ATTACHMENT)) == 0, "Unordered access state is not compatible with any input state");
                UnbindTextureFromInput(pTextureD3D11Impl, pTextureD3D11Impl->GetD3D11Texture());
            }

            if ((Barrier.NewState & (RESOURCE_STATE_GENERIC_READ | RESOURCE_STATE_INPUT_ATTACHMENT)) != 0)
            {
                if ((OldState & RESOURCE_STATE_RENDER_TARGET) != 0)
                    UnbindTextureFromRenderTarget(pTextureD3D11Impl);

                if ((OldState & RESOURCE_STATE_DEPTH_WRITE) != 0)
                    UnbindTextureFromDepthStencil(pTextureD3D11Impl);

                if ((OldState & RESOURCE_STATE_UNORDERED_ACCESS) != 0)
                {
                    UnbindResourceFromUAV(pTextureD3D11Impl, pTextureD3D11Impl->GetD3D11Texture());
                    pTextureD3D11Impl->ClearState(RESOURCE_STATE_UNORDERED_ACCESS);
                }
            }

            if (Barrier.UpdateResourceState)
            {
                pTextureD3D11Impl->SetState(Barrier.NewState);
            }
        }
        else
        {
            VERIFY_EXPR(Barrier.pBuffer);
            auto* pBufferD3D11Impl = ValidatedCast<BufferD3D11Impl>(Barrier.pBuffer);
            auto  OldState         = Barrier.OldState;
            if (OldState == RESOURCE_STATE_UNKNOWN)
            {
                if (pBufferD3D11Impl->IsInKnownState())
                {
                    OldState = pBufferD3D11Impl->GetState();
                }
                else
                {
                    LOG_ERROR_MESSAGE("Failed to transition the state of buffer '", pBufferD3D11Impl->GetDesc().Name, "' because the buffer state is unknown and is not explicitly specified");
                    continue;
                }
            }
            else
            {
                if (pBufferD3D11Impl->IsInKnownState() && pBufferD3D11Impl->GetState() != OldState)
                {
                    LOG_ERROR_MESSAGE("The state ", GetResourceStateString(pBufferD3D11Impl->GetState()), " of buffer '",
                                      pBufferD3D11Impl->GetDesc().Name, "' does not match the old state ", GetResourceStateString(OldState),
                                      " specified by the barrier");
                }
            }

            if ((Barrier.NewState & RESOURCE_STATE_UNORDERED_ACCESS) != 0)
            {
                DEV_CHECK_ERR((Barrier.NewState & RESOURCE_STATE_GENERIC_READ) == 0, "Unordered access state is not compatible with any input state");
                UnbindBufferFromInput(pBufferD3D11Impl, pBufferD3D11Impl->m_pd3d11Buffer);
            }

            if ((Barrier.NewState & RESOURCE_STATE_GENERIC_READ) != 0)
            {
                UnbindResourceFromUAV(pBufferD3D11Impl, pBufferD3D11Impl->m_pd3d11Buffer);
            }

            if (Barrier.UpdateResourceState)
            {
                pBufferD3D11Impl->SetState(Barrier.NewState);
            }
        }
    }
}

void DeviceContextD3D11Impl::ResolveTextureSubresource(ITexture*                               pSrcTexture,
                                                       ITexture*                               pDstTexture,
                                                       const ResolveTextureSubresourceAttribs& ResolveAttribs)
{
    TDeviceContextBase::ResolveTextureSubresource(pSrcTexture, pDstTexture, ResolveAttribs);

    auto*       pSrcTexD3D11 = ValidatedCast<TextureBaseD3D11>(pSrcTexture);
    auto*       pDstTexD3D11 = ValidatedCast<TextureBaseD3D11>(pDstTexture);
    const auto& SrcTexDesc   = pSrcTexD3D11->GetDesc();
    const auto& DstTexDesc   = pDstTexD3D11->GetDesc();

    auto Format = ResolveAttribs.Format;
    if (Format == TEX_FORMAT_UNKNOWN)
    {
        const auto& SrcFmtAttribs = GetTextureFormatAttribs(SrcTexDesc.Format);
        if (!SrcFmtAttribs.IsTypeless)
        {
            Format = SrcTexDesc.Format;
        }
        else
        {
            const auto& DstFmtAttribs = GetTextureFormatAttribs(DstTexDesc.Format);
            DEV_CHECK_ERR(!DstFmtAttribs.IsTypeless, "Resolve operation format can't be typeless when both source and destination textures are typeless");
            Format = DstFmtAttribs.Format;
        }
    }

    auto DXGIFmt        = TexFormatToDXGI_Format(Format);
    auto SrcSubresIndex = D3D11CalcSubresource(ResolveAttribs.SrcMipLevel, ResolveAttribs.SrcSlice, SrcTexDesc.MipLevels);
    auto DstSubresIndex = D3D11CalcSubresource(ResolveAttribs.DstMipLevel, ResolveAttribs.DstSlice, DstTexDesc.MipLevels);
    m_pd3d11DeviceContext->ResolveSubresource(pDstTexD3D11->GetD3D11Texture(), DstSubresIndex, pSrcTexD3D11->GetD3D11Texture(), SrcSubresIndex, DXGIFmt);
}

// clang-format off
#ifdef VERIFY_CONTEXT_BINDINGS
    DEFINE_D3D11CTX_FUNC_POINTERS(GetCBMethods,      GetConstantBuffers)
    DEFINE_D3D11CTX_FUNC_POINTERS(GetSRVMethods,     GetShaderResources)
    DEFINE_D3D11CTX_FUNC_POINTERS(GetSamplerMethods, GetSamplers)

    typedef decltype (&ID3D11DeviceContext::CSGetUnorderedAccessViews) TGetUnorderedAccessViewsType;
    static const TGetUnorderedAccessViewsType GetUAVMethods[] =
    {
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        &ID3D11DeviceContext::CSGetUnorderedAccessViews
    };
// clang-format on

/// \tparam MaxResources - Maximum number of resources that can be bound to D3D11 context
/// \tparam TD3D11ResourceType - Type of D3D11 resource being checked (ID3D11ShaderResourceView,
///                              ID3D11UnorderedAccessView, ID3D11Buffer or ID3D11SamplerState).
/// \tparam TGetD3D11ResourcesType - Type of the device context method used to get the bound
///                                  resources
/// \param CommittedD3D11ResourcesArr - Pointer to the array of currently bound D3D11
///                                 resources, for each shader stage
/// \param GetD3D11ResMethods - Pointer to the array of device context methods to get the bound
///                             resources, for each shader stage
/// \param ResourceName - Resource name
/// \param ShaderType - Shader type for which to check the resources. If Diligent::SHADER_TYPE_UNKNOWN
///                     is provided, all shader stages will be checked
template <UINT MaxResources,
          typename TD3D11ResourceType,
          typename TGetD3D11ResourcesType>
void DeviceContextD3D11Impl::dbgVerifyCommittedResources(TD3D11ResourceType     CommittedD3D11ResourcesArr[][MaxResources],
                                                         Uint8                  NumCommittedResourcesArr[],
                                                         TGetD3D11ResourcesType GetD3D11ResMethods[],
                                                         const Char*            ResourceName,
                                                         SHADER_TYPE            ShaderType)
{
    int iStartInd = 0, iEndInd = NumShaderTypes;
    if (ShaderType != SHADER_TYPE_UNKNOWN)
    {
        iStartInd = GetShaderTypeIndex(ShaderType);
        iEndInd   = iStartInd + 1;
    }
    for (int iShaderTypeInd = iStartInd; iShaderTypeInd < iEndInd; ++iShaderTypeInd)
    {
        const auto         ShaderName                  = GetShaderTypeLiteralName(GetShaderTypeFromIndex(iShaderTypeInd));
        TD3D11ResourceType pctxResources[MaxResources] = {};
        auto               GetResMethod                = GetD3D11ResMethods[iShaderTypeInd];
        if (GetResMethod)
        {
            (m_pd3d11DeviceContext->*GetResMethod)(0, _countof(pctxResources), pctxResources);
        }
        const auto* CommittedResources    = CommittedD3D11ResourcesArr[iShaderTypeInd];
        auto        NumCommittedResources = NumCommittedResourcesArr[iShaderTypeInd];
        for (Uint32 Slot = 0; Slot < _countof(pctxResources); ++Slot)
        {
            if (Slot < NumCommittedResources)
            {
                VERIFY(CommittedResources[Slot] == pctxResources[Slot], ResourceName, " binding mismatch found for ", ShaderName, " shader type at slot ", Slot);
            }
            else
            {
                VERIFY(pctxResources[Slot] == nullptr, ResourceName, " binding mismatch found for ", ShaderName, " shader type at slot ", Slot);
                VERIFY(CommittedResources[Slot] == nullptr, ResourceName, " unexpected non-null resource found for ", ShaderName, " shader type at slot ", Slot);
            }

            if (pctxResources[Slot])
                pctxResources[Slot]->Release();
        }
    }
}

template <UINT MaxResources, typename TD3D11ViewType>
void DeviceContextD3D11Impl::dbgVerifyViewConsistency(TD3D11ViewType  CommittedD3D11ViewArr[][MaxResources],
                                                      ID3D11Resource* CommittedD3D11ResourcesArr[][MaxResources],
                                                      Uint8           NumCommittedResourcesArr[],
                                                      const Char*     ResourceName,
                                                      SHADER_TYPE     ShaderType)
{
    int iStartInd = 0, iEndInd = NumShaderTypes;
    if (ShaderType != SHADER_TYPE_UNKNOWN)
    {
        iStartInd = GetShaderTypeIndex(ShaderType);
        iEndInd   = iStartInd + 1;
    }
    for (int iShaderTypeInd = iStartInd; iShaderTypeInd < iEndInd; ++iShaderTypeInd)
    {
        const auto ShaderName            = GetShaderTypeLiteralName(GetShaderTypeFromIndex(iShaderTypeInd));
        auto*      Views                 = CommittedD3D11ViewArr[iShaderTypeInd];
        auto*      Resources             = CommittedD3D11ResourcesArr[iShaderTypeInd];
        auto       NumCommittedResources = NumCommittedResourcesArr[iShaderTypeInd];
        for (Uint32 Slot = 0; Slot < NumCommittedResources; ++Slot)
        {
            if (Views[Slot] != nullptr)
            {
                CComPtr<ID3D11Resource> pRefRes;
                Views[Slot]->GetResource(&pRefRes);
                VERIFY(pRefRes == Resources[Slot], "Inconsistent ", ResourceName, " detected at slot ", Slot, " in shader ", ShaderName, ". The resource in the view does not match cached D3D11 resource");
            }
        }
    }
}

void DeviceContextD3D11Impl::dbgVerifyCommittedSRVs(SHADER_TYPE ShaderType)
{
    dbgVerifyCommittedResources<D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>(m_CommittedD3D11SRVs, m_NumCommittedSRVs, GetSRVMethods, "SRV", ShaderType);
    dbgVerifyViewConsistency<D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>(m_CommittedD3D11SRVs, m_CommittedD3D11SRVResources, m_NumCommittedSRVs, "SRV", ShaderType);
}

void DeviceContextD3D11Impl::dbgVerifyCommittedUAVs(SHADER_TYPE ShaderType)
{
    dbgVerifyCommittedResources<D3D11_PS_CS_UAV_REGISTER_COUNT>(m_CommittedD3D11UAVs, m_NumCommittedUAVs, GetUAVMethods, "UAV", ShaderType);
    dbgVerifyViewConsistency<D3D11_PS_CS_UAV_REGISTER_COUNT>(m_CommittedD3D11UAVs, m_CommittedD3D11UAVResources, m_NumCommittedUAVs, "UAV", ShaderType);
}

void DeviceContextD3D11Impl::dbgVerifyCommittedSamplers(SHADER_TYPE ShaderType)
{
    dbgVerifyCommittedResources<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT>(m_CommittedD3D11Samplers, m_NumCommittedSamplers, GetSamplerMethods, "Sampler", ShaderType);
}

void DeviceContextD3D11Impl::dbgVerifyCommittedCBs(SHADER_TYPE ShaderType)
{
    dbgVerifyCommittedResources<D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT>(m_CommittedD3D11CBs, m_NumCommittedCBs, GetCBMethods, "Constant buffer", ShaderType);
}

void DeviceContextD3D11Impl::dbgVerifyCommittedIndexBuffer()
{
    RefCntAutoPtr<ID3D11Buffer> pctxIndexBuffer;
    DXGI_FORMAT                 Fmt    = DXGI_FORMAT_UNKNOWN;
    UINT                        Offset = 0;
    m_pd3d11DeviceContext->IAGetIndexBuffer(&pctxIndexBuffer, &Fmt, &Offset);
    if (m_CommittedD3D11IndexBuffer && !pctxIndexBuffer)
        UNEXPECTED("D3D11 index buffer is not bound to the context");
    if (!m_CommittedD3D11IndexBuffer && pctxIndexBuffer)
        UNEXPECTED("Unexpected D3D11 index buffer is bound to the context");

    if (m_CommittedD3D11IndexBuffer && pctxIndexBuffer)
    {
        VERIFY(m_CommittedD3D11IndexBuffer == pctxIndexBuffer, "Index buffer binding mismatch detected");
        if (Fmt == DXGI_FORMAT_R32_UINT)
        {
            VERIFY(m_CommittedIBFormat == VT_UINT32, "Index buffer format mismatch detected");
        }
        else if (Fmt == DXGI_FORMAT_R16_UINT)
        {
            VERIFY(m_CommittedIBFormat == VT_UINT16, "Index buffer format mismatch detected");
        }
        VERIFY(m_CommittedD3D11IndexDataStartOffset == Offset, "Index buffer offset mismatch detected");
    }
}

void DeviceContextD3D11Impl::dbgVerifyCommittedVertexBuffers()
{
    CComPtr<ID3D11InputLayout> pInputLayout;
    m_pd3d11DeviceContext->IAGetInputLayout(&pInputLayout);
    VERIFY(pInputLayout == m_CommittedD3D11InputLayout, "Inconsistent input layout");

    const Uint32  MaxVBs = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    ID3D11Buffer* pVBs[MaxVBs];
    UINT          Strides[MaxVBs];
    UINT          Offsets[MaxVBs];
    m_pd3d11DeviceContext->IAGetVertexBuffers(0, MaxVBs, pVBs, Strides, Offsets);
    auto NumBoundVBs = m_NumCommittedD3D11VBs;
    for (Uint32 Slot = 0; Slot < MaxVBs; ++Slot)
    {
        if (Slot < NumBoundVBs)
        {
            const auto& BoundD3D11VB  = m_CommittedD3D11VertexBuffers[Slot];
            auto        BoundVBStride = m_CommittedD3D11VBStrides[Slot];
            auto        BoundVBOffset = m_CommittedD3D11VBOffsets[Slot];
            if (BoundD3D11VB && !pVBs[Slot])
                VERIFY(pVBs[Slot] == nullptr, "Missing D3D11 buffer detected at slot ", Slot);
            if (!BoundD3D11VB && pVBs[Slot])
                VERIFY(pVBs[Slot] == nullptr, "Unexpected D3D11 buffer detected at slot ", Slot);
            if (BoundD3D11VB && pVBs[Slot])
            {
                VERIFY(BoundD3D11VB == pVBs[Slot], "Vertex buffer mismatch detected at slot ", Slot);
                VERIFY(BoundVBOffset == Offsets[Slot], "Offset mismatch detected at slot ", Slot);
                VERIFY(BoundVBStride == Strides[Slot], "Stride mismatch detected at slot ", Slot);
            }
        }
        else
        {
            VERIFY(pVBs[Slot] == nullptr, "Unexpected D3D11 buffer detected at slot ", Slot);
        }

        if (pVBs[Slot])
            pVBs[Slot]->Release();
    }
}

template <typename TD3D11ShaderType, typename TGetShaderMethodType>
void dbgVerifyCommittedShadersHelper(SHADER_TYPE                      ShaderType,
                                     const CComPtr<ID3D11DeviceChild> BoundD3DShaders[],
                                     ID3D11DeviceContext*             pCtx,
                                     TGetShaderMethodType             GetShaderMethod)
{
    RefCntAutoPtr<TD3D11ShaderType> pctxShader;
    (pCtx->*GetShaderMethod)(&pctxShader, nullptr, nullptr);
    const auto& BoundShader = BoundD3DShaders[GetShaderTypeIndex(ShaderType)];
    VERIFY(BoundShader == pctxShader, GetShaderTypeLiteralName(ShaderType), " binding mismatch detected");
}
void DeviceContextD3D11Impl::dbgVerifyCommittedShaders()
{
#    define VERIFY_SHADER(NAME, Name, N) dbgVerifyCommittedShadersHelper<ID3D11##Name##Shader>(SHADER_TYPE_##NAME, m_CommittedD3DShaders, m_pd3d11DeviceContext, &ID3D11DeviceContext::N##SGetShader)
    // These shaders which are not set will be unbound from the D3D11 device context
    VERIFY_SHADER(VERTEX, Vertex, V);
    VERIFY_SHADER(PIXEL, Pixel, P);
    VERIFY_SHADER(GEOMETRY, Geometry, G);
    VERIFY_SHADER(DOMAIN, Domain, D);
    VERIFY_SHADER(HULL, Hull, H);
    VERIFY_SHADER(COMPUTE, Compute, C);
}

#endif // VERIFY_CONTEXT_BINDINGS
} // namespace Diligent
