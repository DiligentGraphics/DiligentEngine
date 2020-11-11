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
/// Definition of the Diligent::ITextureVk interface

#include "../../GraphicsEngine/interface/Texture.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {3BB9155F-22C5-4365-927E-8C4049F9B949}
static const INTERFACE_ID IID_TextureVk =
    {0x3bb9155f, 0x22c5, 0x4365, {0x92, 0x7e, 0x8c, 0x40, 0x49, 0xf9, 0xb9, 0x49}};

#define DILIGENT_INTERFACE_NAME ITextureVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureVkInclusiveMethods \
    ITextureInclusiveMethods;      \
    ITextureVkMethods TextureVk

// clang-format off

/// Exposes Vulkan-specific functionality of a texture object.
DILIGENT_BEGIN_INTERFACE(ITextureVk, ITexture)
{
    /// Returns Vulkan image handle.

    /// The application must not release the returned image
    VIRTUAL VkImage METHOD(GetVkImage)(THIS) CONST PURE;

    /// Sets Vulkan image layout

    /// \param [in] Layout - Vulkan image layout to set.
    /// \note This function does not perform layout transition, but sets the
    ///       internal texture state to match the given Vulkan layout.
    VIRTUAL void METHOD(SetLayout)(THIS_
                                   VkImageLayout Layout) PURE;

    /// Returns current Vulkan image layout. If the state is unknown to the engine, returns VK_IMAGE_LAYOUT_UNDEFINED
    VIRTUAL VkImageLayout METHOD(GetLayout)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITextureVk_GetVkImage(This)     CALL_IFACE_METHOD(TextureVk, GetVkImage,This)
#    define ITextureVk_SetLayout(This, ...) CALL_IFACE_METHOD(TextureVk, SetLayout, This, __VA_ARGS__)
#    define ITextureVk_GetLayout(This)      CALL_IFACE_METHOD(TextureVk, GetLayout, This)

// clang-format ons

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
