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
/// Implementation of D3D type conversions

/// This file must be included after D3D11TypeDefinitions.h or D3D12TypeDefinitions.h

#include "DXGITypeConversions.hpp"

namespace Diligent
{

template <typename D3D_SHADER_RESOURCE_VIEW_DESC>
void TextureViewDesc_to_D3D_SRV_DESC(const TextureViewDesc& SRVDesc, D3D_SHADER_RESOURCE_VIEW_DESC& d3dSRVDesc, Uint32 SampleCount)
{
    memset(&d3dSRVDesc, 0, sizeof(d3dSRVDesc));
    d3dSRVDesc.Format = TexFormatToDXGI_Format(SRVDesc.Format, BIND_SHADER_RESOURCE);

    switch (SRVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            d3dSRVDesc.ViewDimension             = D3D_SRV_DIMENSION_TEXTURE1D;
            d3dSRVDesc.Texture1D.MipLevels       = SRVDesc.NumMipLevels;
            d3dSRVDesc.Texture1D.MostDetailedMip = SRVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            d3dSRVDesc.ViewDimension                  = D3D_SRV_DIMENSION_TEXTURE1DARRAY;
            d3dSRVDesc.Texture1DArray.ArraySize       = SRVDesc.NumArraySlices;
            d3dSRVDesc.Texture1DArray.FirstArraySlice = SRVDesc.FirstArraySlice;
            d3dSRVDesc.Texture1DArray.MipLevels       = SRVDesc.NumMipLevels;
            d3dSRVDesc.Texture1DArray.MostDetailedMip = SRVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_2D:
            if (SampleCount > 1)
            {
                d3dSRVDesc.ViewDimension                           = D3D_SRV_DIMENSION_TEXTURE2DMS;
                d3dSRVDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
            else
            {
                d3dSRVDesc.ViewDimension             = D3D_SRV_DIMENSION_TEXTURE2D;
                d3dSRVDesc.Texture2D.MipLevels       = SRVDesc.NumMipLevels;
                d3dSRVDesc.Texture2D.MostDetailedMip = SRVDesc.MostDetailedMip;
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (SampleCount > 1)
            {
                d3dSRVDesc.ViewDimension                    = D3D_SRV_DIMENSION_TEXTURE2DMSARRAY;
                d3dSRVDesc.Texture2DMSArray.ArraySize       = SRVDesc.NumArraySlices;
                d3dSRVDesc.Texture2DMSArray.FirstArraySlice = SRVDesc.FirstArraySlice;
            }
            else
            {
                d3dSRVDesc.ViewDimension                  = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                d3dSRVDesc.Texture2DArray.ArraySize       = SRVDesc.NumArraySlices;
                d3dSRVDesc.Texture2DArray.FirstArraySlice = SRVDesc.FirstArraySlice;
                d3dSRVDesc.Texture2DArray.MipLevels       = SRVDesc.NumMipLevels;
                d3dSRVDesc.Texture2DArray.MostDetailedMip = SRVDesc.MostDetailedMip;
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            d3dSRVDesc.ViewDimension             = D3D_SRV_DIMENSION_TEXTURE3D;
            d3dSRVDesc.Texture3D.MipLevels       = SRVDesc.NumMipLevels;
            d3dSRVDesc.Texture3D.MostDetailedMip = SRVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_CUBE:
            d3dSRVDesc.ViewDimension               = D3D_SRV_DIMENSION_TEXTURECUBE;
            d3dSRVDesc.TextureCube.MipLevels       = SRVDesc.NumMipLevels;
            d3dSRVDesc.TextureCube.MostDetailedMip = SRVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            d3dSRVDesc.ViewDimension                     = D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
            d3dSRVDesc.TextureCubeArray.MipLevels        = SRVDesc.NumMipLevels;
            d3dSRVDesc.TextureCubeArray.MostDetailedMip  = SRVDesc.MostDetailedMip;
            d3dSRVDesc.TextureCubeArray.First2DArrayFace = SRVDesc.FirstArraySlice;
            d3dSRVDesc.TextureCubeArray.NumCubes         = SRVDesc.NumArraySlices / 6;
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

template <typename D3D_RENDER_TARGET_VIEW_DESC>
void TextureViewDesc_to_D3D_RTV_DESC(const TextureViewDesc& RTVDesc, D3D_RENDER_TARGET_VIEW_DESC& d3dRTVDesc, Uint32 SampleCount)
{
    memset(&d3dRTVDesc, 0, sizeof(d3dRTVDesc));
    d3dRTVDesc.Format = TexFormatToDXGI_Format(RTVDesc.Format, BIND_RENDER_TARGET);

    switch (RTVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            d3dRTVDesc.ViewDimension      = D3D_RTV_DIMENSION_TEXTURE1D;
            d3dRTVDesc.Texture1D.MipSlice = RTVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            d3dRTVDesc.ViewDimension                  = D3D_RTV_DIMENSION_TEXTURE1DARRAY;
            d3dRTVDesc.Texture1DArray.ArraySize       = RTVDesc.NumArraySlices;
            d3dRTVDesc.Texture1DArray.FirstArraySlice = RTVDesc.FirstArraySlice;
            d3dRTVDesc.Texture1DArray.MipSlice        = RTVDesc.MostDetailedMip;
            break;


        case RESOURCE_DIM_TEX_2D:
            if (SampleCount > 1)
            {
                d3dRTVDesc.ViewDimension                           = D3D_RTV_DIMENSION_TEXTURE2DMS;
                d3dRTVDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
            else
            {
                d3dRTVDesc.ViewDimension      = D3D_RTV_DIMENSION_TEXTURE2D;
                d3dRTVDesc.Texture2D.MipSlice = RTVDesc.MostDetailedMip;
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (SampleCount > 1)
            {
                d3dRTVDesc.ViewDimension                    = D3D_RTV_DIMENSION_TEXTURE2DMSARRAY;
                d3dRTVDesc.Texture2DMSArray.ArraySize       = RTVDesc.NumArraySlices;
                d3dRTVDesc.Texture2DMSArray.FirstArraySlice = RTVDesc.FirstArraySlice;
            }
            else
            {
                d3dRTVDesc.ViewDimension                  = D3D_RTV_DIMENSION_TEXTURE2DARRAY;
                d3dRTVDesc.Texture2DArray.ArraySize       = RTVDesc.NumArraySlices;
                d3dRTVDesc.Texture2DArray.FirstArraySlice = RTVDesc.FirstArraySlice;
                d3dRTVDesc.Texture2DArray.MipSlice        = RTVDesc.MostDetailedMip;
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            d3dRTVDesc.ViewDimension         = D3D_RTV_DIMENSION_TEXTURE3D;
            d3dRTVDesc.Texture3D.FirstWSlice = RTVDesc.FirstDepthSlice;
            d3dRTVDesc.Texture3D.WSize       = RTVDesc.NumDepthSlices;
            d3dRTVDesc.Texture3D.MipSlice    = RTVDesc.MostDetailedMip;
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

template <typename D3D_DEPTH_STENCIL_VIEW_DESC>
void TextureViewDesc_to_D3D_DSV_DESC(const TextureViewDesc& DSVDesc, D3D_DEPTH_STENCIL_VIEW_DESC& d3dDSVDesc, Uint32 SampleCount)
{
    memset(&d3dDSVDesc, 0, sizeof(d3dDSVDesc));
    d3dDSVDesc.Format = TexFormatToDXGI_Format(DSVDesc.Format, BIND_DEPTH_STENCIL);

    switch (DSVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            d3dDSVDesc.ViewDimension      = D3D_DSV_DIMENSION_TEXTURE1D;
            d3dDSVDesc.Texture1D.MipSlice = DSVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            d3dDSVDesc.ViewDimension                  = D3D_DSV_DIMENSION_TEXTURE1DARRAY;
            d3dDSVDesc.Texture1DArray.ArraySize       = DSVDesc.NumArraySlices;
            d3dDSVDesc.Texture1DArray.FirstArraySlice = DSVDesc.FirstArraySlice;
            d3dDSVDesc.Texture1DArray.MipSlice        = DSVDesc.MostDetailedMip;
            break;


        case RESOURCE_DIM_TEX_2D:
            if (SampleCount > 1)
            {
                d3dDSVDesc.ViewDimension                           = D3D_DSV_DIMENSION_TEXTURE2DMS;
                d3dDSVDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
            else
            {
                d3dDSVDesc.ViewDimension      = D3D_DSV_DIMENSION_TEXTURE2D;
                d3dDSVDesc.Texture2D.MipSlice = DSVDesc.MostDetailedMip;
            }
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            if (SampleCount > 1)
            {
                d3dDSVDesc.ViewDimension                    = D3D_DSV_DIMENSION_TEXTURE2DMSARRAY;
                d3dDSVDesc.Texture2DMSArray.ArraySize       = DSVDesc.NumArraySlices;
                d3dDSVDesc.Texture2DMSArray.FirstArraySlice = DSVDesc.FirstArraySlice;
            }
            else
            {
                d3dDSVDesc.ViewDimension                  = D3D_DSV_DIMENSION_TEXTURE2DARRAY;
                d3dDSVDesc.Texture2DArray.ArraySize       = DSVDesc.NumArraySlices;
                d3dDSVDesc.Texture2DArray.FirstArraySlice = DSVDesc.FirstArraySlice;
                d3dDSVDesc.Texture2DArray.MipSlice        = DSVDesc.MostDetailedMip;
            }
            break;

        case RESOURCE_DIM_TEX_3D:
            LOG_ERROR_AND_THROW("Depth stencil views are not supported for 3D textures");
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

template <typename D3D_UNORDERED_ACCESS_VIEW_DESC>
void TextureViewDesc_to_D3D_UAV_DESC(const TextureViewDesc& UAVDesc, D3D_UNORDERED_ACCESS_VIEW_DESC& d3dUAVDesc)
{
    memset(&d3dUAVDesc, 0, sizeof(d3dUAVDesc));
    d3dUAVDesc.Format = TexFormatToDXGI_Format(UAVDesc.Format, BIND_UNORDERED_ACCESS);

    switch (UAVDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            d3dUAVDesc.ViewDimension      = D3D_UAV_DIMENSION_TEXTURE1D;
            d3dUAVDesc.Texture1D.MipSlice = UAVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            d3dUAVDesc.ViewDimension                  = D3D_UAV_DIMENSION_TEXTURE1DARRAY;
            d3dUAVDesc.Texture1DArray.ArraySize       = UAVDesc.NumArraySlices;
            d3dUAVDesc.Texture1DArray.FirstArraySlice = UAVDesc.FirstArraySlice;
            d3dUAVDesc.Texture1DArray.MipSlice        = UAVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_2D:
            d3dUAVDesc.ViewDimension      = D3D_UAV_DIMENSION_TEXTURE2D;
            d3dUAVDesc.Texture2D.MipSlice = UAVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            d3dUAVDesc.ViewDimension                  = D3D_UAV_DIMENSION_TEXTURE2DARRAY;
            d3dUAVDesc.Texture2DArray.ArraySize       = UAVDesc.NumArraySlices;
            d3dUAVDesc.Texture2DArray.FirstArraySlice = UAVDesc.FirstArraySlice;
            d3dUAVDesc.Texture2DArray.MipSlice        = UAVDesc.MostDetailedMip;
            break;

        case RESOURCE_DIM_TEX_3D:
            d3dUAVDesc.ViewDimension         = D3D_UAV_DIMENSION_TEXTURE3D;
            d3dUAVDesc.Texture3D.FirstWSlice = UAVDesc.FirstDepthSlice;
            d3dUAVDesc.Texture3D.WSize       = UAVDesc.NumDepthSlices;
            d3dUAVDesc.Texture3D.MipSlice    = UAVDesc.MostDetailedMip;
            break;

        default:
            UNEXPECTED("Unexpected view type");
    }
}

template <typename D3D_UNORDERED_ACCESS_VIEW_DESC>
void BufferViewDesc_to_D3D_SRV_DESC(const BufferDesc& BuffDesc, const BufferViewDesc& SRVDesc, D3D_UNORDERED_ACCESS_VIEW_DESC& d3dSRVDesc)
{
    VERIFY(SRVDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE, "Incorrect view type: shader resource is expected");

    memset(&d3dSRVDesc, 0, sizeof(d3dSRVDesc));
    const auto& BuffFmt = SRVDesc.Format;
    if (BuffDesc.Mode == BUFFER_MODE_FORMATTED || BuffDesc.Mode == BUFFER_MODE_RAW && BuffFmt.ValueType != VT_UNDEFINED)
        d3dSRVDesc.Format = TypeToDXGI_Format(BuffFmt.ValueType, BuffFmt.NumComponents, BuffFmt.IsNormalized);

    Uint32 ElementByteStride = 0;
    if (BuffDesc.Mode == BUFFER_MODE_FORMATTED || BuffDesc.Mode == BUFFER_MODE_STRUCTURED || BuffDesc.Mode == BUFFER_MODE_RAW && BuffFmt.ValueType != VT_UNDEFINED)
        ElementByteStride = BuffDesc.ElementByteStride;
    else if (BuffDesc.Mode == BUFFER_MODE_RAW && BuffFmt.ValueType == VT_UNDEFINED)
        ElementByteStride = 4;

    if (ElementByteStride != 0)
    {
        DEV_CHECK_ERR((SRVDesc.ByteOffset % ElementByteStride) == 0, "Byte offest (", SRVDesc.ByteOffset, ") is not multiple of element byte stride (", ElementByteStride, ")");
        DEV_CHECK_ERR((SRVDesc.ByteWidth % ElementByteStride) == 0, "Byte width (", SRVDesc.ByteWidth, ")is not multiple of element byte stride (", ElementByteStride, ")");
        d3dSRVDesc.Buffer.FirstElement = SRVDesc.ByteOffset / ElementByteStride;
        d3dSRVDesc.Buffer.NumElements  = SRVDesc.ByteWidth / ElementByteStride;
    }
    d3dSRVDesc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
}

template <typename D3D_UNORDERED_ACCESS_VIEW_DESC>
void BufferViewDesc_to_D3D_UAV_DESC(const BufferDesc& BuffDesc, const BufferViewDesc& UAVDesc, D3D_UNORDERED_ACCESS_VIEW_DESC& d3dUAVDesc)
{
    VERIFY(UAVDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS, "Incorrect view type: unordered access is expected");

    memset(&d3dUAVDesc, 0, sizeof(d3dUAVDesc));
    const auto& BuffFmt = UAVDesc.Format;
    if (BuffDesc.Mode == BUFFER_MODE_FORMATTED || BuffDesc.Mode == BUFFER_MODE_RAW && BuffFmt.ValueType != VT_UNDEFINED)
        d3dUAVDesc.Format = TypeToDXGI_Format(BuffFmt.ValueType, BuffFmt.NumComponents, BuffFmt.IsNormalized);

    Uint32 ElementByteStride = 0;
    if (BuffDesc.Mode == BUFFER_MODE_FORMATTED || BuffDesc.Mode == BUFFER_MODE_STRUCTURED || BuffDesc.Mode == BUFFER_MODE_RAW && BuffFmt.ValueType != VT_UNDEFINED)
        ElementByteStride = BuffDesc.ElementByteStride;
    else if (BuffDesc.Mode == BUFFER_MODE_RAW && BuffFmt.ValueType == VT_UNDEFINED)
        ElementByteStride = 4;

    if (ElementByteStride != 0)
    {
        DEV_CHECK_ERR((UAVDesc.ByteOffset % ElementByteStride) == 0, "Byte offest (", UAVDesc.ByteOffset, ") is not multiple of element byte stride (", ElementByteStride, ")");
        DEV_CHECK_ERR((UAVDesc.ByteWidth % ElementByteStride) == 0, "Byte width (", UAVDesc.ByteWidth, ")is not multiple of element byte stride (", ElementByteStride, ")");
        d3dUAVDesc.Buffer.FirstElement = UAVDesc.ByteOffset / ElementByteStride;
        d3dUAVDesc.Buffer.NumElements  = UAVDesc.ByteWidth / ElementByteStride;
    }

    if (BuffDesc.Mode == BUFFER_MODE_RAW && UAVDesc.Format.ValueType == VT_UNDEFINED)
    {
        d3dUAVDesc.Format       = DXGI_FORMAT_R32_TYPELESS;
        d3dUAVDesc.Buffer.Flags = D3D_BUFFER_UAV_FLAG_RAW;
    }
    else
        d3dUAVDesc.Buffer.Flags = D3D_BUFFER_UAV_FLAG_NONE;

    d3dUAVDesc.ViewDimension = D3D_UAV_DIMENSION_BUFFER;
}

} // namespace Diligent
