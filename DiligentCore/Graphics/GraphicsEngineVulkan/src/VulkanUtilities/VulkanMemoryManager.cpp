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
#include <sstream>
#include "VulkanUtilities/VulkanMemoryManager.hpp"

namespace VulkanUtilities
{

VulkanMemoryAllocation::~VulkanMemoryAllocation()
{
    if (Page != nullptr)
    {
        Page->Free(std::move(*this));
    }
}

VulkanMemoryPage::VulkanMemoryPage(VulkanMemoryManager& ParentMemoryMgr,
                                   VkDeviceSize         PageSize,
                                   uint32_t             MemoryTypeIndex,
                                   bool                 IsHostVisible) noexcept :
    // clang-format off
    m_ParentMemoryMgr{ParentMemoryMgr},
    m_AllocationMgr  {static_cast<AllocationsMgrOffsetType>(PageSize), ParentMemoryMgr.m_Allocator}
// clang-format on
{
    VERIFY(PageSize <= std::numeric_limits<AllocationsMgrOffsetType>::max(),
           "PageSize (", PageSize, ") exceeds maximum allowed value ",
           std::numeric_limits<AllocationsMgrOffsetType>::max());

    VkMemoryAllocateInfo MemAlloc = {};

    MemAlloc.pNext           = nullptr;
    MemAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAlloc.allocationSize  = PageSize;
    MemAlloc.memoryTypeIndex = MemoryTypeIndex;

    auto MemoryName = Diligent::FormatString("Device memory page. Size: ", Diligent::FormatMemorySize(PageSize, 2), ", type: ", MemoryTypeIndex);
    m_VkMemory      = ParentMemoryMgr.m_LogicalDevice.AllocateDeviceMemory(MemAlloc, MemoryName.c_str());

    if (IsHostVisible)
    {
        auto err = ParentMemoryMgr.m_LogicalDevice.MapMemory(
            m_VkMemory,
            0, // offset
            PageSize,
            0, // flags, reserved for future use
            &m_CPUMemory);
        CHECK_VK_ERROR_AND_THROW(err, "Failed to map staging memory");
    }
}

VulkanMemoryPage::~VulkanMemoryPage()
{
    if (m_CPUMemory != nullptr)
    {
        // Unmapping memory is not necessary, byt anyway
        m_ParentMemoryMgr.m_LogicalDevice.UnmapMemory(m_VkMemory);
    }

    VERIFY(IsEmpty(), "Destroying a page with not all allocations released");
}

VulkanMemoryAllocation VulkanMemoryPage::Allocate(VkDeviceSize size, VkDeviceSize alignment)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};
    VERIFY(size <= std::numeric_limits<AllocationsMgrOffsetType>::max(),
           "Allocation size (", size, ") exceeds maximum allowed value ",
           std::numeric_limits<AllocationsMgrOffsetType>::max());
    auto Allocation = m_AllocationMgr.Allocate(static_cast<AllocationsMgrOffsetType>(size), static_cast<AllocationsMgrOffsetType>(alignment));
    if (Allocation.IsValid())
    {
        // Offset may not necessarily be aligned, but the allocation is guaranteed to be large enough
        // to accomodate requested alignment
        VERIFY_EXPR(Diligent::Align(VkDeviceSize{Allocation.UnalignedOffset}, alignment) - Allocation.UnalignedOffset + size <= Allocation.Size);
        return VulkanMemoryAllocation{this, Allocation.UnalignedOffset, Allocation.Size};
    }
    else
    {
        return VulkanMemoryAllocation{};
    }
}

void VulkanMemoryPage::Free(VulkanMemoryAllocation&& Allocation)
{
    m_ParentMemoryMgr.OnFreeAllocation(Allocation.Size, m_CPUMemory != nullptr);
    std::lock_guard<std::mutex> Lock{m_Mutex};
    VERIFY_EXPR(Allocation.UnalignedOffset <= std::numeric_limits<AllocationsMgrOffsetType>::max());
    VERIFY_EXPR(Allocation.Size <= std::numeric_limits<AllocationsMgrOffsetType>::max());
    m_AllocationMgr.Free(static_cast<AllocationsMgrOffsetType>(Allocation.UnalignedOffset), static_cast<AllocationsMgrOffsetType>(Allocation.Size));
    Allocation = VulkanMemoryAllocation{};
}

