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
/// Declaration of Diligent::SamplerD3D11Impl class

#include "SamplerD3D11.h"
#include "RenderDeviceD3D11.h"
#include "SamplerBase.hpp"
#include "RenderDeviceD3D11Impl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Sampler implementation in Direct3D11 backend.
class SamplerD3D11Impl final : public SamplerBase<ISamplerD3D11, RenderDeviceD3D11Impl>
{
public:
    using TSamplerBase = SamplerBase<ISamplerD3D11, RenderDeviceD3D11Impl>;

    SamplerD3D11Impl(IReferenceCounters*          pRefCounters,
                     class RenderDeviceD3D11Impl* pRenderDeviceD3D11,
                     const SamplerDesc&           SamplerDesc);
    ~SamplerD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ISamplerD3D11::ISamplerD3D11() method.
    virtual ID3D11SamplerState* DILIGENT_CALL_TYPE GetD3D11SamplerState() override final { return m_pd3dSampler; }

private:
    /// D3D11 sampler
    CComPtr<ID3D11SamplerState> m_pd3dSampler;
};

} // namespace Diligent
