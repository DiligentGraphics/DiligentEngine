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
/// Definition of the Diligent::ITextureD3D11 interface

#include "../../GraphicsEngine/interface/Texture.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {F3A84CC2-E485-4E72-A08A-437D7FFBA3AB}
static const INTERFACE_ID IID_TextureD3D11 =
    {0xf3a84cc2, 0xe485, 0x4e72, {0xa0, 0x8a, 0x43, 0x7d, 0x7f, 0xfb, 0xa3, 0xab}};

#define DILIGENT_INTERFACE_NAME ITextureD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureD3D11InclusiveMethods \
    ITextureInclusiveMethods;         \
    ITextureD3D11Methods TextureD3D11

/// Exposes Direct3D11-specific functionality of a texture object.
DILIGENT_BEGIN_INTERFACE(ITextureD3D11, ITexture)
{
    /// Returns a pointer to the ID3D11Resource interface of the internal Direct3D11 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ID3D11Resource* METHOD(GetD3D11Texture)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ITextureD3D11_GetD3D11Texture(This) CALL_IFACE_METHOD(TextureD3D11, GetD3D11Texture, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
