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
#include "Texture.h"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

void ValidateTextureDesc(const TextureDesc& Desc)
{
#define LOG_TEXTURE_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Texture '", (Desc.Name ? Desc.Name : ""), "': ", ##__VA_ARGS__)

    const auto& FmtAttribs = GetTextureFormatAttribs(Desc.Format);

    if (Desc.Type == RESOURCE_DIM_UNDEFINED)
    {
        LOG_TEXTURE_ERROR_AND_THROW("Resource dimension is undefined");
    }

    if (!(Desc.Type >= RESOURCE_DIM_TEX_1D && Desc.Type <= RESOURCE_DIM_TEX_CUBE_ARRAY))
    {
        LOG_TEXTURE_ERROR_AND_THROW("Unexpected resource dimension");
    }

    if (Desc.Width == 0)
    {
        LOG_TEXTURE_ERROR_AND_THROW("Texture width cannot be zero");
    }

    // Perform some parameter correctness check
    if (Desc.Type == RESOURCE_DIM_TEX_1D || Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
    {
        if (Desc.Height != FmtAttribs.BlockHeight)
        {
            if (FmtAttribs.BlockHeight == 1)
            {
                LOG_TEXTURE_ERROR_AND_THROW("Height (", Desc.Height, ") of a Texture 1D/Texture 1D Array must be 1");
            }
            else
            {
                LOG_TEXTURE_ERROR_AND_THROW("For block-compressed formats, the height (", Desc.Height,
                                            ") of a Texture 1D/Texture 1D Array must be equal to the compressed block height (",
                                            Uint32{FmtAttribs.BlockHeight}, ")");
            }
        }
    }
    else
    {
        if (Desc.Height == 0)
            LOG_TEXTURE_ERROR_AND_THROW("Texture height cannot be zero");
    }

    if (Desc.Type == RESOURCE_DIM_TEX_3D && Desc.Depth == 0)
    {
        LOG_TEXTURE_ERROR_AND_THROW("3D texture depth cannot be zero");
    }

    if (Desc.Type == RESOURCE_DIM_TEX_1D || Desc.Type == RESOURCE_DIM_TEX_2D)
    {
        if (Desc.ArraySize != 1)
            LOG_TEXTURE_ERROR_AND_THROW("Texture 1D/2D must have one array slice (", Desc.ArraySize, " provided). Use Texture 1D/2D array if you need more than one slice.");
    }

    if (Desc.Type == RESOURCE_DIM_TEX_CUBE || Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        if (Desc.Width != Desc.Height)
            LOG_TEXTURE_ERROR_AND_THROW("For cube map textures, texture width (", Desc.Width, " provided) must match texture height (", Desc.Height, " provided)");

        if (Desc.ArraySize < 6)
            LOG_TEXTURE_ERROR_AND_THROW("Texture cube/cube array must have at least 6 slices (", Desc.ArraySize, " provided).");
    }

    Uint32 MaxDim = 0;
    if (Desc.Type == RESOURCE_DIM_TEX_1D || Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
        MaxDim = Desc.Width;
    else if (Desc.Type == RESOURCE_DIM_TEX_2D || Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY || Desc.Type == RESOURCE_DIM_TEX_CUBE || Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        MaxDim = std::max(Desc.Width, Desc.Height);
    else if (Desc.Type == RESOURCE_DIM_TEX_3D)
        MaxDim = std::max(std::max(Desc.Width, Desc.Height), Desc.Depth);
    VERIFY(MaxDim >= (1U << (Desc.MipLevels - 1)), "Texture '", Desc.Name ? Desc.Name : "", "': Incorrect number of Mip levels (", Desc.MipLevels, ")");

    if (Desc.SampleCount > 1)
    {
        if (!(Desc.Type == RESOURCE_DIM_TEX_2D || Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY))
            LOG_TEXTURE_ERROR_AND_THROW("Only Texture 2D/Texture 2D Array can be multisampled");

        if (Desc.MipLevels != 1)
            LOG_TEXTURE_ERROR_AND_THROW("Multisampled textures must have one mip level (", Desc.MipLevels, " levels specified)");

        if (Desc.BindFlags & BIND_UNORDERED_ACCESS)
            LOG_TEXTURE_ERROR_AND_THROW("UAVs are not allowed for multisampled resources");
    }

    if ((Desc.BindFlags & BIND_RENDER_TARGET) &&
        (Desc.Format == TEX_FORMAT_R8_SNORM || Desc.Format == TEX_FORMAT_RG8_SNORM || Desc.Format == TEX_FORMAT_RGBA8_SNORM ||
         Desc.Format == TEX_FORMAT_R16_SNORM || Desc.Format == TEX_FORMAT_RG16_SNORM || Desc.Format == TEX_FORMAT_RGBA16_SNORM))
    {
        const auto* FmtName = GetTextureFormatAttribs(Desc.Format).Name;
        LOG_WARNING_MESSAGE(FmtName, " texture is created with BIND_RENDER_TARGET flag set.\n"
                                     "There might be an issue in OpenGL driver on NVidia hardware: when rendering to SNORM textures, all negative values are clamped to zero.\n"
                                     "Use UNORM format instead.");
    }

    if (Desc.Usage == USAGE_STAGING)
    {
        if (Desc.BindFlags != 0)
            LOG_TEXTURE_ERROR_AND_THROW("Staging textures cannot be bound to any GPU pipeline stage");

        if (Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS)
            LOG_TEXTURE_ERROR_AND_THROW("Mipmaps cannot be autogenerated for staging textures");

        if (Desc.CPUAccessFlags == 0)
            LOG_TEXTURE_ERROR_AND_THROW("Staging textures must specify CPU access flags");

        if ((Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == (CPU_ACCESS_READ | CPU_ACCESS_WRITE))
            LOG_TEXTURE_ERROR_AND_THROW("Staging textures must use exactly one of ACESS_READ or ACCESS_WRITE flags");
    }
    else if (Desc.Usage == USAGE_UNIFIED)
    {
        LOG_TEXTURE_ERROR_AND_THROW("USAGE_UNIFIED textures are currently not supported");
    }
}


void ValidateTextureRegion(const TextureDesc& TexDesc, Uint32 MipLevel, Uint32 Slice, const Box& Box)
{
#define VERIFY_TEX_PARAMS(Expr, ...)                                                          \
    do                                                                                        \
    {                                                                                         \
        if (!(Expr))                                                                          \
        {                                                                                     \
            LOG_ERROR("Texture '", (TexDesc.Name ? TexDesc.Name : ""), "': ", ##__VA_ARGS__); \
        }                                                                                     \
    } while (false)

#ifdef DILIGENT_DEVELOPMENT
    VERIFY_TEX_PARAMS(MipLevel < TexDesc.MipLevels, "Mip level (", MipLevel, ") is out of allowed range [0, ", TexDesc.MipLevels - 1, "]");
    VERIFY_TEX_PARAMS(Box.MinX < Box.MaxX, "Invalid X range: ", Box.MinX, "..", Box.MaxX);
    VERIFY_TEX_PARAMS(Box.MinY < Box.MaxY, "Invalid Y range: ", Box.MinY, "..", Box.MaxY);
    VERIFY_TEX_PARAMS(Box.MinZ < Box.MaxZ, "Invalid Z range: ", Box.MinZ, "..", Box.MaxZ);

    if (TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY ||
        TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY ||
        TexDesc.Type == RESOURCE_DIM_TEX_CUBE ||
        TexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        VERIFY_TEX_PARAMS(Slice < TexDesc.ArraySize, "Array slice (", Slice, ") is out of range [0,", TexDesc.ArraySize - 1, "]");
    }
    else
    {
        VERIFY_TEX_PARAMS(Slice == 0, "Array slice (", Slice, ") must be 0 for non-array textures");
    }

    const auto& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);

    Uint32 MipWidth = std::max(TexDesc.Width >> MipLevel, 1U);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        VERIFY_EXPR((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0);
        Uint32 BlockAlignedMipWidth = (MipWidth + (FmtAttribs.BlockWidth - 1)) & ~(FmtAttribs.BlockWidth - 1);
        VERIFY_TEX_PARAMS(Box.MaxX <= BlockAlignedMipWidth, "Region max X coordinate (", Box.MaxX, ") is out of allowed range [0, ", BlockAlignedMipWidth, "]");
        VERIFY_TEX_PARAMS((Box.MinX % FmtAttribs.BlockWidth) == 0, "For compressed formats, the region min X coordinate (", Box.MinX, ") must be a multiple of block width (", Uint32{FmtAttribs.BlockWidth}, ")");
        VERIFY_TEX_PARAMS((Box.MaxX % FmtAttribs.BlockWidth) == 0 || Box.MaxX == MipWidth, "For compressed formats, the region max X coordinate (", Box.MaxX, ") must be a multiple of block width (", Uint32{FmtAttribs.BlockWidth}, ") or equal the mip level width (", MipWidth, ")");
    }
    else
        VERIFY_TEX_PARAMS(Box.MaxX <= MipWidth, "Region max X coordinate (", Box.MaxX, ") is out of allowed range [0, ", MipWidth, "]");

    if (TexDesc.Type != RESOURCE_DIM_TEX_1D &&
        TexDesc.Type != RESOURCE_DIM_TEX_1D_ARRAY)
    {
        Uint32 MipHeight = std::max(TexDesc.Height >> MipLevel, 1U);
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            VERIFY_EXPR((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0);
            Uint32 BlockAlignedMipHeight = (MipHeight + (FmtAttribs.BlockHeight - 1)) & ~(FmtAttribs.BlockHeight - 1);
            VERIFY_TEX_PARAMS(Box.MaxY <= BlockAlignedMipHeight, "Region max Y coordinate (", Box.MaxY, ") is out of allowed range [0, ", BlockAlignedMipHeight, "]");
            VERIFY_TEX_PARAMS((Box.MinY % FmtAttribs.BlockHeight) == 0, "For compressed formats, the region min Y coordinate (", Box.MinY, ") must be a multiple of block height (", Uint32{FmtAttribs.BlockHeight}, ")");
            VERIFY_TEX_PARAMS((Box.MaxY % FmtAttribs.BlockHeight) == 0 || Box.MaxY == MipHeight, "For compressed formats, the region max Y coordinate (", Box.MaxY, ") must be a multiple of block height (", Uint32{FmtAttribs.BlockHeight}, ") or equal the mip level height (", MipHeight, ")");
        }
        else
            VERIFY_TEX_PARAMS(Box.MaxY <= MipHeight, "Region max Y coordinate (", Box.MaxY, ") is out of allowed range [0, ", MipHeight, "]");
    }

    if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
    {
        Uint32 MipDepth = std::max(TexDesc.Depth >> MipLevel, 1U);
        VERIFY_TEX_PARAMS(Box.MaxZ <= MipDepth, "Region max Z coordinate (", Box.MaxZ, ") is out of allowed range  [0, ", MipDepth, "]");
    }
    else
    {
        VERIFY_TEX_PARAMS(Box.MinZ == 0, "Region min Z (", Box.MinZ, ") must be 0 for all but 3D textures");
        VERIFY_TEX_PARAMS(Box.MaxZ == 1, "Region max Z (", Box.MaxZ, ") must be 1 for all but 3D textures");
    }
#endif
}

void ValidateUpdateTextureParams(const TextureDesc& TexDesc, Uint32 MipLevel, Uint32 Slice, const Box& DstBox, const TextureSubResData& SubresData)
{
    VERIFY((SubresData.pData != nullptr) ^ (SubresData.pSrcBuffer != nullptr), "Either CPU data pointer (pData) or GPU buffer (pSrcBuffer) must not be null, but not both");
    ValidateTextureRegion(TexDesc, MipLevel, Slice, DstBox);

#ifdef DILIGENT_DEVELOPMENT
    VERIFY_TEX_PARAMS(TexDesc.SampleCount == 1, "Only non-multisampled textures can be updated with UpdateData()");
    VERIFY_TEX_PARAMS((SubresData.Stride & 0x03) == 0, "Texture data stride (", SubresData.Stride, ") must be at least 32-bit aligned");
    VERIFY_TEX_PARAMS((SubresData.DepthStride & 0x03) == 0, "Texture data depth stride (", SubresData.DepthStride, ") must be at least 32-bit aligned");

    auto        UpdateRegionWidth  = DstBox.MaxX - DstBox.MinX;
    auto        UpdateRegionHeight = DstBox.MaxY - DstBox.MinY;
    auto        UpdateRegionDepth  = DstBox.MaxZ - DstBox.MinZ;
    const auto& FmtAttribs         = GetTextureFormatAttribs(TexDesc.Format);
    Uint32      RowSize            = 0;
    Uint32      RowCount           = 0;
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Align update region size by the block size. This is only necessary when updating
        // coarse mip levels. Otherwise UpdateRegionWidth/Height should be multiples of block size
        VERIFY_EXPR((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0);
        VERIFY_EXPR((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0);
        UpdateRegionWidth  = (UpdateRegionWidth + (FmtAttribs.BlockWidth - 1)) & ~(FmtAttribs.BlockWidth - 1);
        UpdateRegionHeight = (UpdateRegionHeight + (FmtAttribs.BlockHeight - 1)) & ~(FmtAttribs.BlockHeight - 1);
        RowSize            = UpdateRegionWidth / Uint32{FmtAttribs.BlockWidth} * Uint32{FmtAttribs.ComponentSize};
        RowCount           = UpdateRegionHeight / FmtAttribs.BlockHeight;
    }
    else
    {
        RowSize  = UpdateRegionWidth * Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        RowCount = UpdateRegionHeight;
    }
    DEV_CHECK_ERR(SubresData.Stride >= RowSize, "Source data stride (", SubresData.Stride, ") is below the image row size (", RowSize, ")");
    const Uint32 PlaneSize = SubresData.Stride * RowCount;
    DEV_CHECK_ERR(UpdateRegionDepth == 1 || SubresData.DepthStride >= PlaneSize, "Source data depth stride (", SubresData.DepthStride, ") is below the image plane size (", PlaneSize, ")");
#endif
}

void ValidateCopyTextureParams(const CopyTextureAttribs& CopyAttribs)
{
    VERIFY_EXPR(CopyAttribs.pSrcTexture != nullptr && CopyAttribs.pDstTexture != nullptr);
    Box         SrcBox;
    const auto& SrcTexDesc = CopyAttribs.pSrcTexture->GetDesc();
    const auto& DstTexDesc = CopyAttribs.pDstTexture->GetDesc();
    auto        pSrcBox    = CopyAttribs.pSrcBox;
    if (pSrcBox == nullptr)
    {
        auto MipLevelAttribs = GetMipLevelProperties(SrcTexDesc, CopyAttribs.SrcMipLevel);
        SrcBox.MaxX          = MipLevelAttribs.LogicalWidth;
        SrcBox.MaxY          = MipLevelAttribs.LogicalHeight;
        SrcBox.MaxZ          = MipLevelAttribs.Depth;
        pSrcBox              = &SrcBox;
    }
    ValidateTextureRegion(SrcTexDesc, CopyAttribs.SrcMipLevel, CopyAttribs.SrcSlice, *pSrcBox);

    Box DstBox;
    DstBox.MinX = CopyAttribs.DstX;
    DstBox.MinY = CopyAttribs.DstY;
    DstBox.MinZ = CopyAttribs.DstZ;
    DstBox.MaxX = DstBox.MinX + (pSrcBox->MaxX - pSrcBox->MinX);
    DstBox.MaxY = DstBox.MinY + (pSrcBox->MaxY - pSrcBox->MinY);
    DstBox.MaxZ = DstBox.MinZ + (pSrcBox->MaxZ - pSrcBox->MinZ);
    ValidateTextureRegion(DstTexDesc, CopyAttribs.DstMipLevel, CopyAttribs.DstSlice, DstBox);
}

void ValidateMapTextureParams(const TextureDesc& TexDesc,
                              Uint32             MipLevel,
                              Uint32             ArraySlice,
                              MAP_TYPE           MapType,
                              Uint32             MapFlags,
                              const Box*         pMapRegion)
{
    VERIFY_TEX_PARAMS(MipLevel < TexDesc.MipLevels, "Mip level (", MipLevel, ") is out of allowed range [0, ", TexDesc.MipLevels - 1, "]");
    if (TexDesc.Type == RESOURCE_DIM_TEX_1D_ARRAY ||
        TexDesc.Type == RESOURCE_DIM_TEX_2D_ARRAY ||
        TexDesc.Type == RESOURCE_DIM_TEX_CUBE ||
        TexDesc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        VERIFY_TEX_PARAMS(ArraySlice < TexDesc.ArraySize, "Array slice (", ArraySlice, ") is out of range [0,", TexDesc.ArraySize - 1, "]");
    }
    else
    {
        VERIFY_TEX_PARAMS(ArraySlice == 0, "Array slice (", ArraySlice, ") must be 0 for non-array textures");
    }

    if (pMapRegion != nullptr)
    {
        ValidateTextureRegion(TexDesc, MipLevel, ArraySlice, *pMapRegion);
    }
}

} // namespace Diligent
