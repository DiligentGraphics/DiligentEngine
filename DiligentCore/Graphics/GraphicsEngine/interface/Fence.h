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
/// Defines Diligent::IFence interface and related data structures

#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {3B19184D-32AB-4701-84F4-9A0C03AE1672}
static const INTERFACE_ID IID_Fence =
    {0x3b19184d, 0x32ab, 0x4701, {0x84, 0xf4, 0x9a, 0xc, 0x3, 0xae, 0x16, 0x72}};

// clang-format off
/// Fence description
struct FenceDesc DILIGENT_DERIVE(DeviceObjectAttribs)
};
typedef struct FenceDesc FenceDesc;

// clang-format off

#define DILIGENT_INTERFACE_NAME IFence
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IFenceInclusiveMethods      \
    IDeviceObjectInclusiveMethods;  \
    IFenceMethods Fence

/// Fence interface

/// Defines the methods to manipulate a fence object
///
/// \remarks When a fence that was previously signaled by IDeviceContext::SignalFence() is destroyed,
///          it may block the GPU until all prior commands have completed execution.
DILIGENT_BEGIN_INTERFACE(IFence, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the fence description used to create the object
    virtual const FenceDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Returns the last completed value signaled by the GPU

    /// \remarks This method is not thread safe (even if the fence object is protected by mutex)
    ///          and must only be called by the same thread that signals the fence via
    ///          IDeviceContext::SignalFence().
    VIRTUAL Uint64 METHOD(GetCompletedValue)(THIS) PURE;

    /// Resets the fence to the specified value.
    VIRTUAL void METHOD(Reset)(THIS_
                               Uint64 Value) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IFence_GetDesc(This) (const struct FenceDesc*)IDeviceObject_GetDesc(This)

#    define IFence_GetCompletedValue(This) CALL_IFACE_METHOD(Fence, GetCompletedValue, This)
#    define IFence_Reset(This, ...)        CALL_IFACE_METHOD(Fence, Reset,             This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
