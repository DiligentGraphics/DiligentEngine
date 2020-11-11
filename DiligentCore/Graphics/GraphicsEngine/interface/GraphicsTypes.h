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
/// Contains basic graphics engine type defintions

#include "../../../Primitives/interface/BasicTypes.h"
#include "../../../Primitives/interface/DebugOutput.h"
#include "../../../Primitives/interface/FlagEnum.h"
#include "../../../Platforms/interface/NativeWindow.h"
#include "APIInfo.h"

/// Graphics engine namespace
DILIGENT_BEGIN_NAMESPACE(Diligent)

struct ITexture;
struct IBuffer;

/// Value type

/// This enumeration describes value type. It is used by
/// - BufferDesc structure to describe value type of a formatted buffer
/// - DrawAttribs structure to describe index type for an indexed draw call
DILIGENT_TYPED_ENUM(VALUE_TYPE, Uint8)
{
    VT_UNDEFINED = 0, ///< Undefined type
    VT_INT8,          ///< Signed 8-bit integer
    VT_INT16,         ///< Signed 16-bit integer
    VT_INT32,         ///< Signed 32-bit integer
    VT_UINT8,         ///< Unsigned 8-bit integer
    VT_UINT16,        ///< Unsigned 16-bit integer
    VT_UINT32,        ///< Unsigned 32-bit integer
    VT_FLOAT16,       ///< Half-precision 16-bit floating point
    VT_FLOAT32,       ///< Full-precision 32-bit floating point
    VT_NUM_TYPES      ///< Helper value storing total number of types in the enumeration
};

/// Resource binding flags

/// [D3D11_BIND_FLAG]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_bind_flag
///
/// This enumeration describes which parts of the pipeline a resource can be bound to.
/// It generally mirrors [D3D11_BIND_FLAG][] enumeration. It is used by
/// - BufferDesc to describe bind flags for a buffer
/// - TextureDesc to describe bind flags for a texture
DILIGENT_TYPED_ENUM(BIND_FLAGS, Uint32)
{
    BIND_NONE               = 0x0L, ///< Undefined binding
    BIND_VERTEX_BUFFER	    = 0x1L, ///< A buffer can be bound as a vertex buffer
    BIND_INDEX_BUFFER	    = 0x2L, ///< A buffer can be bound as an index buffer
    BIND_UNIFORM_BUFFER	    = 0x4L, ///< A buffer can be bound as a uniform buffer
                                    ///  \warning This flag may not be combined with any other bind flag
    BIND_SHADER_RESOURCE	= 0x8L, ///< A buffer or a texture can be bound as a shader resource
                                    ///  \warning This flag cannot be used with MAP_WRITE_NO_OVERWRITE flag 
    BIND_STREAM_OUTPUT	    = 0x10L,///< A buffer can be bound as a target for stream output stage
    BIND_RENDER_TARGET	    = 0x20L,///< A texture can be bound as a render target
    BIND_DEPTH_STENCIL	    = 0x40L,///< A texture can be bound as a depth-stencil target
    BIND_UNORDERED_ACCESS	= 0x80L,///< A buffer or a texture can be bound as an unordered access view
    BIND_INDIRECT_DRAW_ARGS	= 0x100L,///< A buffer can be bound as the source buffer for indirect draw commands
    BIND_INPUT_ATTACHMENT   = 0x200L ///< A texture can be used as render pass input attachment
};
DEFINE_FLAG_ENUM_OPERATORS(BIND_FLAGS)

/// Resource usage

/// [D3D11_USAGE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage
/// This enumeration describes expected resource usage. It generally mirrors [D3D11_USAGE] enumeration.
/// The enumeration is used by
/// - BufferDesc to describe usage for a buffer
/// - TextureDesc to describe usage for a texture
DILIGENT_TYPED_ENUM(USAGE, Uint8)
{
    /// A resource that can only be read by the GPU. It cannot be written by the GPU, 
    /// and cannot be accessed at all by the CPU. This type of resource must be initialized 
    /// when it is created, since it cannot be changed after creation. \n
    /// D3D11 Counterpart: D3D11_USAGE_IMMUTABLE. OpenGL counterpart: GL_STATIC_DRAW
    /// \remarks Static buffers do not allow CPU access and must use CPU_ACCESS_NONE flag.
    USAGE_IMMUTABLE = 0, 

    /// A resource that requires read and write access by the GPU and can also be occasionally
    /// written by the CPU.  \n
    /// D3D11 Counterpart: D3D11_USAGE_DEFAULT. OpenGL counterpart: GL_DYNAMIC_DRAW.
    /// \remarks Default buffers do not allow CPU access and must use CPU_ACCESS_NONE flag.
    USAGE_DEFAULT,

    /// A resource that can be read by the GPU and written at least once per frame by the CPU.  \n
    /// D3D11 Counterpart: D3D11_USAGE_DYNAMIC. OpenGL counterpart: GL_STREAM_DRAW
    /// \remarks Dynamic buffers must use CPU_ACCESS_WRITE flag.
    USAGE_DYNAMIC,

    /// A resource that facilitates transferring data between GPU and CPU. \n
    /// D3D11 Counterpart: D3D11_USAGE_STAGING. OpenGL counterpart: GL_STATIC_READ or
    /// GL_STATIC_COPY depending on the CPU access flags.
    /// \remarks Staging buffers must use exactly one of CPU_ACCESS_WRITE or CPU_ACCESS_READ flags.
    USAGE_STAGING,

    /// A resource residing in a unified memory (e.g. memory shared between CPU and GPU),
    /// that can be read and written by GPU and can also be directly accessed by CPU.
    ///
    /// \remarks An application should check if unified memory is available on the device by quering
    ///          the device caps (see Diligent::IRenderDevice::GetDeviceCaps and Diligent::GraphicsAdapterInfo).
    ///          If there is no unified memory, an application should choose another usage type (typically, USAGE_DEFAULT).
    /// 
    ///          Unified resources must use at least one of CPU_ACCESS_WRITE or CPU_ACCESS_READ flags.
    ///          An application should check supported unified memory CPU access types by quering the device caps.
    ///          (see Diligent::GraphicsAdapterInfo::UnifiedMemoryCPUAccess).
    USAGE_UNIFIED,

    /// Helper value indicating the total number of elements in the enum
    USAGE_NUM_USAGES
};

/// Allowed CPU access mode flags when mapping a resource
    
/// The enumeration is used by
/// - BufferDesc to describe CPU access mode for a buffer
/// - TextureDesc to describe CPU access mode for a texture
/// \note Only USAGE_DYNAMIC resources can be mapped
DILIGENT_TYPED_ENUM(CPU_ACCESS_FLAGS, Uint8)
{
    CPU_ACCESS_NONE  = 0x00, ///< No CPU access
    CPU_ACCESS_READ  = 0x01, ///< A resource can be mapped for reading
    CPU_ACCESS_WRITE = 0x02  ///< A resource can be mapped for writing
};
DEFINE_FLAG_ENUM_OPERATORS(CPU_ACCESS_FLAGS)

/// Resource mapping type

/// [D3D11_MAP]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_map
/// Describes how a mapped resource will be accessed. This enumeration generally
/// mirrors [D3D11_MAP][] enumeration. It is used by
/// - IBuffer::Map to describe buffer mapping type
/// - ITexture::Map to describe texture mapping type
DILIGENT_TYPED_ENUM(MAP_TYPE, Uint8)
{
    /// The resource is mapped for reading. \n
    /// D3D11 counterpart: D3D11_MAP_READ. OpenGL counterpart: GL_MAP_READ_BIT
    MAP_READ = 0x01,       

    /// The resource is mapped for writing. \n
    /// D3D11 counterpart: D3D11_MAP_WRITE. OpenGL counterpart: GL_MAP_WRITE_BIT
    MAP_WRITE = 0x02,          

    /// The resource is mapped for reading and writing. \n
    /// D3D11 counterpart: D3D11_MAP_READ_WRITE. OpenGL counterpart: GL_MAP_WRITE_BIT | GL_MAP_READ_BIT
    MAP_READ_WRITE = 0x03
};

/// Special map flags

/// Describes special arguments for a map operation.
/// This enumeration is used by
/// - IBuffer::Map to describe buffer mapping flags
/// - ITexture::Map to describe texture mapping flags
DILIGENT_TYPED_ENUM(MAP_FLAGS, Uint8)
{
    MAP_FLAG_NONE         = 0x000,

    /// Specifies that map operation should not wait until previous command that
    /// using the same resource completes. Map returns null pointer if the resource
    /// is still in use.\n
    /// D3D11 counterpart:  D3D11_MAP_FLAG_DO_NOT_WAIT
    /// \note: OpenGL does not have corresponding flag, so a buffer will always be mapped
    MAP_FLAG_DO_NOT_WAIT  = 0x001,

    /// Previous contents of the resource will be undefined. This flag is only compatible with MAP_WRITE\n
    /// D3D11 counterpart: D3D11_MAP_WRITE_DISCARD. OpenGL counterpart: GL_MAP_INVALIDATE_BUFFER_BIT
    /// \note OpenGL implementation may orphan a buffer instead 
    MAP_FLAG_DISCARD      = 0x002,

    /// The system will not synchronize pending operations before mapping the buffer. It is responsibility
    /// of the application to make sure that the buffer contents is not overwritten while it is in use by 
    /// the GPU.\n
    /// D3D11 counterpart:  D3D11_MAP_WRITE_NO_OVERWRITE. OpenGL counterpart: GL_MAP_UNSYNCHRONIZED_BIT
    MAP_FLAG_NO_OVERWRITE = 0x004
};
DEFINE_FLAG_ENUM_OPERATORS(MAP_FLAGS)

/// Describes resource dimension

/// This enumeration is used by
/// - TextureDesc to describe texture type
/// - TextureViewDesc to describe texture view type
DILIGENT_TYPED_ENUM(RESOURCE_DIMENSION, Uint8)
{
    RESOURCE_DIM_UNDEFINED = 0,  ///< Texture type undefined
    RESOURCE_DIM_BUFFER,         ///< Buffer
    RESOURCE_DIM_TEX_1D,         ///< One-dimensional texture
    RESOURCE_DIM_TEX_1D_ARRAY,   ///< One-dimensional texture array
    RESOURCE_DIM_TEX_2D,         ///< Two-dimensional texture
    RESOURCE_DIM_TEX_2D_ARRAY,   ///< Two-dimensional texture array
    RESOURCE_DIM_TEX_3D,         ///< Three-dimensional texture
    RESOURCE_DIM_TEX_CUBE,       ///< Cube-map texture
    RESOURCE_DIM_TEX_CUBE_ARRAY, ///< Cube-map array texture
    RESOURCE_DIM_NUM_DIMENSIONS  ///< Helper value that stores the total number of texture types in the enumeration
};

/// Texture view type

/// This enumeration describes allowed view types for a texture view. It is used by TextureViewDesc
/// structure.
DILIGENT_TYPED_ENUM(TEXTURE_VIEW_TYPE, Uint8)
{
    /// Undefined view type
    TEXTURE_VIEW_UNDEFINED = 0,     

    /// A texture view will define a shader resource view that will be used 
    /// as the source for the shader read operations
    TEXTURE_VIEW_SHADER_RESOURCE,

    /// A texture view will define a render target view that will be used
    /// as the target for rendering operations
    TEXTURE_VIEW_RENDER_TARGET,     

    /// A texture view will define a depth stencil view that will be used
    /// as the target for rendering operations
    TEXTURE_VIEW_DEPTH_STENCIL,     

    /// A texture view will define an unordered access view that will be used
    /// for unordered read/write operations from the shaders
    TEXTURE_VIEW_UNORDERED_ACCESS,  

    /// Helper value that stores that total number of texture views
    TEXTURE_VIEW_NUM_VIEWS          
};

/// Buffer view type

/// This enumeration describes allowed view types for a buffer view. It is used by BufferViewDesc
/// structure.
DILIGENT_TYPED_ENUM(BUFFER_VIEW_TYPE, Uint8)
{
    /// Undefined view type
    BUFFER_VIEW_UNDEFINED = 0,

    /// A buffer view will define a shader resource view that will be used 
    /// as the source for the shader read operations
    BUFFER_VIEW_SHADER_RESOURCE,

    /// A buffer view will define an unordered access view that will be used
    /// for unordered read/write operations from the shaders
    BUFFER_VIEW_UNORDERED_ACCESS,

    /// Helper value that stores that total number of buffer views
    BUFFER_VIEW_NUM_VIEWS
};

/// Texture formats

