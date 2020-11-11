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
#include <unordered_map>
#include <array>

#include "VulkanTypeConversions.hpp"
#include "PlatformMisc.hpp"
#include "Align.hpp"

namespace Diligent
{

class TexFormatToVkFormatMapper
{
public:
    TexFormatToVkFormatMapper()
    {
        // clang-format off
        m_FmtToVkFmtMap[TEX_FORMAT_UNKNOWN] = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_TYPELESS] = VK_FORMAT_R32G32B32A32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_FLOAT]    = VK_FORMAT_R32G32B32A32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_UINT]     = VK_FORMAT_R32G32B32A32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_SINT]     = VK_FORMAT_R32G32B32A32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_TYPELESS] = VK_FORMAT_R32G32B32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_FLOAT]    = VK_FORMAT_R32G32B32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_UINT]     = VK_FORMAT_R32G32B32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_SINT]     = VK_FORMAT_R32G32B32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_TYPELESS] = VK_FORMAT_R16G16B16A16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_FLOAT]    = VK_FORMAT_R16G16B16A16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_UNORM]    = VK_FORMAT_R16G16B16A16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_UINT]     = VK_FORMAT_R16G16B16A16_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_SNORM]    = VK_FORMAT_R16G16B16A16_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_SINT]     = VK_FORMAT_R16G16B16A16_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RG32_TYPELESS] = VK_FORMAT_R32G32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG32_FLOAT]    = VK_FORMAT_R32G32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG32_UINT]     = VK_FORMAT_R32G32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG32_SINT]     = VK_FORMAT_R32G32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R32G8X24_TYPELESS]        = VK_FORMAT_D32_SFLOAT_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_D32_FLOAT_S8X24_UINT]     = VK_FORMAT_D32_SFLOAT_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS] = VK_FORMAT_D32_SFLOAT_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_X32_TYPELESS_G8X24_UINT]  = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RGB10A2_TYPELESS] = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB10A2_UNORM]    = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB10A2_UINT]     = VK_FORMAT_A2R10G10B10_UINT_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_R11G11B10_FLOAT]  = VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_TYPELESS]   = VK_FORMAT_R8G8B8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_UNORM]      = VK_FORMAT_R8G8B8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_UNORM_SRGB] = VK_FORMAT_R8G8B8A8_SRGB;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_UINT]       = VK_FORMAT_R8G8B8A8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_SNORM]      = VK_FORMAT_R8G8B8A8_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_SINT]       = VK_FORMAT_R8G8B8A8_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RG16_TYPELESS] = VK_FORMAT_R16G16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_FLOAT]    = VK_FORMAT_R16G16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_UNORM]    = VK_FORMAT_R16G16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_UINT]     = VK_FORMAT_R16G16_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_SNORM]    = VK_FORMAT_R16G16_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_SINT]     = VK_FORMAT_R16G16_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R32_TYPELESS] = VK_FORMAT_R32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_D32_FLOAT]    = VK_FORMAT_D32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_FLOAT]    = VK_FORMAT_R32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_UINT]     = VK_FORMAT_R32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_SINT]     = VK_FORMAT_R32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R24G8_TYPELESS]        = VK_FORMAT_D24_UNORM_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_D24_UNORM_S8_UINT]     = VK_FORMAT_D24_UNORM_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R24_UNORM_X8_TYPELESS] = VK_FORMAT_D24_UNORM_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_X24_TYPELESS_G8_UINT]  = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RG8_TYPELESS] = VK_FORMAT_R8G8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_UNORM]    = VK_FORMAT_R8G8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_UINT]     = VK_FORMAT_R8G8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_SNORM]    = VK_FORMAT_R8G8_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_SINT]     = VK_FORMAT_R8G8_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R16_TYPELESS] = VK_FORMAT_R16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_FLOAT]    = VK_FORMAT_R16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_D16_UNORM]    = VK_FORMAT_D16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_UNORM]    = VK_FORMAT_R16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_UINT]     = VK_FORMAT_R16_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_SNORM]    = VK_FORMAT_R16_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_SINT]     = VK_FORMAT_R16_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R8_TYPELESS] = VK_FORMAT_R8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_UNORM]    = VK_FORMAT_R8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_UINT]     = VK_FORMAT_R8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_SNORM]    = VK_FORMAT_R8_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_SINT]     = VK_FORMAT_R8_SINT;
        m_FmtToVkFmtMap[TEX_FORMAT_A8_UNORM]    = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_R1_UNORM]    = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RGB9E5_SHAREDEXP] = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_B8G8_UNORM]   = VK_FORMAT_UNDEFINED;
        m_FmtToVkFmtMap[TEX_FORMAT_G8R8_G8B8_UNORM]  = VK_FORMAT_UNDEFINED;

        // http://www.g-truc.net/post-0335.html
        // http://renderingpipeline.com/2012/07/texture-compression/
        m_FmtToVkFmtMap[TEX_FORMAT_BC1_TYPELESS]   = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC1_UNORM]      = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC1_UNORM_SRGB] = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC2_TYPELESS]   = VK_FORMAT_BC2_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC2_UNORM]      = VK_FORMAT_BC2_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC2_UNORM_SRGB] = VK_FORMAT_BC2_SRGB_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC3_TYPELESS]   = VK_FORMAT_BC3_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC3_UNORM]      = VK_FORMAT_BC3_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC3_UNORM_SRGB] = VK_FORMAT_BC3_SRGB_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC4_TYPELESS]   = VK_FORMAT_BC4_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC4_UNORM]      = VK_FORMAT_BC4_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC4_SNORM]      = VK_FORMAT_BC4_SNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC5_TYPELESS]   = VK_FORMAT_BC5_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC5_UNORM]      = VK_FORMAT_BC5_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC5_SNORM]      = VK_FORMAT_BC5_SNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_B5G6R5_UNORM]   = VK_FORMAT_B5G6R5_UNORM_PACK16;
        m_FmtToVkFmtMap[TEX_FORMAT_B5G5R5A1_UNORM] = VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRA8_UNORM]    = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRX8_UNORM]    = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM] = VK_FORMAT_UNDEFINED;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRA8_TYPELESS]   = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRA8_UNORM_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRX8_TYPELESS]   = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRX8_UNORM_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;
        m_FmtToVkFmtMap[TEX_FORMAT_BC6H_TYPELESS] = VK_FORMAT_BC6H_UFLOAT_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC6H_UF16]     = VK_FORMAT_BC6H_UFLOAT_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC6H_SF16]     = VK_FORMAT_BC6H_SFLOAT_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC7_TYPELESS]   = VK_FORMAT_BC7_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC7_UNORM]      = VK_FORMAT_BC7_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC7_UNORM_SRGB] = VK_FORMAT_BC7_SRGB_BLOCK;
        // clang-format on
    }

    VkFormat operator[](TEXTURE_FORMAT TexFmt) const
    {
        VERIFY_EXPR(TexFmt < _countof(m_FmtToVkFmtMap));
        return m_FmtToVkFmtMap[TexFmt];
    }

private:
    VkFormat m_FmtToVkFmtMap[TEX_FORMAT_NUM_FORMATS] = {};
};

VkFormat TexFormatToVkFormat(TEXTURE_FORMAT TexFmt)
{
    static const TexFormatToVkFormatMapper FmtMapper;
    return FmtMapper[TexFmt];
}



