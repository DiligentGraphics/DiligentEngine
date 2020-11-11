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

#include <algorithm>

#include "GraphicsAccessories.hpp"
#include "DebugUtilities.hpp"
#include "Align.hpp"

namespace Diligent
{

const Char* GetValueTypeString(VALUE_TYPE Val)
{
    static const Char* ValueTypeStrings[VT_NUM_TYPES] = {};
    static bool        bIsInit                        = false;
    if (!bIsInit)
    {
        // clang-format off
#define INIT_VALUE_TYPE_STR( ValType ) ValueTypeStrings[ValType] = #ValType
        INIT_VALUE_TYPE_STR( VT_UNDEFINED );
        INIT_VALUE_TYPE_STR( VT_INT8    );
        INIT_VALUE_TYPE_STR( VT_INT16   );
        INIT_VALUE_TYPE_STR( VT_INT32   );
        INIT_VALUE_TYPE_STR( VT_UINT8   );
        INIT_VALUE_TYPE_STR( VT_UINT16  );
        INIT_VALUE_TYPE_STR( VT_UINT32  );
        INIT_VALUE_TYPE_STR( VT_FLOAT16 );
        INIT_VALUE_TYPE_STR( VT_FLOAT32 );
#undef  INIT_VALUE_TYPE_STR
        // clang-format on
        static_assert(VT_NUM_TYPES == VT_FLOAT32 + 1, "Not all value type strings initialized.");
        bIsInit = true;
    }

    if (Val >= VT_UNDEFINED && Val < VT_NUM_TYPES)
    {
        return ValueTypeStrings[Val];
    }
    else
    {
        UNEXPECTED("Incorrect value type (", Val, ")");
        return "unknown value type";
    }
}

class TexFormatToViewFormatConverter
{
public:
    TexFormatToViewFormatConverter()
    {
        // clang-format off
        static_assert(TEXTURE_VIEW_SHADER_RESOURCE == 1, "TEXTURE_VIEW_SHADER_RESOURCE == 1 expected");
        static_assert(TEXTURE_VIEW_RENDER_TARGET == 2, "TEXTURE_VIEW_RENDER_TARGET == 2 expected");
        static_assert(TEXTURE_VIEW_DEPTH_STENCIL == 3, "TEXTURE_VIEW_DEPTH_STENCIL == 3 expected");
        static_assert(TEXTURE_VIEW_UNORDERED_ACCESS == 4, "TEXTURE_VIEW_UNORDERED_ACCESS == 4 expected");
#define INIT_TEX_VIEW_FORMAT_INFO(TexFmt, SRVFmt, RTVFmt, DSVFmt, UAVFmt)\
        {\
            m_ViewFormats[ TexFmt ][TEXTURE_VIEW_SHADER_RESOURCE-1]  = TEX_FORMAT_##SRVFmt; \
            m_ViewFormats[ TexFmt ][TEXTURE_VIEW_RENDER_TARGET-1]    = TEX_FORMAT_##RTVFmt; \
            m_ViewFormats[ TexFmt ][TEXTURE_VIEW_DEPTH_STENCIL-1]    = TEX_FORMAT_##DSVFmt; \
            m_ViewFormats[ TexFmt ][TEXTURE_VIEW_UNORDERED_ACCESS-1] = TEX_FORMAT_##UAVFmt; \
        }
        
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_UNKNOWN,                 UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA32_TYPELESS,         RGBA32_FLOAT, RGBA32_FLOAT, UNKNOWN, RGBA32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA32_FLOAT,            RGBA32_FLOAT, RGBA32_FLOAT, UNKNOWN, RGBA32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA32_UINT,             RGBA32_UINT,  RGBA32_UINT,  UNKNOWN, RGBA32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA32_SINT,             RGBA32_SINT,  RGBA32_SINT,  UNKNOWN, RGBA32_SINT);
                                                                       
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB32_TYPELESS,          RGB32_FLOAT, RGB32_FLOAT, UNKNOWN, RGB32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB32_FLOAT,             RGB32_FLOAT, RGB32_FLOAT, UNKNOWN, RGB32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB32_UINT,              RGB32_UINT,  RGB32_UINT,  UNKNOWN, RGB32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB32_SINT,              RGB32_SINT,  RGB32_SINT,  UNKNOWN, RGB32_SINT);
                                                                      
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA16_TYPELESS,         RGBA16_FLOAT, RGBA16_FLOAT, UNKNOWN, RGBA16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA16_FLOAT,            RGBA16_FLOAT, RGBA16_FLOAT, UNKNOWN, RGBA16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA16_UNORM,            RGBA16_UNORM, RGBA16_UNORM, UNKNOWN, RGBA16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA16_UINT,             RGBA16_UINT,  RGBA16_UINT,  UNKNOWN, RGBA16_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA16_SNORM,            RGBA16_SNORM, RGBA16_SNORM, UNKNOWN, RGBA16_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA16_SINT,             RGBA16_SINT,  RGBA16_SINT,  UNKNOWN, RGBA16_SINT);
                                                                       
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG32_TYPELESS,           RG32_FLOAT, RG32_FLOAT, UNKNOWN, RG32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG32_FLOAT,              RG32_FLOAT, RG32_FLOAT, UNKNOWN, RG32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG32_UINT,               RG32_UINT,  RG32_UINT,  UNKNOWN, RG32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG32_SINT,               RG32_SINT,  RG32_SINT,  UNKNOWN, RG32_SINT);
                                                                       
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R32G8X24_TYPELESS,       R32_FLOAT_X8X24_TYPELESS, UNKNOWN, D32_FLOAT_S8X24_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_D32_FLOAT_S8X24_UINT,    R32_FLOAT_X8X24_TYPELESS, UNKNOWN, D32_FLOAT_S8X24_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,R32_FLOAT_X8X24_TYPELESS, UNKNOWN, D32_FLOAT_S8X24_UINT, R32_FLOAT_X8X24_TYPELESS);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_X32_TYPELESS_G8X24_UINT, X32_TYPELESS_G8X24_UINT,  UNKNOWN, D32_FLOAT_S8X24_UINT, X32_TYPELESS_G8X24_UINT);
                                                                       
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB10A2_TYPELESS,        RGB10A2_UNORM, RGB10A2_UNORM, UNKNOWN, RGB10A2_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB10A2_UNORM,           RGB10A2_UNORM, RGB10A2_UNORM, UNKNOWN, RGB10A2_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB10A2_UINT,            RGB10A2_UINT,  RGB10A2_UINT,  UNKNOWN, RGB10A2_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R11G11B10_FLOAT,         R11G11B10_FLOAT, R11G11B10_FLOAT, UNKNOWN, R11G11B10_FLOAT);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA8_TYPELESS,          RGBA8_UNORM_SRGB, RGBA8_UNORM_SRGB, UNKNOWN, RGBA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA8_UNORM,             RGBA8_UNORM,      RGBA8_UNORM,      UNKNOWN, RGBA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA8_UNORM_SRGB,        RGBA8_UNORM_SRGB, RGBA8_UNORM_SRGB, UNKNOWN, RGBA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA8_UINT,              RGBA8_UINT,       RGBA8_UINT,       UNKNOWN, RGBA8_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA8_SNORM,             RGBA8_SNORM,      RGBA8_SNORM,      UNKNOWN, RGBA8_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGBA8_SINT,              RGBA8_SINT,       RGBA8_SINT,       UNKNOWN, RGBA8_SINT);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG16_TYPELESS,           RG16_FLOAT, RG16_FLOAT, UNKNOWN, RG16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG16_FLOAT,              RG16_FLOAT, RG16_FLOAT, UNKNOWN, RG16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG16_UNORM,              RG16_UNORM, RG16_UNORM, UNKNOWN, RG16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG16_UINT,               RG16_UINT,  RG16_UINT,  UNKNOWN, RG16_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG16_SNORM,              RG16_SNORM, RG16_SNORM, UNKNOWN, RG16_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG16_SINT,               RG16_SINT,  RG16_SINT,  UNKNOWN, RG16_SINT);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R32_TYPELESS,            R32_FLOAT, R32_FLOAT, D32_FLOAT, R32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_D32_FLOAT,               R32_FLOAT, R32_FLOAT, D32_FLOAT, R32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R32_FLOAT,               R32_FLOAT, R32_FLOAT, D32_FLOAT, R32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R32_UINT,                R32_UINT,  R32_UINT,  UNKNOWN,   R32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R32_SINT,                R32_SINT,  R32_SINT,  UNKNOWN,   R32_SINT);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R24G8_TYPELESS,          R24_UNORM_X8_TYPELESS, UNKNOWN, D24_UNORM_S8_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_D24_UNORM_S8_UINT,       R24_UNORM_X8_TYPELESS, UNKNOWN, D24_UNORM_S8_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R24_UNORM_X8_TYPELESS,   R24_UNORM_X8_TYPELESS, UNKNOWN, D24_UNORM_S8_UINT, R24_UNORM_X8_TYPELESS);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_X24_TYPELESS_G8_UINT,    X24_TYPELESS_G8_UINT,  UNKNOWN, D24_UNORM_S8_UINT, X24_TYPELESS_G8_UINT);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG8_TYPELESS,            RG8_UNORM, RG8_UNORM, UNKNOWN, RG8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG8_UNORM,               RG8_UNORM, RG8_UNORM, UNKNOWN, RG8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG8_UINT,                RG8_UINT,  RG8_UINT,  UNKNOWN, RG8_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG8_SNORM,               RG8_SNORM, RG8_SNORM, UNKNOWN, RG8_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG8_SINT,                RG8_SINT,  RG8_SINT,  UNKNOWN, RG8_SINT);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R16_TYPELESS,            R16_FLOAT, R16_FLOAT, UNKNOWN,   R16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R16_FLOAT,               R16_FLOAT, R16_FLOAT, UNKNOWN,   R16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_D16_UNORM,               R16_UNORM, R16_UNORM, D16_UNORM, R16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R16_UNORM,               R16_UNORM, R16_UNORM, D16_UNORM, R16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R16_UINT,                R16_UINT,  R16_UINT,  UNKNOWN,   R16_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R16_SNORM,               R16_SNORM, R16_SNORM, UNKNOWN,   R16_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R16_SINT,                R16_SINT,  R16_SINT,  UNKNOWN,   R16_SINT);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R8_TYPELESS,             R8_UNORM, R8_UNORM, UNKNOWN, R8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R8_UNORM,                R8_UNORM, R8_UNORM, UNKNOWN, R8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R8_UINT,                 R8_UINT,  R8_UINT,  UNKNOWN, R8_UINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R8_SNORM,                R8_SNORM, R8_SNORM, UNKNOWN, R8_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R8_SINT,                 R8_SINT,  R8_SINT,  UNKNOWN, R8_SINT);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_A8_UNORM,                A8_UNORM, A8_UNORM, UNKNOWN, A8_UNORM);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R1_UNORM,                R1_UNORM, R1_UNORM, UNKNOWN, R1_UNORM);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RGB9E5_SHAREDEXP,        RGB9E5_SHAREDEXP, RGB9E5_SHAREDEXP, UNKNOWN, RGB9E5_SHAREDEXP);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_RG8_B8G8_UNORM,          RG8_B8G8_UNORM,  RG8_B8G8_UNORM,  UNKNOWN, RG8_B8G8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_G8R8_G8B8_UNORM,         G8R8_G8B8_UNORM, G8R8_G8B8_UNORM, UNKNOWN, G8R8_G8B8_UNORM);

        // http://www.g-truc.net/post-0335.html
        // http://renderingpipeline.com/2012/07/texture-compression/
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC1_TYPELESS,            BC1_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC1_UNORM,               BC1_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC1_UNORM_SRGB,          BC1_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC2_TYPELESS,            BC2_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC2_UNORM,               BC2_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC2_UNORM_SRGB,          BC2_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC3_TYPELESS,            BC3_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC3_UNORM,               BC3_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC3_UNORM_SRGB,          BC3_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC4_TYPELESS,            BC4_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC4_UNORM,               BC4_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC4_SNORM,               BC4_SNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC5_TYPELESS,            BC5_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC5_UNORM,               BC5_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC5_SNORM,               BC5_SNORM,      UNKNOWN, UNKNOWN, UNKNOWN);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_B5G6R5_UNORM,            B5G6R5_UNORM,   B5G6R5_UNORM,    UNKNOWN, B5G6R5_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_B5G5R5A1_UNORM,          B5G5R5A1_UNORM, B5G5R5A1_UNORM,  UNKNOWN, B5G5R5A1_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BGRA8_UNORM,             BGRA8_UNORM,    BGRA8_UNORM,     UNKNOWN, BGRA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BGRX8_UNORM,             BGRX8_UNORM,    BGRX8_UNORM,     UNKNOWN, BGRX8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, R10G10B10_XR_BIAS_A2_UNORM, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BGRA8_TYPELESS,          BGRA8_UNORM_SRGB, BGRA8_UNORM_SRGB, UNKNOWN, BGRA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BGRA8_UNORM_SRGB,        BGRA8_UNORM_SRGB, BGRA8_UNORM_SRGB, UNKNOWN, BGRA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BGRX8_TYPELESS,          BGRX8_UNORM_SRGB, BGRX8_UNORM_SRGB, UNKNOWN, BGRX8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BGRX8_UNORM_SRGB,        BGRX8_UNORM_SRGB, BGRX8_UNORM_SRGB, UNKNOWN, BGRX8_UNORM);

        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC6H_TYPELESS,           BC6H_UF16,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC6H_UF16,               BC6H_UF16,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC6H_SF16,               BC6H_SF16,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC7_TYPELESS,            BC7_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC7_UNORM,               BC7_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO( TEX_FORMAT_BC7_UNORM_SRGB,          BC7_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
#undef INIT_TVIEW_FORMAT_INFO
        // clang-format on
    }

    TEXTURE_FORMAT GetViewFormat(TEXTURE_FORMAT Format, TEXTURE_VIEW_TYPE ViewType, Uint32 BindFlags)
    {
        VERIFY(ViewType > TEXTURE_VIEW_UNDEFINED && ViewType < TEXTURE_VIEW_NUM_VIEWS, "Unexpected texture view type");
        VERIFY(Format >= TEX_FORMAT_UNKNOWN && Format < TEX_FORMAT_NUM_FORMATS, "Unknown texture format");
        switch (Format)
        {
            case TEX_FORMAT_R16_TYPELESS:
            {
                if (BindFlags & BIND_DEPTH_STENCIL)
                {
                    // clang-format off
                    static TEXTURE_FORMAT D16_ViewFmts[] = 
                    {
                        TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UNORM, TEX_FORMAT_D16_UNORM, TEX_FORMAT_R16_UNORM
                    };
                    // clang-format on
                    return D16_ViewFmts[ViewType - 1];
                }
            }

            default: /*do nothing*/ break;
        }

        return m_ViewFormats[Format][ViewType - 1];
    }

