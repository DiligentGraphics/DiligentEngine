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

#include "../../Platforms/interface/PlatformDefinitions.h"

#include "BasicMath.hpp"

#include "../../Graphics/GraphicsEngine/interface/Sampler.h"

namespace Diligent
{

/// Linear texture filter sample info
//
//                              w
//                       | - - - - - >|
//  -----X-------|-------X-------|----*--X-------|-------X
//     i0-1.5          i0+0.5          i1+0.5          i1+1.5
//
//      T[*] = lerp(T[i0], T[i1], w)
//
struct LinearTexFilterSampleInfo
{
    union
    {
        struct
        {
            /// First sample index
            Int32 i0;

            /// Second sample index
            Int32 i1;
        };

        /// Sample indices
        Int32 i[2];
    };

    /// Blend weight
    float w = 0.f;

    LinearTexFilterSampleInfo() noexcept :
        i0{0},
        i1{0}
    {}

    LinearTexFilterSampleInfo(Int32 _i0, Int32 _i1, float _w) noexcept :
        i0{_i0},
        i1{_i1},
        w{_w}
    {
    }

    bool operator==(const LinearTexFilterSampleInfo& rhs) const
    {
        return i0 == rhs.i0 && i1 == rhs.i1 && w == rhs.w;
    }

    bool operator!=(const LinearTexFilterSampleInfo& rhs) const
    {
        return !(*this == rhs);
    }
};

/// Returns linear texture filter sample info, see Diligent::LinearTexFilterSampleInfo.
///
/// \tparam AddressMode       - Texture addressing mode, see Diligent::TEXTURE_ADDRESS_MODE.
/// \tparam IsNormalizedCoord - Whether sample coordinate is normalized.
///
/// \param [in] Width    - Texture width.
/// \param [in] u        - Texture sample coordinate.
/// \return                Linear texture filter sample information, see Diligent::LinearTexFilterSampleInfo.
template <TEXTURE_ADDRESS_MODE AddressMode, bool IsNormalizedCoord>
LinearTexFilterSampleInfo GetLinearTexFilterSampleInfo(Uint32 Width, float u)
{
    float x  = IsNormalizedCoord ? u * static_cast<float>(Width) : u;
    float x0 = FastFloor(x - 0.5f);

    // clang-format off
    LinearTexFilterSampleInfo SampleInfo
    {
        static_cast<Int32>(x0),
        static_cast<Int32>(x0 + 1),
        x - 0.5f - x0
    };
    // clang-format on

    auto WrapCoord = [](Int32 i, Uint32 Width) //
    {
        auto w = static_cast<Int32>(Width);

        // Note that the sign of a%b is implementation-dependent when one of the operands is negative.
        // a/b, to the contrary, is always well-defined.
        i = i - (i / w) * w;
        return i < 0 ? i + w : i;
    };

    auto MirrorCoord = [WrapCoord](Int32 i, Uint32 Width) //
    {
        i = WrapCoord(i, Width * 2);

        auto w = static_cast<Int32>(Width);
        return i >= w ? (w * 2 - 1) - i : i;
    };

    switch (AddressMode)
    {
        case TEXTURE_ADDRESS_UNKNOWN:
            // do nothing
            break;

        case TEXTURE_ADDRESS_WRAP:
            SampleInfo.i0 = WrapCoord(SampleInfo.i0, Width);
            SampleInfo.i1 = WrapCoord(SampleInfo.i1, Width);
            break;

        case TEXTURE_ADDRESS_MIRROR:
            SampleInfo.i0 = MirrorCoord(SampleInfo.i0, Width);
            SampleInfo.i1 = MirrorCoord(SampleInfo.i1, Width);
            break;

        case TEXTURE_ADDRESS_CLAMP:
            SampleInfo.i0 = clamp(SampleInfo.i0, 0, static_cast<Int32>(Width - 1));
            SampleInfo.i1 = clamp(SampleInfo.i1, 0, static_cast<Int32>(Width - 1));
            break;

        default:
            UNEXPECTED("Unexpected texture address mode");
    }

    return SampleInfo;
}

#ifdef DILIGENT_DEBUG
template <TEXTURE_ADDRESS_MODE AddressMode>
void _DbgVerifyFilterInfo(const LinearTexFilterSampleInfo& FilterInfo, Uint32 Width, const char* Direction, float u)
{
}

template <>
inline void _DbgVerifyFilterInfo<TEXTURE_ADDRESS_UNKNOWN>(const LinearTexFilterSampleInfo& FilterInfo, Uint32 Width, const char* Direction, float u)
{
    VERIFY(FilterInfo.i0 >= 0 && FilterInfo.i0 < static_cast<Int32>(Width), "First ", Direction, " sample index (", FilterInfo.i0,
           ") is out of allowed range [0, ", Width - 1, "]. Correct sample coordinate (", u, ") or use one of the texture address modes.");
    VERIFY(FilterInfo.i1 >= 0 && FilterInfo.i1 < static_cast<Int32>(Width), "Second ", Direction, " sample index (", FilterInfo.i1,
           ") is out of allowed range [0, ", Width - 1, "]. Correct sample coordinate (", u, ") or use one of the texture address modes.");
}
#endif

/// Samples 2D texture using bilinear filter.
///
/// \tparam SrcType           - Source pixel type.
/// \tparam DstType           - Destination type.
/// \tparam AddressModeU      - U coordinate address mode.
/// \tparam AddressModeV      - V coordinate address mode.
/// \tparam IsNormalizedCoord - Whether sample coordinates are normalized.
///
/// \param [in] Width         - Texture width.
/// \param [in] Height        - Texture height.
/// \param [in] pData         - Pointer to the texture data.
/// \param [in] Stride        - Data stride, in pixels.
/// \param [in] u             - Sample u coordinate.
/// \param [in] v             - Sample v coordinate.
/// \return                   - Filtered texture sample.
template <typename SrcType,
          typename DstType,
          TEXTURE_ADDRESS_MODE AddressModeU,
          TEXTURE_ADDRESS_MODE AddressModeV,
          bool                 IsNormalizedCoord>
DstType FilterTexture2DBilinear(Uint32         Width,
                                Uint32         Height,
                                const SrcType* pData,
                                size_t         Stride,
                                float          u,
                                float          v)
{
    auto UFilterInfo = GetLinearTexFilterSampleInfo<AddressModeU, IsNormalizedCoord>(Width, u);
    auto VFilterInfo = GetLinearTexFilterSampleInfo<AddressModeV, IsNormalizedCoord>(Height, v);

#ifdef DILIGENT_DEBUG
    {
        _DbgVerifyFilterInfo<AddressModeU>(UFilterInfo, Width, "horizontal", u);
        _DbgVerifyFilterInfo<AddressModeV>(VFilterInfo, Height, "horizontal", v);
    }
#endif

    auto S00 = static_cast<DstType>(pData[UFilterInfo.i0 + VFilterInfo.i0 * Stride]);
    auto S10 = static_cast<DstType>(pData[UFilterInfo.i1 + VFilterInfo.i0 * Stride]);
    auto S01 = static_cast<DstType>(pData[UFilterInfo.i0 + VFilterInfo.i1 * Stride]);
    auto S11 = static_cast<DstType>(pData[UFilterInfo.i1 + VFilterInfo.i1 * Stride]);
    return lerp(lerp(S00, S10, UFilterInfo.w), lerp(S01, S11, UFilterInfo.w), VFilterInfo.w);
}

/// Specialization of FilterTexture2DBilinear function that uses CLAMP texture address mode
/// and takes normalized texture coordinates.
template <typename SrcType, typename DstType>
DstType FilterTexture2DBilinearClamp(Uint32         Width,
                                     Uint32         Height,
                                     const SrcType* pData,
                                     size_t         Stride,
                                     float          u,
                                     float          v)

{
    return FilterTexture2DBilinear<SrcType, DstType, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, true>(Width, Height, pData, Stride, u, v);
}

/// Specialization of FilterTexture2DBilinear function that uses CLAMP texture address mode
/// and takes unnormalized texture coordinates.
template <typename SrcType, typename DstType>
DstType FilterTexture2DBilinearClampUC(Uint32         Width,
                                       Uint32         Height,
                                       const SrcType* pData,
                                       size_t         Stride,
                                       float          u,
                                       float          v)

{
    return FilterTexture2DBilinear<SrcType, DstType, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, false>(Width, Height, pData, Stride, u, v);
}

} // namespace Diligent
