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
/// Definition of the Diligent::IShaderD3D12 interface

#include "../../GraphicsEngineD3DBase/interface/ShaderD3D.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {C059B160-7F31-4029-943D-0996B98EE79A}
static const INTERFACE_ID IID_ShaderD3D12 =
    {0xc059b160, 0x7f31, 0x4029, {0x94, 0x3d, 0x9, 0x96, 0xb9, 0x8e, 0xe7, 0x9a}};

#define DILIGENT_INTERFACE_NAME IShaderD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

// clang-format off
#define IShaderD3D12InclusiveMethods \
    IShaderD3DInclusiveMethods       \
    /*IShaderD3D12Methods ShaderD3D12*/
// clang-format on

#if DILIGENT_CPP_INTERFACE

/// Exposes Direct3D12-specific functionality of a shader object.
DILIGENT_BEGIN_INTERFACE(IShaderD3D12, IShaderD3D){
    /// Returns a pointer to the ID3D12DeviceChild interface of the internal Direct3D12 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    //virtual ID3D12DeviceChild* GetD3D12Shader() = 0;
};
DILIGENT_END_INTERFACE

#endif

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

struct IShaderD3D12Vtbl
{
    IShaderD3D12InclusiveMethods;
};

typedef struct IShaderD3D12
{
    struct IShaderD3D12Vtbl* pVtbl;
} IShaderD3D12;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
