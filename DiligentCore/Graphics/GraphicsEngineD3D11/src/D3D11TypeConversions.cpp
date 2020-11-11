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
#include "D3D11TypeConversions.hpp"

#include "D3D11TypeDefinitions.h"
#include "D3DTypeConversionImpl.hpp"
#include "D3DViewDescConversionImpl.hpp"

namespace Diligent
{

D3D11_FILTER FilterTypeToD3D11Filter(FILTER_TYPE MinFilter, FILTER_TYPE MagFilter, FILTER_TYPE MipFilter)
{
    return FilterTypeToD3DFilter<D3D11_FILTER>(MinFilter, MagFilter, MipFilter);
}

D3D11_TEXTURE_ADDRESS_MODE TexAddressModeToD3D11AddressMode(TEXTURE_ADDRESS_MODE Mode)
{
    return TexAddressModeToD3DAddressMode<D3D11_TEXTURE_ADDRESS_MODE>(Mode);
}

D3D11_COMPARISON_FUNC ComparisonFuncToD3D11ComparisonFunc(COMPARISON_FUNCTION Func)
{
    return ComparisonFuncToD3DComparisonFunc<D3D11_COMPARISON_FUNC>(Func);
}

void DepthStencilStateDesc_To_D3D11_DEPTH_STENCIL_DESC(const DepthStencilStateDesc& DepthStencilDesc,
                                                       D3D11_DEPTH_STENCIL_DESC&    d3d11DSSDesc)
{
    DepthStencilStateDesc_To_D3D_DEPTH_STENCIL_DESC<D3D11_DEPTH_STENCIL_DESC, D3D11_DEPTH_STENCILOP_DESC, D3D11_STENCIL_OP, D3D11_COMPARISON_FUNC>(DepthStencilDesc, d3d11DSSDesc);
}

void RasterizerStateDesc_To_D3D11_RASTERIZER_DESC(const RasterizerStateDesc& RasterizerDesc,
                                                  D3D11_RASTERIZER_DESC&     d3d11RSDesc)
{
    RasterizerStateDesc_To_D3D_RASTERIZER_DESC<D3D11_RASTERIZER_DESC, D3D11_FILL_MODE, D3D11_CULL_MODE>(RasterizerDesc, d3d11RSDesc);
    d3d11RSDesc.ScissorEnable = RasterizerDesc.ScissorEnable ? TRUE : FALSE;
}


void BlendStateDesc_To_D3D11_BLEND_DESC(const BlendStateDesc& BSDesc,
                                        D3D11_BLEND_DESC&     d3d11BSDesc)
{
    BlendStateDescToD3DBlendDesc<D3D11_BLEND_DESC, D3D11_BLEND, D3D11_BLEND_OP>(BSDesc, d3d11BSDesc);

    for (int i = 0; i < 8; ++i)
    {
        const auto& SrcRTDesc = BSDesc.RenderTargets[i];
        if (SrcRTDesc.LogicOperationEnable)
        {
            LOG_ERROR("Logical operations on render targets are not supported by D3D11 device");
        }
    }
}

void LayoutElements_To_D3D11_INPUT_ELEMENT_DESCs(const InputLayoutDesc&                                                               InputLayout,
                                                 std::vector<D3D11_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D11_INPUT_ELEMENT_DESC>>& D3D11InputElements)
{
    LayoutElements_To_D3D_INPUT_ELEMENT_DESCs<D3D11_INPUT_ELEMENT_DESC>(InputLayout, D3D11InputElements);
}

D3D11_PRIMITIVE_TOPOLOGY TopologyToD3D11Topology(PRIMITIVE_TOPOLOGY Topology)
{
    return TopologyToD3DTopology<D3D11_PRIMITIVE_TOPOLOGY>(Topology);
}



void TextureViewDesc_to_D3D11_SRV_DESC(const TextureViewDesc&           TexViewDesc,
                                       D3D11_SHADER_RESOURCE_VIEW_DESC& D3D11SRVDesc,
                                       Uint32                           SampleCount)
{
    TextureViewDesc_to_D3D_SRV_DESC(TexViewDesc, D3D11SRVDesc, SampleCount);
}

void TextureViewDesc_to_D3D11_RTV_DESC(const TextureViewDesc&         TexViewDesc,
                                       D3D11_RENDER_TARGET_VIEW_DESC& D3D11RTVDesc,
                                       Uint32                         SampleCount)
{
    TextureViewDesc_to_D3D_RTV_DESC(TexViewDesc, D3D11RTVDesc, SampleCount);
}

void TextureViewDesc_to_D3D11_DSV_DESC(const TextureViewDesc&         TexViewDesc,
                                       D3D11_DEPTH_STENCIL_VIEW_DESC& D3D11DSVDesc,
                                       Uint32                         SampleCount)
{
    TextureViewDesc_to_D3D_DSV_DESC(TexViewDesc, D3D11DSVDesc, SampleCount);
}

void TextureViewDesc_to_D3D11_UAV_DESC(const TextureViewDesc&            TexViewDesc,
                                       D3D11_UNORDERED_ACCESS_VIEW_DESC& D3D11UAVDesc)
{
    TextureViewDesc_to_D3D_UAV_DESC(TexViewDesc, D3D11UAVDesc);
}



void BufferViewDesc_to_D3D11_SRV_DESC(const BufferDesc&                BuffDesc,
                                      const BufferViewDesc&            SRVDesc,
                                      D3D11_SHADER_RESOURCE_VIEW_DESC& D3D11SRVDesc)
{
    if (BuffDesc.Mode == BUFFER_MODE_RAW && SRVDesc.Format.ValueType == VT_UNDEFINED)
    {
        // Raw buffer view
        UINT ElementByteStride = 4;
        DEV_CHECK_ERR((SRVDesc.ByteOffset % 16) == 0, "Byte offest (", SRVDesc.ByteOffset, ") is not multiple of 16");
        DEV_CHECK_ERR((SRVDesc.ByteWidth % ElementByteStride) == 0, "Byte width (", SRVDesc.ByteWidth, ") is not multiple of 4");
        D3D11SRVDesc.BufferEx.FirstElement = SRVDesc.ByteOffset / ElementByteStride;
        D3D11SRVDesc.BufferEx.NumElements  = SRVDesc.ByteWidth / ElementByteStride;
        D3D11SRVDesc.BufferEx.Flags        = D3D11_BUFFEREX_SRV_FLAG_RAW;
        D3D11SRVDesc.Format                = DXGI_FORMAT_R32_TYPELESS;
        D3D11SRVDesc.ViewDimension         = D3D_SRV_DIMENSION_BUFFEREX;
    }
    else
    {
        BufferViewDesc_to_D3D_SRV_DESC(BuffDesc, SRVDesc, D3D11SRVDesc);
    }
}

void BufferViewDesc_to_D3D11_UAV_DESC(const BufferDesc&                 BuffDesc,
                                      const BufferViewDesc&             UAVDesc,
                                      D3D11_UNORDERED_ACCESS_VIEW_DESC& D3D11UAVDesc)
{
    BufferViewDesc_to_D3D_UAV_DESC(BuffDesc, UAVDesc, D3D11UAVDesc);
}

} // namespace Diligent
