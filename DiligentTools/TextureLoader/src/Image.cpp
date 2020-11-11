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
#include <array>

#include "Image.h"
#include "Errors.hpp"

#include "tiffio.h"
#include "png.h"
#include "PNGCodec.h"
#include "JPEGCodec.h"

#include "DataBlobImpl.hpp"
#include "DebugUtilities.hpp"
#include "RefCntAutoPtr.hpp"
#include "Align.hpp"
#include "GraphicsAccessories.hpp"
#include "BasicFileStream.hpp"
#include "StringTools.hpp"

namespace Diligent
{

class TIFFClientOpenWrapper
{
public:
    explicit TIFFClientOpenWrapper(IDataBlob* pData) noexcept :
        m_Offset{0},
        m_Size{pData->GetSize()},
        m_pData{pData}
    {
    }

    static tmsize_t TIFFReadProc(thandle_t pClientData, void* pBuffer, tmsize_t Size)
    {
        auto* pThis   = reinterpret_cast<TIFFClientOpenWrapper*>(pClientData);
        auto* pSrcPtr = reinterpret_cast<Uint8*>(pThis->m_pData->GetDataPtr()) + pThis->m_Offset;
        memcpy(pBuffer, pSrcPtr, Size);
        pThis->m_Offset += Size;
        return Size;
    }

    static tmsize_t TIFFWriteProc(thandle_t pClientData, void* pBuffer, tmsize_t Size)
    {
        auto* pThis = reinterpret_cast<TIFFClientOpenWrapper*>(pClientData);
        if (pThis->m_Offset + Size > pThis->m_Size)
        {
            pThis->m_Size = pThis->m_Offset + Size;
            pThis->m_pData->Resize(pThis->m_Size);
        }
        auto* pDstPtr = reinterpret_cast<Uint8*>(pThis->m_pData->GetDataPtr()) + pThis->m_Offset;
        memcpy(pDstPtr, pBuffer, Size);
        pThis->m_Offset += Size;
        return Size;
    }

    static toff_t TIFFSeekProc(thandle_t pClientData, toff_t Offset, int Whence)
    {
        auto* pThis = reinterpret_cast<TIFFClientOpenWrapper*>(pClientData);
        switch (Whence)
        {
            case SEEK_SET: pThis->m_Offset = static_cast<size_t>(Offset); break;
            case SEEK_CUR: pThis->m_Offset += static_cast<size_t>(Offset); break;
            case SEEK_END: pThis->m_Offset = pThis->m_Size + static_cast<size_t>(Offset); break;
            default: UNEXPECTED("Unexpected whence");
        }

        return pThis->m_Offset;
    }

    static int TIFFCloseProc(thandle_t pClientData)
    {
        auto* pThis = reinterpret_cast<TIFFClientOpenWrapper*>(pClientData);
        pThis->m_pData.Release();
        pThis->m_Size   = 0;
        pThis->m_Offset = 0;
        return 0;
    }

    static toff_t TIFFSizeProc(thandle_t pClientData)
    {
        auto* pThis = reinterpret_cast<TIFFClientOpenWrapper*>(pClientData);
        return pThis->m_Size;
    }

    static int TIFFMapFileProc(thandle_t pClientData, void** base, toff_t* size)
    {
        UNEXPECTED("Client file mapping is not implemented. Use \'m\' when opening TIFF file to disable file mapping.");
        return 0;
    }