private:
    TEXTURE_FORMAT m_ViewFormats[TEX_FORMAT_NUM_FORMATS][TEXTURE_VIEW_NUM_VIEWS - 1];
};

TEXTURE_FORMAT GetDefaultTextureViewFormat(TEXTURE_FORMAT TextureFormat, TEXTURE_VIEW_TYPE ViewType, Uint32 BindFlags)
{
    static TexFormatToViewFormatConverter FmtConverter;
    return FmtConverter.GetViewFormat(TextureFormat, ViewType, BindFlags);
}

const TextureFormatAttribs& GetTextureFormatAttribs(TEXTURE_FORMAT Format)
{
    static TextureFormatAttribs FmtAttribs[TEX_FORMAT_NUM_FORMATS];
    static bool                 bIsInit = false;
    // Note that this implementation is thread-safe
    // Even if two threads try to call the function at the same time,
    // the worst thing that might happen is that the array will be
    // initialized multiple times. But the result will always be correct.
    if (!bIsInit)
    {
        // clang-format off
#define INIT_TEX_FORMAT_INFO(TexFmt, ComponentSize, NumComponents, ComponentType, IsTypeless, BlockWidth, BlockHeight) \
        FmtAttribs[ TexFmt ] = TextureFormatAttribs(#TexFmt, TexFmt, ComponentSize, NumComponents, ComponentType, IsTypeless, BlockWidth, BlockHeight);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA32_TYPELESS,         4, 4, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA32_FLOAT,            4, 4, COMPONENT_TYPE_FLOAT,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA32_UINT,             4, 4, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA32_SINT,             4, 4, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB32_TYPELESS,          4, 3, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB32_FLOAT,             4, 3, COMPONENT_TYPE_FLOAT,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB32_UINT,              4, 3, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB32_SINT,              4, 3, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA16_TYPELESS,         2, 4, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA16_FLOAT,            2, 4, COMPONENT_TYPE_FLOAT,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA16_UNORM,            2, 4, COMPONENT_TYPE_UNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA16_UINT,             2, 4, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA16_SNORM,            2, 4, COMPONENT_TYPE_SNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA16_SINT,             2, 4, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG32_TYPELESS,           4, 2, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG32_FLOAT,              4, 2, COMPONENT_TYPE_FLOAT,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG32_UINT,               4, 2, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG32_SINT,               4, 2, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R32G8X24_TYPELESS,       4, 2, COMPONENT_TYPE_DEPTH_STENCIL,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_D32_FLOAT_S8X24_UINT,    4, 2, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,4, 2, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_X32_TYPELESS_G8X24_UINT, 4, 2, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB10A2_TYPELESS,        4, 1, COMPONENT_TYPE_COMPOUND,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB10A2_UNORM,           4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB10A2_UINT,            4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R11G11B10_FLOAT,         4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA8_TYPELESS,          1, 4, COMPONENT_TYPE_UNDEFINED,   true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA8_UNORM,             1, 4, COMPONENT_TYPE_UNORM,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA8_UNORM_SRGB,        1, 4, COMPONENT_TYPE_UNORM_SRGB, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA8_UINT,              1, 4, COMPONENT_TYPE_UINT,       false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA8_SNORM,             1, 4, COMPONENT_TYPE_SNORM,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGBA8_SINT,              1, 4, COMPONENT_TYPE_SINT,       false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG16_TYPELESS,           2, 2, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG16_FLOAT,              2, 2, COMPONENT_TYPE_FLOAT,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG16_UNORM,              2, 2, COMPONENT_TYPE_UNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG16_UINT,               2, 2, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG16_SNORM,              2, 2, COMPONENT_TYPE_SNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG16_SINT,               2, 2, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R32_TYPELESS,            4, 1, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_D32_FLOAT,               4, 1, COMPONENT_TYPE_DEPTH,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R32_FLOAT,               4, 1, COMPONENT_TYPE_FLOAT,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R32_UINT,                4, 1, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R32_SINT,                4, 1, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R24G8_TYPELESS,          4, 1, COMPONENT_TYPE_DEPTH_STENCIL,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_D24_UNORM_S8_UINT,       4, 1, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R24_UNORM_X8_TYPELESS,   4, 1, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_X24_TYPELESS_G8_UINT,    4, 1, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG8_TYPELESS,            1, 2, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG8_UNORM,               1, 2, COMPONENT_TYPE_UNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG8_UINT,                1, 2, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG8_SNORM,               1, 2, COMPONENT_TYPE_SNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG8_SINT,                1, 2, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R16_TYPELESS,            2, 1, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R16_FLOAT,               2, 1, COMPONENT_TYPE_FLOAT,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_D16_UNORM,               2, 1, COMPONENT_TYPE_DEPTH,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R16_UNORM,               2, 1, COMPONENT_TYPE_UNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R16_UINT,                2, 1, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R16_SNORM,               2, 1, COMPONENT_TYPE_SNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R16_SINT,                2, 1, COMPONENT_TYPE_SINT,      false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R8_TYPELESS,             1, 1, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R8_UNORM,                1, 1, COMPONENT_TYPE_UNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R8_UINT,                 1, 1, COMPONENT_TYPE_UINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R8_SNORM,                1, 1, COMPONENT_TYPE_SNORM,     false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R8_SINT,                 1, 1, COMPONENT_TYPE_SINT,      false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_A8_UNORM,                1, 1, COMPONENT_TYPE_UNORM,     false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R1_UNORM,                1, 1, COMPONENT_TYPE_UNORM,    false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RGB9E5_SHAREDEXP,        4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_RG8_B8G8_UNORM,          1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_G8R8_G8B8_UNORM,         1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);

        // http://www.g-truc.net/post-0335.html
        // http://renderingpipeline.com/2012/07/texture-compression/
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC1_TYPELESS,            8,  3, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC1_UNORM,               8,  3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC1_UNORM_SRGB,          8,  3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC2_TYPELESS,            16, 4, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC2_UNORM,               16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC2_UNORM_SRGB,          16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC3_TYPELESS,            16, 4, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC3_UNORM,               16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC3_UNORM_SRGB,          16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC4_TYPELESS,            8,  1, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC4_UNORM,               8,  1, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC4_SNORM,               8,  1, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC5_TYPELESS,            16, 2, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC5_UNORM,               16, 2, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC5_SNORM,               16, 2, COMPONENT_TYPE_COMPRESSED, false, 4,4);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_B5G6R5_UNORM,            2, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_B5G5R5A1_UNORM,          2, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BGRA8_UNORM,             1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BGRX8_UNORM,             1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,  4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BGRA8_TYPELESS,          1, 4, COMPONENT_TYPE_UNDEFINED,     true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BGRA8_UNORM_SRGB,        1, 4, COMPONENT_TYPE_UNORM_SRGB,   false, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BGRX8_TYPELESS,          1, 4, COMPONENT_TYPE_UNDEFINED,     true, 1,1);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BGRX8_UNORM_SRGB,        1, 4, COMPONENT_TYPE_UNORM_SRGB,   false, 1,1);

        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC6H_TYPELESS,           16, 3, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC6H_UF16,               16, 3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC6H_SF16,               16, 3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC7_TYPELESS,            16, 4, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC7_UNORM,               16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
        INIT_TEX_FORMAT_INFO( TEX_FORMAT_BC7_UNORM_SRGB,          16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
#undef  INIT_TEX_FORMAT_INFO
        // clang-format on
        static_assert(TEX_FORMAT_NUM_FORMATS == TEX_FORMAT_BC7_UNORM_SRGB + 1, "Not all texture formats initialized.");

#ifdef DILIGENT_DEBUG
        for (Uint32 Fmt = TEX_FORMAT_UNKNOWN; Fmt < TEX_FORMAT_NUM_FORMATS; ++Fmt)
            VERIFY(FmtAttribs[Fmt].Format == static_cast<TEXTURE_FORMAT>(Fmt), "Uninitialized format");
#endif

        bIsInit = true;
    }

    if (Format >= TEX_FORMAT_UNKNOWN && Format < TEX_FORMAT_NUM_FORMATS)
    {
        const auto& Attribs = FmtAttribs[Format];
        VERIFY(Attribs.Format == Format, "Unexpected format");
        return Attribs;
    }
    else
    {
        UNEXPECTED("Texture format (", int{Format}, ") is out of allowed range [0, ", int{TEX_FORMAT_NUM_FORMATS} - 1, "]");
        return FmtAttribs[0];
    }
}