VulkanMemoryAllocation VulkanMemoryManager::Allocate(const VkMemoryRequirements& MemReqs, VkMemoryPropertyFlags MemoryProps)
{
    // memoryTypeBits is a bitmask and contains one bit set for every supported memory type for the resource.
    // Bit i is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties structure for the
    // physical device is supported for the resource.
    auto MemoryTypeIndex = m_PhysicalDevice.GetMemoryTypeIndex(MemReqs.memoryTypeBits, MemoryProps);
    if (MemoryProps == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        // There must be at least one memory type with the DEVICE_LOCAL_BIT bit set
        DEV_CHECK_ERR(MemoryTypeIndex != VulkanUtilities::VulkanPhysicalDevice::InvalidMemoryTypeIndex,
                      "Vulkan spec requires that memoryTypeBits member always contains "
                      "at least one bit set corresponding to a VkMemoryType with a propertyFlags that has the "
                      "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT bit set (11.6)");
    }
    else if (MemoryProps == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        DEV_CHECK_ERR(MemoryTypeIndex != VulkanUtilities::VulkanPhysicalDevice::InvalidMemoryTypeIndex,
                      "Vulkan spec requires that for a VkBuffer not created with the VK_BUFFER_CREATE_SPARSE_BINDING_BIT "
                      "bit set, or for a VkImage that was created with a VK_IMAGE_TILING_LINEAR value in the tiling member "
                      "of the VkImageCreateInfo structure passed to vkCreateImage, the memoryTypeBits member always contains "
                      "at least one bit set corresponding to a VkMemoryType with a propertyFlags that has both the "
                      "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit AND the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit set. (11.6)");
    }
    else if (MemoryTypeIndex == VulkanUtilities::VulkanPhysicalDevice::InvalidMemoryTypeIndex)
    {
        LOG_ERROR_AND_THROW("Failed to find suitable device memory type for a buffer");
    }

    bool HostVisible = (MemoryProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    return Allocate(MemReqs.size, MemReqs.alignment, MemoryTypeIndex, HostVisible);
}

VulkanMemoryAllocation VulkanMemoryManager::Allocate(VkDeviceSize Size, VkDeviceSize Alignment, uint32_t MemoryTypeIndex, bool HostVisible)
{
    VulkanMemoryAllocation Allocation;

    // On integrated GPUs, there is no difference between host-visible and GPU-only
    // memory, so MemoryTypeIndex is the same. As GPU-only pages do not have CPU address,
    // we need to use HostVisible flag to differentiate the two.
    // It is likely a good idea to always keep staging pages separate to reduce fragmenation
    // even though on integrated GPUs same pages can be used for both GPU-only and staging
    // allocations. Staging allocations are short-living and will be released when upload is
    // complete, while GPU-only allocations are expected to be long-living.
    MemoryPageIndex             PageIdx{MemoryTypeIndex, HostVisible};
    std::lock_guard<std::mutex> Lock{m_PagesMtx};

    auto range = m_Pages.equal_range(PageIdx);
    for (auto page_it = range.first; page_it != range.second; ++page_it)
    {
        Allocation = page_it->second.Allocate(Size, Alignment);
        if (Allocation.Page != nullptr)
            break;
    }

    size_t stat_ind = HostVisible ? 1 : 0;
    if (Allocation.Page == nullptr)
    {
        auto PageSize = HostVisible ? m_HostVisiblePageSize : m_DeviceLocalPageSize;
        while (PageSize < Size)
            PageSize *= 2;

        m_CurrAllocatedSize[stat_ind] += PageSize;
        m_PeakAllocatedSize[stat_ind] = std::max(m_PeakAllocatedSize[stat_ind], m_CurrAllocatedSize[stat_ind]);

        auto it = m_Pages.emplace(PageIdx, VulkanMemoryPage{*this, PageSize, MemoryTypeIndex, HostVisible});
        LOG_INFO_MESSAGE("VulkanMemoryManager '", m_MgrName, "': created new ", (HostVisible ? "host-visible" : "device-local"),
                         " page. (", Diligent::FormatMemorySize(PageSize, 2), ", type idx: ", MemoryTypeIndex,
                         "). Current allocated size: ", Diligent::FormatMemorySize(m_CurrAllocatedSize[stat_ind], 2));
        OnNewPageCreated(it->second);
        Allocation = it->second.Allocate(Size, Alignment);
        DEV_CHECK_ERR(Allocation.Page != nullptr, "Failed to allocate new memory page");
    }

    if (Allocation.Page != nullptr)
    {
        VERIFY_EXPR(Size + Diligent::Align(Allocation.UnalignedOffset, Alignment) - Allocation.UnalignedOffset <= Allocation.Size);
    }

    m_CurrUsedSize[stat_ind].fetch_add(Allocation.Size);
    m_PeakUsedSize[stat_ind] = std::max(m_PeakUsedSize[stat_ind], static_cast<VkDeviceSize>(m_CurrUsedSize[stat_ind].load()));

    return Allocation;
}

void VulkanMemoryManager::ShrinkMemory()
{
    std::lock_guard<std::mutex> Lock{m_PagesMtx};
    if (m_CurrAllocatedSize[0] <= m_DeviceLocalReserveSize && m_CurrAllocatedSize[1] <= m_HostVisibleReserveSize)
        return;

    auto it = m_Pages.begin();
    while (it != m_Pages.end())
    {
        auto curr_it = it;
        ++it;
        auto& Page          = curr_it->second;
        bool  IsHostVisible = Page.GetCPUMemory() != nullptr;
        auto  ReserveSize   = IsHostVisible ? m_HostVisibleReserveSize : m_DeviceLocalReserveSize;
        if (Page.IsEmpty() && m_CurrAllocatedSize[IsHostVisible ? 1 : 0] > ReserveSize)
        {
            auto PageSize = Page.GetPageSize();
            m_CurrAllocatedSize[IsHostVisible ? 1 : 0] -= PageSize;
            LOG_INFO_MESSAGE("VulkanMemoryManager '", m_MgrName, "': destroying ", (IsHostVisible ? "host-visible" : "device-local"),
                             " page (", Diligent::FormatMemorySize(PageSize, 2),
                             "). Current allocated size: ",
                             Diligent::FormatMemorySize(m_CurrAllocatedSize[IsHostVisible ? 1 : 0], 2));
            OnPageDestroy(Page);
            m_Pages.erase(curr_it);
        }
    }
}

void VulkanMemoryManager::OnFreeAllocation(VkDeviceSize Size, bool IsHostVisble)
{
    m_CurrUsedSize[IsHostVisble ? 1 : 0].fetch_add(-static_cast<int64_t>(Size));
}

VulkanMemoryManager::~VulkanMemoryManager()
{
    auto PeakDeviceLocalPages  = m_PeakAllocatedSize[0] / m_DeviceLocalPageSize;
    auto PeakHostVisisblePages = m_PeakAllocatedSize[1] / m_HostVisiblePageSize;
    LOG_INFO_MESSAGE("VulkanMemoryManager '", m_MgrName, "' stats:\n"
                                                         "                       Peak used/allocated device-local memory size: ",
                     Diligent::FormatMemorySize(m_PeakUsedSize[0], 2, m_PeakAllocatedSize[0]), " / ",
                     Diligent::FormatMemorySize(m_PeakAllocatedSize[0], 2, m_PeakAllocatedSize[0]),
                     " (", PeakDeviceLocalPages, (PeakDeviceLocalPages == 1 ? " page)" : " pages)"),
                     "\n                       Peak used/allocated host-visible memory size: ",
                     Diligent::FormatMemorySize(m_PeakUsedSize[1], 2, m_PeakAllocatedSize[1]), " / ",
                     Diligent::FormatMemorySize(m_PeakAllocatedSize[1], 2, m_PeakAllocatedSize[1]),
                     " (", PeakHostVisisblePages, (PeakHostVisisblePages == 1 ? " page)" : " pages)"));

    for (auto it = m_Pages.begin(); it != m_Pages.end(); ++it)
        VERIFY(it->second.IsEmpty(), "The page contains outstanding allocations");
    VERIFY(m_CurrUsedSize[0] == 0 && m_CurrUsedSize[1] == 0, "Not all allocations have been released");
}

} // namespace VulkanUtilities
