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
#include <sstream>

#include "VulkanUtilities/VulkanCommandBufferPool.hpp"
#include "VulkanUtilities/VulkanDebug.hpp"
#include "Errors.hpp"
#include "DebugUtilities.hpp"
#include "VulkanErrors.hpp"

namespace VulkanUtilities
{

VulkanCommandBufferPool::VulkanCommandBufferPool(std::shared_ptr<const VulkanLogicalDevice> LogicalDevice,
                                                 uint32_t                                   queueFamilyIndex,
                                                 VkCommandPoolCreateFlags                   flags) :
    m_LogicalDevice{std::move(LogicalDevice)}
{
    VkCommandPoolCreateInfo CmdPoolCI = {};

    CmdPoolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolCI.pNext            = nullptr;
    CmdPoolCI.queueFamilyIndex = queueFamilyIndex;
    CmdPoolCI.flags            = flags;

    m_CmdPool = m_LogicalDevice->CreateCommandPool(CmdPoolCI);
    DEV_CHECK_ERR(m_CmdPool != VK_NULL_HANDLE, "Failed to create vulkan command pool");
#ifdef DILIGENT_DEVELOPMENT
    m_BuffCounter = 0;
#endif
}

VulkanCommandBufferPool::~VulkanCommandBufferPool()
{
    m_CmdPool.Release();
    DEV_CHECK_ERR(m_BuffCounter == 0, m_BuffCounter, " command buffer(s) have not been returned to the pool. If there are outstanding references to these buffers in release queues, FreeCommandBuffer() will crash when attempting to return a buffer to the pool.");
}

VkCommandBuffer VulkanCommandBufferPool::GetCommandBuffer(const char* DebugName)
{
    VkCommandBuffer CmdBuffer = VK_NULL_HANDLE;

    {
        std::lock_guard<std::mutex> Lock{m_Mutex};

        if (!m_CmdBuffers.empty())
        {
            CmdBuffer = m_CmdBuffers.front();
            auto err  = vkResetCommandBuffer(
                CmdBuffer,
                0 // VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT -  specifies that most or all memory resources currently
                  // owned by the command buffer should be returned to the parent command pool.
            );
            DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to reset command buffer");
            (void)err;
            m_CmdBuffers.pop_front();
        }
    }

    // If no cmd buffers were ready to be reused, create a new one
    if (CmdBuffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo BuffAllocInfo = {};

        BuffAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        BuffAllocInfo.pNext              = nullptr;
        BuffAllocInfo.commandPool        = m_CmdPool;
        BuffAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        BuffAllocInfo.commandBufferCount = 1;

        CmdBuffer = m_LogicalDevice->AllocateVkCommandBuffer(BuffAllocInfo);
        DEV_CHECK_ERR(CmdBuffer != VK_NULL_HANDLE, "Failed to allocate vulkan command buffer");
    }

    VkCommandBufferBeginInfo CmdBuffBeginInfo = {};

    CmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBuffBeginInfo.pNext = nullptr;
    CmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Each recording of the command buffer will only be
                                                                          // submitted once, and the command buffer will be reset
                                                                          // and recorded again between each submission.
    CmdBuffBeginInfo.pInheritanceInfo = nullptr;                          // Ignored for a primary command buffer

    auto err = vkBeginCommandBuffer(CmdBuffer, &CmdBuffBeginInfo);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to begin command buffer");
    (void)err;
#ifdef DILIGENT_DEVELOPMENT
    ++m_BuffCounter;
#endif
    return CmdBuffer;
}

void VulkanCommandBufferPool::FreeCommandBuffer(VkCommandBuffer&& CmdBuffer)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};
    m_CmdBuffers.emplace_back(CmdBuffer);
    CmdBuffer = VK_NULL_HANDLE;
#ifdef DILIGENT_DEVELOPMENT
    --m_BuffCounter;
#endif
}

CommandPoolWrapper&& VulkanCommandBufferPool::Release()
{
    m_LogicalDevice.reset();
    m_CmdBuffers.clear();
    return std::move(m_CmdPool);
}

} // namespace VulkanUtilities
