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
/// Implementation of the Diligent::SwapChainBase template class

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "ObjectBase.hpp"
#include "Errors.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Base implementation of the swap chain.

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::ISwapChainGL, Diligent::ISwapChainD3D11,
///                          Diligent::ISwapChainD3D12 or Diligent::ISwapChainVk).
/// \remarks Swap chain holds the strong reference to the device and a weak reference to the
///          immediate context.
template <class BaseInterface>
class SwapChainBase : public ObjectBase<BaseInterface>
{
public:
    using TObjectBase = ObjectBase<BaseInterface>;

    /// \param pRefCounters - reference counters object that controls the lifetime of this swap chain.
    /// \param pDevice - pointer to the device.
    /// \param pDeviceContext - pointer to the device context.
    /// \param SCDesc - swap chain description
    SwapChainBase(IReferenceCounters*  pRefCounters,
                  IRenderDevice*       pDevice,
                  IDeviceContext*      pDeviceContext,
                  const SwapChainDesc& SCDesc) :
        // clang-format off
        TObjectBase          {pRefCounters       },
        m_pRenderDevice      {pDevice            },
        m_wpDeviceContext    {pDeviceContext     },
        m_SwapChainDesc      {SCDesc             },
        m_DesiredPreTransform{SCDesc.PreTransform}
    // clang-format on
    {
    }

    // clang-format off
    SwapChainBase             (const SwapChainBase&)  = delete;
    SwapChainBase             (      SwapChainBase&&) = delete;
    SwapChainBase& operator = (const SwapChainBase&)  = delete;
    SwapChainBase& operator = (     SwapChainBase&&)  = delete;
    // clang-format on

    ~SwapChainBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_SwapChain, TObjectBase)

    /// Implementation of ISwapChain::GetDesc()
    virtual const SwapChainDesc& DILIGENT_CALL_TYPE GetDesc() const override final
    {
        return m_SwapChainDesc;
    }

    virtual void DILIGENT_CALL_TYPE SetMaximumFrameLatency(Uint32 MaxLatency) override
    {}

protected:
    bool Resize(Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewPreTransform, Int32 Dummy = 0 /*To be different from virtual function*/)
    {
        if (NewWidth != 0 && NewHeight != 0 &&
            (m_SwapChainDesc.Width != NewWidth ||
             m_SwapChainDesc.Height != NewHeight ||
             m_DesiredPreTransform != NewPreTransform))
        {
            m_SwapChainDesc.Width  = NewWidth;
            m_SwapChainDesc.Height = NewHeight;
            m_DesiredPreTransform  = NewPreTransform;
            LOG_INFO_MESSAGE("Resizing the swap chain to ", m_SwapChainDesc.Width, "x", m_SwapChainDesc.Height);
            return true;
        }

        return false;
    }

    /// Strong reference to the render device
    RefCntAutoPtr<IRenderDevice> m_pRenderDevice;

    /// Weak references to the immediate device context. The context holds
    /// the strong reference to the swap chain.
    RefCntWeakPtr<IDeviceContext> m_wpDeviceContext;

    /// Swap chain description
    SwapChainDesc m_SwapChainDesc;

    /// Desired surface pre-transformation.
    SURFACE_TRANSFORM m_DesiredPreTransform = SURFACE_TRANSFORM_OPTIMAL;
};

} // namespace Diligent
