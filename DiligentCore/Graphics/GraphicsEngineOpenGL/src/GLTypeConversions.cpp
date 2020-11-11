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

#include "pch.h"
#include <array>

#include "GLTypeConversions.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

namespace
{

class FormatToGLInternalTexFormatMap
{
public:
    FormatToGLInternalTexFormatMap()
    {
        // clang-format off
        // http://www.opengl.org/wiki/Image_Format
        m_FmtToGLFmtMap[TEX_FORMAT_UNKNOWN]                = 0;

        m_FmtToGLFmtMap[TEX_FORMAT_RGBA32_TYPELESS]        = GL_RGBA32F;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA32_FLOAT]           = GL_RGBA32F;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA32_UINT]            = GL_RGBA32UI;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA32_SINT]            = GL_RGBA32I;

        m_FmtToGLFmtMap[TEX_FORMAT_RGB32_TYPELESS]         = GL_RGB32F;
        m_FmtToGLFmtMap[TEX_FORMAT_RGB32_FLOAT]            = GL_RGB32F;
        m_FmtToGLFmtMap[TEX_FORMAT_RGB32_UINT]             = GL_RGB32UI;
        m_FmtToGLFmtMap[TEX_FORMAT_RGB32_SINT]             = GL_RGB32I;

        m_FmtToGLFmtMap[TEX_FORMAT_RGBA16_TYPELESS]        = GL_RGBA16F;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA16_FLOAT]           = GL_RGBA16F;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA16_UNORM]           = GL_RGBA16;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA16_UINT]            = GL_RGBA16UI;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA16_SNORM]           = GL_RGBA16_SNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA16_SINT]            = GL_RGBA16I;

        m_FmtToGLFmtMap[TEX_FORMAT_RG32_TYPELESS]          = GL_RG32F;
        m_FmtToGLFmtMap[TEX_FORMAT_RG32_FLOAT]             = GL_RG32F;
        m_FmtToGLFmtMap[TEX_FORMAT_RG32_UINT]              = GL_RG32UI;
        m_FmtToGLFmtMap[TEX_FORMAT_RG32_SINT]              = GL_RG32I;

        m_FmtToGLFmtMap[TEX_FORMAT_R32G8X24_TYPELESS]      = GL_DEPTH32F_STENCIL8;
        m_FmtToGLFmtMap[TEX_FORMAT_D32_FLOAT_S8X24_UINT]   = GL_DEPTH32F_STENCIL8;
        m_FmtToGLFmtMap[TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS]=GL_DEPTH32F_STENCIL8;
        m_FmtToGLFmtMap[TEX_FORMAT_X32_TYPELESS_G8X24_UINT]= 0;//GL_DEPTH32F_STENCIL8;

        m_FmtToGLFmtMap[TEX_FORMAT_RGB10A2_TYPELESS]       = GL_RGB10_A2;
        m_FmtToGLFmtMap[TEX_FORMAT_RGB10A2_UNORM]          = GL_RGB10_A2;
        m_FmtToGLFmtMap[TEX_FORMAT_RGB10A2_UINT]           = GL_RGB10_A2UI;
        m_FmtToGLFmtMap[TEX_FORMAT_R11G11B10_FLOAT]        = GL_R11F_G11F_B10F;

        m_FmtToGLFmtMap[TEX_FORMAT_RGBA8_TYPELESS]         = GL_RGBA8;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA8_UNORM]            = GL_RGBA8;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA8_UNORM_SRGB]       = GL_SRGB8_ALPHA8;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA8_UINT]             = GL_RGBA8UI;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA8_SNORM]            = GL_RGBA8_SNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_RGBA8_SINT]             = GL_RGBA8I;

        m_FmtToGLFmtMap[TEX_FORMAT_RG16_TYPELESS]          = GL_RG16F;
        m_FmtToGLFmtMap[TEX_FORMAT_RG16_FLOAT]             = GL_RG16F;
        m_FmtToGLFmtMap[TEX_FORMAT_RG16_UNORM]             = GL_RG16;
        m_FmtToGLFmtMap[TEX_FORMAT_RG16_UINT]              = GL_RG16UI;
        m_FmtToGLFmtMap[TEX_FORMAT_RG16_SNORM]             = GL_RG16_SNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_RG16_SINT]              = GL_RG16I;

        m_FmtToGLFmtMap[TEX_FORMAT_R32_TYPELESS]           = GL_R32F;
        m_FmtToGLFmtMap[TEX_FORMAT_D32_FLOAT]              = GL_DEPTH_COMPONENT32F;
        m_FmtToGLFmtMap[TEX_FORMAT_R32_FLOAT]              = GL_R32F;
        m_FmtToGLFmtMap[TEX_FORMAT_R32_UINT]               = GL_R32UI;
        m_FmtToGLFmtMap[TEX_FORMAT_R32_SINT]               = GL_R32I;

        m_FmtToGLFmtMap[TEX_FORMAT_R24G8_TYPELESS]         = GL_DEPTH24_STENCIL8;
        m_FmtToGLFmtMap[TEX_FORMAT_D24_UNORM_S8_UINT]      = GL_DEPTH24_STENCIL8;
        m_FmtToGLFmtMap[TEX_FORMAT_R24_UNORM_X8_TYPELESS]  = GL_DEPTH24_STENCIL8;
        m_FmtToGLFmtMap[TEX_FORMAT_X24_TYPELESS_G8_UINT]   = 0;//GL_DEPTH24_STENCIL8;

        m_FmtToGLFmtMap[TEX_FORMAT_RG8_TYPELESS]           = GL_RG8;
        m_FmtToGLFmtMap[TEX_FORMAT_RG8_UNORM]              = GL_RG8;
        m_FmtToGLFmtMap[TEX_FORMAT_RG8_UINT]               = GL_RG8UI;
        m_FmtToGLFmtMap[TEX_FORMAT_RG8_SNORM]              = GL_RG8_SNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_RG8_SINT]               = GL_RG8I;

        m_FmtToGLFmtMap[TEX_FORMAT_R16_TYPELESS]           = GL_R16F;
        m_FmtToGLFmtMap[TEX_FORMAT_R16_FLOAT]              = GL_R16F;
        m_FmtToGLFmtMap[TEX_FORMAT_D16_UNORM]              = GL_DEPTH_COMPONENT16;
        m_FmtToGLFmtMap[TEX_FORMAT_R16_UNORM]              = GL_R16;
        m_FmtToGLFmtMap[TEX_FORMAT_R16_UINT]               = GL_R16UI;
        m_FmtToGLFmtMap[TEX_FORMAT_R16_SNORM]              = GL_R16_SNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_R16_SINT]               = GL_R16I;

        m_FmtToGLFmtMap[TEX_FORMAT_R8_TYPELESS]            = GL_R8;
        m_FmtToGLFmtMap[TEX_FORMAT_R8_UNORM]               = GL_R8;
        m_FmtToGLFmtMap[TEX_FORMAT_R8_UINT]                = GL_R8UI;
        m_FmtToGLFmtMap[TEX_FORMAT_R8_SNORM]               = GL_R8_SNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_R8_SINT]                = GL_R8I;
        m_FmtToGLFmtMap[TEX_FORMAT_A8_UNORM]               = 0;

        m_FmtToGLFmtMap[TEX_FORMAT_R1_UNORM]               = 0;

        m_FmtToGLFmtMap[TEX_FORMAT_RGB9E5_SHAREDEXP]       = GL_RGB9_E5;
        m_FmtToGLFmtMap[TEX_FORMAT_RG8_B8G8_UNORM]         = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_G8R8_G8B8_UNORM]        = 0;

        // http://www.g-truc.net/post-0335.html
        // http://renderingpipeline.com/2012/07/texture-compression/
        m_FmtToGLFmtMap[TEX_FORMAT_BC1_TYPELESS]           = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC1_UNORM]              = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;  // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC1_UNORM_SRGB]         = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT; // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC2_TYPELESS]           = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC2_UNORM]              = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC2_UNORM_SRGB]         = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC3_TYPELESS]           = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC3_UNORM]              = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC3_UNORM_SRGB]         = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC4_TYPELESS]           = GL_COMPRESSED_RED_RGTC1;
        m_FmtToGLFmtMap[TEX_FORMAT_BC4_UNORM]              = GL_COMPRESSED_RED_RGTC1;
        m_FmtToGLFmtMap[TEX_FORMAT_BC4_SNORM]              = GL_COMPRESSED_SIGNED_RED_RGTC1;
        m_FmtToGLFmtMap[TEX_FORMAT_BC5_TYPELESS]           = GL_COMPRESSED_RG_RGTC2;
        m_FmtToGLFmtMap[TEX_FORMAT_BC5_UNORM]              = GL_COMPRESSED_RG_RGTC2;
        m_FmtToGLFmtMap[TEX_FORMAT_BC5_SNORM]              = GL_COMPRESSED_SIGNED_RG_RGTC2;
        m_FmtToGLFmtMap[TEX_FORMAT_B5G6R5_UNORM]           = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_B5G5R5A1_UNORM]         = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_BGRA8_UNORM]            = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_BGRX8_UNORM]            = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM] = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_BGRA8_TYPELESS]         = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_BGRA8_UNORM_SRGB]       = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_BGRX8_TYPELESS]         = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_BGRX8_UNORM_SRGB]       = 0;
        m_FmtToGLFmtMap[TEX_FORMAT_BC6H_TYPELESS]          = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC6H_UF16]              = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC6H_SF16]              = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
        m_FmtToGLFmtMap[TEX_FORMAT_BC7_TYPELESS]           = GL_COMPRESSED_RGBA_BPTC_UNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_BC7_UNORM]              = GL_COMPRESSED_RGBA_BPTC_UNORM;
        m_FmtToGLFmtMap[TEX_FORMAT_BC7_UNORM_SRGB]         = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
        // clang-format on

        static_assert(TEX_FORMAT_NUM_FORMATS == 100, "Please enter the new format information above");
    }

    GLenum operator[](TEXTURE_FORMAT TexFormat) const
    {
        VERIFY_EXPR(TexFormat < m_FmtToGLFmtMap.size());
        return m_FmtToGLFmtMap[TexFormat];
    }

