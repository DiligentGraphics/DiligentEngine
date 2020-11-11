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

#include <mutex>
#include <array>
#include <deque>
#include <vector>

#include "Query.h"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanPhysicalDevice.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace Diligent
{

class RenderDeviceVkImpl;

class QueryManagerVk
{
public:
    QueryManagerVk(RenderDeviceVkImpl* RenderDeviceVk,
                   const Uint32        QueryHeapSizes[]);
    ~QueryManagerVk();

    // clang-format off
    QueryManagerVk             (const QueryManagerVk&)  = delete;
    QueryManagerVk             (      QueryManagerVk&&) = delete;
    QueryManagerVk& operator = (const QueryManagerVk&)  = delete;
    QueryManagerVk& operator = (      QueryManagerVk&&) = delete;
    // clang-format on

    static constexpr Uint32 InvalidIndex = static_cast<Uint32>(-1);

    Uint32 AllocateQuery(QUERY_TYPE Type);
    void   DiscardQuery(QUERY_TYPE Type, Uint32 Index);

    VkQueryPool GetQueryPool(QUERY_TYPE Type)
    {
        return m_Heaps[Type].vkQueryPool;
    }

    Uint64 GetCounterFrequency() const
    {
        return m_CounterFrequency;
    }

    Uint32 ResetStaleQueries(VulkanUtilities::VulkanCommandBuffer& CmdBuff);

private:
    struct QueryHeapInfo
    {
        VulkanUtilities::QueryPoolWrapper vkQueryPool;

        std::deque<Uint32>  AvailableQueries;
        std::vector<Uint32> StaleQueries;

        Uint32 PoolSize            = 0;
        Uint32 MaxAllocatedQueries = 0;
    };

    std::mutex                                      m_HeapMutex;
    std::array<QueryHeapInfo, QUERY_TYPE_NUM_TYPES> m_Heaps;

    Uint64 m_CounterFrequency = 0;
};

} // namespace Diligent
