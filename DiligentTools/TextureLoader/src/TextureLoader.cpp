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
#include <algorithm>
#include <limits>
#include <math.h>
#include <vector>

#include "TextureLoader.h"
#include "GraphicsAccessories.hpp"
#include "DDSLoader.h"
#include "PNGCodec.h"
#include "JPEGCodec.h"
#include "ColorConversion.h"
#include "Image.h"

extern "C"
{
    Diligent::DECODE_PNG_RESULT Diligent_DecodePng(Diligent::IDataBlob* pSrcPngBits,
                                                   Diligent::IDataBlob* pDstPixels,
                                                   Diligent::ImageDesc* pDstImgDesc);

    Diligent::ENCODE_PNG_RESULT Diligent_EncodePng(const Diligent::Uint8* pSrcPixels,
                                                   Diligent::Uint32       Width,
                                                   Diligent::Uint32       Height,
                                                   Diligent::Uint32       StrideInBytes,
                                                   int                    PngColorType,
                                                   Diligent::IDataBlob*   pDstPngBits);

    Diligent::DECODE_JPEG_RESULT Diligent_DecodeJpeg(Diligent::IDataBlob* pSrcJpegBits,
                                                     Diligent::IDataBlob* pDstPixels,
                                                     Diligent::ImageDesc* pDstImgDesc);

    Diligent::ENCODE_JPEG_RESULT Diligent_EncodeJpeg(Diligent::Uint8*     pSrcRGBData,
                                                     Diligent::Uint32     Width,
                                                     Diligent::Uint32     Height,
                                                     int                  quality,
                                                     Diligent::IDataBlob* pDstJpegBits);
}

namespace Diligent
{

template <typename ChannelType>
ChannelType SRGBAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3)
{
    static constexpr float NormVal = static_cast<float>(std::numeric_limits<ChannelType>::max());

    float fc0 = static_cast<float>(c0) / NormVal;
    float fc1 = static_cast<float>(c1) / NormVal;
    float fc2 = static_cast<float>(c2) / NormVal;
    float fc3 = static_cast<float>(c3) / NormVal;

    float fLinearAverage = (SRGBToLinear(fc0) + SRGBToLinear(fc1) + SRGBToLinear(fc2) + SRGBToLinear(fc3)) / 4.f;
    float fSRGBAverage   = LinearToSRGB(fLinearAverage) * NormVal;

    static constexpr float MinVal = static_cast<float>(std::numeric_limits<ChannelType>::min());
    static constexpr float MaxVal = static_cast<float>(std::numeric_limits<ChannelType>::max());

    fSRGBAverage = std::max(fSRGBAverage, MinVal);
    fSRGBAverage = std::min(fSRGBAverage, MaxVal);

    return static_cast<ChannelType>(fSRGBAverage);
}

template <typename ChannelType>
ChannelType LinearAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3)
{
    static_assert(std::numeric_limits<ChannelType>::is_integer && !std::numeric_limits<ChannelType>::is_signed, "Unsigned integers are expected");
    return static_cast<ChannelType>((static_cast<Uint32>(c0) + static_cast<Uint32>(c1) + static_cast<Uint32>(c2) + static_cast<Uint32>(c3)) / 4);
}

