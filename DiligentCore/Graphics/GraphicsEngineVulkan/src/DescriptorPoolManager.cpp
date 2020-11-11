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
#include "DescriptorPoolManager.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

void DescriptorSetAllocation::Release()
{
    if (Set != VK_NULL_HANDLE)
    {
        VERIFY_EXPR(DescrSetAllocator != nullptr && Pool != VK_NULL_HANDLE);
        DescrSetAllocator->FreeDescriptorSet(Set, Pool, CmdQueueMask);

        Reset();
    }
}

VulkanUtilities::DescriptorPoolWrapper DescriptorPoolManager::CreateDescriptorPool(const char* DebugName) const
{
    VkDescriptorPoolCreateInfo PoolCI = {};

    PoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolCI.pNext = nullptr;
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT specifies that descriptor sets can
    // return their individual allocations to the pool, i.e. all of vkAllocateDescriptorSets,
    // vkFreeDescriptorSets, and vkResetDescriptorPool are allowed. (13.2.3)
    PoolCI.flags         = m_AllowFreeing ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0;
    PoolCI.maxSets       = m_MaxSets;
    PoolCI.poolSizeCount = static_cast<uint32_t>(m_PoolSizes.size());
    PoolCI.pPoolSizes    = m_PoolSizes.data();
    return m_DeviceVkImpl.GetLogicalDevice().CreateDescriptorPool(PoolCI, DebugName);
}

DescriptorPoolManager::~DescriptorPoolManager()
{
    DEV_CHECK_ERR(m_AllocatedPoolCounter == 0, "Not all allocated descriptor pools are returned to the pool manager");
    LOG_INFO_MESSAGE(m_PoolName, " stats: allocated ", m_Pools.size(), " pool(s)");
}

VulkanUtilities::DescriptorPoolWrapper DescriptorPoolManager::GetPool(const char* DebugName)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};
#ifdef DILIGENT_DEVELOPMENT
    ++m_AllocatedPoolCounter;
#endif
    if (m_Pools.empty())
        return CreateDescriptorPool(DebugName);
    else
    {
        auto& LogicalDevice = m_DeviceVkImpl.GetLogicalDevice();
        auto  Pool          = std::move(m_Pools.front());
        VulkanUtilities::SetDescriptorPoolName(LogicalDevice.GetVkDevice(), Pool, DebugName);
        m_Pools.pop_front();
        return Pool;
    }
}

void DescriptorPoolManager::DisposePool(VulkanUtilities::DescriptorPoolWrapper&& Pool, Uint64 QueueMask)
{
    class DescriptorPoolDeleter
    {
    public:
        // clang-format off
        DescriptorPoolDeleter(DescriptorPoolManager&                   _PoolMgr,
                              VulkanUtilities::DescriptorPoolWrapper&& _Pool) noexcept : 
            PoolMgr {&_PoolMgr       },
            Pool    {std::move(_Pool)}
        {}

        DescriptorPoolDeleter            (const DescriptorPoolDeleter&) = delete;
        DescriptorPoolDeleter& operator= (const DescriptorPoolDeleter&) = delete;
        DescriptorPoolDeleter& operator= (      DescriptorPoolDeleter&&)= delete;

        DescriptorPoolDeleter(DescriptorPoolDeleter&& rhs)noexcept : 
            PoolMgr {rhs.PoolMgr        },
            Pool    {std::move(rhs.Pool)}
        {
            rhs.PoolMgr = nullptr;
        }
        // clang-format on

        ~DescriptorPoolDeleter()
        {
            if (PoolMgr != nullptr)
            {
                PoolMgr->FreePool(std::move(Pool));
            }
        }

    private:
        DescriptorPoolManager*                 PoolMgr;
        VulkanUtilities::DescriptorPoolWrapper Pool;
    };

    m_DeviceVkImpl.SafeReleaseDeviceObject(DescriptorPoolDeleter{*this, std::move(Pool)}, QueueMask);
}

void DescriptorPoolManager::FreePool(VulkanUtilities::DescriptorPoolWrapper&& Pool)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};
    m_DeviceVkImpl.GetLogicalDevice().ResetDescriptorPool(Pool);
    m_Pools.emplace_back(std::move(Pool));
#ifdef DILIGENT_DEVELOPMENT
    --m_AllocatedPoolCounter;
#endif
}


static VkDescriptorSet AllocateDescriptorSet(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                             VkDescriptorPool                            Pool,
                                             VkDescriptorSetLayout                       SetLayout,
                                             const char*                                 DebugName)
{
    VkDescriptorSetAllocateInfo DescrSetAllocInfo = {};

    DescrSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescrSetAllocInfo.pNext              = nullptr;
    DescrSetAllocInfo.descriptorPool     = Pool;
    DescrSetAllocInfo.descriptorSetCount = 1;
    DescrSetAllocInfo.pSetLayouts        = &SetLayout;
    // Descriptor pools are externally synchronized, meaning that the application must not allocate
    // and/or free descriptor sets from the same pool in multiple threads simultaneously (13.2.3)
    return LogicalDevice.AllocateVkDescriptorSet(DescrSetAllocInfo, DebugName);
}


DescriptorSetAllocator::~DescriptorSetAllocator()
{
    DEV_CHECK_ERR(m_AllocatedSetCounter == 0, m_AllocatedSetCounter, " descriptor set(s) have not been returned to the allocator. If there are outstanding references to the sets in release queues, the app will crash when DescriptorSetAllocator::FreeDescriptorSet() is called");
}

