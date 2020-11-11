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
/// Declaration of Diligent::FenceD3D12Impl class

#include "FenceD3D12.h"
#include "RenderDeviceD3D12.h"
#include "FenceBase.hpp"
#include "RenderDeviceD3D12Impl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Fence implementation in Direct3D12 backend.
class FenceD3D12Impl final : public FenceBase<IFenceD3D12, RenderDeviceD3D12Impl>
{
public:
    using TFenceBase = FenceBase<IFenceD3D12, RenderDeviceD3D12Impl>;

    FenceD3D12Impl(IReferenceCounters*    pRefCounters,
                   RenderDeviceD3D12Impl* pDevice,
                   const FenceDesc&       Desc);
    ~FenceD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_FenceD3D12, TFenceBase);

    /// Implementation of IFence::GetCompletedValue() in Direct3D12 backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedValue() override final;

    /// Implementation of IFence::GetCompletedValue() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Reset(Uint64 Value) override final;

    /// Implementation of IFenceD3D12::GetD3D12Fence().
    virtual ID3D12Fence* DILIGENT_CALL_TYPE GetD3D12Fence() override final { return m_pd3d12Fence; }

    /// Implementation of IFenceD3D12::WaitForCompletion().
    virtual void DILIGENT_CALL_TYPE WaitForCompletion(Uint64 Value) override final;

private:
    CComPtr<ID3D12Fence> m_pd3d12Fence; ///< D3D12 Fence object
};

} // namespace Diligent