/// This enumeration describes available texture formats and generally mirrors DXGI_FORMAT enumeration.
/// The table below provides detailed information on each format. Most of the formats are widely supported 
/// by all modern APIs (DX10+, OpenGL3.3+ and OpenGLES3.0+). Specific requirements are additionally indicated.
/// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format">DXGI_FORMAT enumeration on MSDN</a>, 
///     <a href = "https://www.opengl.org/wiki/Image_Format">OpenGL Texture Formats</a>
///
DILIGENT_TYPED_ENUM(TEXTURE_FORMAT, Uint16)
{
    /// Unknown format
    TEX_FORMAT_UNKNOWN = 0,  
        
    /// Four-component 128-bit typeless format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32A32_TYPELESS. OpenGL does not have direct counterpart, GL_RGBA32F is used.
    TEX_FORMAT_RGBA32_TYPELESS,

    /// Four-component 128-bit floating-point format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32A32_FLOAT. OpenGL counterpart: GL_RGBA32F.
    TEX_FORMAT_RGBA32_FLOAT,  

    /// Four-component 128-bit unsigned-integer format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32A32_UINT. OpenGL counterpart: GL_RGBA32UI.
    TEX_FORMAT_RGBA32_UINT,

    /// Four-component 128-bit signed-integer format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32A32_SINT. OpenGL counterpart: GL_RGBA32I.
    TEX_FORMAT_RGBA32_SINT,  

    /// Three-component 96-bit typeless format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32_TYPELESS. OpenGL does not have direct counterpart, GL_RGB32F is used.
    /// \warning This format has weak hardware support and is not recommended 
    TEX_FORMAT_RGB32_TYPELESS,  
                                                  
    /// Three-component 96-bit floating-point format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32_FLOAT. OpenGL counterpart: GL_RGB32F.
    /// \warning This format has weak hardware support and is not recommended 
    TEX_FORMAT_RGB32_FLOAT,  

    /// Three-component 96-bit unsigned-integer format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32_UINT. OpenGL counterpart: GL_RGB32UI.
    /// \warning This format has weak hardware support and is not recommended 
    TEX_FORMAT_RGB32_UINT,  

    /// Three-component 96-bit signed-integer format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32B32_SINT. OpenGL counterpart: GL_RGB32I.
    /// \warning This format has weak hardware support and is not recommended 
    TEX_FORMAT_RGB32_SINT,  

    /// Four-component 64-bit typeless format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16B16A16_TYPELESS. OpenGL does not have direct counterpart, GL_RGBA16F is used.
    TEX_FORMAT_RGBA16_TYPELESS,  

    /// Four-component 64-bit half-precision floating-point format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16B16A16_FLOAT. OpenGL counterpart: GL_RGBA16F.
    TEX_FORMAT_RGBA16_FLOAT, 

    /// Four-component 64-bit unsigned-normalized-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16B16A16_UNORM. OpenGL counterpart: GL_RGBA16. \n
    /// [GL_EXT_texture_norm16]: https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_norm16.txt
    /// OpenGLES: [GL_EXT_texture_norm16][] extension is required
    TEX_FORMAT_RGBA16_UNORM, 

    /// Four-component 64-bit unsigned-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16B16A16_UINT. OpenGL counterpart: GL_RGBA16UI.
    TEX_FORMAT_RGBA16_UINT, 

    /// [GL_EXT_texture_norm16]: https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_norm16.txt
    /// Four-component 64-bit signed-normalized-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16B16A16_SNORM. OpenGL counterpart: GL_RGBA16_SNORM. \n
    /// [GL_EXT_texture_norm16]: https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_norm16.txt
    /// OpenGLES: [GL_EXT_texture_norm16][] extension is required
    TEX_FORMAT_RGBA16_SNORM, 

    /// Four-component 64-bit signed-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16B16A16_SINT. OpenGL counterpart: GL_RGBA16I.
    TEX_FORMAT_RGBA16_SINT, 

    /// Two-component 64-bit typeless format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32_TYPELESS. OpenGL does not have direct counterpart, GL_RG32F is used.
    TEX_FORMAT_RG32_TYPELESS, 

    /// Two-component 64-bit floating-point format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32_FLOAT. OpenGL counterpart: GL_RG32F.
    TEX_FORMAT_RG32_FLOAT, 

    /// Two-component 64-bit unsigned-integer format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32_UINT. OpenGL counterpart: GL_RG32UI.
    TEX_FORMAT_RG32_UINT, 

    /// Two-component 64-bit signed-integer format with 32-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R32G32_SINT. OpenGL counterpart: GL_RG32I.
    TEX_FORMAT_RG32_SINT, 

    /// Two-component 64-bit typeless format with 32-bits for R channel and 8 bits for G channel. \n
    /// D3D counterpart: DXGI_FORMAT_R32G8X24_TYPELESS. OpenGL does not have direct counterpart, GL_DEPTH32F_STENCIL8 is used.
    TEX_FORMAT_R32G8X24_TYPELESS, 

    /// Two-component 64-bit format with 32-bit floating-point depth channel and 8-bit stencil channel. \n
    /// D3D counterpart: DXGI_FORMAT_D32_FLOAT_S8X24_UINT. OpenGL counterpart: GL_DEPTH32F_STENCIL8.
    TEX_FORMAT_D32_FLOAT_S8X24_UINT, 

    /// Two-component 64-bit format with 32-bit floating-point R channel and 8+24-bits of typeless data. \n
    /// D3D counterpart: DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS. OpenGL does not have direct counterpart, GL_DEPTH32F_STENCIL8 is used.
    TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, 

    /// Two-component 64-bit format with 32-bit typeless data and 8-bit G channel. \n
    /// D3D counterpart: DXGI_FORMAT_X32_TYPELESS_G8X24_UINT
    /// \warning This format is currently not implemented in OpenGL version
    TEX_FORMAT_X32_TYPELESS_G8X24_UINT, 

    /// Four-component 32-bit typeless format with 10 bits for RGB and 2 bits for alpha channel. \n
    /// D3D counterpart: DXGI_FORMAT_R10G10B10A2_TYPELESS. OpenGL does not have direct counterpart, GL_RGB10_A2 is used.
    TEX_FORMAT_RGB10A2_TYPELESS, 

    /// Four-component 32-bit unsigned-normalized-integer format with 10 bits for each color and 2 bits for alpha channel. \n
    /// D3D counterpart: DXGI_FORMAT_R10G10B10A2_UNORM. OpenGL counterpart: GL_RGB10_A2.
    TEX_FORMAT_RGB10A2_UNORM, 

    /// Four-component 32-bit unsigned-integer format with 10 bits for each color and 2 bits for alpha channel. \n
    /// D3D counterpart: DXGI_FORMAT_R10G10B10A2_UINT. OpenGL counterpart: GL_RGB10_A2UI.
    TEX_FORMAT_RGB10A2_UINT, 

    /// Three-component 32-bit format encoding three partial precision channels using 11 bits for red and green and 10 bits for blue channel. \n
    /// D3D counterpart: DXGI_FORMAT_R11G11B10_FLOAT. OpenGL counterpart: GL_R11F_G11F_B10F.
    TEX_FORMAT_R11G11B10_FLOAT, 

    /// Four-component 32-bit typeless format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8B8A8_TYPELESS. OpenGL does not have direct counterpart, GL_RGBA8 is used.
    TEX_FORMAT_RGBA8_TYPELESS, 

    /// Four-component 32-bit unsigned-normalized-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8B8A8_UNORM. OpenGL counterpart: GL_RGBA8.
    TEX_FORMAT_RGBA8_UNORM,

    /// Four-component 32-bit unsigned-normalized-integer sRGB format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8B8A8_UNORM_SRGB. OpenGL counterpart: GL_SRGB8_ALPHA8.
    TEX_FORMAT_RGBA8_UNORM_SRGB, 

    /// Four-component 32-bit unsigned-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8B8A8_UINT. OpenGL counterpart: GL_RGBA8UI.
    TEX_FORMAT_RGBA8_UINT, 

    /// Four-component 32-bit signed-normalized-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8B8A8_SNORM. OpenGL counterpart: GL_RGBA8_SNORM.
    TEX_FORMAT_RGBA8_SNORM, 

    /// Four-component 32-bit signed-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8B8A8_SINT. OpenGL counterpart: GL_RGBA8I.
    TEX_FORMAT_RGBA8_SINT, 
        
    /// Two-component 32-bit typeless format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16_TYPELESS. OpenGL does not have direct counterpart, GL_RG16F is used.
    TEX_FORMAT_RG16_TYPELESS, 
        
    /// Two-component 32-bit half-precision floating-point format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16_FLOAT. OpenGL counterpart: GL_RG16F.
    TEX_FORMAT_RG16_FLOAT, 
        
    /// Two-component 32-bit unsigned-normalized-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16_UNORM. OpenGL counterpart: GL_RG16. \n
    /// [GL_EXT_texture_norm16]: https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_norm16.txt
    /// OpenGLES: [GL_EXT_texture_norm16][] extension is required
    TEX_FORMAT_RG16_UNORM, 
        
    /// Two-component 32-bit unsigned-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16_UINT. OpenGL counterpart: GL_RG16UI.
    TEX_FORMAT_RG16_UINT, 
        
    /// Two-component 32-bit signed-normalized-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16_SNORM. OpenGL counterpart: GL_RG16_SNORM. \n
    /// [GL_EXT_texture_norm16]: https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_norm16.txt
    /// OpenGLES: [GL_EXT_texture_norm16][] extension is required
    TEX_FORMAT_RG16_SNORM, 
        
    /// Two-component 32-bit signed-integer format with 16-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R16G16_SINT. OpenGL counterpart: GL_RG16I.
    TEX_FORMAT_RG16_SINT, 
        
    /// Single-component 32-bit typeless format. \n
    /// D3D counterpart: DXGI_FORMAT_R32_TYPELESS. OpenGL does not have direct counterpart, GL_R32F is used.
    TEX_FORMAT_R32_TYPELESS, 
        
    /// Single-component 32-bit floating-point depth format. \n
    /// D3D counterpart: DXGI_FORMAT_D32_FLOAT. OpenGL counterpart: GL_DEPTH_COMPONENT32F.
    TEX_FORMAT_D32_FLOAT, 
        
    /// Single-component 32-bit floating-point format. \n
    /// D3D counterpart: DXGI_FORMAT_R32_FLOAT. OpenGL counterpart: GL_R32F.
    TEX_FORMAT_R32_FLOAT, 
        
    /// Single-component 32-bit unsigned-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R32_UINT. OpenGL counterpart: GL_R32UI.
    TEX_FORMAT_R32_UINT, 
        
    /// Single-component 32-bit signed-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R32_SINT. OpenGL counterpart: GL_R32I.
    TEX_FORMAT_R32_SINT, 
        
    /// Two-component 32-bit typeless format with 24 bits for R and 8 bits for G channel. \n
    /// D3D counterpart: DXGI_FORMAT_R24G8_TYPELESS. OpenGL does not have direct counterpart, GL_DEPTH24_STENCIL8 is used.
    TEX_FORMAT_R24G8_TYPELESS, 
        
    /// Two-component 32-bit format with 24 bits for unsigned-normalized-integer depth and 8 bits for stencil. \n
    /// D3D counterpart: DXGI_FORMAT_D24_UNORM_S8_UINT. OpenGL counterpart: GL_DEPTH24_STENCIL8.
    TEX_FORMAT_D24_UNORM_S8_UINT, 
        
    /// Two-component 32-bit format with 24 bits for unsigned-normalized-integer data and 8 bits of unreferenced data. \n
    /// D3D counterpart: DXGI_FORMAT_R24_UNORM_X8_TYPELESS. OpenGL does not have direct counterpart, GL_DEPTH24_STENCIL8 is used.
    TEX_FORMAT_R24_UNORM_X8_TYPELESS, 
        
    /// Two-component 32-bit format with 24 bits of unreferenced data and 8 bits of unsigned-integer data. \n
    /// D3D counterpart: DXGI_FORMAT_X24_TYPELESS_G8_UINT
    /// \warning This format is currently not implemented in OpenGL version
    TEX_FORMAT_X24_TYPELESS_G8_UINT, 
        
    /// Two-component 16-bit typeless format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8_TYPELESS. OpenGL does not have direct counterpart, GL_RG8 is used.
    TEX_FORMAT_RG8_TYPELESS, 
        
    /// Two-component 16-bit unsigned-normalized-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8_UNORM. OpenGL counterpart: GL_RG8.
    TEX_FORMAT_RG8_UNORM, 
        
    /// Two-component 16-bit unsigned-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8_UINT. OpenGL counterpart: GL_RG8UI.
    TEX_FORMAT_RG8_UINT, 
        
    /// Two-component 16-bit signed-normalized-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8_SNORM. OpenGL counterpart: GL_RG8_SNORM.
    TEX_FORMAT_RG8_SNORM, 
        
    /// Two-component 16-bit signed-integer format with 8-bit channels. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8_SINT. OpenGL counterpart: GL_RG8I.
    TEX_FORMAT_RG8_SINT, 
        
    /// Single-component 16-bit typeless format. \n
    /// D3D counterpart: DXGI_FORMAT_R16_TYPELESS. OpenGL does not have direct counterpart, GL_R16F is used.
    TEX_FORMAT_R16_TYPELESS, 
        
    /// Single-component 16-bit half-precisoin floating-point format. \n
    /// D3D counterpart: DXGI_FORMAT_R16_FLOAT. OpenGL counterpart: GL_R16F.
    TEX_FORMAT_R16_FLOAT, 
        
    /// Single-component 16-bit unsigned-normalized-integer depth format. \n
    /// D3D counterpart: DXGI_FORMAT_D16_UNORM. OpenGL counterpart: GL_DEPTH_COMPONENT16.
    TEX_FORMAT_D16_UNORM, 
        
    /// Single-component 16-bit unsigned-normalized-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R16_UNORM. OpenGL counterpart: GL_R16.
    /// [GL_EXT_texture_norm16]: https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_norm16.txt
    /// OpenGLES: [GL_EXT_texture_norm16][] extension is required
    TEX_FORMAT_R16_UNORM, 
        
    /// Single-component 16-bit unsigned-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R16_UINT. OpenGL counterpart: GL_R16UI.
    TEX_FORMAT_R16_UINT, 
        
    /// Single-component 16-bit signed-normalized-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R16_SNORM. OpenGL counterpart: GL_R16_SNORM. \n
    /// [GL_EXT_texture_norm16]: https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_norm16.txt
    /// OpenGLES: [GL_EXT_texture_norm16][] extension is required
    TEX_FORMAT_R16_SNORM, 
        
    /// Single-component 16-bit signed-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R16_SINT. OpenGL counterpart: GL_R16I.
    TEX_FORMAT_R16_SINT, 
        
    /// Single-component 8-bit typeless format. \n
    /// D3D counterpart: DXGI_FORMAT_R8_TYPELESS. OpenGL does not have direct counterpart, GL_R8 is used.
    TEX_FORMAT_R8_TYPELESS, 
        
    /// Single-component 8-bit unsigned-normalized-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R8_UNORM. OpenGL counterpart: GL_R8.
    TEX_FORMAT_R8_UNORM, 
        
    /// Single-component 8-bit unsigned-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R8_UINT. OpenGL counterpart: GL_R8UI.
    TEX_FORMAT_R8_UINT, 
        
    /// Single-component 8-bit signed-normalized-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R8_SNORM. OpenGL counterpart: GL_R8_SNORM.
    TEX_FORMAT_R8_SNORM, 
        
    /// Single-component 8-bit signed-integer format. \n
    /// D3D counterpart: DXGI_FORMAT_R8_SINT. OpenGL counterpart: GL_R8I.
    TEX_FORMAT_R8_SINT, 
        
    /// Single-component 8-bit unsigned-normalized-integer format for alpha only. \n
    /// D3D counterpart: DXGI_FORMAT_A8_UNORM
    /// \warning This format is not availanle in OpenGL
    TEX_FORMAT_A8_UNORM, 
        
    /// Single-component 1-bit format. \n
    /// D3D counterpart: DXGI_FORMAT_R1_UNORM
    /// \warning This format is not availanle in OpenGL
    TEX_FORMAT_R1_UNORM, 
        
    /// Three partial-precision floating pointer numbers sharing single exponent encoded into a 32-bit value. \n
    /// D3D counterpart: DXGI_FORMAT_R9G9B9E5_SHAREDEXP. OpenGL counterpart: GL_RGB9_E5.
    TEX_FORMAT_RGB9E5_SHAREDEXP, 
        
    /// Four-component unsigned-normalized integer format analogous to UYVY encoding. \n
    /// D3D counterpart: DXGI_FORMAT_R8G8_B8G8_UNORM
    /// \warning This format is not availanle in OpenGL
    TEX_FORMAT_RG8_B8G8_UNORM, 
        
    /// Four-component unsigned-normalized integer format analogous to YUY2 encoding. \n
    /// D3D counterpart: DXGI_FORMAT_G8R8_G8B8_UNORM
    /// \warning This format is not availanle in OpenGL
    TEX_FORMAT_G8R8_G8B8_UNORM, 
        
    /// Four-component typeless block-compression format with 1:8 compression ratio.\n
    /// D3D counterpart: DXGI_FORMAT_BC1_TYPELESS. OpenGL does not have direct counterpart, GL_COMPRESSED_RGB_S3TC_DXT1_EXT is used. \n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc1">BC1 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT1_Format">DXT1 on OpenGL.org </a>
    TEX_FORMAT_BC1_TYPELESS, 
        
    /// Four-component unsigned-normalized-integer block-compression format with 5 bits for R, 6 bits for G, 5 bits for B, and 0 or 1 bit for A channel. 
    /// The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:8 compression ratio against RGBA8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC1_UNORM. OpenGL counterpart: GL_COMPRESSED_RGB_S3TC_DXT1_EXT.\n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc1">BC1 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT1_Format">DXT1 on OpenGL.org </a>
    TEX_FORMAT_BC1_UNORM,
        
    /// Four-component unsigned-normalized-integer block-compression sRGB format with 5 bits for R, 6 bits for G, 5 bits for B, and 0 or 1 bit for A channel. \n
    /// The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:8 compression ratio against RGBA8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC1_UNORM_SRGB. OpenGL counterpart: GL_COMPRESSED_SRGB_S3TC_DXT1_EXT.\n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc1">BC1 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT1_Format">DXT1 on OpenGL.org </a>
    TEX_FORMAT_BC1_UNORM_SRGB,
        
    /// Four component typeless block-compression format with 1:4 compression ratio.\n
    /// D3D counterpart: DXGI_FORMAT_BC2_TYPELESS. OpenGL does not have direct counterpart, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT is used. \n 
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc2">BC2 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT3_Format">DXT3 on OpenGL.org </a>
    TEX_FORMAT_BC2_TYPELESS, 
        
    /// Four-component unsigned-normalized-integer block-compression format with 5 bits for R, 6 bits for G, 5 bits for B, and 4 bits for low-coherent separate A channel.
    /// The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC2_UNORM. OpenGL counterpart: GL_COMPRESSED_RGBA_S3TC_DXT3_EXT. \n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc2">BC2 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT3_Format">DXT3 on OpenGL.org </a>
    TEX_FORMAT_BC2_UNORM,
        
    /// Four-component signed-normalized-integer block-compression sRGB format with 5 bits for R, 6 bits for G, 5 bits for B, and 4 bits for low-coherent separate A channel.
    /// The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC2_UNORM_SRGB. OpenGL counterpart: GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT. \n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc2">BC2 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT3_Format">DXT3 on OpenGL.org </a>
    TEX_FORMAT_BC2_UNORM_SRGB,
        
    /// Four-component typeless block-compression format with 1:4 compression ratio.\n
    /// D3D counterpart: DXGI_FORMAT_BC3_TYPELESS. OpenGL does not have direct counterpart, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT is used. \n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc3">BC3 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT5_Format">DXT5 on OpenGL.org </a>
    TEX_FORMAT_BC3_TYPELESS, 
        
    /// Four-component unsigned-normalized-integer block-compression format with 5 bits for R, 6 bits for G, 5 bits for B, and 8 bits for highly-coherent A channel.
    /// The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC3_UNORM. OpenGL counterpart: GL_COMPRESSED_RGBA_S3TC_DXT5_EXT. \n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc3">BC3 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT5_Format">DXT5 on OpenGL.org </a>
    TEX_FORMAT_BC3_UNORM,
        
    /// Four-component unsigned-normalized-integer block-compression sRGB format with 5 bits for R, 6 bits for G, 5 bits for B, and 8 bits for highly-coherent A channel.
    /// The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:4 compression ratio against RGBA8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC3_UNORM_SRGB. OpenGL counterpart: GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT. \n
    /// [GL_EXT_texture_compression_s3tc]: https://www.khronos.org/registry/gles/extensions/EXT/texture_compression_s3tc.txt
    /// OpenGL & OpenGLES: [GL_EXT_texture_compression_s3tc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc3">BC3 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/S3_Texture_Compression#DXT5_Format">DXT5 on OpenGL.org </a>
    TEX_FORMAT_BC3_UNORM_SRGB,
        
    /// One-component typeless block-compression format with 1:2 compression ratio. \n
    /// D3D counterpart: DXGI_FORMAT_BC4_TYPELESS. OpenGL does not have direct counterpart, GL_COMPRESSED_RED_RGTC1 is used. \n
    /// [GL_ARB_texture_compression_rgtc]: https://www.opengl.org/registry/specs/ARB/texture_compression_rgtc.txt
    /// OpenGL & OpenGLES: [GL_ARB_texture_compression_rgtc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc4">BC4 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/Image_Format#Compressed_formats">Compressed formats on OpenGL.org </a>
    TEX_FORMAT_BC4_TYPELESS, 
        
    /// One-component unsigned-normalized-integer block-compression format with 8 bits for R channel.
    /// The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:2 compression ratio against R8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC4_UNORM. OpenGL counterpart: GL_COMPRESSED_RED_RGTC1. \n
    /// [GL_ARB_texture_compression_rgtc]: https://www.opengl.org/registry/specs/ARB/texture_compression_rgtc.txt
    /// OpenGL & OpenGLES: [GL_ARB_texture_compression_rgtc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc4">BC4 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/Image_Format#Compressed_formats">Compressed formats on OpenGL.org </a>
    TEX_FORMAT_BC4_UNORM,
        
    /// One-component signed-normalized-integer block-compression format with 8 bits for R channel.
    /// The pixel data is encoded using 8 bytes per 4x4 block (4 bits per pixel) providing 1:2 compression ratio against R8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC4_SNORM. OpenGL counterpart: GL_COMPRESSED_SIGNED_RED_RGTC1. \n
    /// [GL_ARB_texture_compression_rgtc]: https://www.opengl.org/registry/specs/ARB/texture_compression_rgtc.txt
    /// OpenGL & OpenGLES: [GL_ARB_texture_compression_rgtc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc4">BC4 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/Image_Format#Compressed_formats">Compressed formats on OpenGL.org </a>
    TEX_FORMAT_BC4_SNORM,
        
    /// Two-component typeless block-compression format with 1:2 compression ratio. \n
    /// D3D counterpart: DXGI_FORMAT_BC5_TYPELESS. OpenGL does not have direct counterpart, GL_COMPRESSED_RG_RGTC2 is used. \n
    /// [GL_ARB_texture_compression_rgtc]: https://www.opengl.org/registry/specs/ARB/texture_compression_rgtc.txt
    /// OpenGL & OpenGLES: [GL_ARB_texture_compression_rgtc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc5">BC5 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/Image_Format#Compressed_formats">Compressed formats on OpenGL.org </a>
    TEX_FORMAT_BC5_TYPELESS,
        
    /// Two-component unsigned-normalized-integer block-compression format with 8 bits for R and 8 bits for G channel.
    /// The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:2 compression ratio against RG8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC5_UNORM. OpenGL counterpart: GL_COMPRESSED_RG_RGTC2. \n
    /// [GL_ARB_texture_compression_rgtc]: https://www.opengl.org/registry/specs/ARB/texture_compression_rgtc.txt
    /// OpenGL & OpenGLES: [GL_ARB_texture_compression_rgtc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc5">BC5 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/Image_Format#Compressed_formats">Compressed formats on OpenGL.org </a>
    TEX_FORMAT_BC5_UNORM,
        
    /// Two-component signed-normalized-integer block-compression format with 8 bits for R and 8 bits for G channel.
    /// The pixel data is encoded using 16 bytes per 4x4 block (8 bits per pixel) providing 1:2 compression ratio against RG8 format. \n
    /// D3D counterpart: DXGI_FORMAT_BC5_SNORM. OpenGL counterpart: GL_COMPRESSED_SIGNED_RG_RGTC2. \n
    /// [GL_ARB_texture_compression_rgtc]: https://www.opengl.org/registry/specs/ARB/texture_compression_rgtc.txt
    /// OpenGL & OpenGLES: [GL_ARB_texture_compression_rgtc][] extension is required
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc5">BC5 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/Image_Format#Compressed_formats">Compressed formats on OpenGL.org </a>
    TEX_FORMAT_BC5_SNORM,
        
    /// Three-component 16-bit unsigned-normalized-integer format with 5 bits for blue, 6 bits for green, and 5 bits for red channel. \n
    /// D3D counterpart: DXGI_FORMAT_B5G6R5_UNORM
    /// \warning This format is not available until D3D11.1 and Windows 8. It is also not available in OpenGL
    TEX_FORMAT_B5G6R5_UNORM,
        
    /// Four-component 16-bit unsigned-normalized-integer format with 5 bits for each color channel and 1-bit alpha. \n
    /// D3D counterpart: DXGI_FORMAT_B5G5R5A1_UNORM
    /// \warning This format is not available until D3D11.1 and Windows 8. It is also not available in OpenGL
    TEX_FORMAT_B5G5R5A1_UNORM, 
        
    /// Four-component 32-bit unsigned-normalized-integer format with 8 bits for each channel. \n
    /// D3D counterpart: DXGI_FORMAT_B8G8R8A8_UNORM.
    /// \warning This format is not available in OpenGL
    TEX_FORMAT_BGRA8_UNORM,
        
    /// Four-component 32-bit unsigned-normalized-integer format with 8 bits for each color channel and 8 bits unused. \n
    /// D3D counterpart: DXGI_FORMAT_B8G8R8X8_UNORM.
    /// \warning This format is not available in OpenGL
    TEX_FORMAT_BGRX8_UNORM,
        
    /// Four-component 32-bit 2.8-biased fixed-point format with 10 bits for each color channel and 2-bit alpha. \n
    /// D3D counterpart: DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM.
    /// \warning This format is not available in OpenGL
    TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
        
    /// Four-component 32-bit typeless format with 8 bits for each channel. \n
    /// D3D counterpart: DXGI_FORMAT_B8G8R8A8_TYPELESS.
    /// \warning This format is not available in OpenGL
    TEX_FORMAT_BGRA8_TYPELESS,
        
    /// Four-component 32-bit unsigned-normalized sRGB format with 8 bits for each channel. \n
    /// D3D counterpart: DXGI_FORMAT_B8G8R8A8_UNORM_SRGB.
    /// \warning This format is not available in OpenGL.
    TEX_FORMAT_BGRA8_UNORM_SRGB,
        
    /// Four-component 32-bit typeless format that with 8 bits for each color channel, and 8 bits are unused. \n
    /// D3D counterpart: DXGI_FORMAT_B8G8R8X8_TYPELESS.
    /// \warning This format is not available in OpenGL.
    TEX_FORMAT_BGRX8_TYPELESS,
        
    /// Four-component 32-bit unsigned-normalized sRGB format with 8 bits for each color channel, and 8 bits are unused. \n
    /// D3D counterpart: DXGI_FORMAT_B8G8R8X8_UNORM_SRGB.
    /// \warning This format is not available in OpenGL.
    TEX_FORMAT_BGRX8_UNORM_SRGB,
        
    /// Three-component typeless block-compression format. \n
    /// D3D counterpart: DXGI_FORMAT_BC6H_TYPELESS. OpenGL does not have direct counterpart, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT is used. \n
    /// [GL_ARB_texture_compression_bptc]: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
    /// OpenGL: [GL_ARB_texture_compression_bptc][] extension is required. Not supported in at least OpenGLES3.1
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format">BC6H on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/BPTC_Texture_Compression">BPTC Texture Compression on OpenGL.org </a>
    TEX_FORMAT_BC6H_TYPELESS,
        
    /// Three-component unsigned half-precision floating-point format with 16 bits for each channel. \n
    /// D3D counterpart: DXGI_FORMAT_BC6H_UF16. OpenGL counterpart: GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT. \n
    /// [GL_ARB_texture_compression_bptc]: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
    /// OpenGL: [GL_ARB_texture_compression_bptc][] extension is required. Not supported in at least OpenGLES3.1
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format">BC6H on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/BPTC_Texture_Compression">BPTC Texture Compression on OpenGL.org </a>
    TEX_FORMAT_BC6H_UF16,
        
    /// Three-channel signed half-precision floating-point format with 16 bits per each channel. \n
    /// D3D counterpart: DXGI_FORMAT_BC6H_SF16. OpenGL counterpart: GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT. \n
    /// [GL_ARB_texture_compression_bptc]: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
    /// OpenGL: [GL_ARB_texture_compression_bptc][] extension is required. Not supported in at least OpenGLES3.1
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format">BC6H on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/BPTC_Texture_Compression">BPTC Texture Compression on OpenGL.org </a>
    TEX_FORMAT_BC6H_SF16,
        
    /// Three-component typeless block-compression format. \n
    /// D3D counterpart: DXGI_FORMAT_BC7_TYPELESS. OpenGL does not have direct counterpart, GL_COMPRESSED_RGBA_BPTC_UNORM is used. \n
    /// [GL_ARB_texture_compression_bptc]: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
    /// OpenGL: [GL_ARB_texture_compression_bptc][] extension is required. Not supported in at least OpenGLES3.1
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format">BC7 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/BPTC_Texture_Compression">BPTC Texture Compression on OpenGL.org </a>
    TEX_FORMAT_BC7_TYPELESS,
        
    /// Three-component block-compression unsigned-normalized-integer format with 4 to 7 bits per color channel and 0 to 8 bits of alpha. \n
    /// D3D counterpart: DXGI_FORMAT_BC7_UNORM. OpenGL counterpart: GL_COMPRESSED_RGBA_BPTC_UNORM. \n
    /// [GL_ARB_texture_compression_bptc]: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
    /// OpenGL: [GL_ARB_texture_compression_bptc][] extension is required. Not supported in at least OpenGLES3.1
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format">BC7 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/BPTC_Texture_Compression">BPTC Texture Compression on OpenGL.org </a>
    TEX_FORMAT_BC7_UNORM,
        
    /// Three-component block-compression unsigned-normalized-integer sRGB format with 4 to 7 bits per color channel and 0 to 8 bits of alpha. \n
    /// D3D counterpart: DXGI_FORMAT_BC7_UNORM_SRGB. OpenGL counterpart: GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM. \n
    /// [GL_ARB_texture_compression_bptc]: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
    /// OpenGL: [GL_ARB_texture_compression_bptc][] extension is required. Not supported in at least OpenGLES3.1
    /// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format">BC7 on MSDN </a>, 
    ///     <a href = "https://www.opengl.org/wiki/BPTC_Texture_Compression">BPTC Texture Compression on OpenGL.org </a>
    TEX_FORMAT_BC7_UNORM_SRGB,
        
    /// Helper member containing the total number of texture formats in the enumeration
    TEX_FORMAT_NUM_FORMATS
};

