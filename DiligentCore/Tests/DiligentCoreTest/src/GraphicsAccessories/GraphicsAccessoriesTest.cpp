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

#include <array>

#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(GraphicsAccessories_GraphicsAccessories, GetFilterTypeLiteralName)
{
#define TEST_FILTER_TYPE_ENUM(ENUM_VAL, ShortName)                          \
    {                                                                       \
        EXPECT_STREQ(GetFilterTypeLiteralName(ENUM_VAL, true), #ENUM_VAL);  \
        EXPECT_STREQ(GetFilterTypeLiteralName(ENUM_VAL, false), ShortName); \
    }

    // clang-format off
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_UNKNOWN,                "unknown");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_POINT,                  "point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_LINEAR,                 "linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_ANISOTROPIC,            "anisotropic");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_COMPARISON_POINT,       "comparison point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_COMPARISON_LINEAR,      "comparison linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_COMPARISON_ANISOTROPIC, "comparison anisotropic");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MINIMUM_POINT,          "minimum point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MINIMUM_LINEAR,         "minimum linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MINIMUM_ANISOTROPIC,    "minimum anisotropic");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MAXIMUM_POINT,          "maximum point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MAXIMUM_LINEAR,         "maximum linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MAXIMUM_ANISOTROPIC,    "maximum anisotropic");
    // clang-format on
#undef TEST_FILTER_TYPE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetTextureAddressModeLiteralName)
{
#define TEST_TEX_ADDRESS_MODE_ENUM(ENUM_VAL, ShortName)                             \
    {                                                                               \
        EXPECT_STREQ(GetTextureAddressModeLiteralName(ENUM_VAL, true), #ENUM_VAL);  \
        EXPECT_STREQ(GetTextureAddressModeLiteralName(ENUM_VAL, false), ShortName); \
    }

    // clang-format off
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_UNKNOWN,     "unknown");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_WRAP,        "wrap");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_MIRROR,      "mirror");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_CLAMP,       "clamp");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_BORDER,      "border");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_MIRROR_ONCE, "mirror once");
    // clang-format on
#undef TEST_TEX_ADDRESS_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetComparisonFunctionLiteralName)
{
#define TEST_COMPARISON_FUNC_ENUM(ENUM_VAL, ShortName)                              \
    {                                                                               \
        EXPECT_STREQ(GetComparisonFunctionLiteralName(ENUM_VAL, true), #ENUM_VAL);  \
        EXPECT_STREQ(GetComparisonFunctionLiteralName(ENUM_VAL, false), ShortName); \
    }

    // clang-format off
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_UNKNOWN,       "unknown");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_NEVER,         "never");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_LESS,          "less");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_EQUAL,         "equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_LESS_EQUAL,    "less equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_GREATER,       "greater");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_NOT_EQUAL,     "not equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_GREATER_EQUAL, "greater equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_ALWAYS,        "always");
    // clang-format on
#undef TEST_TEX_ADDRESS_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetBlendFactorLiteralName)
{
#define TEST_BLEND_FACTOR_ENUM(ENUM_VAL)                              \
    {                                                                 \
        EXPECT_STREQ(GetBlendFactorLiteralName(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_UNDEFINED);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_ZERO);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_ONE);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_DEST_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_DEST_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_DEST_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_DEST_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC_ALPHA_SAT);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_BLEND_FACTOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_BLEND_FACTOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC1_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC1_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC1_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC1_ALPHA);
#undef TEST_TEX_ADDRESS_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetBlendOperationLiteralName)
{
#define TEST_BLEND_OP_ENUM(ENUM_VAL)                                     \
    {                                                                    \
        EXPECT_STREQ(GetBlendOperationLiteralName(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_BLEND_OP_ENUM(BLEND_OPERATION_UNDEFINED);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_ADD);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_SUBTRACT);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_REV_SUBTRACT);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_MIN);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_MAX);
