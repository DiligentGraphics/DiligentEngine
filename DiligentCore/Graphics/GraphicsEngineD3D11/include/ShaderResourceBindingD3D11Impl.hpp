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
/// Declaration of Diligent::ShaderResourceBindingD3D11Impl class

#include <array>

#include "ShaderResourceBindingD3D11.h"
#include "RenderDeviceD3D11.h"
#include "ShaderResourceBindingBase.hpp"
#include "ShaderResourceCacheD3D11.hpp"
#include "ShaderResourceLayoutD3D11.hpp"
#include "STDAllocator.hpp"

namespace Diligent
{

class PipelineStateD3D11Impl;

/// Implementation of shader resource binding object in Direct3D11 backend.
class ShaderResourceBindingD3D11Impl final : public ShaderResourceBindingBase<IShaderResourceBindingD3D11, PipelineStateD3D11Impl>
{
public:
    using TBase = ShaderResourceBindingBase<IShaderResourceBindingD3D11, PipelineStateD3D11Impl>;

    ShaderResourceBindingD3D11Impl(IReferenceCounters*     pRefCounters,
                                   PipelineStateD3D11Impl* pPSO,
                                   bool                    IsInternal);
    ~ShaderResourceBindingD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IShaderResourceBinding::BindResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE BindResources(Uint32            ShaderFlags,
                                                  IResourceMapping* pResMapping,
                                                  Uint32            Flags) override final;

    /// Implementation of IShaderResourceBinding::GetVariableByName() in Direct3D11 backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetVariableByName(SHADER_TYPE ShaderType, const char* Name) override final;

    /// Implementation of IShaderResourceBinding::GetVariableCount() in Direct3D11 backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetVariableCount(SHADER_TYPE ShaderType) const override final;

    /// Implementation of IShaderResourceBinding::GetVariableByIndex() in Direct3D11 backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index) override final;

    /// Implementation of IShaderResourceBinding::InitializeStaticResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE InitializeStaticResources(const IPipelineState* pPipelineState) override final;

    ShaderResourceCacheD3D11& GetResourceCache(Uint32 Ind)
    {
        VERIFY_EXPR(Ind < m_NumActiveShaders);
        return m_pBoundResourceCaches[Ind];
    }

    ShaderResourceLayoutD3D11& GetResourceLayout(Uint32 Ind)
    {
        VERIFY_EXPR(Ind < m_NumActiveShaders);
        return m_pResourceLayouts[Ind];
    }

    inline bool IsStaticResourcesBound() const { return m_bIsStaticResourcesBound; }

    Uint32 GetNumActiveShaders() const
    {
        return Uint32{m_NumActiveShaders};
    }

    SHADER_TYPE GetActiveShaderType(Uint32 s) const
    {
        VERIFY_EXPR(s < m_NumActiveShaders);
        return m_ShaderTypes[s];
    }

private:
    void Destruct();

    // The caches are indexed by the shader order in the PSO, not shader index
    ShaderResourceCacheD3D11*  m_pBoundResourceCaches = nullptr;
    ShaderResourceLayoutD3D11* m_pResourceLayouts     = nullptr;

    std::array<SHADER_TYPE, MAX_SHADERS_IN_PIPELINE> m_ShaderTypes = {};

    // Resource layout index in m_pResourceLayouts array for every shader stage,
    // indexed by the shader type pipeline index (returned by GetShaderTypePipelineIndex)
    std::array<Int8, MAX_SHADERS_IN_PIPELINE> m_ResourceLayoutIndex = {-1, -1, -1, -1, -1};

    Uint8 m_NumActiveShaders = 0;

    bool m_bIsStaticResourcesBound = false;
};

} // namespace Diligent
