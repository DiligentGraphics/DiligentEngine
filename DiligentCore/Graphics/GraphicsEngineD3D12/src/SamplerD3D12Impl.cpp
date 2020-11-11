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
#include "SamplerD3D12Impl.hpp"
#include "RenderDeviceD3D12Impl.hpp"
#include "D3D12TypeConversions.hpp"

namespace Diligent
{

SamplerD3D12Impl::SamplerD3D12Impl(IReferenceCounters*    pRefCounters,
                                   RenderDeviceD3D12Impl* pRenderDeviceD3D12,
                                   const SamplerDesc&     SamplerDesc) :
    TSamplerBase{pRefCounters, pRenderDeviceD3D12, SamplerDesc}
{
    auto* pD3D12Device = pRenderDeviceD3D12->GetD3D12Device();

    D3D12_SAMPLER_DESC D3D12SamplerDesc =
        {
            FilterTypeToD3D12Filter(SamplerDesc.MinFilter, SamplerDesc.MagFilter, SamplerDesc.MipFilter),
            TexAddressModeToD3D12AddressMode(SamplerDesc.AddressU),
            TexAddressModeToD3D12AddressMode(SamplerDesc.AddressV),
            TexAddressModeToD3D12AddressMode(SamplerDesc.AddressW),
            SamplerDesc.MipLODBias,
            SamplerDesc.MaxAnisotropy,
            ComparisonFuncToD3D12ComparisonFunc(SamplerDesc.ComparisonFunc),
            {SamplerDesc.BorderColor[0], SamplerDesc.BorderColor[1], SamplerDesc.BorderColor[2], SamplerDesc.BorderColor[3]},
            SamplerDesc.MinLOD,
            SamplerDesc.MaxLOD //
        };

    auto CPUDescriptorAlloc = pRenderDeviceD3D12->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_Descriptor            = std::move(CPUDescriptorAlloc);
    pD3D12Device->CreateSampler(&D3D12SamplerDesc, m_Descriptor.GetCpuHandle());
}

SamplerD3D12Impl::~SamplerD3D12Impl()
{
}

IMPLEMENT_QUERY_INTERFACE(SamplerD3D12Impl, IID_SamplerD3D12, TSamplerBase)

} // namespace Diligent
