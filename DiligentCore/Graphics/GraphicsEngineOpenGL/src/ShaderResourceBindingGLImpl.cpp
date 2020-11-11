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
#include "ShaderResourceBindingGLImpl.hpp"
#include "PipelineStateGLImpl.hpp"
#include "ShaderGLImpl.hpp"
#include "FixedBlockMemoryAllocator.hpp"

namespace Diligent
{

ShaderResourceBindingGLImpl::ShaderResourceBindingGLImpl(IReferenceCounters*  pRefCounters,
                                                         PipelineStateGLImpl* pPSO,
                                                         GLProgramResources*  ProgramResources,
                                                         Uint32               NumPrograms) :
    // clang-format off
    TBase
    {
        pRefCounters,
        pPSO
    },
    m_ResourceLayout{*this}
// clang-format on
{
    pPSO->InitializeSRBResourceCache(m_ResourceCache);

    // Copy only mutable and dynamic variables from master resource layout
    const SHADER_RESOURCE_VARIABLE_TYPE SRBVarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};

    const auto& ResourceLayout = pPSO->GetDesc().ResourceLayout;
    m_ResourceLayout.Initialize(ProgramResources, NumPrograms, pPSO->GetDesc().PipelineType, ResourceLayout, SRBVarTypes, _countof(SRBVarTypes), &m_ResourceCache);
}

ShaderResourceBindingGLImpl::~ShaderResourceBindingGLImpl()
{
    m_ResourceCache.Destroy(GetRawAllocator());
}

IMPLEMENT_QUERY_INTERFACE(ShaderResourceBindingGLImpl, IID_ShaderResourceBindingGL, TBase)

void ShaderResourceBindingGLImpl::BindResources(Uint32 ShaderFlags, IResourceMapping* pResMapping, Uint32 Flags)
{
    m_ResourceLayout.BindResources(static_cast<SHADER_TYPE>(ShaderFlags), pResMapping, Flags, m_ResourceCache);
}

IShaderResourceVariable* ShaderResourceBindingGLImpl::GetVariableByName(SHADER_TYPE ShaderType, const char* Name)
{
    if (!IsConsistentShaderType(ShaderType, m_pPSO->GetDesc().PipelineType))
    {
        LOG_WARNING_MESSAGE("Unable to find mutable/dynamic variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " as the stage is invalid for ", GetPipelineTypeString(m_pPSO->GetDesc().PipelineType), " pipeline '", m_pPSO->GetDesc().Name, "'");
        return nullptr;
    }

    return m_ResourceLayout.GetShaderVariable(ShaderType, Name);
}

Uint32 ShaderResourceBindingGLImpl::GetVariableCount(SHADER_TYPE ShaderType) const
{
    if (!IsConsistentShaderType(ShaderType, m_pPSO->GetDesc().PipelineType))
    {
        LOG_WARNING_MESSAGE("Unable to get the number of mutable/dynamic variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " as the stage is invalid for ", GetPipelineTypeString(m_pPSO->GetDesc().PipelineType), " pipeline '", m_pPSO->GetDesc().Name, "'");
        return 0;
    }

    return m_ResourceLayout.GetNumVariables(ShaderType);
}

IShaderResourceVariable* ShaderResourceBindingGLImpl::GetVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    if (!IsConsistentShaderType(ShaderType, m_pPSO->GetDesc().PipelineType))
    {
        LOG_WARNING_MESSAGE("Unable to get mutable/dynamic variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " as the stage is invalid for ", GetPipelineTypeString(m_pPSO->GetDesc().PipelineType), " pipeline '", m_pPSO->GetDesc().Name, "'");
        return nullptr;
    }

    return m_ResourceLayout.GetShaderVariable(ShaderType, Index);
}

const GLProgramResourceCache& ShaderResourceBindingGLImpl::GetResourceCache(PipelineStateGLImpl* pdbgPSO)
{
#ifdef DILIGENT_DEBUG
    if (pdbgPSO->IsIncompatibleWith(GetPipelineState()))
    {
        LOG_ERROR("Shader resource binding is incompatible with the currently bound pipeline state.");
    }
#endif
    return m_ResourceCache;
}

void ShaderResourceBindingGLImpl::InitializeStaticResources(const IPipelineState* pPipelineState)
{
    if (m_bIsStaticResourcesBound)
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
        DEV_CHECK_ERR(pPipelineState->IsCompatibleWith(GetPipelineState()), "The pipeline state is not compatible with this SRB");
    }

    const auto* pPSOGL          = ValidatedCast<const PipelineStateGLImpl>(pPipelineState);
    const auto& StaticResLayout = pPSOGL->GetStaticResourceLayout();

#ifdef DILIGENT_DEVELOPMENT
    if (!StaticResLayout.dvpVerifyBindings(pPSOGL->GetStaticResourceCache()))
    {
        LOG_ERROR_MESSAGE("Static resources in SRB of PSO '", pPSOGL->GetDesc().Name,
                          "' will not be successfully initialized because not all static resource bindings in shader '", pPSOGL->GetDesc().Name,
                          "' are valid. Please make sure you bind all static resources to PSO before calling InitializeStaticResources() "
                          "directly or indirectly by passing InitStaticResources=true to CreateShaderResourceBinding() method.");
    }
#endif

    StaticResLayout.CopyResources(m_ResourceCache);
    m_bIsStaticResourcesBound = true;
}

} // namespace Diligent
