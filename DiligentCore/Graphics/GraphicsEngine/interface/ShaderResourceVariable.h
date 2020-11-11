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
/// Definition of the Diligent::IShaderResourceVariable interface and related data structures

#include "../../../Primitives/interface/BasicTypes.h"
#include "../../../Primitives/interface/Object.h"
#include "DeviceObject.h"
#include "Shader.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// {0D57DF3F-977D-4C8F-B64C-6675814BC80C}
static const INTERFACE_ID IID_ShaderResourceVariable =
    {0xd57df3f, 0x977d, 0x4c8f, {0xb6, 0x4c, 0x66, 0x75, 0x81, 0x4b, 0xc8, 0xc}};

// clang-format off

/// Describes the type of the shader resource variable
DILIGENT_TYPED_ENUM(SHADER_RESOURCE_VARIABLE_TYPE, Uint8)
{
    /// Shader resource bound to the variable is the same for all SRB instances.
    /// It must be set *once* directly through Pipeline State object.
    SHADER_RESOURCE_VARIABLE_TYPE_STATIC = 0,

    /// Shader resource bound to the variable is specific to the shader resource binding
    /// instance (see Diligent::IShaderResourceBinding). It must be set *once* through
    /// Diligent::IShaderResourceBinding interface. It cannot be set through Diligent::IPipelineState
    /// interface and cannot be change once bound.
    SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE,

    /// Shader variable binding is dynamic. It can be set multiple times for every instance of shader resource
    /// binding (see Diligent::IShaderResourceBinding). It cannot be set through Diligent::IPipelineState interface.
    SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC,

    /// Total number of shader variable types
    SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES
};

#ifdef __cplusplus
static_assert(SHADER_RESOURCE_VARIABLE_TYPE_STATIC == 0 && SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE == 1 && SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC == 2 && SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES == 3, "BIND_SHADER_RESOURCES_UPDATE_* flags rely on shader variable SHADER_RESOURCE_VARIABLE_TYPE_* values being 0,1,2");
#endif

/// Shader resource binding flags
DILIGENT_TYPED_ENUM(BIND_SHADER_RESOURCES_FLAGS, Uint32)
{
    /// Indicates that static shader variable bindings are to be updated.
    BIND_SHADER_RESOURCES_UPDATE_STATIC = (0x01 << SHADER_RESOURCE_VARIABLE_TYPE_STATIC),

    /// Indicates that mutable shader variable bindings are to be updated.
    BIND_SHADER_RESOURCES_UPDATE_MUTABLE = (0x01 << SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE),

    /// Indicates that dynamic shader variable bindings are to be updated.
    BIND_SHADER_RESOURCES_UPDATE_DYNAMIC = (0x01 << SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC),

    /// Indicates that all shader variable types (static, mutable and dynamic) are to be updated.
    /// \note If none of BIND_SHADER_RESOURCES_UPDATE_STATIC, BIND_SHADER_RESOURCES_UPDATE_MUTABLE,
    ///       and BIND_SHADER_RESOURCES_UPDATE_DYNAMIC flags are set, all variable types are updated
    ///       as if BIND_SHADER_RESOURCES_UPDATE_ALL was specified.
    BIND_SHADER_RESOURCES_UPDATE_ALL = (BIND_SHADER_RESOURCES_UPDATE_STATIC | BIND_SHADER_RESOURCES_UPDATE_MUTABLE | BIND_SHADER_RESOURCES_UPDATE_DYNAMIC),

    /// If this flag is specified, all existing bindings will be preserved and
    /// only unresolved ones will be updated.
    /// If this flag is not specified, every shader variable will be
    /// updated if the mapping contains corresponding resource.
    BIND_SHADER_RESOURCES_KEEP_EXISTING = 0x08,

    /// If this flag is specified, all shader bindings are expected
    /// to be resolved after the call. If this is not the case, debug message
    /// will be displayed.
    /// \note Only these variables are verified that are being updated by setting
    ///       BIND_SHADER_RESOURCES_UPDATE_STATIC, BIND_SHADER_RESOURCES_UPDATE_MUTABLE, and
    ///       BIND_SHADER_RESOURCES_UPDATE_DYNAMIC flags.
    BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED = 0x10
};

// clang-format on

#define DILIGENT_INTERFACE_NAME IShaderResourceVariable
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderResourceVariableInclusiveMethods \
    IObjectInclusiveMethods;                    \
    IShaderResourceVariableMethods ShaderResourceVariable

// clang-format off

/// Shader resource variable
DILIGENT_BEGIN_INTERFACE(IShaderResourceVariable, IObject)
{
    /// Binds resource to the variable

    /// \remark The method performs run-time correctness checks.
    ///         For instance, shader resource view cannot
    ///         be assigned to a constant buffer variable.
    VIRTUAL void METHOD(Set)(THIS_
                             IDeviceObject* pObject) PURE;

    /// Binds resource array to the variable

    /// \param [in] ppObjects    - pointer to the array of objects
    /// \param [in] FirstElement - first array element to set
    /// \param [in] NumElements  - number of objects in ppObjects array
    ///
    /// \remark The method performs run-time correctness checks.
    ///         For instance, shader resource view cannot
    ///         be assigned to a constant buffer variable.
    VIRTUAL void METHOD(SetArray)(THIS_
                                  IDeviceObject* const* ppObjects,
                                  Uint32                FirstElement,
                                  Uint32                NumElements) PURE;

    /// Returns the shader resource variable type
    VIRTUAL SHADER_RESOURCE_VARIABLE_TYPE METHOD(GetType)(THIS) CONST PURE;

    /// Returns shader resource description. See Diligent::ShaderResourceDesc.
    VIRTUAL void METHOD(GetResourceDesc)(THIS_
                                         ShaderResourceDesc REF ResourceDesc) CONST PURE;

    /// Returns the variable index that can be used to access the variable.
    VIRTUAL Uint32 METHOD(GetIndex)(THIS) CONST PURE;

    /// Returns true if non-null resource is bound to this variable.

    /// \param [in] ArrayIndex - Resource array index. Must be 0 for
    ///                          non-array variables.
    VIRTUAL bool METHOD(IsBound)(THIS_
                                 Uint32 ArrayIndex) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IShaderResourceVariable_Set(This, ...)             CALL_IFACE_METHOD(ShaderResourceVariable, Set,             This, __VA_ARGS__)
#    define IShaderResourceVariable_SetArray(This, ...)        CALL_IFACE_METHOD(ShaderResourceVariable, SetArray,        This, __VA_ARGS__)
#    define IShaderResourceVariable_GetType(This)              CALL_IFACE_METHOD(ShaderResourceVariable, GetType,         This)
#    define IShaderResourceVariable_GetResourceDesc(This, ...) CALL_IFACE_METHOD(ShaderResourceVariable, GetResourceDesc, This, __VA_ARGS__)
#    define IShaderResourceVariable_GetIndex(This)             CALL_IFACE_METHOD(ShaderResourceVariable, GetIndex,        This)
#    define IShaderResourceVariable_IsBound(This, ...)         CALL_IFACE_METHOD(ShaderResourceVariable, IsBound,         This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