    static void TIFFUnmapFileProc(thandle_t pClientData, void* base, toff_t size)
    {
        UNEXPECTED("Client file mapping is not implemented. Use \'m\' when opening TIFF file to disable file mapping.");
    }

private:
    size_t                   m_Offset;
    size_t                   m_Size;
    RefCntAutoPtr<IDataBlob> m_pData;
};

void Image::LoadTiffFile(IDataBlob* pFileData, const ImageLoadInfo& LoadInfo)
{
    TIFFClientOpenWrapper TiffClientOpenWrpr(pFileData);

    auto TiffFile = TIFFClientOpen("", "rm", &TiffClientOpenWrpr,
                                   TIFFClientOpenWrapper::TIFFReadProc,
                                   TIFFClientOpenWrapper::TIFFWriteProc,
                                   TIFFClientOpenWrapper::TIFFSeekProc,
                                   TIFFClientOpenWrapper::TIFFCloseProc,
                                   TIFFClientOpenWrapper::TIFFSizeProc,
                                   TIFFClientOpenWrapper::TIFFMapFileProc,
                                   TIFFClientOpenWrapper::TIFFUnmapFileProc);

    TIFFGetField(TiffFile, TIFFTAG_IMAGEWIDTH, &m_Desc.Width);
    TIFFGetField(TiffFile, TIFFTAG_IMAGELENGTH, &m_Desc.Height);

    Uint16 SamplesPerPixel = 0;
    // SamplesPerPixel is usually 1 for bilevel, grayscale, and palette-color images.
    // SamplesPerPixel is usually 3 for RGB images. If this value is higher, ExtraSamples
    // should give an indication of the meaning of the additional channels.
    TIFFGetField(TiffFile, TIFFTAG_SAMPLESPERPIXEL, &SamplesPerPixel);
    m_Desc.NumComponents = SamplesPerPixel;

    Uint16 BitsPerSample = 0;
    TIFFGetField(TiffFile, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);

    Uint16 SampleFormat = 0;
    TIFFGetField(TiffFile, TIFFTAG_SAMPLEFORMAT, &SampleFormat);
    if (SampleFormat == 0)
        SampleFormat = SAMPLEFORMAT_UINT;

    switch (SampleFormat)
    {
        case SAMPLEFORMAT_UINT:
            switch (BitsPerSample)
            {
                case 8: m_Desc.ComponentType = VT_UINT8; break;
                case 16: m_Desc.ComponentType = VT_UINT16; break;
                case 32: m_Desc.ComponentType = VT_UINT32; break;
                default: LOG_ERROR_AND_THROW(BitsPerSample, " is not a valid UINT component bit depth. Only 8, 16 and 32 are allowed");
            }
            break;

        case SAMPLEFORMAT_INT:
            switch (BitsPerSample)
            {
                case 8: m_Desc.ComponentType = VT_INT8; break;
                case 16: m_Desc.ComponentType = VT_INT16; break;
                case 32: m_Desc.ComponentType = VT_INT32; break;
                default: LOG_ERROR_AND_THROW(BitsPerSample, " is not a valid INT component bit depth. Only 8, 16 and 32 are allowed");
            }
            break;

        case SAMPLEFORMAT_IEEEFP:
            switch (BitsPerSample)
            {
                case 16: m_Desc.ComponentType = VT_FLOAT16; break;
                case 32: m_Desc.ComponentType = VT_FLOAT32; break;
                default: LOG_ERROR_AND_THROW(BitsPerSample, " is not a valid FLOAT component bit depth. Only 16 and 32 are allowed");
            }
            break;

        case SAMPLEFORMAT_VOID:
            LOG_ERROR_AND_THROW("Untyped tif images are not supported");
            break;

        case SAMPLEFORMAT_COMPLEXINT:
            LOG_ERROR_AND_THROW("Complex int tif images are not supported");
            break;

        case SAMPLEFORMAT_COMPLEXIEEEFP:
            LOG_ERROR_AND_THROW("Complex floating point tif images are not supported");
            break;

        default:
            LOG_ERROR_AND_THROW("Unknown sample format: ", Uint32{SampleFormat});
    }

    auto ScanlineSize = TIFFScanlineSize(TiffFile);
    m_Desc.RowStride  = Align(static_cast<Uint32>(ScanlineSize), 4u);
    m_pData->Resize(size_t{m_Desc.Height} * size_t{m_Desc.RowStride});
    auto* pDataPtr = reinterpret_cast<Uint8*>(m_pData->GetDataPtr());
    for (Uint32 row = 0; row < m_Desc.Height; row++, pDataPtr += m_Desc.RowStride)
    {
        TIFFReadScanline(TiffFile, pDataPtr, row);
    }
    TIFFClose(TiffFile);
}


Image::Image(IReferenceCounters*  pRefCounters,
             IDataBlob*           pFileData,
             const ImageLoadInfo& LoadInfo) :
    TBase{pRefCounters},
    m_pData{MakeNewRCObj<DataBlobImpl>()(0)}
{
    if (LoadInfo.Format == IMAGE_FILE_FORMAT_TIFF)
    {
        LoadTiffFile(pFileData, LoadInfo);
    }
    else if (LoadInfo.Format == IMAGE_FILE_FORMAT_PNG)
    {
        auto Res = DecodePng(pFileData, m_pData.RawPtr(), &m_Desc);
        if (Res != DECODE_PNG_RESULT_OK)
            LOG_ERROR_MESSAGE("Failed to decode png image");
    }
    else if (LoadInfo.Format == IMAGE_FILE_FORMAT_JPEG)
    {
        auto Res = DecodeJpeg(pFileData, m_pData.RawPtr(), &m_Desc);
        if (Res != DECODE_JPEG_RESULT_OK)
            LOG_ERROR_MESSAGE("Failed to decode jpeg image");
    }
    else if (LoadInfo.Format == IMAGE_FILE_FORMAT_DDS)
    {
        LOG_ERROR_MESSAGE("An image can't be created from DDS file. Use CreateTextureFromFile() or CreateTextureFromDDS() functions.");
    }
    else if (LoadInfo.Format == IMAGE_FILE_FORMAT_KTX)
    {
        LOG_ERROR_MESSAGE("An image can't be created from KTX file. Use CreateTextureFromFile() or CreateTextureFromKTX() functions.");
    }
    else
    {
        LOG_ERROR_MESSAGE("Unknown image format.");
    }
}

void Image::CreateFromDataBlob(IDataBlob*           pFileData,
                               const ImageLoadInfo& LoadInfo,
                               Image**              ppImage)
{
    *ppImage = MakeNewRCObj<Image>()(pFileData, LoadInfo);
    (*ppImage)->AddRef();
}


static const std::array<Uint8, 4> GetRGBAOffsets(TEXTURE_FORMAT Format)
{
    switch (Format)
    {
        case TEX_FORMAT_BGRA8_TYPELESS:
        case TEX_FORMAT_BGRA8_UNORM:
        case TEX_FORMAT_BGRA8_UNORM_SRGB:
            return {{2, 1, 0, 3}};
        default:
            return {{0, 1, 2, 3}};
    }
}

std::vector<Uint8> Image::ConvertImageData(Uint32         Width,
                                           Uint32         Height,
                                           const Uint8*   pData,
                                           Uint32         Stride,
                                           TEXTURE_FORMAT SrcFormat,
                                           TEXTURE_FORMAT DstFormat,
                                           bool           KeepAlpha)
{
    const auto& SrcFmtAttribs = GetTextureFormatAttribs(SrcFormat);
    const auto& DstFmtAttribs = GetTextureFormatAttribs(DstFormat);
    VERIFY(SrcFmtAttribs.ComponentSize == 1, "Only 8-bit formats are currently supported");
    VERIFY(DstFmtAttribs.ComponentSize == 1, "Only 8-bit formats are currently supported");

    auto NumDstComponents = SrcFmtAttribs.NumComponents;
    if (!KeepAlpha)
        NumDstComponents = std::min(NumDstComponents, Uint8{3});

    auto SrcOffsets = GetRGBAOffsets(SrcFormat);
    auto DstOffsets = GetRGBAOffsets(DstFormat);

    std::vector<Uint8> ConvertedData(DstFmtAttribs.ComponentSize * NumDstComponents * Width * Height);

    for (Uint32 j = 0; j < Height; ++j)
    {
        for (Uint32 i = 0; i < Width; ++i)
        {
            for (Uint32 c = 0; c < NumDstComponents; ++c)
            {
                ConvertedData[j * Width * NumDstComponents + i * NumDstComponents + DstOffsets[c]] =
                    pData[j * Stride + i * SrcFmtAttribs.NumComponents + SrcOffsets[c]];
            }
        }
    }

    return ConvertedData;
}


void Image::Encode(const EncodeInfo& Info, IDataBlob** ppEncodedData)
{
    RefCntAutoPtr<IDataBlob> pEncodedData(MakeNewRCObj<DataBlobImpl>()(0));
    if (Info.FileFormat == IMAGE_FILE_FORMAT_JPEG)
    {
        auto RGBData = ConvertImageData(Info.Width, Info.Height, reinterpret_cast<const Uint8*>(Info.pData), Info.Stride, Info.TexFormat, TEX_FORMAT_RGBA8_UNORM, false);

        auto Res = EncodeJpeg(RGBData.data(), Info.Width, Info.Height, Info.JpegQuality, pEncodedData.RawPtr());
        if (Res != ENCODE_JPEG_RESULT_OK)
            LOG_ERROR_MESSAGE("Failed to encode jpeg file");
    }
    else if (Info.FileFormat == IMAGE_FILE_FORMAT_PNG)
    {
        const auto*        pData  = reinterpret_cast<const Uint8*>(Info.pData);
        auto               Stride = Info.Stride;
        std::vector<Uint8> ConvertedData;
        if (!((Info.TexFormat == TEX_FORMAT_RGBA8_UNORM || Info.TexFormat == TEX_FORMAT_RGBA8_UNORM_SRGB) && Info.KeepAlpha))
        {
            ConvertedData = ConvertImageData(Info.Width, Info.Height, reinterpret_cast<const Uint8*>(Info.pData), Info.Stride, Info.TexFormat, TEX_FORMAT_RGBA8_UNORM, Info.KeepAlpha);
            pData         = ConvertedData.data();
            Stride        = Info.Width * (Info.KeepAlpha ? 4 : 3);
        }

        auto Res = EncodePng(pData, Info.Width, Info.Height, Stride, Info.KeepAlpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB, pEncodedData.RawPtr());
        if (Res != ENCODE_PNG_RESULT_OK)
            LOG_ERROR_MESSAGE("Failed to encode png file");
    }
    else
    {
        UNSUPPORTED("Unsupported image file format");
    }
    pEncodedData->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ppEncodedData));
}

