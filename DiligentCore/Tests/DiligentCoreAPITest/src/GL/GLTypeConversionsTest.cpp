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

#include "../../include/GL/TestingEnvironmentGL.hpp"
#include "../include/GLTypeConversions.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(GLTypeConversionsTest, PrimitiveTopologyToGLTopology)
{
#define CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "PrimitiveTopologyToGLTopology function assumes that " #EnumElement " equals " #ExpectedValue ": fix PrimTopologyToGLTopologyMap array and update this assertion.")
    // clang-format off
    CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE(PRIMITIVE_TOPOLOGY_UNDEFINED,      0);
    CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE(PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  1);
    CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE(PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 2);
    CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE(PRIMITIVE_TOPOLOGY_POINT_LIST,     3);
    CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE(PRIMITIVE_TOPOLOGY_LINE_LIST,      4);
    CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE(PRIMITIVE_TOPOLOGY_LINE_STRIP,     5);
    static_assert(PRIMITIVE_TOPOLOGY_LINE_STRIP + 1 == PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,
                  "Did you add a new PRIMITIVE_TOPOLOGY enum value that is not patch? Please update PrimitiveTopologyToGLTopology function.");
    // clang-format on
#undef CHECK_PRIMITIVE_TOPOLOGY_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(PrimitiveTopologyToGLTopology(PRIMITIVE_TOPOLOGY_UNDEFINED),      GLenum{0});
    EXPECT_EQ(PrimitiveTopologyToGLTopology(PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),  GLenum{GL_TRIANGLES});
    EXPECT_EQ(PrimitiveTopologyToGLTopology(PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP), GLenum{GL_TRIANGLE_STRIP});
    EXPECT_EQ(PrimitiveTopologyToGLTopology(PRIMITIVE_TOPOLOGY_POINT_LIST),     GLenum{GL_POINTS});
    EXPECT_EQ(PrimitiveTopologyToGLTopology(PRIMITIVE_TOPOLOGY_LINE_LIST),      GLenum{GL_LINES});
    EXPECT_EQ(PrimitiveTopologyToGLTopology(PRIMITIVE_TOPOLOGY_LINE_STRIP),     GLenum{GL_LINE_STRIP});
    // clang-format on
}

TEST(GLTypeConversionsTest, TypeToGLType)
{
#define CHECK_VALUE_TYPE_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "TypeToGLType function assumes that " #EnumElement " equals " #ExpectedValue ": fix TypeToGLTypeMap array and update this assertion.")
    // clang-format off
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_UNDEFINED, 0);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_INT8,      1);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_INT16,     2);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_INT32,     3);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_UINT8,     4);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_UINT16,    5);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_UINT32,    6);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_FLOAT16,   7);
    CHECK_VALUE_TYPE_ENUM_VALUE(VT_FLOAT32,   8);
    // clang-format on
    static_assert(VT_NUM_TYPES == 9, "Did you add a new VALUE_TYPE enum value? Please update TypeToGLType function.");
#undef CHECK_VALUE_TYPE_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(TypeToGLType(VT_UNDEFINED), GLenum{0});
    EXPECT_EQ(TypeToGLType(VT_INT8),      GLenum{GL_BYTE});
    EXPECT_EQ(TypeToGLType(VT_INT16),     GLenum{GL_SHORT});
    EXPECT_EQ(TypeToGLType(VT_INT32),     GLenum{GL_INT});
    EXPECT_EQ(TypeToGLType(VT_UINT8),     GLenum{GL_UNSIGNED_BYTE});
    EXPECT_EQ(TypeToGLType(VT_UINT16),    GLenum{GL_UNSIGNED_SHORT});
    EXPECT_EQ(TypeToGLType(VT_UINT32),    GLenum{GL_UNSIGNED_INT});
    EXPECT_EQ(TypeToGLType(VT_FLOAT16),   GLenum{GL_HALF_FLOAT});
    EXPECT_EQ(TypeToGLType(VT_FLOAT32),   GLenum{GL_FLOAT});
    // clang-format on
}

