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
#include "TextureViewD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"

namespace Diligent
{

TextureViewD3D12Impl::TextureViewD3D12Impl(IReferenceCounters*        pRefCounters,
                                           RenderDeviceD3D12Impl*     pDevice,
                                           const TextureViewDesc&     ViewDesc,
                                           ITexture*                  pTexture,
                                           DescriptorHeapAllocation&& Descriptor,
                                           DescriptorHeapAllocation&& TexArraySRVDescriptor,
                                           DescriptorHeapAllocation&& MipLevelUAVDescriptors,
                                           bool                       bIsDefaultView) :
    // clang-format off
    TTextureViewBase
    {
        pRefCounters,
        pDevice,
        ViewDesc,
        pTexture,
        bIsDefaultView
    },
    m_Descriptor{std::move(Descriptor)}
// clang-format on
{
    if (!TexArraySRVDescriptor.IsNull() && !MipLevelUAVDescriptors.IsNull())
    {
        m_MipGenerationDescriptors = ALLOCATE(GetRawAllocator(), "Raw memory for DescriptorHeapAllocation", DescriptorHeapAllocation, 2);
        new (&m_MipGenerationDescriptors[0]) DescriptorHeapAllocation(std::move(TexArraySRVDescriptor));
        new (&m_MipGenerationDescriptors[1]) DescriptorHeapAllocation(std::move(MipLevelUAVDescriptors));
    }
}

TextureViewD3D12Impl::~TextureViewD3D12Impl()
{
    if (m_MipGenerationDescriptors != nullptr)
    {
        for (Uint32 i = 0; i < 2; ++i)
        {
            m_MipGenerationDescriptors[i].~DescriptorHeapAllocation();
        }
        FREE(GetRawAllocator(), m_MipGenerationDescriptors);
    }
}

IMPLEMENT_QUERY_INTERFACE(TextureViewD3D12Impl, IID_TextureViewD3D12, TTextureViewBase)

} // namespace Diligent
