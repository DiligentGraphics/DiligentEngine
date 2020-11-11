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

#include <cstring>
#include <cstdlib>
#include <algorithm>

#include "TestingSwapChainBase.hpp"
#include "GraphicsAccessories.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../../ThirdParty/stb/stb_image_write.h"

namespace Diligent
{

namespace Testing
{

void CompareTestImages(const Uint8*   pReferencePixels,
                       Uint32         RefPixelsStride,
                       const Uint8*   pPixels,
                       Uint32         PixelsStride,
                       Uint32         Width,
                       Uint32         Height,
                       TEXTURE_FORMAT Format)
{
    VERIFY_EXPR(pReferencePixels != nullptr);
    VERIFY_EXPR(pPixels != nullptr);
    VERIFY_EXPR(Width != 0);
    VERIFY_EXPR(Height != 0);
    VERIFY_EXPR(PixelsStride != 0);
    VERIFY_EXPR(RefPixelsStride != 0);
    VERIFY(Format == TEX_FORMAT_RGBA8_UNORM, GetTextureFormatAttribs(Format).Name, " is not supported");

    bool bIsIdentical = true;

    for (Uint32 row = 0; row < Height; ++row)
    {
        if (memcmp(pReferencePixels + row * RefPixelsStride,
                   pPixels + row * PixelsStride,
                   Width * 4) != 0)
        {
            bIsIdentical = false;
        }
    }

    if (bIsIdentical)
    {
    }
    else
    {
        auto ReportImageStride = (Width * 2) * 3;

        std::vector<Uint8> ReportImage(ReportImageStride * (Height * 2));
        for (Uint32 row = 0; row < Height; ++row)
        {
            for (Uint32 col = 0; col < Width; ++col)
            {
                for (Uint32 c = 0; c < 3; ++c)
                {
                    auto RefVal = pReferencePixels[row * RefPixelsStride + col * 4 + c];
                    auto Val    = pPixels[row * PixelsStride + col * 4 + c];
                    auto diff   = static_cast<Uint8>(std::min(std::abs(int{RefVal} - int{Val}), 255));

                    // clang-format off
                    ReportImage[row            * ReportImageStride +  col * 3          + c] = RefVal;
                    ReportImage[row            * ReportImageStride + (Width + col) * 3 + c] = Val;
                    ReportImage[(row + Height) * ReportImageStride +  col * 3          + c] = diff;
                    ReportImage[(row + Height) * ReportImageStride + (Width + col) * 3 + c] = static_cast<Uint8>(std::min(diff*16, 255));
                    // clang-format on
                }
            }
        }
        const auto* const TestInfo = ::testing::UnitTest::GetInstance()->current_test_info();

        std::string FileName = TestInfo->test_suite_name();
        FileName += '.';
        FileName += TestInfo->name();
        FileName += "_FAIL_.png";
        if (stbi_write_png(FileName.c_str(), Width * 2, Height * 2, 3, ReportImage.data(), (Width * 2) * 3) == 0)
        {
            LOG_ERROR_MESSAGE("Failed to write ", FileName);
        }
        ADD_FAILURE() << "Image rendered by the test is not identical to the reference image";
    }
}

} // namespace Testing

} // namespace Diligent
