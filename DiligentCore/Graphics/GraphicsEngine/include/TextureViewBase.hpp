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
/// Implementation of the Diligent::TextureViewBase template class

#include "TextureView.h"
#include "DeviceObjectBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

/// Template class implementing base functionality for a texture view interface

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::ITextureViewD3D11, Diligent::ITextureViewD3D12,
///                          Diligent::ITextureViewGL or Diligent::ITextureViewVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
///                                (Diligent::RenderDeviceD3D11Impl, Diligent::RenderDeviceD3D12Impl,
///                                 Diligent::RenderDeviceGLImpl, or Diligent::RenderDeviceVkImpl)
template <class BaseInterface, class RenderDeviceImplType>
class TextureViewBase : public DeviceObjectBase<BaseInterface, RenderDeviceImplType, TextureViewDesc>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, TextureViewDesc>;


    /// \param pRefCounters - reference counters object that controls the lifetime of this texture view.
    /// \param pDevice - pointer to the render device.
    /// \param ViewDesc - texture view description.
    /// \param pTexture - pointer to the texture that the view is to be created for.
    /// \param bIsDefaultView - flag indicating if the view is default view, and is thus
    ///						    part of the texture object. In this case the view will attach
    ///							to the texture's reference counters.
    TextureViewBase(IReferenceCounters*    pRefCounters,
                    RenderDeviceImplType*  pDevice,
                    const TextureViewDesc& ViewDesc,
                    ITexture*              pTexture,
                    bool                   bIsDefaultView) :
        // Default views are created as part of the texture, so we cannot not keep strong
        // reference to the texture to avoid cyclic links. Instead, we will attach to the
        // reference counters of the texture.
        TDeviceObjectBase(pRefCounters, pDevice, ViewDesc),
        m_pTexture(pTexture),
        // For non-default view, we will keep strong reference to texture
        m_spTexture(bIsDefaultView ? nullptr : pTexture)
    {}

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_TextureView, TDeviceObjectBase)

    /// Implementation of ITextureView::SetSampler()
    virtual void DILIGENT_CALL_TYPE SetSampler(ISampler* pSampler) override final
    {
#ifdef DILIGENT_DEVELOPMENT
        if (this->m_Desc.ViewType != TEXTURE_VIEW_SHADER_RESOURCE)
            LOG_ERROR("Texture view \"", this->m_Desc.Name, "\": a sampler can be attached to a shader resource view only. The view type is ", GetTexViewTypeLiteralName(this->m_Desc.ViewType));
#endif
        m_pSampler = pSampler;
    }

    /// Implementation of ITextureView::GetSampler()
    virtual ISampler* DILIGENT_CALL_TYPE GetSampler() override final
    {
        return m_pSampler;
    }

    const ISampler* GetSampler() const
    {
        return m_pSampler;
    }

    /// Implementation of ITextureView::GetTexture()
    virtual ITexture* DILIGENT_CALL_TYPE GetTexture() override final
    {
        return m_pTexture;
    }

    const ITexture* DILIGENT_CALL_TYPE GetTexture() const
    {
        return m_pTexture;
    }

    template <typename TextureType>
    TextureType* GetTexture()
    {
        return ValidatedCast<TextureType>(m_pTexture);
    }

    template <typename TextureType>
    TextureType* GetTexture() const
    {
        return ValidatedCast<TextureType>(m_pTexture);
    }

protected:
    /// Strong reference to the sampler
    RefCntAutoPtr<ISampler> m_pSampler;

    /// Raw pointer to the texture
    ITexture* const m_pTexture;

    /// Strong reference to the texture. Used for non-default views
    /// to keep the texture alive
    RefCntAutoPtr<ITexture> m_spTexture;
};

} // namespace Diligent
