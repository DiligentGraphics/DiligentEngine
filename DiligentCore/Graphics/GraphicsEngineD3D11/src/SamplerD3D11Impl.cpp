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
#include "SamplerD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "D3D11TypeConversions.hpp"

namespace Diligent
{

SamplerD3D11Impl::SamplerD3D11Impl(IReferenceCounters*    pRefCounters,
                                   RenderDeviceD3D11Impl* pRenderDeviceD3D11,
                                   const SamplerDesc&     SamplerDesc) :
    // clang-format off
    TSamplerBase
    {
        pRefCounters,
        pRenderDeviceD3D11,
        SamplerDesc
    }
// clang-format on
{
    auto*              pd3d11Device = pRenderDeviceD3D11->GetD3D11Device();
    D3D11_SAMPLER_DESC D3D11SamplerDesc =
        {
            FilterTypeToD3D11Filter(SamplerDesc.MinFilter, SamplerDesc.MagFilter, SamplerDesc.MipFilter),
            TexAddressModeToD3D11AddressMode(SamplerDesc.AddressU),
            TexAddressModeToD3D11AddressMode(SamplerDesc.AddressV),
            TexAddressModeToD3D11AddressMode(SamplerDesc.AddressW),
            SamplerDesc.MipLODBias,
            SamplerDesc.MaxAnisotropy,
            ComparisonFuncToD3D11ComparisonFunc(SamplerDesc.ComparisonFunc),
            {SamplerDesc.BorderColor[0], SamplerDesc.BorderColor[1], SamplerDesc.BorderColor[2], SamplerDesc.BorderColor[3]},
            SamplerDesc.MinLOD,
            SamplerDesc.MaxLOD // clang-format off
        }; // clang-format on

    CHECK_D3D_RESULT_THROW(pd3d11Device->CreateSamplerState(&D3D11SamplerDesc, &m_pd3dSampler),
                           "Failed to create the Direct3D11 sampler");
}

SamplerD3D11Impl::~SamplerD3D11Impl()
{
}

IMPLEMENT_QUERY_INTERFACE(SamplerD3D11Impl, IID_SamplerD3D11, TSamplerBase)

} // namespace Diligent
