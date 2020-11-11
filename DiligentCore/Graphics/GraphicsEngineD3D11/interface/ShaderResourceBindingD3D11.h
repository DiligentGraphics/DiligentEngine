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
/// Definition of the Diligent::IShaderResourceBindingD3D11 interface and related data structures

#include "../../GraphicsEngine/interface/ShaderResourceBinding.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {97A6D4AC-D4AF-4AA9-B46C-67417B89026A}
static const struct INTERFACE_ID IID_ShaderResourceBindingD3D11 =
    {0x97a6d4ac, 0xd4af, 0x4aa9, {0xb4, 0x6c, 0x67, 0x41, 0x7b, 0x89, 0x2, 0x6a}};

#define DILIGENT_INTERFACE_NAME IShaderResourceBindingD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

// clang-format off
#define IShaderResourceBindingD3D11InclusiveMethods \
    IShaderResourceBindingInclusiveMethods          \
    /*IShaderResourceBindingD3D11Methods ShaderResourceBindingD3D11*/
// clang-format on

#if DILIGENT_CPP_INTERFACE

/// Exposes Direct3D11-specific functionality of a shader resource binding object.
DILIGENT_BEGIN_INTERFACE(IShaderResourceBindingD3D11, IShaderResourceBinding){};
DILIGENT_END_INTERFACE

#endif

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

struct IShaderResourceBindingD3D11Vtbl
{
    IShaderResourceBindingD3D11InclusiveMethods;
};

typedef struct IShaderResourceBindingD3D11
{
    struct IShaderResourceBindingD3D11Vtbl* pVtbl;
} IShaderResourceBindingD3D11;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
