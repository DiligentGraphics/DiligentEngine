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

#include "../include/PNGCodec.h"
#include "png.h"

#include "gtest/gtest.h"

#include <vector>

#include "DataBlobImpl.hpp"

using namespace Diligent;

namespace
{

TEST(Tools_TextureLoader, PNGCodec)
{
    constexpr Uint32 TestImgWidth  = 256;
    constexpr Uint32 TestImgHeight = 128;

    for (int EncodeAlpha = 0; EncodeAlpha <= 1; ++EncodeAlpha)
    {
        const Uint32 NumComponents = EncodeAlpha ? 4u : 3u;

        std::vector<Uint8> RefPixels(TestImgWidth * TestImgHeight * NumComponents);
        for (Uint32 y = 0; y < TestImgHeight; ++y)
        {
            for (Uint32 x = 0; x < TestImgWidth; ++x)
            {
                auto idx = x + y * TestImgWidth;

                RefPixels[idx * NumComponents + 0] = (16 + idx) & 0xFF;
                RefPixels[idx * NumComponents + 1] = (32 + idx) & 0xFF;
                RefPixels[idx * NumComponents + 2] = (64 + idx) & 0xFF;
                if (NumComponents == 4)
                    RefPixels[idx * NumComponents + 3] = (96 + idx) & 0xFF;
            }
        }

        RefCntAutoPtr<IDataBlob> pPngData{MakeNewRCObj<DataBlobImpl>()(0)};

        auto Res = EncodePng(RefPixels.data(), TestImgWidth, TestImgHeight, TestImgWidth * NumComponents,
                             EncodeAlpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
                             pPngData);
        ASSERT_EQ(Res, ENCODE_PNG_RESULT_OK);

        RefCntAutoPtr<IDataBlob> pDecodedPixelsBlob{MakeNewRCObj<DataBlobImpl>()(0)};

        ImageDesc DecodedImgDesc;
        DecodePng(pPngData, pDecodedPixelsBlob, &DecodedImgDesc);

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
                    EXPECT_EQ(static_cast<Uint32>(RefVal), static_cast<Uint32>(TestVal)) << "[" << x << "," << y << "][" << c << "]";
                }
            }
        }
    }
}

} // namespace
