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
/// Definition of the Diligent::IDeviceContextD3D12 interface

#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "CommandQueueD3D12.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {DDE9E3AB-5109-4026-92B7-F5E7EC83E21E}
static const INTERFACE_ID IID_DeviceContextD3D12 =
    {0xdde9e3ab, 0x5109, 0x4026, {0x92, 0xb7, 0xf5, 0xe7, 0xec, 0x83, 0xe2, 0x1e}};

#define DILIGENT_INTERFACE_NAME IDeviceContextD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceContextD3D12InclusiveMethods \
    IDeviceContextInclusiveMethods;         \
    IDeviceContextD3D12Methods DeviceContextD3D12

// clang-format off

/// Exposes Direct3D12-specific functionality of a device context.
DILIGENT_BEGIN_INTERFACE(IDeviceContextD3D12, IDeviceContext)
{
    /// Transitions internal D3D12 texture object to a specified state

    /// \param [in] pTexture - texture to transition
    /// \param [in] State - D3D12 resource state this texture to transition to
    VIRTUAL void METHOD(TransitionTextureState)(THIS_
                                                ITexture*             pTexture,
                                                D3D12_RESOURCE_STATES State) PURE;

    /// Transitions internal D3D12 buffer object to a specified state

    /// \param [in] pBuffer - Buffer to transition
    /// \param [in] State - D3D12 resource state this buffer to transition to
    VIRTUAL void METHOD(TransitionBufferState)(THIS_
                                               IBuffer*              pBuffer,
                                               D3D12_RESOURCE_STATES State) PURE;

    /// Returns a pointer to Direct3D12 graphics command list that is currently being recorded

    /// \return - a pointer to the current command list
    ///
    /// \remarks  Any command on the device context may potentially submit the command list for
    ///           execution into the command queue and make it invalid. An application should
    ///           never cache the pointer and should instead request the command list every time it
    ///           needs it.
    ///
    ///           The engine manages the lifetimes of all command buffers, so an application must
    ///           not call AddRef/Release methods on the returned interface.
    ///
    ///           Diligent Engine internally keeps track of all resource state changes (vertex and index
    ///           buffers, pipeline states, render targets, etc.). If an application changes any of these
    ///           states in the command list, it must invalidate the engine's internal state tracking by
    ///           calling IDeviceContext::InvalidateState() and then manually restore all required states via
    ///           appropriate Diligent API calls.
    VIRTUAL ID3D12GraphicsCommandList* METHOD(GetD3D12CommandList)(THIS) PURE;

    /// Locks the internal mutex and returns a pointer to the command queue that is associated with this device context.

    /// \return - a pointer to ICommandQueueD3D12 interface of the command queue associated with the context.
    ///
    /// \remarks  Only immediate device contexts have associated command queues.
    ///
    ///           The engine locks the internal mutex to prevent simultaneous access to the command queue.
    ///           An application must release the lock by calling IDeviceContextD3D12::UnlockCommandQueue()
    ///           when it is done working with the queue or the engine will not be able to submit any command
    ///           list to the queue. Nested calls to LockCommandQueue() are not allowed.
    ///           The queue pointer never changes while the context is alive, so an application may cache and
    ///           use the pointer if it does not need to prevent potential simultaneous access to the queue from
    ///           other threads.
    ///
    ///           The engine manages the lifetimes of command queues and all other device objects,
    ///           so an application must not call AddRef/Release methods on the returned interface.
    VIRTUAL ICommandQueueD3D12* METHOD(LockCommandQueue)(THIS) PURE;

    /// Unlocks the command queue that was previously locked by IDeviceContextD3D12::LockCommandQueue().
    VIRTUAL void METHOD(UnlockCommandQueue)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IDeviceContextD3D12_TransitionTextureState(This, ...) CALL_IFACE_METHOD(DeviceContextD3D12, TransitionTextureState,This, __VA_ARGS__)
#    define IDeviceContextD3D12_TransitionBufferState(This, ...)  CALL_IFACE_METHOD(DeviceContextD3D12, TransitionBufferState, This, __VA_ARGS__)
#    define IDeviceContextD3D12_GetD3D12CommandList(This)         CALL_IFACE_METHOD(DeviceContextD3D12, GetD3D12CommandList,   This)
#    define IDeviceContextD3D12_LockCommandQueue(This)            CALL_IFACE_METHOD(DeviceContextD3D12, LockCommandQueue,      This)
#    define IDeviceContextD3D12_UnlockCommandQueue(This)          CALL_IFACE_METHOD(DeviceContextD3D12, UnlockCommandQueue,    This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
