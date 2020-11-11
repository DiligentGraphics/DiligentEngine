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
/// Definition of the Diligent::IBufferViewD3D12 interface

#include "../../GraphicsEngine/interface/BufferView.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {09643F2F-40D4-4076-B086-9E5CDC2CC4FC}
static const INTERFACE_ID IID_BufferViewD3D12 =
    {0x9643f2f, 0x40d4, 0x4076, {0xb0, 0x86, 0x9e, 0x5c, 0xdc, 0x2c, 0xc4, 0xfc}};

#define DILIGENT_INTERFACE_NAME IBufferViewD3D12
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferViewD3D12InclusiveMethods \
    IBufferViewInclusiveMethods;         \
    IBufferViewD3D12Methods BufferViewD3D12

/// Exposes Direct3D12-specific functionality of a buffer view object.
DILIGENT_BEGIN_INTERFACE(IBufferViewD3D12, IBufferView)
{
    /// Returns CPU descriptor handle of the buffer view.
    VIRTUAL D3D12_CPU_DESCRIPTOR_HANDLE METHOD(GetCPUDescriptorHandle)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IBufferViewD3D12_GetCPUDescriptorHandle(This) CALL_IFACE_METHOD(BufferViewD3D12, GetCPUDescriptorHandle, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
