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

#include "D3D12TypeConversions.hpp"
#include "DXGITypeConversions.hpp"

#include "D3D12TypeDefinitions.h"
#include "D3DTypeConversionImpl.hpp"
#include "D3DViewDescConversionImpl.hpp"
#include "PlatformMisc.hpp"

namespace Diligent
{

D3D12_COMPARISON_FUNC ComparisonFuncToD3D12ComparisonFunc(COMPARISON_FUNCTION Func)
{
    return ComparisonFuncToD3DComparisonFunc<D3D12_COMPARISON_FUNC>(Func);
}

D3D12_FILTER FilterTypeToD3D12Filter(FILTER_TYPE MinFilter, FILTER_TYPE MagFilter, FILTER_TYPE MipFilter)
{
    return FilterTypeToD3DFilter<D3D12_FILTER>(MinFilter, MagFilter, MipFilter);
}

D3D12_TEXTURE_ADDRESS_MODE TexAddressModeToD3D12AddressMode(TEXTURE_ADDRESS_MODE Mode)
{
    return TexAddressModeToD3DAddressMode<D3D12_TEXTURE_ADDRESS_MODE>(Mode);
}

void DepthStencilStateDesc_To_D3D12_DEPTH_STENCIL_DESC(const DepthStencilStateDesc& DepthStencilDesc,
                                                       D3D12_DEPTH_STENCIL_DESC&    d3d12DSSDesc)
{
    DepthStencilStateDesc_To_D3D_DEPTH_STENCIL_DESC<D3D12_DEPTH_STENCIL_DESC, D3D12_DEPTH_STENCILOP_DESC, D3D12_STENCIL_OP, D3D12_COMPARISON_FUNC>(DepthStencilDesc, d3d12DSSDesc);
}

void RasterizerStateDesc_To_D3D12_RASTERIZER_DESC(const RasterizerStateDesc& RasterizerDesc,
                                                  D3D12_RASTERIZER_DESC&     d3d12RSDesc)
{
    RasterizerStateDesc_To_D3D_RASTERIZER_DESC<D3D12_RASTERIZER_DESC, D3D12_FILL_MODE, D3D12_CULL_MODE>(RasterizerDesc, d3d12RSDesc);

    // The sample count that is forced while UAV rendering or rasterizing.
    // Valid values are 0, 1, 2, 4, 8, and optionally 16. 0 indicates that
    // the sample count is not forced.
    d3d12RSDesc.ForcedSampleCount = 0;

    d3d12RSDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}



D3D12_LOGIC_OP LogicOperationToD3D12LogicOp(LOGIC_OPERATION lo)
{
    // Note that this code is safe for multithreaded environments since
    // bIsInit is set to true only AFTER the entire map is initialized.
    static bool           bIsInit                               = false;
    static D3D12_LOGIC_OP D3D12LogicOp[LOGIC_OP_NUM_OPERATIONS] = {};
    if (!bIsInit)
    {
        // clang-format off
        // In a multithreaded environment, several threads can potentially enter
        // this block. This is not a problem since they will just initialize the 
        // memory with the same values more than once
        D3D12LogicOp[ D3D12_LOGIC_OP_CLEAR		    ]  = D3D12_LOGIC_OP_CLEAR;
        D3D12LogicOp[ D3D12_LOGIC_OP_SET			]  = D3D12_LOGIC_OP_SET;
        D3D12LogicOp[ D3D12_LOGIC_OP_COPY			]  = D3D12_LOGIC_OP_COPY;
        D3D12LogicOp[ D3D12_LOGIC_OP_COPY_INVERTED  ]  = D3D12_LOGIC_OP_COPY_INVERTED;
        D3D12LogicOp[ D3D12_LOGIC_OP_NOOP			]  = D3D12_LOGIC_OP_NOOP;
        D3D12LogicOp[ D3D12_LOGIC_OP_INVERT		    ]  = D3D12_LOGIC_OP_INVERT;
        D3D12LogicOp[ D3D12_LOGIC_OP_AND			]  = D3D12_LOGIC_OP_AND;
        D3D12LogicOp[ D3D12_LOGIC_OP_NAND			]  = D3D12_LOGIC_OP_NAND;
        D3D12LogicOp[ D3D12_LOGIC_OP_OR			    ]  = D3D12_LOGIC_OP_OR;
        D3D12LogicOp[ D3D12_LOGIC_OP_NOR			]  = D3D12_LOGIC_OP_NOR;
        D3D12LogicOp[ D3D12_LOGIC_OP_XOR			]  = D3D12_LOGIC_OP_XOR;
        D3D12LogicOp[ D3D12_LOGIC_OP_EQUIV		    ]  = D3D12_LOGIC_OP_EQUIV;
        D3D12LogicOp[ D3D12_LOGIC_OP_AND_REVERSE	]  = D3D12_LOGIC_OP_AND_REVERSE;
        D3D12LogicOp[ D3D12_LOGIC_OP_AND_INVERTED	]  = D3D12_LOGIC_OP_AND_INVERTED;
        D3D12LogicOp[ D3D12_LOGIC_OP_OR_REVERSE	    ]  = D3D12_LOGIC_OP_OR_REVERSE;
        D3D12LogicOp[ D3D12_LOGIC_OP_OR_INVERTED	]  = D3D12_LOGIC_OP_OR_INVERTED;
        // clang-format on

        bIsInit = true;
    }
    if (lo >= LOGIC_OP_CLEAR && lo < LOGIC_OP_NUM_OPERATIONS)
    {
        auto d3dlo = D3D12LogicOp[lo];
        return d3dlo;
    }
    else
    {
        UNEXPECTED("Incorrect blend factor (", lo, ")");
        return static_cast<D3D12_LOGIC_OP>(0);
    }
}


void BlendStateDesc_To_D3D12_BLEND_DESC(const BlendStateDesc& BSDesc, D3D12_BLEND_DESC& d3d12BlendDesc)
{
    BlendStateDescToD3DBlendDesc<D3D12_BLEND_DESC, D3D12_BLEND, D3D12_BLEND_OP>(BSDesc, d3d12BlendDesc);

    for (int i = 0; i < 8; ++i)
    {
        const auto& SrcRTDesc = BSDesc.RenderTargets[i];
        auto&       DstRTDesc = d3d12BlendDesc.RenderTarget[i];

        // The following members only present in D3D_RENDER_TARGET_BLEND_DESC
        DstRTDesc.LogicOpEnable = SrcRTDesc.LogicOperationEnable ? TRUE : FALSE;
        DstRTDesc.LogicOp       = LogicOperationToD3D12LogicOp(SrcRTDesc.LogicOp);
    }
}

void LayoutElements_To_D3D12_INPUT_ELEMENT_DESCs(const InputLayoutDesc&                                                               InputLayout,
                                                 std::vector<D3D12_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D12_INPUT_ELEMENT_DESC>>& d3d12InputElements)
{
    LayoutElements_To_D3D_INPUT_ELEMENT_DESCs<D3D12_INPUT_ELEMENT_DESC>(InputLayout, d3d12InputElements);
}

D3D12_PRIMITIVE_TOPOLOGY TopologyToD3D12Topology(PRIMITIVE_TOPOLOGY Topology)
{
    return TopologyToD3DTopology<D3D12_PRIMITIVE_TOPOLOGY>(Topology);
}



void TextureViewDesc_to_D3D12_SRV_DESC(const TextureViewDesc&           SRVDesc,
                                       D3D12_SHADER_RESOURCE_VIEW_DESC& D3D12SRVDesc,
                                       Uint32                           SampleCount)
{
    TextureViewDesc_to_D3D_SRV_DESC(SRVDesc, D3D12SRVDesc, SampleCount);
    D3D12SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (SRVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            D3D12SRVDesc.Texture1D.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            D3D12SRVDesc.Texture1DArray.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_2D:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12SRVDesc.Texture2D.PlaneSlice          = 0;
                D3D12SRVDesc.Texture2D.ResourceMinLODClamp = 0;
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12SRVDesc.Texture2DArray.PlaneSlice          = 0;
                D3D12SRVDesc.Texture2DArray.ResourceMinLODClamp = 0;
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            D3D12SRVDesc.Texture3D.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_CUBE:
            D3D12SRVDesc.TextureCube.ResourceMinLODClamp = 0;
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            D3D12SRVDesc.TextureCubeArray.ResourceMinLODClamp = 0;
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

void TextureViewDesc_to_D3D12_RTV_DESC(const TextureViewDesc&         RTVDesc,
                                       D3D12_RENDER_TARGET_VIEW_DESC& D3D12RTVDesc,
                                       Uint32                         SampleCount)
{
    TextureViewDesc_to_D3D_RTV_DESC(RTVDesc, D3D12RTVDesc, SampleCount);
    switch (RTVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            break;

        case RESOURCE_DIM_TEX_2D:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12RTVDesc.Texture2D.PlaneSlice = 0;
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (SampleCount > 1)
            {
            }
            else
            {
                D3D12RTVDesc.Texture2DArray.PlaneSlice = 0;
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

void TextureViewDesc_to_D3D12_DSV_DESC(const TextureViewDesc&         DSVDesc,
                                       D3D12_DEPTH_STENCIL_VIEW_DESC& D3D12DSVDesc,
                                       Uint32                         SampleCount)
{
    TextureViewDesc_to_D3D_DSV_DESC(DSVDesc, D3D12DSVDesc, SampleCount);
    D3D12DSVDesc.Flags = D3D12_DSV_FLAG_NONE;
}

void TextureViewDesc_to_D3D12_UAV_DESC(const TextureViewDesc&            UAVDesc,
                                       D3D12_UNORDERED_ACCESS_VIEW_DESC& D3D12UAVDesc)
{
    TextureViewDesc_to_D3D_UAV_DESC(UAVDesc, D3D12UAVDesc);
    switch (UAVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            break;

        case RESOURCE_DIM_TEX_2D:
            D3D12UAVDesc.Texture2D.PlaneSlice = 0;
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            D3D12UAVDesc.Texture2DArray.PlaneSlice = 0;
            break;

        case RESOURCE_DIM_TEX_3D:
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}


void BufferViewDesc_to_D3D12_SRV_DESC(const BufferDesc&                BuffDesc,
                                      const BufferViewDesc&            SRVDesc,
                                      D3D12_SHADER_RESOURCE_VIEW_DESC& D3D12SRVDesc)
{
    BufferViewDesc_to_D3D_SRV_DESC(BuffDesc, SRVDesc, D3D12SRVDesc);
    D3D12SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (BuffDesc.Mode == BUFFER_MODE_RAW && SRVDesc.Format.ValueType == VT_UNDEFINED)
    {
        D3D12SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        D3D12SRVDesc.Format       = DXGI_FORMAT_R32_TYPELESS;
    }
    else
        D3D12SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    VERIFY_EXPR(BuffDesc.BindFlags & BIND_SHADER_RESOURCE);
    if (BuffDesc.Mode == BUFFER_MODE_STRUCTURED)
        D3D12SRVDesc.Buffer.StructureByteStride = BuffDesc.ElementByteStride;
}

void BufferViewDesc_to_D3D12_UAV_DESC(const BufferDesc&                 BuffDesc,
                                      const BufferViewDesc&             UAVDesc,
                                      D3D12_UNORDERED_ACCESS_VIEW_DESC& D3D12UAVDesc)
{
    BufferViewDesc_to_D3D_UAV_DESC(BuffDesc, UAVDesc, D3D12UAVDesc);
    VERIFY_EXPR(BuffDesc.BindFlags & BIND_UNORDERED_ACCESS);
    if (BuffDesc.Mode == BUFFER_MODE_STRUCTURED)
        D3D12UAVDesc.Buffer.StructureByteStride = BuffDesc.ElementByteStride;
}

D3D12_STATIC_BORDER_COLOR BorderColorToD3D12StaticBorderColor(const Float32 BorderColor[])
{
    D3D12_STATIC_BORDER_COLOR StaticBorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 0)
        StaticBorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    else if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 1)
        StaticBorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    else if (BorderColor[0] == 1 && BorderColor[1] == 1 && BorderColor[2] == 1 && BorderColor[3] == 1)
        StaticBorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    else
    {
        LOG_ERROR_MESSAGE("D3D12 static samplers only allow transparent black (0,0,0,0), opaque black (0,0,0,1) or opaque white (1,1,1,1) as border colors.");
    }
    return StaticBorderColor;
}


static D3D12_RESOURCE_STATES ResourceStateFlagToD3D12ResourceState(RESOURCE_STATE StateFlag)
{
    static_assert(RESOURCE_STATE_MAX_BIT == 0x10000, "This function must be updated to handle new resource state flag");
    VERIFY((StateFlag & (StateFlag - 1)) == 0, "Only single bit must be set");
    switch (StateFlag)
    {
        // clang-format off
        case RESOURCE_STATE_UNDEFINED:         return D3D12_RESOURCE_STATE_COMMON;
        case RESOURCE_STATE_VERTEX_BUFFER:     return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case RESOURCE_STATE_CONSTANT_BUFFER:   return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case RESOURCE_STATE_INDEX_BUFFER:      return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case RESOURCE_STATE_RENDER_TARGET:     return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RESOURCE_STATE_UNORDERED_ACCESS:  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case RESOURCE_STATE_DEPTH_WRITE:       return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case RESOURCE_STATE_DEPTH_READ:        return D3D12_RESOURCE_STATE_DEPTH_READ;
        case RESOURCE_STATE_SHADER_RESOURCE:   return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case RESOURCE_STATE_STREAM_OUT:        return D3D12_RESOURCE_STATE_STREAM_OUT;
        case RESOURCE_STATE_INDIRECT_ARGUMENT: return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case RESOURCE_STATE_COPY_DEST:         return D3D12_RESOURCE_STATE_COPY_DEST;
        case RESOURCE_STATE_COPY_SOURCE:       return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case RESOURCE_STATE_RESOLVE_DEST:      return D3D12_RESOURCE_STATE_RESOLVE_DEST;
        case RESOURCE_STATE_RESOLVE_SOURCE:    return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case RESOURCE_STATE_PRESENT:           return D3D12_RESOURCE_STATE_PRESENT;
        // clang-format on
        default:
            UNEXPECTED("Unexpected resource state flag");
            return static_cast<D3D12_RESOURCE_STATES>(0);
    }
}

class StateFlagBitPosToD3D12ResourceState
{
public:
    StateFlagBitPosToD3D12ResourceState()
    {
        static_assert((1 << MaxFlagBitPos) == RESOURCE_STATE_MAX_BIT, "This function must be updated to handle new resource state flag");
        for (Uint32 bit = 0; bit <= MaxFlagBitPos; ++bit)
        {
            FlagBitPosToResStateMap[bit] = ResourceStateFlagToD3D12ResourceState(static_cast<RESOURCE_STATE>(1 << bit));
        }
    }

    D3D12_RESOURCE_STATES operator()(Uint32 BitPos) const
    {
        VERIFY(BitPos <= MaxFlagBitPos, "Resource state flag bit position (", BitPos, ") exceeds max bit position (", MaxFlagBitPos, ")");
        return FlagBitPosToResStateMap[BitPos];
    }

private:
    static constexpr Uint32                              MaxFlagBitPos = 16;
    std::array<D3D12_RESOURCE_STATES, MaxFlagBitPos + 1> FlagBitPosToResStateMap;
};

D3D12_RESOURCE_STATES ResourceStateFlagsToD3D12ResourceStates(RESOURCE_STATE StateFlags)
{
    VERIFY(StateFlags < (RESOURCE_STATE_MAX_BIT << 1), "Resource state flags are out of range");
    static const StateFlagBitPosToD3D12ResourceState BitPosToD3D12ResState;
    D3D12_RESOURCE_STATES                            D3D12ResourceStates = static_cast<D3D12_RESOURCE_STATES>(0);
    Uint32                                           Bits                = StateFlags;
    while (Bits != 0)
    {
        auto lsb = PlatformMisc::GetLSB(Bits);
        D3D12ResourceStates |= BitPosToD3D12ResState(lsb);
        Bits &= ~(1 << lsb);
    }
    return D3D12ResourceStates;
}


static RESOURCE_STATE D3D12ResourceStateToResourceStateFlags(D3D12_RESOURCE_STATES state)
{
    static_assert(RESOURCE_STATE_MAX_BIT == 0x10000, "This function must be updated to handle new resource state flag");
    VERIFY((state & (state - 1)) == 0, "Only single state must be set");
    switch (state)
    {
        // clang-format off
        //case D3D12_RESOURCE_STATE_COMMON:
        case D3D12_RESOURCE_STATE_PRESENT:                    return RESOURCE_STATE_PRESENT;
        case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER: return static_cast<RESOURCE_STATE>(RESOURCE_STATE_VERTEX_BUFFER | RESOURCE_STATE_CONSTANT_BUFFER);
        case D3D12_RESOURCE_STATE_INDEX_BUFFER:               return RESOURCE_STATE_INDEX_BUFFER;
        case D3D12_RESOURCE_STATE_RENDER_TARGET:              return RESOURCE_STATE_RENDER_TARGET;
        case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:           return RESOURCE_STATE_UNORDERED_ACCESS;
        case D3D12_RESOURCE_STATE_DEPTH_WRITE:                return RESOURCE_STATE_DEPTH_WRITE;
        case D3D12_RESOURCE_STATE_DEPTH_READ:                 return RESOURCE_STATE_DEPTH_READ;
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:  return RESOURCE_STATE_SHADER_RESOURCE;
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:      return RESOURCE_STATE_SHADER_RESOURCE;
        case D3D12_RESOURCE_STATE_STREAM_OUT:                 return RESOURCE_STATE_STREAM_OUT;
        case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:          return RESOURCE_STATE_INDIRECT_ARGUMENT;
        case D3D12_RESOURCE_STATE_COPY_DEST:                  return RESOURCE_STATE_COPY_DEST;
        case D3D12_RESOURCE_STATE_COPY_SOURCE:                return RESOURCE_STATE_COPY_SOURCE;
        case D3D12_RESOURCE_STATE_RESOLVE_DEST:               return RESOURCE_STATE_RESOLVE_DEST;
        case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:             return RESOURCE_STATE_RESOLVE_SOURCE;
        // clang-format on
        default:
            UNEXPECTED("Unexpected D3D12 resource state");
            return RESOURCE_STATE_UNKNOWN;
    }
}


class D3D12StateFlagBitPosToResourceState
{
public:
    D3D12StateFlagBitPosToResourceState()
    {
        for (Uint32 bit = 0; bit <= MaxFlagBitPos; ++bit)
        {
            FlagBitPosToResStateMap[bit] = D3D12ResourceStateToResourceStateFlags(static_cast<D3D12_RESOURCE_STATES>(1 << bit));
        }
    }

    RESOURCE_STATE operator()(Uint32 BitPos) const
    {
        VERIFY(BitPos <= MaxFlagBitPos, "Resource state flag bit position (", BitPos, ") exceeds max bit position (", MaxFlagBitPos, ")");
        return FlagBitPosToResStateMap[BitPos];
    }

private:
    static constexpr Uint32                       MaxFlagBitPos = 13;
    std::array<RESOURCE_STATE, MaxFlagBitPos + 1> FlagBitPosToResStateMap;
};

RESOURCE_STATE D3D12ResourceStatesToResourceStateFlags(D3D12_RESOURCE_STATES StateFlags)
{
    if (StateFlags == D3D12_RESOURCE_STATE_PRESENT)
        return RESOURCE_STATE_PRESENT;

    static const D3D12StateFlagBitPosToResourceState BitPosToResState;

    Uint32 ResourceStates = 0;
    Uint32 Bits           = StateFlags;
    while (Bits != 0)
    {
        auto lsb = PlatformMisc::GetLSB(Bits);
        ResourceStates |= BitPosToResState(lsb);
        Bits &= ~(1 << lsb);
    }
    return static_cast<RESOURCE_STATE>(ResourceStates);
}

D3D12_QUERY_TYPE QueryTypeToD3D12QueryType(QUERY_TYPE QueryType)
{
    // clang-format off
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:           return D3D12_QUERY_TYPE_OCCLUSION;
        case QUERY_TYPE_BINARY_OCCLUSION:    return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
        case QUERY_TYPE_TIMESTAMP:           return D3D12_QUERY_TYPE_TIMESTAMP;
        case QUERY_TYPE_PIPELINE_STATISTICS: return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
        case QUERY_TYPE_DURATION:            return D3D12_QUERY_TYPE_TIMESTAMP;

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled");
        default:
            UNEXPECTED("Unexpected query type");
            return static_cast<D3D12_QUERY_TYPE>(-1);
    }
    // clang-format on
}

D3D12_QUERY_HEAP_TYPE QueryTypeToD3D12QueryHeapType(QUERY_TYPE QueryType)
{
    // clang-format off
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:           return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        case QUERY_TYPE_BINARY_OCCLUSION:    return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        case QUERY_TYPE_TIMESTAMP:           return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        case QUERY_TYPE_PIPELINE_STATISTICS: return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
        case QUERY_TYPE_DURATION:            return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled");
        default:
            UNEXPECTED("Unexpected query type");
            return static_cast<D3D12_QUERY_HEAP_TYPE>(-1);
    }
    // clang-format on
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE AttachmentLoadOpToD3D12BeginningAccessType(ATTACHMENT_LOAD_OP LoadOp)
{
    // clang-format off
    switch (LoadOp)
    {
        case ATTACHMENT_LOAD_OP_LOAD:    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        case ATTACHMENT_LOAD_OP_CLEAR:   return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        case ATTACHMENT_LOAD_OP_DISCARD: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;

        default:
            UNEXPECTED("Unexpected attachment load op");
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    }
    // clang-format on
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE AttachmentStoreOpToD3D12EndingAccessType(ATTACHMENT_STORE_OP StoreOp)
{
    // clang-format off
    switch (StoreOp)
    {
        case ATTACHMENT_STORE_OP_STORE:    return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        case ATTACHMENT_STORE_OP_DISCARD:  return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;

        default:
            UNEXPECTED("Unexpected attachment store op");
            return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    }
    // clang-format on
}

} // namespace Diligent
