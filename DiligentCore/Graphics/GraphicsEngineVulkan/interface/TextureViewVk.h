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
/// Definition of the Diligent::ITextureViewVk interface

#include "../../GraphicsEngine/interface/TextureView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {B02AA468-3328-46F3-9777-55E97BF6C86E}
static const INTERFACE_ID IID_TextureViewVk =
    {0xb02aa468, 0x3328, 0x46f3, {0x97, 0x77, 0x55, 0xe9, 0x7b, 0xf6, 0xc8, 0x6e}};

#define DILIGENT_INTERFACE_NAME ITextureViewVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureViewVkInclusiveMethods \
    ITextureViewInclusiveMethods;      \
    ITextureViewVkMethods TextureViewVk

// clang-format off

/// Exposes Vulkan-specific functionality of a texture view object.
DILIGENT_BEGIN_INTERFACE(ITextureViewVk, ITextureView)
{
    /// Returns Vulkan image view handle
    VIRTUAL VkImageView METHOD(GetVulkanImageView)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITextureViewVk_GetVulkanImageView(This) CALL_IFACE_METHOD(TextureViewVk, GetVulkanImageView, This)

// clang-format ons

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