#undef TEST_BLEND_OP_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetFillModeLiteralName)
{
#define TEST_FILL_MODE_ENUM(ENUM_VAL)                              \
    {                                                              \
        EXPECT_STREQ(GetFillModeLiteralName(ENUM_VAL), #ENUM_VAL); \
    }
    TEST_FILL_MODE_ENUM(FILL_MODE_UNDEFINED);
    TEST_FILL_MODE_ENUM(FILL_MODE_WIREFRAME);
    TEST_FILL_MODE_ENUM(FILL_MODE_SOLID);
#undef TEST_FILL_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetCullModeLiteralName)
{
#define TEST_CULL_MODE_ENUM(ENUM_VAL)                              \
    {                                                              \
        EXPECT_STREQ(GetCullModeLiteralName(ENUM_VAL), #ENUM_VAL); \
    }
    TEST_CULL_MODE_ENUM(CULL_MODE_UNDEFINED);
    TEST_CULL_MODE_ENUM(CULL_MODE_NONE);
    TEST_CULL_MODE_ENUM(CULL_MODE_FRONT);
    TEST_CULL_MODE_ENUM(CULL_MODE_BACK);
#undef TEST_CULL_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetStencilOpLiteralName)
{
#define TEST_STENCIL_OP_ENUM(ENUM_VAL)                              \
    {                                                               \
        EXPECT_STREQ(GetStencilOpLiteralName(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_STENCIL_OP_ENUM(STENCIL_OP_UNDEFINED);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_KEEP);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_ZERO);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_REPLACE);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_INCR_SAT);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_DECR_SAT);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_INVERT);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_INCR_WRAP);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_DECR_WRAP);
#undef TEST_STENCIL_OP_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetQueryTypeString)
{
#define TEST_QUERY_TYPE_ENUM(ENUM_VAL)                         \
    {                                                          \
        EXPECT_STREQ(GetQueryTypeString(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_UNDEFINED);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_OCCLUSION);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_BINARY_OCCLUSION);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_TIMESTAMP);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_PIPELINE_STATISTICS);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_DURATION);
    static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are tested");
#undef TEST_QUERY_TYPE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetSurfaceTransformString)
{
#define TEST_SURFACE_TRANSFORM_ENUM(ENUM_VAL)                         \
    {                                                                 \
        EXPECT_STREQ(GetSurfaceTransformString(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_OPTIMAL);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_IDENTITY);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_ROTATE_90);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_ROTATE_180);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_ROTATE_270);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270);
