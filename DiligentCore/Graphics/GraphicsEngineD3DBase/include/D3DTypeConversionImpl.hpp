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

#include <array>

/// \file
/// Implementation of D3D type conversions

/// This file must be included after D3D11TypeDefinitions.h or D3D12TypeDefinitions.h

namespace Diligent
{

template <typename D3D_COMPARISON_FUNC>
inline D3D_COMPARISON_FUNC ComparisonFuncToD3DComparisonFunc(COMPARISON_FUNCTION Func)
{
    // D3D12_COMPARISON_FUNC is equal to D3D11_COMPARISON_FUNC
    switch (Func)
    {
        // clang-format off
        case COMPARISON_FUNC_UNKNOWN: UNEXPECTED("Comparison function is not specified" ); return D3D_COMPARISON_FUNC_ALWAYS;
        case COMPARISON_FUNC_NEVER:         return D3D_COMPARISON_FUNC_NEVER;
	    case COMPARISON_FUNC_LESS:          return D3D_COMPARISON_FUNC_LESS;
	    case COMPARISON_FUNC_EQUAL:         return D3D_COMPARISON_FUNC_EQUAL;
	    case COMPARISON_FUNC_LESS_EQUAL:    return D3D_COMPARISON_FUNC_LESS_EQUAL;
	    case COMPARISON_FUNC_GREATER:       return D3D_COMPARISON_FUNC_GREATER;
	    case COMPARISON_FUNC_NOT_EQUAL:     return D3D_COMPARISON_FUNC_NOT_EQUAL;
	    case COMPARISON_FUNC_GREATER_EQUAL: return D3D_COMPARISON_FUNC_GREATER_EQUAL;
	    case COMPARISON_FUNC_ALWAYS:        return D3D_COMPARISON_FUNC_ALWAYS;
        default: UNEXPECTED("Unknown comparison function" ); return D3D_COMPARISON_FUNC_ALWAYS;
            // clang-format on
    }
}


template <typename D3D_TEXTURE_ADDRESS_MODE>
D3D_TEXTURE_ADDRESS_MODE TexAddressModeToD3DAddressMode(TEXTURE_ADDRESS_MODE Mode)
{
    switch (Mode)
    {
        // clang-format off
        case TEXTURE_ADDRESS_UNKNOWN: UNEXPECTED("Texture address mode is not specified" ); return D3D_TEXTURE_ADDRESS_CLAMP;
        case TEXTURE_ADDRESS_WRAP:          return D3D_TEXTURE_ADDRESS_WRAP;
        case TEXTURE_ADDRESS_MIRROR:        return D3D_TEXTURE_ADDRESS_MIRROR;
        case TEXTURE_ADDRESS_CLAMP:         return D3D_TEXTURE_ADDRESS_CLAMP;
        case TEXTURE_ADDRESS_BORDER:        return D3D_TEXTURE_ADDRESS_BORDER;
        case TEXTURE_ADDRESS_MIRROR_ONCE:   return D3D_TEXTURE_ADDRESS_MIRROR_ONCE;
        default: UNEXPECTED("Unknown texture address mode" ); return D3D_TEXTURE_ADDRESS_CLAMP;
            // clang-format on
    }
}

template <typename D3D_PRIM_TOPOLOGY>
D3D_PRIM_TOPOLOGY TopologyToD3DTopology(PRIMITIVE_TOPOLOGY Topology)
{
    static bool                                                             bIsInit         = false;
    static std::array<D3D_PRIM_TOPOLOGY, PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES> d3dPrimTopology = {};
    if (!bIsInit)
    {
        // clang-format off
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_UNDEFINED]      =  D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_TRIANGLE_LIST]  =  D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] =  D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_POINT_LIST]     =  D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_LINE_LIST]      =  D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_LINE_STRIP]     =  D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST]  = D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST;
        d3dPrimTopology[PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST] = D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;
        // clang-format on
        bIsInit = true;
    }

    VERIFY_EXPR(Topology >= PRIMITIVE_TOPOLOGY_UNDEFINED && Topology < PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES);
    return d3dPrimTopology[Topology];
}


