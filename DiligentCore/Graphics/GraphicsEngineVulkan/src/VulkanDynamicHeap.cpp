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
#include <chrono>
#include <thread>
#include "VulkanDynamicHeap.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

static VkDeviceSize GetDefaultAlignment(const VulkanUtilities::VulkanPhysicalDevice& PhysicalDevice)
{
    const auto& Props  = PhysicalDevice.GetProperties();
    const auto& Limits = Props.limits;
    return std::max(std::max(Limits.minUniformBufferOffsetAlignment, Limits.minTexelBufferOffsetAlignment), Limits.minStorageBufferOffsetAlignment);
}

VulkanDynamicMemoryManager::VulkanDynamicMemoryManager(IMemoryAllocator&   Allocator,
                                                       RenderDeviceVkImpl& DeviceVk,
                                                       Uint32              Size,
                                                       Uint64              CommandQueueMask) :
    // clang-format off
    TBase             {Allocator, Size},
    m_DeviceVk        {DeviceVk},
    m_DefaultAlignment{GetDefaultAlignment(DeviceVk.GetPhysicalDevice())},
    m_CommandQueueMask{CommandQueueMask}
// clang-format on
{
    VERIFY((Size & (MasterBlockAlignment - 1)) == 0, "Heap size (", Size, " is not aligned by the master block alignment (", Uint32{MasterBlockAlignment}, ")");
    VkBufferCreateInfo VkBuffCI = {};

    VkBuffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    VkBuffCI.pNext = nullptr;
    VkBuffCI.flags = 0; // VK_BUFFER_CREATE_SPARSE_BINDING_BIT, VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT, VK_BUFFER_CREATE_SPARSE_ALIASED_BIT
    VkBuffCI.size  = Size;
    VkBuffCI.usage =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    VkBuffCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffCI.queueFamilyIndexCount = 0;
    VkBuffCI.pQueueFamilyIndices   = nullptr;

    const auto& LogicalDevice    = DeviceVk.GetLogicalDevice();
    m_VkBuffer                   = LogicalDevice.CreateBuffer(VkBuffCI, "Dynamic heap buffer");
    VkMemoryRequirements MemReqs = LogicalDevice.GetBufferMemoryRequirements(m_VkBuffer);

    const auto& PhysicalDevice = DeviceVk.GetPhysicalDevice();

    VkMemoryAllocateInfo MemAlloc = {};

    MemAlloc.pNext          = nullptr;
    MemAlloc.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAlloc.allocationSize = MemReqs.size;

    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache management commands vkFlushMappedMemoryRanges
    // and vkInvalidateMappedMemoryRanges are NOT needed to flush host writes to the device or make device writes visible
    // to the host (10.2)
    MemAlloc.memoryTypeIndex = PhysicalDevice.GetMemoryTypeIndex(MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VERIFY(MemAlloc.memoryTypeIndex != VulkanUtilities::VulkanPhysicalDevice::InvalidMemoryTypeIndex,
           "Vulkan spec requires that for a VkBuffer not created with the "
           "VK_BUFFER_CREATE_SPARSE_BINDING_BIT bit set, the memoryTypeBits member always contains at least one bit set "
           "corresponding to a VkMemoryType with a propertyFlags that has both the VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit "
           "and the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit set(11.6)");

    m_BufferMemory = LogicalDevice.AllocateDeviceMemory(MemAlloc, "Host-visible memory for upload buffer");

    void* Data = nullptr;

    auto err = LogicalDevice.MapMemory(
        m_BufferMemory,
        0, // offset
        MemAlloc.allocationSize,
        0, // flags, reserved for future use
        &Data);
    m_CPUAddress = reinterpret_cast<Uint8*>(Data);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to map  memory");

    err = LogicalDevice.BindBufferMemory(m_VkBuffer, m_BufferMemory, 0 /*offset*/);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to bind  bufer memory");

    LOG_INFO_MESSAGE("GPU dynamic heap created. Total buffer size: ", FormatMemorySize(Size, 2));
}

void VulkanDynamicMemoryManager::Destroy()
{
    if (m_VkBuffer)
    {
        m_DeviceVk.GetLogicalDevice().UnmapMemory(m_BufferMemory);
        m_DeviceVk.SafeReleaseDeviceObject(std::move(m_VkBuffer), m_CommandQueueMask);
        m_DeviceVk.SafeReleaseDeviceObject(std::move(m_BufferMemory), m_CommandQueueMask);
    }
    m_CPUAddress = nullptr;
}

VulkanDynamicMemoryManager::~VulkanDynamicMemoryManager()
{
    VERIFY(m_BufferMemory == VK_NULL_HANDLE && m_VkBuffer == VK_NULL_HANDLE, "Vulkan resources must be explcitly released with Destroy()");
    auto Size = GetSize();
    LOG_INFO_MESSAGE("Dynamic memory manager usage stats:\n"
                     "                       Total size: ",
                     FormatMemorySize(Size, 2),
                     ". Peak allocated size: ", FormatMemorySize(m_TotalPeakSize, 2, Size),
                     ". Peak utilization: ",
                     std::fixed, std::setprecision(1), static_cast<double>(m_TotalPeakSize) / static_cast<double>(std::max(Size, size_t{1})) * 100.0, '%');
}


VulkanDynamicMemoryManager::MasterBlock VulkanDynamicMemoryManager::AllocateMasterBlock(OffsetType SizeInBytes, OffsetType Alignment)
{
    if (Alignment == 0)
        Alignment = MasterBlockAlignment;

    if (SizeInBytes > GetSize())
    {
        LOG_ERROR("Requested dynamic allocation size ", SizeInBytes,
                  " exceeds maximum dynamic memory size ", GetSize(),
                  ". The app should increase dynamic heap size.");
        return MasterBlock{};
    }

    auto Block = TBase::AllocateMasterBlock(SizeInBytes, Alignment);
    if (!Block.IsValid())
    {
        // Allocation failed. Try to wait for GPU to finish pending frames to release some space
        auto                          StartIdleTime   = std::chrono::high_resolution_clock::now();
        static constexpr const auto   SleepPeriod     = std::chrono::milliseconds(1);
        static constexpr const auto   MaxIdleDuration = std::chrono::duration<double>{60.0 / 1000.0}; // 60 ms
        std::chrono::duration<double> IdleDuration;
        Uint32                        SleepIterations = 0;
        while (!Block.IsValid() && IdleDuration < MaxIdleDuration)
        {
            m_DeviceVk.PurgeReleaseQueues();
            Block = TBase::AllocateMasterBlock(SizeInBytes, Alignment);
            if (!Block.IsValid())
            {
                std::this_thread::sleep_for(SleepPeriod);
                ++SleepIterations;
            }

            auto CurrTime = std::chrono::high_resolution_clock::now();
            IdleDuration  = std::chrono::duration_cast<std::chrono::duration<double>>(CurrTime - StartIdleTime);
        }

        if (!Block.IsValid())
        {
            // Last resort - idle GPU (there seems to have been a driver bug at some point: vkQueueWaitIdle() would deadlock and never return)
            m_DeviceVk.IdleGPU();
            Block = TBase::AllocateMasterBlock(SizeInBytes, Alignment);
            if (!Block.IsValid())
            {
                LOG_ERROR_MESSAGE("Space in dynamic heap is exausted! After idling for ",
                                  std::fixed, std::setprecision(1), IdleDuration.count() * 1000.0,
                                  " ms still no space is available. Increase the size of the heap by setting "
                                  "EngineVkCreateInfo::DynamicHeapSize to a greater value or optimize dynamic resource usage");
            }
            else
            {
                LOG_WARNING_MESSAGE("Space in dynamic heap is almost exausted. Allocation forced idling the GPU. "
                                    "Increase the size of the heap by setting EngineVkCreateInfo::DynamicHeapSize to a "
                                    "greater value or optimize dynamic resource usage");
            }
        }
        else
        {
            if (SleepIterations == 0)
            {
                LOG_WARNING_MESSAGE("Space in dynamic heap is almost exausted forcing mid-frame shrinkage. "
                                    "Increase the size of the heap buffer by setting EngineVkCreateInfo::DynamicHeapSize to a "
                                    "greater value or optimize dynamic resource usage");
            }
            else
            {
                LOG_WARNING_MESSAGE("Space in dynamic heap is almost exausted. Allocation forced wait time of ",
                                    std::fixed, std::setprecision(1), IdleDuration.count() * 1000.0,
                                    " ms. Increase the size of the heap by setting EngineVkCreateInfo::DynamicHeapSize "
                                    "to a greater value or optimize dynamic resource usage");
            }
        }
    }

    if (Block.IsValid())
    {
        m_TotalPeakSize = std::max(m_TotalPeakSize, GetUsedSize());
    }

    return Block;
}


VulkanDynamicAllocation VulkanDynamicHeap::Allocate(Uint32 SizeInBytes, Uint32 Alignment)
{
    VERIFY_EXPR(Alignment > 0);
    VERIFY(IsPowerOfTwo(Alignment), "Alignment (", Alignment, ") must be power of 2");

    auto       AlignedOffset = InvalidOffset;
    OffsetType AlignedSize   = 0;
    if (SizeInBytes > m_MasterBlockSize / 2)
    {
        // Allocate directly from the memory manager
        auto MasterBlock = m_GlobalDynamicMemMgr.AllocateMasterBlock(SizeInBytes, Alignment);
        if (MasterBlock.IsValid())
        {
            AlignedOffset = Align(MasterBlock.UnalignedOffset, size_t{Alignment});
            AlignedSize   = MasterBlock.Size;
            VERIFY_EXPR(MasterBlock.Size >= SizeInBytes + (AlignedOffset - MasterBlock.UnalignedOffset));
            m_CurrAllocatedSize += static_cast<Uint32>(MasterBlock.Size);
            m_MasterBlocks.emplace_back(MasterBlock);
        }
    }
    else
    {
        if (m_CurrOffset == InvalidOffset || SizeInBytes + (Align(m_CurrOffset, size_t{Alignment}) - m_CurrOffset) > m_AvailableSize)
        {
            auto MasterBlock = m_GlobalDynamicMemMgr.AllocateMasterBlock(m_MasterBlockSize, 0);
            if (MasterBlock.IsValid())
            {
                m_CurrOffset = MasterBlock.UnalignedOffset;
                m_CurrAllocatedSize += static_cast<Uint32>(MasterBlock.Size);
                m_AvailableSize = static_cast<Uint32>(MasterBlock.Size);
                m_MasterBlocks.emplace_back(MasterBlock);
            }
        }

        if (m_CurrOffset != InvalidOffset)
        {
            AlignedOffset = Align(m_CurrOffset, size_t{Alignment});
            AlignedSize   = SizeInBytes + (AlignedOffset - m_CurrOffset);
            if (AlignedSize <= m_AvailableSize)
            {
                m_AvailableSize -= static_cast<Uint32>(AlignedSize);
                m_CurrOffset += static_cast<Uint32>(AlignedSize);
            }
            else
                AlignedOffset = InvalidOffset;
        }
    }

    // Every device context uses its own dynamic heap, so there is no need to lock
    if (AlignedOffset != InvalidOffset)
    {
        m_CurrAlignedSize += static_cast<Uint32>(AlignedSize);
        m_CurrUsedSize += SizeInBytes;
        m_PeakAlignedSize   = std::max(m_PeakAlignedSize, m_CurrAlignedSize);
        m_PeakUsedSize      = std::max(m_PeakUsedSize, m_CurrUsedSize);
        m_PeakAllocatedSize = std::max(m_PeakAllocatedSize, m_CurrAllocatedSize);

        VERIFY_EXPR((AlignedOffset & (Alignment - 1)) == 0);
        return VulkanDynamicAllocation{m_GlobalDynamicMemMgr, AlignedOffset, SizeInBytes};
    }
    else
        return VulkanDynamicAllocation{};
}

void VulkanDynamicHeap::ReleaseMasterBlocks(RenderDeviceVkImpl& DeviceVkImpl, Uint64 CmdQueueMask)
{
    m_GlobalDynamicMemMgr.ReleaseMasterBlocks(m_MasterBlocks, DeviceVkImpl, CmdQueueMask);
    m_MasterBlocks.clear();

    m_CurrOffset    = InvalidOffset;
    m_AvailableSize = 0;

    m_CurrUsedSize      = 0;
    m_CurrAlignedSize   = 0;
    m_CurrAllocatedSize = 0;
}

VulkanDynamicHeap::~VulkanDynamicHeap()
{
    DEV_CHECK_ERR(m_MasterBlocks.empty(), m_MasterBlocks.size(), " master block(s) have not been returned to dynamic memory manager");

    auto PeakAllocatedPages = m_PeakAllocatedSize / m_MasterBlockSize;
    LOG_INFO_MESSAGE(m_HeapName,
                     " usage stats:\n"
                     "                       Peak used/aligned/allocated size: ",
                     FormatMemorySize(m_PeakUsedSize, 2, m_PeakAllocatedSize), " / ",
                     FormatMemorySize(m_PeakAlignedSize, 2, m_PeakAllocatedSize), " / ",
                     FormatMemorySize(m_PeakAllocatedSize, 2, m_PeakAllocatedSize),
                     " (", PeakAllocatedPages, (PeakAllocatedPages == 1 ? " page)" : " pages)"),
                     ". Peak efficiency (used/aligned): ", std::fixed, std::setprecision(1), static_cast<double>(m_PeakUsedSize) / static_cast<double>(std::max(m_PeakAlignedSize, 1U)) * 100.0, '%',
                     ". Peak utilization (used/allocated): ", std::fixed, std::setprecision(1), static_cast<double>(m_PeakUsedSize) / static_cast<double>(std::max(m_PeakAllocatedSize, 1U)) * 100.0, '%');
}

} // namespace Diligent