/// Filter type

/// This enumeration defines filter type. It is used by SamplerDesc structure to define min, mag and mip filters.
/// \note On D3D11, comparison filters only work with textures that have the following formats: 
/// R32_FLOAT_X8X24_TYPELESS, R32_FLOAT, R24_UNORM_X8_TYPELESS, R16_UNORM.
DILIGENT_TYPED_ENUM(FILTER_TYPE, Uint8)
{
    FILTER_TYPE_UNKNOWN  = 0,           ///< Unknown filter type
    FILTER_TYPE_POINT,                  ///< Point filtering
    FILTER_TYPE_LINEAR,                 ///< Linear filtering
    FILTER_TYPE_ANISOTROPIC,            ///< Anisotropic filtering
    FILTER_TYPE_COMPARISON_POINT,       ///< Comparison-point filtering
    FILTER_TYPE_COMPARISON_LINEAR,      ///< Comparison-linear filtering
    FILTER_TYPE_COMPARISON_ANISOTROPIC, ///< Comparison-anisotropic filtering
    FILTER_TYPE_MINIMUM_POINT,          ///< Minimum-point filtering (DX12 only)
    FILTER_TYPE_MINIMUM_LINEAR,         ///< Minimum-linear filtering (DX12 only)
    FILTER_TYPE_MINIMUM_ANISOTROPIC,    ///< Minimum-anisotropic filtering (DX12 only)
    FILTER_TYPE_MAXIMUM_POINT,          ///< Maximum-point filtering (DX12 only)
    FILTER_TYPE_MAXIMUM_LINEAR,         ///< Maximum-linear filtering (DX12 only)
    FILTER_TYPE_MAXIMUM_ANISOTROPIC,    ///< Maximum-anisotropic filtering (DX12 only)
    FILTER_TYPE_NUM_FILTERS             ///< Helper value that stores the total number of filter types in the enumeration
};

/// Texture address mode

/// [D3D11_TEXTURE_ADDRESS_MODE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_texture_address_mode
/// [D3D12_TEXTURE_ADDRESS_MODE]: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_texture_address_mode
/// Defines a technique for resolving texture coordinates that are outside of 
/// the boundaries of a texture. The enumeration generally mirrors [D3D11_TEXTURE_ADDRESS_MODE][]/[D3D12_TEXTURE_ADDRESS_MODE][] enumeration. 
/// It is used by SamplerDesc structure to define the address mode for U,V and W texture coordinates.
DILIGENT_TYPED_ENUM(TEXTURE_ADDRESS_MODE, Uint8)
{
    /// Unknown mode
    TEXTURE_ADDRESS_UNKNOWN = 0,

    /// Tile the texture at every integer junction. \n
    /// Direct3D Counterpart: D3D11_TEXTURE_ADDRESS_WRAP/D3D12_TEXTURE_ADDRESS_MODE_WRAP. OpenGL counterpart: GL_REPEAT
    TEXTURE_ADDRESS_WRAP	= 1,

    /// Flip the texture at every integer junction. \n
    /// Direct3D Counterpart: D3D11_TEXTURE_ADDRESS_MIRROR/D3D12_TEXTURE_ADDRESS_MODE_MIRROR. OpenGL counterpart: GL_MIRRORED_REPEAT
    TEXTURE_ADDRESS_MIRROR	= 2,

    /// Texture coordinates outside the range [0.0, 1.0] are set to the 
    /// texture color at 0.0 or 1.0, respectively. \n
    /// Direct3D Counterpart: D3D11_TEXTURE_ADDRESS_CLAMP/D3D12_TEXTURE_ADDRESS_MODE_CLAMP. OpenGL counterpart: GL_CLAMP_TO_EDGE
    TEXTURE_ADDRESS_CLAMP	= 3,

    /// Texture coordinates outside the range [0.0, 1.0] are set to the border color specified
    /// specified in SamplerDesc structure. \n
    /// Direct3D Counterpart: D3D11_TEXTURE_ADDRESS_BORDER/D3D12_TEXTURE_ADDRESS_MODE_BORDER. OpenGL counterpart: GL_CLAMP_TO_BORDER
    TEXTURE_ADDRESS_BORDER	= 4,

    /// Similar to TEXTURE_ADDRESS_MIRROR and TEXTURE_ADDRESS_CLAMP. Takes the absolute 
    /// value of the texture coordinate (thus, mirroring around 0), and then clamps to 
    /// the maximum value. \n
    /// Direct3D Counterpart: D3D11_TEXTURE_ADDRESS_MIRROR_ONCE/D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE. OpenGL counterpart: GL_MIRROR_CLAMP_TO_EDGE
    /// \note GL_MIRROR_CLAMP_TO_EDGE is only available in OpenGL4.4+, and is not available until at least OpenGLES3.1
    TEXTURE_ADDRESS_MIRROR_ONCE	= 5,

    /// Helper value that stores the total number of texture address modes in the enumeration
    TEXTURE_ADDRESS_NUM_MODES
};

