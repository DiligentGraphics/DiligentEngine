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

#pragma once

/// \file
/// Implementation of the Diligent::ShaderResourceBindingBase template class

#include <array>

#include "ShaderResourceBinding.h"
#include "ObjectBase.hpp"
#include "GraphicsTypes.h"
#include "Constants.h"
#include "RefCntAutoPtr.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

/// Template class implementing base functionality for a shader resource binding

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IShaderResourceBindingGL, Diligent::IShaderResourceBindingD3D11,
///                          Diligent::IShaderResourceBindingD3D12 or Diligent::IShaderResourceBindingVk).
/// \tparam PipelineStateImplType - Type of the pipeline state implementation
///                                 (Diligent::PipelineStateD3D12Impl, Diligent::PipelineStateVkImpl, etc.)
template <class BaseInterface, class PipelineStateImplType>
class ShaderResourceBindingBase : public ObjectBase<BaseInterface>
{
public:
    typedef ObjectBase<BaseInterface> TObjectBase;

    /// \param pRefCounters - reference counters object that controls the lifetime of this SRB.
    /// \param pPSO - pipeline state that this SRB belongs to.
    /// \param IsInternal - flag indicating if the shader resource binding is an internal PSO object and
    ///						must not keep a strong reference to the PSO.
    ShaderResourceBindingBase(IReferenceCounters* pRefCounters, PipelineStateImplType* pPSO, bool IsInternal = false) :
        TObjectBase{pRefCounters},
        m_spPSO{IsInternal ? nullptr : pPSO},
        m_pPSO{pPSO}
    {}

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_ShaderResourceBinding, TObjectBase)

    /// Implementation of IShaderResourceBinding::GetPipelineState().
    virtual IPipelineState* DILIGENT_CALL_TYPE GetPipelineState() override final
    {
        return m_pPSO;
    }

    template <typename PSOType>
    PSOType* GetPipelineState()
    {
        return ValidatedCast<PSOType>(m_pPSO);
    }

    template <typename PSOType>
    PSOType* GetPipelineState() const
    {
        return ValidatedCast<PSOType>(m_pPSO);
    }

protected:
    Int8 GetVariableByNameHelper(SHADER_TYPE ShaderType, const char* Name, const std::array<Int8, MAX_SHADERS_IN_PIPELINE>& ResourceLayoutIndex) const
    {
        const auto PipelineType = m_pPSO->GetDesc().PipelineType;
        if (!IsConsistentShaderType(ShaderType, PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to find mutable/dynamic variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(m_pPSO->GetDesc().PipelineType), " pipeline '", m_pPSO->GetDesc().Name, "'");
            return -1;
        }

        const auto ShaderInd    = GetShaderTypePipelineIndex(ShaderType, PipelineType);
        const auto ResLayoutInd = ResourceLayoutIndex[ShaderInd];
        if (ResLayoutInd < 0)
        {
            LOG_WARNING_MESSAGE("Unable to find mutable/dynamic variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", m_pPSO->GetDesc().Name, "'");
        }

        return ResLayoutInd;
    }

    Int8 GetVariableCountHelper(SHADER_TYPE ShaderType, const std::array<Int8, MAX_SHADERS_IN_PIPELINE>& ResourceLayoutIndex) const
    {
        const auto PipelineType = m_pPSO->GetDesc().PipelineType;
        if (!IsConsistentShaderType(ShaderType, PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get the number of mutable/dynamic variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(m_pPSO->GetDesc().PipelineType), " pipeline '", m_pPSO->GetDesc().Name, "'");
            return -1;
        }

        const auto ShaderInd    = GetShaderTypePipelineIndex(ShaderType, PipelineType);
        const auto ResLayoutInd = ResourceLayoutIndex[ShaderInd];
        if (ResLayoutInd < 0)
        {
            LOG_WARNING_MESSAGE("Unable to get the number of mutable/dynamic variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", m_pPSO->GetDesc().Name, "'");
        }

        return ResLayoutInd;
    }

    Int8 GetVariableByIndexHelper(SHADER_TYPE ShaderType, Uint32 Index, const std::array<Int8, MAX_SHADERS_IN_PIPELINE>& ResourceLayoutIndex)
    {
        const auto PipelineType = m_pPSO->GetDesc().PipelineType;
        if (!IsConsistentShaderType(ShaderType, PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get mutable/dynamic variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(m_pPSO->GetDesc().PipelineType), " pipeline '", m_pPSO->GetDesc().Name, "'");
            return -1;
        }

        const auto ShaderInd    = GetShaderTypePipelineIndex(ShaderType, PipelineType);
        const auto ResLayoutInd = ResourceLayoutIndex[ShaderInd];
        if (ResLayoutInd < 0)
        {
            LOG_WARNING_MESSAGE("Unable to get mutable/dynamic variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", m_pPSO->GetDesc().Name, "'");
        }

        return ResLayoutInd;
    }

    /// Strong reference to PSO. We must use strong reference, because
    /// shader resource binding uses PSO's memory allocator to allocate
    /// memory for shader resource cache.
    RefCntAutoPtr<PipelineStateImplType> m_spPSO;
    PipelineStateImplType* const         m_pPSO;
};

} // namespace Diligent