#undef TEST_SURFACE_TRANSFORM_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetTextureFormatAttribs)
{
    auto CheckFormatSize = [](TEXTURE_FORMAT* begin, TEXTURE_FORMAT* end, Uint32 RefSize) //
    {
        for (auto fmt = begin; fmt != end; ++fmt)
        {
            auto FmtAttrs = GetTextureFormatAttribs(*fmt);
            EXPECT_EQ(Uint32{FmtAttrs.ComponentSize} * Uint32{FmtAttrs.NumComponents}, RefSize);
        }
    };

    auto CheckNumComponents = [](TEXTURE_FORMAT* begin, TEXTURE_FORMAT* end, Uint32 RefComponents) //
    {
        for (auto fmt = begin; fmt != end; ++fmt)
        {
            auto FmtAttrs = GetTextureFormatAttribs(*fmt);
            EXPECT_EQ(FmtAttrs.NumComponents, RefComponents);
        }
    };

    auto CheckComponentType = [](TEXTURE_FORMAT* begin, TEXTURE_FORMAT* end, COMPONENT_TYPE RefType) //
    {
        for (auto fmt = begin; fmt != end; ++fmt)
        {
            auto FmtAttrs = GetTextureFormatAttribs(*fmt);
            EXPECT_EQ(FmtAttrs.ComponentType, RefType);
        }
    };

    TEXTURE_FORMAT _16ByteFormats[] =
        {
            TEX_FORMAT_RGBA32_TYPELESS, TEX_FORMAT_RGBA32_FLOAT, TEX_FORMAT_RGBA32_UINT, TEX_FORMAT_RGBA32_SINT //
        };
    CheckFormatSize(std::begin(_16ByteFormats), std::end(_16ByteFormats), 16);

    TEXTURE_FORMAT _12ByteFormats[] =
        {
            TEX_FORMAT_RGB32_TYPELESS, TEX_FORMAT_RGB32_FLOAT, TEX_FORMAT_RGB32_UINT, TEX_FORMAT_RGB32_SINT //
        };
    CheckFormatSize(std::begin(_12ByteFormats), std::end(_12ByteFormats), 12);

    TEXTURE_FORMAT _8ByteFormats[] =
        {
            TEX_FORMAT_RGBA16_TYPELESS, TEX_FORMAT_RGBA16_FLOAT, TEX_FORMAT_RGBA16_UNORM, TEX_FORMAT_RGBA16_UINT, TEX_FORMAT_RGBA16_SNORM, TEX_FORMAT_RGBA16_SINT,
            TEX_FORMAT_RG32_TYPELESS, TEX_FORMAT_RG32_FLOAT, TEX_FORMAT_RG32_UINT, TEX_FORMAT_RG32_SINT,
            TEX_FORMAT_R32G8X24_TYPELESS, TEX_FORMAT_D32_FLOAT_S8X24_UINT, TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, TEX_FORMAT_X32_TYPELESS_G8X24_UINT //
        };
    CheckFormatSize(std::begin(_8ByteFormats), std::end(_8ByteFormats), 8);

    TEXTURE_FORMAT _4ByteFormats[] =
        {
            TEX_FORMAT_RGB10A2_TYPELESS, TEX_FORMAT_RGB10A2_UNORM, TEX_FORMAT_RGB10A2_UINT, TEX_FORMAT_R11G11B10_FLOAT,
            TEX_FORMAT_RGBA8_TYPELESS, TEX_FORMAT_RGBA8_UNORM, TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_RGBA8_UINT, TEX_FORMAT_RGBA8_SNORM, TEX_FORMAT_RGBA8_SINT,
            TEX_FORMAT_RG16_TYPELESS, TEX_FORMAT_RG16_FLOAT, TEX_FORMAT_RG16_UNORM, TEX_FORMAT_RG16_UINT, TEX_FORMAT_RG16_SNORM, TEX_FORMAT_RG16_SINT,
            TEX_FORMAT_R32_TYPELESS, TEX_FORMAT_D32_FLOAT, TEX_FORMAT_R32_FLOAT, TEX_FORMAT_R32_UINT, TEX_FORMAT_R32_SINT,
            TEX_FORMAT_R24G8_TYPELESS, TEX_FORMAT_D24_UNORM_S8_UINT, TEX_FORMAT_R24_UNORM_X8_TYPELESS, TEX_FORMAT_X24_TYPELESS_G8_UINT,
            TEX_FORMAT_RGB9E5_SHAREDEXP, TEX_FORMAT_RG8_B8G8_UNORM, TEX_FORMAT_G8R8_G8B8_UNORM,
            TEX_FORMAT_BGRA8_UNORM, TEX_FORMAT_BGRX8_UNORM, TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
            TEX_FORMAT_BGRA8_TYPELESS, TEX_FORMAT_BGRA8_UNORM_SRGB, TEX_FORMAT_BGRX8_TYPELESS, TEX_FORMAT_BGRX8_UNORM_SRGB //
        };
    CheckFormatSize(std::begin(_4ByteFormats), std::end(_4ByteFormats), 4);

    TEXTURE_FORMAT _2ByteFormats[] =
        {
            TEX_FORMAT_RG8_TYPELESS, TEX_FORMAT_RG8_UNORM, TEX_FORMAT_RG8_UINT, TEX_FORMAT_RG8_SNORM, TEX_FORMAT_RG8_SINT,
            TEX_FORMAT_R16_TYPELESS, TEX_FORMAT_R16_FLOAT, TEX_FORMAT_D16_UNORM, TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UINT, TEX_FORMAT_R16_SNORM, TEX_FORMAT_R16_SINT,
            TEX_FORMAT_B5G6R5_UNORM, TEX_FORMAT_B5G5R5A1_UNORM //
        };
    CheckFormatSize(std::begin(_2ByteFormats), std::end(_2ByteFormats), 2);

    TEXTURE_FORMAT _1ByteFormats[] =
        {
            TEX_FORMAT_R8_TYPELESS, TEX_FORMAT_R8_UNORM, TEX_FORMAT_R8_UINT, TEX_FORMAT_R8_SNORM, TEX_FORMAT_R8_SINT, TEX_FORMAT_A8_UNORM, //TEX_FORMAT_R1_UNORM
        };
    CheckFormatSize(std::begin(_1ByteFormats), std::end(_1ByteFormats), 1);

    TEXTURE_FORMAT _4ComponentFormats[] =
        {
            TEX_FORMAT_RGBA32_TYPELESS, TEX_FORMAT_RGBA32_FLOAT, TEX_FORMAT_RGBA32_UINT, TEX_FORMAT_RGBA32_SINT,
            TEX_FORMAT_RGBA16_TYPELESS, TEX_FORMAT_RGBA16_FLOAT, TEX_FORMAT_RGBA16_UNORM, TEX_FORMAT_RGBA16_UINT, TEX_FORMAT_RGBA16_SNORM, TEX_FORMAT_RGBA16_SINT,
            TEX_FORMAT_RGBA8_TYPELESS, TEX_FORMAT_RGBA8_UNORM, TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_RGBA8_UINT, TEX_FORMAT_RGBA8_SNORM, TEX_FORMAT_RGBA8_SINT,
            TEX_FORMAT_RG8_B8G8_UNORM, TEX_FORMAT_G8R8_G8B8_UNORM,
            TEX_FORMAT_BGRA8_UNORM, TEX_FORMAT_BGRX8_UNORM, TEX_FORMAT_BGRA8_TYPELESS, TEX_FORMAT_BGRA8_UNORM_SRGB, TEX_FORMAT_BGRX8_TYPELESS, TEX_FORMAT_BGRX8_UNORM_SRGB //
        };
    CheckNumComponents(std::begin(_4ComponentFormats), std::end(_4ComponentFormats), 4);

    TEXTURE_FORMAT _3ComponentFormats[] =
        {
            TEX_FORMAT_RGB32_TYPELESS,
            TEX_FORMAT_RGB32_FLOAT,
            TEX_FORMAT_RGB32_UINT,
            TEX_FORMAT_RGB32_SINT //
        };
    CheckNumComponents(std::begin(_3ComponentFormats), std::end(_3ComponentFormats), 3);

    TEXTURE_FORMAT _2ComponentFormats[] =
        {
            TEX_FORMAT_RG32_TYPELESS, TEX_FORMAT_RG32_FLOAT, TEX_FORMAT_RG32_UINT, TEX_FORMAT_RG32_SINT,
            TEX_FORMAT_R32G8X24_TYPELESS, TEX_FORMAT_D32_FLOAT_S8X24_UINT, TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, TEX_FORMAT_X32_TYPELESS_G8X24_UINT,
            TEX_FORMAT_RG16_TYPELESS, TEX_FORMAT_RG16_FLOAT, TEX_FORMAT_RG16_UNORM, TEX_FORMAT_RG16_UINT, TEX_FORMAT_RG16_SNORM, TEX_FORMAT_RG16_SINT,
            TEX_FORMAT_RG8_TYPELESS, TEX_FORMAT_RG8_UNORM, TEX_FORMAT_RG8_UINT, TEX_FORMAT_RG8_SNORM, TEX_FORMAT_RG8_SINT //
        };
    CheckNumComponents(std::begin(_2ComponentFormats), std::end(_2ComponentFormats), 2);

    TEXTURE_FORMAT _1ComponentFormats[] =
        {
            TEX_FORMAT_RGB10A2_TYPELESS, TEX_FORMAT_RGB10A2_UNORM, TEX_FORMAT_RGB10A2_UINT, TEX_FORMAT_R11G11B10_FLOAT,
            TEX_FORMAT_R32_TYPELESS, TEX_FORMAT_D32_FLOAT, TEX_FORMAT_R32_FLOAT, TEX_FORMAT_R32_UINT, TEX_FORMAT_R32_SINT,
            TEX_FORMAT_R24G8_TYPELESS, TEX_FORMAT_D24_UNORM_S8_UINT, TEX_FORMAT_R24_UNORM_X8_TYPELESS, TEX_FORMAT_X24_TYPELESS_G8_UINT,
            TEX_FORMAT_R16_TYPELESS, TEX_FORMAT_R16_FLOAT, TEX_FORMAT_D16_UNORM, TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UINT, TEX_FORMAT_R16_SNORM, TEX_FORMAT_R16_SINT,
            TEX_FORMAT_R8_TYPELESS, TEX_FORMAT_R8_UNORM, TEX_FORMAT_R8_UINT, TEX_FORMAT_R8_SNORM, TEX_FORMAT_R8_SINT, TEX_FORMAT_A8_UNORM, //TEX_FORMAT_R1_UNORM
            TEX_FORMAT_RGB9E5_SHAREDEXP,
            TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, TEX_FORMAT_B5G6R5_UNORM, TEX_FORMAT_B5G5R5A1_UNORM //
        };
    CheckNumComponents(std::begin(_1ComponentFormats), std::end(_1ComponentFormats), 1);


    TEXTURE_FORMAT FloatFormats[] =
        {
            TEX_FORMAT_RGBA32_FLOAT,
            TEX_FORMAT_RGB32_FLOAT,
            TEX_FORMAT_RGBA16_FLOAT,
            TEX_FORMAT_RG32_FLOAT,
            TEX_FORMAT_RG16_FLOAT,
            TEX_FORMAT_R32_FLOAT,
            TEX_FORMAT_R16_FLOAT //
        };
    CheckComponentType(std::begin(FloatFormats), std::end(FloatFormats), COMPONENT_TYPE_FLOAT);

    TEXTURE_FORMAT SintFormats[] =
        {
            TEX_FORMAT_RGBA32_SINT,
            TEX_FORMAT_RGB32_SINT,
            TEX_FORMAT_RGBA16_SINT,
            TEX_FORMAT_RG32_SINT,
            TEX_FORMAT_RGBA8_SINT,
            TEX_FORMAT_RG16_SINT,
            TEX_FORMAT_R32_SINT,
            TEX_FORMAT_RG8_SINT,
            TEX_FORMAT_R16_SINT,
            TEX_FORMAT_R8_SINT //
        };
    CheckComponentType(std::begin(SintFormats), std::end(SintFormats), COMPONENT_TYPE_SINT);

    TEXTURE_FORMAT UintFormats[] =
        {
            TEX_FORMAT_RGBA32_UINT,
            TEX_FORMAT_RGB32_UINT,
            TEX_FORMAT_RGBA16_UINT,
            TEX_FORMAT_RG32_UINT,
            TEX_FORMAT_RGBA8_UINT,
            TEX_FORMAT_RG16_UINT,
            TEX_FORMAT_R32_UINT,
            TEX_FORMAT_RG8_UINT,
            TEX_FORMAT_R16_UINT,
            TEX_FORMAT_R8_UINT //
        };
    CheckComponentType(std::begin(UintFormats), std::end(UintFormats), COMPONENT_TYPE_UINT);

    TEXTURE_FORMAT UnormFormats[] =
        {
            TEX_FORMAT_RGBA16_UNORM,
            TEX_FORMAT_RGBA8_UNORM,
            TEX_FORMAT_RG16_UNORM,
            TEX_FORMAT_RG8_UNORM,
            TEX_FORMAT_R16_UNORM,
            TEX_FORMAT_R8_UNORM,
            TEX_FORMAT_A8_UNORM,
            TEX_FORMAT_R1_UNORM,
            TEX_FORMAT_RG8_B8G8_UNORM,
            TEX_FORMAT_G8R8_G8B8_UNORM,
            TEX_FORMAT_BGRA8_UNORM,
            TEX_FORMAT_BGRX8_UNORM //
        };
    CheckComponentType(std::begin(UnormFormats), std::end(UnormFormats), COMPONENT_TYPE_UNORM);

    TEXTURE_FORMAT UnormSRGBFormats[] =
        {
            TEX_FORMAT_RGBA8_UNORM_SRGB,
            TEX_FORMAT_BGRA8_UNORM_SRGB,
            TEX_FORMAT_BGRX8_UNORM_SRGB //
        };
    CheckComponentType(std::begin(UnormSRGBFormats), std::end(UnormSRGBFormats), COMPONENT_TYPE_UNORM_SRGB);

    TEXTURE_FORMAT SnormFormats[] =
        {
            TEX_FORMAT_RGBA16_SNORM,
            TEX_FORMAT_RGBA8_SNORM,
            TEX_FORMAT_RG16_SNORM,
            TEX_FORMAT_RG8_SNORM,
            TEX_FORMAT_R16_SNORM,
            TEX_FORMAT_R8_SNORM //
        };
    CheckComponentType(std::begin(SnormFormats), std::end(SnormFormats), COMPONENT_TYPE_SNORM);

    TEXTURE_FORMAT UndefinedFormats[] =
        {
            TEX_FORMAT_RGBA32_TYPELESS,
            TEX_FORMAT_RGB32_TYPELESS,
            TEX_FORMAT_RGBA16_TYPELESS,
            TEX_FORMAT_RG32_TYPELESS,
            TEX_FORMAT_RGBA8_TYPELESS,
            TEX_FORMAT_RG16_TYPELESS,
            TEX_FORMAT_R32_TYPELESS,
            TEX_FORMAT_RG8_TYPELESS,
            TEX_FORMAT_R16_TYPELESS,
            TEX_FORMAT_R8_TYPELESS,
            TEX_FORMAT_BGRA8_TYPELESS,
            TEX_FORMAT_BGRX8_TYPELESS //
        };
    CheckComponentType(std::begin(UndefinedFormats), std::end(UndefinedFormats), COMPONENT_TYPE_UNDEFINED);

    TEXTURE_FORMAT DepthFormats[] =
        {
            TEX_FORMAT_D32_FLOAT,
            TEX_FORMAT_D16_UNORM //
        };
    CheckComponentType(std::begin(DepthFormats), std::end(DepthFormats), COMPONENT_TYPE_DEPTH);

    TEXTURE_FORMAT DepthStencilFormats[] =
        {
            TEX_FORMAT_R32G8X24_TYPELESS,
            TEX_FORMAT_D32_FLOAT_S8X24_UINT,
            TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,
            TEX_FORMAT_X32_TYPELESS_G8X24_UINT,
            TEX_FORMAT_R24G8_TYPELESS,
            TEX_FORMAT_D24_UNORM_S8_UINT,
            TEX_FORMAT_R24_UNORM_X8_TYPELESS,
            TEX_FORMAT_X24_TYPELESS_G8_UINT //
        };
    CheckComponentType(std::begin(DepthStencilFormats), std::end(DepthStencilFormats), COMPONENT_TYPE_DEPTH_STENCIL);

    TEXTURE_FORMAT CompoundFormats[] =
        {
            TEX_FORMAT_RGB10A2_TYPELESS,
            TEX_FORMAT_RGB10A2_UNORM,
            TEX_FORMAT_RGB10A2_UINT,
            TEX_FORMAT_RGB9E5_SHAREDEXP,
            TEX_FORMAT_R11G11B10_FLOAT,
            TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
            TEX_FORMAT_B5G6R5_UNORM,
            TEX_FORMAT_B5G5R5A1_UNORM //
        };
    CheckComponentType(std::begin(CompoundFormats), std::end(CompoundFormats), COMPONENT_TYPE_COMPOUND);


    TEXTURE_FORMAT CompressedFormats[] =
        {
            TEX_FORMAT_BC1_TYPELESS, TEX_FORMAT_BC1_UNORM, TEX_FORMAT_BC1_UNORM_SRGB,
            TEX_FORMAT_BC2_TYPELESS, TEX_FORMAT_BC2_UNORM, TEX_FORMAT_BC2_UNORM_SRGB,
            TEX_FORMAT_BC3_TYPELESS, TEX_FORMAT_BC3_UNORM, TEX_FORMAT_BC3_UNORM_SRGB,
            TEX_FORMAT_BC4_TYPELESS, TEX_FORMAT_BC4_UNORM, TEX_FORMAT_BC4_SNORM,
            TEX_FORMAT_BC5_TYPELESS, TEX_FORMAT_BC5_UNORM, TEX_FORMAT_BC5_SNORM,
            TEX_FORMAT_BC6H_TYPELESS, TEX_FORMAT_BC6H_UF16, TEX_FORMAT_BC6H_SF16,
            TEX_FORMAT_BC7_TYPELESS, TEX_FORMAT_BC7_UNORM, TEX_FORMAT_BC7_UNORM_SRGB //
        };
    CheckComponentType(std::begin(CompressedFormats), std::end(CompressedFormats), COMPONENT_TYPE_COMPRESSED);
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypeIndex)
{
    static_assert(SHADER_TYPE_LAST == 0x080, "Please update the test below to handle the new shader type");

    // clang-format off
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_UNKNOWN),          -1);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_VERTEX),        VSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_PIXEL),         PSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_GEOMETRY),      GSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_HULL),          HSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_DOMAIN),        DSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_COMPUTE),       CSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_AMPLIFICATION), ASInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_MESH),          MSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_LAST),          LastShaderInd);
    // clang-format on

    for (Int32 i = 0; i <= LastShaderInd; ++i)
    {
        auto ShaderType = static_cast<SHADER_TYPE>(1 << i);
        EXPECT_EQ(GetShaderTypeIndex(ShaderType), i);
    }
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypeFromIndex)
{
    static_assert(SHADER_TYPE_LAST == 0x080, "Please update the test below to handle the new shader type");

    EXPECT_EQ(GetShaderTypeFromIndex(VSInd), SHADER_TYPE_VERTEX);
    EXPECT_EQ(GetShaderTypeFromIndex(PSInd), SHADER_TYPE_PIXEL);
    EXPECT_EQ(GetShaderTypeFromIndex(GSInd), SHADER_TYPE_GEOMETRY);
    EXPECT_EQ(GetShaderTypeFromIndex(HSInd), SHADER_TYPE_HULL);
    EXPECT_EQ(GetShaderTypeFromIndex(DSInd), SHADER_TYPE_DOMAIN);
    EXPECT_EQ(GetShaderTypeFromIndex(CSInd), SHADER_TYPE_COMPUTE);
    EXPECT_EQ(GetShaderTypeFromIndex(ASInd), SHADER_TYPE_AMPLIFICATION);
    EXPECT_EQ(GetShaderTypeFromIndex(MSInd), SHADER_TYPE_MESH);

    EXPECT_EQ(GetShaderTypeFromIndex(LastShaderInd), SHADER_TYPE_LAST);

    for (Int32 i = 0; i <= LastShaderInd; ++i)
    {
        auto ShaderType = static_cast<SHADER_TYPE>(1 << i);
        EXPECT_EQ(GetShaderTypeFromIndex(i), ShaderType);
    }
}

