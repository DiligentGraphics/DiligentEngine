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

#include "FilteringTools.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace Diligent
{

static std::ostream& operator<<(std::ostream& os, const LinearTexFilterSampleInfo& FilterInfo)
{
    return os << "i0=" << FilterInfo.i0 << " i1=" << FilterInfo.i1 << " w=" << FilterInfo.w;
}

} // namespace Diligent

namespace
{

template <TEXTURE_ADDRESS_MODE AddressMode>
void TestGetLinearTexFilterSampleInfo(float u, Int32 i0, Int32 i1, float w, Uint32 Width = 128)
{
    const LinearTexFilterSampleInfo RefSampleInfo{i0, i1, w};

    {
        auto SampleInfo = GetLinearTexFilterSampleInfo<AddressMode, false>(Width, u);
        EXPECT_EQ(SampleInfo, RefSampleInfo) << "u=" << u << " width=" << Width;
    }

    {
        auto SampleInfo = GetLinearTexFilterSampleInfo<AddressMode, true>(Width, u / static_cast<float>(Width));
        EXPECT_EQ(SampleInfo, RefSampleInfo) << "u_norm=" << u / static_cast<float>(Width) << " width=" << Width;
    }
}

TEST(Common_AdvancedMath, GetLinearTexFilterSampleInfo)
{
    // TEXTURE_ADDRESS_CLAMP
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(0.f, 0, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(-0.5f, 0, 0, 0.0f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(-1.f, 0, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(-128.f, 0, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(-129.f, 0, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(-129.5f, 0, 0, 0.0f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(-256.f, 0, 0, 0.5f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(0.75f, 0, 1, 0.25f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(1.00f, 0, 1, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(1.25f, 0, 1, 0.75f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(1.50f, 1, 2, 0.00f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(126.75f, 126, 127, 0.25f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(127.00f, 126, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(127.25f, 126, 127, 0.75f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(127.50f, 127, 127, 0.00f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(128.00f, 127, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(128.50f, 127, 127, 0.00f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(129.00f, 127, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(255.00f, 127, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(256.00f, 127, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_CLAMP>(257.00f, 127, 127, 0.50f);


    // TEXTURE_ADDRESS_WRAP
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(0.f, 127, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(-0.5f, 127, 0, 0.0f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(-1.f, 126, 127, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(-128.f, 127, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(-129.f, 126, 127, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(-129.5f, 126, 127, 0.0f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(-256.f, 127, 0, 0.5f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(0.75f, 0, 1, 0.25f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(1.00f, 0, 1, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(1.25f, 0, 1, 0.75f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(1.50f, 1, 2, 0.00f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(126.75f, 126, 127, 0.25f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(127.00f, 126, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(127.25f, 126, 127, 0.75f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(127.50f, 127, 0, 0.00f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(128.00f, 127, 0, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(128.50f, 0, 1, 0.00f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(129.00f, 0, 1, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(255.00f, 126, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(256.00f, 127, 0, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_WRAP>(257.00f, 0, 1, 0.50f);


    // TEXTURE_ADDRESS_MIRROR
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(0.f, 0, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(-0.5f, 0, 0, 0.0f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(-1.f, 1, 0, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(-128.f, 127, 127, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(-129.f, 126, 127, 0.5f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(-129.5f, 126, 127, 0.0f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(-256.f, 0, 0, 0.5f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(0.75f, 0, 1, 0.25f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(1.00f, 0, 1, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(1.25f, 0, 1, 0.75f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(1.50f, 1, 2, 0.00f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(126.75f, 126, 127, 0.25f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(127.00f, 126, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(127.25f, 126, 127, 0.75f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(127.50f, 127, 127, 0.00f);

    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(128.00f, 127, 127, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(128.50f, 127, 126, 0.00f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(129.00f, 127, 126, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(129.50f, 126, 125, 0.00f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(255.00f, 1, 0, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(256.00f, 0, 0, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(257.00f, 0, 1, 0.50f);
    TestGetLinearTexFilterSampleInfo<TEXTURE_ADDRESS_MIRROR>(258.00f, 1, 2, 0.50f);
}

template <TEXTURE_ADDRESS_MODE AddressMode>
void TestFilterTexture2DBilinear(float u, float v, float ref)
{
    // clang-format off
    constexpr float Data[] = 
    {
          8,   16,   24,   32,
         40,   48,   56,   64,
         72,   80,   88,   96,
        104,  112,  120,  128
    };
    // clang-format on

    constexpr Uint32 Width  = 4;
    constexpr Uint32 Height = 4;

    auto Val = FilterTexture2DBilinear<float, float, AddressMode, AddressMode, false>(Width, Height, Data, Width, u, v);
    EXPECT_EQ(Val, ref) << "u=" << u << " v=" << v;

    u /= static_cast<float>(Width);
    v /= static_cast<float>(Height);
    Val = FilterTexture2DBilinear<float, float, AddressMode, AddressMode, true>(Width, Height, Data, Width, u, v);
    EXPECT_EQ(Val, ref) << "u_norm=" << u << " v_norm=" << v;
}

TEST(Common_FilteringTools, FilterTexture2DBilinear)
{
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(0.5f, 0.5f, 8.f);
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(0.5f, 0.5f, 8.f);
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(0.5f, 0.5f, 8.f);

    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(3.5f, 3.5f, 128.f);
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(3.5f, 3.5f, 128.f);
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(3.5f, 3.5f, 128.f);

    {
        auto ref = 8.f * 0.75f * 0.75f + 16.f * 0.25f * 0.75f + 40.f * 0.75f * 0.25f + 48.f * 0.25f * 0.25f;
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(0.75f, 0.75f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(0.75f, 0.75f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(0.75f, 0.75f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_UNKNOWN>(0.75f, 0.75f, ref);
    }

    {
        auto ref = 8.f * 0.25f * 0.75f + 16.f * 0.75f * 0.75f + 40.f * 0.25f * 0.25f + 48.f * 0.75f * 0.25f;
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(1.25f, 0.75f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(1.25f, 0.75f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(1.25f, 0.75f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_UNKNOWN>(1.25f, 0.75f, ref);
    }

    {
        auto ref = 8.f * 0.75f * 0.25f + 16.f * 0.25f * 0.25f + 40.f * 0.75f * 0.75f + 48.f * 0.25f * 0.75f;
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(0.75f, 1.25f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(0.75f, 1.25f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(0.75f, 1.25f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_UNKNOWN>(0.75f, 1.25f, ref);
    }

    {
        auto ref = 8.f * 0.25f * 0.25f + 16.f * 0.75f * 0.25f + 40.f * 0.25f * 0.75f + 48.f * 0.75f * 0.75f;
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(1.25f, 1.25f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(1.25f, 1.25f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(1.25f, 1.25f, ref);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_UNKNOWN>(1.25f, 1.25f, ref);
    }

    // TEXTURE_ADDRESS_CLAMP
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(-0.5f, -0.5f, 8.f);
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(-4.0f, -4.0f, 8.f);
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(4.5f, 4.5f, 128.f);
    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_CLAMP>(8.0f, 8.0f, 128.f);


    // TEXTURE_ADDRESS_WRAP
    //
    //      8    16    24    32        8    16    24    32        8    16    24    32
    //     40    48    56    64       40    48    56    64       40    48    56    64
    //     72    80    88    96       72    80    88    96       72    80    88    96
    //    104   112   120   128      104   112   120   128      104   112   120   128
    //
    //      8    16    24    32        8    16    24    32        8    16    24    32
    //     40    48    56    64       40    48    56    64       40    48    56    64
    //     72    80    88    96       72    80    88    96       72    80    88    96
    //    104   112   120   128      104   112   120   128      104   112   120   128
    //
    //      8    16    24    32        8    16    24    32        8    16    24    32
    //     40    48    56    64       40    48    56    64       40    48    56    64
    //     72    80    88    96       72    80    88    96       72    80    88    96
    //    104   112   120   128      104   112   120   128      104   112   120   128

    {
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(0.f, 0.f, (128.f + 104.f + 32.f + 8.f) * 0.25f);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(4.f, 0.f, (128.f + 104.f + 32.f + 8.f) * 0.25f);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(0.f, 4.f, (128.f + 104.f + 32.f + 8.f) * 0.25f);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(4.f, 4.f, (128.f + 104.f + 32.f + 8.f) * 0.25f);

        constexpr auto ref = 88.f * 0.25f * 0.25f + 96.f * 0.75f * 0.25f + 120.f * 0.25f * 0.75f + 128.f * 0.75f * 0.75f;
        for (float dx = -8; dx <= +8; dx += 4)
        {
            for (float dy = -8; dy <= +8; dy += 4)
            {
                TestFilterTexture2DBilinear<TEXTURE_ADDRESS_WRAP>(-0.75f + dx, -0.75f + dy, ref);
            }
        }
    }

    // TEXTURE_ADDRESS_MIRROR
    //
    //    128   120   112   104      104   112   120   128      128   120   112   104
    //     96    88    80    72       72    80    88    96       96    88    80    72
    //     64    56    48    40       40    48    56    64       64    56    48    40
    //     32    24    16     8        8    16    24    32       32    24    16     8
    //
    //     32    24    16     8        8    16    24    32       32    24    16     8
    //     64    56    48    40       40    48    56    64       64    56    48    40
    //     96    88    80    72       72    80    88    96       96    88    80    72
    //    128   120   112   104      104   112   120   128      128   120   112   104
    //
    //    128   120   112   104      104   112   120   128      128   120   112   104
    //     96    88    80    72       72    80    88    96       96    88    80    72
    //     64    56    48    40       40    48    56    64       64    56    48    40
    //     32    24    16     8        8    16    24    32       32    24    16     8

    {
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(0.f, 0.f, 8.f);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(4.f, 0.f, 32.f);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(0.f, 4.f, 104.f);
        TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(4.f, 4.f, 128.f);

        for (float dx = -16; dx <= +16; dx += 8)
        {
            for (float dy = -16; dy <= +16; dy += 8)
            {
                {
                    auto ref = 48.f * 0.25f * 0.25f + 40.f * 0.75f * 0.25f + 16.f * 0.25f * 0.75f + 8.f * 0.75f * 0.75f;
                    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(-0.75f + dx, -0.75f + dy, ref);
                }
                {
                    auto ref = 128.f * 0.75f * 0.75f + 120.f * 0.25f * 0.75f + 96.f * 0.75f * 0.25f + 88.f * 0.25f * 0.25f;
                    TestFilterTexture2DBilinear<TEXTURE_ADDRESS_MIRROR>(4.75f + dx, 4.75f + dy, ref);
                }
            }
        }
    }
}

} // namespace
