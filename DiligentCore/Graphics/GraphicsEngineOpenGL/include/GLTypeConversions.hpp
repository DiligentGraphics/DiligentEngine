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

namespace Diligent
{

inline GLenum PrimitiveTopologyToGLTopology(PRIMITIVE_TOPOLOGY PrimTopology)
{
    // clang-format off
    static constexpr GLenum PrimTopologyToGLTopologyMap[] =
    {
        0,                 // PRIMITIVE_TOPOLOGY_UNDEFINED = 0
        GL_TRIANGLES,      // PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        GL_TRIANGLE_STRIP, // PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        GL_POINTS,         // PRIMITIVE_TOPOLOGY_POINT_LIST
        GL_LINES,          // PRIMITIVE_TOPOLOGY_LINE_LIST
        GL_LINE_STRIP      // PRIMITIVE_TOPOLOGY_LINE_STRIP
    };
    // clang-format on

    VERIFY_EXPR(PrimTopology < _countof(PrimTopologyToGLTopologyMap));
    return PrimTopologyToGLTopologyMap[PrimTopology];
}

inline GLenum TypeToGLType(VALUE_TYPE Value)
{
    // clang-format off
    static constexpr GLenum TypeToGLTypeMap[] = 
    {
        0,                  // VT_UNDEFINED = 0
        GL_BYTE,            // VT_INT8
        GL_SHORT,           // VT_INT16
        GL_INT,             // VT_INT32
        GL_UNSIGNED_BYTE,   // VT_UINT8
        GL_UNSIGNED_SHORT,  // VT_UINT16
        GL_UNSIGNED_INT,    // VT_UINT32
        GL_HALF_FLOAT,      // VT_FLOAT16
        GL_FLOAT            // VT_FLOAT32
    };
    // clang-format on

    VERIFY_EXPR(Value < _countof(TypeToGLTypeMap));
    return TypeToGLTypeMap[Value];
}

inline GLenum UsageToGLUsage(const BufferDesc& Desc)
{
    static_assert(USAGE_NUM_USAGES == 5, "Please update this function to handle the new usage type");

    // http://www.informit.com/articles/article.aspx?p=2033340&seqNum=2
    // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glBufferData.xml
    switch (Desc.Usage)
    {
        // STATIC: The data store contents will be modified once and used many times.
        // STREAM: The data store contents will be modified once and used at MOST a few times.
        // DYNAMIC: The data store contents will be modified repeatedly and used many times.

        // clang-format off
        case USAGE_IMMUTABLE:   return GL_STATIC_DRAW;
        case USAGE_DEFAULT:     return GL_STATIC_DRAW;
        case USAGE_UNIFIED:     return GL_STATIC_DRAW;
        case USAGE_DYNAMIC:     return GL_DYNAMIC_DRAW;
        case USAGE_STAGING:
            if(Desc.CPUAccessFlags & CPU_ACCESS_READ)
                return GL_STATIC_READ;
            else
                return GL_STATIC_COPY;

        default: UNEXPECTED( "Unknow usage" ); return 0;
            // clang-format on
    }
}

inline void FilterTypeToGLFilterType(FILTER_TYPE Filter, GLenum& GLFilter, Bool& bIsAnisotropic, Bool& bIsComparison)
{
    switch (Filter)
    {
        case FILTER_TYPE_UNKNOWN:
            UNEXPECTED("Unspecified filter type");
            bIsAnisotropic = false;
            bIsComparison  = false;
            GLFilter       = GL_NEAREST;
            break;

        case FILTER_TYPE_POINT:
            bIsAnisotropic = false;
            bIsComparison  = false;
            GLFilter       = GL_NEAREST;
            break;

        case FILTER_TYPE_LINEAR:
            bIsAnisotropic = false;
            bIsComparison  = false;
            GLFilter       = GL_LINEAR;
            break;

        case FILTER_TYPE_ANISOTROPIC:
            bIsAnisotropic = true;
            bIsComparison  = false;
            GLFilter       = GL_LINEAR;
            break;

        case FILTER_TYPE_COMPARISON_POINT:
            bIsAnisotropic = false;
            bIsComparison  = true;
            GLFilter       = GL_NEAREST;
            break;

        case FILTER_TYPE_COMPARISON_LINEAR:
            bIsAnisotropic = false;
            bIsComparison  = true;
            GLFilter       = GL_LINEAR;
            break;

        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
            bIsAnisotropic = true;
            bIsComparison  = true;
            GLFilter       = GL_LINEAR;
            break;

        default:
            UNEXPECTED("Unknown filter type");
            bIsAnisotropic = false;
            bIsComparison  = false;
            GLFilter       = GL_NEAREST;
            break;
    }
}

inline GLenum CorrectGLTexFormat(GLenum GLTexFormat, Uint32 BindFlags)
{
    if (BindFlags & BIND_DEPTH_STENCIL)
    {
        if (GLTexFormat == GL_R32F)
            GLTexFormat = GL_DEPTH_COMPONENT32F;
        if (GLTexFormat == GL_R16)
            GLTexFormat = GL_DEPTH_COMPONENT16;
    }
    return GLTexFormat;
}


inline GLenum TexAddressModeToGLAddressMode(TEXTURE_ADDRESS_MODE Mode)
{
    // clang-format off
    static constexpr GLenum TexAddressModeToGLAddressModeMap[] = 
    {
        0,                       // TEXTURE_ADDRESS_UNKNOWN = 0
        GL_REPEAT,               // TEXTURE_ADDRESS_WRAP
        GL_MIRRORED_REPEAT,      // TEXTURE_ADDRESS_MIRROR
        GL_CLAMP_TO_EDGE,        // TEXTURE_ADDRESS_CLAMP
        GL_CLAMP_TO_BORDER,      // TEXTURE_ADDRESS_BORDER

        // Only available in OpenGL 4.4+
        // This mode seems to be different from D3D11_TEXTURE_ADDRESS_MIRROR_ONCE
        // The texture coord is clamped to the [-1, 1] range, but mirrors the 
        // negative direction with the positive. Basically, it acts as
        // GL_CLAMP_TO_EDGE except that it takes the absolute value of the texture 
        // coordinates before clamping.
        GL_MIRROR_CLAMP_TO_EDGE  // TEXTURE_ADDRESS_MIRROR_ONCE
    };
    // clang-format on

    VERIFY_EXPR(Mode < _countof(TexAddressModeToGLAddressModeMap));
    return TexAddressModeToGLAddressModeMap[Mode];
}

inline GLenum CompareFuncToGLCompareFunc(COMPARISON_FUNCTION Func)
{
    // clang-format off
    static constexpr GLenum CompareFuncToGLCompareFuncMap[] = 
    {
        0,              // COMPARISON_FUNC_UNKNOWN = 0
        GL_NEVER,       // COMPARISON_FUNC_NEVER
        GL_LESS,        // COMPARISON_FUNC_LESS
        GL_EQUAL,       // COMPARISON_FUNC_EQUAL
        GL_LEQUAL,      // COMPARISON_FUNC_LESS_EQUAL
        GL_GREATER,     // COMPARISON_FUNC_GREATER
        GL_NOTEQUAL,    // COMPARISON_FUNC_NOT_EQUAL
        GL_GEQUAL,      // COMPARISON_FUNC_GREATER_EQUAL
        GL_ALWAYS       // COMPARISON_FUNC_ALWAYS
    };
    // clang-format on

    VERIFY_EXPR(Func < _countof(CompareFuncToGLCompareFuncMap));
    return CompareFuncToGLCompareFuncMap[Func];
}

struct NativePixelAttribs
{
    GLenum PixelFormat  = 0;
    GLenum DataType     = 0;
    Bool   IsCompressed = 0;