DescriptorSetAllocation DescriptorSetAllocator::Allocate(Uint64 CommandQueueMask, VkDescriptorSetLayout SetLayout, const char* DebugName)
{
    // Descriptor pools are externally synchronized, meaning that the application must not allocate
    // and/or free descriptor sets from the same pool in multiple threads simultaneously (13.2.3)
    std::lock_guard<std::mutex> Lock{m_Mutex};

    const auto& LogicalDevice = m_DeviceVkImpl.GetLogicalDevice();
    // Try all pools starting from the frontmost
    for (auto it = m_Pools.begin(); it != m_Pools.end(); ++it)
    {
        auto& Pool = *it;
        auto  Set  = AllocateDescriptorSet(LogicalDevice, Pool, SetLayout, DebugName);
        if (Set != VK_NULL_HANDLE)
        {
            // Move the pool to the front
            if (it != m_Pools.begin())
            {
                std::swap(*it, m_Pools.front());
            }

#ifdef DILIGENT_DEVELOPMENT
            ++m_AllocatedSetCounter;
#endif
            return {Set, Pool, CommandQueueMask, *this};
        }
    }

    // Failed to allocate descriptor from existing pools -> create a new one
    LOG_INFO_MESSAGE("Allocated new descriptor pool");
    m_Pools.emplace_front(CreateDescriptorPool("Descriptor pool"));

    auto& NewPool = m_Pools.front();
    auto  Set     = AllocateDescriptorSet(LogicalDevice, NewPool, SetLayout, DebugName);
    DEV_CHECK_ERR(Set != VK_NULL_HANDLE, "Failed to allocate descriptor set");

#ifdef DILIGENT_DEVELOPMENT
    ++m_AllocatedSetCounter;
#endif

    return {Set, NewPool, CommandQueueMask, *this};
}

void DescriptorSetAllocator::FreeDescriptorSet(VkDescriptorSet Set, VkDescriptorPool Pool, Uint64 QueueMask)
{
    class DescriptorSetDeleter
    {
    public:
        // clang-format off
        DescriptorSetDeleter(DescriptorSetAllocator& _Allocator,
                             VkDescriptorSet         _Set,
                             VkDescriptorPool        _Pool) : 
            Allocator {&_Allocator},
            Set       {_Set       },
            Pool      {_Pool      }
        {}

        DescriptorSetDeleter             (const DescriptorSetDeleter&) = delete;
        DescriptorSetDeleter& operator = (const DescriptorSetDeleter&) = delete;
        DescriptorSetDeleter& operator = (      DescriptorSetDeleter&&)= delete;

        DescriptorSetDeleter(DescriptorSetDeleter&& rhs)noexcept : 
            Allocator {rhs.Allocator},
            Set       {rhs.Set      },
            Pool      {rhs.Pool     }
        {
            rhs.Allocator = nullptr;
            rhs.Set       = VK_NULL_HANDLE;
            rhs.Pool      = VK_NULL_HANDLE;
        }
        // clang-format on

        ~DescriptorSetDeleter()
        {
            if (Allocator != nullptr)
            {
                std::lock_guard<std::mutex> Lock{Allocator->m_Mutex};
                Allocator->m_DeviceVkImpl.GetLogicalDevice().FreeDescriptorSet(Pool, Set);
#ifdef DILIGENT_DEVELOPMENT
                --Allocator->m_AllocatedSetCounter;
#endif
            }
        }

    private:
        DescriptorSetAllocator* Allocator;
        VkDescriptorSet         Set;
        VkDescriptorPool        Pool;
    };
    m_DeviceVkImpl.SafeReleaseDeviceObject(DescriptorSetDeleter{*this, Set, Pool}, QueueMask);
}


VkDescriptorSet DynamicDescriptorSetAllocator::Allocate(VkDescriptorSetLayout SetLayout, const char* DebugName)
{
    VkDescriptorSet set           = VK_NULL_HANDLE;
    const auto&     LogicalDevice = m_GlobalPoolMgr.GetDeviceVkImpl().GetLogicalDevice();
    if (!m_AllocatedPools.empty())
    {
        set = AllocateDescriptorSet(LogicalDevice, m_AllocatedPools.back(), SetLayout, DebugName);
    }

    if (set == VK_NULL_HANDLE)
    {
        m_AllocatedPools.emplace_back(m_GlobalPoolMgr.GetPool("Dynamic Descriptor Pool"));
        set = AllocateDescriptorSet(LogicalDevice, m_AllocatedPools.back(), SetLayout, DebugName);
    }

    return set;
}

void DynamicDescriptorSetAllocator::ReleasePools(Uint64 QueueMask)
{
    for (auto& Pool : m_AllocatedPools)
    {
        m_GlobalPoolMgr.DisposePool(std::move(Pool), QueueMask);
    }
    m_PeakPoolCount = std::max(m_PeakPoolCount, m_AllocatedPools.size());
    m_AllocatedPools.clear();
}

DynamicDescriptorSetAllocator::~DynamicDescriptorSetAllocator()
{
    DEV_CHECK_ERR(m_AllocatedPools.empty(), "All allocated pools must be returned to the parent descriptor pool manager");
    LOG_INFO_MESSAGE(m_Name, " peak descriptor pool count: ", m_PeakPoolCount);
}

} // namespace Diligent
