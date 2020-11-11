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
/// Definition of the Diligent::IShader interface and related data structures

#include "../../../Primitives/interface/FileStream.h"
#include "../../../Primitives/interface/FlagEnum.h"
#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {2989B45C-143D-4886-B89C-C3271C2DCC5D}
static const INTERFACE_ID IID_Shader =
    {0x2989b45c, 0x143d, 0x4886, {0xb8, 0x9c, 0xc3, 0x27, 0x1c, 0x2d, 0xcc, 0x5d}};

// clang-format off

/// Describes the shader type
DILIGENT_TYPED_ENUM(SHADER_TYPE, Uint32)
{
    SHADER_TYPE_UNKNOWN       = 0x000, ///< Unknown shader type
    SHADER_TYPE_VERTEX        = 0x001, ///< Vertex shader
    SHADER_TYPE_PIXEL         = 0x002, ///< Pixel (fragment) shader
    SHADER_TYPE_GEOMETRY      = 0x004, ///< Geometry shader
    SHADER_TYPE_HULL          = 0x008, ///< Hull (tessellation control) shader
    SHADER_TYPE_DOMAIN        = 0x010, ///< Domain (tessellation evaluation) shader
    SHADER_TYPE_COMPUTE       = 0x020, ///< Compute shader
    SHADER_TYPE_AMPLIFICATION = 0x040, ///< Amplification (task) shader
    SHADER_TYPE_MESH          = 0x080, ///< Mesh shader
    SHADER_TYPE_LAST          = SHADER_TYPE_MESH
};
DEFINE_FLAG_ENUM_OPERATORS(SHADER_TYPE);


/// Describes the shader source code language
DILIGENT_TYPED_ENUM(SHADER_SOURCE_LANGUAGE, Uint32)
{
    /// Default language (GLSL for OpenGL/OpenGLES/Vulkan devices, HLSL for Direct3D11/Direct3D12 devices)
    SHADER_SOURCE_LANGUAGE_DEFAULT = 0,

    /// The source language is HLSL
    SHADER_SOURCE_LANGUAGE_HLSL,

    /// The source language is GLSL
    SHADER_SOURCE_LANGUAGE_GLSL,

    /// The source language is GLSL that should be compiled verbatim

    /// By default the engine prepends GLSL shader source code with platform-specific
    /// definitions. For instance it adds appropriate #version directive (e.g. '#version 430 core' or 
    /// '#version 310 es') so that the same source will work on different versions of desktop OpenGL and OpenGLES.
    /// When SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM is used, the source code will be compiled as is.
    /// Note that shader macros are ignored when compiling GLSL verbatim in OpenGL backend, and an application
    /// should add the macro definitions to the source code.
    SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM
};


/// Describes the shader compiler that will be used to compile the shader source code
DILIGENT_TYPED_ENUM(SHADER_COMPILER, Uint32)
{
    /// Default compiler for specific language and API that is selected as follows:
    ///     - Direct3D11:      legacy HLSL compiler (FXC)
    ///     - Direct3D12:      legacy HLSL compiler (FXC)
    ///     - OpenGL(ES) GLSL: native compiler
    ///     - OpenGL(ES) HLSL: HLSL2GLSL converter and native compiler
    ///     - Vulkan GLSL:     built-in glslang
    ///     - Vulkan HLSL:     built-in glslang (with limitted support for Shader Model 6.x)
    SHADER_COMPILER_DEFAULT = 0,

    /// Built-in glslang compiler for GLSL and HLSL.
    SHADER_COMPILER_GLSLANG,

    /// Modern HLSL compiler (DXC) for Direct3D12 and Vulkan with Shader Model 6.x support.
    SHADER_COMPILER_DXC,
        
    /// Legacy HLSL compiler (FXC) for Direct3D11 and Direct3D12 supporting shader models up to 5.1.
    SHADER_COMPILER_FXC,
};


/// Describes the flags that can be passed over to IShaderSourceInputStreamFactory::CreateInputStream2() function.
DILIGENT_TYPED_ENUM(CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS, Uint32)
{
    /// No flag.
    CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_NONE = 0x00,

    /// Do not output any messages if the file is not found or
    /// other errors occur.
    CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_SILENT = 0x01,
};
DEFINE_FLAG_ENUM_OPERATORS(CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS);

/// Shader description
struct ShaderDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Shader type. See Diligent::SHADER_TYPE.
    SHADER_TYPE ShaderType DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);
};
typedef struct ShaderDesc ShaderDesc;

// clang-format on

// {3EA98781-082F-4413-8C30-B9BA6D82DBB7}
static const INTERFACE_ID IID_IShaderSourceInputStreamFactory =
    {0x3ea98781, 0x82f, 0x4413, {0x8c, 0x30, 0xb9, 0xba, 0x6d, 0x82, 0xdb, 0xb7}};


// clang-format off

#define DILIGENT_INTERFACE_NAME IShaderSourceInputStreamFactory
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderSourceInputStreamFactoryInclusiveMethods \
    IObjectInclusiveMethods;                            \
    IShaderSourceInputStreamFactoryMethods ShaderSourceInputStreamFactory

