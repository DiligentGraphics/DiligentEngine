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

#include <unordered_map>
#include "VulkanUtilities/VulkanMemoryManager.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"

namespace Diligent
{

// Upload heap is used by a device context to update texture and buffer regions through
// UpdateBufferRegion() and UpdateTextureRegion().
//
// The heap allocates pages from the global memory manager.
// The pages are released and returned to the manager at the end of every frame.
//
//   _______________________________________________________________________________________________________________________________
//  |                                                                                                                               |
//  |                                                  VulkanUploadHeap                                                             |
//  |                                                                                                                               |
//  |  || - - - - - - - - - - Page[0] - - - - - - - - - - -||    || - - - - - - - - - - Page[1] - - - - - - - - - - -||             |
//  |  || Allocation0 | Allocation1 |  ...   | AllocationN ||    || Allocation0 | Allocation1 |  ...   | AllocationM ||   ...       |
//  |__________|____________________________________________________________________________________________________________________|
//             |                                      A                   |
//             |                                      |                   |
//             |Allocate()             CreateNewPage()|                   |ReleaseAllocatedPages()
//             |                                ______|___________________V____
//             V                               |                              |
//   VulkanUploadAllocation                    |    Global Memory Manager     |
//                                             |    (VulkanMemoryManager)     |
//                                             |                              |
//                                             |______________________________|
//
class RenderDeviceVkImpl;

struct VulkanUploadAllocation
{
    VulkanUploadAllocation() noexcept {}
    // clang-format off
    VulkanUploadAllocation(void*        _CPUAddress,
                           VkDeviceSize _Size,
                           VkDeviceSize _AlignedOffset,
                           VkBuffer     _vkBuffer) noexcept :
        vkBuffer     {_vkBuffer     },
        CPUAddress   {_CPUAddress   },
        Size         {_Size         },
        AlignedOffset{_AlignedOffset}
    {}
    VulkanUploadAllocation             (const VulkanUploadAllocation&)  = delete;
    VulkanUploadAllocation& operator = (const VulkanUploadAllocation&)  = delete;
    VulkanUploadAllocation             (      VulkanUploadAllocation&&) = default;
    VulkanUploadAllocation& operator = (      VulkanUploadAllocation&&) = default;
    // clang-format on

    VkBuffer     vkBuffer      = VK_NULL_HANDLE; // Vulkan buffer associated with this memory.
    void*        CPUAddress    = nullptr;
    VkDeviceSize Size          = 0;
    VkDeviceSize AlignedOffset = 0;
};

class VulkanUploadHeap
{
public:
    VulkanUploadHeap(RenderDeviceVkImpl& RenderDevice,
                     std::string         HeapName,
                     VkDeviceSize        PageSize);

    // clang-format off
    VulkanUploadHeap            (const VulkanUploadHeap&)  = delete;
    VulkanUploadHeap            (      VulkanUploadHeap&&) = delete;
    VulkanUploadHeap& operator= (const VulkanUploadHeap&)  = delete;
    VulkanUploadHeap& operator= (      VulkanUploadHeap&&) = delete;
    // clang-format on

    ~VulkanUploadHeap();

    VulkanUploadAllocation Allocate(VkDeviceSize SizeInBytes, VkDeviceSize Alignment);

    // Releases all allocated pages that are later returned to the global memory manager by the release queues.
    // As global memory manager is hosted by the render device, the upload heap can be destroyed before the
    // pages are actually returned to the manager.
    void ReleaseAllocatedPages(Uint64 CmdQueueMask);

    size_t GetStalePagesCount() const
    {
        return m_Pages.size();
    }

private:
    RenderDeviceVkImpl& m_RenderDevice;
    std::string         m_HeapName;
    const VkDeviceSize  m_PageSize;

    struct UploadPageInfo
    {
        // clang-format off
        UploadPageInfo(VulkanUtilities::VulkanMemoryAllocation&& _MemAllocation, 
                       VulkanUtilities::BufferWrapper&&          _Buffer,
                       Uint8*                                    _CPUAddress) :
            MemAllocation{std::move(_MemAllocation)},
            Buffer       {std::move(_Buffer)       },
            CPUAddress   {_CPUAddress              }
        {
        }
        // clang-format on

        VulkanUtilities::VulkanMemoryAllocation MemAllocation;
        VulkanUtilities::BufferWrapper          Buffer;
        Uint8* const                            CPUAddress = nullptr;
    };
    std::vector<UploadPageInfo> m_Pages;

    struct CurrPageInfo
    {
        VkBuffer     vkBuffer       = VK_NULL_HANDLE;
        Uint8*       CurrCPUAddress = nullptr;
        VkDeviceSize CurrOffset     = 0;
        VkDeviceSize AvailableSize  = 0;

        void Reset(UploadPageInfo& NewPage, VkDeviceSize PageSize)
        {
            vkBuffer       = NewPage.Buffer;
            CurrCPUAddress = NewPage.CPUAddress;
            CurrOffset     = 0;
            AvailableSize  = PageSize;
        }

        void Advance(VkDeviceSize SizeInBytes)
        {
            CurrCPUAddress += SizeInBytes;
            CurrOffset += SizeInBytes;
            AvailableSize -= SizeInBytes;
        }
    } m_CurrPage;

    VkDeviceSize m_CurrFrameSize     = 0;
    VkDeviceSize m_PeakFrameSize     = 0;
    VkDeviceSize m_CurrAllocatedSize = 0;
    VkDeviceSize m_PeakAllocatedSize = 0;

    UploadPageInfo CreateNewPage(VkDeviceSize SizeInBytes) const;
};

} // namespace Diligent
