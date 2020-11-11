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
/// Definition of the Diligent::IDeviceContextVk interface

#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "CommandQueueVk.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {72AEB1BA-C6AD-42EC-8811-7ED9C72176BB}
static const INTERFACE_ID IID_DeviceContextVk =
    {0x72aeb1ba, 0xc6ad, 0x42ec, {0x88, 0x11, 0x7e, 0xd9, 0xc7, 0x21, 0x76, 0xbb}};

#define DILIGENT_INTERFACE_NAME IDeviceContextVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceContextVkInclusiveMethods \
    IDeviceContextInclusiveMethods;      \
    IDeviceContextVkMethods DeviceContextVk

// clang-format off

/// Exposes Vulkan-specific functionality of a device context.
DILIGENT_BEGIN_INTERFACE(IDeviceContextVk, IDeviceContext)
{
    /// Transitions internal vulkan image to a specified layout

    /// \param [in] pTexture - texture to transition
    /// \param [in] NewLayout - Vulkan image layout this texture to transition to
    /// \remarks The texture state must be known to the engine.
    VIRTUAL void METHOD(TransitionImageLayout)(THIS_
                                               ITexture*     pTexture,
                                               VkImageLayout NewLayout) PURE;

    /// Transitions internal vulkan buffer object to a specified state

    /// \param [in] pBuffer - Buffer to transition
    /// \param [in] NewAccessFlags - Access flags to set for the buffer
    /// \remarks The buffer state must be known to the engine.
    VIRTUAL void METHOD(BufferMemoryBarrier)(THIS_
                                             IBuffer*      pBuffer,
                                             VkAccessFlags NewAccessFlags) PURE;

    /// Locks the internal mutex and returns a pointer to the command queue that is associated with this device context.

    /// \return - a pointer to ICommandQueueVk interface of the command queue associated with the context.
    ///
    /// \remarks  Only immediate device contexts have associated command queues.
    ///
    ///           The engine locks the internal mutex to prevent simultaneous access to the command queue.
    ///           An application must release the lock by calling IDeviceContextVk::UnlockCommandQueue()
    ///           when it is done working with the queue or the engine will not be able to submit any command
    ///           list to the queue. Nested calls to LockCommandQueue() are not allowed.
    ///           The queue pointer never changes while the context is alive, so an application may cache and
    ///           use the pointer if it does not need to prevent potential simultaneous access to the queue from
    ///           other threads.
    ///
    ///           The engine manages the lifetimes of command queues and all other device objects,
    ///           so an application must not call AddRef/Release methods on the returned interface.
    VIRTUAL ICommandQueueVk* METHOD(LockCommandQueue)(THIS) PURE;

    /// Unlocks the command queue that was previously locked by IDeviceContextVk::LockCommandQueue().
    VIRTUAL void METHOD(UnlockCommandQueue)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IDeviceContextVk_TransitionImageLayout(This, ...) CALL_IFACE_METHOD(DeviceContextVk, TransitionImageLayout, This, __VA_ARGS__)
#    define IDeviceContextVk_BufferMemoryBarrier(This, ...)   CALL_IFACE_METHOD(DeviceContextVk, BufferMemoryBarrier,   This, __VA_ARGS__)
#    define IDeviceContextVk_LockCommandQueue(This)           CALL_IFACE_METHOD(DeviceContextVk, LockCommandQueue,      This)
#    define IDeviceContextVk_UnlockCommandQueue(This)         CALL_IFACE_METHOD(DeviceContextVk, UnlockCommandQueue,    This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