template <typename ChannelType>
void ComputeCoarseMip(Uint32      NumChannels,
                      bool        IsSRGB,
                      const void* pFineMip,
                      Uint32      FineMipStride,
                      Uint32      FineMipWidth,
                      Uint32      FineMipHeight,
                      void*       pCoarseMip,
                      Uint32      CoarseMipStride,
                      Uint32      CoarseMipWidth,
                      Uint32      CoarseMipHeight)
{
    VERIFY_EXPR(FineMipWidth > 0 && FineMipHeight > 0 && FineMipStride > 0);
    VERIFY_EXPR(CoarseMipWidth > 0 && CoarseMipHeight > 0 && CoarseMipStride > 0);

    for (Uint32 row = 0; row < CoarseMipHeight; ++row)
    {
        auto src_row0 = row * 2;
        auto src_row1 = std::min(row * 2 + 1, FineMipHeight - 1);

        auto pSrcRow0 = reinterpret_cast<const ChannelType*>(reinterpret_cast<const Uint8*>(pFineMip) + src_row0 * FineMipStride);
        auto pSrcRow1 = reinterpret_cast<const ChannelType*>(reinterpret_cast<const Uint8*>(pFineMip) + src_row1 * FineMipStride);

        for (Uint32 col = 0; col < CoarseMipWidth; ++col)
        {
            auto src_col0 = col * 2;
            auto src_col1 = std::min(col * 2 + 1, FineMipWidth - 1);

            for (Uint32 c = 0; c < NumChannels; ++c)
            {
                auto Chnl00 = pSrcRow0[src_col0 * NumChannels + c];
                auto Chnl01 = pSrcRow0[src_col1 * NumChannels + c];
                auto Chnl10 = pSrcRow1[src_col0 * NumChannels + c];
                auto Chnl11 = pSrcRow1[src_col1 * NumChannels + c];

                auto& DstCol = reinterpret_cast<ChannelType*>(reinterpret_cast<Uint8*>(pCoarseMip) + row * CoarseMipStride)[col * NumChannels + c];
                if (IsSRGB)
                    DstCol = SRGBAverage(Chnl00, Chnl01, Chnl10, Chnl11);
                else
                    DstCol = LinearAverage(Chnl00, Chnl01, Chnl10, Chnl11);
            }
        }
    }
}

template <typename ChannelType>
void RGBToRGBA(const void* pRGBData,
               Uint32      RGBStride,
               void*       pRGBAData,
               Uint32      RGBAStride,
               Uint32      Width,
               Uint32      Height)
{
    for (size_t row = 0; row < size_t{Height}; ++row)
        for (size_t col = 0; col < size_t{Width}; ++col)
        {
            for (int c = 0; c < 3; ++c)
            {
                reinterpret_cast<ChannelType*>((reinterpret_cast<Uint8*>(pRGBAData) + size_t{RGBAStride} * row))[col * 4 + c] =
                    reinterpret_cast<const ChannelType*>((reinterpret_cast<const Uint8*>(pRGBData) + size_t{RGBStride} * row))[col * 3 + c];
            }
            reinterpret_cast<ChannelType*>((reinterpret_cast<Uint8*>(pRGBAData) + size_t{RGBAStride} * row))[col * 4 + 3] = std::numeric_limits<ChannelType>::max();
        }
}

