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
/// Definition of the Diligent::IBufferViewD3D11 interface

#include "../../GraphicsEngine/interface/BufferView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {6ABA95FC-CD7D-4C03-8CAE-AFC45F9696B7}
static const struct INTERFACE_ID IID_BufferViewD3D11 =
    {0x6aba95fc, 0xcd7d, 0x4c03, {0x8c, 0xae, 0xaf, 0xc4, 0x5f, 0x96, 0x96, 0xb7}};

#define DILIGENT_INTERFACE_NAME IBufferViewD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferViewD3D11InclusiveMethods \
    IBufferViewInclusiveMethods;         \
    IBufferViewD3D11Methods BufferViewD3D11

/// Exposes Direct3D11-specific functionality of a buffer view object.
DILIGENT_BEGIN_INTERFACE(IBufferViewD3D11, IBufferView)
{
    /// Returns a pointer to the ID3D11View interface of the internal Direct3D11 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL ID3D11View* METHOD(GetD3D11View)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IBufferViewD3D11_GetD3D11View(This) CALL_IFACE_METHOD(BufferViewD3D11, GetD3D11View, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
