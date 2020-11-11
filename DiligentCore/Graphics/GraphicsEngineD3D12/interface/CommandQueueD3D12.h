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
/// Definition of the Diligent::ICommandQueueD3D12 interface

#include "../../../Primitives/interface/Object.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {D89693CE-F3F4-44B5-B7EF-24115AAD085E}
static const INTERFACE_ID IID_CommandQueueD3D12 =
    {0xd89693ce, 0xf3f4, 0x44b5, {0xb7, 0xef, 0x24, 0x11, 0x5a, 0xad, 0x8, 0x5e}};

#define DILIGENT_INTERFACE_NAME ICommandQueueD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ICommandQueueD3D12InclusiveMethods \
    IObjectInclusiveMethods;               \
    ICommandQueueD3D12Methods CommandQueueD3D12

// clang-format off

/// Command queue interface
DILIGENT_BEGIN_INTERFACE(ICommandQueueD3D12, IObject)
{
    /// Returns the fence value that will be signaled next time
    VIRTUAL Uint64 METHOD(GetNextFenceValue)(THIS) CONST PURE;

    /// Executes a given command list

    /// \return Fence value associated with the executed command list
    VIRTUAL Uint64 METHOD(Submit)(THIS_
                                  ID3D12GraphicsCommandList* commandList) PURE;

    /// Returns D3D12 command queue. May return null if queue is anavailable
    VIRTUAL ID3D12CommandQueue* METHOD(GetD3D12CommandQueue)(THIS) PURE;

    /// Returns value of the last completed fence
    VIRTUAL Uint64 METHOD(GetCompletedFenceValue)(THIS) PURE;

    /// Blocks execution until all pending GPU commands are complete
    VIRTUAL Uint64 METHOD(WaitForIdle)(THIS) PURE;

    /// Signals the given fence
    VIRTUAL void METHOD(SignalFence)(THIS_
                                     ID3D12Fence* pFence,
                                     Uint64       Value) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ICommandQueueD3D12_GetNextFenceValue(This)      CALL_IFACE_METHOD(CommandQueueD3D12, GetNextFenceValue,     This)
#    define ICommandQueueD3D12_Submit(This, ...)            CALL_IFACE_METHOD(CommandQueueD3D12, Submit,                This, __VA_ARGS__)
#    define ICommandQueueD3D12_GetD3D12CommandQueue(This)   CALL_IFACE_METHOD(CommandQueueD3D12, GetD3D12CommandQueue,  This)
#    define ICommandQueueD3D12_GetCompletedFenceValue(This) CALL_IFACE_METHOD(CommandQueueD3D12, GetCompletedFenceValue,This)
#    define ICommandQueueD3D12_WaitForIdle(This)            CALL_IFACE_METHOD(CommandQueueD3D12, WaitForIdle,           This)
#    define ICommandQueueD3D12_SignalFence(This, ...)       CALL_IFACE_METHOD(CommandQueueD3D12, SignalFence,           This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