const Char* GetTexViewTypeLiteralName(TEXTURE_VIEW_TYPE ViewType)
{
    static const Char* TexViewLiteralNames[TEXTURE_VIEW_NUM_VIEWS] = {};
    static bool        bIsInit                                     = false;
    // Note that this implementation is thread-safe
    // Even if two threads try to call the function at the same time,
    // the worst thing that might happen is that the array will be
    // initialized multiple times. But the result will always be correct.
    if (!bIsInit)
    {
        // clang-format off
#define INIT_TEX_VIEW_TYPE_NAME(ViewType)TexViewLiteralNames[ViewType] = #ViewType
        INIT_TEX_VIEW_TYPE_NAME( TEXTURE_VIEW_UNDEFINED );
        INIT_TEX_VIEW_TYPE_NAME( TEXTURE_VIEW_SHADER_RESOURCE );
        INIT_TEX_VIEW_TYPE_NAME( TEXTURE_VIEW_RENDER_TARGET );
        INIT_TEX_VIEW_TYPE_NAME( TEXTURE_VIEW_DEPTH_STENCIL );
        INIT_TEX_VIEW_TYPE_NAME( TEXTURE_VIEW_UNORDERED_ACCESS );
#undef  INIT_TEX_VIEW_TYPE_NAME
        // clang-format on
        static_assert(TEXTURE_VIEW_NUM_VIEWS == TEXTURE_VIEW_UNORDERED_ACCESS + 1, "Not all texture views names initialized.");

        bIsInit = true;
    }

    if (ViewType >= TEXTURE_VIEW_UNDEFINED && ViewType < TEXTURE_VIEW_NUM_VIEWS)
    {
        return TexViewLiteralNames[ViewType];
    }
    else
    {
        UNEXPECTED("Texture view type (", ViewType, ") is out of allowed range [0, ", TEXTURE_VIEW_NUM_VIEWS - 1, "]");
        return "<Unknown texture view type>";
    }
}

