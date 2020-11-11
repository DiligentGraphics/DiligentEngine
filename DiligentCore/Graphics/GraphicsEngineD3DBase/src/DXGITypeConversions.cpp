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

#include "DXGITypeConversions.hpp"
#include "BasicTypes.h"
#include "DebugUtilities.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

DXGI_FORMAT TypeToDXGI_Format(VALUE_TYPE ValType, Uint32 NumComponents, Bool bIsNormalized)
{
    switch (ValType)
    {
        case VT_FLOAT16:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return DXGI_FORMAT_R16_FLOAT;
                case 2: return DXGI_FORMAT_R16G16_FLOAT;
                case 4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
            }
        }

        case VT_FLOAT32:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return DXGI_FORMAT_R32_FLOAT;
                case 2: return DXGI_FORMAT_R32G32_FLOAT;
                case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
                case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
                default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
            }
        }

        case VT_INT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return DXGI_FORMAT_R32_SINT;
                case 2: return DXGI_FORMAT_R32G32_SINT;
                case 3: return DXGI_FORMAT_R32G32B32_SINT;
                case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
                default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
            }
        }

        case VT_UINT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return DXGI_FORMAT_R32_UINT;
                case 2: return DXGI_FORMAT_R32G32_UINT;
                case 3: return DXGI_FORMAT_R32G32B32_UINT;
                case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
                default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
            }
        }

        case VT_INT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R16_SNORM;
                    case 2: return DXGI_FORMAT_R16G16_SNORM;
                    case 4: return DXGI_FORMAT_R16G16B16A16_SNORM;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R16_SINT;
                    case 2: return DXGI_FORMAT_R16G16_SINT;
                    case 4: return DXGI_FORMAT_R16G16B16A16_SINT;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
        }

        case VT_UINT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R16_UNORM;
                    case 2: return DXGI_FORMAT_R16G16_UNORM;
                    case 4: return DXGI_FORMAT_R16G16B16A16_UNORM;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R16_UINT;
                    case 2: return DXGI_FORMAT_R16G16_UINT;
                    case 4: return DXGI_FORMAT_R16G16B16A16_UINT;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
        }

        case VT_INT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R8_SNORM;
                    case 2: return DXGI_FORMAT_R8G8_SNORM;
                    case 4: return DXGI_FORMAT_R8G8B8A8_SNORM;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R8_SINT;
                    case 2: return DXGI_FORMAT_R8G8_SINT;
                    case 4: return DXGI_FORMAT_R8G8B8A8_SINT;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
        }

        case VT_UINT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R8_UNORM;
                    case 2: return DXGI_FORMAT_R8G8_UNORM;
                    case 4: return DXGI_FORMAT_R8G8B8A8_UNORM;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return DXGI_FORMAT_R8_UINT;
                    case 2: return DXGI_FORMAT_R8G8_UINT;
                    case 4: return DXGI_FORMAT_R8G8B8A8_UINT;
                    default: UNEXPECTED("Unusupported number of components"); return DXGI_FORMAT_UNKNOWN;
                }
            }
        }

        default: UNEXPECTED("Unusupported format"); return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT CorrectDXGIFormat(DXGI_FORMAT DXGIFormat, Uint32 BindFlags)
{
    if ((BindFlags & BIND_DEPTH_STENCIL) && (BindFlags != BIND_DEPTH_STENCIL))
    {
        switch (DXGIFormat)
        {
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_D32_FLOAT:
                DXGIFormat = DXGI_FORMAT_R32_TYPELESS;
                break;

            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                DXGIFormat = DXGI_FORMAT_R24G8_TYPELESS;
                break;

            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_D16_UNORM:
                DXGIFormat = DXGI_FORMAT_R16_TYPELESS;
                break;

            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                DXGIFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
                break;

            default:
                UNEXPECTED("Unsupported depth-stencil format");
                break;
        }
    }

    if (BindFlags == BIND_DEPTH_STENCIL)
    {
        switch (DXGIFormat)
        {
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_R32_FLOAT:
                DXGIFormat = DXGI_FORMAT_D32_FLOAT;
                break;

            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                DXGIFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                break;

            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_R16_UNORM:
                DXGIFormat = DXGI_FORMAT_D16_UNORM;
                break;
        }
    }

    if (BindFlags == BIND_SHADER_RESOURCE || BindFlags == BIND_UNORDERED_ACCESS)
    {
        switch (DXGIFormat)
        {
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
                DXGIFormat = DXGI_FORMAT_R32_FLOAT;
                break;

            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                DXGIFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;

            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
                DXGIFormat = DXGI_FORMAT_R16_UNORM;
                break;
        }
    }

    return DXGIFormat;
}

