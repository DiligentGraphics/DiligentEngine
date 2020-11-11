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
#include <thread>
#include <atlbase.h>

#include "FenceD3D12Impl.hpp"
#include "EngineMemory.h"
#include "RenderDeviceD3D12Impl.hpp"
namespace Diligent
{

FenceD3D12Impl::FenceD3D12Impl(IReferenceCounters*    pRefCounters,
                               RenderDeviceD3D12Impl* pDevice,
                               const FenceDesc&       Desc) :
    TFenceBase{pRefCounters, pDevice, Desc}
{
    auto* pd3d12Device = ValidatedCast<RenderDeviceD3D12Impl>(pDevice)->GetD3D12Device();
    auto  hr           = pd3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(m_pd3d12Fence), reinterpret_cast<void**>(static_cast<ID3D12Fence**>(&m_pd3d12Fence)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create D3D12 fence");
}

FenceD3D12Impl::~FenceD3D12Impl()
{
}

Uint64 FenceD3D12Impl::GetCompletedValue()
{
    return m_pd3d12Fence->GetCompletedValue();
}

void FenceD3D12Impl::Reset(Uint64 Value)
{
    m_pd3d12Fence->Signal(Value);
}

void FenceD3D12Impl::WaitForCompletion(Uint64 Value)
{
    while (m_pd3d12Fence->GetCompletedValue() < Value)
        std::this_thread::yield();
}

} // namespace Diligent
