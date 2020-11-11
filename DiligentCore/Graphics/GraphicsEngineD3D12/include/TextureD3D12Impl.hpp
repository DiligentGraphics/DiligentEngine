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
/// Declaration of Diligent::TextureD3D12Impl class

#include "TextureD3D12.h"
#include "RenderDeviceD3D12.h"
#include "TextureBase.hpp"
#include "TextureViewD3D12Impl.hpp"
#include "D3D12ResourceBase.hpp"
#include "RenderDeviceD3D12Impl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Implementation of a texture object in Direct3D12 backend.
class TextureD3D12Impl final : public TextureBase<ITextureD3D12, RenderDeviceD3D12Impl, TextureViewD3D12Impl, FixedBlockMemoryAllocator>, public D3D12ResourceBase
{
public:
    using TTextureBase = TextureBase<ITextureD3D12, RenderDeviceD3D12Impl, TextureViewD3D12Impl, FixedBlockMemoryAllocator>;
    using ViewImplType = TextureViewD3D12Impl;

    // Creates a new D3D12 resource
    TextureD3D12Impl(IReferenceCounters*        pRefCounters,
                     FixedBlockMemoryAllocator& TexViewObjAllocator,
                     RenderDeviceD3D12Impl*     pDeviceD3D12,
                     const TextureDesc&         TexDesc,
                     const TextureData*         pInitData = nullptr);

    // Attaches to an existing D3D12 resource
    TextureD3D12Impl(IReferenceCounters*          pRefCounters,
                     FixedBlockMemoryAllocator&   TexViewObjAllocator,
                     class RenderDeviceD3D12Impl* pDeviceD3D12,
                     const TextureDesc&           TexDesc,
                     RESOURCE_STATE               InitialState,
                     ID3D12Resource*              pTexture);
    ~TextureD3D12Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ITextureD3D12::GetD3D12Texture().
    virtual ID3D12Resource* DILIGENT_CALL_TYPE GetD3D12Texture() override final { return GetD3D12Resource(); }

    /// Implementation of ITexture::GetNativeHandle() in Direct3D12 backend.
    virtual void* DILIGENT_CALL_TYPE GetNativeHandle() override final { return GetD3D12Texture(); }

    /// Implementation of ITextureD3D12::SetD3D12ResourceState().
    virtual void DILIGENT_CALL_TYPE SetD3D12ResourceState(D3D12_RESOURCE_STATES state) override final;

    /// Implementation of ITextureD3D12::GetD3D12ResourceState().
    virtual D3D12_RESOURCE_STATES DILIGENT_CALL_TYPE GetD3D12ResourceState() const override final;

    D3D12_RESOURCE_DESC GetD3D12TextureDesc() const;

    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& GetStagingFootprint(Uint32 Subresource)
    {
        VERIFY_EXPR(m_StagingFootprints != nullptr);
        VERIFY_EXPR(Subresource <= (Uint32{m_Desc.MipLevels} * (m_Desc.Type == RESOURCE_DIM_TEX_3D ? 1 : Uint32{m_Desc.ArraySize})));
        return m_StagingFootprints[Subresource];
    }

protected:
    void CreateViewInternal(const struct TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView) override final;
    //void PrepareD3D12InitData(const TextureData &InitData, Uint32 NumSubresources, std::vector<D3D12_SUBRESOURCE_DATA> &D3D12InitData);

    void CreateSRV(TextureViewDesc& SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle);
    void CreateRTV(TextureViewDesc& RTVDesc, D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle);
    void CreateDSV(TextureViewDesc& DSVDesc, D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle);
    void CreateUAV(TextureViewDesc& UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* m_StagingFootprints = nullptr;

    friend class RenderDeviceD3D12Impl;
};

} // namespace Diligent
