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
/// Declaration of Diligent::ShaderVkImpl class

#include "RenderDeviceVk.h"
#include "ShaderVk.h"
#include "ShaderBase.hpp"
#include "SPIRVShaderResources.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

class ResourceMapping;
class FixedBlockMemoryAllocator;

/// Shader object object implementation in Vulkan backend.
class ShaderVkImpl final : public ShaderBase<IShaderVk, RenderDeviceVkImpl>
{
public:
    using TShaderBase = ShaderBase<IShaderVk, RenderDeviceVkImpl>;

    ShaderVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pRenderDeviceVk, const ShaderCreateInfo& CreationAttribs);
    ~ShaderVkImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_ShaderVk, TShaderBase);

    /// Implementation of IShader::GetResourceCount() in Vulkan backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final
    {
        return m_pShaderResources->GetTotalResources();
    }

    /// Implementation of IShader::GetResource() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final;

    /// Implementation of IShaderVk::GetSPIRV().
    virtual const std::vector<uint32_t>& DILIGENT_CALL_TYPE GetSPIRV() const override final
    {
        return m_SPIRV;
    }

    const std::shared_ptr<const SPIRVShaderResources>& GetShaderResources() const { return m_pShaderResources; }

    const char* GetEntryPoint() const { return m_EntryPoint.c_str(); }

private:
    void MapHLSLVertexShaderInputs();

    // SPIRVShaderResources class instance must be referenced through the shared pointer, because
    // it is referenced by ShaderResourceLayoutVk class instances
    std::shared_ptr<const SPIRVShaderResources> m_pShaderResources;

    std::string           m_EntryPoint;
    std::vector<uint32_t> m_SPIRV;
};

} // namespace Diligent