    NativePixelAttribs() noexcept {}

    explicit NativePixelAttribs(GLenum _PixelFormat, GLenum _DataType, Bool _IsCompressed = False) noexcept :
        PixelFormat{_PixelFormat},
        DataType{_DataType},
        IsCompressed{_IsCompressed}
    {}
};

inline Uint32 GetNumPixelFormatComponents(GLenum Format)
{
    switch (Format)
    {
        case GL_RGBA:
        case GL_RGBA_INTEGER:
            return 4;

        case GL_RGB:
        case GL_RGB_INTEGER:
            return 3;

        case GL_RG:
        case GL_RG_INTEGER:
            return 2;

        case GL_RED:
        case GL_RED_INTEGER:
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
            return 1;

        default: UNEXPECTED("Unknonw pixel format"); return 0;
    };
}

inline Uint32 GetPixelTypeSize(GLenum Type)
{
    switch (Type)
    {
        // clang-format off
        case GL_FLOAT:          return sizeof(GLfloat);

        case GL_UNSIGNED_INT_10_10_10_2: 
        case GL_UNSIGNED_INT_2_10_10_10_REV:
        case GL_UNSIGNED_INT_10F_11F_11F_REV:
        case GL_UNSIGNED_INT_24_8:
        case GL_UNSIGNED_INT_5_9_9_9_REV:
        case GL_UNSIGNED_INT:   return sizeof(GLuint);
        
        case GL_INT:            return sizeof(GLint);
        case GL_HALF_FLOAT:     return sizeof(GLhalf);

        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_SHORT_5_6_5_REV:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_SHORT_1_5_5_5_REV:
        case GL_UNSIGNED_SHORT: return sizeof(GLushort);

        case GL_SHORT:          return sizeof(GLshort);
        case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
        case GL_BYTE:           return sizeof(GLbyte);
        
        case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:return sizeof(GLfloat) + sizeof(GLuint);

        default: UNEXPECTED( "Unknonw pixel type" ); return 0;
            // clang-format on
    }
}

inline GLenum AccessFlags2GLAccess(UAV_ACCESS_FLAG UAVAccessFlags)
{
    // clang-format off
    static constexpr GLenum AccessFlags2GLAccessMap[] = 
    {
        0,             // UAV_ACCESS_UNSPECIFIED == 0
        GL_READ_ONLY,  // UAV_ACCESS_FLAG_READ
        GL_WRITE_ONLY, // UAV_ACCESS_FLAG_WRITE
        GL_READ_WRITE  // UAV_ACCESS_FLAG_READ_WRITE
    };
    // clang-format on

    VERIFY_EXPR(UAVAccessFlags < _countof(AccessFlags2GLAccessMap));
    return AccessFlags2GLAccessMap[UAVAccessFlags];
}

inline GLenum StencilOp2GlStencilOp(STENCIL_OP StencilOp)
{
    // clang-format off
    static constexpr GLenum StencilOp2GlStencilOpMap[] = 
    {
        0,             // STENCIL_OP_UNDEFINED == 0
        GL_KEEP,       // STENCIL_OP_KEEP
        GL_ZERO,       // STENCIL_OP_ZERO
        GL_REPLACE,    // STENCIL_OP_REPLACE
        GL_INCR,       // STENCIL_OP_INCR_SAT
        GL_DECR,       // STENCIL_OP_DECR_SAT
        GL_INVERT,     // STENCIL_OP_INVERT
        GL_INCR_WRAP,  // STENCIL_OP_INCR_WRAP
        GL_DECR_WRAP,  // STENCIL_OP_DECR_WRAP
    };
    // clang-format on

    VERIFY_EXPR(StencilOp < _countof(StencilOp2GlStencilOpMap));
    return StencilOp2GlStencilOpMap[StencilOp];
}

inline GLenum BlendFactor2GLBlend(BLEND_FACTOR bf)
{
    // clang-format off
    static constexpr GLenum BlendFactor2GLBlendMap[] = 
    {
        0,                           // BLEND_FACTOR_UNDEFINED == 0
        GL_ZERO,                     // BLEND_FACTOR_ZERO
        GL_ONE,                      // BLEND_FACTOR_ONE
        GL_SRC_COLOR,                // BLEND_FACTOR_SRC_COLOR
        GL_ONE_MINUS_SRC_COLOR,      // BLEND_FACTOR_INV_SRC_COLOR
        GL_SRC_ALPHA,                // BLEND_FACTOR_SRC_ALPHA
        GL_ONE_MINUS_SRC_ALPHA,      // BLEND_FACTOR_INV_SRC_ALPHA
        GL_DST_ALPHA,                // BLEND_FACTOR_DEST_ALPHA
        GL_ONE_MINUS_DST_ALPHA,      // BLEND_FACTOR_INV_DEST_ALPHA
        GL_DST_COLOR,                // BLEND_FACTOR_DEST_COLOR
        GL_ONE_MINUS_DST_COLOR,      // BLEND_FACTOR_INV_DEST_COLOR
        GL_SRC_ALPHA_SATURATE,       // BLEND_FACTOR_SRC_ALPHA_SAT
        GL_CONSTANT_COLOR,           // BLEND_FACTOR_BLEND_FACTOR
        GL_ONE_MINUS_CONSTANT_COLOR, // BLEND_FACTOR_INV_BLEND_FACTOR
        GL_SRC1_COLOR,               // BLEND_FACTOR_SRC1_COLOR
        GL_ONE_MINUS_SRC1_COLOR,     // BLEND_FACTOR_INV_SRC1_COLOR
        GL_SRC1_ALPHA,               // BLEND_FACTOR_SRC1_ALPHA
        GL_ONE_MINUS_SRC1_ALPHA      // BLEND_FACTOR_INV_SRC1_ALPHA
    };
    // clang-format on

    VERIFY_EXPR(bf < _countof(BlendFactor2GLBlendMap));
    return BlendFactor2GLBlendMap[bf];
}

inline GLenum BlendOperation2GLBlendOp(BLEND_OPERATION BlendOp)
{
    // clang-format off
    static constexpr GLenum BlendOperation2GLBlendOpMap[] = 
    {
        0,                        // BLEND_OPERATION_UNDEFINED
        GL_FUNC_ADD,              // BLEND_OPERATION_ADD
        GL_FUNC_SUBTRACT,         // BLEND_OPERATION_SUBTRACT
        GL_FUNC_REVERSE_SUBTRACT, // BLEND_OPERATION_REV_SUBTRACT
        GL_MIN,                   // BLEND_OPERATION_MIN
        GL_MAX                    // BLEND_OPERATION_MAX
    };
    // clang-format on

    VERIFY_EXPR(BlendOp < _countof(BlendOperation2GLBlendOpMap));
    return BlendOperation2GLBlendOpMap[BlendOp];
}


GLenum         TexFormatToGLInternalTexFormat(TEXTURE_FORMAT TexFormat, Uint32 BindFlags = 0);
TEXTURE_FORMAT GLInternalTexFormatToTexFormat(GLenum GlFormat);

NativePixelAttribs GetNativePixelTransferAttribs(TEXTURE_FORMAT TexFormat);
GLenum             TypeToGLTexFormat(VALUE_TYPE ValType, Uint32 NumComponents, Bool bIsNormalized);

} // namespace Diligent