private:
    std::array<GLenum, TEX_FORMAT_NUM_FORMATS> m_FmtToGLFmtMap = {};
};

} // namespace

GLenum TexFormatToGLInternalTexFormat(TEXTURE_FORMAT TexFormat, Uint32 BindFlags)
{
    static const FormatToGLInternalTexFormatMap FormatMap;
    if (TexFormat >= TEX_FORMAT_UNKNOWN && TexFormat < TEX_FORMAT_NUM_FORMATS)
    {
        auto GLFormat = FormatMap[TexFormat];
        if (BindFlags != 0)
            GLFormat = CorrectGLTexFormat(GLFormat, BindFlags);
        return GLFormat;
    }
    else
    {
        UNEXPECTED("Texture format (", int{TexFormat}, ") is out of allowed range [0, ", int{TEX_FORMAT_NUM_FORMATS} - 1, "]");
        return 0;
    }
}

namespace
{

class InternalTexFormatToTexFormatMap
{
public:
    InternalTexFormatToTexFormatMap()
    {
        for (TEXTURE_FORMAT TexFmt = TEX_FORMAT_UNKNOWN; TexFmt < TEX_FORMAT_NUM_FORMATS; TexFmt = static_cast<TEXTURE_FORMAT>(static_cast<int>(TexFmt) + 1))
        {
            auto ComponentType = GetTextureFormatAttribs(TexFmt).ComponentType;
            // clang-format off
            if (ComponentType == COMPONENT_TYPE_UNDEFINED || 
                ComponentType == COMPONENT_TYPE_DEPTH_STENCIL ||
                TexFmt == TEX_FORMAT_RGB10A2_TYPELESS ||
                TexFmt == TEX_FORMAT_BC1_TYPELESS ||
                TexFmt == TEX_FORMAT_BC2_TYPELESS ||
                TexFmt == TEX_FORMAT_BC3_TYPELESS ||
                TexFmt == TEX_FORMAT_BC4_TYPELESS ||
                TexFmt == TEX_FORMAT_BC5_TYPELESS ||
                TexFmt == TEX_FORMAT_BC6H_TYPELESS ||
                TexFmt == TEX_FORMAT_BC7_TYPELESS)
                continue; // Skip typeless and depth-stencil formats
            // clang-format on
            auto GlTexFormat = TexFormatToGLInternalTexFormat(TexFmt);
            if (GlTexFormat != 0)
            {
                VERIFY_EXPR(m_FormatMap.find(GlTexFormat) == m_FormatMap.end());
                m_FormatMap[GlTexFormat] = TexFmt;
            }
            m_FormatMap[TexFormatToGLInternalTexFormat(TEX_FORMAT_D32_FLOAT_S8X24_UINT)] = TEX_FORMAT_D32_FLOAT_S8X24_UINT;
            m_FormatMap[TexFormatToGLInternalTexFormat(TEX_FORMAT_D24_UNORM_S8_UINT)]    = TEX_FORMAT_D24_UNORM_S8_UINT;
        }
    }

