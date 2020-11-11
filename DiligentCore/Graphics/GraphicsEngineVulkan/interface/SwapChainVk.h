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
/// Definition of the Diligent::ISwapChainVk interface

#include "../../GraphicsEngine/interface/SwapChain.h"
#include "TextureViewVk.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {22A39881-5EC5-4A9C-8395-90215F04A5CC}
static const INTERFACE_ID IID_SwapChainVk =
    {0x22a39881, 0x5ec5, 0x4a9c, {0x83, 0x95, 0x90, 0x21, 0x5f, 0x4, 0xa5, 0xcc}};

#define DILIGENT_INTERFACE_NAME ISwapChainVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISwapChainVkInclusiveMethods \
    ISwapChainInclusiveMethods;      \
    ISwapChainVkMethods SwapChainVk

/// Exposes Vulkan-specific functionality of a swap chain.
DILIGENT_BEGIN_INTERFACE(ISwapChainVk, ISwapChain)
{
    /// Returns a handle to the Vulkan swap chain object.
    VIRTUAL VkSwapchainKHR METHOD(GetVkSwapChain)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ISwapChainVk_GetVkSwapChain(This)  CALL_IFACE_METHOD(SwapChainVk, GetVkSwapChain, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
