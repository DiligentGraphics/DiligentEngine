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

// clang-format off

/// \file
/// Definition of the Diligent::IBufferView interface and related data structures

#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {E2E83490-E9D2-495B-9A83-ABB413A38B07}
static const struct INTERFACE_ID IID_BufferView =
    {0xe2e83490, 0xe9d2, 0x495b, {0x9a, 0x83, 0xab, 0xb4, 0x13, 0xa3, 0x8b, 0x7}};

/// Buffer format description
struct BufferFormat
{
    /// Type of components. For a formatted buffer views, this value cannot be VT_UNDEFINED
    VALUE_TYPE ValueType    DEFAULT_INITIALIZER(VT_UNDEFINED);

    /// Number of components. Allowed values: 1, 2, 3, 4. 
    /// For a formatted buffer, this value cannot be 0
    Uint8 NumComponents     DEFAULT_INITIALIZER(0);

    /// For signed and unsigned integer value types 
    /// (VT_INT8, VT_INT16, VT_INT32, VT_UINT8, VT_UINT16, VT_UINT32)
    /// indicates if the value should be normalized to [-1,+1] or 
    /// [0, 1] range respectively. For floating point types
    /// (VT_FLOAT16 and VT_FLOAT32), this member is ignored.
    Bool IsNormalized       DEFAULT_INITIALIZER(False);


#if DILIGENT_CPP_INTERFACE
    // We have to explicitly define constructors because otherwise Apple's clang fails to compile the following legitimate code:
    //     BufferFormat{VT_FLOAT32, 4}
    
    BufferFormat()noexcept{}

    BufferFormat(VALUE_TYPE _ValueType,
                 Uint8      _NumComponents,
                 Bool       _IsNormalized   = BufferFormat{}.IsNormalized)noexcept : 
        ValueType     {_ValueType    },
        NumComponents {_NumComponents},
        IsNormalized  {_IsNormalized }
    {}


    /// Tests if two structures are equivalent
    bool operator == (const BufferFormat& RHS)const
    {
        return ValueType     == RHS.ValueType && 
               NumComponents == RHS.NumComponents &&
               IsNormalized  == RHS.IsNormalized;
    }
#endif
};
typedef struct BufferFormat BufferFormat;

/// Buffer view description
struct BufferViewDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// View type. See Diligent::BUFFER_VIEW_TYPE for details.
    BUFFER_VIEW_TYPE ViewType DEFAULT_INITIALIZER(BUFFER_VIEW_UNDEFINED);

    /// Format of the view. This member is only used for formatted and raw buffers.
    /// To create raw view of a raw buffer, set Format.ValueType member to VT_UNDEFINED
    /// (default value).
    struct BufferFormat Format;

    /// Offset in bytes from the beginnig of the buffer to the start of the
    /// buffer region referenced by the view
    Uint32 ByteOffset       DEFAULT_INITIALIZER(0);

    /// Size in bytes of the referenced buffer region
    Uint32 ByteWidth        DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    BufferViewDesc()noexcept{}

    explicit
    BufferViewDesc(BUFFER_VIEW_TYPE _ViewType,
                   BufferFormat     _Format     = BufferViewDesc{}.Format,
                   Uint32           _ByteOffset = BufferViewDesc{}.ByteOffset,
                   Uint32           _ByteWidth  = BufferViewDesc{}.ByteWidth)noexcept :
        ViewType    {_ViewType  },
        Format      {_Format    },
        ByteOffset  {_ByteOffset},
        ByteWidth   {_ByteWidth }
    {}

    /// Comparison operator tests if two structures are equivalent

    /// \param [in] RHS - reference to the structure to perform comparison with
    /// \return 
    /// - True if all members of the two structures are equal.
    /// - False otherwise
    /// \remarks
    /// The operator ignores DeviceObjectAttribs::Name field.
    bool operator==(const BufferViewDesc& RHS) const
    {
               // Name is primarily used for debug purposes and does not affect the view.
               // It is ignored in comparison operation.
        return //strcmp(Name, RHS.Name) == 0 &&
               ViewType  == RHS.ViewType   &&
               ByteOffset== RHS.ByteOffset &&
               ByteWidth == RHS.ByteWidth  &&
               Format    == RHS.Format;
    }
#endif
};
typedef struct BufferViewDesc BufferViewDesc;

#define DILIGENT_INTERFACE_NAME IBufferView
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferViewInclusiveMethods \
    IDeviceObjectInclusiveMethods;  \
    IBufferViewMethods BufferView

/// Buffer view interface

/// To create a buffer view, call IBuffer::CreateView().
/// \remarks
/// Buffer view holds strong references to the buffer. The buffer
/// will not be destroyed until all views are released.
DILIGENT_BEGIN_INTERFACE(IBufferView, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the buffer view description used to create the object
    virtual const BufferViewDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Returns pointer to the referenced buffer object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    VIRTUAL struct IBuffer* METHOD(GetBuffer)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IBufferView_GetDesc(This) (const struct BufferViewDesc*)IDeviceObject_GetDesc(This)

#    define IBufferView_GetBuffer(This) CALL_IFACE_METHOD(BufferView, GetBuffer, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
