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
/// Definition of the Diligent::IShaderD3D11 interface

#include "../../GraphicsEngineD3DBase/interface/ShaderD3D.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {C513E83E-B037-405B-8B49-BF8F5C220DEE}
static const struct INTERFACE_ID IID_ShaderD3D11 =
    {0xc513e83e, 0xb037, 0x405b, {0x8b, 0x49, 0xbf, 0x8f, 0x5c, 0x22, 0xd, 0xee}};

#define DILIGENT_INTERFACE_NAME IShaderD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderD3D11InclusiveMethods \
    IShaderD3DInclusiveMethods;      \
    IShaderD3D11Methods ShaderD3D11

/// Exposes Direct3D11-specific functionality of a shader object.
DILIGENT_BEGIN_INTERFACE(IShaderD3D11, IShaderD3D)
{
    /// Returns a pointer to the ID3D11DeviceChild interface of the internal Direct3D11 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ID3D11DeviceChild* METHOD(GetD3D11Shader)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IShaderD3D11_GetD3D11Shader(This) CALL_IFACE_METHOD(ShaderD3D11, GetD3D11Shader, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
