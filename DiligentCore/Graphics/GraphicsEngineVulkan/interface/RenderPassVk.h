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
/// Definition of the Diligent::IRenderPassVk interface

#include "../../GraphicsEngine/interface/RenderPass.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {3DE6938F-D34D-4135-A6FA-15A89E9525D0}
static const INTERFACE_ID IID_RenderPassVk =
    {0x3de6938f, 0xd34d, 0x4135, {0xa6, 0xfa, 0x15, 0xa8, 0x9e, 0x95, 0x25, 0xd0}};

#define DILIGENT_INTERFACE_NAME IRenderPassVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IRenderPassVkInclusiveMethods                              \
    /*IRenderPassInclusiveMethods*/ IDeviceObjectInclusiveMethods; \
    IRenderPassVkMethods RenderPassVk

/// Exposes Vulkan-specific functionality of a RenderPass object.
DILIGENT_BEGIN_INTERFACE(IRenderPassVk, IRenderPass)
{
    /// Returns a Vulkan RenderPass object handle
    VIRTUAL VkRenderPass METHOD(GetVkRenderPass)() CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IRenderPassVk_GetVkRenderPass(This) CALL_IFACE_METHOD(RenderPassVk, GetVkRenderPass, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
