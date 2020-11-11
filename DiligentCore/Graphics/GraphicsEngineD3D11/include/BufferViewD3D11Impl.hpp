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
/// Declaration of Diligent::BufferViewD3D11Impl class

#include "BufferViewD3D11.h"
#include "RenderDeviceD3D11.h"
#include "BufferViewBase.hpp"
#include "RenderDeviceD3D11Impl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Buffer view implementation in Direct3D11 backend.
class BufferViewD3D11Impl final : public BufferViewBase<IBufferViewD3D11, RenderDeviceD3D11Impl>
{
public:
    using TBufferViewBase = BufferViewBase<IBufferViewD3D11, RenderDeviceD3D11Impl>;

    BufferViewD3D11Impl(IReferenceCounters*    pRefCounters,
                        RenderDeviceD3D11Impl* pDevice,
                        const BufferViewDesc&  ViewDesc,
                        IBuffer*               pBuffer,
                        ID3D11View*            pD3D11View,
                        bool                   bIsDefaultView);

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) final;

    /// Implementation of IBufferViewD3D11::GetD3D11View().
    virtual ID3D11View* DILIGENT_CALL_TYPE GetD3D11View() override final
    {
        return m_pD3D11View;
    }

protected:
    CComPtr<ID3D11View> m_pD3D11View; ///<D3D11 view
};

} // namespace Diligent
