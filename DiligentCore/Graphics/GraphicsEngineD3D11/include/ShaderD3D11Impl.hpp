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
/// Declaration of Diligent::ShaderD3D11Impl class

#include "ShaderD3D11.h"
#include "RenderDeviceD3D11.h"
#include "ShaderBase.hpp"
#include "ShaderD3DBase.hpp"
#include "ShaderResourceLayoutD3D11.hpp"
#include "ShaderResourceCacheD3D11.hpp"
#include "EngineD3D11Defines.h"
#include "ShaderResourcesD3D11.hpp"
#include "RenderDeviceD3D11Impl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;
class ResourceMapping;

/// Shader implementation in Direct3D11 backend.
class ShaderD3D11Impl final : public ShaderBase<IShaderD3D11, RenderDeviceD3D11Impl>, public ShaderD3DBase
{
public:
    using TShaderBase = ShaderBase<IShaderD3D11, RenderDeviceD3D11Impl>;

    ShaderD3D11Impl(IReferenceCounters*          pRefCounters,
                    class RenderDeviceD3D11Impl* pRenderDeviceD3D11,
                    const ShaderCreateInfo&      ShaderCI);
    ~ShaderD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IShader::GetResourceCount() in Direct3D11 backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final
    {
        return m_pShaderResources->GetTotalResources();
    }

    /// Implementation of IShader::GetResource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final
    {
        ResourceDesc = m_pShaderResources->GetHLSLShaderResourceDesc(Index);
    }

    /// Implementation of IShaderD3D::GetHLSLResource() method.
    virtual void DILIGENT_CALL_TYPE GetHLSLResource(Uint32 Index, HLSLShaderResourceDesc& ResourceDesc) const override final
    {
        ResourceDesc = m_pShaderResources->GetHLSLShaderResourceDesc(Index);
    }

    /// Implementation of IShaderD3D11::GetD3D11Shader() method.
    virtual ID3D11DeviceChild* DILIGENT_CALL_TYPE GetD3D11Shader() override final
    {
        return m_pShader;
    }

    ID3DBlob* GetBytecode() { return m_pShaderByteCode; }

    const std::shared_ptr<const ShaderResourcesD3D11>& GetD3D11Resources() const { return m_pShaderResources; }

private:
    /// D3D11 shader
    CComPtr<ID3D11DeviceChild> m_pShader;

    // ShaderResources class instance must be referenced through the shared pointer, because
    // it is referenced by ShaderResourceLayoutD3D11 class instances
    std::shared_ptr<const ShaderResourcesD3D11> m_pShaderResources;
};

} // namespace Diligent
