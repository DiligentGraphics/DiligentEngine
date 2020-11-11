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

#include "VulkanUtilities/VulkanFencePool.hpp"
#include "Errors.hpp"
#include "DebugUtilities.hpp"

namespace VulkanUtilities
{

VulkanFencePool::VulkanFencePool(std::shared_ptr<const VulkanLogicalDevice> LogicalDevice) noexcept :
    m_LogicalDevice{std::move(LogicalDevice)}
{}

VulkanFencePool::~VulkanFencePool()
{
#ifdef DILIGENT_DEVELOPMENT
    for (const auto& fence : m_Fences)
    {
        DEV_CHECK_ERR(m_LogicalDevice->GetFenceStatus(fence) == VK_SUCCESS, "Destroying a fence that has not been signaled");
    }
#endif
    m_Fences.clear();
}

FenceWrapper VulkanFencePool::GetFence()
{
    FenceWrapper Fence;
    if (!m_Fences.empty())
    {
        Fence = std::move(m_Fences.back());
        m_LogicalDevice->ResetFence(Fence);
        m_Fences.pop_back();
    }
    else
    {
        VkFenceCreateInfo FenceCI = {};

        FenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCI.pNext = nullptr;
        FenceCI.flags = 0; // Available flag: VK_FENCE_CREATE_SIGNALED_BIT

        Fence = m_LogicalDevice->CreateFence(FenceCI);
    }
    return Fence;
}

void VulkanFencePool::DisposeFence(FenceWrapper&& Fence)
{
    DEV_CHECK_ERR(m_LogicalDevice->GetFenceStatus(Fence) == VK_SUCCESS, "Disposing a fence that has not been signaled");
    m_Fences.emplace_back(std::move(Fence));
}

} // namespace VulkanUtilities