    TEXTURE_FORMAT operator[](GLenum GlFormat) const
    {
        auto formatIt = m_FormatMap.find(GlFormat);
        if (formatIt != m_FormatMap.end())
        {
            VERIFY_EXPR(GlFormat == TexFormatToGLInternalTexFormat(formatIt->second));
            return formatIt->second;
        }
        else
        {
            UNEXPECTED("Unknown GL format");
            return TEX_FORMAT_UNKNOWN;
        }
    }

private:
    std::unordered_map<GLenum, TEXTURE_FORMAT> m_FormatMap;
};

} // namespace

TEXTURE_FORMAT GLInternalTexFormatToTexFormat(GLenum GlFormat)
{
    static const InternalTexFormatToTexFormatMap FormatMap;
    return FormatMap[GlFormat];
}

NativePixelAttribs GetNativePixelTransferAttribs(TEXTURE_FORMAT TexFormat)
{
    // http://www.opengl.org/wiki/Pixel_Transfer

    static std::array<NativePixelAttribs, TEX_FORMAT_NUM_FORMATS> FmtToGLPixelFmt;

    static bool bAttribsMapIntialized = false;
    if (!bAttribsMapIntialized)
    {
        // clang-format off
        // http://www.opengl.org/wiki/Image_Format
        FmtToGLPixelFmt[TEX_FORMAT_UNKNOWN]                = NativePixelAttribs();

        FmtToGLPixelFmt[TEX_FORMAT_RGBA32_TYPELESS]        = NativePixelAttribs(GL_RGBA,         GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA32_FLOAT]           = NativePixelAttribs(GL_RGBA,         GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA32_UINT]            = NativePixelAttribs(GL_RGBA_INTEGER, GL_UNSIGNED_INT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA32_SINT]            = NativePixelAttribs(GL_RGBA_INTEGER, GL_INT);

        FmtToGLPixelFmt[TEX_FORMAT_RGB32_TYPELESS]         = NativePixelAttribs(GL_RGB,         GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RGB32_FLOAT]            = NativePixelAttribs(GL_RGB,         GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RGB32_UINT]             = NativePixelAttribs(GL_RGB_INTEGER, GL_UNSIGNED_INT);
        FmtToGLPixelFmt[TEX_FORMAT_RGB32_SINT]             = NativePixelAttribs(GL_RGB_INTEGER, GL_INT);

        FmtToGLPixelFmt[TEX_FORMAT_RGBA16_TYPELESS]        = NativePixelAttribs(GL_RGBA,         GL_HALF_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA16_FLOAT]           = NativePixelAttribs(GL_RGBA,         GL_HALF_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA16_UNORM]           = NativePixelAttribs(GL_RGBA,         GL_UNSIGNED_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA16_UINT]            = NativePixelAttribs(GL_RGBA_INTEGER, GL_UNSIGNED_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA16_SNORM]           = NativePixelAttribs(GL_RGBA,         GL_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA16_SINT]            = NativePixelAttribs(GL_RGBA_INTEGER, GL_SHORT);

        FmtToGLPixelFmt[TEX_FORMAT_RG32_TYPELESS]          = NativePixelAttribs(GL_RG,           GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RG32_FLOAT]             = NativePixelAttribs(GL_RG,           GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RG32_UINT]              = NativePixelAttribs(GL_RG_INTEGER,   GL_UNSIGNED_INT);
        FmtToGLPixelFmt[TEX_FORMAT_RG32_SINT]              = NativePixelAttribs(GL_RG_INTEGER,   GL_INT);

        FmtToGLPixelFmt[TEX_FORMAT_R32G8X24_TYPELESS]      = NativePixelAttribs(GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
        FmtToGLPixelFmt[TEX_FORMAT_D32_FLOAT_S8X24_UINT]   = NativePixelAttribs(GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
        FmtToGLPixelFmt[TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS]=NativePixelAttribs(GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
        FmtToGLPixelFmt[TEX_FORMAT_X32_TYPELESS_G8X24_UINT]= NativePixelAttribs(GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

        // Components are normally packed with the first component in the most significant
        // bits of the bitfield, and successive component occupying progressively less
        // significant locations.Types whose token names end with _REV reverse the component
        // packing order from least to most significant locations.
        FmtToGLPixelFmt[TEX_FORMAT_RGB10A2_TYPELESS]       = NativePixelAttribs(GL_RGBA,         GL_UNSIGNED_INT_2_10_10_10_REV);
        FmtToGLPixelFmt[TEX_FORMAT_RGB10A2_UNORM]          = NativePixelAttribs(GL_RGBA,         GL_UNSIGNED_INT_2_10_10_10_REV);
        FmtToGLPixelFmt[TEX_FORMAT_RGB10A2_UINT]           = NativePixelAttribs(GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV);
        FmtToGLPixelFmt[TEX_FORMAT_R11G11B10_FLOAT]        = NativePixelAttribs(GL_RGB,          GL_UNSIGNED_INT_10F_11F_11F_REV);

        FmtToGLPixelFmt[TEX_FORMAT_RGBA8_TYPELESS]         = NativePixelAttribs(GL_RGBA,         GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA8_UNORM]            = NativePixelAttribs(GL_RGBA,         GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA8_UNORM_SRGB]       = NativePixelAttribs(GL_RGBA,         GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA8_UINT]             = NativePixelAttribs(GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA8_SNORM]            = NativePixelAttribs(GL_RGBA,         GL_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RGBA8_SINT]             = NativePixelAttribs(GL_RGBA_INTEGER, GL_BYTE);

        FmtToGLPixelFmt[TEX_FORMAT_RG16_TYPELESS]          = NativePixelAttribs(GL_RG,           GL_HALF_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RG16_FLOAT]             = NativePixelAttribs(GL_RG,           GL_HALF_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_RG16_UNORM]             = NativePixelAttribs(GL_RG,           GL_UNSIGNED_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_RG16_UINT]              = NativePixelAttribs(GL_RG_INTEGER,   GL_UNSIGNED_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_RG16_SNORM]             = NativePixelAttribs(GL_RG,           GL_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_RG16_SINT]              = NativePixelAttribs(GL_RG_INTEGER,   GL_SHORT);

        FmtToGLPixelFmt[TEX_FORMAT_R32_TYPELESS]           = NativePixelAttribs(GL_RED,              GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_D32_FLOAT]              = NativePixelAttribs(GL_DEPTH_COMPONENT,  GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_R32_FLOAT]              = NativePixelAttribs(GL_RED,              GL_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_R32_UINT]               = NativePixelAttribs(GL_RED_INTEGER,      GL_UNSIGNED_INT);
        FmtToGLPixelFmt[TEX_FORMAT_R32_SINT]               = NativePixelAttribs(GL_RED_INTEGER,      GL_INT);

        FmtToGLPixelFmt[TEX_FORMAT_R24G8_TYPELESS]         = NativePixelAttribs(GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8);
        FmtToGLPixelFmt[TEX_FORMAT_D24_UNORM_S8_UINT]      = NativePixelAttribs(GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8);
        FmtToGLPixelFmt[TEX_FORMAT_R24_UNORM_X8_TYPELESS]  = NativePixelAttribs(GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8);
        FmtToGLPixelFmt[TEX_FORMAT_X24_TYPELESS_G8_UINT]   = NativePixelAttribs(GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8);

        FmtToGLPixelFmt[TEX_FORMAT_RG8_TYPELESS]           = NativePixelAttribs(GL_RG,           GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RG8_UNORM]              = NativePixelAttribs(GL_RG,           GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RG8_UINT]               = NativePixelAttribs(GL_RG_INTEGER,   GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RG8_SNORM]              = NativePixelAttribs(GL_RG,           GL_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_RG8_SINT]               = NativePixelAttribs(GL_RG_INTEGER,   GL_BYTE);

        FmtToGLPixelFmt[TEX_FORMAT_R16_TYPELESS]           = NativePixelAttribs(GL_RED,          GL_HALF_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_R16_FLOAT]              = NativePixelAttribs(GL_RED,          GL_HALF_FLOAT);
        FmtToGLPixelFmt[TEX_FORMAT_D16_UNORM]              = NativePixelAttribs(GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_R16_UNORM]              = NativePixelAttribs(GL_RED,          GL_UNSIGNED_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_R16_UINT]               = NativePixelAttribs(GL_RED_INTEGER,  GL_UNSIGNED_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_R16_SNORM]              = NativePixelAttribs(GL_RED,          GL_SHORT);
        FmtToGLPixelFmt[TEX_FORMAT_R16_SINT]               = NativePixelAttribs(GL_RED_INTEGER,  GL_SHORT);

        FmtToGLPixelFmt[TEX_FORMAT_R8_TYPELESS]            = NativePixelAttribs(GL_RED,          GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_R8_UNORM]               = NativePixelAttribs(GL_RED,          GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_R8_UINT]                = NativePixelAttribs(GL_RED_INTEGER,  GL_UNSIGNED_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_R8_SNORM]               = NativePixelAttribs(GL_RED,          GL_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_R8_SINT]                = NativePixelAttribs(GL_RED_INTEGER,  GL_BYTE);
        FmtToGLPixelFmt[TEX_FORMAT_A8_UNORM]               = NativePixelAttribs();

        FmtToGLPixelFmt[TEX_FORMAT_R1_UNORM]               = NativePixelAttribs();

        FmtToGLPixelFmt[TEX_FORMAT_RGB9E5_SHAREDEXP]       = NativePixelAttribs(GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV);
        FmtToGLPixelFmt[TEX_FORMAT_RG8_B8G8_UNORM]         = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_G8R8_G8B8_UNORM]        = NativePixelAttribs();

        // http://www.g-truc.net/post-0335.html
        // http://renderingpipeline.com/2012/07/texture-compression/
        FmtToGLPixelFmt[TEX_FORMAT_BC1_TYPELESS]           = NativePixelAttribs(GL_RGB, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC1_UNORM]              = NativePixelAttribs(GL_RGB, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC1_UNORM_SRGB]         = NativePixelAttribs(GL_RGB, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC2_TYPELESS]           = NativePixelAttribs(GL_RGBA, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC2_UNORM]              = NativePixelAttribs(GL_RGBA, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC2_UNORM_SRGB]         = NativePixelAttribs(GL_RGBA, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC3_TYPELESS]           = NativePixelAttribs(GL_RGBA, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC3_UNORM]              = NativePixelAttribs(GL_RGBA, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC3_UNORM_SRGB]         = NativePixelAttribs(GL_RGBA, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC4_TYPELESS]           = NativePixelAttribs(GL_RED, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC4_UNORM]              = NativePixelAttribs(GL_RED, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC4_SNORM]              = NativePixelAttribs(GL_RED, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC5_TYPELESS]           = NativePixelAttribs(GL_RG, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC5_UNORM]              = NativePixelAttribs(GL_RG, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_BC5_SNORM]              = NativePixelAttribs(GL_RG, 0, True);
        FmtToGLPixelFmt[TEX_FORMAT_B5G6R5_UNORM]           = NativePixelAttribs(GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV);
        FmtToGLPixelFmt[TEX_FORMAT_B5G5R5A1_UNORM]         = NativePixelAttribs(GL_RGB, GL_UNSIGNED_SHORT_1_5_5_5_REV);
        FmtToGLPixelFmt[TEX_FORMAT_BGRA8_UNORM]            = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_BGRX8_UNORM]            = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM] = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_BGRA8_TYPELESS]         = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_BGRA8_UNORM_SRGB]       = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_BGRX8_TYPELESS]         = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_BGRX8_UNORM_SRGB]       = NativePixelAttribs();
        FmtToGLPixelFmt[TEX_FORMAT_BC6H_TYPELESS]          = NativePixelAttribs(GL_RGB,  0,  True);
        FmtToGLPixelFmt[TEX_FORMAT_BC6H_UF16]              = NativePixelAttribs(GL_RGB,  0,  True);
        FmtToGLPixelFmt[TEX_FORMAT_BC6H_SF16]              = NativePixelAttribs(GL_RGB,  0,  True);
        FmtToGLPixelFmt[TEX_FORMAT_BC7_TYPELESS]           = NativePixelAttribs(GL_RGBA, 0,  True);
        FmtToGLPixelFmt[TEX_FORMAT_BC7_UNORM]              = NativePixelAttribs(GL_RGBA, 0,  True);
        FmtToGLPixelFmt[TEX_FORMAT_BC7_UNORM_SRGB]         = NativePixelAttribs(GL_RGBA, 0,  True);
        // clang-format on
        bAttribsMapIntialized = true;
    }

    if (TexFormat > TEX_FORMAT_UNKNOWN && TexFormat < TEX_FORMAT_NUM_FORMATS)
    {
        return FmtToGLPixelFmt[TexFormat];
    }
    else
    {
        UNEXPECTED("Texture format (", int{TexFormat}, ") is out of allowed range [1, ", int{TEX_FORMAT_NUM_FORMATS} - 1, "]");
        return FmtToGLPixelFmt[0];
    }
}