DXGI_FORMAT TexFormatToDXGI_Format(TEXTURE_FORMAT TexFormat, Uint32 BindFlags)
{
    static Bool        bFormatMapIntialized                    = false;
    static DXGI_FORMAT FmtToDXGIFmtMap[TEX_FORMAT_NUM_FORMATS] = {DXGI_FORMAT_UNKNOWN};
    if (!bFormatMapIntialized)
    {
        // clang-format off
        FmtToDXGIFmtMap[TEX_FORMAT_UNKNOWN]                = DXGI_FORMAT_UNKNOWN;

        FmtToDXGIFmtMap[TEX_FORMAT_RGBA32_TYPELESS]        = DXGI_FORMAT_R32G32B32A32_TYPELESS; 
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA32_FLOAT]           = DXGI_FORMAT_R32G32B32A32_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA32_UINT]            = DXGI_FORMAT_R32G32B32A32_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA32_SINT]            = DXGI_FORMAT_R32G32B32A32_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_RGB32_TYPELESS]         = DXGI_FORMAT_R32G32B32_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_RGB32_FLOAT]            = DXGI_FORMAT_R32G32B32_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_RGB32_UINT]             = DXGI_FORMAT_R32G32B32_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_RGB32_SINT]             = DXGI_FORMAT_R32G32B32_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_RGBA16_TYPELESS]        = DXGI_FORMAT_R16G16B16A16_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA16_FLOAT]           = DXGI_FORMAT_R16G16B16A16_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA16_UNORM]           = DXGI_FORMAT_R16G16B16A16_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA16_UINT]            = DXGI_FORMAT_R16G16B16A16_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA16_SNORM]           = DXGI_FORMAT_R16G16B16A16_SNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA16_SINT]            = DXGI_FORMAT_R16G16B16A16_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_RG32_TYPELESS]          = DXGI_FORMAT_R32G32_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_RG32_FLOAT]             = DXGI_FORMAT_R32G32_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_RG32_UINT]              = DXGI_FORMAT_R32G32_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_RG32_SINT]              = DXGI_FORMAT_R32G32_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_R32G8X24_TYPELESS]      = DXGI_FORMAT_R32G8X24_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_D32_FLOAT_S8X24_UINT]   = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS]= DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_X32_TYPELESS_G8X24_UINT]= DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

        FmtToDXGIFmtMap[TEX_FORMAT_RGB10A2_TYPELESS]       = DXGI_FORMAT_R10G10B10A2_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_RGB10A2_UNORM]          = DXGI_FORMAT_R10G10B10A2_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RGB10A2_UINT]           = DXGI_FORMAT_R10G10B10A2_UINT;

        FmtToDXGIFmtMap[TEX_FORMAT_R11G11B10_FLOAT]        = DXGI_FORMAT_R11G11B10_FLOAT;

        FmtToDXGIFmtMap[TEX_FORMAT_RGBA8_TYPELESS]         = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA8_UNORM]            = DXGI_FORMAT_R8G8B8A8_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA8_UNORM_SRGB]       = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA8_UINT]             = DXGI_FORMAT_R8G8B8A8_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA8_SNORM]            = DXGI_FORMAT_R8G8B8A8_SNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RGBA8_SINT]             = DXGI_FORMAT_R8G8B8A8_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_RG16_TYPELESS]          = DXGI_FORMAT_R16G16_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_RG16_FLOAT]             = DXGI_FORMAT_R16G16_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_RG16_UNORM]             = DXGI_FORMAT_R16G16_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RG16_UINT]              = DXGI_FORMAT_R16G16_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_RG16_SNORM]             = DXGI_FORMAT_R16G16_SNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RG16_SINT]              = DXGI_FORMAT_R16G16_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_R32_TYPELESS]           = DXGI_FORMAT_R32_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_D32_FLOAT]              = DXGI_FORMAT_D32_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_R32_FLOAT]              = DXGI_FORMAT_R32_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_R32_UINT]               = DXGI_FORMAT_R32_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_R32_SINT]               = DXGI_FORMAT_R32_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_R24G8_TYPELESS]         = DXGI_FORMAT_R24G8_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_D24_UNORM_S8_UINT]      = DXGI_FORMAT_D24_UNORM_S8_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_R24_UNORM_X8_TYPELESS]  = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_X24_TYPELESS_G8_UINT]   = DXGI_FORMAT_X24_TYPELESS_G8_UINT;

        FmtToDXGIFmtMap[TEX_FORMAT_RG8_TYPELESS]           = DXGI_FORMAT_R8G8_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_RG8_UNORM]              = DXGI_FORMAT_R8G8_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RG8_UINT]               = DXGI_FORMAT_R8G8_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_RG8_SNORM]              = DXGI_FORMAT_R8G8_SNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_RG8_SINT]               = DXGI_FORMAT_R8G8_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_R16_TYPELESS]           = DXGI_FORMAT_R16_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_R16_FLOAT]              = DXGI_FORMAT_R16_FLOAT;
        FmtToDXGIFmtMap[TEX_FORMAT_D16_UNORM]              = DXGI_FORMAT_D16_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_R16_UNORM]              = DXGI_FORMAT_R16_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_R16_UINT]               = DXGI_FORMAT_R16_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_R16_SNORM]              = DXGI_FORMAT_R16_SNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_R16_SINT]               = DXGI_FORMAT_R16_SINT;

        FmtToDXGIFmtMap[TEX_FORMAT_R8_TYPELESS]            = DXGI_FORMAT_R8_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_R8_UNORM]               = DXGI_FORMAT_R8_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_R8_UINT]                = DXGI_FORMAT_R8_UINT;
        FmtToDXGIFmtMap[TEX_FORMAT_R8_SNORM]               = DXGI_FORMAT_R8_SNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_R8_SINT]                = DXGI_FORMAT_R8_SINT;
        FmtToDXGIFmtMap[TEX_FORMAT_A8_UNORM]               = DXGI_FORMAT_A8_UNORM;

        FmtToDXGIFmtMap[TEX_FORMAT_R1_UNORM]               = DXGI_FORMAT_R1_UNORM ;
        FmtToDXGIFmtMap[TEX_FORMAT_RGB9E5_SHAREDEXP]       = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        FmtToDXGIFmtMap[TEX_FORMAT_RG8_B8G8_UNORM]         = DXGI_FORMAT_R8G8_B8G8_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_G8R8_G8B8_UNORM]        = DXGI_FORMAT_G8R8_G8B8_UNORM;

        FmtToDXGIFmtMap[TEX_FORMAT_BC1_TYPELESS]           = DXGI_FORMAT_BC1_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BC1_UNORM]              = DXGI_FORMAT_BC1_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BC1_UNORM_SRGB]         = DXGI_FORMAT_BC1_UNORM_SRGB;
        FmtToDXGIFmtMap[TEX_FORMAT_BC2_TYPELESS]           = DXGI_FORMAT_BC2_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BC2_UNORM]              = DXGI_FORMAT_BC2_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BC2_UNORM_SRGB]         = DXGI_FORMAT_BC2_UNORM_SRGB;
        FmtToDXGIFmtMap[TEX_FORMAT_BC3_TYPELESS]           = DXGI_FORMAT_BC3_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BC3_UNORM]              = DXGI_FORMAT_BC3_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BC3_UNORM_SRGB]         = DXGI_FORMAT_BC3_UNORM_SRGB;
        FmtToDXGIFmtMap[TEX_FORMAT_BC4_TYPELESS]           = DXGI_FORMAT_BC4_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BC4_UNORM]              = DXGI_FORMAT_BC4_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BC4_SNORM]              = DXGI_FORMAT_BC4_SNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BC5_TYPELESS]           = DXGI_FORMAT_BC5_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BC5_UNORM]              = DXGI_FORMAT_BC5_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BC5_SNORM]              = DXGI_FORMAT_BC5_SNORM;

        FmtToDXGIFmtMap[TEX_FORMAT_B5G6R5_UNORM]           = DXGI_FORMAT_B5G6R5_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_B5G5R5A1_UNORM]         = DXGI_FORMAT_B5G5R5A1_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BGRA8_UNORM]            = DXGI_FORMAT_B8G8R8A8_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BGRX8_UNORM]            = DXGI_FORMAT_B8G8R8X8_UNORM;

        FmtToDXGIFmtMap[TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM]= DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

        FmtToDXGIFmtMap[TEX_FORMAT_BGRA8_TYPELESS]         = DXGI_FORMAT_B8G8R8A8_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BGRA8_UNORM_SRGB]       = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        FmtToDXGIFmtMap[TEX_FORMAT_BGRX8_TYPELESS]         = DXGI_FORMAT_B8G8R8X8_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BGRX8_UNORM_SRGB]       = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

        FmtToDXGIFmtMap[TEX_FORMAT_BC6H_TYPELESS]          = DXGI_FORMAT_BC6H_TYPELESS;
        FmtToDXGIFmtMap[TEX_FORMAT_BC6H_UF16]              = DXGI_FORMAT_BC6H_UF16;
        FmtToDXGIFmtMap[TEX_FORMAT_BC6H_SF16]              = DXGI_FORMAT_BC6H_SF16;
        FmtToDXGIFmtMap[TEX_FORMAT_BC7_TYPELESS]           = DXGI_FORMAT_BC7_TYPELESS ;
        FmtToDXGIFmtMap[TEX_FORMAT_BC7_UNORM]              = DXGI_FORMAT_BC7_UNORM;
        FmtToDXGIFmtMap[TEX_FORMAT_BC7_UNORM_SRGB]         = DXGI_FORMAT_BC7_UNORM_SRGB;
        // clang-format on

        bFormatMapIntialized = true;
    }

    if (TexFormat >= TEX_FORMAT_UNKNOWN && TexFormat < TEX_FORMAT_NUM_FORMATS)
    {
        auto DXGIFormat = FmtToDXGIFmtMap[TexFormat];
        VERIFY(TexFormat == TEX_FORMAT_UNKNOWN || DXGIFormat != DXGI_FORMAT_UNKNOWN, "Unsupported texture format");
        if (BindFlags != 0)
            DXGIFormat = CorrectDXGIFormat(DXGIFormat, BindFlags);
        return DXGIFormat;
    }
    else
    {
        UNEXPECTED("Texture format (", TexFormat, ") is out of allowed range [0, ", TEX_FORMAT_NUM_FORMATS - 1, "]");
        return DXGI_FORMAT_UNKNOWN;
    }
}