/// Comparison function

/// [D3D11_COMPARISON_FUNC]: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_comparison_func
/// [D3D12_COMPARISON_FUNC]: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_comparison_func
/// This enumeartion defines a comparison function. It generally mirrors [D3D11_COMPARISON_FUNC]/[D3D12_COMPARISON_FUNC] enum and is used by
/// - SamplerDesc to define a comparison function if one of the comparison mode filters is used
/// - StencilOpDesc to define a stencil function
/// - DepthStencilStateDesc to define a depth function
DILIGENT_TYPED_ENUM(COMPARISON_FUNCTION, Uint8)
{
    /// Unknown comparison function
    COMPARISON_FUNC_UNKNOWN = 0,

    /// Comparison never passes. \n
    /// Direct3D counterpart: D3D11_COMPARISON_NEVER/D3D12_COMPARISON_FUNC_NEVER. OpenGL counterpart: GL_NEVER.
    COMPARISON_FUNC_NEVER,

    /// Comparison passes if the source data is less than the destination data.\n
    /// Direct3D counterpart: D3D11_COMPARISON_LESS/D3D12_COMPARISON_FUNC_LESS. OpenGL counterpart: GL_LESS.
    COMPARISON_FUNC_LESS,

    /// Comparison passes if the source data is equal to the destination data.\n
    /// Direct3D counterpart: D3D11_COMPARISON_EQUAL/D3D12_COMPARISON_FUNC_EQUAL. OpenGL counterpart: GL_EQUAL.
    COMPARISON_FUNC_EQUAL,

    /// Comparison passes if the source data is less than or equal to the destination data.\n
    /// Direct3D counterpart: D3D11_COMPARISON_LESS_EQUAL/D3D12_COMPARISON_FUNC_LESS_EQUAL. OpenGL counterpart: GL_LEQUAL.
    COMPARISON_FUNC_LESS_EQUAL,

    /// Comparison passes if the source data is greater than the destination data.\n
    /// Direct3D counterpart: 3D11_COMPARISON_GREATER/D3D12_COMPARISON_FUNC_GREATER. OpenGL counterpart: GL_GREATER.
    COMPARISON_FUNC_GREATER,

    /// Comparison passes if the source data is not equal to the destination data.\n
    /// Direct3D counterpart: D3D11_COMPARISON_NOT_EQUAL/D3D12_COMPARISON_FUNC_NOT_EQUAL. OpenGL counterpart: GL_NOTEQUAL.
    COMPARISON_FUNC_NOT_EQUAL,
        
    /// Comparison passes if the source data is greater than or equal to the destination data.\n
    /// Direct3D counterpart: D3D11_COMPARISON_GREATER_EQUAL/D3D12_COMPARISON_FUNC_GREATER_EQUAL. OpenGL counterpart: GL_GEQUAL.
    COMPARISON_FUNC_GREATER_EQUAL,
        
    /// Comparison always passes. \n
    /// Direct3D counterpart: D3D11_COMPARISON_ALWAYS/D3D12_COMPARISON_FUNC_ALWAYS. OpenGL counterpart: GL_ALWAYS.
    COMPARISON_FUNC_ALWAYS,

    /// Helper value that stores the total number of comparison functions in the enumeration
    COMPARISON_FUNC_NUM_FUNCTIONS
};

/// Miscellaneous texture flags

/// The enumeration is used by TextureDesc to describe misc texture flags
DILIGENT_TYPED_ENUM(MISC_TEXTURE_FLAGS, Uint8)
{
    MISC_TEXTURE_FLAG_NONE          = 0x00,

    /// Allow automatic mipmap generation with ITextureView::GenerateMips()

    /// \note A texture must be created with BIND_RENDER_TARGET bind flag
    MISC_TEXTURE_FLAG_GENERATE_MIPS = 0x01
};
DEFINE_FLAG_ENUM_OPERATORS(MISC_TEXTURE_FLAGS)

/// Input primitive topology.

/// This enumeration is used by GraphicsPipelineDesc structure to define input primitive topology.
DILIGENT_TYPED_ENUM(PRIMITIVE_TOPOLOGY, Uint8)
{
    /// Undefined topology
    PRIMITIVE_TOPOLOGY_UNDEFINED = 0,

    /// Interpret the vertex data as a list of triangles.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST. OpenGL counterpart: GL_TRIANGLES.
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,

    /// Interpret the vertex data as a triangle strip.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP. OpenGL counterpart: GL_TRIANGLE_STRIP.
    PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,

    /// Interpret the vertex data as a list of points.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_POINTLIST. OpenGL counterpart: GL_POINTS.
    PRIMITIVE_TOPOLOGY_POINT_LIST,

    /// Interpret the vertex data as a list of lines.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_LINELIST. OpenGL counterpart: GL_LINES.
    PRIMITIVE_TOPOLOGY_LINE_LIST,

    /// Interpret the vertex data as a line strip.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_LINESTRIP. OpenGL counterpart: GL_LINE_STRIP.
    PRIMITIVE_TOPOLOGY_LINE_STRIP,

    /// Interpret the vertex data as a list of one control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of two control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of three control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of four control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of five control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of six control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of seven control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of eight control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of nine control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of ten control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 11 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 12 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 13 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 14 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 15 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 16 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 17 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 18 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 19 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 20 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 21 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 22 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 23 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 24 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 25 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 26 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 27 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 28 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 29 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 30 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 31 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST,

    /// Interpret the vertex data as a list of 32 control point patches.\n
    /// D3D counterpart: D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST. OpenGL counterpart: GL_PATCHES.
    PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST,

    /// Helper value that stores the total number of topologies in the enumeration
    PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES
};


/// Defines optimized depth-stencil clear value.
struct DepthStencilClearValue
{
    /// Depth clear value
    Float32 Depth   DEFAULT_INITIALIZER(1.f);
    /// Stencil clear value
    Uint8 Stencil   DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    DepthStencilClearValue()noexcept{}

    DepthStencilClearValue(Float32 _Depth,
                           Uint8   _Stencil)noexcept : 
        Depth   {_Depth  },
        Stencil {_Stencil}
    {}
#endif
};
typedef struct DepthStencilClearValue DepthStencilClearValue;

/// Defines optimized clear value.
struct OptimizedClearValue
{
    /// Format
    TEXTURE_FORMAT Format       DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);

    /// Render target clear value
    Float32        Color[4]     DEFAULT_INITIALIZER({});

    /// Depth stencil clear value
    DepthStencilClearValue DepthStencil;

#if DILIGENT_CPP_INTERFACE
    bool operator == (const OptimizedClearValue& rhs)const
    {
        return Format == rhs.Format &&
               Color[0] == rhs.Color[0] &&
               Color[1] == rhs.Color[1] &&
               Color[2] == rhs.Color[2] &&
               Color[3] == rhs.Color[3] &&
               DepthStencil.Depth   == rhs.DepthStencil.Depth &&
               DepthStencil.Stencil == rhs.DepthStencil.Stencil;
    }
#endif
};
typedef struct OptimizedClearValue OptimizedClearValue;


/// Describes common device object attributes
struct DeviceObjectAttribs
{
    /// Object name
    const Char* Name DEFAULT_INITIALIZER(nullptr);

    // We have to explicitly define constructors because otherwise Apple's clang fails to compile the following legitimate code:
    //     DeviceObjectAttribs{"Name"}

#if DILIGENT_CPP_INTERFACE
    DeviceObjectAttribs()noexcept{}

    explicit DeviceObjectAttribs(const Char* _Name) : 
        Name{_Name}
    {}
#endif
};
typedef struct DeviceObjectAttribs DeviceObjectAttribs;
    
/// Hardware adapter type
DILIGENT_TYPED_ENUM(ADAPTER_TYPE, Uint8)
{
    /// Adapter type is unknown
    ADAPTER_TYPE_UNKNOWN = 0,

    /// Software adapter
    ADAPTER_TYPE_SOFTWARE,

    /// Hardware adapter
    ADAPTER_TYPE_HARDWARE
};


/// Flags indicating how an image is stretched to fit a given monitor's resolution.
/// \sa <a href = "https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb173066(v=vs.85)">DXGI_MODE_SCALING enumeration on MSDN</a>, 
enum SCALING_MODE
{
    /// Unspecified scaling.
    /// D3D Counterpart: DXGI_MODE_SCALING_UNSPECIFIED.
    SCALING_MODE_UNSPECIFIED = 0,

    /// Specifies no scaling. The image is centered on the display. 
    /// This flag is typically used for a fixed-dot-pitch display (such as an LED display).
    /// D3D Counterpart: DXGI_MODE_SCALING_CENTERED.
    SCALING_MODE_CENTERED = 1,

    /// Specifies stretched scaling.
    /// D3D Counterpart: DXGI_MODE_SCALING_STRETCHED.
    SCALING_MODE_STRETCHED = 2
};


/// Flags indicating the method the raster uses to create an image on a surface.
/// \sa <a href = "https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb173067(v=vs.85)">DXGI_MODE_SCANLINE_ORDER enumeration on MSDN</a>, 
enum SCANLINE_ORDER
{
    /// Scanline order is unspecified
    /// D3D Counterpart: DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED.
    SCANLINE_ORDER_UNSPECIFIED = 0,

    /// The image is created from the first scanline to the last without skipping any
    /// D3D Counterpart: DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE.
    SCANLINE_ORDER_PROGRESSIVE = 1,

    /// The image is created beginning with the upper field
    /// D3D Counterpart: DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST.
    SCANLINE_ORDER_UPPER_FIELD_FIRST = 2,

    /// The image is created beginning with the lower field
    /// D3D Counterpart: DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST.
    SCANLINE_ORDER_LOWER_FIELD_FIRST = 3
};

/// Display mode attributes
struct DisplayModeAttribs
{
    /// Display resolution width
    Uint32 Width                    DEFAULT_INITIALIZER(0);

    /// Display resolution height
    Uint32 Height                   DEFAULT_INITIALIZER(0);

    /// Display format
    TEXTURE_FORMAT Format           DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);

    /// Refresh rate numerator
    Uint32 RefreshRateNumerator     DEFAULT_INITIALIZER(0);

    /// Refresh rate denominator
    Uint32 RefreshRateDenominator   DEFAULT_INITIALIZER(0);

    /// The scanline drawing mode. 
    enum SCALING_MODE Scaling           DEFAULT_INITIALIZER(SCALING_MODE_UNSPECIFIED);

    /// The scaling mode. 
    enum SCANLINE_ORDER ScanlineOrder   DEFAULT_INITIALIZER(SCANLINE_ORDER_UNSPECIFIED);
};
typedef struct DisplayModeAttribs DisplayModeAttribs;

/// Defines allowed swap chain usage flags
DILIGENT_TYPED_ENUM(SWAP_CHAIN_USAGE_FLAGS, Uint32)
{
    /// No allowed usage
    SWAP_CHAIN_USAGE_NONE           = 0x00L,
        
    /// Swap chain can be used as render target ouput
    SWAP_CHAIN_USAGE_RENDER_TARGET  = 0x01L,

    /// Swap chain images can be used as shader inputs
    SWAP_CHAIN_USAGE_SHADER_INPUT   = 0x02L,

    /// Swap chain images can be used as source of copy operation
    SWAP_CHAIN_USAGE_COPY_SOURCE    = 0x04L
};
DEFINE_FLAG_ENUM_OPERATORS(SWAP_CHAIN_USAGE_FLAGS)


/// The transform applied to the image content prior to presentation.
DILIGENT_TYPED_ENUM(SURFACE_TRANSFORM, Uint32)
{
    /// Uset the most optimal surface transform.
    SURFACE_TRANSFORM_OPTIMAL = 0,

    /// The image content is presented without being transformed.
    SURFACE_TRANSFORM_IDENTITY,

    /// The image content is rotated 90 degrees clockwise.
    SURFACE_TRANSFORM_ROTATE_90,

    /// The image content is rotated 180 degrees clockwise.
    SURFACE_TRANSFORM_ROTATE_180,

    /// The image content is rotated 270 degrees clockwise.
    SURFACE_TRANSFORM_ROTATE_270,

    /// The image content is mirrored horizontally.
    SURFACE_TRANSFORM_HORIZONTAL_MIRROR,

    /// The image content is mirrored horizontally, then rotated 90 degrees clockwise.
    SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90,

    /// The  image content is mirrored horizontally, then rotated 180 degrees clockwise.
    SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180,

    /// The  image content is mirrored horizontally, then rotated 270 degrees clockwise.
    SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270
};


/// Swap chain description
struct SwapChainDesc
{
    /// The swap chain width. Default value is 0
    Uint32 Width                        DEFAULT_INITIALIZER(0);

    /// The swap chain height. Default value is 0
    Uint32 Height                       DEFAULT_INITIALIZER(0);
        
    /// Back buffer format. Default value is Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB
    TEXTURE_FORMAT ColorBufferFormat    DEFAULT_INITIALIZER(TEX_FORMAT_RGBA8_UNORM_SRGB);
        
    /// Depth buffer format. Default value is Diligent::TEX_FORMAT_D32_FLOAT.
    /// Use Diligent::TEX_FORMAT_UNKNOWN to create the swap chain without depth buffer.
    TEXTURE_FORMAT DepthBufferFormat    DEFAULT_INITIALIZER(TEX_FORMAT_D32_FLOAT);

    /// Swap chain usage flags. Default value is Diligent::SWAP_CHAIN_USAGE_RENDER_TARGET
    SWAP_CHAIN_USAGE_FLAGS Usage        DEFAULT_INITIALIZER(SWAP_CHAIN_USAGE_RENDER_TARGET);

    /// The transform, relative to the presentation engine's natural orientation,
    /// applied to the image content prior to presentation.
    ///
    /// \note When default value (SURFACE_TRANSFORM_OPTIMAL) is used, the engine will
    ///       select the most optimal surface transformation. An application may request
    ///       specific transform (e.g. SURFACE_TRANSFORM_IDENTITY) and the engine will
    ///       try to use that. However, if the transform is not available, the engine will
    ///       select the most optimal transform.
    ///       After the swap chain has been created, this member will contain the actual
    ///       transform selected by the engine and can be queried through ISwapChain::GetDesc()
    ///       method.
    SURFACE_TRANSFORM PreTransform      DEFAULT_INITIALIZER(SURFACE_TRANSFORM_OPTIMAL);

    /// The number of buffers in the swap chain
    Uint32 BufferCount                  DEFAULT_INITIALIZER(2);
    
    /// Default depth value, which is used as optimized depth clear value in D3D12
    Float32 DefaultDepthValue           DEFAULT_INITIALIZER(1.f);

    /// Default stencil value, which is used as optimized stencil clear value in D3D12
    Uint8 DefaultStencilValue           DEFAULT_INITIALIZER(0);

    /// Indicates if this is a primary swap chain. When Present() is called
    /// for the primary swap chain, the engine releases stale resources.
    bool  IsPrimary                     DEFAULT_INITIALIZER(true);

#if DILIGENT_CPP_INTERFACE
    SwapChainDesc()noexcept{}

    /// Constructor intializes the structure members with default values
    SwapChainDesc(Uint32         _Width,
                  Uint32         _Height,
                  TEXTURE_FORMAT _ColorBufferFormat,
                  TEXTURE_FORMAT _DepthBufferFormat,
                  Uint32         _BufferCount         = SwapChainDesc{}.BufferCount,
                  Float32        _DefaultDepthValue   = SwapChainDesc{}.DefaultDepthValue,
                  Uint8          _DefaultStencilValue = SwapChainDesc{}.DefaultStencilValue,
                  bool           _IsPrimary           = SwapChainDesc{}.IsPrimary) :
        Width               {_Width              },
        Height              {_Height             },
        ColorBufferFormat   {_ColorBufferFormat  },
        DepthBufferFormat   {_DepthBufferFormat  },
        BufferCount         {_BufferCount        },
        DefaultDepthValue   {_DefaultDepthValue  },
        DefaultStencilValue {_DefaultStencilValue},
        IsPrimary           {_IsPrimary          }
    {}
#endif
};
typedef struct SwapChainDesc SwapChainDesc;