void CreateTextureFromImage(Image*                 pSrcImage,
                            const TextureLoadInfo& TexLoadInfo,
                            IRenderDevice*         pDevice,
                            ITexture**             ppTexture)
{
    const auto& ImgDesc = pSrcImage->GetDesc();
    TextureDesc TexDesc;
    TexDesc.Name      = TexLoadInfo.Name;
    TexDesc.Type      = RESOURCE_DIM_TEX_2D;
    TexDesc.Width     = ImgDesc.Width;
    TexDesc.Height    = ImgDesc.Height;
    TexDesc.MipLevels = ComputeMipLevelsCount(TexDesc.Width, TexDesc.Height);
    if (TexLoadInfo.MipLevels > 0)
        TexDesc.MipLevels = std::min(TexDesc.MipLevels, TexLoadInfo.MipLevels);
    TexDesc.Usage          = TexLoadInfo.Usage;
    TexDesc.BindFlags      = TexLoadInfo.BindFlags;
    TexDesc.Format         = TexLoadInfo.Format;
    TexDesc.CPUAccessFlags = TexLoadInfo.CPUAccessFlags;
    auto ChannelDepth      = GetValueSize(ImgDesc.ComponentType) * 8;

    Uint32 NumComponents = ImgDesc.NumComponents == 3 ? 4 : ImgDesc.NumComponents;
    bool   IsSRGB        = (ImgDesc.NumComponents >= 3 && ChannelDepth == 8) ? TexLoadInfo.IsSRGB : false;
    if (TexDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        if (ChannelDepth == 8)
        {
            switch (NumComponents)
            {
                case 1: TexDesc.Format = TEX_FORMAT_R8_UNORM; break;
                case 2: TexDesc.Format = TEX_FORMAT_RG8_UNORM; break;
                case 4: TexDesc.Format = IsSRGB ? TEX_FORMAT_RGBA8_UNORM_SRGB : TEX_FORMAT_RGBA8_UNORM; break;
                default: LOG_ERROR_AND_THROW("Unexpected number of color channels (", ImgDesc.NumComponents, ")");
            }
        }
        else if (ChannelDepth == 16)
        {
            switch (NumComponents)
            {
                case 1: TexDesc.Format = TEX_FORMAT_R16_UNORM; break;
                case 2: TexDesc.Format = TEX_FORMAT_RG16_UNORM; break;
                case 4: TexDesc.Format = TEX_FORMAT_RGBA16_UNORM; break;
                default: LOG_ERROR_AND_THROW("Unexpected number of color channels (", ImgDesc.NumComponents, ")");
            }
        }
        else
            LOG_ERROR_AND_THROW("Unsupported color channel depth (", ChannelDepth, ")");
    }
    else
    {
        const auto& TexFmtDesc = GetTextureFormatAttribs(TexDesc.Format);
        if (TexFmtDesc.NumComponents != NumComponents)
            LOG_ERROR_AND_THROW("Incorrect number of components ", ImgDesc.NumComponents, ") for texture format ", TexFmtDesc.Name);
        if (TexFmtDesc.ComponentSize != ChannelDepth / 8)
            LOG_ERROR_AND_THROW("Incorrect channel size ", ChannelDepth, ") for texture format ", TexFmtDesc.Name);
    }


    std::vector<TextureSubResData>  pSubResources(TexDesc.MipLevels);
    std::vector<std::vector<Uint8>> Mips(TexDesc.MipLevels);

    if (ImgDesc.NumComponents == 3)
    {
        VERIFY_EXPR(NumComponents == 4);
        auto RGBAStride = ImgDesc.Width * NumComponents * ChannelDepth / 8;
        RGBAStride      = (RGBAStride + 3) & (-4);
        Mips[0].resize(size_t{RGBAStride} * size_t{ImgDesc.Height});
        pSubResources[0].pData  = Mips[0].data();
        pSubResources[0].Stride = RGBAStride;
        if (ChannelDepth == 8)
        {
            RGBToRGBA<Uint8>(pSrcImage->GetData()->GetDataPtr(), ImgDesc.RowStride,
                             Mips[0].data(), RGBAStride,
                             ImgDesc.Width, ImgDesc.Height);
        }
        else if (ChannelDepth == 16)
        {
            RGBToRGBA<Uint16>(pSrcImage->GetData()->GetDataPtr(), ImgDesc.RowStride,
                              Mips[0].data(), RGBAStride,
                              ImgDesc.Width, ImgDesc.Height);
        }
    }
    else
    {
        pSubResources[0].pData  = pSrcImage->GetData()->GetDataPtr();
        pSubResources[0].Stride = ImgDesc.RowStride;
    }

    auto MipWidth  = TexDesc.Width;
    auto MipHeight = TexDesc.Height;
    for (Uint32 m = 1; m < TexDesc.MipLevels; ++m)
    {
        auto CoarseMipWidth  = std::max(MipWidth / 2u, 1u);
        auto CoarseMipHeight = std::max(MipHeight / 2u, 1u);
        auto CoarseMipStride = CoarseMipWidth * NumComponents * ChannelDepth / 8;
        CoarseMipStride      = (CoarseMipStride + 3) & (-4);
        Mips[m].resize(size_t{CoarseMipStride} * size_t{CoarseMipHeight});

        if (TexLoadInfo.GenerateMips)
        {
            if (ChannelDepth == 8)
            {
                ComputeCoarseMip<Uint8>(NumComponents, IsSRGB,
                                        pSubResources[m - 1].pData, pSubResources[m - 1].Stride,
                                        MipWidth, MipHeight,
                                        Mips[m].data(), CoarseMipStride,
                                        CoarseMipWidth, CoarseMipHeight);
            }
            else if (ChannelDepth == 16)
            {
                ComputeCoarseMip<Uint16>(NumComponents, IsSRGB,
                                         pSubResources[m - 1].pData, pSubResources[m - 1].Stride,
                                         MipWidth, MipHeight,
                                         Mips[m].data(), CoarseMipStride,
                                         CoarseMipWidth, CoarseMipHeight);
            }
        }

        pSubResources[m].pData  = Mips[m].data();
        pSubResources[m].Stride = CoarseMipStride;

        MipWidth  = CoarseMipWidth;
        MipHeight = CoarseMipHeight;
    }

    TextureData TexData;
    TexData.pSubResources   = pSubResources.data();
    TexData.NumSubresources = TexDesc.MipLevels;

    pDevice->CreateTexture(TexDesc, &TexData, ppTexture);
}

