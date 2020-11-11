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
/// Definition of the Diligent::IFenceD3D12 interface

#include "../../GraphicsEngine/interface/Fence.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {053C0D8C-3757-4220-A9CC-4749EC4794AD}
static const INTERFACE_ID IID_FenceD3D12 =
    {0x53c0d8c, 0x3757, 0x4220, {0xa9, 0xcc, 0x47, 0x49, 0xec, 0x47, 0x94, 0xad}};

#define DILIGENT_INTERFACE_NAME IFenceD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

// clang-format off

#define IFenceD3D12InclusiveMethods \
    IFenceInclusiveMethods;         \
    IFenceD3D12Methods FenceD3D12

/// Exposes Direct3D12-specific functionality of a fence object.
DILIGENT_BEGIN_INTERFACE(IFenceD3D12, IFence)
{
    /// Returns a pointer to the ID3D12Fence interface of the internal Direct3D12 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ID3D12Fence* METHOD(GetD3D12Fence)(THIS) PURE;

    /// Waits until the fence reaches the specified value, on the host.
    VIRTUAL void METHOD(WaitForCompletion)(THIS_
                                           Uint64 Value) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IFenceD3D12_GetD3D12Fence(This)          CALL_IFACE_METHOD(FenceD3D12, GetD3D12Fence,     This)
#    define IFenceD3D12_WaitForCompletion(This, ...) CALL_IFACE_METHOD(FenceD3D12, WaitForCompletion, This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