/// Full screen mode description
/// \sa <a href = "https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_swap_chain_fullscreen_desc">DXGI_SWAP_CHAIN_FULLSCREEN_DESC structure on MSDN</a>, 
struct FullScreenModeDesc
{
    /// A Boolean value that specifies whether the swap chain is in fullscreen mode.
    Bool Fullscreen                 DEFAULT_INITIALIZER(False);

    /// Refresh rate numerator
    Uint32 RefreshRateNumerator     DEFAULT_INITIALIZER(0);

    /// Refresh rate denominator
    Uint32 RefreshRateDenominator   DEFAULT_INITIALIZER(0);

    /// The scanline drawing mode. 
    enum SCALING_MODE    Scaling         DEFAULT_INITIALIZER(SCALING_MODE_UNSPECIFIED);

    /// The scaling mode. 
    enum SCANLINE_ORDER  ScanlineOrder   DEFAULT_INITIALIZER(SCANLINE_ORDER_UNSPECIFIED);
};
typedef struct FullScreenModeDesc FullScreenModeDesc;


/// Query type.
enum QUERY_TYPE
{
    /// Query type is undefined.
    QUERY_TYPE_UNDEFINED = 0,

    /// Gets the number of samples that passed the depth and stencil tests in between IDeviceContext::BeginQuery
    /// and IDeviceContext::EndQuery. IQuery::GetData fills a Diligent::QueryDataOcclusion struct.
    QUERY_TYPE_OCCLUSION,

    /// Acts like QUERY_TYPE_OCCLUSION except that it returns simply a binary true/false result: false indicates that no samples
    /// passed depth and stencil testing, true indicates that at least one sample passed depth and stencil testing.
    /// IQuery::GetData fills a Diligent::QueryDataBinaryOcclusion struct.
    QUERY_TYPE_BINARY_OCCLUSION,

    /// Gets the GPU timestamp corresponding to IDeviceContext::EndQuery call. Fot this query
    /// type IDeviceContext::BeginQuery is disabled. IQuery::GetData fills a Diligent::QueryDataTimestamp struct.
    QUERY_TYPE_TIMESTAMP,

    /// Gets pipeline statistics, such as the number of pixel shader invocations in between IDeviceContext::BeginQuery
    /// and IDeviceContext::EndQuery. IQuery::GetData fills a Diligent::QueryDataPipelineStatistics struct.
    QUERY_TYPE_PIPELINE_STATISTICS,

    /// Gets the number of high-frequency counter ticks between IDeviceContext::BeginQuery and
    /// IDeviceContext::EndQuery calls. IQuery::GetData fills a Diligent::QueryDataDuration struct.
    QUERY_TYPE_DURATION,

    /// The number of query types in the enum
    QUERY_TYPE_NUM_TYPES
};



/// Device type
enum RENDER_DEVICE_TYPE
{
    RENDER_DEVICE_TYPE_UNDEFINED = 0,  ///< Undefined device
    RENDER_DEVICE_TYPE_D3D11,          ///< D3D11 device
    RENDER_DEVICE_TYPE_D3D12,          ///< D3D12 device
    RENDER_DEVICE_TYPE_GL,             ///< OpenGL device 
    RENDER_DEVICE_TYPE_GLES,           ///< OpenGLES device
    RENDER_DEVICE_TYPE_VULKAN,         ///< Vulkan device
    RENDER_DEVICE_TYPE_METAL           ///< Metal device (not yet implemented)
};


/// Texture sampler capabilities
struct SamplerCaps
{
    /// Indicates if device supports border texture addressing mode
    Bool BorderSamplingModeSupported   DEFAULT_INITIALIZER(False);

    /// Indicates if device supports anisotrpoic filtering
    Bool AnisotropicFilteringSupported DEFAULT_INITIALIZER(False);

    /// Indicates if device supports MIP load bias
    Bool LODBiasSupported              DEFAULT_INITIALIZER(False);
};
typedef struct SamplerCaps SamplerCaps;


/// Texture capabilities
struct TextureCaps
{
    /// Maximum dimension (width) of a 1D texture, or 0 if 1D textures are not supported.
    Uint32 MaxTexture1DDimension   DEFAULT_INITIALIZER(0);

    /// Maximum number of slices in a 1D texture array, or 0 if 1D texture arrays are not supported.
    Uint32 MaxTexture1DArraySlices DEFAULT_INITIALIZER(0);

    /// Maximum dimension (width or height) of a 2D texture.
    Uint32 MaxTexture2DDimension   DEFAULT_INITIALIZER(0);

    /// Maximum number of slices in a 2D texture array, or 0 if 2D texture arrays are not supported.
    Uint32 MaxTexture2DArraySlices DEFAULT_INITIALIZER(0);

    /// Maximum dimension (width, height, or depth) of a 3D texture, or 0 if 3D textures are not supported.
    Uint32 MaxTexture3DDimension   DEFAULT_INITIALIZER(0);

    /// Maximum dimension (width or height) of a cubemap face, or 0 if cubemap textures are not supported.
    Uint32 MaxTextureCubeDimension DEFAULT_INITIALIZER(0);

    /// Indicates if device supports 2D multisampled textures
    Bool Texture2DMSSupported      DEFAULT_INITIALIZER(False);

    /// Indicates if device supports 2D multisampled texture arrays
    Bool Texture2DMSArraySupported DEFAULT_INITIALIZER(False);

    /// Indicates if device supports texture views
    Bool TextureViewSupported      DEFAULT_INITIALIZER(False);

    /// Indicates if device supports cubemap arrays
    Bool CubemapArraysSupported    DEFAULT_INITIALIZER(False);
};
typedef struct TextureCaps TextureCaps;
    

/// Device feature state
DILIGENT_TYPED_ENUM(DEVICE_FEATURE_STATE, Uint8)
{
    /// Device feature is disabled.
    DEVICE_FEATURE_STATE_DISABLED = 0,

    /// Device feature is enabled.

    /// If a feature is requested to be enabled during the initialization through
    /// EngineCreateInfo::Feautures, but is not supported by the device/driver/platform,
    /// the engine will fail to initialize.
    DEVICE_FEATURE_STATE_ENABLED = 1,

    /// Device feature is optional. 

    /// During the initialization the engine will attempt to enable the feature.
    /// If the feature is not supported by the device/driver/platform,
    /// the engine will successfully be initialized, but the feature will be disabled.
    /// The actual feature state can be queried from DeviceCaps structure.
    DEVICE_FEATURE_STATE_OPTIONAL = 2
};


/// Describes the device features
struct DeviceFeatures
{
    /// Indicates if device supports separable programs
    DEVICE_FEATURE_STATE SeparablePrograms             DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports resource queries from shader objects.

    /// \ note  This feature indicates if IShader::GetResourceCount() and IShader::GetResourceDesc() methods
    ///         can be used to query the list of resources of individual shader objects.
    ///         Shader variable queries from pipeline state and shader resource binding objects are always
    ///         available.
    ///
    ///         The feature is always enabled in Direct3D11, Direct3D12 and Vulkan. It is enabled in
    ///         OpenGL when separable programs are available, and it is always disabled in Metal.
    DEVICE_FEATURE_STATE ShaderResourceQueries         DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports indirect draw commands
    DEVICE_FEATURE_STATE IndirectRendering             DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports wireframe fill mode
    DEVICE_FEATURE_STATE WireframeFill                 DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports multithreaded resource creation
    DEVICE_FEATURE_STATE MultithreadedResourceCreation DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports compute shaders
    DEVICE_FEATURE_STATE ComputeShaders                DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports geometry shaders
    DEVICE_FEATURE_STATE GeometryShaders               DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);
        
    /// Indicates if device supports tessellation
    DEVICE_FEATURE_STATE Tessellation                  DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);
        
    /// Indicates if device supports mesh and amplification shaders
    DEVICE_FEATURE_STATE MeshShaders                   DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports bindless resources
    DEVICE_FEATURE_STATE BindlessResources             DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports occlusion queries (see Diligent::QUERY_TYPE_OCCLUSION).
    DEVICE_FEATURE_STATE OcclusionQueries              DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports binary occlusion queries (see Diligent::QUERY_TYPE_BINARY_OCCLUSION).
    DEVICE_FEATURE_STATE BinaryOcclusionQueries        DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports timestamp queries (see Diligent::QUERY_TYPE_TIMESTAMP).
    DEVICE_FEATURE_STATE TimestampQueries              DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports pipeline statistics queries (see Diligent::QUERY_TYPE_PIPELINE_STATISTICS).
    DEVICE_FEATURE_STATE PipelineStatisticsQueries     DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports duration queries (see Diligent::QUERY_TYPE_DURATION).
    DEVICE_FEATURE_STATE DurationQueries               DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports depth bias clamping
    DEVICE_FEATURE_STATE DepthBiasClamp                DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports depth clamping
    DEVICE_FEATURE_STATE DepthClamp                    DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports depth clamping
    DEVICE_FEATURE_STATE IndependentBlend              DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports dual-source blend
    DEVICE_FEATURE_STATE DualSourceBlend               DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports multiviewport
    DEVICE_FEATURE_STATE MultiViewport                 DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports all BC-compressed formats
    DEVICE_FEATURE_STATE TextureCompressionBC          DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports writes to UAVs as well as atomic operations in vertex,
    /// tessellation, and geometry shader stages.
    DEVICE_FEATURE_STATE VertexPipelineUAVWritesAndAtomics DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports writes to UAVs as well as atomic operations in pixel
    /// shader stage.
    DEVICE_FEATURE_STATE PixelUAVWritesAndAtomics          DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Specifies whether all the extended UAV texture formats are available in shader code.
    DEVICE_FEATURE_STATE TextureUAVExtendedFormats         DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports native 16-bit float operations. Note that there are separate features
    /// that indicate if device supports loading 16-bit floats from buffers and passing them between shader stages.
    /// 
    /// \note   16-bit support is quite tricky, the following post should help understand it better:
    ///         https://therealmjp.github.io/posts/shader-fp16/
    DEVICE_FEATURE_STATE ShaderFloat16                     DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports reading and writing 16-bit floats and ints from buffers bound
    /// as shader resource or unordered access views.
    DEVICE_FEATURE_STATE ResourceBuffer16BitAccess         DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports reading 16-bit floats and ints from uniform buffers.
    DEVICE_FEATURE_STATE UniformBuffer16BitAccess          DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if 16-bit floats and ints can be used as input/output of a shader entry point.
    DEVICE_FEATURE_STATE ShaderInputOutput16               DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports native 8-bit integer operations.
    DEVICE_FEATURE_STATE ShaderInt8                        DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports reading and writing 8-bit types from buffers bound
    /// as shader resource or unordered access views.
    DEVICE_FEATURE_STATE ResourceBuffer8BitAccess         DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);

    /// Indicates if device supports reading 8-bit types from uniform buffers.
    DEVICE_FEATURE_STATE UniformBuffer8BitAccess          DEFAULT_INITIALIZER(DEVICE_FEATURE_STATE_DISABLED);


#if DILIGENT_CPP_INTERFACE
    DeviceFeatures() noexcept {}

    explicit DeviceFeatures(DEVICE_FEATURE_STATE State) noexcept :
        SeparablePrograms                 {State},
        ShaderResourceQueries             {State},
        IndirectRendering                 {State},
        WireframeFill                     {State},
        MultithreadedResourceCreation     {State},
        ComputeShaders                    {State},
        GeometryShaders                   {State},
        Tessellation                      {State},
        MeshShaders                       {State},
        BindlessResources                 {State},
        OcclusionQueries                  {State},
        BinaryOcclusionQueries            {State},
        TimestampQueries                  {State},
        PipelineStatisticsQueries         {State},
        DurationQueries                   {State},
        DepthBiasClamp                    {State},
        DepthClamp                        {State},
        IndependentBlend                  {State},
        DualSourceBlend                   {State},
        MultiViewport                     {State},
        TextureCompressionBC              {State},
        VertexPipelineUAVWritesAndAtomics {State},
        PixelUAVWritesAndAtomics          {State},
        TextureUAVExtendedFormats         {State},
        ShaderFloat16                     {State},
        ResourceBuffer16BitAccess         {State},
        UniformBuffer16BitAccess          {State},
        ShaderInputOutput16               {State},
        ShaderInt8                        {State},
        ResourceBuffer8BitAccess          {State},
        UniformBuffer8BitAccess           {State}
    {
#   if defined(_MSC_VER) && defined(_WIN64)
        static_assert(sizeof(*this) == 31, "Did you add a new feature to DeviceFeatures? Please handle its status above.");
#   endif
    }
#endif
};
typedef struct DeviceFeatures DeviceFeatures;


/// Graphics adapter vendor
DILIGENT_TYPED_ENUM(ADAPTER_VENDOR, Uint8)
{
    /// Adapter vendor is unknown
    ADAPTER_VENDOR_UNKNOWN = 0,

    /// Adapter vendor is NVidia
    ADAPTER_VENDOR_NVIDIA,

    /// Adapter vendor is AMD
    ADAPTER_VENDOR_AMD,

    /// Adapter vendor is Intel
    ADAPTER_VENDOR_INTEL,

    /// Adapter vendor is ARM
    ADAPTER_VENDOR_ARM,

    /// Adapter vendor is Qualcomm
    ADAPTER_VENDOR_QUALCOMM,

    /// Adapter vendor is Imagination Technologies
    ADAPTER_VENDOR_IMGTECH,

    /// Adapter vendor is Microsoft (software rasterizer)
    ADAPTER_VENDOR_MSFT
};

/// Graphics adapter properties
struct GraphicsAdapterInfo
{
    /// A string that contains the adapter description.
    char Description[128]   DEFAULT_INITIALIZER({});

    /// Adapter type, see Diligent::ADAPTER_TYPE.
    ADAPTER_TYPE   Type     DEFAULT_INITIALIZER(ADAPTER_TYPE_UNKNOWN);

    /// Adapter vendor, see Diligent::ADAPTER_VENDOR.
    ADAPTER_VENDOR Vendor   DEFAULT_INITIALIZER(ADAPTER_VENDOR_UNKNOWN);

    /// The PCI ID of the hardware vendor (if available).
    Uint32 VendorId         DEFAULT_INITIALIZER(0);

    /// The PCI ID of the hardware device (if available).
    Uint32 DeviceId         DEFAULT_INITIALIZER(0);

    /// Number of video outputs this adapter has (if available).
    Uint32 NumOutputs       DEFAULT_INITIALIZER(0);

    /// The amount of local video memory that is inaccessible by CPU, in bytes.

    /// \note Device-local memory is where USAGE_DEFAULT and USAGE_IMMUTABLE resources
    ///       are typically allocated.
    ///
    ///       On some devices it may not be possible to query the memory size,
    ///       in which case all memory sizes will be zero.
    Uint64  DeviceLocalMemory   DEFAULT_INITIALIZER(0);


    /// The amount of host-visible memory that can be accessed by CPU and is visible by GPU, in bytes.

    /// \note Host-visible memory is where USAGE_DYNAMIC and USAGE_STAGING resources
    ///       are typically allocated.
    Uint64  HostVisibileMemory  DEFAULT_INITIALIZER(0);


    /// The amount of unified memory that can be directly accessed by both CPU and GPU, in bytes.

    /// \note Unified memory is where USAGE_UNIFIED resources are typically allocated, but
    ///       resourecs with other usages may be allocated as well if there is no corresponding
    ///       memory type.
    Uint64  UnifiedMemory       DEFAULT_INITIALIZER(0);

    /// Supported access types for the unified memory.
    CPU_ACCESS_FLAGS UnifiedMemoryCPUAccess DEFAULT_INITIALIZER(CPU_ACCESS_NONE);
};
typedef struct GraphicsAdapterInfo GraphicsAdapterInfo;