const Char* GetBufferViewTypeLiteralName(BUFFER_VIEW_TYPE ViewType)
{
    static const Char* BuffViewLiteralNames[BUFFER_VIEW_NUM_VIEWS] = {};
    static bool        bIsInit                                     = false;
    // Note that this implementation is thread-safe
    // Even if two threads try to call the function at the same time,
    // the worst thing that might happen is that the array will be
    // initialized multiple times. But the result will always be correct.
    if (!bIsInit)
    {
        // clang-format off
#define INIT_BUFF_VIEW_TYPE_NAME(ViewType)BuffViewLiteralNames[ViewType] = #ViewType
        INIT_BUFF_VIEW_TYPE_NAME( BUFFER_VIEW_UNDEFINED );
        INIT_BUFF_VIEW_TYPE_NAME( BUFFER_VIEW_SHADER_RESOURCE );
        INIT_BUFF_VIEW_TYPE_NAME( BUFFER_VIEW_UNORDERED_ACCESS );
 #undef INIT_BUFF_VIEW_TYPE_NAME
        // clang-format on
        static_assert(BUFFER_VIEW_NUM_VIEWS == BUFFER_VIEW_UNORDERED_ACCESS + 1, "Not all buffer views names initialized.");

        bIsInit = true;
    }

    if (ViewType >= BUFFER_VIEW_UNDEFINED && ViewType < BUFFER_VIEW_NUM_VIEWS)
    {
        return BuffViewLiteralNames[ViewType];
    }
    else
    {
        UNEXPECTED("Buffer view type (", ViewType, ") is out of allowed range [0, ", BUFFER_VIEW_NUM_VIEWS - 1, "]");
        return "<Unknown buffer view type>";
    }
}

const Char* GetShaderTypeLiteralName(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x080, "Please handle the new shader type in the switch below");
    switch (ShaderType)
    {
        // clang-format off
#define RETURN_SHADER_TYPE_NAME(ShaderType)\
        case ShaderType: return #ShaderType;

        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_UNKNOWN      )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_VERTEX       )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_PIXEL        )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_GEOMETRY     )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_HULL         )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_DOMAIN       )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_COMPUTE      )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_AMPLIFICATION)
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_MESH         )
#undef  RETURN_SHADER_TYPE_NAME
            // clang-format on

        default: UNEXPECTED("Unknown shader type constant ", Uint32{ShaderType}); return "<Unknown shader type>";
    }
}

String GetShaderStagesString(SHADER_TYPE ShaderStages)
{
    String StagesStr;
    for (Uint32 Stage = SHADER_TYPE_VERTEX; ShaderStages != 0 && Stage <= SHADER_TYPE_LAST; Stage <<= 1)
    {
        if (ShaderStages & Stage)
        {
            if (StagesStr.length())
                StagesStr += ", ";
            StagesStr += GetShaderTypeLiteralName(static_cast<SHADER_TYPE>(Stage));
            ShaderStages &= ~static_cast<SHADER_TYPE>(Stage);
        }
    }
    VERIFY_EXPR(ShaderStages == 0);
    return StagesStr;
}

const Char* GetShaderVariableTypeLiteralName(SHADER_RESOURCE_VARIABLE_TYPE VarType, bool bGetFullName)
{
    static const Char* ShortVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES];
    static const Char* FullVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES];
    static bool        bVarTypeStrsInit = false;
    if (!bVarTypeStrsInit)
    {
        ShortVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_STATIC]  = "static";
        ShortVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE] = "mutable";
        ShortVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC] = "dynamic";
        FullVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_STATIC]   = "SHADER_RESOURCE_VARIABLE_TYPE_STATIC";
        FullVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE]  = "SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE";
        FullVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC]  = "SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC";

        static_assert(SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC + 1, "Not all shader variable types initialized.");

        bVarTypeStrsInit = true;
    }
    if (VarType >= SHADER_RESOURCE_VARIABLE_TYPE_STATIC && VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES)
        return (bGetFullName ? FullVarTypeNameStrings : ShortVarTypeNameStrings)[VarType];
    else
    {
        UNEXPECTED("Unknow shader variable type");
        return "unknow";
    }
}


const Char* GetShaderResourceTypeLiteralName(SHADER_RESOURCE_TYPE ResourceType, bool bGetFullName)
{
    switch (ResourceType)
    {
        // clang-format off
        case SHADER_RESOURCE_TYPE_UNKNOWN:         return bGetFullName ?  "SHADER_RESOURCE_TYPE_UNKNOWN"         : "unknown";
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER: return bGetFullName ?  "SHADER_RESOURCE_TYPE_CONSTANT_BUFFER" : "constant buffer";
        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:     return bGetFullName ?  "SHADER_RESOURCE_TYPE_TEXTURE_SRV"     : "texture SRV";
        case SHADER_RESOURCE_TYPE_BUFFER_SRV:      return bGetFullName ?  "SHADER_RESOURCE_TYPE_BUFFER_SRV"      : "buffer SRV";
        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:     return bGetFullName ?  "SHADER_RESOURCE_TYPE_TEXTURE_UAV"     : "texture UAV";
        case SHADER_RESOURCE_TYPE_BUFFER_UAV:      return bGetFullName ?  "SHADER_RESOURCE_TYPE_BUFFER_UAV"      : "buffer UAV";
        case SHADER_RESOURCE_TYPE_SAMPLER:         return bGetFullName ?  "SHADER_RESOURCE_TYPE_SAMPLER"         : "sampler";
        // clang-format on
        default:
            UNEXPECTED("Unexepcted resource type (", Uint32{ResourceType}, ")");
            return "UNKNOWN";
    }
}

const Char* GetFilterTypeLiteralName(FILTER_TYPE FilterType, bool bGetFullName)
{
    switch (FilterType)
    {
        // clang-format off
        case FILTER_TYPE_UNKNOWN:                return bGetFullName ? "FILTER_TYPE_UNKNOWN"                : "unknown";
        case FILTER_TYPE_POINT:                  return bGetFullName ? "FILTER_TYPE_POINT"                  : "point";
        case FILTER_TYPE_LINEAR:                 return bGetFullName ? "FILTER_TYPE_LINEAR"                 : "linear";
        case FILTER_TYPE_ANISOTROPIC:            return bGetFullName ? "FILTER_TYPE_ANISOTROPIC"            : "anisotropic";
        case FILTER_TYPE_COMPARISON_POINT:       return bGetFullName ? "FILTER_TYPE_COMPARISON_POINT"       : "comparison point";
        case FILTER_TYPE_COMPARISON_LINEAR:      return bGetFullName ? "FILTER_TYPE_COMPARISON_LINEAR"      : "comparison linear";
        case FILTER_TYPE_COMPARISON_ANISOTROPIC: return bGetFullName ? "FILTER_TYPE_COMPARISON_ANISOTROPIC" : "comparison anisotropic";
        case FILTER_TYPE_MINIMUM_POINT:          return bGetFullName ? "FILTER_TYPE_MINIMUM_POINT"          : "minimum point";
        case FILTER_TYPE_MINIMUM_LINEAR:         return bGetFullName ? "FILTER_TYPE_MINIMUM_LINEAR"         : "minimum linear";
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:    return bGetFullName ? "FILTER_TYPE_MINIMUM_ANISOTROPIC"    : "minimum anisotropic";
        case FILTER_TYPE_MAXIMUM_POINT:          return bGetFullName ? "FILTER_TYPE_MAXIMUM_POINT"          : "maximum point";
        case FILTER_TYPE_MAXIMUM_LINEAR:         return bGetFullName ? "FILTER_TYPE_MAXIMUM_LINEAR"         : "maximum linear";
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:    return bGetFullName ? "FILTER_TYPE_MAXIMUM_ANISOTROPIC"    : "maximum anisotropic";
        // clang-format on
        default:
            UNEXPECTED("Unexepcted filter type (", Uint32{FilterType}, ")");
            return "UNKNOWN";
    }
}

const Char* GetTextureAddressModeLiteralName(TEXTURE_ADDRESS_MODE AddressMode, bool bGetFullName)
{
    switch (AddressMode)
    {
        // clang-format off
        case TEXTURE_ADDRESS_UNKNOWN:     return bGetFullName ? "TEXTURE_ADDRESS_UNKNOWN"     : "unknown";
        case TEXTURE_ADDRESS_WRAP:        return bGetFullName ? "TEXTURE_ADDRESS_WRAP"        : "wrap";
        case TEXTURE_ADDRESS_MIRROR:      return bGetFullName ? "TEXTURE_ADDRESS_MIRROR"      : "mirror";
        case TEXTURE_ADDRESS_CLAMP:       return bGetFullName ? "TEXTURE_ADDRESS_CLAMP"       : "clamp";
        case TEXTURE_ADDRESS_BORDER:      return bGetFullName ? "TEXTURE_ADDRESS_BORDER"      : "border";
        case TEXTURE_ADDRESS_MIRROR_ONCE: return bGetFullName ? "TEXTURE_ADDRESS_MIRROR_ONCE" : "mirror once";
        // clang-format on
        default:
            UNEXPECTED("Unexepcted texture address mode (", Uint32{AddressMode}, ")");
            return "UNKNOWN";
    }
}

