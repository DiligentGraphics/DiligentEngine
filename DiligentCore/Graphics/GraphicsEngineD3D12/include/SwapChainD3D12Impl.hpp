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
/// Declaration of Diligent::SwapChainD3D12Impl class

#include <dxgi1_4.h>
#include "SwapChainD3D12.h"
#include "SwapChainD3DBase.hpp"

namespace Diligent
{

/// Swap chain implementation in Direct3D12 backend.
class SwapChainD3D12Impl final : public SwapChainD3DBase<ISwapChainD3D12, IDXGISwapChain3>
{
public:
    using TSwapChainBase = SwapChainD3DBase<ISwapChainD3D12, IDXGISwapChain3>;

    SwapChainD3D12Impl(IReferenceCounters*           pRefCounters,
                       const SwapChainDesc&          SwapChainDesc,
                       const FullScreenModeDesc&     FSDesc,
                       class RenderDeviceD3D12Impl*  pRenderDeviceD3D12,
                       class DeviceContextD3D12Impl* pDeviceContextD3D12,
                       const NativeWindow&           Window);
    ~SwapChainD3D12Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ISwapChain::Present() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Present(Uint32 SyncInterval) override final;

    /// Implementation of ISwapChain::Resize() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE Resize(Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewPreTransform) override final;

    /// Implementation of ISwapChainD3D12::GetDXGISwapChain().
    virtual IDXGISwapChain* DILIGENT_CALL_TYPE GetDXGISwapChain() override final { return m_pSwapChain; }

    /// Implementation of ISwapChain::GetCurrentBackBufferRTV() in Direct3D12 backend.
    virtual ITextureViewD3D12* DILIGENT_CALL_TYPE GetCurrentBackBufferRTV() override final
    {
        auto CurrentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
        VERIFY_EXPR(CurrentBackBufferIndex >= 0 && CurrentBackBufferIndex < m_SwapChainDesc.BufferCount);
        return m_pBackBufferRTV[CurrentBackBufferIndex];
    }

    /// Implementation of ISwapChain::GetDepthBufferDSV() in Direct3D12 backend.
    virtual ITextureViewD3D12* DILIGENT_CALL_TYPE GetDepthBufferDSV() override final { return m_pDepthBufferDSV; }

private:
    virtual void UpdateSwapChain(bool CreateNew) override final;
    void         InitBuffersAndViews();

    std::vector<RefCntAutoPtr<ITextureViewD3D12>, STDAllocatorRawMem<RefCntAutoPtr<ITextureViewD3D12>>> m_pBackBufferRTV;
    RefCntAutoPtr<ITextureViewD3D12>                                                                    m_pDepthBufferDSV;
};

} // namespace Diligent
