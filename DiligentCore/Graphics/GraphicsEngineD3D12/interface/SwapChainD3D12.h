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
/// Definition of the Diligent::ISwapChainD3D12 interface

#include <dxgi1_4.h>

#include "../../GraphicsEngine/interface/SwapChain.h"
#include "TextureViewD3D12.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {C9F8384D-A45E-4970-8447-394177E5B0EE}
static const INTERFACE_ID IID_SwapChainD3D12 =
    {0xc9f8384d, 0xa45e, 0x4970, {0x84, 0x47, 0x39, 0x41, 0x77, 0xe5, 0xb0, 0xee}};

#define DILIGENT_INTERFACE_NAME ISwapChainD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISwapChainD3D12InclusiveMethods \
    ISwapChainInclusiveMethods;         \
    ISwapChainD3D12Methods SwapChainD3D12

/// Exposes Direct3D12-specific functionality of a swap chain.
DILIGENT_BEGIN_INTERFACE(ISwapChainD3D12, ISwapChain)
{
    /// Returns a pointer to the IDXGISwapChain interface of the internal DXGI object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL IDXGISwapChain* METHOD(GetDXGISwapChain)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ISwapChainD3D12_GetDXGISwapChain(This)  CALL_IFACE_METHOD(SwapChainD3D12, GetDXGISwapChain, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