const Char* GetComparisonFunctionLiteralName(COMPARISON_FUNCTION ComparisonFunc, bool bGetFullName)
{
    switch (ComparisonFunc)
    {
        // clang-format off
        case COMPARISON_FUNC_UNKNOWN:       return bGetFullName ? "COMPARISON_FUNC_UNKNOWN"      : "unknown";
        case COMPARISON_FUNC_NEVER:         return bGetFullName ? "COMPARISON_FUNC_NEVER"        : "never";
        case COMPARISON_FUNC_LESS:          return bGetFullName ? "COMPARISON_FUNC_LESS"         : "less";
        case COMPARISON_FUNC_EQUAL:         return bGetFullName ? "COMPARISON_FUNC_EQUAL"        : "equal";
        case COMPARISON_FUNC_LESS_EQUAL:    return bGetFullName ? "COMPARISON_FUNC_LESS_EQUAL"   : "less equal";
        case COMPARISON_FUNC_GREATER:       return bGetFullName ? "COMPARISON_FUNC_GREATER"      : "greater";
        case COMPARISON_FUNC_NOT_EQUAL:     return bGetFullName ? "COMPARISON_FUNC_NOT_EQUAL"    : "not equal";
        case COMPARISON_FUNC_GREATER_EQUAL: return bGetFullName ? "COMPARISON_FUNC_GREATER_EQUAL": "greater equal";
        case COMPARISON_FUNC_ALWAYS:        return bGetFullName ? "COMPARISON_FUNC_ALWAYS"       : "always";
        // clang-format on
        default:
            UNEXPECTED("Unexepcted comparison function (", Uint32{ComparisonFunc}, ")");
            return "UNKNOWN";
    }
}

const Char* GetStencilOpLiteralName(STENCIL_OP StencilOp)
{
#define STENCIL_OP_TO_STR(Op) \
    case Op: return #Op

    switch (StencilOp)
    {
        STENCIL_OP_TO_STR(STENCIL_OP_UNDEFINED);
        STENCIL_OP_TO_STR(STENCIL_OP_KEEP);
        STENCIL_OP_TO_STR(STENCIL_OP_ZERO);
        STENCIL_OP_TO_STR(STENCIL_OP_REPLACE);
        STENCIL_OP_TO_STR(STENCIL_OP_INCR_SAT);
        STENCIL_OP_TO_STR(STENCIL_OP_DECR_SAT);
        STENCIL_OP_TO_STR(STENCIL_OP_INVERT);
        STENCIL_OP_TO_STR(STENCIL_OP_INCR_WRAP);
        STENCIL_OP_TO_STR(STENCIL_OP_DECR_WRAP);

        default:
            UNEXPECTED("Unexepcted stencil operation (", static_cast<Uint32>(StencilOp), ")");
            return "UNKNOWN";
    }
#undef STENCIL_OP_TO_STR
}

const Char* GetBlendFactorLiteralName(BLEND_FACTOR BlendFactor)
{
#define BLEND_FACTOR_TO_STR(Factor) \
    case Factor: return #Factor

    switch (BlendFactor)
    {
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_UNDEFINED);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_ZERO);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_ONE);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_DEST_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_DEST_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_DEST_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_DEST_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC_ALPHA_SAT);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_BLEND_FACTOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_BLEND_FACTOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC1_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC1_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC1_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC1_ALPHA);

        default:
            UNEXPECTED("Unexepcted blend factor (", static_cast<int>(BlendFactor), ")");
            return "UNKNOWN";
    }
#undef BLEND_FACTOR_TO_STR
}

const Char* GetBlendOperationLiteralName(BLEND_OPERATION BlendOp)
{
#define BLEND_OP_TO_STR(BlendOp) \
    case BlendOp: return #BlendOp

    switch (BlendOp)
    {
        BLEND_OP_TO_STR(BLEND_OPERATION_UNDEFINED);
        BLEND_OP_TO_STR(BLEND_OPERATION_ADD);
        BLEND_OP_TO_STR(BLEND_OPERATION_SUBTRACT);
        BLEND_OP_TO_STR(BLEND_OPERATION_REV_SUBTRACT);
        BLEND_OP_TO_STR(BLEND_OPERATION_MIN);
        BLEND_OP_TO_STR(BLEND_OPERATION_MAX);

        default:
            UNEXPECTED("Unexepcted blend operation (", static_cast<int>(BlendOp), ")");
            return "UNKNOWN";
    }
#undef BLEND_OP_TO_STR
}

const Char* GetFillModeLiteralName(FILL_MODE FillMode)
{
#define FILL_MODE_TO_STR(Mode) \
    case Mode: return #Mode

    switch (FillMode)
    {
        FILL_MODE_TO_STR(FILL_MODE_UNDEFINED);
        FILL_MODE_TO_STR(FILL_MODE_WIREFRAME);
        FILL_MODE_TO_STR(FILL_MODE_SOLID);

        default:
            UNEXPECTED("Unexepcted fill mode (", static_cast<int>(FillMode), ")");
            return "UNKNOWN";
    }
#undef FILL_MODE_TO_STR
}

const Char* GetCullModeLiteralName(CULL_MODE CullMode)
{
#define CULL_MODE_TO_STR(Mode) \
    case Mode: return #Mode

    switch (CullMode)
    {
        CULL_MODE_TO_STR(CULL_MODE_UNDEFINED);
        CULL_MODE_TO_STR(CULL_MODE_NONE);
        CULL_MODE_TO_STR(CULL_MODE_FRONT);
        CULL_MODE_TO_STR(CULL_MODE_BACK);

        default:
            UNEXPECTED("Unexepcted cull mode (", static_cast<int>(CullMode), ")");
            return "UNKNOWN";
    }
#undef CULL_MODE_TO_STR
}

const Char* GetMapTypeString(MAP_TYPE MapType)
{
    switch (MapType)
    {
        case MAP_READ: return "MAP_READ";
        case MAP_WRITE: return "MAP_WRITE";
        case MAP_READ_WRITE: return "MAP_READ_WRITE";

        default:
            UNEXPECTED("Unexpected map type");
            return "Unknown map type";
    }
}

/// Returns the string containing the usage
const Char* GetUsageString(USAGE Usage)
{
    static_assert(USAGE_NUM_USAGES == 5, "Please update the function to handle the new usage type");

    static const Char* UsageStrings[USAGE_NUM_USAGES];
    static bool        bUsageStringsInit = false;
    if (!bUsageStringsInit)
    {
        // clang-format off
#define INIT_USGAGE_STR(Usage)UsageStrings[Usage] = #Usage
        INIT_USGAGE_STR(USAGE_IMMUTABLE);
        INIT_USGAGE_STR(USAGE_DEFAULT);
        INIT_USGAGE_STR(USAGE_DYNAMIC);
        INIT_USGAGE_STR(USAGE_STAGING);
        INIT_USGAGE_STR(USAGE_UNIFIED);
#undef  INIT_USGAGE_STR
        // clang-format on
        bUsageStringsInit = true;
    }
    if (Usage >= USAGE_IMMUTABLE && Usage < USAGE_NUM_USAGES)
        return UsageStrings[Usage];
    else
    {
        UNEXPECTED("Unknow usage");
        return "Unknown usage";
    }
}

const Char* GetResourceDimString(RESOURCE_DIMENSION TexType)
{
    static const Char* TexTypeStrings[RESOURCE_DIM_NUM_DIMENSIONS];
    static bool        bTexTypeStrsInit = false;
    if (!bTexTypeStrsInit)
    {
        TexTypeStrings[RESOURCE_DIM_UNDEFINED]      = "Undefined";
        TexTypeStrings[RESOURCE_DIM_BUFFER]         = "Buffer";
        TexTypeStrings[RESOURCE_DIM_TEX_1D]         = "Texture 1D";
        TexTypeStrings[RESOURCE_DIM_TEX_1D_ARRAY]   = "Texture 1D Array";
        TexTypeStrings[RESOURCE_DIM_TEX_2D]         = "Texture 2D";
        TexTypeStrings[RESOURCE_DIM_TEX_2D_ARRAY]   = "Texture 2D Array";
        TexTypeStrings[RESOURCE_DIM_TEX_3D]         = "Texture 3D";
        TexTypeStrings[RESOURCE_DIM_TEX_CUBE]       = "Texture Cube";
        TexTypeStrings[RESOURCE_DIM_TEX_CUBE_ARRAY] = "Texture Cube Array";
        static_assert(RESOURCE_DIM_NUM_DIMENSIONS == RESOURCE_DIM_TEX_CUBE_ARRAY + 1, "Not all texture type strings initialized.");

        bTexTypeStrsInit = true;
    }
    if (TexType >= RESOURCE_DIM_UNDEFINED && TexType < RESOURCE_DIM_NUM_DIMENSIONS)
        return TexTypeStrings[TexType];
    else
    {
        UNEXPECTED("Unknow texture type");
        return "Unknow texture type";
    }
}