// ================= Rasterizer state attributes conversion functions =================

template <typename D3D_FILL_MODE>
D3D_FILL_MODE FillModeToD3DFillMode(FILL_MODE FillMode)
{
    // D3D12_FILL_MODE is identical tp D3D11_FILL_MODE
    static bool                                           bIsInit      = false;
    static std::array<D3D_FILL_MODE, FILL_MODE_NUM_MODES> d3dFillModes = {};
    if (!bIsInit)
    {
        d3dFillModes[FILL_MODE_WIREFRAME] = D3D_FILL_MODE_WIREFRAME;
        d3dFillModes[FILL_MODE_SOLID]     = D3D_FILL_MODE_SOLID;

        bIsInit = true;
    }
    if (FILL_MODE_UNDEFINED < FillMode && FillMode < FILL_MODE_NUM_MODES)
    {
        auto d3dFillMode = d3dFillModes[FillMode];
        VERIFY(d3dFillMode != 0, "Incorrect fill mode");
        return d3dFillMode;
    }
    else
    {
        UNEXPECTED("Incorrect fill mode (", FillMode, ")");
        return static_cast<D3D_FILL_MODE>(0);
    }
}

template <typename D3D_CULL_MODE>
D3D_CULL_MODE CullModeToD3DCullMode(CULL_MODE CullMode)
{
    // D3D_CULL_MODE is identical to D3D11_CULL_MODE
    static bool                                           bIsInit      = false;
    static std::array<D3D_CULL_MODE, CULL_MODE_NUM_MODES> d3dCullModes = {};
    if (!bIsInit)
    {
        d3dCullModes[CULL_MODE_NONE]  = D3D_CULL_MODE_NONE;
        d3dCullModes[CULL_MODE_FRONT] = D3D_CULL_MODE_FRONT;
        d3dCullModes[CULL_MODE_BACK]  = D3D_CULL_MODE_BACK;

        bIsInit = true;
    }

    if (CULL_MODE_UNDEFINED < CullMode && CullMode < CULL_MODE_NUM_MODES)
    {
        auto d3dCullMode = d3dCullModes[CullMode];
        VERIFY(d3dCullMode != 0, "Incorrect cull mode");
        return d3dCullMode;
    }
    else
    {
        UNEXPECTED("Incorrect cull mode (", CullMode, ")");
        return static_cast<D3D_CULL_MODE>(0);
    }
}

template <typename D3D_RASTERIZER_DESC, typename D3D_FILL_MODE, typename D3D_CULL_MODE>
void RasterizerStateDesc_To_D3D_RASTERIZER_DESC(const RasterizerStateDesc& RasterizerDesc, D3D_RASTERIZER_DESC& d3dRSDesc)
{
    d3dRSDesc.FillMode              = FillModeToD3DFillMode<D3D_FILL_MODE>(RasterizerDesc.FillMode);
    d3dRSDesc.CullMode              = CullModeToD3DCullMode<D3D_CULL_MODE>(RasterizerDesc.CullMode);
    d3dRSDesc.FrontCounterClockwise = RasterizerDesc.FrontCounterClockwise ? TRUE : FALSE;
    d3dRSDesc.DepthBias             = RasterizerDesc.DepthBias;
    d3dRSDesc.DepthBiasClamp        = RasterizerDesc.DepthBiasClamp;
    d3dRSDesc.SlopeScaledDepthBias  = RasterizerDesc.SlopeScaledDepthBias;
    d3dRSDesc.DepthClipEnable       = RasterizerDesc.DepthClipEnable ? TRUE : FALSE;

    //d3d12RSDesc.ScissorEnable         = RSDesc.ScissorEnable         ? TRUE : FALSE;

    d3dRSDesc.AntialiasedLineEnable = RasterizerDesc.AntialiasedLineEnable ? TRUE : FALSE;
    d3dRSDesc.MultisampleEnable     = d3dRSDesc.AntialiasedLineEnable;
}



