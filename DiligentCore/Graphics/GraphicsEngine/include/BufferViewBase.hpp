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
/// Implementation of the Diligent::BufferViewBase template class

#include "BufferView.h"
#include "DeviceObjectBase.hpp"
#include "GraphicsTypes.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Template class implementing base functionality for a buffer view object

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IBufferViewD3D11, Diligent::IBufferViewD3D12,
///                          Diligent::IBufferViewGL or Diligent::IBufferViewVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
///                                (Diligent::RenderDeviceD3D11Impl, Diligent::RenderDeviceD3D12Impl,
///                                 Diligent::RenderDeviceGLImpl, or Diligent::RenderDeviceVkImpl)
template <class BaseInterface, class RenderDeviceImplType>
class BufferViewBase : public DeviceObjectBase<BaseInterface, RenderDeviceImplType, BufferViewDesc>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, BufferViewDesc>;

    /// \param pRefCounters - reference counters object that controls the lifetime of this buffer view.
    /// \param pDevice - pointer to the render device.
    /// \param ViewDesc - buffer view description.
    /// \param pBuffer - pointer to the buffer that the view is to be created for.
    /// \param bIsDefaultView - flag indicating if the view is default view, and is thus
    ///						    part of the buffer object. In this case the view will attach
    ///							to the buffer's reference counters.
    BufferViewBase(IReferenceCounters*   pRefCounters,
                   RenderDeviceImplType* pDevice,
                   const BufferViewDesc& ViewDesc,
                   IBuffer*              pBuffer,
                   bool                  bIsDefaultView) :
        // Default views are created as part of the buffer, so we cannot not keep strong
        // reference to the buffer to avoid cyclic links. Instead, we will attach to the
        // reference counters of the buffer.
        TDeviceObjectBase(pRefCounters, pDevice, ViewDesc),
        m_pBuffer{pBuffer},
        // For non-default view, we will keep strong reference to buffer
        m_spBuffer{bIsDefaultView ? nullptr : pBuffer}
    {}

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BufferView, TDeviceObjectBase)

    /// Implementation of IBufferView::GetBuffer()
    virtual IBuffer* DILIGENT_CALL_TYPE GetBuffer() override final
    {
        return m_pBuffer;
    }

    template <typename BufferType>
    BufferType* GetBuffer()
    {
        return ValidatedCast<BufferType>(m_pBuffer);
    }

    template <typename BufferType>
    BufferType* GetBuffer() const
    {
        return ValidatedCast<BufferType>(m_pBuffer);
    }

protected:
    /// Pointer to the buffer
    IBuffer* const m_pBuffer;

    /// Strong reference to the buffer. Used for non-default views
    /// to keep the buffer alive
    RefCntAutoPtr<IBuffer> m_spBuffer;
};

} // namespace Diligent