const Char* GetBindFlagString(Uint32 BindFlag)
{
    VERIFY((BindFlag & (BindFlag - 1)) == 0, "More than one bind flag specified");
    switch (BindFlag)
    {
        // clang-format off
#define BIND_FLAG_STR_CASE(Flag) case Flag: return #Flag;
        BIND_FLAG_STR_CASE( BIND_VERTEX_BUFFER )
	    BIND_FLAG_STR_CASE( BIND_INDEX_BUFFER  )
	    BIND_FLAG_STR_CASE( BIND_UNIFORM_BUFFER  )
	    BIND_FLAG_STR_CASE( BIND_SHADER_RESOURCE )
	    BIND_FLAG_STR_CASE( BIND_STREAM_OUTPUT )
	    BIND_FLAG_STR_CASE( BIND_RENDER_TARGET )
	    BIND_FLAG_STR_CASE( BIND_DEPTH_STENCIL )
	    BIND_FLAG_STR_CASE( BIND_UNORDERED_ACCESS )
        BIND_FLAG_STR_CASE( BIND_INDIRECT_DRAW_ARGS )
#undef  BIND_FLAG_STR_CASE
        // clang-format on
        default: UNEXPECTED("Unexpected bind flag ", BindFlag); return "";
    }
}

String GetBindFlagsString(Uint32 BindFlags, const char* Delimeter)
{
    if (BindFlags == 0)
        return "0";
    String Str;
    for (Uint32 Flag = BIND_VERTEX_BUFFER; BindFlags && Flag <= BIND_INDIRECT_DRAW_ARGS; Flag <<= 1)
    {
        if (BindFlags & Flag)
        {
            if (!Str.empty())
                Str += Delimeter;
            Str += GetBindFlagString(Flag);
            BindFlags &= ~Flag;
        }
    }
    VERIFY(BindFlags == 0, "Unknown bind flags left");
    return Str;
}


static const Char* GetSingleCPUAccessFlagString(Uint32 CPUAccessFlag)
{
    VERIFY((CPUAccessFlag & (CPUAccessFlag - 1)) == 0, "More than one access flag specified");
    switch (CPUAccessFlag)
    {
        // clang-format off
#define CPU_ACCESS_FLAG_STR_CASE(Flag) case Flag: return #Flag;
        CPU_ACCESS_FLAG_STR_CASE( CPU_ACCESS_READ )
	    CPU_ACCESS_FLAG_STR_CASE( CPU_ACCESS_WRITE  )
#undef  CPU_ACCESS_FLAG_STR_CASE
        // clang-format on
        default: UNEXPECTED("Unexpected CPU access flag ", CPUAccessFlag); return "";
    }
}

String GetCPUAccessFlagsString(Uint32 CpuAccessFlags)
{
    if (CpuAccessFlags == 0)
        return "0";
    String Str;
    for (Uint32 Flag = CPU_ACCESS_READ; CpuAccessFlags && Flag <= CPU_ACCESS_WRITE; Flag <<= 1)
    {
        if (CpuAccessFlags & Flag)
        {
            if (Str.length())
                Str += '|';
            Str += GetSingleCPUAccessFlagString(Flag);
            CpuAccessFlags &= ~Flag;
        }
    }
    VERIFY(CpuAccessFlags == 0, "Unknown CPU access flags left");
    return Str;
}

// There is a nice standard function to_string, but there is a bug on gcc
// and it does not work
template <typename T>
String ToString(T Val)
{
    std::stringstream ss;
    ss << Val;
    return ss.str();
}

String GetTextureDescString(const TextureDesc& Desc)
{
    String Str = "Type: ";
    Str += GetResourceDimString(Desc.Type);
    Str += "; size: ";
    Str += ToString(Desc.Width);
    if (Desc.Type == RESOURCE_DIM_TEX_2D || Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY || Desc.Type == RESOURCE_DIM_TEX_3D || Desc.Type == RESOURCE_DIM_TEX_CUBE || Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        Str += "x";
        Str += ToString(Desc.Height);
    }

    if (Desc.Type == RESOURCE_DIM_TEX_3D)
    {
        Str += "x";
        Str += ToString(Desc.Depth);
    }

    if (Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY || Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY || Desc.Type == RESOURCE_DIM_TEX_CUBE || Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        Str += "; Num Slices: ";
        Str += ToString(Desc.ArraySize);
    }

    auto FmtName = GetTextureFormatAttribs(Desc.Format).Name;
    Str += "; Format: ";
    Str += FmtName;

    Str += "; Mip levels: ";
    Str += ToString(Desc.MipLevels);

    Str += "; Sample Count: ";
    Str += ToString(Desc.SampleCount);

    Str += "; Usage: ";
    Str += GetUsageString(Desc.Usage);

    Str += "; Bind Flags: ";
    Str += GetBindFlagsString(Desc.BindFlags);

    Str += "; CPU access: ";
    Str += GetCPUAccessFlagsString(Desc.CPUAccessFlags);

    return Str;
}

const Char* GetBufferModeString(BUFFER_MODE Mode)
{
    static const Char* BufferModeStrings[BUFFER_MODE_NUM_MODES];
    static bool        bBuffModeStringsInit = false;
    if (!bBuffModeStringsInit)
    {
        // clang-format off
#define INIT_BUFF_MODE_STR(Mode)BufferModeStrings[Mode] = #Mode
        INIT_BUFF_MODE_STR( BUFFER_MODE_UNDEFINED );
        INIT_BUFF_MODE_STR( BUFFER_MODE_FORMATTED );
        INIT_BUFF_MODE_STR( BUFFER_MODE_STRUCTURED );
        INIT_BUFF_MODE_STR( BUFFER_MODE_RAW );
#undef  INIT_BUFF_MODE_STR
        // clang-format on
        static_assert(BUFFER_MODE_NUM_MODES == BUFFER_MODE_RAW + 1, "Not all buffer mode strings initialized.");
        bBuffModeStringsInit = true;
    }
    if (Mode >= BUFFER_MODE_UNDEFINED && Mode < BUFFER_MODE_NUM_MODES)
        return BufferModeStrings[Mode];
    else
    {
        UNEXPECTED("Unknown buffer mode");
        return "Unknown buffer mode";
    }
}

String GetBufferFormatString(const BufferFormat& Fmt)
{
    String Str;
    Str += GetValueTypeString(Fmt.ValueType);
    if (Fmt.IsNormalized)
        Str += " norm";
    Str += " x ";
    Str += ToString(Uint32{Fmt.NumComponents});
    return Str;
}

String GetBufferDescString(const BufferDesc& Desc)
{
    String Str;
    Str += "Size: ";
    bool bIsLarge = false;
    if (Desc.uiSizeInBytes > (1 << 20))
    {
        Str += ToString(Desc.uiSizeInBytes / (1 << 20));
        Str += " Mb (";
        bIsLarge = true;
    }
    else if (Desc.uiSizeInBytes > (1 << 10))
    {
        Str += ToString(Desc.uiSizeInBytes / (1 << 10));
        Str += " Kb (";
        bIsLarge = true;
    }

    Str += ToString(Desc.uiSizeInBytes);
    Str += " bytes";
    if (bIsLarge)
        Str += ')';

    Str += "; Mode: ";
    Str += GetBufferModeString(Desc.Mode);

    Str += "; Usage: ";
    Str += GetUsageString(Desc.Usage);

    Str += "; Bind Flags: ";
    Str += GetBindFlagsString(Desc.BindFlags);

    Str += "; CPU access: ";
    Str += GetCPUAccessFlagsString(Desc.CPUAccessFlags);

    Str += "; stride: ";
    Str += ToString(Desc.ElementByteStride);
    Str += " bytes";

    return Str;
}

