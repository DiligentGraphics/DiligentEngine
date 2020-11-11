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
/// Definition of the Diligent::ITextureD3D12 interface

#include "../../GraphicsEngine/interface/Texture.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {CF5522EF-8116-4D76-ADF1-5CC8FB31FF66}
static const INTERFACE_ID IID_TextureD3D12 =
    {0xcf5522ef, 0x8116, 0x4d76, {0xad, 0xf1, 0x5c, 0xc8, 0xfb, 0x31, 0xff, 0x66}};

#define DILIGENT_INTERFACE_NAME ITextureD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureD3D12InclusiveMethods \
    ITextureInclusiveMethods;         \
    ITextureD3D12Methods TextureD3D12

// clang-format off

/// Exposes Direct3D12-specific functionality of a texture object.
DILIGENT_BEGIN_INTERFACE(ITextureD3D12, ITexture)
{
    /// Returns a pointer to the ID3D12Resource interface of the internal Direct3D12 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ID3D12Resource* METHOD(GetD3D12Texture)(THIS) PURE;

    /// Sets the texture usage state

    /// \param [in] state - D3D12 resource state to be set for this texture
    VIRTUAL void METHOD(SetD3D12ResourceState)(THIS_
                                                   D3D12_RESOURCE_STATES state) PURE;

    /// Returns current D3D12 texture state.
    /// If the state is unknown to the engine (Diligent::RESOURCE_STATE_UNKNOWN),
    /// returns D3D12_RESOURCE_STATE_COMMON (0).
    VIRTUAL D3D12_RESOURCE_STATES METHOD(GetD3D12ResourceState)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITextureD3D12_GetD3D12Texture(This)            CALL_IFACE_METHOD(TextureD3D12, GetD3D12Texture,       This)
#    define ITextureD3D12_SetD3D12ResourceState(This, ...) CALL_IFACE_METHOD(TextureD3D12, SetD3D12ResourceState, This, __VA_ARGS__)
#    define ITextureD3D12_GetD3D12ResourceState(This)      CALL_IFACE_METHOD(TextureD3D12, GetD3D12ResourceState, This)

// clang-format ons

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