class VkFormatToTexFormatMapper
{
public:
    VkFormatToTexFormatMapper()
    {
        // clang-format off
        m_VkFmtToTexFmtMap[VK_FORMAT_UNDEFINED]                 = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R4G4_UNORM_PACK8]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R4G4B4A4_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B4G4R4A4_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R5G6B5_UNORM_PACK16]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B5G6R5_UNORM_PACK16]       = TEX_FORMAT_B5G6R5_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R5G5B5A1_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B5G5R5A1_UNORM_PACK16]     = TEX_FORMAT_B5G5R5A1_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_A1R5G5B5_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8_UNORM]                  = TEX_FORMAT_R8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SNORM]                  = TEX_FORMAT_R8_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_USCALED]                = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SSCALED]                = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_UINT]                   = TEX_FORMAT_R8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SINT]                   = TEX_FORMAT_R8_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SRGB]                   = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_UNORM]                = TEX_FORMAT_RG8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SNORM]                = TEX_FORMAT_RG8_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_USCALED]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SSCALED]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_UINT]                 = TEX_FORMAT_RG8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SINT]                 = TEX_FORMAT_RG8_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SRGB]                 = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_UNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_USCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SSCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_UINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SRGB]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_UNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_USCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SSCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_UINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SRGB]               = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_UNORM]            = TEX_FORMAT_RGBA8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SNORM]            = TEX_FORMAT_RGBA8_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_USCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SSCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_UINT]             = TEX_FORMAT_RGBA8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SINT]             = TEX_FORMAT_RGBA8_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SRGB]             = TEX_FORMAT_RGBA8_UNORM_SRGB;

        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_UNORM]            = TEX_FORMAT_BGRA8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SNORM]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_USCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SSCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_UINT]             = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SINT]             = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SRGB]             = TEX_FORMAT_BGRA8_UNORM_SRGB;

        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_UNORM_PACK32]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SNORM_PACK32]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_USCALED_PACK32]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SSCALED_PACK32]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_UINT_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SINT_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SRGB_PACK32]      = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_UNORM_PACK32]  = TEX_FORMAT_RGB10A2_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_SNORM_PACK32]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_USCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_SSCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_UINT_PACK32]       = TEX_FORMAT_RGB10A2_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_SINT_PACK32]       = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_UNORM_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_SNORM_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_USCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_SSCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_UINT_PACK32]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_SINT_PACK32]       = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16_UNORM]             = TEX_FORMAT_R16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SNORM]             = TEX_FORMAT_R16_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_USCALED]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SSCALED]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_UINT]              = TEX_FORMAT_R16_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SINT]              = TEX_FORMAT_R16_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SFLOAT]            = TEX_FORMAT_R16_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_UNORM]          = TEX_FORMAT_RG16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SNORM]          = TEX_FORMAT_RG16_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_USCALED]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SSCALED]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_UINT]           = TEX_FORMAT_RG16_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SINT]           = TEX_FORMAT_RG16_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SFLOAT]         = TEX_FORMAT_RG16_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_UNORM]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SNORM]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_USCALED]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SSCALED]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_UINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SFLOAT]      = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_UNORM]    = TEX_FORMAT_RGBA16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SNORM]    = TEX_FORMAT_RGBA16_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_USCALED]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SSCALED]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_UINT]     = TEX_FORMAT_RGBA16_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SINT]     = TEX_FORMAT_RGBA16_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SFLOAT]   = TEX_FORMAT_RGBA16_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R32_UINT]              = TEX_FORMAT_R32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32_SINT]              = TEX_FORMAT_R32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32_SFLOAT]            = TEX_FORMAT_R32_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32_UINT]           = TEX_FORMAT_RG32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32_SINT]           = TEX_FORMAT_RG32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32_SFLOAT]         = TEX_FORMAT_RG32_FLOAT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32_UINT]        = TEX_FORMAT_RGB32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32_SINT]        = TEX_FORMAT_RGB32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32_SFLOAT]      = TEX_FORMAT_RGB32_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32A32_UINT]     = TEX_FORMAT_RGBA32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32A32_SINT]     = TEX_FORMAT_RGBA32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32A32_SFLOAT]   = TEX_FORMAT_RGBA32_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64_UINT]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64_SINT]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64_SFLOAT]            = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64_UINT]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64_SINT]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64_SFLOAT]         = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64_UINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64_SINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64_SFLOAT]      = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64A64_UINT]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64A64_SINT]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64A64_SFLOAT]   = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_B10G11R11_UFLOAT_PACK32]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_E5B9G9R9_UFLOAT_PACK32]    = TEX_FORMAT_RGB9E5_SHAREDEXP;
        m_VkFmtToTexFmtMap[VK_FORMAT_D16_UNORM]                 = TEX_FORMAT_D16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_X8_D24_UNORM_PACK32]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_D32_SFLOAT]                = TEX_FORMAT_D32_FLOAT;
        m_VkFmtToTexFmtMap[VK_FORMAT_S8_UINT]                   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_D16_UNORM_S8_UINT]         = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_D24_UNORM_S8_UINT]         = TEX_FORMAT_D24_UNORM_S8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_D32_SFLOAT_S8_UINT]        = TEX_FORMAT_D32_FLOAT_S8X24_UINT;

        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGB_UNORM_BLOCK]       = TEX_FORMAT_BC1_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGB_SRGB_BLOCK]        = TEX_FORMAT_BC1_UNORM_SRGB;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGBA_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGBA_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC2_UNORM_BLOCK]           = TEX_FORMAT_BC2_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC2_SRGB_BLOCK]            = TEX_FORMAT_BC2_UNORM_SRGB;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC3_UNORM_BLOCK]           = TEX_FORMAT_BC3_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC3_SRGB_BLOCK]            = TEX_FORMAT_BC3_UNORM_SRGB;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC4_UNORM_BLOCK]           = TEX_FORMAT_BC4_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC4_SNORM_BLOCK]           = TEX_FORMAT_BC4_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC5_UNORM_BLOCK]           = TEX_FORMAT_BC5_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC5_SNORM_BLOCK]           = TEX_FORMAT_BC5_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC6H_UFLOAT_BLOCK]         = TEX_FORMAT_BC6H_UF16;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC6H_SFLOAT_BLOCK]         = TEX_FORMAT_BC6H_SF16;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC7_UNORM_BLOCK]           = TEX_FORMAT_BC7_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC7_SRGB_BLOCK]            = TEX_FORMAT_BC7_UNORM_SRGB;

        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11_UNORM_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11_SNORM_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11G11_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11G11_SNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_4x4_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_4x4_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x4_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x4_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x5_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x5_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x5_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x5_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x6_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x6_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x5_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x5_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x6_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x6_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x8_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x8_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x5_UNORM_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x5_SRGB_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x6_UNORM_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x6_SRGB_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x8_UNORM_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x8_SRGB_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x10_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x10_SRGB_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x10_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x10_SRGB_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x12_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x12_SRGB_BLOCK]     = TEX_FORMAT_UNKNOWN;



        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8B8G8R8_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B8G8R8G8_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8R8_2PLANE_420_UNORM]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8R8_2PLANE_422_UNORM]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R10X6_UNORM_PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R10X6G10X6_UNORM_2PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R12X4_UNORM_PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R12X4G12X4_UNORM_2PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16B16G16R16_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B16G16R16G16_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16R16_2PLANE_420_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16R16_2PLANE_422_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        // clang-format on
    }

    TEXTURE_FORMAT operator[](VkFormat VkFmt) const
    {
        if (VkFmt < VK_FORMAT_RANGE_SIZE)
        {
            return m_VkFmtToTexFmtMap[VkFmt];
        }
        else
        {
            auto it = m_VkFmtToTexFmtMapExt.find(VkFmt);
            return it != m_VkFmtToTexFmtMapExt.end() ? it->second : TEX_FORMAT_UNKNOWN;
        }
    }

private:
    TEXTURE_FORMAT                               m_VkFmtToTexFmtMap[VK_FORMAT_RANGE_SIZE] = {};
    std::unordered_map<VkFormat, TEXTURE_FORMAT> m_VkFmtToTexFmtMapExt;
};