TEST(GraphicsAccessories_GraphicsAccessories, IsConsistentShaderType)
{
    {
        std::array<bool, LastShaderInd + 1> ValidGraphicsStages;
        ValidGraphicsStages.fill(false);
        ValidGraphicsStages[VSInd] = true;
        ValidGraphicsStages[HSInd] = true;
        ValidGraphicsStages[DSInd] = true;
        ValidGraphicsStages[GSInd] = true;
        ValidGraphicsStages[PSInd] = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_GRAPHICS), ValidGraphicsStages[i]);
        }
    }

    {
        std::array<bool, LastShaderInd + 1> ValidComputeStages;
        ValidComputeStages.fill(false);
        ValidComputeStages[CSInd] = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_COMPUTE), ValidComputeStages[i]);
        }
    }

    {
        std::array<bool, LastShaderInd + 1> ValidMeshStages;
        ValidMeshStages.fill(false);
        ValidMeshStages[ASInd] = true;
        ValidMeshStages[MSInd] = true;
        ValidMeshStages[PSInd] = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_MESH), ValidMeshStages[i]);
        }
    }
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypePipelineIndex)
{
    auto TestPipelineType = [](PIPELINE_TYPE PipelineType) {
        std::array<Int32, MAX_SHADERS_IN_PIPELINE> IndexUsed;
        IndexUsed.fill(false);

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            if (IsConsistentShaderType(ShaderType, PipelineType))
            {
                auto Index = GetShaderTypePipelineIndex(ShaderType, PipelineType);
                ASSERT_LE(Index, static_cast<Int32>(MAX_SHADERS_IN_PIPELINE)) << " shader pipeline type index " << Index << " is out of range";
                EXPECT_FALSE(IndexUsed[Index]) << " shader pipeline type index " << Index << " is already used for another shader stage";
                IndexUsed[Index] = true;
            }
        }
    };

    TestPipelineType(PIPELINE_TYPE_GRAPHICS);
    TestPipelineType(PIPELINE_TYPE_COMPUTE);
    TestPipelineType(PIPELINE_TYPE_MESH);
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypeFromPipelineIndex)
{
    auto TestPipelineType = [](PIPELINE_TYPE PipelineType) {
        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            if (IsConsistentShaderType(ShaderType, PipelineType))
            {
                auto Index = GetShaderTypePipelineIndex(ShaderType, PipelineType);
                EXPECT_EQ(GetShaderTypeFromPipelineIndex(Index, PipelineType), ShaderType);
            }
        }
    };

    TestPipelineType(PIPELINE_TYPE_GRAPHICS);
    TestPipelineType(PIPELINE_TYPE_COMPUTE);
    TestPipelineType(PIPELINE_TYPE_MESH);
}

} // namespace
