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
/// Definition of the Diligent::IBufferD3D12 interface

#include "../../GraphicsEngine/interface/Buffer.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {3E9B15ED-A289-48DC-8214-C6E3E6177378}
static const INTERFACE_ID IID_BufferD3D12 =
    {0x3e9b15ed, 0xa289, 0x48dc, {0x82, 0x14, 0xc6, 0xe3, 0xe6, 0x17, 0x73, 0x78}};

#define DILIGENT_INTERFACE_NAME IBufferD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferD3D12InclusiveMethods \
    IBufferInclusiveMethods;         \
    IBufferD3D12Methods BufferD3D12

// clang-format off

/// Exposes Direct3D12-specific functionality of a buffer object.
DILIGENT_BEGIN_INTERFACE(IBufferD3D12, IBuffer)
{
    /// Returns a pointer to the ID3D12Resource interface of the internal Direct3D12 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    /// \param [in] DataStartByteOffset - Offset from the beginning of the buffer
    ///                            to the start of the data. This parameter
    ///                            is required for dynamic buffers, which are
    ///                            suballocated in a dynamic upload heap
    /// \param [in] pContext - Device context within which address of the buffer is requested.
    VIRTUAL ID3D12Resource* METHOD(GetD3D12Buffer)(THIS_
                                                   Uint64 REF      DataStartByteOffset,
                                                   IDeviceContext* pContext) PURE;

    /// Sets the buffer usage state

    /// \param [in] state - D3D12 resource state to be set for this buffer
    VIRTUAL void METHOD(SetD3D12ResourceState)(THIS_
                                               D3D12_RESOURCE_STATES state) PURE;

    /// Returns current D3D12 buffer state.
    /// If the state is unknown to the engine (Diligent::RESOURCE_STATE_UNKNOWN),
    /// returns D3D12_RESOURCE_STATE_COMMON (0).
    VIRTUAL D3D12_RESOURCE_STATES METHOD(GetD3D12ResourceState)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IBufferD3D12_GetD3D12Buffer(This, ...)        CALL_IFACE_METHOD(BufferD3D12, GetD3D12Buffer,        This, __VA_ARGS__)
#    define IBufferD3D12_SetD3D12ResourceState(This, ...) CALL_IFACE_METHOD(BufferD3D12, SetD3D12ResourceState, This, __VA_ARGS__)
#    define IBufferD3D12_GetD3D12ResourceState(This)      CALL_IFACE_METHOD(BufferD3D12, GetD3D12ResourceState, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