// ================= Blend state attributes conversion functions =================

template <typename D3D_BLEND>
D3D_BLEND BlendFactorToD3DBlend(BLEND_FACTOR bf)
{
    // D3D11_BLEND and D3D12_BLEND are identical

    // Note that this code is safe for multithreaded environments since
    // bIsInit is set to true only AFTER the entire map is initialized.
    static bool                                            bIsInit  = false;
    static std::array<D3D_BLEND, BLEND_FACTOR_NUM_FACTORS> D3DBlend = {};
    if (!bIsInit)
    {
        // In a multithreaded environment, several threads can potentially enter
        // this block. This is not a problem since they will just initialize the
        // memory with the same values more than once

        // clang-format off
        D3DBlend[BLEND_FACTOR_ZERO            ] =  D3D_BLEND_ZERO;
        D3DBlend[BLEND_FACTOR_ONE             ] =  D3D_BLEND_ONE;
        D3DBlend[BLEND_FACTOR_SRC_COLOR       ] =  D3D_BLEND_SRC_COLOR;
        D3DBlend[BLEND_FACTOR_INV_SRC_COLOR   ] =  D3D_BLEND_INV_SRC_COLOR;
        D3DBlend[BLEND_FACTOR_SRC_ALPHA       ] =  D3D_BLEND_SRC_ALPHA;
        D3DBlend[BLEND_FACTOR_INV_SRC_ALPHA   ] =  D3D_BLEND_INV_SRC_ALPHA;
        D3DBlend[BLEND_FACTOR_DEST_ALPHA      ] =  D3D_BLEND_DEST_ALPHA;
        D3DBlend[BLEND_FACTOR_INV_DEST_ALPHA  ] =  D3D_BLEND_INV_DEST_ALPHA;
        D3DBlend[BLEND_FACTOR_DEST_COLOR      ] =  D3D_BLEND_DEST_COLOR;
        D3DBlend[BLEND_FACTOR_INV_DEST_COLOR  ] =  D3D_BLEND_INV_DEST_COLOR;
        D3DBlend[BLEND_FACTOR_SRC_ALPHA_SAT   ] =  D3D_BLEND_SRC_ALPHA_SAT;
        D3DBlend[BLEND_FACTOR_BLEND_FACTOR    ] =  D3D_BLEND_BLEND_FACTOR;
        D3DBlend[BLEND_FACTOR_INV_BLEND_FACTOR] =  D3D_BLEND_INV_BLEND_FACTOR;
        D3DBlend[BLEND_FACTOR_SRC1_COLOR      ] =  D3D_BLEND_SRC1_COLOR;
        D3DBlend[BLEND_FACTOR_INV_SRC1_COLOR  ] =  D3D_BLEND_INV_SRC1_COLOR;
        D3DBlend[BLEND_FACTOR_SRC1_ALPHA      ] =  D3D_BLEND_SRC1_ALPHA;
        D3DBlend[BLEND_FACTOR_INV_SRC1_ALPHA  ] =  D3D_BLEND_INV_SRC1_ALPHA;
        // clang-format on

        bIsInit = true;
    }
    if (bf > BLEND_FACTOR_UNDEFINED && bf < BLEND_FACTOR_NUM_FACTORS)
    {
        auto d3dbf = D3DBlend[bf];
        VERIFY(d3dbf != 0, "Incorrect blend factor");
        return d3dbf;
    }
    else
    {
        UNEXPECTED("Incorrect blend factor (", bf, ")");
        return static_cast<D3D_BLEND>(0);
    }
}