TEXTURE_FORMAT VkFormatToTexFormat(VkFormat VkFmt)
{
    static const VkFormatToTexFormatMapper FmtMapper;
    return FmtMapper[VkFmt];
}



VkFormat TypeToVkFormat(VALUE_TYPE ValType, Uint32 NumComponents, Bool bIsNormalized)
{
    switch (ValType)
    {
        case VT_FLOAT16:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R16_SFLOAT;
                case 2: return VK_FORMAT_R16G16_SFLOAT;
                case 3: return VK_FORMAT_R16G16B16_SFLOAT;
                case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
                default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_FLOAT32:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R32_SFLOAT;
                case 2: return VK_FORMAT_R32G32_SFLOAT;
                case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_INT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R32_SINT;
                case 2: return VK_FORMAT_R32G32_SINT;
                case 3: return VK_FORMAT_R32G32B32_SINT;
                case 4: return VK_FORMAT_R32G32B32A32_SINT;
                default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_UINT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R32_UINT;
                case 2: return VK_FORMAT_R32G32_UINT;
                case 3: return VK_FORMAT_R32G32B32_UINT;
                case 4: return VK_FORMAT_R32G32B32A32_UINT;
                default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_INT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_SNORM;
                    case 2: return VK_FORMAT_R16G16_SNORM;
                    case 3: return VK_FORMAT_R16G16B16_SNORM;
                    case 4: return VK_FORMAT_R16G16B16A16_SNORM;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_SINT;
                    case 2: return VK_FORMAT_R16G16_SINT;
                    case 3: return VK_FORMAT_R16G16B16_SINT;
                    case 4: return VK_FORMAT_R16G16B16A16_SINT;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        case VT_UINT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_UNORM;
                    case 2: return VK_FORMAT_R16G16_UNORM;
                    case 3: return VK_FORMAT_R16G16B16_UNORM;
                    case 4: return VK_FORMAT_R16G16B16A16_UNORM;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_UINT;
                    case 2: return VK_FORMAT_R16G16_UINT;
                    case 3: return VK_FORMAT_R16G16B16_UINT;
                    case 4: return VK_FORMAT_R16G16B16A16_UINT;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        case VT_INT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_SNORM;
                    case 2: return VK_FORMAT_R8G8_SNORM;
                    case 3: return VK_FORMAT_R8G8B8_SNORM;
                    case 4: return VK_FORMAT_R8G8B8A8_SNORM;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_SINT;
                    case 2: return VK_FORMAT_R8G8_SINT;
                    case 3: return VK_FORMAT_R8G8B8_SINT;
                    case 4: return VK_FORMAT_R8G8B8A8_SINT;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        case VT_UINT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_UNORM;
                    case 2: return VK_FORMAT_R8G8_UNORM;
                    case 3: return VK_FORMAT_R8G8B8_UNORM;
                    case 4: return VK_FORMAT_R8G8B8A8_UNORM;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_UINT;
                    case 2: return VK_FORMAT_R8G8_UINT;
                    case 3: return VK_FORMAT_R8G8B8_UINT;
                    case 4: return VK_FORMAT_R8G8B8A8_UINT;
                    default: UNEXPECTED("Unusupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        default: UNEXPECTED("Unusupported format"); return VK_FORMAT_UNDEFINED;
    }
}

VkPolygonMode FillModeToVkPolygonMode(FILL_MODE FillMode)
{
    switch (FillMode)
    {
        case FILL_MODE_UNDEFINED:
            UNEXPECTED("Undefined fill mode");
            return VK_POLYGON_MODE_FILL;

        // clang-format off
        case FILL_MODE_SOLID:     return VK_POLYGON_MODE_FILL;
        case FILL_MODE_WIREFRAME: return VK_POLYGON_MODE_LINE;
            // clang-format on

        default:
            UNEXPECTED("Unexpected fill mode");
            return VK_POLYGON_MODE_FILL;
    }
}

VkCullModeFlagBits CullModeToVkCullMode(CULL_MODE CullMode)
{
    switch (CullMode)
    {
        case CULL_MODE_UNDEFINED:
            UNEXPECTED("Undefined cull mode");
            return VK_CULL_MODE_NONE;

        // clang-format off
        case CULL_MODE_NONE:  return VK_CULL_MODE_NONE;
        case CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case CULL_MODE_BACK:  return VK_CULL_MODE_BACK_BIT;
            // clang-format on

        default:
            UNEXPECTED("Unexpected cull mode");
            return VK_CULL_MODE_NONE;
    }
}

VkPipelineRasterizationStateCreateInfo RasterizerStateDesc_To_VkRasterizationStateCI(const RasterizerStateDesc& RasterizerDesc)
{
    VkPipelineRasterizationStateCreateInfo RSStateCI = {};

    RSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RSStateCI.pNext = nullptr;
    RSStateCI.flags = 0; // Reserved for future use.

    // If depth clamping is enabled, before the incoming fragment's zf is compared to za, zf is clamped to
    // [min(n,f), max(n,f)], where n and f are the minDepth and maxDepth depth range values of the viewport
    // used by this fragment, respectively (25.10)
    // This value is the opposite of clip enable
    RSStateCI.depthClampEnable = RasterizerDesc.DepthClipEnable ? VK_FALSE : VK_TRUE;

    RSStateCI.rasterizerDiscardEnable = VK_FALSE;                                                                                         // Whether primitives are discarded immediately before the rasterization stage.
    RSStateCI.polygonMode             = FillModeToVkPolygonMode(RasterizerDesc.FillMode);                                                 // 24.7.2
    RSStateCI.cullMode                = CullModeToVkCullMode(RasterizerDesc.CullMode);                                                    // 24.7.1
    RSStateCI.frontFace               = RasterizerDesc.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE; // 24.7.1
    // Depth bias (24.7.3)
    RSStateCI.depthBiasEnable = (RasterizerDesc.DepthBias != 0 || RasterizerDesc.SlopeScaledDepthBias != 0.f) ? VK_TRUE : VK_FALSE;
    RSStateCI.depthBiasConstantFactor =
        static_cast<float>(RasterizerDesc.DepthBias);         // a scalar factor applied to an implementation-dependent constant
                                                              // that relates to the usable resolution of the depth buffer
    RSStateCI.depthBiasClamp = RasterizerDesc.DepthBiasClamp; // maximum (or minimum) depth bias of a fragment.
    RSStateCI.depthBiasSlopeFactor =
        RasterizerDesc.SlopeScaledDepthBias; //  a scalar factor applied to a fragment's slope in depth bias calculations.
    RSStateCI.lineWidth = 1.f;               // If the wide lines feature is not enabled, and no element of the pDynamicStates member of
                                             // pDynamicState is VK_DYNAMIC_STATE_LINE_WIDTH, the lineWidth member of
                                             // pRasterizationState must be 1.0 (9.2)

    return RSStateCI;
}

VkCompareOp ComparisonFuncToVkCompareOp(COMPARISON_FUNCTION CmpFunc)
{
    switch (CmpFunc)
    {
        // clang-format off
        case COMPARISON_FUNC_UNKNOWN: 
            UNEXPECTED("Comparison function is not specified" ); 
            return VK_COMPARE_OP_ALWAYS;

        case COMPARISON_FUNC_NEVER:         return VK_COMPARE_OP_NEVER;
        case COMPARISON_FUNC_LESS:          return VK_COMPARE_OP_LESS;
        case COMPARISON_FUNC_EQUAL:         return VK_COMPARE_OP_EQUAL;
        case COMPARISON_FUNC_LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case COMPARISON_FUNC_GREATER:       return VK_COMPARE_OP_GREATER;
        case COMPARISON_FUNC_NOT_EQUAL:     return VK_COMPARE_OP_NOT_EQUAL;
        case COMPARISON_FUNC_GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case COMPARISON_FUNC_ALWAYS:        return VK_COMPARE_OP_ALWAYS;
            // clang-format on

        default:
            UNEXPECTED("Unknown comparison function");
            return VK_COMPARE_OP_ALWAYS;
    }
}

VkStencilOp StencilOpToVkStencilOp(STENCIL_OP StencilOp)
{
    switch (StencilOp)
    {
        // clang-format off
        case STENCIL_OP_UNDEFINED:
            UNEXPECTED("Undefined stencil operation");
            return VK_STENCIL_OP_KEEP;

        case STENCIL_OP_KEEP:       return VK_STENCIL_OP_KEEP;
        case STENCIL_OP_ZERO:       return VK_STENCIL_OP_ZERO;
        case STENCIL_OP_REPLACE:    return VK_STENCIL_OP_REPLACE;
        case STENCIL_OP_INCR_SAT:   return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case STENCIL_OP_DECR_SAT:   return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case STENCIL_OP_INVERT:     return VK_STENCIL_OP_INVERT;
        case STENCIL_OP_INCR_WRAP:  return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case STENCIL_OP_DECR_WRAP:  return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            // clang-format on

        default:
            UNEXPECTED("Unknown stencil operation");
            return VK_STENCIL_OP_KEEP;
    }
}

VkStencilOpState StencilOpDescToVkStencilOpState(const StencilOpDesc& desc, Uint8 StencilReadMask, Uint8 StencilWriteMask)
{
    // Stencil state (25.9)
    VkStencilOpState StencilState = {};
    StencilState.failOp           = StencilOpToVkStencilOp(desc.StencilFailOp);
    StencilState.passOp           = StencilOpToVkStencilOp(desc.StencilPassOp);
    StencilState.depthFailOp      = StencilOpToVkStencilOp(desc.StencilDepthFailOp);
    StencilState.compareOp        = ComparisonFuncToVkCompareOp(desc.StencilFunc);

    // The s least significant bits of compareMask,  where s is the number of bits in the stencil framebuffer attachment,
    // are bitwise ANDed with both the reference and the stored stencil value, and the resulting masked values are those
    // that participate in the comparison controlled by compareOp (25.9)
    StencilState.compareMask = StencilReadMask;

    // The least significant s bits of writeMask, where s is the number of bits in the stencil framebuffer
    // attachment, specify an integer mask. Where a 1 appears in this mask, the corresponding bit in the stencil
    // value in the depth / stencil attachment is written; where a 0 appears, the bit is not written (25.9)
    StencilState.writeMask = StencilWriteMask;

    StencilState.reference = 0; // Set dynamically

    return StencilState;
}


VkPipelineDepthStencilStateCreateInfo DepthStencilStateDesc_To_VkDepthStencilStateCI(const DepthStencilStateDesc& DepthStencilDesc)
{
    // Depth-stencil state (25.7)
    VkPipelineDepthStencilStateCreateInfo DSStateCI = {};

    DSStateCI.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DSStateCI.pNext                 = nullptr;
    DSStateCI.flags                 = 0; // reserved for future use
    DSStateCI.depthTestEnable       = DepthStencilDesc.DepthEnable ? VK_TRUE : VK_FALSE;
    DSStateCI.depthWriteEnable      = DepthStencilDesc.DepthWriteEnable ? VK_TRUE : VK_FALSE;
    DSStateCI.depthCompareOp        = ComparisonFuncToVkCompareOp(DepthStencilDesc.DepthFunc); // 25.10
    DSStateCI.depthBoundsTestEnable = VK_FALSE;                                                // 25.8
    DSStateCI.stencilTestEnable     = DepthStencilDesc.StencilEnable ? VK_TRUE : VK_FALSE;     // 25.9
    DSStateCI.front                 = StencilOpDescToVkStencilOpState(DepthStencilDesc.FrontFace, DepthStencilDesc.StencilReadMask, DepthStencilDesc.StencilWriteMask);
    DSStateCI.back                  = StencilOpDescToVkStencilOpState(DepthStencilDesc.BackFace, DepthStencilDesc.StencilReadMask, DepthStencilDesc.StencilWriteMask);
    // Depth Bounds Test (25.8)
    DSStateCI.minDepthBounds = 0; // must be between 0.0 and 1.0, inclusive
    DSStateCI.maxDepthBounds = 1; // must be between 0.0 and 1.0, inclusive

    return DSStateCI;
}

class BlendFactorToVkBlendFactorMapper
{
public:
    BlendFactorToVkBlendFactorMapper()
    {
        // 26.1.1
        m_Map[BLEND_FACTOR_ZERO]             = VK_BLEND_FACTOR_ZERO;
        m_Map[BLEND_FACTOR_ONE]              = VK_BLEND_FACTOR_ONE;
        m_Map[BLEND_FACTOR_SRC_COLOR]        = VK_BLEND_FACTOR_SRC_COLOR;
        m_Map[BLEND_FACTOR_INV_SRC_COLOR]    = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        m_Map[BLEND_FACTOR_SRC_ALPHA]        = VK_BLEND_FACTOR_SRC_ALPHA;
        m_Map[BLEND_FACTOR_INV_SRC_ALPHA]    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_Map[BLEND_FACTOR_DEST_ALPHA]       = VK_BLEND_FACTOR_DST_ALPHA;
        m_Map[BLEND_FACTOR_INV_DEST_ALPHA]   = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        m_Map[BLEND_FACTOR_DEST_COLOR]       = VK_BLEND_FACTOR_DST_COLOR;
        m_Map[BLEND_FACTOR_INV_DEST_COLOR]   = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        m_Map[BLEND_FACTOR_SRC_ALPHA_SAT]    = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        m_Map[BLEND_FACTOR_BLEND_FACTOR]     = VK_BLEND_FACTOR_CONSTANT_COLOR;
        m_Map[BLEND_FACTOR_INV_BLEND_FACTOR] = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        m_Map[BLEND_FACTOR_SRC1_COLOR]       = VK_BLEND_FACTOR_SRC1_COLOR;
        m_Map[BLEND_FACTOR_INV_SRC1_COLOR]   = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        m_Map[BLEND_FACTOR_SRC1_ALPHA]       = VK_BLEND_FACTOR_SRC1_ALPHA;
        m_Map[BLEND_FACTOR_INV_SRC1_ALPHA]   = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }

    VkBlendFactor operator[](BLEND_FACTOR bf) const
    {
        VERIFY_EXPR(bf > BLEND_FACTOR_UNDEFINED && bf < BLEND_FACTOR_NUM_FACTORS);
        return m_Map[static_cast<int>(bf)];
    }

private:
    std::array<VkBlendFactor, BLEND_FACTOR_NUM_FACTORS> m_Map = {};
};


class LogicOperationToVkLogicOp
{
public:
    LogicOperationToVkLogicOp()
    {
        // 26.2
        m_Map[LOGIC_OP_CLEAR]         = VK_LOGIC_OP_CLEAR;
        m_Map[LOGIC_OP_SET]           = VK_LOGIC_OP_SET;
        m_Map[LOGIC_OP_COPY]          = VK_LOGIC_OP_COPY;
        m_Map[LOGIC_OP_COPY_INVERTED] = VK_LOGIC_OP_COPY_INVERTED;
        m_Map[LOGIC_OP_NOOP]          = VK_LOGIC_OP_NO_OP;
        m_Map[LOGIC_OP_INVERT]        = VK_LOGIC_OP_INVERT;
        m_Map[LOGIC_OP_AND]           = VK_LOGIC_OP_AND;
        m_Map[LOGIC_OP_NAND]          = VK_LOGIC_OP_NAND;
        m_Map[LOGIC_OP_OR]            = VK_LOGIC_OP_OR;
        m_Map[LOGIC_OP_NOR]           = VK_LOGIC_OP_NOR;
        m_Map[LOGIC_OP_XOR]           = VK_LOGIC_OP_XOR;
        m_Map[LOGIC_OP_EQUIV]         = VK_LOGIC_OP_EQUIVALENT;
        m_Map[LOGIC_OP_AND_REVERSE]   = VK_LOGIC_OP_AND_REVERSE;
        m_Map[LOGIC_OP_AND_INVERTED]  = VK_LOGIC_OP_AND_INVERTED;
        m_Map[LOGIC_OP_OR_REVERSE]    = VK_LOGIC_OP_OR_REVERSE;
        m_Map[LOGIC_OP_OR_INVERTED]   = VK_LOGIC_OP_OR_INVERTED;
    }

    VkLogicOp operator[](LOGIC_OPERATION op) const
    {
        VERIFY_EXPR(op >= LOGIC_OP_CLEAR && op < LOGIC_OP_NUM_OPERATIONS);
        return m_Map[static_cast<int>(op)];
    }

private:
    std::array<VkLogicOp, LOGIC_OP_NUM_OPERATIONS> m_Map = {};
};

class BlendOperationToVkBlendOp
{
public:
    BlendOperationToVkBlendOp()
    {
        // 26.1.3
        m_Map[BLEND_OPERATION_ADD]          = VK_BLEND_OP_ADD;
        m_Map[BLEND_OPERATION_SUBTRACT]     = VK_BLEND_OP_SUBTRACT;
        m_Map[BLEND_OPERATION_REV_SUBTRACT] = VK_BLEND_OP_REVERSE_SUBTRACT;
        m_Map[BLEND_OPERATION_MIN]          = VK_BLEND_OP_MIN;
        m_Map[BLEND_OPERATION_MAX]          = VK_BLEND_OP_MAX;
    }

    VkBlendOp operator[](BLEND_OPERATION op) const
    {
        VERIFY_EXPR(op > BLEND_OPERATION_UNDEFINED && op < BLEND_OPERATION_NUM_OPERATIONS);
        return m_Map[static_cast<int>(op)];
    }

private:
    std::array<VkBlendOp, BLEND_OPERATION_NUM_OPERATIONS> m_Map = {};
};

VkPipelineColorBlendAttachmentState RenderTargetBlendDescToVkColorBlendAttachmentState(const RenderTargetBlendDesc& RTBlendDesc)
{
    static const BlendFactorToVkBlendFactorMapper BFtoVKBF;
    static const BlendOperationToVkBlendOp        BOtoVKBO;
    VkPipelineColorBlendAttachmentState           AttachmentBlendState = {};

    AttachmentBlendState.blendEnable         = RTBlendDesc.BlendEnable;
    AttachmentBlendState.srcColorBlendFactor = BFtoVKBF[RTBlendDesc.SrcBlend];
    AttachmentBlendState.dstColorBlendFactor = BFtoVKBF[RTBlendDesc.DestBlend];
    AttachmentBlendState.colorBlendOp        = BOtoVKBO[RTBlendDesc.BlendOp];
    AttachmentBlendState.srcAlphaBlendFactor = BFtoVKBF[RTBlendDesc.SrcBlendAlpha];
    AttachmentBlendState.dstAlphaBlendFactor = BFtoVKBF[RTBlendDesc.DestBlendAlpha];
    AttachmentBlendState.alphaBlendOp        = BOtoVKBO[RTBlendDesc.BlendOpAlpha];
    AttachmentBlendState.colorWriteMask =
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_RED) ? VK_COLOR_COMPONENT_R_BIT : 0) |
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_GREEN) ? VK_COLOR_COMPONENT_G_BIT : 0) |
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_BLUE) ? VK_COLOR_COMPONENT_B_BIT : 0) |
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_ALPHA) ? VK_COLOR_COMPONENT_A_BIT : 0);

    return AttachmentBlendState;
}