/// Device capabilities
struct DeviceCaps
{
    /// Device type. See Diligent::DeviceType.
    enum RENDER_DEVICE_TYPE DevType DEFAULT_INITIALIZER(RENDER_DEVICE_TYPE_UNDEFINED);

    /// Major revision of the graphics API supported by the graphics adapter.
    /// Note that this value indicates the maximum supported feature level, so,
    /// for example, if the device type is D3D11, this value will be 10 when 
    /// the maximum supported Direct3D feature level of the graphics adapter is 10.0.
    Int32 MajorVersion DEFAULT_INITIALIZER(0);

    /// Minor revision of the graphics API supported by the graphics adapter.
    /// Similar to MajorVersion, this value indicates the maximum supported feature level.
    Int32 MinorVersion DEFAULT_INITIALIZER(0);

    /// Adapter info, see Diligent::GraphicsAdapterInfo.
    GraphicsAdapterInfo AdapterInfo;

    /// Texture sampling capabilities. See Diligent::SamplerCaps.
    SamplerCaps SamCaps;

    /// Texture capabilities. See Diligent::TextureCaps.
    TextureCaps TexCaps;

    /// Device features. See Diligent::DeviceFeatures.

    /// \note For optional features requested during the initialization, the
    ///       struct will indicate the actual feature state (enabled or disabled).
    DeviceFeatures Features;

#if DILIGENT_CPP_INTERFACE
    bool IsGLDevice()const
    {
        return DevType == RENDER_DEVICE_TYPE_GL || DevType == RENDER_DEVICE_TYPE_GLES;
    }
    bool IsD3DDevice()const
    {
        return DevType == RENDER_DEVICE_TYPE_D3D11 || DevType == RENDER_DEVICE_TYPE_D3D12;
    }
    bool IsVulkanDevice()const
    {
        return DevType == RENDER_DEVICE_TYPE_VULKAN;
    }
    bool IsMetalDevice()const
    {
        return DevType == RENDER_DEVICE_TYPE_METAL;
    }

    struct NDCAttribs
    {
        const float MinZ;          // Minimum z value of normalized device coordinate space
        const float ZtoDepthScale; // NDC z to depth scale
        const float YtoVScale;     // Scale to transform NDC y coordinate to texture V coordinate

        float GetZtoDepthBias() const
        {
            // Returns ZtoDepthBias such that given NDC z coordinate, depth value can be
            // computed as follows:
            // d = z * ZtoDepthScale + ZtoDepthBias
            return -MinZ * ZtoDepthScale;
        }
    };

    const NDCAttribs& GetNDCAttribs()const
    {
        if (IsVulkanDevice())
        {
            // Note that Vulkan itself does not invert Y coordinate when transforming
            // normalized device Y to window space. However, we use negative viewport
            // height which achieves the same effect as in D3D, thererfore we need to
            // invert y (see comments in DeviceContextVkImpl::CommitViewports() for details)
            static constexpr const NDCAttribs NDCAttribsVk {0.0f, 1.0f, -0.5f};
            return NDCAttribsVk;
        }
        else if (IsD3DDevice())
        {
            static constexpr const NDCAttribs NDCAttribsD3D {0.0f, 1.0f, -0.5f};
            return NDCAttribsD3D;
        }
        else if (IsGLDevice())
        {
            static constexpr const NDCAttribs NDCAttribsGL {-1.0f, 0.5f, 0.5f};
            return NDCAttribsGL;
        }
        else
        {
            static constexpr const NDCAttribs NDCAttribsDefault {0.0f, 1.0f, 0.5f};
            return NDCAttribsDefault;
        }
    }
#endif
};
typedef struct DeviceCaps DeviceCaps;


/// Engine creation attibutes
struct EngineCreateInfo
{
    /// API version number.
    Int32                    APIVersion             DEFAULT_INITIALIZER(DILIGENT_API_VERSION);

    /// Number of deferred contexts to create when initializing the engine. If non-zero number 
    /// is given, pointers to the contexts are written to ppContexts array by the engine factory 
    /// functions (IEngineFactoryD3D11::CreateDeviceAndContextsD3D11,
    /// IEngineFactoryD3D12::CreateDeviceAndContextsD3D12, and IEngineFactoryVk::CreateDeviceAndContextsVk)
    /// starting at position 1.
    Uint32                   NumDeferredContexts    DEFAULT_INITIALIZER(0);

    /// Requested device features.

    /// \remarks    If a feature is requested to be enabled, but is not supported
    ///             by the device/driver/platform, the engine will fail to initialize.
    ///
    ///             If a feature is requested to be optioanl, the engine will attempt to enable the feature.
    ///             If the feature is not supported by the device/driver/platform,
    ///             the engine will successfully be initialized, but the feature will be disabled.
    ///             The actual feature state can be queried from DeviceCaps structure.
    DeviceFeatures Features;

    /// Pointer to the raw memory allocator that will be used for all memory allocation/deallocation
    /// operations in the engine
    struct IMemoryAllocator* pRawMemAllocator       DEFAULT_INITIALIZER(nullptr);

    /// Pointer to the user-specified debug message callback function
    DebugMessageCallbackType DebugMessageCallback   DEFAULT_INITIALIZER(nullptr);
};
typedef struct EngineCreateInfo EngineCreateInfo;

/// Attributes of the OpenGL-based engine implementation
struct EngineGLCreateInfo DILIGENT_DERIVE(EngineCreateInfo)

	/// Native window wrapper
	NativeWindow Window;

    /// Create debug OpenGL context and enable debug output.

    /// Debug contexts are intended for use during application development, and
    /// provide additional runtime checking, validation, and logging
    /// functionality while possibly incurring performance penalties
    bool CreateDebugContext     DEFAULT_INITIALIZER(false);

    /// Force using non-separable programs.

    /// Setting this to true is typically needed for testing purposes only.
    bool ForceNonSeparablePrograms DEFAULT_INITIALIZER(false);
};
typedef struct EngineGLCreateInfo EngineGLCreateInfo;


/// Debug flags that can be specified when creating Direct3D11-based engine implementation.
///
/// \sa CreateDeviceAndContextsD3D11Type, CreateSwapChainD3D11Type, LoadGraphicsEngineD3D11
DILIGENT_TYPED_ENUM(D3D11_DEBUG_FLAGS, Uint32)
{
    /// No debug flag
    D3D11_DEBUG_FLAG_NONE                                = 0x00,

    /// Whether to create Direct3D11 debug device
    D3D11_DEBUG_FLAG_CREATE_DEBUG_DEVICE                 = 0x01,

    /// Before executing draw/dispatch command, verify that
    /// all required shader resources are bound to the device context
    D3D11_DEBUG_FLAG_VERIFY_COMMITTED_SHADER_RESOURCES   = 0x02,

    /// Verify that all committed cotext resources are relevant,
    /// i.e. they are consistent with the committed resource cache.
    /// This is very expensive and should generally not be necessary.
    D3D11_DEBUG_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE = 0x04
};
DEFINE_FLAG_ENUM_OPERATORS(D3D11_DEBUG_FLAGS)

/// Direct3D11/12 feature level
DILIGENT_TYPED_ENUM(DIRECT3D_FEATURE_LEVEL, Uint8)
{
    /// Feature level 10.0
    DIRECT3D_FEATURE_LEVEL_10_0,

    /// Feature level 10.1
    DIRECT3D_FEATURE_LEVEL_10_1,

    /// Feature level 11.0
    DIRECT3D_FEATURE_LEVEL_11_0,

    /// Feature level 11.1
    DIRECT3D_FEATURE_LEVEL_11_1,

    /// Feature level 12.0
    DIRECT3D_FEATURE_LEVEL_12_0,

    /// Feature level 12.1
    DIRECT3D_FEATURE_LEVEL_12_1
};

static const Uint32 DEFAULT_ADAPTER_ID = 0xFFFFFFFFU;

/// Attributes specific to D3D11 engine
struct EngineD3D11CreateInfo DILIGENT_DERIVE(EngineCreateInfo)
           
    /// Id of the hardware adapter the engine should be initialized on.
    Uint32                 AdapterId           DEFAULT_INITIALIZER(DEFAULT_ADAPTER_ID);

    /// Minimum required Direct3D feature level.
    DIRECT3D_FEATURE_LEVEL MinimumFeatureLevel DEFAULT_INITIALIZER(DIRECT3D_FEATURE_LEVEL_11_0);

    /// Debug flags. See Diligent::D3D11_DEBUG_FLAGS for a list of allowed values.
    ///
    /// \sa CreateDeviceAndContextsD3D11Type, CreateSwapChainD3D11Type, LoadGraphicsEngineD3D11
    D3D11_DEBUG_FLAGS      DebugFlags          DEFAULT_INITIALIZER(D3D11_DEBUG_FLAG_NONE);
};
typedef struct EngineD3D11CreateInfo EngineD3D11CreateInfo;


/// Attributes specific to D3D12 engine
struct EngineD3D12CreateInfo DILIGENT_DERIVE(EngineCreateInfo)

    /// Name of the D3D12 DLL to load. Ignored on UWP.
    const char* D3D12DllName       DEFAULT_INITIALIZER("d3d12.dll");

    /// Id of the hardware adapter the engine should be initialized on.
    Uint32      AdapterId          DEFAULT_INITIALIZER(DEFAULT_ADAPTER_ID);

    /// Minimum required Direct3D feature level.
    DIRECT3D_FEATURE_LEVEL MinimumFeatureLevel DEFAULT_INITIALIZER(DIRECT3D_FEATURE_LEVEL_11_0);

    /// Enable Direct3D12 debug layer.
    bool EnableDebugLayer           DEFAULT_INITIALIZER(false);

    /// Enable validation on the GPU timeline.
    /// See https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation
    /// This flag only has effect if EnableDebugLayer is true.
    /// \note Enabling this option may slow things down a lot.
    bool EnableGPUBasedValidation   DEFAULT_INITIALIZER(false);

    /// Whether to break execution when D3D12 debug layer detects an error.
    /// This flag only has effect if EnableDebugLayer is true.
    bool BreakOnError               DEFAULT_INITIALIZER(false);

    /// Whether to break execution when D3D12 debug layer detects a memory corruption.
    /// This flag only has effect if EnableDebugLayer is true.
    bool BreakOnCorruption          DEFAULT_INITIALIZER(true);

    /// Size of the CPU descriptor heap allocations for different heap types.
    Uint32 CPUDescriptorHeapAllocationSize[4]
#if DILIGENT_CPP_INTERFACE
        {
            8192,  // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            2048,  // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
            1024,  // D3D12_DESCRIPTOR_HEAP_TYPE_RTV
            1024   // D3D12_DESCRIPTOR_HEAP_TYPE_DSV
        }
#endif
    ;

    /// The size of the GPU descriptor heap region designated to static/mutable
    /// shader resource variables.
    /// Every Shader Resource Binding object allocates one descriptor
    /// per any static/mutable shader resource variable (every array 
    /// element counts) when the object is created. All required descriptors
    /// are allocated in one continuous chunk.
    /// GPUDescriptorHeapSize defines the total number of all descriptors
    /// that can be allocated across all SRB objects.
    /// Note that due to heap fragmentation, releaseing two chunks of sizes
    /// N and M does not necessarily make the chunk of size N+M available.
    Uint32 GPUDescriptorHeapSize[2]