const Char* GetResourceStateFlagString(RESOURCE_STATE State)
{
    VERIFY((State & (State - 1)) == 0, "Single state is expected");
    static_assert(RESOURCE_STATE_MAX_BIT == 0x10000, "Please update this function to handle the new resource state");
    switch (State)
    {
        // clang-format off
        case RESOURCE_STATE_UNKNOWN:           return "UNKNOWN";
        case RESOURCE_STATE_UNDEFINED:         return "UNDEFINED";
        case RESOURCE_STATE_VERTEX_BUFFER:     return "VERTEX_BUFFER";
        case RESOURCE_STATE_CONSTANT_BUFFER:   return "CONSTANT_BUFFER";
        case RESOURCE_STATE_INDEX_BUFFER:      return "INDEX_BUFFER";
        case RESOURCE_STATE_RENDER_TARGET:     return "RENDER_TARGET";
        case RESOURCE_STATE_UNORDERED_ACCESS:  return "UNORDERED_ACCESS";
        case RESOURCE_STATE_DEPTH_WRITE:       return "DEPTH_WRITE";
        case RESOURCE_STATE_DEPTH_READ:        return "DEPTH_READ";
        case RESOURCE_STATE_SHADER_RESOURCE:   return "SHADER_RESOURCE";
        case RESOURCE_STATE_STREAM_OUT:        return "STREAM_OUT";
        case RESOURCE_STATE_INDIRECT_ARGUMENT: return "INDIRECT_ARGUMENT";
        case RESOURCE_STATE_COPY_DEST:         return "COPY_DEST";
        case RESOURCE_STATE_COPY_SOURCE:       return "COPY_SOURCE";
        case RESOURCE_STATE_RESOLVE_DEST:      return "RESOLVE_DEST";
        case RESOURCE_STATE_RESOLVE_SOURCE:    return "RESOLVE_SOURCE";
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return "INPUT_ATTACHMENT";
        case RESOURCE_STATE_PRESENT:           return "PRESENT";
        // clang-format on
        default:
            UNEXPECTED("Unknown resource state");
            return "UNKNOWN";
    }
}

String GetResourceStateString(RESOURCE_STATE State)
{
    if (State == RESOURCE_STATE_UNKNOWN)
        return "UNKNOWN";

    String str;
    while (State != 0)
    {
        if (!str.empty())
            str.push_back('|');

        auto lsb = State & ~(State - 1);

        const auto* StateFlagString = GetResourceStateFlagString(static_cast<RESOURCE_STATE>(lsb));
        str.append(StateFlagString);
        State = static_cast<RESOURCE_STATE>(State & ~lsb);
    }
    return str;
}

const char* GetQueryTypeString(QUERY_TYPE QueryType)
{
    // clang-format off
    switch(QueryType)
    {
        case QUERY_TYPE_UNDEFINED:           return "QUERY_TYPE_UNDEFINED";
        case QUERY_TYPE_OCCLUSION:           return "QUERY_TYPE_OCCLUSION";
        case QUERY_TYPE_BINARY_OCCLUSION:    return "QUERY_TYPE_BINARY_OCCLUSION";
        case QUERY_TYPE_TIMESTAMP:           return "QUERY_TYPE_TIMESTAMP";
        case QUERY_TYPE_PIPELINE_STATISTICS: return "QUERY_TYPE_PIPELINE_STATISTICS";
        case QUERY_TYPE_DURATION:            return "QUERY_TYPE_DURATION";

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled");

        default:
            UNEXPECTED("Unepxected query type");
            return "Unknown";
    }
    // clang-format on
}

const char* GetSurfaceTransformString(SURFACE_TRANSFORM SrfTransform)
{
    // clang-format off
    switch (SrfTransform)
    {
#define SRF_TRANSFORM_CASE(Transform) case Transform: return #Transform

        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_OPTIMAL);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_IDENTITY);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_ROTATE_90);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_ROTATE_180);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_ROTATE_270);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270);

#undef SRF_TRANSFORM_CASE

        default:
            UNEXPECTED("Unexpected surface transform");
            return "UNKNOWN";
    }
    // clang-format on
}

const char* GetPipelineTypeString(PIPELINE_TYPE PipelineType)
{
    // clang-format off
    switch (PipelineType)
    {
        case PIPELINE_TYPE_COMPUTE:  return "compute";
        case PIPELINE_TYPE_GRAPHICS: return "graphics";
        case PIPELINE_TYPE_MESH:     return "mesh";

        default:
            UNEXPECTED("Unexpected pipeline type");
            return "unknown";
    }
    // clang-format on
}

const char* GetShaderCompilerTypeString(SHADER_COMPILER Compiler)
{
    switch (Compiler)
    {
        case SHADER_COMPILER_DEFAULT: return "Default";
        case SHADER_COMPILER_GLSLANG: return "glslang";
        case SHADER_COMPILER_DXC: return "DXC";
        case SHADER_COMPILER_FXC: return "FXC";

        default:
            UNEXPECTED("Unexpected shader compiler");
            return "UNKNOWN";
    };
}

Uint32 ComputeMipLevelsCount(Uint32 Width)
{
    if (Width == 0)
        return 0;

    Uint32 MipLevels = 0;
    while ((Width >> MipLevels) > 0)
        ++MipLevels;
    VERIFY(Width >= (1U << (MipLevels - 1)) && Width < (1U << MipLevels), "Incorrect number of Mip levels");
    return MipLevels;
}

Uint32 ComputeMipLevelsCount(Uint32 Width, Uint32 Height)
{
    return ComputeMipLevelsCount(std::max(Width, Height));
}

Uint32 ComputeMipLevelsCount(Uint32 Width, Uint32 Height, Uint32 Depth)
{
    return ComputeMipLevelsCount(std::max(std::max(Width, Height), Depth));
}

bool VerifyResourceStates(RESOURCE_STATE State, bool IsTexture)
{
    static_assert(RESOURCE_STATE_MAX_BIT == 0x10000, "Please update this function to handle the new resource state");

    // clang-format off
#define VERIFY_EXCLUSIVE_STATE(ExclusiveState)\
if ( (State & ExclusiveState) != 0 && (State & ~ExclusiveState) != 0 )\
{\
    LOG_ERROR_MESSAGE("State ", GetResourceStateString(State), " is invalid: " #ExclusiveState " can't be combined with any other state");\
    return false;\
}

    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_UNDEFINED);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_UNORDERED_ACCESS);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_RENDER_TARGET);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_DEPTH_WRITE);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_COPY_DEST);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_RESOLVE_DEST);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_PRESENT);
#undef VERIFY_EXCLUSIVE_STATE
    // clang-format on

    if (IsTexture)
    {
        // clang-format off
        if (State &
            (RESOURCE_STATE_VERTEX_BUFFER   |
             RESOURCE_STATE_CONSTANT_BUFFER |
             RESOURCE_STATE_INDEX_BUFFER    |
             RESOURCE_STATE_STREAM_OUT      |
             RESOURCE_STATE_INDIRECT_ARGUMENT))
        {
            LOG_ERROR_MESSAGE("State ", GetResourceStateString(State), " is invalid: states RESOURCE_STATE_VERTEX_BUFFER, "
                              "RESOURCE_STATE_CONSTANT_BUFFER, RESOURCE_STATE_INDEX_BUFFER, RESOURCE_STATE_STREAM_OUT, "
                              "RESOURCE_STATE_INDIRECT_ARGUMENT are not applicable to textures");
            return false;
        }
        // clang-format on
    }
    else
    {
        // clang-format off
        if (State &
            (RESOURCE_STATE_RENDER_TARGET  |
             RESOURCE_STATE_DEPTH_WRITE    |
             RESOURCE_STATE_DEPTH_READ     |
             RESOURCE_STATE_RESOLVE_SOURCE |
             RESOURCE_STATE_RESOLVE_DEST   |
             RESOURCE_STATE_PRESENT        |
             RESOURCE_STATE_INPUT_ATTACHMENT))
        {
            LOG_ERROR_MESSAGE("State ", GetResourceStateString(State), " is invalid: states RESOURCE_STATE_RENDER_TARGET, "
                              "RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_RESOLVE_SOURCE, "
                              "RESOURCE_STATE_RESOLVE_DEST, RESOURCE_STATE_PRESENT, RESOURCE_STATE_INPUT_ATTACHMENT "
                              "are not applicable to buffers");
            return false;
        }
        // clang-format on
    }

    return true;
}