void BlendStateDesc_To_VkBlendStateCI(const BlendStateDesc&                             BSDesc,
                                      VkPipelineColorBlendStateCreateInfo&              ColorBlendStateCI,
                                      std::vector<VkPipelineColorBlendAttachmentState>& ColorBlendAttachments)
{
    // Color blend state (26.1)
    static const LogicOperationToVkLogicOp LogicOpToVkLogicOp;
    ColorBlendStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendStateCI.pNext             = nullptr;
    ColorBlendStateCI.flags             = 0;                                            // reserved for future use
    ColorBlendStateCI.logicOpEnable     = BSDesc.RenderTargets[0].LogicOperationEnable; // 26.2
    ColorBlendStateCI.logicOp           = LogicOpToVkLogicOp[BSDesc.RenderTargets[0].LogicOp];
    ColorBlendStateCI.blendConstants[0] = 0.f; // We use dynamic blend constants
    ColorBlendStateCI.blendConstants[1] = 0.f;
    ColorBlendStateCI.blendConstants[2] = 0.f;
    ColorBlendStateCI.blendConstants[3] = 0.f;
    // attachmentCount must equal the colorAttachmentCount for the subpass in which this pipeline is used.
    for (uint32_t attachment = 0; attachment < ColorBlendStateCI.attachmentCount; ++attachment)
    {
        const auto& RTBlendState          = BSDesc.IndependentBlendEnable ? BSDesc.RenderTargets[attachment] : BSDesc.RenderTargets[0];
        ColorBlendAttachments[attachment] = RenderTargetBlendDescToVkColorBlendAttachmentState(RTBlendState);
    }
}