IMAGE_FILE_FORMAT Image::GetFileFormat(const Uint8* pData, size_t Size)
{
    if (Size >= 3 && pData[0] == 0xFF && pData[1] == 0xD8 && pData[2] == 0xFF)
        return IMAGE_FILE_FORMAT_JPEG;

    if (Size >= 8 &&
        pData[0] == 0x89 && pData[1] == 0x50 && pData[2] == 0x4E && pData[3] == 0x47 &&
        pData[4] == 0x0D && pData[5] == 0x0A && pData[6] == 0x1A && pData[7] == 0x0A)
        return IMAGE_FILE_FORMAT_PNG;

    if (Size >= 4 &&
        ((pData[0] == 0x49 && pData[1] == 0x20 && pData[2] == 0x49) ||
         (pData[0] == 0x49 && pData[1] == 0x49 && pData[2] == 0x2A && pData[3] == 0x00) ||
         (pData[0] == 0x4D && pData[1] == 0x4D && pData[2] == 0x00 && pData[3] == 0x2A) ||
         (pData[0] == 0x4D && pData[1] == 0x4D && pData[2] == 0x00 && pData[3] == 0x2B)))
        return IMAGE_FILE_FORMAT_TIFF;

    if (Size >= 4 && pData[0] == 0x44 && pData[1] == 0x44 && pData[2] == 0x53 && pData[3] == 0x20)
        return IMAGE_FILE_FORMAT_DDS;

    static constexpr Uint8 KTX10FileIdentifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    static constexpr Uint8 KTX20FileIdentifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    if (Size >= 12 &&
        (memcmp(pData, KTX10FileIdentifier, sizeof(KTX10FileIdentifier)) == 0 ||
         memcmp(pData, KTX20FileIdentifier, sizeof(KTX20FileIdentifier)) == 0))
        return IMAGE_FILE_FORMAT_KTX;

    return IMAGE_FILE_FORMAT_UNKNOWN;
}


