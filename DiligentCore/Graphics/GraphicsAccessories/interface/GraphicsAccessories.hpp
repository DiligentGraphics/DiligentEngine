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
/// Defines graphics engine utilities

#include "../../GraphicsEngine/interface/GraphicsTypes.h"
#include "../../GraphicsEngine/interface/Shader.h"
#include "../../GraphicsEngine/interface/Texture.h"
#include "../../GraphicsEngine/interface/Buffer.h"
#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "../../../Platforms/interface/PlatformMisc.hpp"

namespace Diligent
{

/// Template structure to convert VALUE_TYPE enumeration into C-type
template <VALUE_TYPE ValType>
struct VALUE_TYPE2CType
{};

/// VALUE_TYPE2CType<> template specialization for 8-bit integer value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_INT8>::CType MyInt8Var;
template <> struct VALUE_TYPE2CType<VT_INT8>
{
    typedef Int8 CType;
};

/// VALUE_TYPE2CType<> template specialization for 16-bit integer value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_INT16>::CType MyInt16Var;
template <> struct VALUE_TYPE2CType<VT_INT16>
{
    typedef Int16 CType;
};

/// VALUE_TYPE2CType<> template specialization for 32-bit integer value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_INT32>::CType MyInt32Var;
template <> struct VALUE_TYPE2CType<VT_INT32>
{
    typedef Int32 CType;
};

/// VALUE_TYPE2CType<> template specialization for 8-bit unsigned-integer value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_UINT8>::CType MyUint8Var;
template <> struct VALUE_TYPE2CType<VT_UINT8>
{
    typedef Uint8 CType;
};

/// VALUE_TYPE2CType<> template specialization for 16-bit unsigned-integer value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_UINT16>::CType MyUint16Var;
template <> struct VALUE_TYPE2CType<VT_UINT16>
{
    typedef Uint16 CType;
};

/// VALUE_TYPE2CType<> template specialization for 32-bit unsigned-integer value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_UINT32>::CType MyUint32Var;
template <> struct VALUE_TYPE2CType<VT_UINT32>
{
    typedef Uint32 CType;
};

/// VALUE_TYPE2CType<> template specialization for half-precision 16-bit floating-point value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_FLOAT16>::CType MyFloat16Var;
///
/// \note 16-bit floating-point values have no corresponding C++ type and are translated to Uint16
template <> struct VALUE_TYPE2CType<VT_FLOAT16>
{
    typedef Uint16 CType;
};

/// VALUE_TYPE2CType<> template specialization for full-precision 32-bit floating-point value type.

/// Usage example:
///
///     VALUE_TYPE2CType<VT_FLOAT32>::CType MyFloat32Var;
template <> struct VALUE_TYPE2CType<VT_FLOAT32>
{
    typedef Float32 CType;
};


static const Uint32 ValueTypeToSizeMap[] =
    // clang-format off
{
    0,
    sizeof(VALUE_TYPE2CType<VT_INT8>    :: CType),
    sizeof(VALUE_TYPE2CType<VT_INT16>   :: CType),
    sizeof(VALUE_TYPE2CType<VT_INT32>   :: CType),
    sizeof(VALUE_TYPE2CType<VT_UINT8>   :: CType),
    sizeof(VALUE_TYPE2CType<VT_UINT16>  :: CType),
    sizeof(VALUE_TYPE2CType<VT_UINT32>  :: CType),
    sizeof(VALUE_TYPE2CType<VT_FLOAT16> :: CType),
    sizeof(VALUE_TYPE2CType<VT_FLOAT32> :: CType)
};
// clang-format on
static_assert(VT_NUM_TYPES == VT_FLOAT32 + 1, "Not all value type sizes initialized.");

/// Returns the size of the specified value type
inline Uint32 GetValueSize(VALUE_TYPE Val)
{
    VERIFY_EXPR(Val < _countof(ValueTypeToSizeMap));
    return ValueTypeToSizeMap[Val];
}

/// Returns the string representing the specified value type
const Char* GetValueTypeString(VALUE_TYPE Val);

/// Returns invariant texture format attributes, see TextureFormatAttribs for details.

/// \param [in] Format - Texture format which attributes are requested for.
/// \return Constant reference to the TextureFormatAttribs structure containing
///         format attributes.
const TextureFormatAttribs& GetTextureFormatAttribs(TEXTURE_FORMAT Format);

/// Returns the default format for a specified texture view type

/// The default view is defined as follows:
/// * For a fully qualified texture format, the SRV/RTV/UAV view format is the same as texture format;
///   DSV format, if avaialble, is adjusted accrodingly (R32_FLOAT -> D32_FLOAT)
/// * For 32-bit typeless formats, default view is XXXX32_FLOAT (where XXXX are the actual format components)\n
/// * For 16-bit typeless formats, default view is XXXX16_FLOAT (where XXXX are the actual format components)\n
/// ** R16_TYPELESS is special. If BIND_DEPTH_STENCIL flag is set, it is translated to R16_UNORM/D16_UNORM;
///    otherwise it is translated to R16_FLOAT.
/// * For 8-bit typeless formats, default view is XXXX8_UNORM (where XXXX are the actual format components)\n
/// * sRGB is always chosen if it is available (RGBA8_UNORM_SRGB, TEX_FORMAT_BC1_UNORM_SRGB, etc.)
/// * For combined depth-stencil formats, SRV format references depth component (R24_UNORM_X8_TYPELESS for D24S8 formats, and
///   R32_FLOAT_X8X24_TYPELESS for D32S8X24 formats)
/// * For compressed formats, only SRV format is defined
///
/// \param [in] Format - texture format, for which the view format is requested
/// \param [in] ViewType - texture view type
/// \param [in] BindFlags - texture bind flags
/// \return  texture view type format
TEXTURE_FORMAT GetDefaultTextureViewFormat(TEXTURE_FORMAT TextureFormat, TEXTURE_VIEW_TYPE ViewType, Uint32 BindFlags);

/// Returns the default format for a specified texture view type

/// \param [in] TexDesc - texture description
/// \param [in] ViewType - texture view type
/// \return  texture view type format
inline TEXTURE_FORMAT GetDefaultTextureViewFormat(const TextureDesc& TexDesc, TEXTURE_VIEW_TYPE ViewType)
{
    return GetDefaultTextureViewFormat(TexDesc.Format, ViewType, TexDesc.BindFlags);
}

/// Returns the literal name of a texture view type. For instance,
/// for a shader resource view, "TEXTURE_VIEW_SHADER_RESOURCE" will be returned.

/// \param [in] ViewType - Texture view type.
/// \return Literal name of the texture view type.
const Char* GetTexViewTypeLiteralName(TEXTURE_VIEW_TYPE ViewType);

/// Returns the literal name of a buffer view type. For instance,
/// for an unordered access view, "BUFFER_VIEW_UNORDERED_ACCESS" will be returned.

/// \param [in] ViewType - Buffer view type.
/// \return Literal name of the buffer view type.
const Char* GetBufferViewTypeLiteralName(BUFFER_VIEW_TYPE ViewType);

/// Returns the literal name of a shader type. For instance,
/// for a pixel shader, "SHADER_TYPE_PIXEL" will be returned.

/// \param [in] ShaderType - Shader type.
/// \return Literal name of the shader type.
const Char* GetShaderTypeLiteralName(SHADER_TYPE ShaderType);

/// \param [in] ShaderStages - Shader stages.
/// \return The string representing the shader stages. For example,
///         if ShaderStages == SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL,
///         the following string will be returned:
///         "SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL"
String GetShaderStagesString(SHADER_TYPE ShaderStages);

/// Returns the literal name of a shader variable type. For instance,
/// for SHADER_RESOURCE_VARIABLE_TYPE_STATIC, if bGetFullName == true, "SHADER_RESOURCE_VARIABLE_TYPE_STATIC" will be returned;
/// if bGetFullName == false, "static" will be returned

/// \param [in] VarType - Variable type.
/// \param [in] bGetFullName - Whether to return string representation of the enum value
/// \return Literal name of the shader variable type.
const Char* GetShaderVariableTypeLiteralName(SHADER_RESOURCE_VARIABLE_TYPE VarType, bool bGetFullName = false);

/// Returns the literal name of a shader resource type. For instance,
/// for SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, if bGetFullName == true, "SHADER_RESOURCE_TYPE_CONSTANT_BUFFER" will be returned;
/// if bGetFullName == false, "constant buffer" will be returned

/// \param [in] ResourceType - Resource type.
/// \param [in] bGetFullName - Whether to return string representation of the enum value
/// \return Literal name of the shader resource type.
const Char* GetShaderResourceTypeLiteralName(SHADER_RESOURCE_TYPE ResourceType, bool bGetFullName = false);

/// Overloaded function that returns the literal name of a texture view type.
/// see GetTexViewTypeLiteralName().
inline const Char* GetViewTypeLiteralName(TEXTURE_VIEW_TYPE TexViewType)
{
    return GetTexViewTypeLiteralName(TexViewType);
}

/// Overloaded function that returns the literal name of a buffer view type.
/// see GetBufferViewTypeLiteralName().
inline const Char* GetViewTypeLiteralName(BUFFER_VIEW_TYPE BuffViewType)
{
    return GetBufferViewTypeLiteralName(BuffViewType);
}

/// Returns the literal name of a filter type. For instance,
/// for FILTER_TYPE_POINT, if bGetFullName == true, "FILTER_TYPE_POINT" will be returned;
/// if bGetFullName == false, "point" will be returned.

/// \param [in] FilterType   - Filter type, see Diligent::FILTER_TYPE.
/// \param [in] bGetFullName - Whether to return string representation of the enum value.
/// \return                    Literal name of the filter type.
const Char* GetFilterTypeLiteralName(FILTER_TYPE FilterType, bool bGetFullName);


/// Returns the literal name of a texture address mode. For instance,
/// for TEXTURE_ADDRESS_WRAP, if bGetFullName == true, "TEXTURE_ADDRESS_WRAP" will be returned;
/// if bGetFullName == false, "wrap" will be returned.

/// \param [in] AddressMode  - Texture address mode, see Diligent::TEXTURE_ADDRESS_MODE.
/// \param [in] bGetFullName - Whether to return string representation of the enum value.
/// \return                    Literal name of the address mode.
const Char* GetTextureAddressModeLiteralName(TEXTURE_ADDRESS_MODE AddressMode, bool bGetFullName);


/// Returns the literal name of a comparison function. For instance,
/// for COMPARISON_FUNC_LESS, if bGetFullName == true, "COMPARISON_FUNC_LESS" will be returned;
/// if bGetFullName == false, "less" will be returned.

/// \param [in] ComparisonFunc - Comparison function, see Diligent::COMPARISON_FUNCTION.
/// \param [in] bGetFullName   - Whether to return string representation of the enum value.
/// \return                      Literal name of the comparison function.
const Char* GetComparisonFunctionLiteralName(COMPARISON_FUNCTION ComparisonFunc, bool bGetFullName);


/// Returns the literal name of a stencil operation.

/// \param [in] StencilOp - Stencil operation, see Diligent::STENCIL_OP.
/// \return                 Literal name of the stencil operation.
const Char* GetStencilOpLiteralName(STENCIL_OP StencilOp);


/// Returns the literal name of a blend factor.

/// \param [in] BlendFactor - Blend factor, see Diligent::BLEND_FACTOR.
/// \return                   Literal name of the blend factor.
const Char* GetBlendFactorLiteralName(BLEND_FACTOR BlendFactor);


/// Returns the literal name of a blend operation.

/// \param [in] BlendOp - Blend operation, see Diligent::BLEND_OPERATION.
/// \return               Literal name of the blend operation.
const Char* GetBlendOperationLiteralName(BLEND_OPERATION BlendOp);


/// Returns the literal name of a fill mode.

/// \param [in] FillMode - Fill mode, see Diligent::FILL_MODE.
/// \return                Literal name of the fill mode.
const Char* GetFillModeLiteralName(FILL_MODE FillMode);

/// Returns the literal name of a cull mode.

/// \param [in] CullMode - Cull mode, see Diligent::CULL_MODE.
/// \return                Literal name of the cull mode.
const Char* GetCullModeLiteralName(CULL_MODE CullMode);

/// Returns the string containing the map type
const Char* GetMapTypeString(MAP_TYPE MapType);

/// Returns the string containing the usage
const Char* GetUsageString(USAGE Usage);

/// Returns the string containing the texture type
const Char* GetResourceDimString(RESOURCE_DIMENSION TexType);

/// Returns the string containing single bind flag
const Char* GetBindFlagString(Uint32 BindFlag);

/// Returns the string containing the bind flags
String GetBindFlagsString(Uint32 BindFlags, const char* Delimeter = "|");

/// Returns the string containing the CPU access flags
String GetCPUAccessFlagsString(Uint32 CpuAccessFlags);

/// Returns the string containing the texture description
String GetTextureDescString(const TextureDesc& Desc);

/// Returns the string containing the buffer format description
String GetBufferFormatString(const BufferFormat& Fmt);

/// Returns the string containing the buffer mode description
const Char* GetBufferModeString(BUFFER_MODE Mode);

/// Returns the string containing the buffer description
String GetBufferDescString(const BufferDesc& Desc);

/// Returns the string containing the buffer mode description
const Char* GetResourceStateFlagString(RESOURCE_STATE State);
String      GetResourceStateString(RESOURCE_STATE State);

/// Helper template function that converts object description into a string
template <typename TObjectDescType>
String GetObjectDescString(const TObjectDescType&)
{
    return "";
}

/// Template specialization for texture description
template <>
inline String GetObjectDescString(const TextureDesc& TexDesc)
{
    String Str{"Tex desc: "};
    Str += GetTextureDescString(TexDesc);
    return Str;
}

/// Template specialization for buffer description
template <>
inline String GetObjectDescString(const BufferDesc& BuffDesc)
{
    String Str{"Buff desc: "};
    Str += GetBufferDescString(BuffDesc);
    return Str;
}

const char* GetQueryTypeString(QUERY_TYPE QueryType);

const char* GetSurfaceTransformString(SURFACE_TRANSFORM SrfTransform);

const char* GetPipelineTypeString(PIPELINE_TYPE PipelineType);

const char* GetShaderCompilerTypeString(SHADER_COMPILER Compiler);


Uint32 ComputeMipLevelsCount(Uint32 Width);
Uint32 ComputeMipLevelsCount(Uint32 Width, Uint32 Height);
Uint32 ComputeMipLevelsCount(Uint32 Width, Uint32 Height, Uint32 Depth);

inline bool IsComparisonFilter(FILTER_TYPE FilterType)
{
    return FilterType == FILTER_TYPE_COMPARISON_POINT ||
        FilterType == FILTER_TYPE_COMPARISON_LINEAR ||
        FilterType == FILTER_TYPE_COMPARISON_ANISOTROPIC;
}

inline bool IsAnisotropicFilter(FILTER_TYPE FilterType)
{
    return FilterType == FILTER_TYPE_ANISOTROPIC ||
        FilterType == FILTER_TYPE_COMPARISON_ANISOTROPIC ||
        FilterType == FILTER_TYPE_MINIMUM_ANISOTROPIC ||
        FilterType == FILTER_TYPE_MAXIMUM_ANISOTROPIC;
}

bool VerifyResourceStates(RESOURCE_STATE State, bool IsTexture);

/// Describes the mip level properties
struct MipLevelProperties
{
    Uint32 LogicalWidth   = 0;
    Uint32 LogicalHeight  = 0;
    Uint32 StorageWidth   = 0;
    Uint32 StorageHeight  = 0;
    Uint32 Depth          = 1;
    Uint32 RowSize        = 0;
    Uint32 DepthSliceSize = 0;
    Uint32 MipSize        = 0;
};

MipLevelProperties GetMipLevelProperties(const TextureDesc& TexDesc, Uint32 MipLevel);

ADAPTER_VENDOR VendorIdToAdapterVendor(Uint32 VendorId);


inline Int32 GetShaderTypeIndex(SHADER_TYPE Type)
{
    if (Type == SHADER_TYPE_UNKNOWN)
        return -1;

    VERIFY(Type > SHADER_TYPE_UNKNOWN && Type <= SHADER_TYPE_LAST, "Value ", Uint32{Type}, " is not a valid SHADER_TYPE enum value");
    VERIFY(((Uint32{Type} & (Uint32{Type} - 1)) == 0), "Only single shader stage should be provided");

    return PlatformMisc::GetLSB(Type);
}

static_assert(SHADER_TYPE_LAST == 0x080, "Please add the new shader type index below");

static constexpr Int32 VSInd = 0;
static constexpr Int32 PSInd = 1;
static constexpr Int32 GSInd = 2;
static constexpr Int32 HSInd = 3;
static constexpr Int32 DSInd = 4;
static constexpr Int32 CSInd = 5;
static constexpr Int32 ASInd = 6;
static constexpr Int32 MSInd = 7;

static constexpr Int32 LastShaderInd = MSInd;

// clang-format off
static_assert(SHADER_TYPE_VERTEX        == (1 << VSInd), "VSInd is not consistent with SHADER_TYPE_VERTEX");
static_assert(SHADER_TYPE_PIXEL         == (1 << PSInd), "PSInd is not consistent with SHADER_TYPE_PIXEL");
static_assert(SHADER_TYPE_GEOMETRY      == (1 << GSInd), "GSInd is not consistent with SHADER_TYPE_GEOMETRY");
static_assert(SHADER_TYPE_HULL          == (1 << HSInd), "HSInd is not consistent with SHADER_TYPE_HULL");
static_assert(SHADER_TYPE_DOMAIN        == (1 << DSInd), "DSInd is not consistent with SHADER_TYPE_DOMAIN");
static_assert(SHADER_TYPE_COMPUTE       == (1 << CSInd), "CSInd is not consistent with SHADER_TYPE_COMPUTE");
static_assert(SHADER_TYPE_AMPLIFICATION == (1 << ASInd), "ASInd is not consistent with SHADER_TYPE_AMPLIFICATION");
static_assert(SHADER_TYPE_MESH          == (1 << MSInd), "MSInd is not consistent with SHADER_TYPE_MESH");

static_assert(SHADER_TYPE_LAST == (1 << LastShaderInd), "LastShaderInd is not consistent with SHADER_TYPE_LAST");
// clang-format on

inline SHADER_TYPE GetShaderTypeFromIndex(Int32 Index)
{
    VERIFY(Index >= 0 && Index <= LastShaderInd, "Shader type index is out of range");
    return static_cast<SHADER_TYPE>(1 << Index);
}


bool        IsConsistentShaderType(SHADER_TYPE ShaderType, PIPELINE_TYPE PipelineType);
Int32       GetShaderTypePipelineIndex(SHADER_TYPE ShaderType, PIPELINE_TYPE PipelineType);
SHADER_TYPE GetShaderTypeFromPipelineIndex(Int32 Index, PIPELINE_TYPE PipelineType);

/// Returns an offset from the beginning of the buffer backing a staging texture
/// to the specified location within the given subresource.
///
/// \param [in] TexDesc     - Staging texture description.
/// \param [in] ArraySlice  - Array slice.
/// \param [in] MipLevel    - Mip level.
/// \param [in] Alignment   - Subresource alignment. The alignment is applied
///                           to whole subresources only, but not to the row/depth strides.
///                           In other words, there may be padding between subresources, but
///                           texels in every subresource are assumed to be tightly packed.
/// \param [in] LocationX   - X location within the subresoure.
/// \param [in] LocationY   - Y location within the subresoure.
/// \param [in] LocationZ   - Z location within the subresoure.
///
/// \return     Offset from the beginning of the buffer to the given location.
///
/// \remarks    Alignment is applied to the subresource sizes, such that the beginning of data
///             of every subresource starts at an offset aligned by 'Alignment'. The alignment
///             is not applied to the row/depth strides and texels in all subresources are assumed
///             to be tightly packed.
///
///             Subres 0
///              stride
///       |<-------------->|
///       |________________|       Subres 1
///       |                |        stride
///       |                |     |<------->|
//        |                |     |_________|
///       |    Subres 0    |     |         |
///       |                |     | Subres 1|
///       |                |     |         |                     _
///       |________________|     |_________|         ...        |_|
///       A                      A                              A
///       |                      |                              |
///     Buffer start            Subres 1 offset,               Subres N offset,
///                          aligned by 'Alignment'         aligned by 'Alignment'
///
Uint32 GetStagingTextureLocationOffset(const TextureDesc& TexDesc,
                                       Uint32             ArraySlice,
                                       Uint32             MipLevel,
                                       Uint32             Alignment,
                                       Uint32             LocationX,
                                       Uint32             LocationY,
                                       Uint32             LocationZ);

/// Returns an offset from the beginning of the buffer backing a staging texture
/// to the given subresource.
/// Texels within subresources are assumed to be tightly packed. There is no padding
/// except between whole subresources.
inline Uint32 GetStagingTextureSubresourceOffset(const TextureDesc& TexDesc,
                                                 Uint32             ArraySlice,
                                                 Uint32             MipLevel,
                                                 Uint32             Alignment)
{
    return GetStagingTextureLocationOffset(TexDesc, ArraySlice, MipLevel, Alignment, 0, 0, 0);
}

} // namespace Diligent