VkVertexInputRate LayoutElemFrequencyToVkInputRate(INPUT_ELEMENT_FREQUENCY frequency)
{
    switch (frequency)
    {
        case INPUT_ELEMENT_FREQUENCY_UNDEFINED:
            UNEXPECTED("Undefined layout element frequency");
            return VK_VERTEX_INPUT_RATE_VERTEX;

        case INPUT_ELEMENT_FREQUENCY_PER_VERTEX: return VK_VERTEX_INPUT_RATE_VERTEX;
        case INPUT_ELEMENT_FREQUENCY_PER_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;

        default:
            UNEXPECTED("Unknown layout element frequency");
            return VK_VERTEX_INPUT_RATE_VERTEX;
    }
}

void InputLayoutDesc_To_VkVertexInputStateCI(const InputLayoutDesc&                                              LayoutDesc,
                                             VkPipelineVertexInputStateCreateInfo&                               VertexInputStateCI,
                                             std::array<VkVertexInputBindingDescription, MAX_LAYOUT_ELEMENTS>&   BindingDescriptions,
                                             std::array<VkVertexInputAttributeDescription, MAX_LAYOUT_ELEMENTS>& AttributeDescription)
{
    // Vertex input description (20.2)
    VertexInputStateCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputStateCI.pNext                           = nullptr;
    VertexInputStateCI.flags                           = 0; // reserved for future use.
    VertexInputStateCI.vertexBindingDescriptionCount   = 0;
    VertexInputStateCI.pVertexBindingDescriptions      = BindingDescriptions.data();
    VertexInputStateCI.vertexAttributeDescriptionCount = LayoutDesc.NumElements;
    VertexInputStateCI.pVertexAttributeDescriptions    = AttributeDescription.data();
    std::array<Int32, MAX_LAYOUT_ELEMENTS> BufferSlot2BindingDescInd;
    BufferSlot2BindingDescInd.fill(-1);
    for (Uint32 elem = 0; elem < LayoutDesc.NumElements; ++elem)
    {
        auto& LayoutElem     = LayoutDesc.LayoutElements[elem];
        auto& BindingDescInd = BufferSlot2BindingDescInd[LayoutElem.BufferSlot];
        if (BindingDescInd < 0)
        {
            BindingDescInd        = VertexInputStateCI.vertexBindingDescriptionCount++;
            auto& BindingDesc     = BindingDescriptions[BindingDescInd];
            BindingDesc.binding   = LayoutElem.BufferSlot;
            BindingDesc.stride    = LayoutElem.Stride;
            BindingDesc.inputRate = LayoutElemFrequencyToVkInputRate(LayoutElem.Frequency);
        }

        const auto& BindingDesc = BindingDescriptions[BindingDescInd];
        VERIFY(BindingDesc.binding == LayoutElem.BufferSlot, "Inconsistent buffer slot");
        VERIFY(BindingDesc.stride == LayoutElem.Stride, "Inconsistent strides");
        VERIFY(BindingDesc.inputRate == LayoutElemFrequencyToVkInputRate(LayoutElem.Frequency), "Incosistent layout element frequency");

        auto& AttribDesc    = AttributeDescription[elem];
        AttribDesc.binding  = BindingDesc.binding;
        AttribDesc.location = LayoutElem.InputIndex;
        AttribDesc.format   = TypeToVkFormat(LayoutElem.ValueType, LayoutElem.NumComponents, LayoutElem.IsNormalized);
        AttribDesc.offset   = LayoutElem.RelativeOffset;
    }
}

