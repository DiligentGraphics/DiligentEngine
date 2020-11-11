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
 *  In no event and under no legal theory, whether in tort (including neVkigence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly neVkigent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include "pch.h"

#include "FenceVkImpl.hpp"
#include "EngineMemory.h"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

FenceVkImpl::FenceVkImpl(IReferenceCounters* pRefCounters,
                         RenderDeviceVkImpl* pRendeDeviceVkImpl,
                         const FenceDesc&    Desc,
                         bool                IsDeviceInternal) :
    // clang-format off
    TFenceBase
    {
        pRefCounters,
        pRendeDeviceVkImpl,
        Desc,
        IsDeviceInternal
    },
    m_FencePool{pRendeDeviceVkImpl->GetLogicalDevice().GetSharedPtr()}
// clang-format on
{
}

FenceVkImpl::~FenceVkImpl()
{
    if (!m_PendingFences.empty())
    {
        LOG_INFO_MESSAGE("FenceVkImpl::~FenceVkImpl(): waiting for ", m_PendingFences.size(), " pending Vulkan ",
                         (m_PendingFences.size() > 1 ? "fences." : "fence."));
        // Vulkan spec states that all queue submission commands that refer to
        // a fence must have completed execution before the fence is destroyed.
        // (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-vkDestroyFence-fence-01120)
        Wait(UINT64_MAX);
    }
}

Uint64 FenceVkImpl::GetCompletedValue()
{
    const auto& LogicalDevice = m_pDevice->GetLogicalDevice();
    while (!m_PendingFences.empty())
    {
        auto& Value_Fence = m_PendingFences.front();

        auto status = LogicalDevice.GetFenceStatus(Value_Fence.second);
        if (status == VK_SUCCESS)
        {
            if (Value_Fence.first > m_LastCompletedFenceValue)
                m_LastCompletedFenceValue = Value_Fence.first;
            m_FencePool.DisposeFence(std::move(Value_Fence.second));
            m_PendingFences.pop_front();
        }
        else
        {
            break;
        }
    }

    return m_LastCompletedFenceValue;
}

void FenceVkImpl::Reset(Uint64 Value)
{
    DEV_CHECK_ERR(Value >= m_LastCompletedFenceValue, "Resetting fence '", m_Desc.Name, "' to the value (", Value, ") that is smaller than the last completed value (", m_LastCompletedFenceValue, ")");
    if (Value > m_LastCompletedFenceValue)
        m_LastCompletedFenceValue = Value;
}


void FenceVkImpl::Wait(Uint64 Value)
{
    const auto& LogicalDevice = m_pDevice->GetLogicalDevice();
    while (!m_PendingFences.empty())
    {
        auto& val_fence = m_PendingFences.front();
        if (val_fence.first > Value)
            break;

        auto status = LogicalDevice.GetFenceStatus(val_fence.second);
        if (status == VK_NOT_READY)
        {
            VkFence FenceToWait = val_fence.second;

            status = LogicalDevice.WaitForFences(1, &FenceToWait, VK_TRUE, UINT64_MAX);
        }

        DEV_CHECK_ERR(status == VK_SUCCESS, "All pending fences must now be complete!");
        (void)status;
        if (val_fence.first > m_LastCompletedFenceValue)
            m_LastCompletedFenceValue = val_fence.first;
        m_FencePool.DisposeFence(std::move(val_fence.second));

        m_PendingFences.pop_front();
    }
}

} // namespace Diligent
