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
/// Implementation of the Diligent::FramebufferBase template class

#include "Framebuffer.h"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"
#include "TextureView.h"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

void ValidateFramebufferDesc(const FramebufferDesc& Desc);

/// Template class implementing base functionality for the framebuffer object.

/// \tparam BaseInterface - base interface that this class will inheret
///                         (e.g. Diligent::IFramebufferVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
///                                (Diligent::RenderDeviceD3D11Impl, Diligent::RenderDeviceD3D12Impl,
///                                 Diligent::RenderDeviceGLImpl, or Diligent::RenderDeviceVkImpl)
template <class BaseInterface, class RenderDeviceImplType>
class FramebufferBase : public DeviceObjectBase<BaseInterface, RenderDeviceImplType, FramebufferDesc>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, FramebufferDesc>;

    /// \param pRefCounters      - reference counters object that controls the lifetime of this framebuffer pass.
    /// \param pDevice           - pointer to the device.
    /// \param Desc              - Framebuffer description.
    /// \param bIsDeviceInternal - flag indicating if the Framebuffer is an internal device object and
    ///							   must not keep a strong reference to the device.
    FramebufferBase(IReferenceCounters*    pRefCounters,
                    RenderDeviceImplType*  pDevice,
                    const FramebufferDesc& Desc,
                    bool                   bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal},
        m_pRenderPass{Desc.pRenderPass}
    {
        ValidateFramebufferDesc(Desc);

        if (this->m_Desc.AttachmentCount > 0)
        {
            m_ppAttachments =
                ALLOCATE(GetRawAllocator(), "Memory for framebuffer attachment array", ITextureView*, this->m_Desc.AttachmentCount);
            this->m_Desc.ppAttachments = m_ppAttachments;
            for (Uint32 i = 0; i < this->m_Desc.AttachmentCount; ++i)
            {
                if (Desc.ppAttachments[i] == nullptr)
                    continue;

                m_ppAttachments[i] = Desc.ppAttachments[i];
                m_ppAttachments[i]->AddRef();

                if (this->m_Desc.Width == 0 || this->m_Desc.Height == 0 || this->m_Desc.NumArraySlices == 0)
                {
                    const auto& ViewDesc = m_ppAttachments[i]->GetDesc();
                    const auto& TexDesc  = m_ppAttachments[i]->GetTexture()->GetDesc();

                    auto MipLevelProps = GetMipLevelProperties(TexDesc, ViewDesc.MostDetailedMip);
                    if (this->m_Desc.Width == 0)
                        this->m_Desc.Width = MipLevelProps.LogicalWidth;
                    if (this->m_Desc.Height == 0)
                        this->m_Desc.Height = MipLevelProps.LogicalHeight;
                    if (this->m_Desc.NumArraySlices == 0)
                        this->m_Desc.NumArraySlices = ViewDesc.NumArraySlices;
                }
            }
        }

        // It is legal for a subpass to use no color or depth/stencil attachments, either because it has no attachment
        // references or because all of them are VK_ATTACHMENT_UNUSED. This kind of subpass can use shader side effects
        // such as image stores and atomics to produce an output. In this case, the subpass continues to use the width,
        // height, and layers of the framebuffer to define the dimensions of the rendering area.
        if (this->m_Desc.Width == 0)
            LOG_ERROR_AND_THROW("The framebuffer width is zero and can't be automatically determined as there are no non-null attachments");
        if (this->m_Desc.Height == 0)
            LOG_ERROR_AND_THROW("The framebuffer height is zero and can't be automatically determined as there are no non-null attachments");
        if (this->m_Desc.NumArraySlices == 0)
            LOG_ERROR_AND_THROW("The framebuffer array slice count is zero and can't be automatically determined as there are no non-null attachments");

        Desc.pRenderPass->AddRef();
    }

    ~FramebufferBase()
    {
        if (this->m_Desc.AttachmentCount > 0)
        {
            VERIFY_EXPR(m_ppAttachments != nullptr);
            for (Uint32 i = 0; i < this->m_Desc.AttachmentCount; ++i)
            {
                m_ppAttachments[i]->Release();
            }
            GetRawAllocator().Free(m_ppAttachments);
        }
        this->m_Desc.pRenderPass->Release();
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Framebuffer, TDeviceObjectBase)

private:
    RefCntAutoPtr<IRenderPass> m_pRenderPass;

    ITextureView** m_ppAttachments = nullptr;
};

} // namespace Diligent