void PrimitiveTopology_To_VkPrimitiveTopologyAndPatchCPCount(PRIMITIVE_TOPOLOGY   PrimTopology,
                                                             VkPrimitiveTopology& VkPrimTopology,
                                                             uint32_t&            PatchControlPoints)
{
    PatchControlPoints = 0;
    switch (PrimTopology)
    {
        case PRIMITIVE_TOPOLOGY_UNDEFINED:
            UNEXPECTED("Undefined primitive topology");
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            return;

        case PRIMITIVE_TOPOLOGY_POINT_LIST:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_LINE_LIST:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_LINE_STRIP:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            return;

        default:
            VERIFY_EXPR(PrimTopology >= PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST && PrimTopology < PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES);
            VkPrimTopology     = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            PatchControlPoints = static_cast<uint32_t>(PrimTopology) - static_cast<uint32_t>(PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST) + 1;
            return;
    }
}

VkFilter FilterTypeToVkFilter(FILTER_TYPE FilterType)
{
    switch (FilterType)
    {
        case FILTER_TYPE_UNKNOWN:
            UNEXPECTED("Unknown filter type");
            return VK_FILTER_NEAREST;

        case FILTER_TYPE_POINT:
        case FILTER_TYPE_COMPARISON_POINT:
        case FILTER_TYPE_MINIMUM_POINT:
        case FILTER_TYPE_MAXIMUM_POINT:
            return VK_FILTER_NEAREST;

        case FILTER_TYPE_LINEAR:
        case FILTER_TYPE_ANISOTROPIC:
        case FILTER_TYPE_COMPARISON_LINEAR:
        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
        case FILTER_TYPE_MINIMUM_LINEAR:
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:
        case FILTER_TYPE_MAXIMUM_LINEAR:
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:
            return VK_FILTER_LINEAR;

        default:
            UNEXPECTED("Unexpected filter type");
            return VK_FILTER_NEAREST;
    }
}