#if DILIGENT_CPP_INTERFACE
        {
            16384, // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            1024   // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        }
#endif
    ;

    /// The size of the GPU descriptor heap region designated to dynamic
    /// shader resource variables.
    /// Every Shader Resource Binding object allocates one descriptor
    /// per any dynamic shader resource variable (every array element counts)
    /// every time the object is commited via IDeviceContext::CommitShaderResources.
    /// All used dynamic descriptors are discarded at the end of the frame
    /// and recycled when they are no longer used by the GPU.
    /// GPUDescriptorHeapDynamicSize defines the total number of descriptors
    /// that can be used for dynamic variables across all SRBs and all frames
    /// currently in flight.
    /// Note that in Direct3D12, the size of sampler descriptor heap is limited
    /// by 2048. Since Diligent Engine allocates single heap for all variable types,
    /// GPUDescriptorHeapSize[1] + GPUDescriptorHeapDynamicSize[1] must not
    /// exceed 2048.
    Uint32 GPUDescriptorHeapDynamicSize[2]
#if DILIGENT_CPP_INTERFACE
        {
            8192,  // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            1024   // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        }
#endif
    ;

    /// The size of the chunk that dynamic descriptor allocations manager
    /// requests from the main GPU descriptor heap.
    /// The total number of dynamic descriptors avaialble across all frames in flight is 
    /// defined by GPUDescriptorHeapDynamicSize. Every device context allocates dynamic
    /// descriptors in two stages: it first requests a chunk from the global heap, and the 
    /// performs linear suballocations from this chunk in a lock-free manner. The size of 
    /// this chunk is defined by DynamicDescriptorAllocationChunkSize, thus there will be total
    /// GPUDescriptorHeapDynamicSize/DynamicDescriptorAllocationChunkSize chunks in
    /// the heap of each type.
    Uint32 DynamicDescriptorAllocationChunkSize[2]
#if DILIGENT_CPP_INTERFACE
        {
            256,  // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            32    // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        }
#endif
    ;

    /// Number of commands to flush the command list. Only draw/dispatch commands count
    /// towards the limit. Command lists are only flushed when pipeline state is changed
    /// or when backbuffer is presented.
    Uint32 NumCommandsToFlushCmdList DEFAULT_INITIALIZER(256);

    /// A device context uses dynamic heap when it needs to allocate temporary
    /// CPU-accessible memory to update a resource via IBufer::UpdateData() or
    /// ITexture::UpdateData(), or to map dynamic resources.
    /// Device contexts first request a chunk of memory from global dynamic
    /// resource manager and then suballocate from this chunk in a lock-free
    /// fashion. DynamicHeapPageSize defines the size of this chunk.
    Uint32 DynamicHeapPageSize       DEFAULT_INITIALIZER(1 << 20);

    /// Number of dynamic heap pages that will be reserved by the
    /// global dynamic heap manager to avoid page creation at run time.
    Uint32 NumDynamicHeapPagesToReserve DEFAULT_INITIALIZER(1);

    /// Query pool size for each query type.
    Uint32 QueryPoolSizes[QUERY_TYPE_NUM_TYPES]
#if DILIGENT_CPP_INTERFACE
        {
            0,   // Ignored
            128, // QUERY_TYPE_OCCLUSION
            128, // QUERY_TYPE_BINARY_OCCLUSION
            512, // QUERY_TYPE_TIMESTAMP
            128, // QUERY_TYPE_PIPELINE_STATISTICS
            256, // QUERY_TYPE_DURATION
        }
#endif
    ;

    /// Path to DirectX Shader Compiler, which is required to use Shader Model 6.0+ features.
    /// By default, the engine will search for "dxcompiler.dll".
    const char* pDxCompilerPath DEFAULT_INITIALIZER(nullptr);
};
typedef struct EngineD3D12CreateInfo EngineD3D12CreateInfo;


/// Descriptor pool size
struct VulkanDescriptorPoolSize
{
    Uint32 MaxDescriptorSets                DEFAULT_INITIALIZER(0);
    Uint32 NumSeparateSamplerDescriptors    DEFAULT_INITIALIZER(0);
    Uint32 NumCombinedSamplerDescriptors    DEFAULT_INITIALIZER(0);
    Uint32 NumSampledImageDescriptors       DEFAULT_INITIALIZER(0);
    Uint32 NumStorageImageDescriptors       DEFAULT_INITIALIZER(0);
    Uint32 NumUniformBufferDescriptors      DEFAULT_INITIALIZER(0);
    Uint32 NumStorageBufferDescriptors      DEFAULT_INITIALIZER(0);
    Uint32 NumUniformTexelBufferDescriptors DEFAULT_INITIALIZER(0);
    Uint32 NumStorageTexelBufferDescriptors DEFAULT_INITIALIZER(0);
    Uint32 NumInputAttachmentDescriptors    DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    VulkanDescriptorPoolSize()noexcept {}

    VulkanDescriptorPoolSize(Uint32 _MaxDescriptorSets,
                             Uint32 _NumSeparateSamplerDescriptors,
                             Uint32 _NumCombinedSamplerDescriptors,
                             Uint32 _NumSampledImageDescriptors,
                             Uint32 _NumStorageImageDescriptors,
                             Uint32 _NumUniformBufferDescriptors,
                             Uint32 _NumStorageBufferDescriptors,
                             Uint32 _NumUniformTexelBufferDescriptors,
                             Uint32 _NumStorageTexelBufferDescriptors,
                             Uint32 _NumInputAttachmentDescriptors)noexcept :
        MaxDescriptorSets               {_MaxDescriptorSets               },
        NumSeparateSamplerDescriptors   {_NumSeparateSamplerDescriptors   },
        NumCombinedSamplerDescriptors   {_NumCombinedSamplerDescriptors   },
        NumSampledImageDescriptors      {_NumSampledImageDescriptors      },
        NumStorageImageDescriptors      {_NumStorageImageDescriptors      },
        NumUniformBufferDescriptors     {_NumUniformBufferDescriptors     },
        NumStorageBufferDescriptors     {_NumStorageBufferDescriptors     },
        NumUniformTexelBufferDescriptors{_NumUniformTexelBufferDescriptors},
        NumStorageTexelBufferDescriptors{_NumStorageTexelBufferDescriptors},
        NumInputAttachmentDescriptors   {_NumInputAttachmentDescriptors   }
    {
        // On clang aggregate initialization fails to compile if 
        // structure members have default initializers
    }
#endif
};
typedef struct VulkanDescriptorPoolSize VulkanDescriptorPoolSize;


/// Attributes specific to Vulkan engine
struct EngineVkCreateInfo DILIGENT_DERIVE(EngineCreateInfo)
    
    /// Id of the hardware adapter the engine should be initialized on.
    Uint32             AdapterId                DEFAULT_INITIALIZER(DEFAULT_ADAPTER_ID);

    /// Enable Vulkan validation layers.
    bool               EnableValidation         DEFAULT_INITIALIZER(false);
        
    /// Number of global Vulkan extensions
    Uint32             GlobalExtensionCount     DEFAULT_INITIALIZER(0);

    /// List of global Vulkan extensions to enable.
    const char* const* ppGlobalExtensionNames   DEFAULT_INITIALIZER(nullptr);

    /// Allocator used as pAllocator parameter in callse to Vulkan Create* functions
    void* pVkAllocator                          DEFAULT_INITIALIZER(nullptr);

    /// Number of commands to flush the command buffer. Only draw/dispatch commands count
    /// towards the limit. Command buffers are only flushed when pipeline state is changed
    /// or when backbuffer is presented.
    Uint32 NumCommandsToFlushCmdBuffer          DEFAULT_INITIALIZER(256);
        
    /// Size of the main descriptor pool that is used to allocate descriptor sets
    /// for static and mutable variables. If allocation from the current pool fails,
    /// the engine creates another one.
    VulkanDescriptorPoolSize MainDescriptorPoolSize
#if DILIGENT_CPP_INTERFACE
        //Max  SepSm  CmbSm  SmpImg StrImg   UB     SB    UTxB   StTxB  InptAtt
        {8192,  1024,  8192,  8192,  1024,  4096,  4096,  1024,  1024,   256}
#endif
    ;

    /// Size of the dynamic descriptor pool that is used to allocate descriptor sets
    /// for dynamic variables. Every device context has its own dynamic descriptor set allocator.
    /// The allocator requests pools from global dynamic descriptor pool manager, and then 
    /// performs lock-free suballocations from the pool.
    
    VulkanDescriptorPoolSize DynamicDescriptorPoolSize
#if DILIGENT_CPP_INTERFACE
        //Max  SepSm  CmbSm  SmpImg StrImg   UB     SB    UTxB   StTxB  InptAtt
        {2048,   256,  2048,  2048,   256,  1024,  1024,   256,   256,    64}
#endif
    ;

    /// Allocation granularity for device-local memory
    Uint32 DeviceLocalMemoryPageSize        DEFAULT_INITIALIZER(16 << 20);

    /// Allocation granularity for host-visible memory
    Uint32 HostVisibleMemoryPageSize        DEFAULT_INITIALIZER(16 << 20);

    /// Amount of device-local memory reserved by the engine. 
    /// The engine does not pre-allocate the memory, but rather keeps free
    /// pages when resources are released
    Uint32 DeviceLocalMemoryReserveSize     DEFAULT_INITIALIZER(256 << 20);

    /// Amount of host-visible memory reserved by the engine.
    /// The engine does not pre-allocate the memory, but rather keeps free
    /// pages when resources are released
    Uint32 HostVisibleMemoryReserveSize     DEFAULT_INITIALIZER(256 << 20);

    /// Page size of the upload heap that is allocated by immediate/deferred
    /// contexts from the global memory manager to perform lock-free dynamic
    /// suballocations.
    /// Upload heap is used to update resources with UpdateData()
    Uint32 UploadHeapPageSize               DEFAULT_INITIALIZER(1 << 20);

    /// Size of the dynamic heap (the buffer that is used to suballocate 
    /// memory for dynamic resources) shared by all contexts.
    Uint32 DynamicHeapSize                  DEFAULT_INITIALIZER(8 << 20);

    /// Size of the memory chunk suballocated by immediate/deferred context from
    /// the global dynamic heap to perform lock-free dynamic suballocations
    Uint32 DynamicHeapPageSize              DEFAULT_INITIALIZER(256 << 10);

    /// Query pool size for each query type.
    Uint32 QueryPoolSizes[QUERY_TYPE_NUM_TYPES]
#if DILIGENT_CPP_INTERFACE
    {
        0,   // Ignored
        128, // QUERY_TYPE_OCCLUSION
        128, // QUERY_TYPE_BINARY_OCCLUSION
        512, // QUERY_TYPE_TIMESTAMP
        128, // QUERY_TYPE_PIPELINE_STATISTICS
        256  // QUERY_TYPE_DURATION
    }
#endif
    ;

    /// Path to DirectX Shader Compiler, which is required to use Shader Model 6.0+
    /// features when compiling shaders from HLSL.
    const char* pDxCompilerPath DEFAULT_INITIALIZER(nullptr);
};
typedef struct EngineVkCreateInfo EngineVkCreateInfo;


/// Attributes of the Metal-based engine implementation
struct EngineMtlCreateInfo DILIGENT_DERIVE(EngineCreateInfo)

};
typedef struct EngineMtlCreateInfo EngineMtlCreateInfo;


/// Box
struct Box
{
    Uint32 MinX DEFAULT_INITIALIZER(0); ///< Minimal X coordinate. Default value is 0
    Uint32 MaxX DEFAULT_INITIALIZER(0); ///< Maximal X coordinate. Default value is 0
    Uint32 MinY DEFAULT_INITIALIZER(0); ///< Minimal Y coordinate. Default value is 0
    Uint32 MaxY DEFAULT_INITIALIZER(0); ///< Maximal Y coordinate. Default value is 0
    Uint32 MinZ DEFAULT_INITIALIZER(0); ///< Minimal Z coordinate. Default value is 0
    Uint32 MaxZ DEFAULT_INITIALIZER(1); ///< Maximal Z coordinate. Default value is 1

#if DILIGENT_CPP_INTERFACE
    Box(Uint32 _MinX, Uint32 _MaxX,	
        Uint32 _MinY, Uint32 _MaxY,	
        Uint32 _MinZ, Uint32 _MaxZ) noexcept: 	
        MinX {_MinX},
        MaxX {_MaxX},
        MinY {_MinY},
        MaxY {_MaxY},
        MinZ {_MinZ},
        MaxZ {_MaxZ}	
    {}

    Box(Uint32 _MinX, Uint32 _MaxX,	
        Uint32 _MinY, Uint32 _MaxY) noexcept:
        Box{_MinX, _MaxX, _MinY, _MaxY, 0, 1}
    {}

    Box(Uint32 _MinX, Uint32 _MaxX) noexcept:
        Box{_MinX, _MaxX, 0, 0, 0, 1}
    {}

    Box() noexcept {}
#endif
};
typedef struct Box Box;


/// Describes texture format component type
DILIGENT_TYPED_ENUM(COMPONENT_TYPE, Uint8)
{
    /// Undefined component type
    COMPONENT_TYPE_UNDEFINED,

    /// Floating point component type
    COMPONENT_TYPE_FLOAT,

    /// Signed-normalized-integer component type
    COMPONENT_TYPE_SNORM,

    /// Unsigned-normalized-integer component type
    COMPONENT_TYPE_UNORM,

    /// Unsigned-normalized-integer sRGB component type
    COMPONENT_TYPE_UNORM_SRGB,

    /// Signed-integer component type
    COMPONENT_TYPE_SINT,

    /// Unsigned-integer component type
    COMPONENT_TYPE_UINT,

    /// Depth component type
    COMPONENT_TYPE_DEPTH,

    /// Depth-stencil component type
    COMPONENT_TYPE_DEPTH_STENCIL,

    /// Compound component type (example texture formats: TEX_FORMAT_R11G11B10_FLOAT or TEX_FORMAT_RGB9E5_SHAREDEXP)
    COMPONENT_TYPE_COMPOUND,

    /// Compressed component type
    COMPONENT_TYPE_COMPRESSED,
};

/// Describes invariant texture format attributes. These attributes are 
/// intrinsic to the texture format itself and do not depend on the 
/// format support.
struct TextureFormatAttribs
{
    /// Literal texture format name (for instance, for TEX_FORMAT_RGBA8_UNORM format, this
    /// will be "TEX_FORMAT_RGBA8_UNORM")
    const Char* Name             DEFAULT_INITIALIZER("TEX_FORMAT_UNKNOWN");

    /// Texture format, see Diligent::TEXTURE_FORMAT for a list of supported texture formats
    TEXTURE_FORMAT Format        DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);

    /// Size of one component in bytes (for instance, for TEX_FORMAT_RGBA8_UNORM format, this will be 1)
    /// For compressed formats, this is the block size in bytes (for TEX_FORMAT_BC1_UNORM format, this will be 8)
    Uint8 ComponentSize          DEFAULT_INITIALIZER(0);

    /// Number of components
    Uint8 NumComponents          DEFAULT_INITIALIZER(0);

    /// Component type, see Diligent::COMPONENT_TYPE for details.
    COMPONENT_TYPE ComponentType DEFAULT_INITIALIZER(COMPONENT_TYPE_UNDEFINED);

    /// Bool flag indicating if the format is a typeless format
    bool IsTypeless              DEFAULT_INITIALIZER(false);

    /// For block-compressed formats, compression block width
    Uint8 BlockWidth             DEFAULT_INITIALIZER(0);

    /// For block-compressed formats, compression block height
    Uint8 BlockHeight            DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    /// For non-compressed formats, returns the texel size.
    /// For block-compressed formats, returns the block size.
    Uint32 GetElementSize() const
    {
        return Uint32{ComponentSize} * (ComponentType != COMPONENT_TYPE_COMPRESSED ? Uint32{NumComponents} : Uint32{1});
    }

    /// Initializes the structure
    TextureFormatAttribs( const Char*    _Name,
                            TEXTURE_FORMAT _Format, 
                            Uint8          _ComponentSize,
                            Uint8          _NumComponents,
                            COMPONENT_TYPE _ComponentType,
                            bool           _IsTypeless,
                            Uint8          _BlockWidth,
                            Uint8          _BlockHeight)noexcept : 
        Name         {_Name         },
        Format       {_Format       },
        ComponentSize{_ComponentSize},
        NumComponents{_NumComponents},
        ComponentType{_ComponentType},
        IsTypeless   {_IsTypeless   },
        BlockWidth   {_BlockWidth   },
        BlockHeight  {_BlockHeight  }
    {
    }

    TextureFormatAttribs()noexcept {}
#endif
};
typedef struct TextureFormatAttribs TextureFormatAttribs;


/// Basic texture format description

/// This structure is returned by IRenderDevice::GetTextureFormatInfo()
struct TextureFormatInfo DILIGENT_DERIVE(TextureFormatAttribs)

    /// Indicates if the format is supported by the device
    bool Supported  DEFAULT_INITIALIZER(false);

    // Explicitly pad the structure to 8-byte boundary
    bool Padding[7];
};
typedef struct TextureFormatInfo TextureFormatInfo;


/// Describes device support of a particular resource dimension for a given texture format.
DILIGENT_TYPED_ENUM(RESOURCE_DIMENSION_SUPPORT, Uint32)
{
    /// The device does not support any resources for this format.
    RESOURCE_DIMENSION_SUPPORT_NONE           = 0,

    /// Indicates if the device supports buffer resources for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_BUFFER         = 1 << RESOURCE_DIM_BUFFER,

    /// Indicates if the device supports 1D textures for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_TEX_1D         = 1 << RESOURCE_DIM_TEX_1D,

    /// Indicates if the device supports 1D texture arrays for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_TEX_1D_ARRAY   = 1 << RESOURCE_DIM_TEX_1D_ARRAY,

    /// Indicates if the device supports 2D textures for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_TEX_2D         = 1 << RESOURCE_DIM_TEX_2D,

    /// Indicates if the device supports 2D texture arrays for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY   = 1 << RESOURCE_DIM_TEX_2D_ARRAY,

    /// Indicates if the device supports 3D textures for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_TEX_3D         = 1 << RESOURCE_DIM_TEX_3D,

    /// Indicates if the device supports cube textures for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_TEX_CUBE       = 1 << RESOURCE_DIM_TEX_CUBE,

    /// Indicates if the device supports cube texture arrays for a particular texture format.
    RESOURCE_DIMENSION_SUPPORT_TEX_CUBE_ARRAY = 1 << RESOURCE_DIM_TEX_CUBE_ARRAY
};
DEFINE_FLAG_ENUM_OPERATORS(RESOURCE_DIMENSION_SUPPORT);


/// Extended texture format information.

/// This structure is returned by IRenderDevice::GetTextureFormatInfoExt()
struct TextureFormatInfoExt DILIGENT_DERIVE(TextureFormatInfo)

    /// Allowed bind flags for this format.
    BIND_FLAGS BindFlags    DEFAULT_INITIALIZER(BIND_NONE);

    /// A bitmask specifying all the supported resource dimensions for this texture format,
    /// see Diligent::RESOURCE_DIMENSION_SUPPORT.

    /// For every supported resource dimension in RESOURCE_DIMENSION enum,
    /// the corresponding bit in the mask will be set to 1.
    /// For example, support for Texture2D resource dimension can be checked as follows:
    ///
    ///     (Dimensions & RESOURCE_DIMENSION_SUPPORT_TEX_2D) != 0
    ///
    RESOURCE_DIMENSION_SUPPORT Dimensions DEFAULT_INITIALIZER(RESOURCE_DIMENSION_SUPPORT_NONE);

    /// A bitmask specifying all the supported sample counts for this texture format.
    /// If the format supports n samples, then (SampleCounts & n) != 0
    Uint32     SampleCounts DEFAULT_INITIALIZER(0);

    /// Indicates if the format can be filtered in the shader.
    Bool       Filterable   DEFAULT_INITIALIZER(false);
};
typedef struct TextureFormatInfoExt TextureFormatInfoExt;


/// Pipeline stage flags.

