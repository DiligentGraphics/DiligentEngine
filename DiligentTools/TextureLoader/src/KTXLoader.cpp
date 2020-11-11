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

#include <algorithm>
#include <vector>

#include "TextureLoader.h"
#include "GraphicsAccessories.hpp"
#include "Align.hpp"

#define GL_RGBA32F            0x8814
#define GL_RGBA32UI           0x8D70
#define GL_RGBA32I            0x8D82
#define GL_RGB32F             0x8815
#define GL_RGB32UI            0x8D71
#define GL_RGB32I             0x8D83
#define GL_RGBA16F            0x881A
#define GL_RGBA16             0x805B
#define GL_RGBA16UI           0x8D76
#define GL_RGBA16_SNORM       0x8F9B
#define GL_RGBA16I            0x8D88
#define GL_RG32F              0x8230
#define GL_RG32UI             0x823C
#define GL_RG32I              0x823B
#define GL_DEPTH32F_STENCIL8  0x8CAD
#define GL_RGB10_A2           0x8059
#define GL_RGB10_A2UI         0x906F
#define GL_R11F_G11F_B10F     0x8C3A
#define GL_RGBA8              0x8058
#define GL_RGBA8UI            0x8D7C
#define GL_RGBA8_SNORM        0x8F97
#define GL_RGBA8I             0x8D8E
#define GL_RG16F              0x822F
#define GL_RG16               0x822C
#define GL_RG16UI             0x823A
#define GL_RG16_SNORM         0x8F99
#define GL_RG16I              0x8239
#define GL_R32F               0x822E
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_R32UI              0x8236
#define GL_R32I               0x8235
#define GL_DEPTH24_STENCIL8   0x88F0
#define GL_RG8                0x822B
#define GL_RG8UI              0x8238
#define GL_RG8_SNORM          0x8F95
#define GL_RG8I               0x8237
#define GL_R16F               0x822D
#define GL_DEPTH_COMPONENT16  0x81A5
#define GL_R16                0x822A
#define GL_R16UI              0x8234
#define GL_R16_SNORM          0x8F98
#define GL_R16I               0x8233
#define GL_R8                 0x8229
#define GL_R8UI               0x8232
#define GL_R8_SNORM           0x8F94
#define GL_R8I                0x8231
#define GL_RGB9_E5            0x8C3D


#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT        0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT       0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT       0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT       0x83F3
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT       0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#define GL_COMPRESSED_RED_RGTC1                0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1         0x8DBC
#define GL_COMPRESSED_RG_RGTC2                 0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2          0x8DBE
#define GL_COMPRESSED_RGBA_BPTC_UNORM          0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM    0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT    0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT  0x8E8F