MipLevelProperties GetMipLevelProperties(const TextureDesc& TexDesc, Uint32 MipLevel)
{
    MipLevelProperties MipProps;
    const auto&        FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);

    MipProps.LogicalWidth  = std::max(TexDesc.Width >> MipLevel, 1u);
    MipProps.LogicalHeight = std::max(TexDesc.Height >> MipLevel, 1u);
    MipProps.Depth         = (TexDesc.Type == RESOURCE_DIM_TEX_3D) ? std::max(TexDesc.Depth >> MipLevel, 1u) : 1u;
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        VERIFY_EXPR(FmtAttribs.BlockWidth > 1 && FmtAttribs.BlockHeight > 1);
        VERIFY((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0, "Compressed block width is expected to be power of 2");
        VERIFY((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0, "Compressed block height is expected to be power of 2");
        // For block-compression formats, all parameters are still specified in texels rather than compressed texel blocks (18.4.1)
        MipProps.StorageWidth   = Align(MipProps.LogicalWidth, Uint32{FmtAttribs.BlockWidth});
        MipProps.StorageHeight  = Align(MipProps.LogicalHeight, Uint32{FmtAttribs.BlockHeight});
        MipProps.RowSize        = MipProps.StorageWidth / Uint32{FmtAttribs.BlockWidth} * Uint32{FmtAttribs.ComponentSize}; // ComponentSize is the block size
        MipProps.DepthSliceSize = MipProps.StorageHeight / Uint32{FmtAttribs.BlockHeight} * MipProps.RowSize;
        MipProps.MipSize        = MipProps.DepthSliceSize * MipProps.Depth;
    }
    else
    {
        MipProps.StorageWidth   = MipProps.LogicalWidth;
        MipProps.StorageHeight  = MipProps.LogicalHeight;
        MipProps.RowSize        = MipProps.StorageWidth * Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        MipProps.DepthSliceSize = MipProps.RowSize * MipProps.StorageHeight;
        MipProps.MipSize        = MipProps.DepthSliceSize * MipProps.Depth;
    }

    return MipProps;
}

ADAPTER_VENDOR VendorIdToAdapterVendor(Uint32 VendorId)
{
    switch (VendorId)
    {
        case 0x01002: return ADAPTER_VENDOR_AMD;
        case 0x010DE: return ADAPTER_VENDOR_NVIDIA;
        case 0x08086: return ADAPTER_VENDOR_INTEL;
        case 0x013B5: return ADAPTER_VENDOR_ARM;
        case 0x05143: return ADAPTER_VENDOR_QUALCOMM;
        case 0x01010: return ADAPTER_VENDOR_IMGTECH;
        case 0x01414: return ADAPTER_VENDOR_MSFT;

        default:
            return ADAPTER_VENDOR_UNKNOWN;
    }
}

bool IsConsistentShaderType(SHADER_TYPE ShaderType, PIPELINE_TYPE PipelineType)
{
    static_assert(SHADER_TYPE_LAST == 0x080, "Please update the switch below to handle the new shader type");
    switch (PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
            return ShaderType == SHADER_TYPE_VERTEX ||
                ShaderType == SHADER_TYPE_HULL ||
                ShaderType == SHADER_TYPE_DOMAIN ||
                ShaderType == SHADER_TYPE_GEOMETRY ||
                ShaderType == SHADER_TYPE_PIXEL;

        case PIPELINE_TYPE_COMPUTE:
            return ShaderType == SHADER_TYPE_COMPUTE;

        case PIPELINE_TYPE_MESH:
            return ShaderType == SHADER_TYPE_AMPLIFICATION ||
                ShaderType == SHADER_TYPE_MESH ||
                ShaderType == SHADER_TYPE_PIXEL;

        default:
            UNEXPECTED("Unexpected pipeline type");
            return false;
    }
}

Int32 GetShaderTypePipelineIndex(SHADER_TYPE ShaderType, PIPELINE_TYPE PipelineType)
{
    VERIFY(IsConsistentShaderType(ShaderType, PipelineType), "Shader type ", GetShaderTypeLiteralName(ShaderType),
           " is inconsistent with pipeline type ", GetPipelineTypeString(PipelineType));
    VERIFY(IsPowerOfTwo(Uint32{ShaderType}), "More than one shader type is specified");

    static_assert(SHADER_TYPE_LAST == 0x080, "Please update the switch below to handle the new shader type");
    switch (ShaderType)
    {
        case SHADER_TYPE_UNKNOWN:
            return -1;

        case SHADER_TYPE_VERTEX:        // Graphics
        case SHADER_TYPE_AMPLIFICATION: // Mesh
        case SHADER_TYPE_COMPUTE:       // Compute
            return 0;

        case SHADER_TYPE_HULL: // Graphics
        case SHADER_TYPE_MESH: // Mesh
            return 1;

        case SHADER_TYPE_DOMAIN: // Graphics
            return 2;

        case SHADER_TYPE_GEOMETRY: // Graphics
            return 3;

        case SHADER_TYPE_PIXEL: // Graphics or Mesh
            return 4;

        default:
            UNEXPECTED("Unexpected shader type (", ShaderType, ")");
            return -1;
    }
}

SHADER_TYPE GetShaderTypeFromPipelineIndex(Int32 Index, PIPELINE_TYPE PipelineType)
{
    static_assert(SHADER_TYPE_LAST == 0x080, "Please update the switch below to handle the new shader type");
    switch (PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
            switch (Index)
            {
                case 0: return SHADER_TYPE_VERTEX;
                case 1: return SHADER_TYPE_HULL;
                case 2: return SHADER_TYPE_DOMAIN;
                case 3: return SHADER_TYPE_GEOMETRY;
                case 4: return SHADER_TYPE_PIXEL;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid graphics pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        case PIPELINE_TYPE_COMPUTE:
            switch (Index)
            {
                case 0: return SHADER_TYPE_COMPUTE;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid compute pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        case PIPELINE_TYPE_MESH:
            switch (Index)
            {
                case 0: return SHADER_TYPE_AMPLIFICATION;
                case 1: return SHADER_TYPE_MESH;
                case 4: return SHADER_TYPE_PIXEL;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid mesh pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        default:
            UNEXPECTED("Unexpected pipeline type");
            return SHADER_TYPE_UNKNOWN;
    }
}


Uint32 GetStagingTextureLocationOffset(const TextureDesc& TexDesc,
                                       Uint32             ArraySlice,
                                       Uint32             MipLevel,
                                       Uint32             Alignment,
                                       Uint32             LocationX,
                                       Uint32             LocationY,
                                       Uint32             LocationZ)
{
    VERIFY_EXPR(ArraySlice < TexDesc.ArraySize && MipLevel < TexDesc.MipLevels || ArraySlice == TexDesc.ArraySize && MipLevel == 0);

    Uint32 Offset = 0;
    if (ArraySlice > 0)
    {
        Uint32 ArraySliceSize = 0;
        for (Uint32 mip = 0; mip < TexDesc.MipLevels; ++mip)
        {
            auto MipInfo = GetMipLevelProperties(TexDesc, mip);
            ArraySliceSize += Align(MipInfo.MipSize, Alignment);
        }

        Offset = ArraySliceSize;
        if (TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY ||
            TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY ||
            TexDesc.Type == RESOURCE_DIM_TEX_CUBE ||
            TexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
            Offset *= ArraySlice;
    }

    for (Uint32 mip = 0; mip < MipLevel; ++mip)
    {
        auto MipInfo = GetMipLevelProperties(TexDesc, mip);
        Offset += Align(MipInfo.MipSize, Alignment);
    }

    if (ArraySlice == TexDesc.ArraySize)
    {
        VERIFY(LocationX == 0 && LocationY == 0 && LocationZ == 0,
               "Staging buffer size is requested: location must be (0,0,0).");
    }
    else if (LocationX != 0 || LocationY != 0 || LocationZ != 0)
    {
        const auto& MipLevelAttribs = GetMipLevelProperties(TexDesc, MipLevel);
        const auto& FmtAttribs      = GetTextureFormatAttribs(TexDesc.Format);
        VERIFY(LocationX < MipLevelAttribs.LogicalWidth && LocationY < MipLevelAttribs.LogicalHeight && LocationZ < MipLevelAttribs.Depth,
               "Specified location is out of bounds");
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            VERIFY((LocationX % FmtAttribs.BlockWidth) == 0 && (LocationY % FmtAttribs.BlockHeight) == 0,
                   "For compressed texture formats, location must be a multiple of compressed block size.");
        }

        // For compressed-block formats, RowSize is the size of one compressed row.
        // For non-compressed formats, BlockHeight is 1.
        Offset += (LocationZ * MipLevelAttribs.StorageHeight + LocationY) / FmtAttribs.BlockHeight * MipLevelAttribs.RowSize;

        // For non-compressed formats, BlockWidth is 1.
        Offset += (LocationX / FmtAttribs.BlockWidth) * FmtAttribs.GetElementSize();

        // Note: this addressing complies with how Vulkan (as well as OpenGL/GLES and Metal) address
        // textures when copying data to/from buffers:
        //      address of (x,y,z) = bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
    }

    return Offset;
}


} // namespace Diligent
