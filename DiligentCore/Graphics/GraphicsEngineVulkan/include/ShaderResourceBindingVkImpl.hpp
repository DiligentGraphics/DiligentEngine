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
/// Declaration of Diligent::ShaderResourceBindingVkImpl class

#include "ShaderResourceBindingVk.h"
#include "RenderDeviceVk.h"
#include "ShaderResourceBindingBase.hpp"
#include "ShaderBase.hpp"
#include "ShaderResourceCacheVk.hpp"
#include "ShaderVariableVk.hpp"

namespace Diligent
{

class PipelineStateVkImpl;

/// Implementation of the Diligent::IShaderResourceBindingVk interface
// sizeof(ShaderResourceBindingVkImpl) == 72 (x64, msvc, Release)
class ShaderResourceBindingVkImpl final : public ShaderResourceBindingBase<IShaderResourceBindingVk, PipelineStateVkImpl>
{
public:
    using TBase = ShaderResourceBindingBase<IShaderResourceBindingVk, PipelineStateVkImpl>;

    ShaderResourceBindingVkImpl(IReferenceCounters*  pRefCounters,
                                PipelineStateVkImpl* pPSO,
                                bool                 IsPSOInternal);
    ~ShaderResourceBindingVkImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IShaderResourceBinding::BindResources() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE BindResources(Uint32 ShaderFlags, IResourceMapping* pResMapping, Uint32 Flags) override final;

    /// Implementation of IShaderResourceBinding::GetVariableByName() in Vulkan backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetVariableByName(SHADER_TYPE ShaderType, const char* Name) override final;

    /// Implementation of IShaderResourceBinding::GetVariableCount() in Vulkan backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetVariableCount(SHADER_TYPE ShaderType) const override final;

    /// Implementation of IShaderResourceBinding::GetVariableByIndex() in Vulkan backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index) override final;

    /// Implementation of IShaderResourceBinding::InitializeStaticResources() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE InitializeStaticResources(const IPipelineState* pPipelineState) override final;

    ShaderResourceCacheVk& GetResourceCache() { return m_ShaderResourceCache; }

    bool StaticResourcesInitialized() const { return m_bStaticResourcesInitialized; }

private:
    void Destruct();

    ShaderResourceCacheVk    m_ShaderResourceCache;
    ShaderVariableManagerVk* m_pShaderVarMgrs = nullptr;

    // Resource layout index in m_ShaderResourceCache array for every shader stage,
    // indexed by the shader type pipeline index (returned by GetShaderTypePipelineIndex)
    std::array<Int8, MAX_SHADERS_IN_PIPELINE> m_ResourceLayoutIndex = {-1, -1, -1, -1, -1};

    bool  m_bStaticResourcesInitialized = false;
    Uint8 m_NumShaders                  = 0;
};

} // namespace Diligent
