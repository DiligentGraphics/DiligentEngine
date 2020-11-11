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
/// Definition of the Diligent::IBufferVk interface

#include "../../GraphicsEngine/interface/Buffer.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {12D8EC02-96F4-431E-9695-C5F572CC7587}
static const INTERFACE_ID IID_BufferVk =
    {0x12d8ec02, 0x96f4, 0x431e, {0x96, 0x95, 0xc5, 0xf5, 0x72, 0xcc, 0x75, 0x87}};

#define DILIGENT_INTERFACE_NAME IBufferVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferVkInclusiveMethods \
    IBufferInclusiveMethods;      \
    IBufferVkMethods BufferVk

// clang-format off

/// Exposes Vulkan-specific functionality of a buffer object.
DILIGENT_BEGIN_INTERFACE(IBufferVk, IBuffer)
{
    /// Returns a vulkan buffer handle
    VIRTUAL VkBuffer METHOD(GetVkBuffer)(THIS) CONST PURE;

    /// Sets vulkan access flags

    /// \param [in] AccessFlags - Vulkan access flags to be set for this buffer
    VIRTUAL void METHOD(SetAccessFlags)(THIS_
                                        VkAccessFlags AccessFlags) PURE;

    /// If the buffer state is known to the engine (i.e. not Diligent::RESOURCE_STATE_UNKNOWN),
    /// returns Vulkan access flags corresponding to the state. If the state is unknown, returns 0.
    VIRTUAL VkAccessFlags METHOD(GetAccessFlags)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IBufferVk_GetVkBuffer(This)         CALL_IFACE_METHOD(BufferVk, GetVkBuffer, This)
#    define IBufferVk_SetAccessFlags(This, ...) CALL_IFACE_METHOD(BufferVk, SetAccessFlags, This, __VA_ARGS__)
#    define IBufferVk_GetAccessFlags(This)      CALL_IFACE_METHOD(BufferVk, GetAccessFlags, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
