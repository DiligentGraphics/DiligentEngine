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

// Descriptor heap management utilities.
// See http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/ for details

#pragma once

#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include "VulkanUtilities/VulkanObjectWrappers.hpp"

namespace Diligent
{

class DescriptorSetAllocator;
class RenderDeviceVkImpl;

// This class manages descriptor set allocation.
// The class destructor calls DescriptorSetAllocator::FreeDescriptorSet() that moves
// the set into the release queue.
// sizeof(DescriptorSetAllocation) == 32 (x64)
class DescriptorSetAllocation
{
public:
    // clang-format off
    DescriptorSetAllocation(VkDescriptorSet         _Set,
                            VkDescriptorPool        _Pool,
                            Uint64                  _CmdQueueMask,
                            DescriptorSetAllocator& _DescrSetAllocator)noexcept :
        Set              {_Set               },
        Pool             {_Pool              },
        CmdQueueMask     {_CmdQueueMask      },
        DescrSetAllocator{&_DescrSetAllocator}
    {}
    DescriptorSetAllocation()noexcept{}

    DescriptorSetAllocation             (const DescriptorSetAllocation&) = delete;
    DescriptorSetAllocation& operator = (const DescriptorSetAllocation&) = delete;

    DescriptorSetAllocation(DescriptorSetAllocation&& rhs)noexcept : 
        Set              {rhs.Set              },
        Pool             {rhs.Pool             },
        CmdQueueMask     {rhs.CmdQueueMask     },
        DescrSetAllocator{rhs.DescrSetAllocator}
    {
        rhs.Reset();
    }
    // clang-format on

    DescriptorSetAllocation& operator=(DescriptorSetAllocation&& rhs) noexcept
    {
        Release();

        Set               = rhs.Set;
        CmdQueueMask      = rhs.CmdQueueMask;
        Pool              = rhs.Pool;
        DescrSetAllocator = rhs.DescrSetAllocator;

        rhs.Reset();

        return *this;
    }

    operator bool() const
    {
        return Set != VK_NULL_HANDLE;
    }

    void Reset()
    {
        Set               = VK_NULL_HANDLE;
        Pool              = VK_NULL_HANDLE;
        CmdQueueMask      = 0;
        DescrSetAllocator = nullptr;
    }

    void Release();

    ~DescriptorSetAllocation()
    {
        Release();
    }

    VkDescriptorSet GetVkDescriptorSet() const { return Set; }

private:
    VkDescriptorSet         Set               = VK_NULL_HANDLE;
    VkDescriptorPool        Pool              = VK_NULL_HANDLE;
    Uint64                  CmdQueueMask      = 0;
    DescriptorSetAllocator* DescrSetAllocator = nullptr;
};


// The class manages pool of descriptor set pools
//      ______________________________
//     |                              |
//     |     DescriptorPoolManager    |
//     |                              |
//     |   | Pool[0] | Pool[1] | ...  |
//     |______________________________|
//             |            A
//   GetPool() |            | FreePool()
//             V            |
//
class DescriptorPoolManager
{
public:
    // clang-format off
    DescriptorPoolManager(RenderDeviceVkImpl&               DeviceVkImpl,
                          std::string                       PoolName,
                          std::vector<VkDescriptorPoolSize> PoolSizes,
                          uint32_t                          MaxSets,
                          bool                              AllowFreeing) noexcept:
        m_DeviceVkImpl{DeviceVkImpl        },
        m_PoolName    {std::move(PoolName) },
        m_PoolSizes   (std::move(PoolSizes)),
        m_MaxSets     {MaxSets             },
        m_AllowFreeing{AllowFreeing        }
    {
#ifdef DILIGENT_DEVELOPMENT
        m_AllocatedPoolCounter = 0;
#endif
    }
    ~DescriptorPoolManager();

    DescriptorPoolManager             (const DescriptorPoolManager&) = delete;
    DescriptorPoolManager& operator = (const DescriptorPoolManager&) = delete;
    DescriptorPoolManager             (DescriptorPoolManager&&)      = delete;
    DescriptorPoolManager& operator = (DescriptorPoolManager&&)      = delete;
    // clang-format on

    VulkanUtilities::DescriptorPoolWrapper GetPool(const char* DebugName);

    void DisposePool(VulkanUtilities::DescriptorPoolWrapper&& Pool, Uint64 QueueMask);