template <typename D3D_BLEND_OP>
D3D_BLEND_OP BlendOperationToD3DBlendOp(BLEND_OPERATION BlendOp)
{
    // D3D12_BLEND_OP and D3D11_BLEND_OP are identical

    static bool                                                     bIsInit    = false;
    static std::array<D3D_BLEND_OP, BLEND_OPERATION_NUM_OPERATIONS> D3DBlendOp = {};
    if (!bIsInit)
    {
        // clang-format off
        D3DBlendOp[BLEND_OPERATION_ADD         ] = D3D_BLEND_OP_ADD;
        D3DBlendOp[BLEND_OPERATION_SUBTRACT    ] = D3D_BLEND_OP_SUBTRACT;
        D3DBlendOp[BLEND_OPERATION_REV_SUBTRACT] = D3D_BLEND_OP_REV_SUBTRACT;
        D3DBlendOp[BLEND_OPERATION_MIN         ] = D3D_BLEND_OP_MIN;
        D3DBlendOp[BLEND_OPERATION_MAX         ] = D3D_BLEND_OP_MAX;
        // clang-format on

        bIsInit = true;
    }

    if (BlendOp > BLEND_OPERATION_UNDEFINED && BlendOp < BLEND_OPERATION_NUM_OPERATIONS)
    {
        auto d3dbop = D3DBlendOp[BlendOp];
        VERIFY(d3dbop != 0, "Incorrect blend operation");
        return d3dbop;
    }
    else
    {
        UNEXPECTED("Incorrect blend operation (", BlendOp, ")");
        return static_cast<D3D_BLEND_OP>(0);
    }
}

template <typename D3D_BLEND_DESC, typename D3D_BLEND, typename D3D_BLEND_OP>
void BlendStateDescToD3DBlendDesc(const BlendStateDesc& BSDesc, D3D_BLEND_DESC& d3d12BlendDesc)
{
    // D3D_BLEND_DESC and D3D11_BLEND_DESC structures are identical
    d3d12BlendDesc.AlphaToCoverageEnable  = BSDesc.AlphaToCoverageEnable ? TRUE : FALSE;
    d3d12BlendDesc.IndependentBlendEnable = BSDesc.IndependentBlendEnable ? TRUE : FALSE;
    VERIFY(MAX_RENDER_TARGETS >= 8, "Number of render targets is expected to be at least 8");
    for (int i = 0; i < 8; ++i)
    {
        const auto& SrcRTDesc = BSDesc.RenderTargets[i];
        auto&       DstRTDesc = d3d12BlendDesc.RenderTarget[i];
        DstRTDesc.BlendEnable = SrcRTDesc.BlendEnable ? TRUE : FALSE;

        DstRTDesc.SrcBlend  = BlendFactorToD3DBlend<D3D_BLEND>(SrcRTDesc.SrcBlend);
        DstRTDesc.DestBlend = BlendFactorToD3DBlend<D3D_BLEND>(SrcRTDesc.DestBlend);
        DstRTDesc.BlendOp   = BlendOperationToD3DBlendOp<D3D_BLEND_OP>(SrcRTDesc.BlendOp);

        DstRTDesc.SrcBlendAlpha  = BlendFactorToD3DBlend<D3D_BLEND>(SrcRTDesc.SrcBlendAlpha);
        DstRTDesc.DestBlendAlpha = BlendFactorToD3DBlend<D3D_BLEND>(SrcRTDesc.DestBlendAlpha);
        DstRTDesc.BlendOpAlpha   = BlendOperationToD3DBlendOp<D3D_BLEND_OP>(SrcRTDesc.BlendOpAlpha);

        DstRTDesc.RenderTargetWriteMask =
            ((SrcRTDesc.RenderTargetWriteMask & COLOR_MASK_RED) ? D3D_COLOR_WRITE_ENABLE_RED : 0) |
            ((SrcRTDesc.RenderTargetWriteMask & COLOR_MASK_GREEN) ? D3D_COLOR_WRITE_ENABLE_GREEN : 0) |
            ((SrcRTDesc.RenderTargetWriteMask & COLOR_MASK_BLUE) ? D3D_COLOR_WRITE_ENABLE_BLUE : 0) |
            ((SrcRTDesc.RenderTargetWriteMask & COLOR_MASK_ALPHA) ? D3D_COLOR_WRITE_ENABLE_ALPHA : 0);
    }
}



// ====================== Depth-stencil state attributes conversion functions ======================