void CreateTextureFromDDS(IDataBlob*             pDDSData,
                          const TextureLoadInfo& TexLoadInfo,
                          IRenderDevice*         pDevice,
                          ITexture**             ppTexture)
{
    CreateDDSTextureFromMemoryEx(pDevice,
                                 reinterpret_cast<const Uint8*>(pDDSData->GetDataPtr()),
                                 static_cast<size_t>(pDDSData->GetSize()),
                                 0, // maxSize
                                 TexLoadInfo.Usage,
                                 TexLoadInfo.Name,
                                 TexLoadInfo.BindFlags,
                                 TexLoadInfo.CPUAccessFlags,
                                 MISC_TEXTURE_FLAG_NONE, // miscFlags
                                 TexLoadInfo.IsSRGB,     // forceSRGB
                                 ppTexture);
}

DECODE_PNG_RESULT DecodePng(IDataBlob* pSrcPngBits,
                            IDataBlob* pDstPixels,
                            ImageDesc* pDstImgDesc)
{
    return Diligent_DecodePng(pSrcPngBits, pDstPixels, pDstImgDesc);
}

ENCODE_PNG_RESULT EncodePng(const Uint8* pSrcPixels,
                            Uint32       Width,
                            Uint32       Height,
                            Uint32       StrideInBytes,
                            int          PngColorType,
                            IDataBlob*   pDstPngBits)
{
    return Diligent_EncodePng(pSrcPixels, Width, Height, StrideInBytes, PngColorType, pDstPngBits);
}


DECODE_JPEG_RESULT DecodeJpeg(IDataBlob* pSrcJpegBits,
                              IDataBlob* pDstPixels,
                              ImageDesc* pDstImgDesc)
{
    return Diligent_DecodeJpeg(pSrcJpegBits, pDstPixels, pDstImgDesc);
}

ENCODE_JPEG_RESULT EncodeJpeg(Uint8*     pSrcRGBPixels,
                              Uint32     Width,
                              Uint32     Height,
                              int        quality,
                              IDataBlob* pDstJpegBits)
{
    return Diligent_EncodeJpeg(pSrcRGBPixels, Width, Height, quality, pDstJpegBits);
}

} // namespace Diligent

extern "C"
{
    void Diligent_CreateTextureFromImage(Diligent::Image*                 pSrcImage,
                                         const Diligent::TextureLoadInfo& TexLoadInfo,
                                         Diligent::IRenderDevice*         pDevice,
                                         Diligent::ITexture**             ppTexture)
    {
        Diligent::CreateTextureFromImage(pSrcImage, TexLoadInfo, pDevice, ppTexture);
    }

    void Diligent_CreateTextureFromDDS(Diligent::IDataBlob*             pDDSData,
                                       const Diligent::TextureLoadInfo& TexLoadInfo,
                                       Diligent::IRenderDevice*         pDevice,
                                       Diligent::ITexture**             ppTexture)

    {
        Diligent::CreateTextureFromDDS(pDDSData, TexLoadInfo, pDevice, ppTexture);
    }
}