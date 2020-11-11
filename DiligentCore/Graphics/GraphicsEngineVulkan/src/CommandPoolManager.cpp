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
#include "CommandPoolManager.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

CommandPoolManager::CommandPoolManager(RenderDeviceVkImpl&      DeviceVkImpl,
                                       std::string              Name,
                                       uint32_t                 queueFamilyIndex,
                                       VkCommandPoolCreateFlags flags) noexcept :
    // clang-format off
    m_DeviceVkImpl    {DeviceVkImpl     },
    m_Name            {std::move(Name)  },
    m_QueueFamilyIndex{queueFamilyIndex },
    m_CmdPoolFlags    {flags            },
    m_CmdPools        (STD_ALLOCATOR_RAW_MEM(VulkanUtilities::CommandPoolWrapper, GetRawAllocator(), "Allocator for deque<VulkanUtilities::CommandPoolWrapper>"))
// clang-format on
{
#ifdef DILIGENT_DEVELOPMENT
    m_AllocatedPoolCounter = 0;
#endif
}

VulkanUtilities::CommandPoolWrapper CommandPoolManager::AllocateCommandPool(const char* DebugName)
{
    std::lock_guard<std::mutex> LockGuard{m_Mutex};

    VulkanUtilities::CommandPoolWrapper CmdPool;
    if (!m_CmdPools.empty())
    {
        CmdPool = std::move(m_CmdPools.front());
        m_CmdPools.pop_front();
    }

    auto& LogicalDevice = m_DeviceVkImpl.GetLogicalDevice();
    if (CmdPool == VK_NULL_HANDLE)
    {
        VkCommandPoolCreateInfo CmdPoolCI = {};

        CmdPoolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CmdPoolCI.pNext            = nullptr;
        CmdPoolCI.queueFamilyIndex = m_QueueFamilyIndex;
        CmdPoolCI.flags            = m_CmdPoolFlags;

        CmdPool = LogicalDevice.CreateCommandPool(CmdPoolCI);
        DEV_CHECK_ERR(CmdPool != VK_NULL_HANDLE, "Failed to create Vulkan command pool");
    }

    LogicalDevice.ResetCommandPool(CmdPool);

#ifdef DILIGENT_DEVELOPMENT
    ++m_AllocatedPoolCounter;
#endif
    return std::move(CmdPool);
}

void CommandPoolManager::SafeReleaseCommandPool(VulkanUtilities::CommandPoolWrapper&& CmdPool, Uint32 CmdQueueIndex, Uint64 FenceValue)
{
    class CommandPoolDeleter
    {
    public:
        CommandPoolDeleter(CommandPoolManager& _CmdPoolMgr, VulkanUtilities::CommandPoolWrapper&& _Pool) :
            // clang-format off
            CmdPoolMgr{&_CmdPoolMgr    },
            Pool      {std::move(_Pool)}
        // clang-format on
        {
            VERIFY_EXPR(Pool != VK_NULL_HANDLE);
        }

        // clang-format off
        CommandPoolDeleter             (const CommandPoolDeleter&)  = delete;
        CommandPoolDeleter& operator = (const CommandPoolDeleter&)  = delete;
        CommandPoolDeleter& operator = (      CommandPoolDeleter&&) = delete;

        CommandPoolDeleter(CommandPoolDeleter&& rhs) : 
            CmdPoolMgr{rhs.CmdPoolMgr     },
            Pool      {std::move(rhs.Pool)}
        {
            rhs.CmdPoolMgr = nullptr;
        }
        // clang-format on

        ~CommandPoolDeleter()
        {
            if (CmdPoolMgr != nullptr)
            {
                CmdPoolMgr->FreeCommandPool(std::move(Pool));
            }
        }

    private:
        CommandPoolManager*                 CmdPoolMgr;
        VulkanUtilities::CommandPoolWrapper Pool;
    };

    // Discard command pool directly to the release queue since we know exactly which queue it was submitted to
    // as well as the associated FenceValue
    m_DeviceVkImpl.GetReleaseQueue(CmdQueueIndex).DiscardResource(CommandPoolDeleter{*this, std::move(CmdPool)}, FenceValue);
}

void CommandPoolManager::FreeCommandPool(VulkanUtilities::CommandPoolWrapper&& CmdPool)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);
#ifdef DILIGENT_DEVELOPMENT
    --m_AllocatedPoolCounter;
#endif
    m_CmdPools.emplace_back(std::move(CmdPool));
}

void CommandPoolManager::DestroyPools()
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);
    DEV_CHECK_ERR(m_AllocatedPoolCounter == 0, m_AllocatedPoolCounter, " pool(s) have not been freed. This will cause a crash if the references to these pools are still in release queues when CommandPoolManager::FreeCommandPool() is called for destroyed CommandPoolManager object.");
    LOG_INFO_MESSAGE(m_Name, " allocated descriptor pool count: ", m_CmdPools.size());
    m_CmdPools.clear();
}

CommandPoolManager::~CommandPoolManager()
{
    DEV_CHECK_ERR(m_CmdPools.empty() && m_AllocatedPoolCounter == 0, "Command pools have not been destroyed");
}

} // namespace Diligent