IMAGE_FILE_FORMAT CreateImageFromFile(const Char* FilePath,
                                      Image**     ppImage,
                                      IDataBlob** ppRawData)
{
    auto ImgFileFormat = IMAGE_FILE_FORMAT_UNKNOWN;
    try
    {
        RefCntAutoPtr<BasicFileStream> pFileStream(MakeNewRCObj<BasicFileStream>()(FilePath, EFileAccessMode::Read));
        if (!pFileStream->IsValid())
            LOG_ERROR_AND_THROW("Failed to open image file \"", FilePath, '\"');

        RefCntAutoPtr<IDataBlob> pFileData(MakeNewRCObj<DataBlobImpl>()(0));
        pFileStream->ReadBlob(pFileData);

        ImgFileFormat = Image::GetFileFormat(reinterpret_cast<Uint8*>(pFileData->GetDataPtr()), pFileData->GetSize());
        if (ImgFileFormat == IMAGE_FILE_FORMAT_UNKNOWN)
        {
            LOG_WARNING_MESSAGE("Unable to derive image format from the header for file \"", FilePath, "\". Trying to analyze extension.");

            // Try to use extension to derive format
            auto* pDotPos = strrchr(FilePath, '.');
            if (pDotPos == nullptr)
                LOG_ERROR_AND_THROW("Unable to recognize file format: file name \"", FilePath, "\" does not contain extension");

            auto* pExtension = pDotPos + 1;
            if (*pExtension == 0)
                LOG_ERROR_AND_THROW("Unable to recognize file format: file name \"", FilePath, "\" contain empty extension");

            String Extension = StrToLower(pExtension);
            if (Extension == "png")
                ImgFileFormat = IMAGE_FILE_FORMAT_PNG;
            else if (Extension == "jpeg" || Extension == "jpg")
                ImgFileFormat = IMAGE_FILE_FORMAT_JPEG;
            else if (Extension == "tiff" || Extension == "tif")
                ImgFileFormat = IMAGE_FILE_FORMAT_TIFF;
            else if (Extension == "dds")
                ImgFileFormat = IMAGE_FILE_FORMAT_DDS;
            else if (Extension == "ktx")
                ImgFileFormat = IMAGE_FILE_FORMAT_KTX;
            else
                LOG_ERROR_AND_THROW("Unsupported file format ", Extension);
        }

        if (ImgFileFormat == IMAGE_FILE_FORMAT_PNG ||
            ImgFileFormat == IMAGE_FILE_FORMAT_JPEG ||
            ImgFileFormat == IMAGE_FILE_FORMAT_TIFF)
        {
            ImageLoadInfo ImgLoadInfo;
            ImgLoadInfo.Format = ImgFileFormat;
            Image::CreateFromDataBlob(pFileData, ImgLoadInfo, ppImage);
        }
        else if (ppRawData != nullptr)
        {
            *ppRawData = pFileData.Detach();
        }
    }
    catch (std::runtime_error& err)
    {
        LOG_ERROR("Failed to create image from file: ", err.what());
    }

    return ImgFileFormat;
}

} // namespace Diligent
