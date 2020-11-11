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
/// Definition of the Diligent::IShaderD3D interface and related data structures

#include "../../GraphicsEngine/interface/Shader.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// {1EA0898C-1612-457F-B74E-808843D2CBE3}
static const struct INTERFACE_ID IID_ShaderD3D =
    {0x1ea0898c, 0x1612, 0x457f, {0xb7, 0x4e, 0x80, 0x88, 0x43, 0xd2, 0xcb, 0xe3}};


// clang-format off

/// HLSL resource description
struct HLSLShaderResourceDesc DILIGENT_DERIVE(ShaderResourceDesc)

    Uint32 ShaderRegister DEFAULT_INITIALIZER(0);
};
typedef struct HLSLShaderResourceDesc HLSLShaderResourceDesc;

// clang-format on

#define DILIGENT_INTERFACE_NAME IShaderD3D
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderD3DInclusiveMethods \
    IShaderInclusiveMethods;       \
    IShaderD3DMethods ShaderD3D

// clang-format off

/// Exposes Direct3D-specific functionality of a shader object.
DILIGENT_BEGIN_INTERFACE(IShaderD3D, IShader)
{
    /// Returns HLSL shader resource description
    VIRTUAL void METHOD(GetHLSLResource)(THIS_
                                         Uint32                     Index,
                                         HLSLShaderResourceDesc REF ResourceDesc) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IShaderD3D_GetHLSLResource(This, ...) CALL_IFACE_METHOD(ShaderD3D, GetHLSLResource, This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
