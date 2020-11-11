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
/// Definition of the Diligent::IFramebufferVk interface

#include "../../GraphicsEngine/interface/Framebuffer.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {846BE360-D89B-41AD-B089-7F2439ADCE3A}
static const INTERFACE_ID IID_FramebufferVk =
    {0x846be360, 0xd89b, 0x41ad, {0xb0, 0x89, 0x7f, 0x24, 0x39, 0xad, 0xce, 0x3a}};

#define DILIGENT_INTERFACE_NAME IFramebufferVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IFramebufferVkInclusiveMethods                              \
    /*IFramebufferInclusiveMethods*/ IDeviceObjectInclusiveMethods; \
    IFramebufferVkMethods FramebufferVk

/// Exposes Vulkan-specific functionality of a Framebuffer object.
DILIGENT_BEGIN_INTERFACE(IFramebufferVk, IFramebuffer)
{
    /// Returns a Vulkan framebuffer object handle
    VIRTUAL VkFramebuffer METHOD(GetVkFramebuffer)() CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IFramebufferVk_GetVkFramebuffer(This) CALL_IFACE_METHOD(FramebufferVk, GetVkFramebuffer, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