TEST(GLTypeConversionsTest, TexAddressModeToGLAddressMode)
{
#define CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "TexAddressModeToGLAddressMode function assumes that " #EnumElement " equals " #ExpectedValue ": fix TexAddressModeToGLAddressModeMap array and update this assertion.")
    // clang-format off
    CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE(TEXTURE_ADDRESS_UNKNOWN,     0);
    CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE(TEXTURE_ADDRESS_WRAP,        1);
    CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE(TEXTURE_ADDRESS_MIRROR,      2);
    CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE(TEXTURE_ADDRESS_CLAMP,       3);
    CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE(TEXTURE_ADDRESS_BORDER,      4);
    CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE(TEXTURE_ADDRESS_MIRROR_ONCE, 5);
    // clang-format on
    static_assert(TEXTURE_ADDRESS_NUM_MODES == 6,
                  "Did you add a new TEXTURE_ADDRESS_MODE enum value? Please update TexAddressModeToGLAddressMode function.");
#undef CHECK_TEXTURE_ADDRESS_MODE_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(TexAddressModeToGLAddressMode(TEXTURE_ADDRESS_UNKNOWN),     GLenum{0});
    EXPECT_EQ(TexAddressModeToGLAddressMode(TEXTURE_ADDRESS_WRAP),        GLenum{GL_REPEAT});
    EXPECT_EQ(TexAddressModeToGLAddressMode(TEXTURE_ADDRESS_MIRROR),      GLenum{GL_MIRRORED_REPEAT});
    EXPECT_EQ(TexAddressModeToGLAddressMode(TEXTURE_ADDRESS_CLAMP),       GLenum{GL_CLAMP_TO_EDGE});
    EXPECT_EQ(TexAddressModeToGLAddressMode(TEXTURE_ADDRESS_BORDER),      GLenum{GL_CLAMP_TO_BORDER});
    EXPECT_EQ(TexAddressModeToGLAddressMode(TEXTURE_ADDRESS_MIRROR_ONCE), GLenum{GL_MIRROR_CLAMP_TO_EDGE});
    // clang-format on
}

TEST(GLTypeConversionsTest, CompareFuncToGLCompareFunc)
{
#define CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "CompareFuncToGLCompareFunc function assumes that " #EnumElement " equals " #ExpectedValue ": fix CompareFuncToGLCompareFuncMap array and update this assertion.")
    // clang-format off
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_UNKNOWN,       0);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_NEVER,         1);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_LESS,          2);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_EQUAL,         3);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_LESS_EQUAL,    4);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_GREATER,       5);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_NOT_EQUAL,     6);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_GREATER_EQUAL, 7);
    CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE(COMPARISON_FUNC_ALWAYS,        8);
    // clang-format on
    static_assert(COMPARISON_FUNC_NUM_FUNCTIONS == 9,
                  "Did you add a new COMPARISON_FUNCTION enum value? Please update CompareFuncToGLCompareFunc function.");
#undef CHECK_COMPARISON_FUNC_MODE_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_UNKNOWN),       GLenum{0});
    EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_NEVER),         GLenum{GL_NEVER});
	EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_LESS),          GLenum{GL_LESS});
	EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_EQUAL),         GLenum{GL_EQUAL});
	EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_LESS_EQUAL),    GLenum{GL_LEQUAL});
	EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_GREATER),       GLenum{GL_GREATER});
	EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_NOT_EQUAL),     GLenum{GL_NOTEQUAL});
	EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_GREATER_EQUAL), GLenum{GL_GEQUAL});
	EXPECT_EQ(CompareFuncToGLCompareFunc(COMPARISON_FUNC_ALWAYS),        GLenum{GL_ALWAYS});
    // clang-format on
}


