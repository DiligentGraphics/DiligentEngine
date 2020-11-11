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
/// Definition of the Diligent::IFenceD3D11 interface

#include "../../GraphicsEngine/interface/Fence.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {45F2BE28-652B-4180-B6E4-E75F83F63CC7}
static const struct INTERFACE_ID IID_FenceD3D11 =
    {0x45f2be28, 0x652b, 0x4180, {0xb6, 0xe4, 0xe7, 0x5f, 0x83, 0xf6, 0x3c, 0xc7}};

#define DILIGENT_INTERFACE_NAME IFenceD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

// clang-format off
#define IFenceD3D11InclusiveMethods \
    IFenceInclusiveMethods          \
    /*IFenceD3D11Methods FenceD3D11*/
// clang-format on

#if DILIGENT_CPP_INTERFACE

/// Exposes Direct3D11-specific functionality of a fence object.
class IFenceD3D11 : public IFence
{
};

#endif

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

struct IFenceD3D11Vtbl
{
    IFenceD3D11InclusiveMethods;
};

typedef struct IFenceD3D11
{
    struct IFenceD3D11Vtbl* pVtbl;
} IFenceD3D11;

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
