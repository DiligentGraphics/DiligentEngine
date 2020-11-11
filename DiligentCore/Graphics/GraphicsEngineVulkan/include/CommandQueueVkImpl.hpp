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

/// \file
/// Declaration of Diligent::CommandQueueVkImpl class

#include <mutex>
#include <deque>
#include "VulkanUtilities/VulkanHeaders.h"
#include "CommandQueueVk.h"
#include "ObjectBase.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "FenceVkImpl.hpp"

namespace Diligent
{

/// Implementation of the Diligent::ICommandQueueVk interface
class CommandQueueVkImpl final : public ObjectBase<ICommandQueueVk>
{
public:
    using TBase = ObjectBase<ICommandQueueVk>;

    CommandQueueVkImpl(IReferenceCounters*                                   pRefCounters,
                       std::shared_ptr<VulkanUtilities::VulkanLogicalDevice> LogicalDevice,
                       uint32_t                                              QueueFamilyIndex);
    ~CommandQueueVkImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ICommandQueueVk::GetNextFenceValue().
    virtual Uint64 DILIGENT_CALL_TYPE GetNextFenceValue() const override final { return m_NextFenceValue; }

    /// Implementation of ICommandQueueVk::Submit().
    virtual Uint64 DILIGENT_CALL_TYPE SubmitCmdBuffer(VkCommandBuffer cmdBuffer) override final;

    /// Implementation of ICommandQueueVk::Submit().
    virtual Uint64 DILIGENT_CALL_TYPE Submit(const VkSubmitInfo& SubmitInfo) override final;

    /// Implementation of ICommandQueueVk::Present().
    virtual VkResult DILIGENT_CALL_TYPE Present(const VkPresentInfoKHR& PresentInfo) override final;

    /// Implementation of ICommandQueueVk::GetVkQueue().
    virtual VkQueue DILIGENT_CALL_TYPE GetVkQueue() override final { return m_VkQueue; }

    /// Implementation of ICommandQueueVk::GetQueueFamilyIndex().
    virtual uint32_t DILIGENT_CALL_TYPE GetQueueFamilyIndex() const override final { return m_QueueFamilyIndex; }

    /// Implementation of ICommandQueueVk::GetQueueFamilyIndex().
    virtual Uint64 DILIGENT_CALL_TYPE WaitForIdle() override final;

    /// Implementation of ICommandQueueVk::GetCompletedFenceValue().
    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedFenceValue() override final;

    /// Implementation of ICommandQueueVk::SignalFence().
    virtual void DILIGENT_CALL_TYPE SignalFence(VkFence vkFence) override final;

    void SetFence(RefCntAutoPtr<FenceVkImpl> pFence) { m_pFence = std::move(pFence); }

private:
    std::shared_ptr<VulkanUtilities::VulkanLogicalDevice> m_LogicalDevice;

    const VkQueue  m_VkQueue;
    const uint32_t m_QueueFamilyIndex;
    // Fence is signaled right after a command buffer has been
    // submitted to the command queue for execution.
    // All command buffers with fence value less than or equal to the signaled value
    // are guaranteed to be finished by the GPU
    RefCntAutoPtr<FenceVkImpl> m_pFence;

    // A value that will be signaled by the command queue next
    Atomics::AtomicInt64 m_NextFenceValue;

    std::mutex m_QueueMutex;
};

} // namespace Diligent
