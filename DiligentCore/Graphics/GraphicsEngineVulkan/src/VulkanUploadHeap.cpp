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
#include "VulkanUploadHeap.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

VulkanUploadHeap::VulkanUploadHeap(RenderDeviceVkImpl& RenderDevice,
                                   std::string         HeapName,
                                   VkDeviceSize        PageSize) :
    // clang-format off
    m_RenderDevice {RenderDevice       },
    m_HeapName     {std::move(HeapName)},
    m_PageSize     {PageSize           }
// clang-format on
{
}

VulkanUploadHeap::~VulkanUploadHeap()
{
    DEV_CHECK_ERR(m_Pages.empty(), "Upload heap '", m_HeapName, "' not all pages are released");
    auto PeakAllocatedPages = m_PeakAllocatedSize / m_PageSize;
    LOG_INFO_MESSAGE(m_HeapName, " peak used/allocated frame size: ", FormatMemorySize(m_PeakFrameSize, 2, m_PeakAllocatedSize),
                     " / ", FormatMemorySize(m_PeakAllocatedSize, 2),
                     " (", PeakAllocatedPages, (PeakAllocatedPages == 1 ? " page)" : " pages)"));
}

VulkanUploadHeap::UploadPageInfo VulkanUploadHeap::CreateNewPage(VkDeviceSize SizeInBytes) const
{
    VkBufferCreateInfo StagingBufferCI = {};

    StagingBufferCI.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    StagingBufferCI.pNext                 = nullptr;
    StagingBufferCI.flags                 = 0; // VK_BUFFER_CREATE_SPARSE_BINDING_BIT, VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT, VK_BUFFER_CREATE_SPARSE_ALIASED_BIT
    StagingBufferCI.size                  = SizeInBytes;
    StagingBufferCI.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    StagingBufferCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    StagingBufferCI.queueFamilyIndexCount = 0;
    StagingBufferCI.pQueueFamilyIndices   = nullptr;

    const auto& LogicalDevice   = m_RenderDevice.GetLogicalDevice();
    const auto& PhysicalDevice  = m_RenderDevice.GetPhysicalDevice();
    auto&       GlobalMemoryMgr = m_RenderDevice.GetGlobalMemoryManager();

    auto NewBuffer       = LogicalDevice.CreateBuffer(StagingBufferCI, "Upload buffer");
    auto MemReqs         = LogicalDevice.GetBufferMemoryRequirements(NewBuffer);
    auto MemoryTypeIndex = PhysicalDevice.GetMemoryTypeIndex(MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    DEV_CHECK_ERR(MemoryTypeIndex != VulkanUtilities::VulkanPhysicalDevice::InvalidMemoryTypeIndex,
                  "Vulkan spec requires that for a VkBuffer not created with the VK_BUFFER_CREATE_SPARSE_BINDING_BIT "
                  "bit set, or for a VkImage that was created with a VK_IMAGE_TILING_LINEAR value in the tiling member "
                  "of the VkImageCreateInfo structure passed to vkCreateImage, the memoryTypeBits member always contains "
                  "at least one bit set corresponding to a VkMemoryType with a propertyFlags that has both the "
                  "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit AND the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit set. (11.6)");

    auto MemAllocation = GlobalMemoryMgr.Allocate(MemReqs.size, MemReqs.alignment, MemoryTypeIndex, true);

    auto AlignedOffset = (MemAllocation.UnalignedOffset + (MemReqs.alignment - 1)) & ~(MemReqs.alignment - 1);
    auto err           = LogicalDevice.BindBufferMemory(NewBuffer, MemAllocation.Page->GetVkMemory(), AlignedOffset);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to bind buffer memory");
    (void)err;
    auto CPUAddress = reinterpret_cast<Uint8*>(MemAllocation.Page->GetCPUMemory()) + AlignedOffset;

    return UploadPageInfo{std::move(MemAllocation), std::move(NewBuffer), CPUAddress};
}

VulkanUploadAllocation VulkanUploadHeap::Allocate(VkDeviceSize SizeInBytes, VkDeviceSize Alignment)
{
    VERIFY(IsPowerOfTwo(Alignment), "Alignment (", Alignment, ") must be power of two");

    VulkanUploadAllocation Allocation;
    if (SizeInBytes >= m_PageSize / 2)
    {
        // Allocate large chunk directly from the memory manager
        auto NewPage          = CreateNewPage(SizeInBytes);
        Allocation.vkBuffer   = NewPage.Buffer;
        Allocation.CPUAddress = NewPage.CPUAddress;
        Allocation.Size       = SizeInBytes;
        VERIFY(Alignment < SizeInBytes, "Alignment must be smaller than the page size");
        Allocation.AlignedOffset = 0;
        m_CurrAllocatedSize += NewPage.MemAllocation.Size;
        m_Pages.emplace_back(std::move(NewPage));
    }
    else
    {
        auto AlignmentOffset = Align(m_CurrPage.CurrOffset, Alignment) - m_CurrPage.CurrOffset;
        if (m_CurrPage.AvailableSize < SizeInBytes + AlignmentOffset)
        {
            // Allocate new page
            auto NewPage = CreateNewPage(m_PageSize);
            m_CurrPage.Reset(NewPage, m_PageSize);
            m_CurrAllocatedSize += NewPage.MemAllocation.Size;
            m_Pages.emplace_back(std::move(NewPage));
            VERIFY_EXPR((m_CurrPage.CurrOffset & (Alignment - 1)) == 0);
            AlignmentOffset = 0;
        }

        m_CurrPage.Advance(AlignmentOffset);
        VERIFY_EXPR((m_CurrPage.CurrOffset & (Alignment - 1)) == 0);
        Allocation.vkBuffer      = m_CurrPage.vkBuffer;
        Allocation.CPUAddress    = m_CurrPage.CurrCPUAddress;
        Allocation.Size          = SizeInBytes;
        Allocation.AlignedOffset = m_CurrPage.CurrOffset;
        m_CurrPage.Advance(SizeInBytes);
    }
    m_CurrFrameSize += SizeInBytes; // Count unaligned size
    m_PeakFrameSize     = std::max(m_CurrFrameSize, m_PeakFrameSize);
    m_PeakAllocatedSize = std::max(m_CurrAllocatedSize, m_PeakAllocatedSize);

    VERIFY_EXPR((Allocation.AlignedOffset & (Alignment - 1)) == 0);
    return Allocation;
}

void VulkanUploadHeap::ReleaseAllocatedPages(Uint64 CmdQueueMask)
{
    // The pages will go into the stale resources queue first, however they will move into the release
    // queue rightaway when RenderDeviceVkImpl::FlushStaleResources() is called by the DeviceContextVkImpl::FinishFrame()
    for (auto& Page : m_Pages)
    {
        m_RenderDevice.SafeReleaseDeviceObject(std::move(Page.MemAllocation), CmdQueueMask);
        m_RenderDevice.SafeReleaseDeviceObject(std::move(Page.Buffer), CmdQueueMask);
    }

    m_Pages.clear();

    m_CurrPage          = CurrPageInfo{};
    m_CurrFrameSize     = 0;
    m_CurrAllocatedSize = 0;
}

} // namespace Diligent
