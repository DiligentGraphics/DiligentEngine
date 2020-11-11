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
/// Defines Diligent::IDeviceObject interface

#include "../../../Primitives/interface/Object.h"
#include "GraphicsTypes.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// {5B4CCA0B-5075-4230-9759-F48769EE5502}
static const INTERFACE_ID IID_DeviceObject =
    {0x5b4cca0b, 0x5075, 0x4230, {0x97, 0x59, 0xf4, 0x87, 0x69, 0xee, 0x55, 0x2}};

#define DILIGENT_INTERFACE_NAME IDeviceObject
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"
#define IDeviceObjectInclusiveMethods \
    IObjectInclusiveMethods;          \
    IDeviceObjectMethods DeviceObject

/// Base interface for all objects created by the render device Diligent::IRenderDevice
DILIGENT_BEGIN_INTERFACE(IDeviceObject, IObject)
{
    /// Returns the object description
    VIRTUAL const DeviceObjectAttribs REF METHOD(GetDesc)(THIS) CONST PURE;


    /// Returns unique identifier assigned to an object

    /// \remarks Unique identifiers can be used to reliably check if two objects are identical.
    ///          Note that the engine resuses memory reclaimed after an object has been released.
    ///          For example, if a texture object is released and then another texture is created,
    ///          the engine may return the same pointer, so pointer-to-pointer comparisons are not
    ///          reliable. Unique identifiers, on the other hand, are guaranteed to be, well, unique.
    ///
    ///          Unique identifiers are object-specifics, so, for instance, buffer identifiers
    ///          are not comparable to texture identifiers.
    ///
    ///          Unique identifiers are only meaningful within one session. After an application
    ///          restarts, all identifiers become invalid.
    ///
    ///          Valid identifiers are always positive values. Zero and negative values can never be
    ///          assigned to an object and are always guaranteed to be invalid.
    VIRTUAL Int32 METHOD(GetUniqueID)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IDeviceObject_GetDesc(This)     CALL_IFACE_METHOD(DeviceObject, GetDesc,     This)
#    define IDeviceObject_GetUniqueID(This) CALL_IFACE_METHOD(DeviceObject, GetUniqueID, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