    RenderDeviceVkImpl& GetDeviceVkImpl() { return m_DeviceVkImpl; }

#ifdef DILIGENT_DEVELOPMENT
    int32_t GetAllocatedPoolCounter() const
    {
        return m_AllocatedPoolCounter;
    }
#endif

protected:
    VulkanUtilities::DescriptorPoolWrapper CreateDescriptorPool(const char* DebugName) const;

    RenderDeviceVkImpl& m_DeviceVkImpl;
    const std::string   m_PoolName;

    const std::vector<VkDescriptorPoolSize> m_PoolSizes;
    const uint32_t                          m_MaxSets;
    const bool                              m_AllowFreeing;

    std::mutex                                         m_Mutex;
    std::deque<VulkanUtilities::DescriptorPoolWrapper> m_Pools;

private:
    void FreePool(VulkanUtilities::DescriptorPoolWrapper&& Pool);

#ifdef DILIGENT_DEVELOPMENT
    std::atomic_int32_t m_AllocatedPoolCounter;
#endif
};


// The class allocates descriptor sets from the main descriptor pool.
// Descriptors sets can be released and returned to the pool
class DescriptorSetAllocator : public DescriptorPoolManager
{
public:
    friend class DescriptorSetAllocation;
    DescriptorSetAllocator(RenderDeviceVkImpl&               DeviceVkImpl,
                           std::string                       PoolName,
                           std::vector<VkDescriptorPoolSize> PoolSizes,
                           uint32_t                          MaxSets,
                           bool                              AllowFreeing) noexcept :
        // clang-format off
        DescriptorPoolManager
        {
            DeviceVkImpl,
            std::move(PoolName),
            std::move(PoolSizes),
            MaxSets,
            AllowFreeing
        }
    // clang-format on
    {
#ifdef DILIGENT_DEVELOPMENT
        m_AllocatedSetCounter = 0;
#endif
    }

    ~DescriptorSetAllocator();

    DescriptorSetAllocation Allocate(Uint64 CommandQueueMask, VkDescriptorSetLayout SetLayout, const char* DebugName = "");

#ifdef DILIGENT_DEVELOPMENT
    int32_t GetAllocatedDescriptorSetCounter() const
    {
        return m_AllocatedSetCounter;
    }
#endif

private:
    void FreeDescriptorSet(VkDescriptorSet Set, VkDescriptorPool Pool, Uint64 QueueMask);

#ifdef DILIGENT_DEVELOPMENT
    std::atomic_int32_t m_AllocatedSetCounter;
#endif
};


// DynamicDescriptorSetAllocator manages dynamic descriptor sets. It first requests descriptor pool from
// the global manager and allocates descriptor sets from this pool. When space in the pool is exhausted,
// the class requests a new pool.
// The class is not thread-safe as device contexts must not be used in multiple threads simultaneously.
// All allocated pools are recycled at the end of every frame.
//   ____________________________________________________________________________
//  |                                                                            |
//  |                           DynamicDescriptorSetAllocator                    |
//  |                                                                            |
//  |  || DescriptorPool[0] | DescriptorPool[1] |  ...   | DescriptorPool[N] ||  |
//  |__________|_________________________________________________________________|
//             |                          A                   |
//             |                          |                   |
//             |Allocate()       GetPool()|                   |FreePool()
//             |                     _____|___________________V____
//             V                    |                              |
//       VkDescriptorSet            |     DescriptorPoolManager    |
//                                  |                              |
//                                  |   |Global dynamic buffer|    |
//                                  |______________________________|
//
class DynamicDescriptorSetAllocator
{
public:
    DynamicDescriptorSetAllocator(DescriptorPoolManager& PoolMgr, std::string Name) :
        // clang-format off
        m_GlobalPoolMgr{PoolMgr        },
        m_Name         {std::move(Name)}
    // clang-format on
    {}
    ~DynamicDescriptorSetAllocator();

    VkDescriptorSet Allocate(VkDescriptorSetLayout SetLayout, const char* DebugName);

    // Releases all allocated pools that are later returned to the global pool manager.
    // As global pool manager is hosted by the render device, the allocator can
    // be destroyed before the pools are actually returned to the global pool manager.
    void ReleasePools(Uint64 QueueMask);

    size_t GetAllocatedPoolCount() const { return m_AllocatedPools.size(); }

private:
    DescriptorPoolManager&                              m_GlobalPoolMgr;
    const std::string                                   m_Name;
    std::vector<VulkanUtilities::DescriptorPoolWrapper> m_AllocatedPools;
    size_t                                              m_PeakPoolCount = 0;
};

} // namespace Diligent