/// These flags mirror [VkPipelineStageFlagBits](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkPipelineStageFlagBits)
/// enum and only have effect in Vulkan backend.
DILIGENT_TYPED_ENUM(PIPELINE_STAGE_FLAGS, Uint32)
{
    /// Undefined stage
    PIPELINE_STAGE_FLAG_UNDEFINED                    = 0x00000000,

    /// The top of the pipeline.
    PIPELINE_STAGE_FLAG_TOP_OF_PIPE                  = 0x00000001,

    /// The stage of the pipeline where Draw/DispatchIndirect data structures are consumed.
    PIPELINE_STAGE_FLAG_DRAW_INDIRECT                = 0x00000002,

    /// The stage of the pipeline where vertex and index buffers are consumed.
    PIPELINE_STAGE_FLAG_VERTEX_INPUT                 = 0x00000004,

    /// Vertex shader stage.
    PIPELINE_STAGE_FLAG_VERTEX_SHADER                = 0x00000008,
    
    /// Hull shader stage.
    PIPELINE_STAGE_FLAG_HULL_SHADER                  = 0x00000010,
    
    /// Domain shader stage.
    PIPELINE_STAGE_FLAG_DOMAIN_SHADER                = 0x00000020,
    
    /// Geometry shader stage.
    PIPELINE_STAGE_FLAG_GEOMETRY_SHADER              = 0x00000040,
    
    /// Pixel shader stage.
    PIPELINE_STAGE_FLAG_PIXEL_SHADER                 = 0x00000080,
    
    /// The stage of the pipeline where early fragment tests (depth and
    /// stencil tests before fragment shading) are performed. This stage
    /// also includes subpass load operations for framebuffer attachments
    /// with a depth/stencil format.
    PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS         = 0x00000100,
    
    /// The stage of the pipeline where late fragment tests (depth and 
    /// stencil tests after fragment shading) are performed. This stage
    /// also includes subpass store operations for framebuffer attachments
    /// with a depth/stencil format.
    PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS          = 0x00000200,
    
    /// The stage of the pipeline after blending where the final color values
    /// are output from the pipeline. This stage also includes subpass load
    /// and store operations and multisample resolve operations for framebuffer
    /// attachments with a color or depth/stencil format.
    PIPELINE_STAGE_FLAG_RENDER_TARGET                = 0x00000400,
    
    /// Compute shader stage.
    PIPELINE_STAGE_FLAG_COMPUTE_SHADER               = 0x00000800,
    
    /// The stage where all copy and outside-of-renderpass
    /// resolve and clear operations happen.
    PIPELINE_STAGE_FLAG_TRANSFER                     = 0x00001000,
    
    /// The bottom of the pipeline.
    PIPELINE_STAGE_FLAG_BOTTOM_OF_PIPE               = 0x00002000,
    
    /// A pseudo-stage indicating execution on the host of reads/writes
    /// of device memory. This stage is not invoked by any commands recorded
    /// in a command buffer.
    PIPELINE_STAGE_FLAG_HOST                         = 0x00004000,
    
    /// The stage of the pipeline where the predicate of conditional rendering is consumed.
    PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING        = 0x00040000,

    /// The stage of the pipeline where the shading rate texture is
    /// read to determine the shading rate for portions of a rasterized primitive.
    PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE         = 0x00400000,
    
    /// Ray tracing shader.
    PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER           = 0x00200000,
    
    /// Acceleration structure build shader.
    PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD = 0x02000000,
    
    /// Task shader stage.
    PIPELINE_STAGE_FLAG_TASK_SHADER                  = 0x00080000,
    
    /// Mesh shader stage.
    PIPELINE_STAGE_FLAG_MESH_SHADER                  = 0x00100000,
    
    /// 
    PIPELINE_STAGE_FLAG_FRAGMENT_DENSITY_PROCESS     = 0x00800000,
    
    /// Default pipeline stage that is determined by the resource state.
    /// For example, RESOURCE_STATE_RENDER_TARGET corresponds to
    /// PIPELINE_STAGE_FLAG_RENDER_TARGET pipeline stage.
    PIPELINE_STAGE_FLAG_DEFAULT                      = 0x80000000
};
DEFINE_FLAG_ENUM_OPERATORS(PIPELINE_STAGE_FLAGS)


/// Access flag.

/// The flags mirror [VkAccessFlags](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkAccessFlags) enum
/// and only have effect in Vulkan backend.
DILIGENT_TYPED_ENUM(ACCESS_FLAGS, Uint32)
{
    /// No access
    ACCESS_FLAG_NONE                         = 0x00000000,

    /// Read access to indirect command data read as part of an indirect
    /// drawing or dispatch command.
    ACCESS_FLAG_INDIRECT_COMMAND_READ        = 0x00000001,

    /// Read access to an index buffer as part of an indexed drawing command
    ACCESS_FLAG_INDEX_READ                   = 0x00000002,

    /// Read access to a vertex buffer as part of a drawing command
    ACCESS_FLAG_VERTEX_READ                  = 0x00000004,

    /// Read access to a uniform buffer
    ACCESS_FLAG_UNIFORM_READ                 = 0x00000008,

    /// Read access to an input attachment within a render pass during fragment shading
    ACCESS_FLAG_INPUT_ATTACHMENT_READ        = 0x00000010,

    /// Read access from a shader resource, formatted buffer, UAV
    ACCESS_FLAG_SHADER_READ                  = 0x00000020,

    /// Write access to a UAV
    ACCESS_FLAG_SHADER_WRITE                 = 0x00000040,

    /// Read access to a color render target, such as via blending,
    /// logic operations, or via certain subpass load operations.
    ACCESS_FLAG_RENDER_TARGET_READ           = 0x00000080,
    
    /// Write access to a color render target, resolve, or depth/stencil resolve
    /// attachment during a render pass or via certain subpass load and store operations.
    ACCESS_FLAG_RENDER_TARGET_WRITE          = 0x00000100,

    /// Read access to a depth/stencil buffer, via depth or stencil operations
    /// or via certain subpass load operations
    ACCESS_FLAG_DEPTH_STENCIL_READ           = 0x00000200,

    /// Write access to a depth/stencil buffer, via depth or stencil operations
    /// or via certain subpass load and store operations
    ACCESS_FLAG_DEPTH_STENCIL_WRITE          = 0x00000400,

    /// Read access to an texture or buffer in a copy operation.
    ACCESS_FLAG_COPY_SRC                     = 0x00000800,

    /// Write access to an texture or buffer in a copy operation.
    ACCESS_FLAG_COPY_DST                     = 0x00001000,

    /// Read access by a host operation. Accesses of this type are
    /// not performed through a resource, but directly on memory.
    ACCESS_FLAG_HOST_READ                    = 0x00002000,

    /// Write access by a host operation. Accesses of this type are
    /// not performed through a resource, but directly on memory.
    ACCESS_FLAG_HOST_WRITE                   = 0x00004000,

    /// All read accesses. It is always valid in any access mask,
    /// and is treated as equivalent to setting all READ access flags
    /// that are valid where it is used.
    ACCESS_FLAG_MEMORY_READ                  = 0x00008000,

    /// All write accesses. It is always valid in any access mask,
    /// and is treated as equivalent to setting all WRITE access
    // flags that are valid where it is used.
    ACCESS_FLAG_MEMORY_WRITE                 = 0x00010000,

    /// Read access to a predicate as part of conditional rendering.
    ACCESS_FLAG_CONDITIONAL_RENDERING_READ   = 0x00100000,

    /// Read access to a shading rate texture as part of a drawing comman.
    ACCESS_FLAG_SHADING_RATE_TEXTURE_READ    = 0x00800000,

    /// Read access to an acceleration structure as part of a trace or build command.
    ACCESS_FLAG_ACCELERATION_STRUCTURE_READ  = 0x00200000,

    /// Write access to an acceleration structure or acceleration structure
    /// scratch buffer as part of a build command.
    ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE = 0x00400000,

    /// Read access to a fragment density map attachment during
    /// dynamic fragment density map operations.
    ACCESS_FLAG_FRAGMENT_DENSITY_MAP_READ    = 0x01000000,

    /// Default access type that is determined by the resource state.
    /// For example, RESOURCE_STATE_RENDER_TARGET corresponds to
    /// ACCESS_FLAG_RENDER_TARGET_WRITE access type.
    ACCESS_FLAG_DEFAULT                      = 0x80000000
};
DEFINE_FLAG_ENUM_OPERATORS(ACCESS_FLAGS)


/// Resource usage state
DILIGENT_TYPED_ENUM(RESOURCE_STATE, Uint32)
{
    /// The resource state is not known to the engine and is managed by the application
    RESOURCE_STATE_UNKNOWN              = 0x00000,

    /// The resource state is known to the engine, but is undefined. A resource is typically in an undefined state right after initialization.
    RESOURCE_STATE_UNDEFINED            = 0x00001,

    /// The resource is accessed as vertex buffer
    RESOURCE_STATE_VERTEX_BUFFER        = 0x00002,

    /// The resource is accessed as constant (uniform) buffer
    RESOURCE_STATE_CONSTANT_BUFFER      = 0x00004,

    /// The resource is accessed as index buffer
    RESOURCE_STATE_INDEX_BUFFER         = 0x00008,

    /// The resource is accessed as render target
    RESOURCE_STATE_RENDER_TARGET        = 0x00010,
        
    /// The resource is used for unordered access
    RESOURCE_STATE_UNORDERED_ACCESS     = 0x00020,

    /// The resource is used in a writable depth-stencil view or in clear operation
    RESOURCE_STATE_DEPTH_WRITE          = 0x00040,

    /// The resource is used in a read-only depth-stencil view
    RESOURCE_STATE_DEPTH_READ           = 0x00080,

    /// The resource is accessed from a shader
    RESOURCE_STATE_SHADER_RESOURCE      = 0x00100,
        
    /// The resource is used as the destination for stream output
    RESOURCE_STATE_STREAM_OUT           = 0x00200,

    /// The resource is used as indirect draw/dispatch arguments buffer
    RESOURCE_STATE_INDIRECT_ARGUMENT    = 0x00400,

    /// The resource is used as the destination in a copy operation
    RESOURCE_STATE_COPY_DEST            = 0x00800,

    /// The resource is used as the source in a copy operation 
    RESOURCE_STATE_COPY_SOURCE          = 0x01000,
        
    /// The resource is used as the destination in a resolve operation 
    RESOURCE_STATE_RESOLVE_DEST         = 0x02000,
        
    /// The resource is used as the source in a resolve operation 
    RESOURCE_STATE_RESOLVE_SOURCE       = 0x04000,

    /// The resource is used as input attachment in a render pass subpass
    RESOURCE_STATE_INPUT_ATTACHMENT     = 0x08000,

    /// The resource is used for present
    RESOURCE_STATE_PRESENT              = 0x10000,

    RESOURCE_STATE_MAX_BIT              = 0x10000,

    RESOURCE_STATE_GENERIC_READ         = RESOURCE_STATE_VERTEX_BUFFER     |
                                          RESOURCE_STATE_CONSTANT_BUFFER   |
                                          RESOURCE_STATE_INDEX_BUFFER      |
                                          RESOURCE_STATE_SHADER_RESOURCE   |
                                          RESOURCE_STATE_INDIRECT_ARGUMENT |
                                          RESOURCE_STATE_COPY_SOURCE
};
DEFINE_FLAG_ENUM_OPERATORS(RESOURCE_STATE);

/// State transition barrier type
DILIGENT_TYPED_ENUM(STATE_TRANSITION_TYPE, Uint8)
{
    /// Perform state transition immediately.
    STATE_TRANSITION_TYPE_IMMEDIATE = 0,

    /// Begin split barrier. This mode only has effect in Direct3D12 backend, and corresponds to
    /// [D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY](https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_resource_barrier_flags)
    /// flag. See https://docs.microsoft.com/en-us/windows/desktop/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#split-barriers.
    /// In other backends, begin-split barriers are ignored.
    STATE_TRANSITION_TYPE_BEGIN,

    /// End split barrier. This mode only has effect in Direct3D12 backend, and corresponds to
    /// [D3D12_RESOURCE_BARRIER_FLAG_END_ONLY](https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_resource_barrier_flags)
    /// flag. See https://docs.microsoft.com/en-us/windows/desktop/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#split-barriers.
    /// In other backends, this mode is similar to STATE_TRANSITION_TYPE_IMMEDIATE.
    STATE_TRANSITION_TYPE_END
};

static const Uint32 REMAINING_MIP_LEVELS   = 0xFFFFFFFFU;
static const Uint32 REMAINING_ARRAY_SLICES = 0xFFFFFFFFU;

/// Resource state transition barrier description
struct StateTransitionDesc
{
    /// Texture to transition.
    /// \note Exactly one of pTexture or pBuffer must be non-null.
    struct ITexture* pTexture DEFAULT_INITIALIZER(nullptr);
        
    /// Buffer to transition.
    /// \note Exactly one of pTexture or pBuffer must be non-null.
    struct IBuffer* pBuffer   DEFAULT_INITIALIZER(nullptr);
        
    /// When transitioning a texture, first mip level of the subresource range to transition.
    Uint32 FirstMipLevel     DEFAULT_INITIALIZER(0);

    /// When transitioning a texture, number of mip levels of the subresource range to transition.
    Uint32 MipLevelsCount    DEFAULT_INITIALIZER(REMAINING_MIP_LEVELS);

    /// When transitioning a texture, first array slice of the subresource range to transition.
    Uint32 FirstArraySlice   DEFAULT_INITIALIZER(0);

    /// When transitioning a texture, number of array slices of the subresource range to transition.
    Uint32 ArraySliceCount   DEFAULT_INITIALIZER(REMAINING_ARRAY_SLICES);

    /// Resource state before transition. If this value is RESOURCE_STATE_UNKNOWN,
    /// internal resource state will be used, which must be defined in this case.
    RESOURCE_STATE OldState  DEFAULT_INITIALIZER(RESOURCE_STATE_UNKNOWN);

    /// Resource state after transition.
    RESOURCE_STATE NewState  DEFAULT_INITIALIZER(RESOURCE_STATE_UNKNOWN);

    /// State transition type, see Diligent::STATE_TRANSITION_TYPE.

    /// \note When issuing UAV barrier (i.e. OldState and NewState equal RESOURCE_STATE_UNORDERED_ACCESS),
    ///       TransitionType must be STATE_TRANSITION_TYPE_IMMEDIATE.
    STATE_TRANSITION_TYPE TransitionType DEFAULT_INITIALIZER(STATE_TRANSITION_TYPE_IMMEDIATE);

    /// If set to true, the internal resource state will be set to NewState and the engine
    /// will be able to take over the resource state management. In this case it is the 
    /// responsibility of the application to make sure that all subresources are indeed in
    /// designated state.
    /// If set to false, internal resource state will be unchanged.
    /// \note When TransitionType is STATE_TRANSITION_TYPE_BEGIN, this member must be false.
    bool UpdateResourceState  DEFAULT_INITIALIZER(false);

#if DILIGENT_CPP_INTERFACE
    StateTransitionDesc()noexcept{}

    StateTransitionDesc(ITexture*             _pTexture, 
                        RESOURCE_STATE        _OldState,
                        RESOURCE_STATE        _NewState, 
                        Uint32                _FirstMipLevel   = 0,
                        Uint32                _MipLevelsCount  = REMAINING_MIP_LEVELS,
                        Uint32                _FirstArraySlice = 0,
                        Uint32                _ArraySliceCount = REMAINING_ARRAY_SLICES,
                        STATE_TRANSITION_TYPE _TransitionType  = STATE_TRANSITION_TYPE_IMMEDIATE,
                        bool                  _UpdateState     = false)noexcept : 
        pTexture            {_pTexture       },
        FirstMipLevel       {_FirstMipLevel  },
        MipLevelsCount      {_MipLevelsCount },
        FirstArraySlice     {_FirstArraySlice},
        ArraySliceCount     {_ArraySliceCount},
        OldState            {_OldState       },
        NewState            {_NewState       },
        TransitionType      {_TransitionType },
        UpdateResourceState {_UpdateState    }
    {}

    StateTransitionDesc(ITexture*      _pTexture, 
                        RESOURCE_STATE _OldState,
                        RESOURCE_STATE _NewState, 
                        bool           _UpdateState)noexcept :
        StateTransitionDesc
        {
            _pTexture,
            _OldState,
            _NewState,
            0,
            REMAINING_MIP_LEVELS,
            0,
            REMAINING_ARRAY_SLICES,
            STATE_TRANSITION_TYPE_IMMEDIATE,
            _UpdateState
        }
    {}

    StateTransitionDesc(IBuffer*       _pBuffer, 
                        RESOURCE_STATE _OldState,
                        RESOURCE_STATE _NewState,
                        bool           _UpdateState)noexcept : 
        pBuffer             {_pBuffer    },
        OldState            {_OldState   },
        NewState            {_NewState   },
        UpdateResourceState {_UpdateState}
    {}
#endif
};
typedef struct StateTransitionDesc StateTransitionDesc;

DILIGENT_END_NAMESPACE // namespace Diligent
