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
#include "ShaderResourceBindingD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "ShaderD3D12Impl.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "LinearAllocator.hpp"

namespace Diligent
{

ShaderResourceBindingD3D12Impl::ShaderResourceBindingD3D12Impl(IReferenceCounters*     pRefCounters,
                                                               PipelineStateD3D12Impl* pPSO,
                                                               bool                    IsPSOInternal) :
    // clang-format off
    TBase
    {
        pRefCounters,
        pPSO,
        IsPSOInternal
    },
    m_ShaderResourceCache{ShaderResourceCacheD3D12::DbgCacheContentType::SRBResources},
    m_NumShaders         {static_cast<decltype(m_NumShaders)>(pPSO->GetNumShaderStages())}
// clang-format on
{
    try
    {
        m_ResourceLayoutIndex.fill(-1);

        LinearAllocator MemPool{GetRawAllocator()};
        MemPool.AddSpace<ShaderVariableManagerD3D12>(m_NumShaders);
        MemPool.Reserve();
        m_pShaderVarMgrs = MemPool.ConstructArray<ShaderVariableManagerD3D12>(m_NumShaders, std::ref(*this), std::ref(m_ShaderResourceCache));

        // The memory is now owned by ShaderResourceBindingD3D12Impl and will be freed by Destruct().
        auto* Ptr = MemPool.ReleaseOwnership();
        VERIFY_EXPR(Ptr == m_pShaderVarMgrs);
        (void)Ptr;

        // It is important to construct all objects before initializing them because if an exception is thrown,
        // destructors will be called for all objects

        auto* pRenderDeviceD3D12Impl = ValidatedCast<RenderDeviceD3D12Impl>(pPSO->GetDevice());
        auto& ResCacheDataAllocator  = pPSO->GetSRBMemoryAllocator().GetResourceCacheDataAllocator(0);
        pPSO->GetRootSignature().InitResourceCache(pRenderDeviceD3D12Impl, m_ShaderResourceCache, ResCacheDataAllocator);

        for (Uint32 s = 0; s < m_NumShaders; ++s)
        {
            const auto  ShaderType = pPSO->GetShaderStageType(s);
            const auto& SrcLayout  = pPSO->GetShaderResLayout(s);
            const auto  ShaderInd  = GetShaderTypePipelineIndex(ShaderType, pPSO->GetDesc().PipelineType);

            auto& VarDataAllocator = pPSO->GetSRBMemoryAllocator().GetShaderVariableDataAllocator(s);

            // http://diligentgraphics.com/diligent-engine/architecture/d3d12/shader-resource-layout#Initializing-Resource-Layouts-in-a-Shader-Resource-Binding-Object
            const SHADER_RESOURCE_VARIABLE_TYPE AllowedVarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
            m_pShaderVarMgrs[s].Initialize(
                SrcLayout,
                VarDataAllocator,
                AllowedVarTypes,
                _countof(AllowedVarTypes) //
            );

            m_ResourceLayoutIndex[ShaderInd] = static_cast<Int8>(s);
        }
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}


ShaderResourceBindingD3D12Impl::~ShaderResourceBindingD3D12Impl()
{
    Destruct();
}

void ShaderResourceBindingD3D12Impl::Destruct()
{
    if (m_pShaderVarMgrs != nullptr)
    {
        auto& SRBMemAllocator = m_pPSO->GetSRBMemoryAllocator();
        for (Uint32 s = 0; s < m_NumShaders; ++s)
        {
            auto& VarDataAllocator = SRBMemAllocator.GetShaderVariableDataAllocator(s);
            m_pShaderVarMgrs[s].Destroy(VarDataAllocator);
            m_pShaderVarMgrs[s].~ShaderVariableManagerD3D12();
        }
        GetRawAllocator().Free(m_pShaderVarMgrs);
    }
}

IMPLEMENT_QUERY_INTERFACE(ShaderResourceBindingD3D12Impl, IID_ShaderResourceBindingD3D12, TBase)

void ShaderResourceBindingD3D12Impl::BindResources(Uint32 ShaderFlags, IResourceMapping* pResMapping, Uint32 Flags)
{
    const auto PipelineType = m_pPSO->GetDesc().PipelineType;
    for (Int32 ShaderInd = 0; ShaderInd < static_cast<Int32>(m_ResourceLayoutIndex.size()); ++ShaderInd)
    {
        auto ResLayoutInd = m_ResourceLayoutIndex[ShaderInd];
        if (ResLayoutInd >= 0)
        {
            // ShaderInd is the shader type pipeline index here
            const auto ShaderType = GetShaderTypeFromPipelineIndex(ShaderInd, PipelineType);
            if (ShaderFlags & ShaderType)
            {
                m_pShaderVarMgrs[ResLayoutInd].BindResources(pResMapping, Flags);
            }
        }
    }
}

IShaderResourceVariable* ShaderResourceBindingD3D12Impl::GetVariableByName(SHADER_TYPE ShaderType, const char* Name)
{
    auto ResLayoutInd = GetVariableByNameHelper(ShaderType, Name, m_ResourceLayoutIndex);
    if (ResLayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(ResLayoutInd) < Uint32{m_NumShaders});
    return m_pShaderVarMgrs[ResLayoutInd].GetVariable(Name);
}

Uint32 ShaderResourceBindingD3D12Impl::GetVariableCount(SHADER_TYPE ShaderType) const
{
    auto ResLayoutInd = GetVariableCountHelper(ShaderType, m_ResourceLayoutIndex);
    if (ResLayoutInd < 0)
        return 0;

    VERIFY_EXPR(static_cast<Uint32>(ResLayoutInd) < Uint32{m_NumShaders});
    return m_pShaderVarMgrs[ResLayoutInd].GetVariableCount();
}

IShaderResourceVariable* ShaderResourceBindingD3D12Impl::GetVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    auto ResLayoutInd = GetVariableByIndexHelper(ShaderType, Index, m_ResourceLayoutIndex);
    if (ResLayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(ResLayoutInd) < Uint32{m_NumShaders});
    return m_pShaderVarMgrs[ResLayoutInd].GetVariable(Index);
}


#ifdef DILIGENT_DEVELOPMENT
void ShaderResourceBindingD3D12Impl::dvpVerifyResourceBindings(const PipelineStateD3D12Impl* pPSO) const
{
    auto* pRefPSO = GetPipelineState<const PipelineStateD3D12Impl>();
    if (pPSO->IsIncompatibleWith(pRefPSO))
    {
        LOG_ERROR("Shader resource binding is incompatible with the pipeline state \"", pPSO->GetDesc().Name, '\"');
        return;
    }
    for (Uint32 l = 0; l < m_NumShaders; ++l)
    {
        // Use reference layout from pipeline state that contains all shader resource types
        const auto& ShaderResLayout = pRefPSO->GetShaderResLayout(l);
        ShaderResLayout.dvpVerifyBindings(m_ShaderResourceCache);
    }
#    ifdef DILIGENT_DEBUG
    m_ShaderResourceCache.DbgVerifyBoundDynamicCBsCounter();
#    endif
}
#endif


void ShaderResourceBindingD3D12Impl::InitializeStaticResources(const IPipelineState* pPSO)
{
    if (StaticResourcesInitialized())
    {
        LOG_WARNING_MESSAGE("Static resources have already been initialized in this shader "
                            "resource binding object. The operation will be ignored.");
        return;
    }

    if (pPSO == nullptr)
    {
        pPSO = GetPipelineState();
    }
    else
    {
        DEV_CHECK_ERR(pPSO->IsCompatibleWith(GetPipelineState()), "The pipeline state is not compatible with this SRB");
    }

    auto* pPSO12     = ValidatedCast<const PipelineStateD3D12Impl>(pPSO);
    auto  NumShaders = pPSO12->GetNumShaderStages();
    // Copy static resources
    for (Uint32 s = 0; s < NumShaders; ++s)
    {
        const auto& ShaderResLayout = pPSO12->GetShaderResLayout(s);
        auto&       StaticResLayout = pPSO12->GetStaticShaderResLayout(s);
        auto&       StaticResCache  = pPSO12->GetStaticShaderResCache(s);

#ifdef DILIGENT_DEVELOPMENT
        if (!StaticResLayout.dvpVerifyBindings(StaticResCache))
        {
            LOG_ERROR_MESSAGE("Static resources in SRB of PSO '", pPSO12->GetDesc().Name,
                              "' will not be successfully initialized because not all static resource bindings in shader type '",
                              GetShaderTypeLiteralName(pPSO12->GetShaderStageType(s)),
                              "' are valid. Please make sure you bind all static resources to PSO before calling InitializeStaticResources() "
                              "directly or indirectly by passing InitStaticResources=true to CreateShaderResourceBinding() method.");
        }
#endif
        StaticResLayout.CopyStaticResourceDesriptorHandles(StaticResCache, ShaderResLayout, m_ShaderResourceCache);
    }

#ifdef DILIGENT_DEBUG
    m_ShaderResourceCache.DbgVerifyBoundDynamicCBsCounter();
#endif

    m_bStaticResourcesInitialized = true;
}

} // namespace Diligent