namespace Diligent
{

namespace
{

TEXTURE_FORMAT FindDiligentTextureFormat(std::uint32_t GLInternalFormat)
{
    switch (GLInternalFormat)
    {
        // clang-format off
        case GL_RGBA32F:        return TEX_FORMAT_RGBA32_FLOAT;
        case GL_RGBA32UI:       return TEX_FORMAT_RGBA32_UINT;
        case GL_RGBA32I:        return TEX_FORMAT_RGBA32_SINT;

        case GL_RGB32F:         return TEX_FORMAT_RGB32_FLOAT;
        case GL_RGB32UI:        return TEX_FORMAT_RGB32_UINT;
        case GL_RGB32I:         return TEX_FORMAT_RGB32_SINT;
        
        case GL_RGBA16F:        return TEX_FORMAT_RGBA16_FLOAT;
        case GL_RGBA16:         return TEX_FORMAT_RGBA16_UNORM;
        case GL_RGBA16UI:       return TEX_FORMAT_RGBA16_UINT;
        case GL_RGBA16_SNORM:   return TEX_FORMAT_RGBA16_SNORM;
        case GL_RGBA16I:        return TEX_FORMAT_RGBA16_SINT;
        
        case GL_RG32F:          return TEX_FORMAT_RG32_FLOAT;
        case GL_RG32UI:         return TEX_FORMAT_RG32_UINT;
        case GL_RG32I:          return TEX_FORMAT_RG32_SINT;

        case GL_DEPTH32F_STENCIL8: return TEX_FORMAT_D32_FLOAT_S8X24_UINT;

        case GL_RGB10_A2:       return TEX_FORMAT_RGB10A2_UNORM;
        case GL_RGB10_A2UI:     return TEX_FORMAT_RGB10A2_UINT;
        case GL_R11F_G11F_B10F: return TEX_FORMAT_R11G11B10_FLOAT;
    
        case GL_RGBA8:          return TEX_FORMAT_RGBA8_UNORM;
        case GL_RGBA8UI:        return TEX_FORMAT_RGBA8_UINT;
        case GL_RGBA8_SNORM:    return TEX_FORMAT_RGBA8_SNORM;
        case GL_RGBA8I:         return TEX_FORMAT_RGBA8_SINT;
        
        case GL_RG16F:          return TEX_FORMAT_RG16_FLOAT;
        case GL_RG16:           return TEX_FORMAT_RG16_UNORM;
        case GL_RG16UI:         return TEX_FORMAT_RG16_UINT;
        case GL_RG16_SNORM:     return TEX_FORMAT_RG16_SNORM;
        case GL_RG16I:          return TEX_FORMAT_RG16_SINT;
        
        case GL_R32F:               return TEX_FORMAT_R32_FLOAT;
        case GL_DEPTH_COMPONENT32F: return TEX_FORMAT_D32_FLOAT;
        case GL_R32UI:              return TEX_FORMAT_R32_UINT;
        case GL_R32I:               return TEX_FORMAT_R32_SINT;
        
        case GL_DEPTH24_STENCIL8: return TEX_FORMAT_D24_UNORM_S8_UINT;

        case GL_RG8:        return TEX_FORMAT_RG8_UNORM;
        case GL_RG8UI:      return TEX_FORMAT_RG8_UINT;
        case GL_RG8_SNORM:  return TEX_FORMAT_RG8_SNORM;
        case GL_RG8I:       return TEX_FORMAT_RG8_SINT;

        case GL_R16F:               return TEX_FORMAT_R16_FLOAT;
        case GL_DEPTH_COMPONENT16:  return TEX_FORMAT_D16_UNORM;
        case GL_R16:                return TEX_FORMAT_R16_UNORM;
        case GL_R16UI:              return TEX_FORMAT_R16_UINT;
        case GL_R16_SNORM:          return TEX_FORMAT_R16_SNORM;
        case GL_R16I:               return TEX_FORMAT_R16_SINT;
        
        case GL_R8:                 return TEX_FORMAT_R8_UNORM;
        case GL_R8UI:               return TEX_FORMAT_R8_UINT;
        case GL_R8_SNORM:           return TEX_FORMAT_R8_SNORM;
        case GL_R8I:                return TEX_FORMAT_R8_SINT;

        case GL_RGB9_E5:            return TEX_FORMAT_RGB9E5_SHAREDEXP;

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:        return TEX_FORMAT_BC1_UNORM;
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:       return TEX_FORMAT_BC1_UNORM_SRGB;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:       return TEX_FORMAT_BC2_UNORM;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return TEX_FORMAT_BC2_UNORM_SRGB;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:       return TEX_FORMAT_BC3_UNORM;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return TEX_FORMAT_BC3_UNORM_SRGB;
        case GL_COMPRESSED_RED_RGTC1:                return TEX_FORMAT_BC4_UNORM;
        case GL_COMPRESSED_SIGNED_RED_RGTC1:         return TEX_FORMAT_BC4_SNORM;
        case GL_COMPRESSED_RG_RGTC2:                 return TEX_FORMAT_BC5_UNORM;
        case GL_COMPRESSED_SIGNED_RG_RGTC2:          return TEX_FORMAT_BC5_SNORM;

        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: return TEX_FORMAT_BC6H_UF16;
        case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:   return TEX_FORMAT_BC6H_SF16;
        case GL_COMPRESSED_RGBA_BPTC_UNORM:         return TEX_FORMAT_BC7_UNORM;
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:   return TEX_FORMAT_BC7_UNORM_SRGB;
        // clang-format on
        default:
            UNSUPPORTED("Unsupported internal format");
            return TEX_FORMAT_UNKNOWN;
    }
}

} // namespace


struct KTX10Header
{
    std::uint32_t Endianness;
    std::uint32_t GLType;
    std::uint32_t GLTypeSize;
    std::uint32_t GLFormat;
    std::uint32_t GLInternalFormat;
    std::uint32_t GLBaseInternalFormat;
    std::uint32_t Width;
    std::uint32_t Height;
    std::uint32_t Depth;
    std::uint32_t NumberOfArrayElements;
    std::uint32_t NumberOfFaces;
    std::uint32_t NumberOfMipmapLevels;
    std::uint32_t BytesOfKeyValueData;
};

void CreateTextureFromKTX(IDataBlob*             pKTXData,
                          const TextureLoadInfo& TexLoadInfo,
                          IRenderDevice*         pDevice,
                          ITexture**             ppTexture)
{
    static constexpr Uint8 KTX10FileIdentifier[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    const Uint8*           pData                   = reinterpret_cast<const Uint8*>(pKTXData->GetDataPtr());
    const auto             DataSize                = pKTXData->GetSize();
    if (DataSize >= 12 && memcmp(pData, KTX10FileIdentifier, sizeof(KTX10FileIdentifier)) == 0)
    {
        pData += sizeof(KTX10FileIdentifier);
        const KTX10Header& Header = *reinterpret_cast<KTX10Header const*>(pData);
        pData += sizeof(KTX10Header);
        // Skip key value data
        pData += Header.BytesOfKeyValueData;

        TextureDesc TexDesc;
        TexDesc.Name   = TexLoadInfo.Name;
        TexDesc.Format = FindDiligentTextureFormat(Header.GLInternalFormat);
        if (TexDesc.Format == TEX_FORMAT_UNKNOWN)
            LOG_ERROR_AND_THROW("Failed to find appropriate Diligent format for internal gl format ", Header.GLInternalFormat);
        TexDesc.Width     = Header.Width;
        TexDesc.Height    = std::max(Header.Height, 1u);
        TexDesc.Depth     = std::max(Header.Depth, 1u);
        TexDesc.MipLevels = std::max(Header.NumberOfMipmapLevels, 1u);
        TexDesc.BindFlags = TexLoadInfo.BindFlags;
        TexDesc.Usage     = TexLoadInfo.Usage;
        auto NumFaces     = std::max(Header.NumberOfFaces, 1u);
        if (NumFaces == 6)
        {
            TexDesc.ArraySize = std::max(Header.NumberOfArrayElements, 1u) * NumFaces;
            TexDesc.Type      = TexDesc.ArraySize > 6 ? RESOURCE_DIM_TEX_CUBE_ARRAY : RESOURCE_DIM_TEX_CUBE;
        }
        else
        {
            if (TexDesc.Depth > 1)
            {
                TexDesc.ArraySize = 1;
                TexDesc.Type      = RESOURCE_DIM_TEX_3D;
            }
            else
            {
                TexDesc.ArraySize = std::max(Header.NumberOfArrayElements, 1u);
                TexDesc.Type      = TexDesc.ArraySize > 1 ? RESOURCE_DIM_TEX_2D_ARRAY : RESOURCE_DIM_TEX_2D;
            }
        }

        auto                           ArraySize = (TexDesc.Type != RESOURCE_DIM_TEX_3D ? TexDesc.ArraySize : 1);
        std::vector<TextureSubResData> SubresData(TexDesc.MipLevels * ArraySize);
        for (Uint32 mip = 0; mip < TexDesc.MipLevels; ++mip)
        {
            pData += sizeof(std::uint32_t);
            auto MipInfo = GetMipLevelProperties(TexDesc, mip);

            for (Uint32 layer = 0; layer < ArraySize; ++layer)
            {
                SubresData[mip + layer * TexDesc.MipLevels] =
                    TextureSubResData{pData, MipInfo.RowSize, MipInfo.DepthSliceSize};
                pData += Align(MipInfo.MipSize, 4u);
            }
        }
        VERIFY(pData - reinterpret_cast<const Uint8*>(pKTXData->GetDataPtr()) == static_cast<ptrdiff_t>(DataSize), "Unexpected data size");

        TextureData InitData(SubresData.data(), static_cast<Uint32>(SubresData.size()));
        pDevice->CreateTexture(TexDesc, &InitData, ppTexture);
    }
    else
    {
        LOG_ERROR_AND_THROW("ktx2.0 is not currently supported");
    }
}

} // namespace Diligent

extern "C"
{
    void Diligent_CreateTextureFromKTX(Diligent::IDataBlob*             pKTXData,
                                       const Diligent::TextureLoadInfo& TexLoadInfo,
                                       Diligent::IRenderDevice*         pDevice,
                                       Diligent::ITexture**             ppTexture)
    {
        Diligent::CreateTextureFromKTX(pKTXData, TexLoadInfo, pDevice, ppTexture);
    }
}