/// Shader source stream factory interface
DILIGENT_BEGIN_INTERFACE(IShaderSourceInputStreamFactory, IObject)
{
    VIRTUAL void METHOD(CreateInputStream)(THIS_
                                           const Char*   Name,
                                           IFileStream** ppStream) PURE;

    VIRTUAL void METHOD(CreateInputStream2)(THIS_
                                            const Char*                             Name,
                                            CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS Flags,
                                            IFileStream**                           ppStream) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format on

#    define IShaderSourceInputStreamFactory_CreateInputStream(This, ...)  CALL_IFACE_METHOD(ShaderSourceInputStreamFactory, CreateInputStream, This, __VA_ARGS__)
#    define IShaderSourceInputStreamFactory_CreateInputStream2(This, ...) CALL_IFACE_METHOD(ShaderSourceInputStreamFactory, CreateInputStream2, This, __VA_ARGS__)

#endif


struct ShaderMacro
{
    const Char* Name       DEFAULT_INITIALIZER(nullptr);
    const Char* Definition DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    ShaderMacro() noexcept
    {}
    ShaderMacro(const Char* _Name,
                const Char* _Def) noexcept :
        Name{_Name},
        Definition{_Def}
    {}
#endif
};
typedef struct ShaderMacro ShaderMacro;

/// Shader version
struct ShaderVersion
{
    /// Major revision
    Uint8 Major DEFAULT_INITIALIZER(0);

    /// Minor revision
    Uint8 Minor DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    ShaderVersion() noexcept
    {}
    ShaderVersion(Uint8 _Major, Uint8 _Minor) noexcept :
        Major{_Major},
        Minor{_Minor}
    {}

    bool operator==(const ShaderVersion& rhs) const
    {
        return Major == rhs.Major && Minor == rhs.Minor;
    }
#endif
};
typedef struct ShaderVersion ShaderVersion;

/// Shader creation attributes
struct ShaderCreateInfo
{
    /// Source file path

    /// If source file path is provided, Source and ByteCode members must be null
    const Char* FilePath DEFAULT_INITIALIZER(nullptr);

    /// Pointer to the shader source input stream factory.

    /// The factory is used to load the shader source file if FilePath is not null.
    /// It is also used to create additional input streams for shader include files
    IShaderSourceInputStreamFactory* pShaderSourceStreamFactory DEFAULT_INITIALIZER(nullptr);

    /// HLSL->GLSL conversion stream

    /// If HLSL->GLSL converter is used to convert HLSL shader source to
    /// GLSL, this member can provide pointer to the conversion stream. It is useful
    /// when the same file is used to create a number of different shaders. If
    /// ppConversionStream is null, the converter will parse the same file
    /// every time new shader is converted. If ppConversionStream is not null,
    /// the converter will write pointer to the conversion stream to *ppConversionStream
    /// the first time and will use it in all subsequent times.
    /// For all subsequent conversions, FilePath member must be the same, or
    /// new stream will be crated and warning message will be displayed.
    struct IHLSL2GLSLConversionStream** ppConversionStream DEFAULT_INITIALIZER(nullptr);

    /// Shader source

    /// If shader source is provided, FilePath and ByteCode members must be null
    const Char* Source DEFAULT_INITIALIZER(nullptr);

    /// Compiled shader bytecode.

    /// If shader byte code is provided, FilePath and Source members must be null
    /// \note. This option is supported for D3D11, D3D12 and Vulkan backends.
    ///        For D3D11 and D3D12 backends, HLSL bytecode should be provided. Vulkan
    ///        backend expects SPIRV bytecode.
    ///        The bytecode must contain reflection information. If shaders were compiled
    ///        using fxc, make sure that /Qstrip_reflect option is *not* specified.
    ///        HLSL shaders need to be compiled against 4.0 profile or higher.
    const void* ByteCode DEFAULT_INITIALIZER(nullptr);

    /// Size of the compiled shader bytecode

    /// Byte code size (in bytes) must be provided if ByteCode is not null
    size_t ByteCodeSize DEFAULT_INITIALIZER(0);

    /// Shader entry point

    /// This member is ignored if ByteCode is not null
    const Char* EntryPoint DEFAULT_INITIALIZER("main");

    /// Shader macros

    /// This member is ignored if ByteCode is not null
    const ShaderMacro* Macros DEFAULT_INITIALIZER(nullptr);

    /// If set to true, textures will be combined with texture samplers.
    /// The CombinedSamplerSuffix member defines the suffix added to the texture variable
    /// name to get corresponding sampler name. When using combined samplers,
    /// the sampler assigned to the shader resource view is automatically set when
    /// the view is bound. Otherwise samplers need to be explicitly set similar to other
    /// shader variables.
    bool UseCombinedTextureSamplers DEFAULT_INITIALIZER(false);

    /// If UseCombinedTextureSamplers is true, defines the suffix added to the
    /// texture variable name to get corresponding sampler name.  For example,
    /// for default value "_sampler", a texture named "tex" will be combined
    /// with sampler named "tex_sampler".
    /// If UseCombinedTextureSamplers is false, this member is ignored.
    const Char* CombinedSamplerSuffix DEFAULT_INITIALIZER("_sampler");

    /// Shader description. See Diligent::ShaderDesc.
    ShaderDesc Desc;

    /// Shader source language. See Diligent::SHADER_SOURCE_LANGUAGE.
    SHADER_SOURCE_LANGUAGE SourceLanguage DEFAULT_INITIALIZER(SHADER_SOURCE_LANGUAGE_DEFAULT);

    /// Shader compiler. See Diligent::SHADER_COMPILER.
    SHADER_COMPILER ShaderCompiler DEFAULT_INITIALIZER(SHADER_COMPILER_DEFAULT);

    /// HLSL shader model to use when compiling the shader. When default value
    /// is given (0, 0), the engine will attempt to use the highest HLSL shader model
    /// supported by the device. If the shader is created from the byte code, this value
    /// has no effect.
    ///
    /// \note When HLSL source is converted to GLSL, corresponding GLSL/GLESSL version will be used.
    ShaderVersion HLSLVersion DEFAULT_INITIALIZER({});

    /// GLSL version to use when creating the shader. When default value
    /// is given (0, 0), the engine will attempt to use the highest GLSL version
    /// supported by the device.
    ShaderVersion GLSLVersion DEFAULT_INITIALIZER({});

    /// GLES shading language version to use when creating the shader. When default value
    /// is given (0, 0), the engine will attempt to use the highest GLESSL version
    /// supported by the device.
    ShaderVersion GLESSLVersion DEFAULT_INITIALIZER({});


    /// Memory address where pointer to the compiler messages data blob will be written

    /// The buffer contains two null-terminated strings. The first one is the compiler
    /// output message. The second one is the full shader source code including definitions added
    /// by the engine. Data blob object must be released by the client.
    IDataBlob** ppCompilerOutput DEFAULT_INITIALIZER(nullptr);
};
typedef struct ShaderCreateInfo ShaderCreateInfo;

// clang-format off
/// Describes shader resource type
DILIGENT_TYPED_ENUM(SHADER_RESOURCE_TYPE, Uint8)
{
    /// Shader resource type is unknown
    SHADER_RESOURCE_TYPE_UNKNOWN = 0,

    /// Constant (uniform) buffer
    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,

    /// Shader resource view of a texture (sampled image)
    SHADER_RESOURCE_TYPE_TEXTURE_SRV,

    /// Shader resource view of a buffer (read-only storage image)
    SHADER_RESOURCE_TYPE_BUFFER_SRV,

    /// Unordered access view of a texture (sotrage image)
    SHADER_RESOURCE_TYPE_TEXTURE_UAV,

    /// Unordered access view of a buffer (storage buffer)
    SHADER_RESOURCE_TYPE_BUFFER_UAV,

    /// Sampler (separate sampler)
    SHADER_RESOURCE_TYPE_SAMPLER,

    /// Input attachment in a render pass
    SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT,

    SHADER_RESOURCE_TYPE_LAST = SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT
};
// clang-format on

/// Shader resource description
struct ShaderResourceDesc
{
#if DILIGENT_CPP_INTERFACE
    ShaderResourceDesc() noexcept
    {}

    ShaderResourceDesc(const char*          _Name,
                       SHADER_RESOURCE_TYPE _Type,
                       Uint32               _ArraySize) noexcept :
        Name{_Name},
        Type{_Type},
        ArraySize{_ArraySize}
    {}
#endif

    // clang-format off
    /// Shader resource name
    const char*          Name      DEFAULT_INITIALIZER(nullptr);

    /// Shader resource type, see Diligent::SHADER_RESOURCE_TYPE.
    SHADER_RESOURCE_TYPE Type      DEFAULT_INITIALIZER(SHADER_RESOURCE_TYPE_UNKNOWN);

    /// Array size. For non-array resource this value is 1.
    Uint32               ArraySize DEFAULT_INITIALIZER(0);
    // clang-format on
};
typedef struct ShaderResourceDesc ShaderResourceDesc;

#define DILIGENT_INTERFACE_NAME IShader
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderInclusiveMethods    \
    IDeviceObjectInclusiveMethods; \
    IShaderMethods Shader

// clang-format off

/// Shader interface
DILIGENT_BEGIN_INTERFACE(IShader, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the shader description
    virtual const ShaderDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Returns the total number of shader resources
    VIRTUAL Uint32 METHOD(GetResourceCount)(THIS) CONST PURE;

    /// Returns the pointer to the array of shader resources
    VIRTUAL void METHOD(GetResourceDesc)(THIS_
                                         Uint32 Index,
                                         ShaderResourceDesc REF ResourceDesc) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IShader_GetDesc(This) (const struct ShaderDesc*)IDeviceObject_GetDesc(This)

#    define IShader_GetResourceCount(This)     CALL_IFACE_METHOD(Shader, GetResourceCount, This)
#    define IShader_GetResourceDesc(This, ...) CALL_IFACE_METHOD(Shader, GetResourceDesc,  This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
