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

#include "../interface/Image.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// clang-format off

/// JPEG image decoding result.
DILIGENT_TYPED_ENUM(DECODE_JPEG_RESULT, Uint32)
{
    /// JPEG image was decoded successfully.
    DECODE_JPEG_RESULT_OK = 0,

    /// Invalid arguments (e.g. null pointer).
    DECODE_JPEG_RESULT_INVALID_ARGUMENTS,

    /// Failed to initialize the decoder.
    DECODE_JPEG_RESULT_INITIALIZATION_FAILED,

    /// An unexpected error occurred while decoding the file.
    DECODE_JPEG_RESULT_DECODING_ERROR
};

/// JPEG image encoding result.
DILIGENT_TYPED_ENUM(ENCODE_JPEG_RESULT, Uint32)
{
    /// Encoding finished successfully.
    ENCODE_JPEG_RESULT_OK = 0,

    /// Invalid arguments (e.g. null pointer).
    ENCODE_JPEG_RESULT_INVALID_ARGUMENTS,

    /// Failed to initialize the encoder.
    ENCODE_JPEG_RESULT_INITIALIZATION_FAILED
};
// clang-format on


/// Decodes jpeg image.

/// \param [in]  pSrcJpegBits - JPEG image encoded bits.
/// \param [out] pDstPixels   - Decoded pixels data blob. The pixels are always tightly packed
///                             (for instance, components of 3-channel image will be written as |r|g|b|r|g|b|r|g|b|...).
/// \param [out] pDstImgDesc  - Decoded image description.
/// \return                     Decoding result, see Diligent::DECODE_JPEG_RESULT.
DECODE_JPEG_RESULT DILIGENT_GLOBAL_FUNCTION(DecodeJpeg)(IDataBlob* pSrcJpegBits,
                                                        IDataBlob* pDstPixels,
                                                        ImageDesc* pDstImgDesc);


/// Encodes an image jpeg PNG format.

/// \param [in] pSrcPixels    - Source pixels. The pixels must be tightly packed
///                             (for instance, components of 3-channel image must be stored as |r|g|b|r|g|b|r|g|b|...).
///                             At the moment, only RGB images are supported.
/// \param [in] Width         - Image width.
/// \param [in] Height        - Image height.
/// \param [in] quality       - JPEG encoding qulaity, in the range from 0 to 100.
/// \param [out] pDstJpegBits - Encoded JPEG image bits.
/// \return                     Encoding result, see Diligent::ENCODE_JPEG_RESULT.
ENCODE_JPEG_RESULT DILIGENT_GLOBAL_FUNCTION(EncodeJpeg)(Uint8*     pSrcRGBPixels,
                                                        Uint32     Width,
                                                        Uint32     Height,
                                                        int        quality,
                                                        IDataBlob* pDstJpegBits);

DILIGENT_END_NAMESPACE // namespace Diligent