VkSamplerMipmapMode FilterTypeToVkMipmapMode(FILTER_TYPE FilterType)
{
    switch (FilterType)
    {
        case FILTER_TYPE_UNKNOWN:
            UNEXPECTED("Unknown filter type");
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case FILTER_TYPE_POINT:
        case FILTER_TYPE_COMPARISON_POINT:
        case FILTER_TYPE_MINIMUM_POINT:
        case FILTER_TYPE_MAXIMUM_POINT:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case FILTER_TYPE_LINEAR:
        case FILTER_TYPE_ANISOTROPIC:
        case FILTER_TYPE_COMPARISON_LINEAR:
        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
        case FILTER_TYPE_MINIMUM_LINEAR:
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:
        case FILTER_TYPE_MAXIMUM_LINEAR:
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;

        default:
            UNEXPECTED("Only point and linear filter types are allowed for mipmap mode");
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

VkSamplerAddressMode AddressModeToVkAddressMode(TEXTURE_ADDRESS_MODE AddressMode)
{
    switch (AddressMode)
    {
        case TEXTURE_ADDRESS_UNKNOWN:
            UNEXPECTED("Unknown address mode");
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        // clang-format off
        case TEXTURE_ADDRESS_WRAP:   return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TEXTURE_ADDRESS_MIRROR: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TEXTURE_ADDRESS_CLAMP:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TEXTURE_ADDRESS_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case TEXTURE_ADDRESS_MIRROR_ONCE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            // clang-format on

        default:
            UNEXPECTED("Unexpected address mode");
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

VkBorderColor BorderColorToVkBorderColor(const Float32 BorderColor[])
{
    VkBorderColor vkBorderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 0)
        vkBorderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    else if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 1)
        vkBorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    else if (BorderColor[0] == 1 && BorderColor[1] == 1 && BorderColor[2] == 1 && BorderColor[3] == 1)
        vkBorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    else
    {
        LOG_ERROR_MESSAGE("Vulkan samplers only allow transparent black (0,0,0,0), opaque black (0,0,0,1) or opaque white (1,1,1,1) as border colors.");
    }

    return vkBorderColor;
}


static VkAccessFlags ResourceStateFlagToVkAccessFlags(RESOURCE_STATE StateFlag)
{
    // Currently not used:
    //VK_ACCESS_HOST_READ_BIT
    //VK_ACCESS_HOST_WRITE_BIT
    //VK_ACCESS_MEMORY_READ_BIT
    //VK_ACCESS_MEMORY_WRITE_BIT
    //VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT
    //VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT
    //VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT
    //VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX
    //VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX
    //VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT
    //VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV
    //VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NVX
    //VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NVX

    static_assert(RESOURCE_STATE_MAX_BIT == 0x10000, "This function must be updated to handle new resource state flag");
    VERIFY((StateFlag & (StateFlag - 1)) == 0, "Only single bit must be set");
    switch (StateFlag)
    {
        // clang-format off
        case RESOURCE_STATE_UNDEFINED:         return 0;
        case RESOURCE_STATE_VERTEX_BUFFER:     return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        case RESOURCE_STATE_CONSTANT_BUFFER:   return VK_ACCESS_UNIFORM_READ_BIT;
        case RESOURCE_STATE_INDEX_BUFFER:      return VK_ACCESS_INDEX_READ_BIT;
        case RESOURCE_STATE_RENDER_TARGET:     return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case RESOURCE_STATE_UNORDERED_ACCESS:  return VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        case RESOURCE_STATE_DEPTH_WRITE:       return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case RESOURCE_STATE_DEPTH_READ:        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case RESOURCE_STATE_SHADER_RESOURCE:   return VK_ACCESS_SHADER_READ_BIT;
        case RESOURCE_STATE_STREAM_OUT:        return VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
        case RESOURCE_STATE_INDIRECT_ARGUMENT: return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        case RESOURCE_STATE_COPY_DEST:         return VK_ACCESS_TRANSFER_WRITE_BIT;
        case RESOURCE_STATE_COPY_SOURCE:       return VK_ACCESS_TRANSFER_READ_BIT;
        case RESOURCE_STATE_RESOLVE_DEST:      return VK_ACCESS_TRANSFER_WRITE_BIT;
        case RESOURCE_STATE_RESOLVE_SOURCE:    return VK_ACCESS_TRANSFER_READ_BIT;
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case RESOURCE_STATE_PRESENT:           return 0;
            // clang-format on

        default:
            UNEXPECTED("Unexpected resource state flag");
            return 0;
    }
}

class StateFlagBitPosToVkAccessFlags
{
public:
    StateFlagBitPosToVkAccessFlags()
    {
        static_assert((1 << MaxFlagBitPos) == RESOURCE_STATE_MAX_BIT, "This function must be updated to handle new resource state flag");
        for (Uint32 bit = 0; bit < MaxFlagBitPos; ++bit)
        {
            FlagBitPosToVkAccessFlagsMap[bit] = ResourceStateFlagToVkAccessFlags(static_cast<RESOURCE_STATE>(1 << bit));
        }
    }

    VkAccessFlags operator()(Uint32 BitPos) const
    {
        VERIFY(BitPos <= MaxFlagBitPos, "Resource state flag bit position (", BitPos, ") exceeds max bit position (", Uint32{MaxFlagBitPos}, ")");
        return FlagBitPosToVkAccessFlagsMap[BitPos];
    }

private:
    static constexpr const Uint32                MaxFlagBitPos = 16;
    std::array<VkAccessFlags, MaxFlagBitPos + 1> FlagBitPosToVkAccessFlagsMap;
};


VkAccessFlags ResourceStateFlagsToVkAccessFlags(RESOURCE_STATE StateFlags)
{
    VERIFY(Uint32{StateFlags} < (RESOURCE_STATE_MAX_BIT << 1), "Resource state flags are out of range");
    static const StateFlagBitPosToVkAccessFlags BitPosToAccessFlags;

    VkAccessFlags AccessFlags = 0;
    Uint32        Bits        = StateFlags;
    while (Bits != 0)
    {
        auto lsb = PlatformMisc::GetLSB(Bits);
        AccessFlags |= BitPosToAccessFlags(lsb);
        Bits &= ~(1 << lsb);
    }
    return AccessFlags;
}

RESOURCE_STATE VkAccessFlagsToResourceStates(VkAccessFlagBits AccessFlagBit)
{
    VERIFY((AccessFlagBit & (AccessFlagBit - 1)) == 0, "Single access flag bit is expected");

    switch (AccessFlagBit)
    {
        // clang-format off
        case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:                 return RESOURCE_STATE_INDIRECT_ARGUMENT;
        case VK_ACCESS_INDEX_READ_BIT:                            return RESOURCE_STATE_INDEX_BUFFER;
        case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:                 return RESOURCE_STATE_VERTEX_BUFFER;
        case VK_ACCESS_UNIFORM_READ_BIT:                          return RESOURCE_STATE_CONSTANT_BUFFER;
        case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:                 return RESOURCE_STATE_INPUT_ATTACHMENT;
        case VK_ACCESS_SHADER_READ_BIT:                           return RESOURCE_STATE_SHADER_RESOURCE;
        case VK_ACCESS_SHADER_WRITE_BIT:                          return RESOURCE_STATE_UNORDERED_ACCESS;
        case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:                 return RESOURCE_STATE_RENDER_TARGET;
        case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:                return RESOURCE_STATE_RENDER_TARGET;
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:         return RESOURCE_STATE_DEPTH_READ;
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:        return RESOURCE_STATE_DEPTH_WRITE;
        case VK_ACCESS_TRANSFER_READ_BIT:                         return RESOURCE_STATE_COPY_SOURCE;
        case VK_ACCESS_TRANSFER_WRITE_BIT:                        return RESOURCE_STATE_COPY_DEST;
        case VK_ACCESS_HOST_READ_BIT:                             return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_HOST_WRITE_BIT:                            return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_MEMORY_READ_BIT:                           return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_MEMORY_WRITE_BIT:                          return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT:          return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT:   return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT:  return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT:        return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV:            return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV:           return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT: return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV:            return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV:        return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV:       return RESOURCE_STATE_UNKNOWN;
            // clang-format on
        default:
            UNEXPECTED("Unknown access flag");
            return RESOURCE_STATE_UNKNOWN;
    }
}


class VkAccessFlagBitPosToResourceState
{
public:
    VkAccessFlagBitPosToResourceState()
    {
        for (Uint32 bit = 0; bit < MaxFlagBitPos; ++bit)
        {
            FlagBitPosToResourceState[bit] = VkAccessFlagsToResourceStates(static_cast<VkAccessFlagBits>(1 << bit));
        }
    }

    RESOURCE_STATE operator()(Uint32 BitPos) const
    {
        VERIFY(BitPos <= MaxFlagBitPos, "Resource state flag bit position (", BitPos, ") exceeds max bit position (", Uint32{MaxFlagBitPos}, ")");
        return FlagBitPosToResourceState[BitPos];
    }

private:
    static constexpr const Uint32                 MaxFlagBitPos = 20;
    std::array<RESOURCE_STATE, MaxFlagBitPos + 1> FlagBitPosToResourceState;
};


RESOURCE_STATE VkAccessFlagsToResourceStates(VkAccessFlags AccessFlags)
{
    static const VkAccessFlagBitPosToResourceState BitPosToState;
    Uint32                                         State = 0;
    while (AccessFlags != 0)
    {
        auto lsb = PlatformMisc::GetLSB(AccessFlags);
        State |= BitPosToState(lsb);
        AccessFlags &= ~(1 << lsb);
    }
    return static_cast<RESOURCE_STATE>(State);
}



VkImageLayout ResourceStateToVkImageLayout(RESOURCE_STATE StateFlag, bool IsInsideRenderPass)
{
    if (StateFlag == RESOURCE_STATE_UNKNOWN)
    {
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
    // Currently not used:
    //VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
    //VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
    //VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV
    //VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
    //VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,

    static_assert(RESOURCE_STATE_MAX_BIT == 0x10000, "This function must be updated to handle new resource state flag");
    VERIFY((StateFlag & (StateFlag - 1)) == 0, "Only single bit must be set");
    switch (StateFlag)
    {
        // clang-format off
        case RESOURCE_STATE_UNDEFINED:         return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_VERTEX_BUFFER:     UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_CONSTANT_BUFFER:   UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_INDEX_BUFFER:      UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_RENDER_TARGET:     return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case RESOURCE_STATE_UNORDERED_ACCESS:  return VK_IMAGE_LAYOUT_GENERAL;
        case RESOURCE_STATE_DEPTH_WRITE:       return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case RESOURCE_STATE_DEPTH_READ:        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case RESOURCE_STATE_SHADER_RESOURCE:   return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case RESOURCE_STATE_STREAM_OUT:        UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_INDIRECT_ARGUMENT: UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_COPY_DEST:         return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case RESOURCE_STATE_COPY_SOURCE:       return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case RESOURCE_STATE_RESOLVE_DEST:      return IsInsideRenderPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case RESOURCE_STATE_RESOLVE_SOURCE:    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case RESOURCE_STATE_PRESENT:           return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            // clang-format on

        default:
            UNEXPECTED("Unexpected resource state flag (", StateFlag, ")");
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

RESOURCE_STATE VkImageLayoutToResourceState(VkImageLayout Layout)
{
    static_assert(RESOURCE_STATE_MAX_BIT == 0x10000, "This function must be updated to handle new resource state flag");
    switch (Layout)
    {
        // clang-format off
        case VK_IMAGE_LAYOUT_UNDEFINED:                                  return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_GENERAL:                                    return RESOURCE_STATE_UNORDERED_ACCESS;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:                   return RESOURCE_STATE_RENDER_TARGET;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:           return RESOURCE_STATE_DEPTH_WRITE;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:            return RESOURCE_STATE_DEPTH_READ;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:                   return RESOURCE_STATE_SHADER_RESOURCE;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:                       return RESOURCE_STATE_COPY_SOURCE;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:                       return RESOURCE_STATE_COPY_DEST;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:                             UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                            return RESOURCE_STATE_PRESENT;
        case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:                         UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:                    UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        default:
            UNEXPECTED("Unknown image layout (", Layout, ")");
            return RESOURCE_STATE_UNDEFINED;
            // clang-format on
    }
}

SURFACE_TRANSFORM VkSurfaceTransformFlagToSurfaceTransform(VkSurfaceTransformFlagBitsKHR vkTransformFlag)
{
    VERIFY(IsPowerOfTwo(static_cast<Uint32>(vkTransformFlag)), "Only single transform bit is expected");

    // clang-format off
    switch (vkTransformFlag)
    {
        case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:                     return SURFACE_TRANSFORM_IDENTITY;
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:                    return SURFACE_TRANSFORM_ROTATE_90;
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:                   return SURFACE_TRANSFORM_ROTATE_180;
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:                   return SURFACE_TRANSFORM_ROTATE_270;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:            return SURFACE_TRANSFORM_HORIZONTAL_MIRROR;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:  return SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR: return SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR: return SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270;

        default:
            UNEXPECTED("Unexpected surface transform");
            return SURFACE_TRANSFORM_IDENTITY;
    }
    // clang-format on
}

VkSurfaceTransformFlagBitsKHR SurfaceTransformToVkSurfaceTransformFlag(SURFACE_TRANSFORM SrfTransform)
{
    // clang-format off
    switch (SrfTransform)
    {
        case SURFACE_TRANSFORM_OPTIMAL:
            UNEXPECTED("Optimal transform does not have corresponding Vulkan flag");
            return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

        case SURFACE_TRANSFORM_IDENTITY:                      return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        case SURFACE_TRANSFORM_ROTATE_90:                     return VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
        case SURFACE_TRANSFORM_ROTATE_180:                    return VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR;
        case SURFACE_TRANSFORM_ROTATE_270:                    return VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR:             return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:   return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:  return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:  return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR; 

        default:
            UNEXPECTED("Unexpected surface transform");
            return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    // clang-format on
}


#define ASSERT_SAME(Val1, Val2) static_assert(static_cast<int>(Val1) == static_cast<int>(Val2), #Val1 " is expected to be equal to " #Val2)

ASSERT_SAME(ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD);
ASSERT_SAME(ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_CLEAR);
ASSERT_SAME(ATTACHMENT_LOAD_OP_DISCARD, VK_ATTACHMENT_LOAD_OP_DONT_CARE);
VkAttachmentLoadOp AttachmentLoadOpToVkAttachmentLoadOp(ATTACHMENT_LOAD_OP LoadOp)
{
    return static_cast<VkAttachmentLoadOp>(LoadOp);
}
ATTACHMENT_LOAD_OP VkAttachmentLoadOpToAttachmentLoadOp(VkAttachmentLoadOp VkLoadOp)
{
    return static_cast<ATTACHMENT_LOAD_OP>(VkLoadOp);
}


ASSERT_SAME(ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_STORE);
ASSERT_SAME(ATTACHMENT_STORE_OP_DISCARD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
VkAttachmentStoreOp AttachmentStoreOpToVkAttachmentStoreOp(ATTACHMENT_STORE_OP StoreOp)
{
    return static_cast<VkAttachmentStoreOp>(StoreOp);
}
ATTACHMENT_STORE_OP VkAttachmentStoreOpToAttachmentStoreOp(VkAttachmentStoreOp VkStoreOp)
{
    return static_cast<ATTACHMENT_STORE_OP>(VkStoreOp);
}


// clang-format off
ASSERT_SAME(PIPELINE_STAGE_FLAG_TOP_OF_PIPE,                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_DRAW_INDIRECT,                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_VERTEX_INPUT,                 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_VERTEX_SHADER,                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_HULL_SHADER,                  VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_DOMAIN_SHADER,                VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_GEOMETRY_SHADER,              VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_PIXEL_SHADER,                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS,         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS,          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_RENDER_TARGET,                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_COMPUTE_SHADER,               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_TRANSFER,                     VK_PIPELINE_STAGE_TRANSFER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_BOTTOM_OF_PIPE,               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_HOST,                         VK_PIPELINE_STAGE_HOST_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING,        VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE,         VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER,           VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_TASK_SHADER,                  VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_MESH_SHADER,                  VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_FRAGMENT_DENSITY_PROCESS,     VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT);
// clang-format on
VkPipelineStageFlags PipelineStageFlagsToVkPipelineStageFlags(PIPELINE_STAGE_FLAGS PipelineStageFlags)
{
    return static_cast<VkPipelineStageFlags>(PipelineStageFlags);
}


// clang-format off
static_assert(ACCESS_FLAG_NONE == 0, "");
ASSERT_SAME(ACCESS_FLAG_INDIRECT_COMMAND_READ,        VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_INDEX_READ,                   VK_ACCESS_INDEX_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_VERTEX_READ,                  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_UNIFORM_READ,                 VK_ACCESS_UNIFORM_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_INPUT_ATTACHMENT_READ,        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_SHADER_READ,                  VK_ACCESS_SHADER_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_SHADER_WRITE,                 VK_ACCESS_SHADER_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_RENDER_TARGET_READ,           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_RENDER_TARGET_WRITE,          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_DEPTH_STENCIL_READ,           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_DEPTH_STENCIL_WRITE,          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_COPY_SRC,                     VK_ACCESS_TRANSFER_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_COPY_DST,                     VK_ACCESS_TRANSFER_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_HOST_READ,                    VK_ACCESS_HOST_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_HOST_WRITE,                   VK_ACCESS_HOST_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_MEMORY_READ,                  VK_ACCESS_MEMORY_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_MEMORY_WRITE,                 VK_ACCESS_MEMORY_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_CONDITIONAL_RENDERING_READ,   VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT);
ASSERT_SAME(ACCESS_FLAG_SHADING_RATE_TEXTURE_READ,    VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV);
ASSERT_SAME(ACCESS_FLAG_ACCELERATION_STRUCTURE_READ,  VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV);
ASSERT_SAME(ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV);
ASSERT_SAME(ACCESS_FLAG_FRAGMENT_DENSITY_MAP_READ,    VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT);
// clang-format on
VkAccessFlags AccessFlagsToVkAccessFlags(ACCESS_FLAGS AccessFlags)
{
    return static_cast<VkAccessFlags>(AccessFlags);
}
#undef ASSERT_SAME


VkShaderStageFlagBits ShaderTypeToVkShaderStageFlagBit(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == SHADER_TYPE_MESH, "Please update the switch below to handle the new shader type");
    VERIFY(IsPowerOfTwo(Uint32{ShaderType}), "More than one shader type is specified");
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:           return VK_SHADER_STAGE_VERTEX_BIT;
        case SHADER_TYPE_HULL:             return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case SHADER_TYPE_DOMAIN:           return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case SHADER_TYPE_GEOMETRY:         return VK_SHADER_STAGE_GEOMETRY_BIT;
        case SHADER_TYPE_PIXEL:            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SHADER_TYPE_COMPUTE:          return VK_SHADER_STAGE_COMPUTE_BIT;
        case SHADER_TYPE_AMPLIFICATION:    return VK_SHADER_STAGE_TASK_BIT_NV;
        case SHADER_TYPE_MESH:             return VK_SHADER_STAGE_MESH_BIT_NV;
        // clang-format on
        default:
            UNEXPECTED("Unknown shader type");
            return VK_SHADER_STAGE_VERTEX_BIT;
    }
}

} // namespace Diligent
