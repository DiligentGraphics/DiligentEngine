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
#include "ShaderResourceBindingVkImpl.hpp"
#include "PipelineStateVkImpl.hpp"
#include "ShaderVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "LinearAllocator.hpp"

namespace Diligent
{

ShaderResourceBindingVkImpl::ShaderResourceBindingVkImpl(IReferenceCounters*  pRefCounters,
                                                         PipelineStateVkImpl* pPSO,
                                                         bool                 IsPSOInternal) :
    // clang-format off
    TBase
    {
        pRefCounters,
        pPSO,
        IsPSOInternal
    },
    m_ShaderResourceCache{ShaderResourceCacheVk::DbgCacheContentType::SRBResources}
// clang-format on
{
    try
    {
        m_ResourceLayoutIndex.fill(-1);

        m_NumShaders = static_cast<decltype(m_NumShaders)>(pPSO->GetNumShaderStages());

        LinearAllocator MemPool{GetRawAllocator()};
        MemPool.AddSpace<ShaderVariableManagerVk>(m_NumShaders);
        MemPool.Reserve();
        m_pShaderVarMgrs = MemPool.ConstructArray<ShaderVariableManagerVk>(m_NumShaders, std::ref(*this), std::ref(m_ShaderResourceCache));

        // The memory is now owned by ShaderResourceBindingVkImpl and will be freed by Destruct().
        auto* Ptr = MemPool.ReleaseOwnership();
        VERIFY_EXPR(Ptr == m_pShaderVarMgrs);
        (void)Ptr;

        // It is important to construct all objects before initializing them because if an exception is thrown,
        // destructors will be called for all objects

        auto* pRenderDeviceVkImpl = pPSO->GetDevice();
        // This will only allocate memory and initialize descriptor sets in the resource cache
        // Resources will be initialized by InitializeResourceMemoryInCache()
        auto& ResourceCacheDataAllocator = pPSO->GetSRBMemoryAllocator().GetResourceCacheDataAllocator(0);
        pPSO->GetPipelineLayout().InitResourceCache(pRenderDeviceVkImpl, m_ShaderResourceCache, ResourceCacheDataAllocator, pPSO->GetDesc().Name);

        for (Uint32 s = 0; s < m_NumShaders; ++s)
        {
            auto ShaderInd = GetShaderTypePipelineIndex(pPSO->GetShaderStageType(s), pPSO->GetDesc().PipelineType);

            m_ResourceLayoutIndex[ShaderInd] = static_cast<Int8>(s);

            auto& VarDataAllocator = pPSO->GetSRBMemoryAllocator().GetShaderVariableDataAllocator(s);

            const auto& SrcLayout = pPSO->GetShaderResLayout(s);
            // Use source layout to initialize resource memory in the cache
            SrcLayout.InitializeResourceMemoryInCache(m_ShaderResourceCache);

            // Create shader variable manager in place
            // Initialize vars manager to reference mutable and dynamic variables
            // Note that the cache has space for all variable types
            const SHADER_RESOURCE_VARIABLE_TYPE VarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
            m_pShaderVarMgrs[s].Initialize(SrcLayout, VarDataAllocator, VarTypes, _countof(VarTypes));
        }
#ifdef DILIGENT_DEBUG
        m_ShaderResourceCache.DbgVerifyResourceInitialization();
#endif
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

ShaderResourceBindingVkImpl::~ShaderResourceBindingVkImpl()
{
    Destruct();
}

void ShaderResourceBindingVkImpl::Destruct()
{
    if (m_pShaderVarMgrs != nullptr)
    {
        auto& SRBMemAllocator = m_pPSO->GetSRBMemoryAllocator();
        for (Uint32 s = 0; s < m_NumShaders; ++s)
        {
            auto& VarDataAllocator = SRBMemAllocator.GetShaderVariableDataAllocator(s);
            m_pShaderVarMgrs[s].DestroyVariables(VarDataAllocator);
            m_pShaderVarMgrs[s].~ShaderVariableManagerVk();
        }

        GetRawAllocator().Free(m_pShaderVarMgrs);
    }
}

IMPLEMENT_QUERY_INTERFACE(ShaderResourceBindingVkImpl, IID_ShaderResourceBindingVk, TBase)

void ShaderResourceBindingVkImpl::BindResources(Uint32 ShaderFlags, IResourceMapping* pResMapping, Uint32 Flags)
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

IShaderResourceVariable* ShaderResourceBindingVkImpl::GetVariableByName(SHADER_TYPE ShaderType, const char* Name)
{
    auto ResLayoutInd = GetVariableByNameHelper(ShaderType, Name, m_ResourceLayoutIndex);
    if (ResLayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(ResLayoutInd) < Uint32{m_NumShaders});
    return m_pShaderVarMgrs[ResLayoutInd].GetVariable(Name);
}

Uint32 ShaderResourceBindingVkImpl::GetVariableCount(SHADER_TYPE ShaderType) const
{
    auto ResLayoutInd = GetVariableCountHelper(ShaderType, m_ResourceLayoutIndex);
    if (ResLayoutInd < 0)
        return 0;

    VERIFY_EXPR(static_cast<Uint32>(ResLayoutInd) < Uint32{m_NumShaders});
    return m_pShaderVarMgrs[ResLayoutInd].GetVariableCount();
}

IShaderResourceVariable* ShaderResourceBindingVkImpl::GetVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    auto ResLayoutInd = GetVariableByIndexHelper(ShaderType, Index, m_ResourceLayoutIndex);
    if (ResLayoutInd < 0)
        return nullptr;

    VERIFY_EXPR(static_cast<Uint32>(ResLayoutInd) < Uint32{m_NumShaders});
    return m_pShaderVarMgrs[ResLayoutInd].GetVariable(Index);
}

void ShaderResourceBindingVkImpl::InitializeStaticResources(const IPipelineState* pPipelineState)
{
    if (StaticResourcesInitialized())
    {
        LOG_WARNING_MESSAGE("Static resources have already been initialized in this shader resource binding object. The operation will be ignored.");
        return;
    }

    if (pPipelineState == nullptr)
    {
        pPipelineState = GetPipelineState();
    }
    else
    {
        DEV_CHECK_ERR(pPipelineState->IsCompatibleWith(GetPipelineState()), "The pipeline state '", pPipelineState->GetDesc().Name,
                      "' is not compatible with the pipeline state '", m_pPSO->GetDesc().Name,
                      "' this SRB was created from and cannot be used to initialize static resources.");
    }

    auto* pPSOVK = ValidatedCast<const PipelineStateVkImpl>(pPipelineState);
    pPSOVK->InitializeStaticSRBResources(m_ShaderResourceCache);
    m_bStaticResourcesInitialized = true;
}

} // namespace Diligent
