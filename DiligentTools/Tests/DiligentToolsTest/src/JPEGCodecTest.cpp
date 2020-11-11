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

#include "../include/JPEGCodec.h"

#include "gtest/gtest.h"

#include "DataBlobImpl.hpp"

#include <cmath>

using namespace Diligent;

namespace
{

TEST(Tools_TextureLoader, JPEGCodec)
{
    constexpr Uint32 TestImgWidth  = 256;
    constexpr Uint32 TestImgHeight = 128;
    constexpr Uint32 NumComponents = 3;

    std::vector<Uint8> RefPixels(TestImgWidth * TestImgHeight * NumComponents);
    for (Uint32 y = 0; y < TestImgHeight; ++y)
    {
        for (Uint32 x = 0; x < TestImgWidth; ++x)
        {
            auto idx = x + y * TestImgWidth;

            RefPixels[idx * NumComponents + 0] = (16 + idx) & 0xFF;
            RefPixels[idx * NumComponents + 1] = (32 + idx) & 0xFF;
            RefPixels[idx * NumComponents + 2] = (64 + idx) & 0xFF;
        }
    }

    RefCntAutoPtr<IDataBlob> pJpgData{MakeNewRCObj<DataBlobImpl>()(0)};

    auto Res = EncodeJpeg(RefPixels.data(), TestImgWidth, TestImgHeight, 100, pJpgData);
    ASSERT_EQ(Res, ENCODE_JPEG_RESULT_OK);

    RefCntAutoPtr<IDataBlob> pDecodedPixelsBlob{MakeNewRCObj<DataBlobImpl>()(0)};

    ImageDesc DecodedImgDesc;
    DecodeJpeg(pJpgData, pDecodedPixelsBlob, &DecodedImgDesc);

    ASSERT_EQ(DecodedImgDesc.Width, TestImgWidth);
    ASSERT_EQ(DecodedImgDesc.Height, TestImgHeight);
    ASSERT_EQ(DecodedImgDesc.NumComponents, NumComponents);
    ASSERT_EQ(DecodedImgDesc.ComponentType, VT_UINT8);

    const Uint8* pTestPixels = reinterpret_cast<const Uint8*>(pDecodedPixelsBlob->GetDataPtr());
    for (Uint32 y = 0; y < TestImgHeight; ++y)
    {
        for (Uint32 x = 0; x < TestImgWidth; ++x)
        {
            for (Uint32 c = 0; c < NumComponents; ++c)
            {
                auto RefVal  = RefPixels[(x + y * TestImgWidth) * NumComponents + c];
                auto TestVal = pTestPixels[x * DecodedImgDesc.NumComponents + c + y * DecodedImgDesc.RowStride];
                auto Diff    = std::abs(static_cast<int>(RefVal) - static_cast<int>(TestVal));
                EXPECT_LE(Diff, 1) << "[" << x << "," << y << "][" << c << "]: " << static_cast<int>(RefVal) << " vs " << static_cast<int>(TestVal);
            }
        }
    }
}

} // namespace