class DXGIFmtToFmtMapInitializer
{
public:
    DXGIFmtToFmtMapInitializer(TEXTURE_FORMAT DXGIFmtToFmtMap[])
    {
        for (TEXTURE_FORMAT fmt = TEX_FORMAT_UNKNOWN; fmt < TEX_FORMAT_NUM_FORMATS; fmt = static_cast<TEXTURE_FORMAT>(fmt + 1))
        {
            auto DXGIFmt = TexFormatToDXGI_Format(fmt);
            VERIFY_EXPR(DXGIFmt <= DXGI_FORMAT_B4G4R4A4_UNORM);
            DXGIFmtToFmtMap[DXGIFmt] = fmt;
        }
    }
};

TEXTURE_FORMAT DXGI_FormatToTexFormat(DXGI_FORMAT DXGIFormat)
{
    static_assert(DXGI_FORMAT_B4G4R4A4_UNORM == 115, "Unexpected DXGI format value");
    static TEXTURE_FORMAT             DXGIFmtToFmtMap[DXGI_FORMAT_B4G4R4A4_UNORM + 1];
    static DXGIFmtToFmtMapInitializer Initializer(DXGIFmtToFmtMap);

    if (DXGIFormat >= DXGI_FORMAT_UNKNOWN && DXGIFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)
    {
        auto Format = DXGIFmtToFmtMap[DXGIFormat];
        VERIFY(DXGIFormat == DXGI_FORMAT_UNKNOWN || Format != TEX_FORMAT_UNKNOWN, "Unsupported texture format");
        VERIFY_EXPR(DXGIFormat == TexFormatToDXGI_Format(Format));
        return Format;
    }
    else
    {
        UNEXPECTED("DXGI texture format (", DXGIFormat, ") is out of allowed range [0, ", DXGI_FORMAT_BC7_UNORM_SRGB, "]");
        return TEX_FORMAT_UNKNOWN;
    }
}

GraphicsAdapterInfo DXGI_ADAPTER_DESC_To_GraphicsAdapterInfo(const DXGI_ADAPTER_DESC1& dxgiAdapterDesc)
{
    GraphicsAdapterInfo AdapterInfo;

    WideCharToMultiByte(CP_ACP, 0, dxgiAdapterDesc.Description, -1, AdapterInfo.Description, _countof(AdapterInfo.Description), NULL, FALSE);
    AdapterInfo.Type               = (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? ADAPTER_TYPE_SOFTWARE : ADAPTER_TYPE_HARDWARE;
    AdapterInfo.Vendor             = VendorIdToAdapterVendor(dxgiAdapterDesc.VendorId);
    AdapterInfo.VendorId           = dxgiAdapterDesc.VendorId;
    AdapterInfo.DeviceId           = dxgiAdapterDesc.DeviceId;
    AdapterInfo.NumOutputs         = 0;
    AdapterInfo.DeviceLocalMemory  = dxgiAdapterDesc.DedicatedVideoMemory;
    AdapterInfo.HostVisibileMemory = dxgiAdapterDesc.SharedSystemMemory;
    AdapterInfo.UnifiedMemory      = 0;

    return AdapterInfo;
}

} // namespace Diligent
