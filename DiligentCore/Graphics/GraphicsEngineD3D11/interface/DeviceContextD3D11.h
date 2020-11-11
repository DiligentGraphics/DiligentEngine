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
/// Definition of the Diligent::IDeviceContextD3D11 interface

#include "../../GraphicsEngine/interface/DeviceContext.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {F0EE0335-C8AB-4EC1-BB15-B8EE5F003B99}
static const struct INTERFACE_ID IID_DeviceContextD3D11 =
    {0xf0ee0335, 0xc8ab, 0x4ec1, {0xbb, 0x15, 0xb8, 0xee, 0x5f, 0x0, 0x3b, 0x99}};

#define DILIGENT_INTERFACE_NAME IDeviceContextD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceContextD3D11InclusiveMethods \
    IDeviceContextInclusiveMethods;         \
    IDeviceContextD3D11Methods DeviceContextD3D11

/// Exposes Direct3D11-specific functionality of a device context.
DILIGENT_BEGIN_INTERFACE(IDeviceContextD3D11, IDeviceContext)
{
    /// Returns a pointer to the ID3D11DeviceContext interface of the internal Direct3D11 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ID3D11DeviceContext* METHOD(GetD3D11DeviceContext)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IDeviceContextD3D11_GetD3D11DeviceContext(This) CALL_IFACE_METHOD(DeviceContextD3D11, GetD3D11DeviceContext, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
