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
/// Definition of the Diligent::IShaderGL interface

#include "../../GraphicsEngine/interface/Shader.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {2FF3C191-285B-4E6C-BD0B-D084DDEA6FCC}
static const INTERFACE_ID IID_ShaderGL =
    {0x2ff3c191, 0x285b, 0x4e6c, {0xbd, 0xb, 0xd0, 0x84, 0xdd, 0xea, 0x6f, 0xcc}};

#define DILIGENT_INTERFACE_NAME IShaderGL
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderGLInclusiveMethods \
    IShaderInclusiveMethods
//IShaderGLMethods ShaderGL

#if DILIGENT_CPP_INTERFACE

/// Exposes OpenGL-specific functionality of a shader object.
DILIGENT_BEGIN_INTERFACE(IShaderGL, IShader){};
DILIGENT_END_INTERFACE

#endif

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

typedef struct IShaderGLVtbl
{
    IShaderGLInclusiveMethods;
} IShaderGLVtbl;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