template <typename D3D_STENCIL_OP>
D3D_STENCIL_OP StencilOpToD3DStencilOp(STENCIL_OP StencilOp)
{
    static bool                                           bIsInit          = false;
    static std::array<D3D_STENCIL_OP, STENCIL_OP_NUM_OPS> StOpToD3DStOpMap = {};
    if (!bIsInit)
    {
        // clang-format off
        StOpToD3DStOpMap[STENCIL_OP_KEEP     ] = D3D_STENCIL_OP_KEEP;
        StOpToD3DStOpMap[STENCIL_OP_ZERO     ] = D3D_STENCIL_OP_ZERO;
        StOpToD3DStOpMap[STENCIL_OP_REPLACE  ] = D3D_STENCIL_OP_REPLACE;
        StOpToD3DStOpMap[STENCIL_OP_INCR_SAT ] = D3D_STENCIL_OP_INCR_SAT;
        StOpToD3DStOpMap[STENCIL_OP_DECR_SAT ] = D3D_STENCIL_OP_DECR_SAT;
        StOpToD3DStOpMap[STENCIL_OP_INVERT   ] = D3D_STENCIL_OP_INVERT;
        StOpToD3DStOpMap[STENCIL_OP_INCR_WRAP] = D3D_STENCIL_OP_INCR;
        StOpToD3DStOpMap[STENCIL_OP_DECR_WRAP] = D3D_STENCIL_OP_DECR;
        // clang-format on

        bIsInit = true;
    }

    if (StencilOp > STENCIL_OP_UNDEFINED && StencilOp < STENCIL_OP_NUM_OPS)
    {
        auto d3dStencilOp = StOpToD3DStOpMap[StencilOp];
        VERIFY(d3dStencilOp != 0, "Unexpected stencil op");
        return d3dStencilOp;
    }
    else
    {
        UNEXPECTED("Stencil operation (", StencilOp, ") is out of allowed range [1, ", STENCIL_OP_NUM_OPS - 1, "]");
        return static_cast<D3D_STENCIL_OP>(0);
    }
}

template <typename D3D_DEPTH_STENCILOP_DESC, typename D3D_STENCIL_OP, typename D3D_COMPARISON_FUNC>
D3D_DEPTH_STENCILOP_DESC StencilOpDescToD3DStencilOpDesc(const StencilOpDesc& StOpDesc)
{
    // D3D12_DEPTH_STENCILOP_DESC is identical to D3D11_DEPTH_STENCILOP_DESC
    D3D_DEPTH_STENCILOP_DESC D3DStOpDesc;
    D3DStOpDesc.StencilFailOp      = StencilOpToD3DStencilOp<D3D_STENCIL_OP>(StOpDesc.StencilFailOp);
    D3DStOpDesc.StencilDepthFailOp = StencilOpToD3DStencilOp<D3D_STENCIL_OP>(StOpDesc.StencilDepthFailOp);
    D3DStOpDesc.StencilPassOp      = StencilOpToD3DStencilOp<D3D_STENCIL_OP>(StOpDesc.StencilPassOp);
    D3DStOpDesc.StencilFunc        = ComparisonFuncToD3DComparisonFunc<D3D_COMPARISON_FUNC>(StOpDesc.StencilFunc);
    return D3DStOpDesc;
}