TEST(GLTypeConversionsTest, AccessFlags2GLAccess)
{
#define CHECK_UAV_ACCESS_FLAG_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "AccessFlags2GLAccess function assumes that " #EnumElement " equals " #ExpectedValue ": fix AccessFlags2GLAccessMap array and update this assertion.")
    // clang-format off
    CHECK_UAV_ACCESS_FLAG_ENUM_VALUE(UAV_ACCESS_UNSPECIFIED,     0);
    CHECK_UAV_ACCESS_FLAG_ENUM_VALUE(UAV_ACCESS_FLAG_READ,       1);
    CHECK_UAV_ACCESS_FLAG_ENUM_VALUE(UAV_ACCESS_FLAG_WRITE,      2);
    CHECK_UAV_ACCESS_FLAG_ENUM_VALUE(UAV_ACCESS_FLAG_READ_WRITE, 3);
    // clang-format on
#undef CHECK_UAV_ACCESS_FLAG_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(AccessFlags2GLAccess(UAV_ACCESS_UNSPECIFIED),     GLenum{0});
    EXPECT_EQ(AccessFlags2GLAccess(UAV_ACCESS_FLAG_READ),       GLenum{GL_READ_ONLY});
    EXPECT_EQ(AccessFlags2GLAccess(UAV_ACCESS_FLAG_WRITE),      GLenum{GL_WRITE_ONLY});
    EXPECT_EQ(AccessFlags2GLAccess(UAV_ACCESS_FLAG_READ_WRITE), GLenum{GL_READ_WRITE});
    // clang-format on
}

TEST(GLTypeConversionsTest, StencilOp2GlStencilOp)
{
#define CHECK_STENCIL_OP_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "StencilOp2GlStencilOp function assumes that " #EnumElement " equals " #ExpectedValue ": fix StencilOp2GlStencilOpMap array and update this assertion.")
    // clang-format off
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_UNDEFINED,  0);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_KEEP,       1);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_ZERO,       2);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_REPLACE,    3);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_INCR_SAT,   4);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_DECR_SAT,   5);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_INVERT,     6);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_INCR_WRAP,  7);
    CHECK_STENCIL_OP_ENUM_VALUE(STENCIL_OP_DECR_WRAP,  8);
    // clang-format on
    static_assert(STENCIL_OP_NUM_OPS == 9, "Did you add a new STENCIL_OP enum value? Please update StencilOp2GlStencilOp function.");
#undef CHECK_STENCIL_OP_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_UNDEFINED), GLenum{0});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_KEEP),      GLenum{GL_KEEP});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_ZERO),      GLenum{GL_ZERO});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_REPLACE),   GLenum{GL_REPLACE});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_INCR_SAT),  GLenum{GL_INCR});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_DECR_SAT),  GLenum{GL_DECR});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_INVERT),    GLenum{GL_INVERT});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_INCR_WRAP), GLenum{GL_INCR_WRAP});
    EXPECT_EQ(StencilOp2GlStencilOp(STENCIL_OP_DECR_WRAP), GLenum{GL_DECR_WRAP});
    // clang-format on
}

TEST(GLTypeConversionsTest, BlendFactor2GLBlend)
{
#define CHECK_BLEND_FACTOR_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "BlendFactor2GLBlend function assumes that " #EnumElement " equals " #ExpectedValue ": fix BlendFactor2GLBlendMap array and update this assertion.")
    // clang-format off
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_UNDEFINED,         0);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_ZERO,              1);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_ONE,               2);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_SRC_COLOR,         3);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_INV_SRC_COLOR,     4);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_SRC_ALPHA,         5);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_INV_SRC_ALPHA,     6);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_DEST_ALPHA,        7);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_INV_DEST_ALPHA,    8);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_DEST_COLOR,        9);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_INV_DEST_COLOR,   10);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_SRC_ALPHA_SAT,    11);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_BLEND_FACTOR,     12);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_INV_BLEND_FACTOR, 13);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_SRC1_COLOR,       14);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_INV_SRC1_COLOR,   15);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_SRC1_ALPHA,       16);
    CHECK_BLEND_FACTOR_ENUM_VALUE(BLEND_FACTOR_INV_SRC1_ALPHA,   17);
    // clang-format on
    static_assert(BLEND_FACTOR_NUM_FACTORS == 18, "Did you add a new BLEND_FACTOR enum value? Please update BlendFactor2GLBlend function.");
