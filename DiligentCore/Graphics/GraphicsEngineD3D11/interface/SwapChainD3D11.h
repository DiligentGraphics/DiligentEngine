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
/// Definition of the Diligent::ISwapChainD3D11 interface

#include "../../GraphicsEngine/interface/SwapChain.h"
#include "TextureViewD3D11.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {4DAF2E76-9204-4DC4-A53A-B00097412D3A}
static const struct INTERFACE_ID IID_SwapChainD3D11 =
    {0x4daf2e76, 0x9204, 0x4dc4, {0xa5, 0x3a, 0xb0, 0x0, 0x97, 0x41, 0x2d, 0x3a}};

#define DILIGENT_INTERFACE_NAME ISwapChainD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISwapChainD3D11InclusiveMethods \
    ISwapChainInclusiveMethods;         \
    ISwapChainD3D11Methods SwapChainD3D11

/// Exposes Direct3D11-specific functionality of a swap chain.
DILIGENT_BEGIN_INTERFACE(ISwapChainD3D11, ISwapChain)
{
    /// Returns render target view of the back buffer in the swap chain
    VIRTUAL struct ITextureViewD3D11* METHOD(GetCurrentBackBufferRTV)(THIS) PURE;

    /// Returns depth-stencil view of the depth buffer
    VIRTUAL struct ITextureViewD3D11* METHOD(GetDepthBufferDSV)(THIS) PURE;

    /// Returns a pointer to the IDXGISwapChain interface of the internal DXGI object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL IDXGISwapChain* METHOD(GetDXGISwapChain)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ISwapChainD3D11_GetCurrentBackBufferRTV(This) CALL_IFACE_METHOD(SwapChainD3D11, GetCurrentBackBufferRTV, This)
#    define ISwapChainD3D11_GetDepthBufferDSV(This)       CALL_IFACE_METHOD(SwapChainD3D11, GetDepthBufferDSV,       This)
#    define ISwapChainD3D11_GetDXGISwapChain(This)        CALL_IFACE_METHOD(SwapChainD3D11, GetDXGISwapChain,        This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
