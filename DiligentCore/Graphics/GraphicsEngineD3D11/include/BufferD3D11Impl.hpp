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
/// Declaration of Diligent::BufferD3D11Impl class

#include "BufferD3D11.h"
#include "RenderDeviceD3D11.h"
#include "BufferBase.hpp"
#include "BufferViewD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Buffer object implementation in Direct3D11 backend.
class BufferD3D11Impl final : public BufferBase<IBufferD3D11, RenderDeviceD3D11Impl, BufferViewD3D11Impl, FixedBlockMemoryAllocator>
{
public:
    using TBufferBase = BufferBase<IBufferD3D11, RenderDeviceD3D11Impl, BufferViewD3D11Impl, FixedBlockMemoryAllocator>;

    BufferD3D11Impl(IReferenceCounters*          pRefCounters,
                    FixedBlockMemoryAllocator&   BuffViewObjMemAllocator,
                    class RenderDeviceD3D11Impl* pDeviceD3D11,
                    const BufferDesc&            BuffDesc,
                    const BufferData*            pBuffData = nullptr);

    BufferD3D11Impl(IReferenceCounters*          pRefCounters,
                    FixedBlockMemoryAllocator&   BuffViewObjMemAllocator,
                    class RenderDeviceD3D11Impl* pDeviceD3D11,
                    const BufferDesc&            BuffDesc,
                    RESOURCE_STATE               InitialState,
                    ID3D11Buffer*                pd3d11Buffer);

    ~BufferD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IBufferD3D11::GetD3D11Buffer().
    virtual ID3D11Buffer* DILIGENT_CALL_TYPE GetD3D11Buffer() override final { return m_pd3d11Buffer; }

    /// Implementation of IBuffer::GetNativeHandle().
    virtual void* DILIGENT_CALL_TYPE GetNativeHandle() override final { return GetD3D11Buffer(); }

    void AddState(RESOURCE_STATE State)
    {
        m_State = static_cast<RESOURCE_STATE>(m_State & ~static_cast<Uint32>(RESOURCE_STATE_UNDEFINED));
        m_State = static_cast<RESOURCE_STATE>(m_State | State);
    }

    void ClearState(RESOURCE_STATE State)
    {
        VERIFY_EXPR(IsInKnownState());
        m_State = static_cast<RESOURCE_STATE>(m_State & ~static_cast<Uint32>(State));
        if (m_State == RESOURCE_STATE_UNKNOWN)
            m_State = RESOURCE_STATE_UNDEFINED;
    }

private:
    virtual void CreateViewInternal(const struct BufferViewDesc& ViewDesc, IBufferView** ppView, bool bIsDefaultView) override;

    void CreateUAV(struct BufferViewDesc& UAVDesc, ID3D11UnorderedAccessView** ppD3D11UAV);
    void CreateSRV(struct BufferViewDesc& SRVDesc, ID3D11ShaderResourceView** ppD3D11SRV);

    friend class DeviceContextD3D11Impl;
    CComPtr<ID3D11Buffer> m_pd3d11Buffer; ///< D3D11 buffer object
};

} // namespace Diligent