#undef CHECK_BLEND_FACTOR_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_UNDEFINED),         GLenum{0});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_ONE),               GLenum{GL_ONE});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_SRC_COLOR),         GLenum{GL_SRC_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_INV_SRC_COLOR),     GLenum{GL_ONE_MINUS_SRC_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_SRC_ALPHA),         GLenum{GL_SRC_ALPHA});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_INV_SRC_ALPHA),     GLenum{GL_ONE_MINUS_SRC_ALPHA});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_DEST_ALPHA),        GLenum{GL_DST_ALPHA});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_INV_DEST_ALPHA),    GLenum{GL_ONE_MINUS_DST_ALPHA});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_DEST_COLOR),        GLenum{GL_DST_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_INV_DEST_COLOR),    GLenum{GL_ONE_MINUS_DST_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_SRC_ALPHA_SAT),     GLenum{GL_SRC_ALPHA_SATURATE});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_BLEND_FACTOR),      GLenum{GL_CONSTANT_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_INV_BLEND_FACTOR),  GLenum{GL_ONE_MINUS_CONSTANT_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_SRC1_COLOR),        GLenum{GL_SRC1_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_INV_SRC1_COLOR),    GLenum{GL_ONE_MINUS_SRC1_COLOR});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_SRC1_ALPHA),        GLenum{GL_SRC1_ALPHA});
    EXPECT_EQ(BlendFactor2GLBlend(BLEND_FACTOR_INV_SRC1_ALPHA),    GLenum{GL_ONE_MINUS_SRC1_ALPHA});
    // clang-format on
}

TEST(GLTypeConversionsTest, BlendOperation2GLBlendOp)
{

#define CHECK_BLEND_OPERATION_ENUM_VALUE(EnumElement, ExpectedValue) static_assert(EnumElement == ExpectedValue, "BlendOperation2GLBlendOp function assumes that " #EnumElement " equals " #ExpectedValue ": fix BlendOperation2GLBlendOpMap array and update this assertion.")
    // clang-format off
    CHECK_BLEND_OPERATION_ENUM_VALUE(BLEND_OPERATION_UNDEFINED,     0);
    CHECK_BLEND_OPERATION_ENUM_VALUE(BLEND_OPERATION_ADD,           1);
    CHECK_BLEND_OPERATION_ENUM_VALUE(BLEND_OPERATION_SUBTRACT,      2);
    CHECK_BLEND_OPERATION_ENUM_VALUE(BLEND_OPERATION_REV_SUBTRACT,  3);
    CHECK_BLEND_OPERATION_ENUM_VALUE(BLEND_OPERATION_MIN,           4);
    CHECK_BLEND_OPERATION_ENUM_VALUE(BLEND_OPERATION_MAX,           5);
    // clang-format on
    static_assert(BLEND_OPERATION_NUM_OPERATIONS == 6,
                  "Did you add a new BLEND_OPERATION enum value? Please update BlendOperation2GLBlendOp function.");
#undef CHECK_BLEND_OPERATION_ENUM_VALUE

    // clang-format off
    EXPECT_EQ(BlendOperation2GLBlendOp(BLEND_OPERATION_UNDEFINED),    GLenum{0});
    EXPECT_EQ(BlendOperation2GLBlendOp(BLEND_OPERATION_ADD),          GLenum{GL_FUNC_ADD});
    EXPECT_EQ(BlendOperation2GLBlendOp(BLEND_OPERATION_SUBTRACT),     GLenum{GL_FUNC_SUBTRACT});
    EXPECT_EQ(BlendOperation2GLBlendOp(BLEND_OPERATION_REV_SUBTRACT), GLenum{GL_FUNC_REVERSE_SUBTRACT});
    EXPECT_EQ(BlendOperation2GLBlendOp(BLEND_OPERATION_MIN),          GLenum{GL_MIN});
    EXPECT_EQ(BlendOperation2GLBlendOp(BLEND_OPERATION_MAX),          GLenum{GL_MAX});
    // clang-format on
}

} // namespace
