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
#include "TextureVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "TextureViewVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

TextureVkImpl::TextureVkImpl(IReferenceCounters*        pRefCounters,
                             FixedBlockMemoryAllocator& TexViewObjAllocator,
                             RenderDeviceVkImpl*        pRenderDeviceVk,
                             const TextureDesc&         TexDesc,
                             const TextureData*         pInitData /*= nullptr*/) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pRenderDeviceVk,
        TexDesc
    }
// clang-format on
{
    if (m_Desc.Usage == USAGE_IMMUTABLE && (pInitData == nullptr || pInitData->pSubResources == nullptr))
        LOG_ERROR_AND_THROW("Immutable textures must be initialized with data at creation time: pInitData can't be null");

    const auto& FmtAttribs    = GetTextureFormatAttribs(m_Desc.Format);
    const auto& LogicalDevice = pRenderDeviceVk->GetLogicalDevice();

    if (m_Desc.Usage == USAGE_IMMUTABLE || m_Desc.Usage == USAGE_DEFAULT || m_Desc.Usage == USAGE_DYNAMIC)
    {
        VkImageCreateInfo ImageCI = {};

        ImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageCI.pNext = nullptr;
        ImageCI.flags = 0;
        if (m_Desc.Type == RESOURCE_DIM_TEX_CUBE || m_Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
            ImageCI.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        if (FmtAttribs.IsTypeless)
            ImageCI.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT; // Specifies that the image can be used to create a
                                                                 // VkImageView with a different format from the image.

        if (m_Desc.Type == RESOURCE_DIM_TEX_1D || m_Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY)
            ImageCI.imageType = VK_IMAGE_TYPE_1D;
        else if (m_Desc.Type == RESOURCE_DIM_TEX_2D || m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY || m_Desc.Type == RESOURCE_DIM_TEX_CUBE || m_Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
            ImageCI.imageType = VK_IMAGE_TYPE_2D;
        else if (m_Desc.Type == RESOURCE_DIM_TEX_3D)
        {
            ImageCI.imageType = VK_IMAGE_TYPE_3D;
            ImageCI.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        }
        else
        {
            LOG_ERROR_AND_THROW("Unknown texture type");
        }

        TEXTURE_FORMAT InternalTexFmt = m_Desc.Format;
        if (FmtAttribs.IsTypeless)
        {
            TEXTURE_VIEW_TYPE PrimaryViewType;
            if (m_Desc.BindFlags & BIND_DEPTH_STENCIL)
                PrimaryViewType = TEXTURE_VIEW_DEPTH_STENCIL;
            else if (m_Desc.BindFlags & BIND_UNORDERED_ACCESS)
                PrimaryViewType = TEXTURE_VIEW_UNORDERED_ACCESS;
            else if (m_Desc.BindFlags & BIND_RENDER_TARGET)
                PrimaryViewType = TEXTURE_VIEW_RENDER_TARGET;
            else
                PrimaryViewType = TEXTURE_VIEW_SHADER_RESOURCE;
            InternalTexFmt = GetDefaultTextureViewFormat(m_Desc, PrimaryViewType);
        }

        ImageCI.format = TexFormatToVkFormat(InternalTexFmt);

        ImageCI.extent.width  = m_Desc.Width;
        ImageCI.extent.height = (m_Desc.Type == RESOURCE_DIM_TEX_1D || m_Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY) ? 1 : m_Desc.Height;
        ImageCI.extent.depth  = (m_Desc.Type == RESOURCE_DIM_TEX_3D) ? m_Desc.Depth : 1;

        ImageCI.mipLevels = m_Desc.MipLevels;
        if (m_Desc.Type == RESOURCE_DIM_TEX_1D_ARRAY ||
            m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY ||
            m_Desc.Type == RESOURCE_DIM_TEX_CUBE ||
            m_Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
            ImageCI.arrayLayers = m_Desc.ArraySize;
        else
            ImageCI.arrayLayers = 1;

        ImageCI.samples = static_cast<VkSampleCountFlagBits>(m_Desc.SampleCount);
        ImageCI.tiling  = VK_IMAGE_TILING_OPTIMAL;

        ImageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (m_Desc.BindFlags & BIND_RENDER_TARGET)
        {
            // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required for vkCmdClearColorImage()
            ImageCI.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (m_Desc.BindFlags & BIND_DEPTH_STENCIL)
        {
            // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required for vkCmdClearDepthStencilImage()
            ImageCI.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (m_Desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            ImageCI.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (m_Desc.BindFlags & BIND_SHADER_RESOURCE)
        {
            ImageCI.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (m_Desc.BindFlags & BIND_INPUT_ATTACHMENT)
        {
            ImageCI.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        if (m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS)
        {
            if (CheckCSBasedMipGenerationSupport(ImageCI.format))
            {
                ImageCI.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                m_bCSBasedMipGenerationSupported = true;
            }
            else
            {
                const auto& PhysicalDevice = pRenderDeviceVk->GetPhysicalDevice();
                auto        FmtProperties  = PhysicalDevice.GetPhysicalDeviceFormatProperties(ImageCI.format);
                (void)FmtProperties;
                DEV_CHECK_ERR((FmtProperties.optimalTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT)) == (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT),
                              "Texture format ", GetTextureFormatAttribs(InternalTexFmt).Name,
                              " does not support blitting. Automatic mipmap generation can't be done neither by CS nor by blitting.");

                DEV_CHECK_ERR((FmtProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0,
                              "Texture format ", GetTextureFormatAttribs(InternalTexFmt).Name,
                              " does not support linear filtering. Automatic mipmap generation can't be "
                              "done neither by CS nor by blitting.");
            }
        }

        ImageCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        ImageCI.queueFamilyIndexCount = 0;
        ImageCI.pQueueFamilyIndices   = nullptr;

        // initialLayout must be either VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED (11.4)
        // If it is VK_IMAGE_LAYOUT_PREINITIALIZED, then the image data can be preinitialized by the host
        // while using this layout, and the transition away from this layout will preserve that data.
        // If it is VK_IMAGE_LAYOUT_UNDEFINED, then the contents of the data are considered to be undefined,
        // and the transition away from this layout is not guaranteed to preserve that data.
        ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        bool bInitializeTexture = (pInitData != nullptr && pInitData->pSubResources != nullptr && pInitData->NumSubresources > 0);

        m_VulkanImage = LogicalDevice.CreateImage(ImageCI, m_Desc.Name);

        VkMemoryRequirements MemReqs = LogicalDevice.GetImageMemoryRequirements(m_VulkanImage);

        VkMemoryPropertyFlags ImageMemoryFlags = 0;
        if (m_Desc.Usage == USAGE_STAGING)
            ImageMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        else
            ImageMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VERIFY(IsPowerOfTwo(MemReqs.alignment), "Alignment is not power of 2!");
        m_MemoryAllocation = pRenderDeviceVk->AllocateMemory(MemReqs, ImageMemoryFlags);
        auto AlignedOffset = Align(m_MemoryAllocation.UnalignedOffset, MemReqs.alignment);
        VERIFY_EXPR(m_MemoryAllocation.Size >= MemReqs.size + (AlignedOffset - m_MemoryAllocation.UnalignedOffset));
        auto Memory = m_MemoryAllocation.Page->GetVkMemory();
        auto err    = LogicalDevice.BindImageMemory(m_VulkanImage, Memory, AlignedOffset);
        CHECK_VK_ERROR_AND_THROW(err, "Failed to bind image memory");


        // Vulkan validation layers do not like uninitialized memory, so if no initial data
        // is provided, we will clear the memory

        VulkanUtilities::CommandPoolWrapper CmdPool;
        VkCommandBuffer                     vkCmdBuff;
        pRenderDeviceVk->AllocateTransientCmdPool(CmdPool, vkCmdBuff, "Transient command pool to copy staging data to a device buffer");

        VkImageAspectFlags aspectMask = 0;
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            if (bInitializeTexture)
            {
                UNSUPPORTED("Initializing depth-stencil texture is not currently supported");
                // Only single aspect bit must be specified when copying texture data
            }
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        // For either clear or copy command, dst layout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        VkImageSubresourceRange SubresRange;
        SubresRange.aspectMask           = aspectMask;
        SubresRange.baseArrayLayer       = 0;
        SubresRange.layerCount           = VK_REMAINING_ARRAY_LAYERS;
        SubresRange.baseMipLevel         = 0;
        SubresRange.levelCount           = VK_REMAINING_MIP_LEVELS;
        auto EnabledGraphicsShaderStages = LogicalDevice.GetEnabledGraphicsShaderStages();
        VulkanUtilities::VulkanCommandBuffer::TransitionImageLayout(vkCmdBuff, m_VulkanImage, ImageCI.initialLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, SubresRange, EnabledGraphicsShaderStages);
        SetState(RESOURCE_STATE_COPY_DEST);
        const auto CurrentLayout = GetLayout();
        VERIFY_EXPR(CurrentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        if (bInitializeTexture)
        {
            Uint32 ExpectedNumSubresources = ImageCI.mipLevels * ImageCI.arrayLayers;
            if (pInitData->NumSubresources != ExpectedNumSubresources)
                LOG_ERROR_AND_THROW("Incorrect number of subresources in init data. ", ExpectedNumSubresources, " expected, while ", pInitData->NumSubresources, " provided");

            std::vector<VkBufferImageCopy> Regions(pInitData->NumSubresources);

            Uint64 uploadBufferSize = 0;
            Uint32 subres           = 0;
            for (Uint32 layer = 0; layer < ImageCI.arrayLayers; ++layer)
            {
                for (Uint32 mip = 0; mip < ImageCI.mipLevels; ++mip)
                {
                    const auto& SubResData = pInitData->pSubResources[subres];
                    (void)SubResData;
                    auto& CopyRegion = Regions[subres];

                    auto MipInfo = GetMipLevelProperties(m_Desc, mip);

                    CopyRegion.bufferOffset = uploadBufferSize; // offset in bytes from the start of the buffer object
                    // bufferRowLength and bufferImageHeight specify the data in buffer memory as a subregion
                    // of a larger two- or three-dimensional image, and control the addressing calculations of
                    // data in buffer memory. If either of these values is zero, that aspect of the buffer memory
                    // is considered to be tightly packed according to the imageExtent. (18.4)
                    CopyRegion.bufferRowLength   = 0;
                    CopyRegion.bufferImageHeight = 0;
                    // For block-compression formats, all parameters are still specified in texels rather than compressed texel blocks (18.4.1)
                    CopyRegion.imageOffset = VkOffset3D{0, 0, 0};
                    CopyRegion.imageExtent = VkExtent3D{MipInfo.LogicalWidth, MipInfo.LogicalHeight, MipInfo.Depth};

                    CopyRegion.imageSubresource.aspectMask     = aspectMask;
                    CopyRegion.imageSubresource.mipLevel       = mip;
                    CopyRegion.imageSubresource.baseArrayLayer = layer;
                    CopyRegion.imageSubresource.layerCount     = 1;

                    VERIFY(SubResData.Stride == 0 || SubResData.Stride >= MipInfo.RowSize, "Stride is too small");
                    // For compressed-block formats, MipInfo.RowSize is the size of one row of blocks
                    VERIFY(SubResData.DepthStride == 0 || SubResData.DepthStride >= (MipInfo.StorageHeight / FmtAttribs.BlockHeight) * MipInfo.RowSize, "Depth stride is too small");

                    // bufferOffset must be a multiple of 4 (18.4)
                    // If the calling command's VkImage parameter is a compressed image, bufferOffset
                    // must be a multiple of the compressed texel block size in bytes (18.4). This
                    // is automatically guaranteed as MipWidth and MipHeight are rounded to block size
                    uploadBufferSize += (MipInfo.MipSize + 3) & (~3);
                    ++subres;
                }
            }
            VERIFY_EXPR(subres == pInitData->NumSubresources);

            VkBufferCreateInfo VkStagingBuffCI    = {};
            VkStagingBuffCI.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            VkStagingBuffCI.pNext                 = nullptr;
            VkStagingBuffCI.flags                 = 0;
            VkStagingBuffCI.size                  = uploadBufferSize;
            VkStagingBuffCI.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VkStagingBuffCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
            VkStagingBuffCI.queueFamilyIndexCount = 0;
            VkStagingBuffCI.pQueueFamilyIndices   = nullptr;

            std::string StagingBufferName = "Upload buffer for '";
            StagingBufferName += m_Desc.Name;
            StagingBufferName += '\'';
            VulkanUtilities::BufferWrapper StagingBuffer = LogicalDevice.CreateBuffer(VkStagingBuffCI, StagingBufferName.c_str());

            VkMemoryRequirements StagingBufferMemReqs = LogicalDevice.GetBufferMemoryRequirements(StagingBuffer);
            VERIFY(IsPowerOfTwo(StagingBufferMemReqs.alignment), "Alignment is not power of 2!");
            // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache management commands vkFlushMappedMemoryRanges
            // and vkInvalidateMappedMemoryRanges are NOT needed to flush host writes to the device or make device writes visible
            // to the host (10.2)
            auto StagingMemoryAllocation = pRenderDeviceVk->AllocateMemory(StagingBufferMemReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            auto StagingBufferMemory     = StagingMemoryAllocation.Page->GetVkMemory();
            auto AlignedStagingMemOffset = Align(StagingMemoryAllocation.UnalignedOffset, StagingBufferMemReqs.alignment);
            VERIFY_EXPR(StagingMemoryAllocation.Size >= StagingBufferMemReqs.size + (AlignedStagingMemOffset - StagingMemoryAllocation.UnalignedOffset));

            auto* StagingData = reinterpret_cast<uint8_t*>(StagingMemoryAllocation.Page->GetCPUMemory());
            VERIFY_EXPR(StagingData != nullptr);
            StagingData += AlignedStagingMemOffset;

            subres = 0;
            for (Uint32 layer = 0; layer < ImageCI.arrayLayers; ++layer)
            {
                for (Uint32 mip = 0; mip < ImageCI.mipLevels; ++mip)
                {
                    const auto& SubResData = pInitData->pSubResources[subres];
                    const auto& CopyRegion = Regions[subres];

                    auto MipInfo = GetMipLevelProperties(m_Desc, mip);

                    VERIFY_EXPR(MipInfo.LogicalWidth == CopyRegion.imageExtent.width);
                    VERIFY_EXPR(MipInfo.LogicalHeight == CopyRegion.imageExtent.height);
                    VERIFY_EXPR(MipInfo.Depth == CopyRegion.imageExtent.depth);

                    VERIFY(SubResData.Stride == 0 || SubResData.Stride >= MipInfo.RowSize, "Stride is too small");
                    // For compressed-block formats, MipInfo.RowSize is the size of one row of blocks
                    VERIFY(SubResData.DepthStride == 0 || SubResData.DepthStride >= (MipInfo.StorageHeight / FmtAttribs.BlockHeight) * MipInfo.RowSize, "Depth stride is too small");

                    for (Uint32 z = 0; z < MipInfo.Depth; ++z)
                    {
                        for (Uint32 y = 0; y < MipInfo.StorageHeight; y += FmtAttribs.BlockHeight)
                        {
                            memcpy(StagingData + CopyRegion.bufferOffset + ((y + z * MipInfo.StorageHeight) / FmtAttribs.BlockHeight) * MipInfo.RowSize,
                                   // SubResData.Stride must be the stride of one row of compressed blocks
                                   reinterpret_cast<const uint8_t*>(SubResData.pData) + (y / FmtAttribs.BlockHeight) * SubResData.Stride + z * SubResData.DepthStride,
                                   MipInfo.RowSize);
                        }
                    }

                    ++subres;
                }
            }
            VERIFY_EXPR(subres == pInitData->NumSubresources);

            err = LogicalDevice.BindBufferMemory(StagingBuffer, StagingBufferMemory, AlignedStagingMemOffset);
            CHECK_VK_ERROR_AND_THROW(err, "Failed to bind staging bufer memory");

            VulkanUtilities::VulkanCommandBuffer::BufferMemoryBarrier(vkCmdBuff, StagingBuffer, 0, VK_ACCESS_TRANSFER_READ_BIT, EnabledGraphicsShaderStages);

            // Copy commands MUST be recorded outside of a render pass instance. This is OK here
            // as copy will be the only command in the cmd buffer
            vkCmdCopyBufferToImage(vkCmdBuff, StagingBuffer, m_VulkanImage,
                                   CurrentLayout, // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.4)
                                   static_cast<uint32_t>(Regions.size()), Regions.data());

            Uint32 QueueIndex = 0;
            pRenderDeviceVk->ExecuteAndDisposeTransientCmdBuff(QueueIndex, vkCmdBuff, std::move(CmdPool));

            // After command buffer is submitted, safe-release resources. This strategy
            // is little overconservative as the resources will be released after the first
            // command buffer submitted through the immediate context will be completed
            pRenderDeviceVk->SafeReleaseDeviceObject(std::move(StagingBuffer), Uint64{1} << Uint64{QueueIndex});
            pRenderDeviceVk->SafeReleaseDeviceObject(std::move(StagingMemoryAllocation), Uint64{1} << Uint64{QueueIndex});
        }
        else
        {
            VkImageSubresourceRange Subresource;
            Subresource.aspectMask     = aspectMask;
            Subresource.baseMipLevel   = 0;
            Subresource.levelCount     = VK_REMAINING_MIP_LEVELS;
            Subresource.baseArrayLayer = 0;
            Subresource.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
            {
                if (FmtAttribs.ComponentType != COMPONENT_TYPE_COMPRESSED)
                {
                    VkClearColorValue ClearColor = {};
                    vkCmdClearColorImage(vkCmdBuff, m_VulkanImage,
                                         CurrentLayout, // must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                         &ClearColor, 1, &Subresource);
                }
            }
            else if (aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT ||
                     aspectMask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
            {
                VkClearDepthStencilValue ClearValue = {};
                vkCmdClearDepthStencilImage(vkCmdBuff, m_VulkanImage,
                                            CurrentLayout, // must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                            &ClearValue, 1, &Subresource);
            }
            else
            {
                UNEXPECTED("Unexpected aspect mask");
            }
            Uint32 QueueIndex = 0;
            pRenderDeviceVk->ExecuteAndDisposeTransientCmdBuff(QueueIndex, vkCmdBuff, std::move(CmdPool));
        }
    }
    else if (m_Desc.Usage == USAGE_STAGING)
    {
        VkBufferCreateInfo VkStagingBuffCI = {};

        VkStagingBuffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        VkStagingBuffCI.pNext = nullptr;
        VkStagingBuffCI.flags = 0;
        VkStagingBuffCI.size  = GetStagingTextureSubresourceOffset(m_Desc, m_Desc.ArraySize, 0, StagingBufferOffsetAlignment);

        // clang-format off
        DEV_CHECK_ERR((m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_READ ||
                      (m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_WRITE,
                      "Exactly one of CPU_ACCESS_READ or CPU_ACCESS_WRITE flags must be specified");
        // clang-format on
        VkMemoryPropertyFlags MemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        if (m_Desc.CPUAccessFlags & CPU_ACCESS_READ)
        {
            VkStagingBuffCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            MemProperties |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            SetState(RESOURCE_STATE_COPY_DEST);

            // We do not set HOST_COHERENT bit, so we will have to use InvalidateMappedMemoryRanges,
            // which requires the ranges to be aligned by nonCoherentAtomSize.
            const auto& DeviceLimits = pRenderDeviceVk->GetPhysicalDevice().GetProperties().limits;
            // Align the buffer size to ensure that any aligned range is always in bounds.
            VkStagingBuffCI.size = Align(VkStagingBuffCI.size, DeviceLimits.nonCoherentAtomSize);
        }
        else if (m_Desc.CPUAccessFlags & CPU_ACCESS_WRITE)
        {
            VkStagingBuffCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache management commands vkFlushMappedMemoryRanges
            // and vkInvalidateMappedMemoryRanges are NOT needed to flush host writes to the device or make device writes visible
            // to the host (10.2)
            MemProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            SetState(RESOURCE_STATE_COPY_SOURCE);
        }
        else
            UNEXPECTED("Unexpected CPU access");

        VkStagingBuffCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        VkStagingBuffCI.queueFamilyIndexCount = 0;
        VkStagingBuffCI.pQueueFamilyIndices   = nullptr;

        std::string StagingBufferName = "Staging buffer for '";
        StagingBufferName += m_Desc.Name;
        StagingBufferName += '\'';
        m_StagingBuffer = LogicalDevice.CreateBuffer(VkStagingBuffCI, StagingBufferName.c_str());

        VkMemoryRequirements StagingBufferMemReqs = LogicalDevice.GetBufferMemoryRequirements(m_StagingBuffer);
        VERIFY(IsPowerOfTwo(StagingBufferMemReqs.alignment), "Alignment is not power of 2!");

        m_MemoryAllocation           = pRenderDeviceVk->AllocateMemory(StagingBufferMemReqs, MemProperties);
        auto StagingBufferMemory     = m_MemoryAllocation.Page->GetVkMemory();
        auto AlignedStagingMemOffset = Align(m_MemoryAllocation.UnalignedOffset, StagingBufferMemReqs.alignment);
        VERIFY_EXPR(m_MemoryAllocation.Size >= StagingBufferMemReqs.size + (AlignedStagingMemOffset - m_MemoryAllocation.UnalignedOffset));

        auto err = LogicalDevice.BindBufferMemory(m_StagingBuffer, StagingBufferMemory, AlignedStagingMemOffset);
        CHECK_VK_ERROR_AND_THROW(err, "Failed to bind staging bufer memory");

        m_StagingDataAlignedOffset = AlignedStagingMemOffset;
    }
    else
    {
        UNSUPPORTED("Unsupported usage ", GetUsageString(m_Desc.Usage));
    }

    VERIFY_EXPR(IsInKnownState());
}

TextureVkImpl::TextureVkImpl(IReferenceCounters*        pRefCounters,
                             FixedBlockMemoryAllocator& TexViewObjAllocator,
                             RenderDeviceVkImpl*        pDeviceVk,
                             const TextureDesc&         TexDesc,
                             RESOURCE_STATE             InitialState,
                             VkImage                    VkImageHandle) :
    TTextureBase{pRefCounters, TexViewObjAllocator, pDeviceVk, TexDesc},
    m_VulkanImage{VkImageHandle}
{
    SetState(InitialState);
}

IMPLEMENT_QUERY_INTERFACE(TextureVkImpl, IID_TextureVk, TTextureBase)

void TextureVkImpl::CreateViewInternal(const TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "View pointer address is null");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto& TexViewAllocator = m_pDevice->GetTexViewObjAllocator();
        VERIFY(&TexViewAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");

        auto UpdatedViewDesc = ViewDesc;
        CorrectTextureViewDesc(UpdatedViewDesc);

        VulkanUtilities::ImageViewWrapper ImgView = CreateImageView(UpdatedViewDesc);
        auto                              pViewVk = NEW_RC_OBJ(TexViewAllocator, "TextureViewVkImpl instance", TextureViewVkImpl, bIsDefaultView ? this : nullptr)(GetDevice(), UpdatedViewDesc, this, std::move(ImgView), bIsDefaultView);
        VERIFY(pViewVk->GetDesc().ViewType == ViewDesc.ViewType, "Incorrect view type");

        if (bIsDefaultView)
            *ppView = pViewVk;
        else
            pViewVk->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppView));


        if ((UpdatedViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0 &&
            m_bCSBasedMipGenerationSupported &&
            CheckCSBasedMipGenerationSupport(TexFormatToVkFormat(pViewVk->GetDesc().Format)))
        {
            auto* pMipLevelViews = ALLOCATE(GetRawAllocator(), "Raw memory for mip level views", TextureViewVkImpl::MipLevelViewAutoPtrType, UpdatedViewDesc.NumMipLevels * 2);
            for (Uint32 MipLevel = 0; MipLevel < UpdatedViewDesc.NumMipLevels; ++MipLevel)
            {
                auto CreateMipLevelView = [&](TEXTURE_VIEW_TYPE ViewType, Uint32 MipLevel, TextureViewVkImpl::MipLevelViewAutoPtrType* ppMipLevelView) {
                    TextureViewDesc MipLevelViewDesc = pViewVk->GetDesc();
                    // Always create texture array views
                    std::stringstream name_ss;
                    name_ss << "Internal " << (ViewType == TEXTURE_VIEW_SHADER_RESOURCE ? "SRV" : "UAV")
                            << " of mip level " << MipLevel << " of texture view '" << pViewVk->GetDesc().Name << "'";
                    auto name                   = name_ss.str();
                    MipLevelViewDesc.Name       = name.c_str();
                    MipLevelViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                    MipLevelViewDesc.ViewType   = ViewType;
                    MipLevelViewDesc.MostDetailedMip += MipLevel;
                    MipLevelViewDesc.NumMipLevels = 1;

                    if (ViewType == TEXTURE_VIEW_UNORDERED_ACCESS)
                    {
                        if (MipLevelViewDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB)
                            MipLevelViewDesc.Format = TEX_FORMAT_RGBA8_UNORM;
                    }

                    VulkanUtilities::ImageViewWrapper ImgView = CreateImageView(MipLevelViewDesc);
                    // Attach to parent view
                    auto pMipLevelViewVk = NEW_RC_OBJ(TexViewAllocator, "TextureViewVkImpl instance", TextureViewVkImpl, pViewVk)(GetDevice(), MipLevelViewDesc, this, std::move(ImgView), bIsDefaultView);
                    new (ppMipLevelView) TextureViewVkImpl::MipLevelViewAutoPtrType(pMipLevelViewVk, STDDeleter<TextureViewVkImpl, FixedBlockMemoryAllocator>(TexViewAllocator));
                };

                CreateMipLevelView(TEXTURE_VIEW_SHADER_RESOURCE, MipLevel, &pMipLevelViews[MipLevel * 2]);
                CreateMipLevelView(TEXTURE_VIEW_UNORDERED_ACCESS, MipLevel, &pMipLevelViews[MipLevel * 2 + 1]);
            }

            if (auto pImmediateCtx = m_pDevice->GetImmediateContext())
            {
                auto& GenerateMipsHelper = pImmediateCtx.RawPtr<DeviceContextVkImpl>()->GetGenerateMipsHelper();
                GenerateMipsHelper.WarmUpCache(pViewVk->GetDesc().Format);
            }

            pViewVk->AssignMipLevelViews(pMipLevelViews);
        }
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetTexViewTypeLiteralName(ViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", ViewDesc.Name ? ViewDesc.Name : "", "\" (", ViewTypeName, ") for texture \"", m_Desc.Name ? m_Desc.Name : "", "\"");
    }
}

TextureVkImpl::~TextureVkImpl()
{
    // Vk object can only be destroyed when it is no longer used by the GPU
    // Wrappers for external texture will not be destroyed as they are created with null device pointer
    if (m_VulkanImage)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VulkanImage), m_Desc.CommandQueueMask);
    if (m_StagingBuffer)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_StagingBuffer), m_Desc.CommandQueueMask);
    m_pDevice->SafeReleaseDeviceObject(std::move(m_MemoryAllocation), m_Desc.CommandQueueMask);
}

VulkanUtilities::ImageViewWrapper TextureVkImpl::CreateImageView(TextureViewDesc& ViewDesc)
{
    // clang-format off
    VERIFY(ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE ||
           ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET ||
           ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL ||
           ViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS,
           "Unexpected view type");
    // clang-format on
    if (ViewDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        ViewDesc.Format = m_Desc.Format;
    }

    VkImageViewCreateInfo ImageViewCI = {};

    ImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCI.pNext = nullptr;
    ImageViewCI.flags = 0; // reserved for future use.
    ImageViewCI.image = m_VulkanImage;

    switch (ViewDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_1D;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;

        case RESOURCE_DIM_TEX_2D:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;

        case RESOURCE_DIM_TEX_3D:
            if (ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET || ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL)
            {
                ViewDesc.TextureDim  = RESOURCE_DIM_TEX_2D_ARRAY;
                ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            }
            else
            {
                ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_3D;
                Uint32 MipDepth      = std::max(Uint32{m_Desc.Depth} >> Uint32{ViewDesc.MostDetailedMip}, 1U);
                if (ViewDesc.FirstDepthSlice != 0 || ViewDesc.NumDepthSlices != MipDepth)
                {
                    LOG_ERROR("3D texture view '", (ViewDesc.Name ? ViewDesc.Name : ""), "' (most detailed mip: ", Uint32{ViewDesc.MostDetailedMip},
                              "; mip levels: ", Uint32{ViewDesc.NumMipLevels}, "; first slice: ", ViewDesc.FirstDepthSlice,
                              "; num depth slices: ", ViewDesc.NumDepthSlices, ") of texture '", m_Desc.Name,
                              "' does not references all depth slices (", MipDepth,
                              ") in the mip level. 3D texture views in Vulkan must address all depth slices.");
                    ViewDesc.FirstDepthSlice = 0;
                    ViewDesc.NumDepthSlices  = MipDepth;
                }
            }
            break;

        case RESOURCE_DIM_TEX_CUBE:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            break;

        default: UNEXPECTED("Unexpcted view dimension");
    }

    TEXTURE_FORMAT CorrectedViewFormat = ViewDesc.Format;
    if (m_Desc.BindFlags & BIND_DEPTH_STENCIL)
        CorrectedViewFormat = GetDefaultTextureViewFormat(CorrectedViewFormat, TEXTURE_VIEW_DEPTH_STENCIL, m_Desc.BindFlags);
    ImageViewCI.format                        = TexFormatToVkFormat(CorrectedViewFormat);
    ImageViewCI.components                    = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
    ImageViewCI.subresourceRange.baseMipLevel = ViewDesc.MostDetailedMip;
    ImageViewCI.subresourceRange.levelCount   = ViewDesc.NumMipLevels;
    if (ViewDesc.TextureDim == RESOURCE_DIM_TEX_1D_ARRAY ||
        ViewDesc.TextureDim == RESOURCE_DIM_TEX_2D_ARRAY ||
        ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE ||
        ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE_ARRAY)
    {
        ImageViewCI.subresourceRange.baseArrayLayer = ViewDesc.FirstArraySlice;
        ImageViewCI.subresourceRange.layerCount     = ViewDesc.NumArraySlices;
    }
    else
    {
        ImageViewCI.subresourceRange.baseArrayLayer = 0;
        ImageViewCI.subresourceRange.layerCount     = 1;
    }

    const auto& FmtAttribs = GetTextureFormatAttribs(CorrectedViewFormat);

    if (ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL)
    {
        // When an imageView of a depth/stencil image is used as a depth/stencil framebuffer attachment,
        // the aspectMask is ignored and both depth and stencil image subresources are used. (11.5)
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        else
            UNEXPECTED("Unexpected component type for a depth-stencil view format");
    }
    else
    {
        // aspectMask must be only VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT
        // if format is a color, depth-only or stencil-only format, respectively.  (11.5)
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
        {
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            if (ViewDesc.Format == TEX_FORMAT_D32_FLOAT_S8X24_UINT ||
                ViewDesc.Format == TEX_FORMAT_D24_UNORM_S8_UINT)
            {
                ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            else if (ViewDesc.Format == TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS ||
                     ViewDesc.Format == TEX_FORMAT_R24_UNORM_X8_TYPELESS)
            {
                ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            else if (ViewDesc.Format == TEX_FORMAT_X32_TYPELESS_G8X24_UINT ||
                     ViewDesc.Format == TEX_FORMAT_X24_TYPELESS_G8_UINT)
            {
                ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            else
                UNEXPECTED("Unexpected depth-stencil texture format");
        }
        else
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    const auto& LogicalDevice = m_pDevice->GetLogicalDevice();

    std::string ViewName = "Image view for \'";
    ViewName += m_Desc.Name;
    ViewName += '\'';
    return LogicalDevice.CreateImageView(ImageViewCI, ViewName.c_str());
}

bool TextureVkImpl::CheckCSBasedMipGenerationSupport(VkFormat vkFmt) const
{
#if !DILIGENT_NO_GLSLANG
    VERIFY_EXPR(m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS);
    if (m_Desc.Type == RESOURCE_DIM_TEX_2D || m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY)
    {
        const auto& PhysicalDevice = m_pDevice->GetPhysicalDevice();
        auto        FmtProperties  = PhysicalDevice.GetPhysicalDeviceFormatProperties(vkFmt);
        if ((FmtProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0)
        {
            return true;
        }
    }
#endif

    return false;
}

void TextureVkImpl::SetLayout(VkImageLayout Layout)
{
    SetState(VkImageLayoutToResourceState(Layout));
}

VkImageLayout TextureVkImpl::GetLayout() const
{
    return ResourceStateToVkImageLayout(GetState());
}

void TextureVkImpl::InvalidateStagingRange(VkDeviceSize Offset, VkDeviceSize Size)
{
    const auto& LogicalDevice    = m_pDevice->GetLogicalDevice();
    const auto& PhysDeviceLimits = m_pDevice->GetPhysicalDevice().GetProperties().limits;

    VkMappedMemoryRange InvalidateRange = {};

    InvalidateRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    InvalidateRange.pNext  = nullptr;
    InvalidateRange.memory = m_MemoryAllocation.Page->GetVkMemory();

    Offset += m_StagingDataAlignedOffset;
    auto AlignedOffset = AlignDown(Offset, PhysDeviceLimits.nonCoherentAtomSize);
    Size += Offset - AlignedOffset;
    auto AlignedSize = Align(Size, PhysDeviceLimits.nonCoherentAtomSize);

    InvalidateRange.offset = AlignedOffset;
    InvalidateRange.size   = AlignedSize;

    auto err = LogicalDevice.InvalidateMappedMemoryRanges(1, &InvalidateRange);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to invalidated mapped texture memory range");
    (void)err;
}

} // namespace Diligent