template <typename D3D_DEPTH_STENCIL_DESC, typename D3D_DEPTH_STENCILOP_DESC, typename D3D_STENCIL_OP, typename D3D_COMPARISON_FUNC>
void DepthStencilStateDesc_To_D3D_DEPTH_STENCIL_DESC(const DepthStencilStateDesc& DepthStencilDesc, D3D_DEPTH_STENCIL_DESC& d3dDSSDesc)
{
    // D3D_DEPTH_STENCIL_DESC is identical to D3D11_DEPTH_STENCIL_DESC
    d3dDSSDesc.DepthEnable      = DepthStencilDesc.DepthEnable ? TRUE : FALSE;
    d3dDSSDesc.DepthWriteMask   = DepthStencilDesc.DepthWriteEnable ? D3D_DEPTH_WRITE_MASK_ALL : D3D_DEPTH_WRITE_MASK_ZERO;
    d3dDSSDesc.DepthFunc        = ComparisonFuncToD3DComparisonFunc<D3D_COMPARISON_FUNC>(DepthStencilDesc.DepthFunc);
    d3dDSSDesc.StencilEnable    = DepthStencilDesc.StencilEnable ? TRUE : FALSE;
    d3dDSSDesc.StencilReadMask  = DepthStencilDesc.StencilReadMask;
    d3dDSSDesc.StencilWriteMask = DepthStencilDesc.StencilWriteMask;
    d3dDSSDesc.FrontFace        = StencilOpDescToD3DStencilOpDesc<D3D_DEPTH_STENCILOP_DESC, D3D_STENCIL_OP, D3D_COMPARISON_FUNC>(DepthStencilDesc.FrontFace);
    d3dDSSDesc.BackFace         = StencilOpDescToD3DStencilOpDesc<D3D_DEPTH_STENCILOP_DESC, D3D_STENCIL_OP, D3D_COMPARISON_FUNC>(DepthStencilDesc.BackFace);
}



template <typename D3D_INPUT_ELEMENT_DESC>
void LayoutElements_To_D3D_INPUT_ELEMENT_DESCs(const InputLayoutDesc&                                                           InputLayout,
                                               std::vector<D3D_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D_INPUT_ELEMENT_DESC>>& D3DInputElements)
{
    // D3D12_INPUT_ELEMENT_DESC and D3D11_INPUT_ELEMENT_DESC are identical
    D3DInputElements.resize(InputLayout.NumElements);
    for (Uint32 iElem = 0; iElem < InputLayout.NumElements; ++iElem)
    {
        const auto& CurrElem         = InputLayout.LayoutElements[iElem];
        auto&       D3DElem          = D3DInputElements[iElem];
        D3DElem.SemanticName         = CurrElem.HLSLSemantic;
        D3DElem.SemanticIndex        = CurrElem.InputIndex;
        D3DElem.AlignedByteOffset    = CurrElem.RelativeOffset;
        D3DElem.InputSlot            = CurrElem.BufferSlot;
        D3DElem.Format               = TypeToDXGI_Format(CurrElem.ValueType, CurrElem.NumComponents, CurrElem.IsNormalized);
        D3DElem.InputSlotClass       = (CurrElem.Frequency == INPUT_ELEMENT_FREQUENCY_PER_VERTEX) ? D3D_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        D3DElem.InstanceDataStepRate = (CurrElem.Frequency == INPUT_ELEMENT_FREQUENCY_PER_VERTEX) ? 0 : CurrElem.InstanceDataStepRate;
    }
}