GLenum TypeToGLTexFormat(VALUE_TYPE ValType, Uint32 NumComponents, Bool bIsNormalized)
{
    switch (ValType)
    {
        case VT_FLOAT16:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return GL_R16F;
                case 2: return GL_RG16F;
                case 4: return GL_RGBA16F;
                default: UNEXPECTED("Unusupported number of components"); return 0;
            }
        }

        case VT_FLOAT32:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return GL_R32F;
                case 2: return GL_RG32F;
                case 3: return GL_RGB32F;
                case 4: return GL_RGBA32F;
                default: UNEXPECTED("Unusupported number of components"); return 0;
            }
        }

        case VT_INT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return GL_R32I;
                case 2: return GL_RG32I;
                case 3: return GL_RGB32I;
                case 4: return GL_RGBA32I;
                default: UNEXPECTED("Unusupported number of components"); return 0;
            }
        }

        case VT_UINT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return GL_R32UI;
                case 2: return GL_RG32UI;
                case 3: return GL_RGB32UI;
                case 4: return GL_RGBA32UI;
                default: UNEXPECTED("Unusupported number of components"); return 0;
            }
        }

        case VT_INT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return GL_R16_SNORM;
                    case 2: return GL_RG16_SNORM;
                    case 4: return GL_RGBA16_SNORM;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return GL_R16I;
                    case 2: return GL_RG16I;
                    case 4: return GL_RGBA16I;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
        }

        case VT_UINT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return GL_R16;
                    case 2: return GL_RG16;
                    case 4: return GL_RGBA16;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return GL_R16UI;
                    case 2: return GL_RG16UI;
                    case 4: return GL_RGBA16UI;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
        }

        case VT_INT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return GL_R8_SNORM;
                    case 2: return GL_RG8_SNORM;
                    case 4: return GL_RGBA8_SNORM;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return GL_R8I;
                    case 2: return GL_RG8I;
                    case 4: return GL_RGBA8I;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
        }

        case VT_UINT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return GL_R8;
                    case 2: return GL_RG8;
                    case 4: return GL_RGBA8;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return GL_R8UI;
                    case 2: return GL_RG8UI;
                    case 4: return GL_RGBA8UI;
                    default: UNEXPECTED("Unusupported number of components"); return 0;
                }
            }
        }

        default: UNEXPECTED("Unusupported format"); return 0;
    }
}

} // namespace Diligent
