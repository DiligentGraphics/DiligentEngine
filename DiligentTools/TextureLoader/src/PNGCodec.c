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

#include <stdlib.h>
#include <string.h>

#include "png.h"
#include "GraphicsTypes.h"

#include "PNGCodec.h"

struct PNGReadFnState
{
    IDataBlob* pPngBits;
    size_t     Offset;
};
typedef struct PNGReadFnState PNGReadFnState;

static void PngReadCallback(png_structp pngPtr, png_bytep data, png_size_t length)
{
    PNGReadFnState* pState  = (PNGReadFnState*)(png_get_io_ptr(pngPtr));
    Uint8*          pDstPtr = (Uint8*)IDataBlob_GetDataPtr(pState->pPngBits) + pState->Offset;
    memcpy(data, pDstPtr, length);
    pState->Offset += length;
}

DECODE_PNG_RESULT Diligent_DecodePng(IDataBlob* pSrcPngBits,
                                     IDataBlob* pDstPixels,
                                     ImageDesc* pDstImgDesc)
{
    if (!pSrcPngBits || !pDstPixels || !pDstImgDesc)
        return DECODE_PNG_RESULT_INVALID_ARGUMENTS;

    // http://www.piko3d.net/tutorials/libpng-tutorial-loading-png-files-from-streams/
    // http://www.libpng.org/pub/png/book/chapter13.html#png.ch13.div.10
    // https://gist.github.com/niw/5963798

    const size_t    PngSigSize = 8;
    png_const_bytep pngsig     = (png_const_bytep)IDataBlob_GetDataPtr(pSrcPngBits);
    //Let LibPNG check the signature. If this function returns 0, everything is OK.
    if (png_sig_cmp(pngsig, 0, PngSigSize) != 0)
    {
        return DECODE_PNG_RESULT_INVALID_SIGNATURE;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        return DECODE_PNG_RESULT_INITIALIZATION_FAILED;
    }

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        png_destroy_read_struct(&png, &info, (png_infopp)0);
        return DECODE_PNG_RESULT_INITIALIZATION_FAILED;
    }

    png_bytep* rowPtrs = NULL;
    if (setjmp(png_jmpbuf(png)))
    {
        if (rowPtrs)
            free(rowPtrs);
        // When an error occurs during parsing, libPNG will jump to here
        png_destroy_read_struct(&png, &info, (png_infopp)0);
        return DECODE_PNG_RESULT_DECODING_ERROR;
    }

    PNGReadFnState ReadState;
    ReadState.pPngBits = pSrcPngBits;
    ReadState.Offset   = 0;

    png_set_read_fn(png, (png_voidp)&ReadState, PngReadCallback);

    png_read_info(png, info);

    png_byte bit_depth = png_get_bit_depth(png, info);

    // PNG files store 16-bit pixels in network byte order (big-endian, ie
    // most significant bytes first). png_set_swap() shall switch the byte-order
    // to little-endian (ie, least significant bits first).
    if (bit_depth == 16)
        png_set_swap(png);

    png_byte color_type = png_get_color_type(png, info);

    // See http://www.libpng.org/pub/png/libpng-manual.txt
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        // Transform paletted images into 8-bit rgba
        png_set_palette_to_rgb(png);
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    {
        // Expand 1, 2, or 4-bit images to 8-bit
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

#if 0
    // These color_type don't have an alpha channel then fill it with 0xff.
    if( color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE )
        png_set_filler( png, 0xFF, PNG_FILLER_AFTER );

    if( color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
        png_set_gray_to_rgb( png );
#endif

    png_read_update_info(png, info);

    bit_depth                  = png_get_bit_depth(png, info);
    pDstImgDesc->Width         = png_get_image_width(png, info);
    pDstImgDesc->Height        = png_get_image_height(png, info);
    pDstImgDesc->NumComponents = png_get_channels(png, info);
    switch (bit_depth)
    {
        case 8: pDstImgDesc->ComponentType = VT_UINT8; break;
        case 16: pDstImgDesc->ComponentType = VT_UINT16; break;
        case 32: pDstImgDesc->ComponentType = VT_UINT32; break;
        default:
        {
            png_destroy_read_struct(&png, &info, (png_infopp)0);
            return DECODE_PNG_RESULT_INVALID_BIT_DEPTH;
        }
    }

    //Array of row pointers. One for every row.
    rowPtrs = malloc(sizeof(png_bytep) * pDstImgDesc->Height);

    //Alocate a buffer with enough space.
    pDstImgDesc->RowStride = pDstImgDesc->Width * (Uint32)bit_depth * pDstImgDesc->NumComponents / 8u;
    // Align stride to 4 bytes
    pDstImgDesc->RowStride = (pDstImgDesc->RowStride + 3u) & ~3u;

    IDataBlob_Resize(pDstPixels, pDstImgDesc->Height * pDstImgDesc->RowStride);
    png_bytep pRow0 = IDataBlob_GetDataPtr(pDstPixels);
    for (size_t i = 0; i < pDstImgDesc->Height; i++)
        rowPtrs[i] = pRow0 + i * pDstImgDesc->RowStride;

    //Read the imagedata and write it to the adresses pointed to
    //by rowptrs (in other words: our image databuffer)
    png_read_image(png, rowPtrs);

    free(rowPtrs);
    png_destroy_read_struct(&png, &info, (png_infopp)0);

    return DECODE_PNG_RESULT_OK;
}

static void PngWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
    IDataBlob* pEncodedData = (IDataBlob*)png_get_io_ptr(png_ptr);
    size_t     PrevSize     = IDataBlob_GetSize(pEncodedData);
    IDataBlob_Resize(pEncodedData, PrevSize + length);
    Uint8* pBytes = (Uint8*)IDataBlob_GetDataPtr(pEncodedData);
    memcpy(pBytes + PrevSize, data, length);
}

ENCODE_PNG_RESULT Diligent_EncodePng(const Uint8* pSrcPixels,
                                     Uint32       Width,
                                     Uint32       Height,
                                     Uint32       StrideInBytes,
                                     int          PngColorType,
                                     IDataBlob*   pDstPngBits)
{
    if (!pSrcPixels || !pDstPngBits || Width == 0 || Height == 0 || StrideInBytes == 0)
        return ENCODE_PNG_RESULT_INVALID_ARGUMENTS;

    png_struct* strct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!strct)
    {
        return ENCODE_PNG_RESULT_INITIALIZATION_FAILED;
    }

    png_info* info = png_create_info_struct(strct);
    if (!info)
    {
        png_destroy_write_struct(&strct, &info);
        return ENCODE_PNG_RESULT_INITIALIZATION_FAILED;
    }

    png_bytep* rowPtrs = NULL;
    if (setjmp(png_jmpbuf(strct)) != 0)
    {
        if (rowPtrs)
            free(rowPtrs);
        png_destroy_write_struct(&strct, &info);
        return ENCODE_PNG_RESULT_INITIALIZATION_FAILED;
    }

    png_set_IHDR(strct, info, Width, Height, 8,
                 PngColorType,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    //png_set_compression_level(p, 1);
    rowPtrs = malloc(sizeof(png_bytep) * Height);
    for (size_t y = 0; y < Height; ++y)
        rowPtrs[y] = (Uint8*)pSrcPixels + y * StrideInBytes;

    png_set_rows(strct, info, rowPtrs);

    png_set_write_fn(strct, pDstPngBits, PngWriteCallback, NULL);
    png_write_png(strct, info, PNG_TRANSFORM_IDENTITY, NULL);

    free(rowPtrs);

    png_destroy_write_struct(&strct, &info);

    return ENCODE_PNG_RESULT_OK;
}