template <typename D3D_FILTER>
D3D_FILTER FilterTypeToD3DFilter(FILTER_TYPE MinFilter, FILTER_TYPE MagFilter, FILTER_TYPE MipFilter)
{
    switch (MinFilter)
    {
        // Regular filters
        case FILTER_TYPE_POINT:
            if (MagFilter == FILTER_TYPE_POINT)
            {
                if (MipFilter == FILTER_TYPE_POINT)
                    return D3D_FILTER_MIN_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_LINEAR)
                    return D3D_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_POINT)
                    return D3D_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_LINEAR)
                    return D3D_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_LINEAR:
            if (MagFilter == FILTER_TYPE_POINT)
            {
                if (MipFilter == FILTER_TYPE_POINT)
                    return D3D_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_LINEAR)
                    return D3D_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_POINT)
                    return D3D_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_LINEAR)
                    return D3D_FILTER_MIN_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_ANISOTROPIC:
            VERIFY(MagFilter == FILTER_TYPE_ANISOTROPIC && MipFilter == FILTER_TYPE_ANISOTROPIC,
                   "For anistropic filtering, all filters must be anisotropic");
            return D3D_FILTER_ANISOTROPIC;
            break;



        // Comparison filters
        case FILTER_TYPE_COMPARISON_POINT:
            if (MagFilter == FILTER_TYPE_COMPARISON_POINT)
            {
                if (MipFilter == FILTER_TYPE_COMPARISON_POINT)
                    return D3D_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_COMPARISON_LINEAR)
                    return D3D_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_COMPARISON_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_COMPARISON_POINT)
                    return D3D_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_COMPARISON_LINEAR)
                    return D3D_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_COMPARISON_LINEAR:
            if (MagFilter == FILTER_TYPE_COMPARISON_POINT)
            {
                if (MipFilter == FILTER_TYPE_COMPARISON_POINT)
                    return D3D_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_COMPARISON_LINEAR)
                    return D3D_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_COMPARISON_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_COMPARISON_POINT)
                    return D3D_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_COMPARISON_LINEAR)
                    return D3D_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
            VERIFY(MagFilter == FILTER_TYPE_COMPARISON_ANISOTROPIC && MipFilter == FILTER_TYPE_COMPARISON_ANISOTROPIC,
                   "For comparison anistropic filtering, all filters must be anisotropic");
            return D3D_FILTER_COMPARISON_ANISOTROPIC;
            break;



        // Minimum filters
        case FILTER_TYPE_MINIMUM_POINT:
            if (MagFilter == FILTER_TYPE_MINIMUM_POINT)
            {
                if (MipFilter == FILTER_TYPE_MINIMUM_POINT)
                    return D3D_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MINIMUM_LINEAR)
                    return D3D_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_MINIMUM_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_MINIMUM_POINT)
                    return D3D_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MINIMUM_LINEAR)
                    return D3D_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_MINIMUM_LINEAR:
            if (MagFilter == FILTER_TYPE_MINIMUM_POINT)
            {
                if (MipFilter == FILTER_TYPE_MINIMUM_POINT)
                    return D3D_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MINIMUM_LINEAR)
                    return D3D_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_MINIMUM_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_MINIMUM_POINT)
                    return D3D_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MINIMUM_LINEAR)
                    return D3D_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_MINIMUM_ANISOTROPIC:
            VERIFY(MagFilter == FILTER_TYPE_MINIMUM_ANISOTROPIC && MipFilter == FILTER_TYPE_MINIMUM_ANISOTROPIC,
                   "For minimum anistropic filtering, all filters must be anisotropic");
            return D3D_FILTER_MINIMUM_ANISOTROPIC;
            break;



        // Maximum filters
        case FILTER_TYPE_MAXIMUM_POINT:
            if (MagFilter == FILTER_TYPE_MAXIMUM_POINT)
            {
                if (MipFilter == FILTER_TYPE_MAXIMUM_POINT)
                    return D3D_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MAXIMUM_LINEAR)
                    return D3D_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_MAXIMUM_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_MAXIMUM_POINT)
                    return D3D_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MAXIMUM_LINEAR)
                    return D3D_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_MAXIMUM_LINEAR:
            if (MagFilter == FILTER_TYPE_MAXIMUM_POINT)
            {
                if (MipFilter == FILTER_TYPE_MAXIMUM_POINT)
                    return D3D_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MAXIMUM_LINEAR)
                    return D3D_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
            else if (MagFilter == FILTER_TYPE_MAXIMUM_LINEAR)
            {
                if (MipFilter == FILTER_TYPE_MAXIMUM_POINT)
                    return D3D_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
                else if (MipFilter == FILTER_TYPE_MAXIMUM_LINEAR)
                    return D3D_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
            }
            break;

        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:
            VERIFY(MagFilter == FILTER_TYPE_MAXIMUM_ANISOTROPIC && MipFilter == FILTER_TYPE_MAXIMUM_ANISOTROPIC,
                   "For maximum anistropic filtering, all filters must be anisotropic");
            return D3D_FILTER_MAXIMUM_ANISOTROPIC;
            break;
    }

    UNEXPECTED("Unsupported filter combination");
    return D3D_FILTER_MIN_MAG_MIP_POINT;
}

} // namespace Diligent
