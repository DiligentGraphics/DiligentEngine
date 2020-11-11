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
/// Declaration of Diligent::BufferVkImpl class

#include "BufferVk.h"
#include "RenderDeviceVk.h"
#include "BufferBase.hpp"
#include "BufferViewVkImpl.hpp"
#include "VulkanDynamicHeap.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanMemoryManager.hpp"
#include "STDAllocator.hpp"
#include "RenderDeviceVkImpl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;
class DeviceContextVkImpl;

/// Buffer object implementation in Vulkan backend.
class BufferVkImpl final : public BufferBase<IBufferVk, RenderDeviceVkImpl, BufferViewVkImpl, FixedBlockMemoryAllocator>
{
public:
    using TBufferBase = BufferBase<IBufferVk, RenderDeviceVkImpl, BufferViewVkImpl, FixedBlockMemoryAllocator>;

    BufferVkImpl(IReferenceCounters*        pRefCounters,
                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                 RenderDeviceVkImpl*        pDeviceVk,
                 const BufferDesc&          BuffDesc,
                 const BufferData*          pBuffData = nullptr);

    BufferVkImpl(IReferenceCounters*        pRefCounters,
                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                 class RenderDeviceVkImpl*  pDeviceVk,
                 const BufferDesc&          BuffDesc,
                 RESOURCE_STATE             InitialState,
                 VkBuffer                   vkBuffer);
    ~BufferVkImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

#ifdef DILIGENT_DEVELOPMENT
    void DvpVerifyDynamicAllocation(DeviceContextVkImpl* pCtx) const;
#endif

    Uint32 GetDynamicOffset(Uint32 CtxId, DeviceContextVkImpl* pCtx) const
    {
        if (m_VulkanBuffer != VK_NULL_HANDLE)
        {
            return 0;
        }
        else
        {
            VERIFY(m_Desc.Usage == USAGE_DYNAMIC, "Dynamic buffer is expected");
            VERIFY_EXPR(!m_DynamicAllocations.empty());
#ifdef DILIGENT_DEVELOPMENT
            DvpVerifyDynamicAllocation(pCtx);
#endif
            auto& DynAlloc = m_DynamicAllocations[CtxId];
            return static_cast<Uint32>(DynAlloc.AlignedOffset);
        }
    }

    /// Implementation of IBufferVk::GetVkBuffer().
    virtual VkBuffer DILIGENT_CALL_TYPE GetVkBuffer() const override final;

    /// Implementation of IBuffer::GetNativeHandle() in Vulkan backend.
    virtual void* DILIGENT_CALL_TYPE GetNativeHandle() override final
    {
        auto vkBuffer = GetVkBuffer();
        return reinterpret_cast<void*>(vkBuffer);
    }

    /// Implementation of IBufferVk::SetAccessFlags().
    virtual void DILIGENT_CALL_TYPE SetAccessFlags(VkAccessFlags AccessFlags) override final;

    /// Implementation of IBufferVk::GetAccessFlags().
    virtual VkAccessFlags DILIGENT_CALL_TYPE GetAccessFlags() const override final;

    bool CheckAccessFlags(VkAccessFlags AccessFlags) const
    {
        return (GetAccessFlags() & AccessFlags) == AccessFlags;
    }

    void* GetCPUAddress()
    {
        VERIFY_EXPR(m_Desc.Usage == USAGE_STAGING || m_Desc.Usage == USAGE_UNIFIED);
        return reinterpret_cast<Uint8*>(m_MemoryAllocation.Page->GetCPUMemory()) + m_BufferMemoryAlignedOffset;
    }

private:
    friend class DeviceContextVkImpl;

    virtual void CreateViewInternal(const struct BufferViewDesc& ViewDesc, IBufferView** ppView, bool bIsDefaultView) override;

    VulkanUtilities::BufferViewWrapper CreateView(struct BufferViewDesc& ViewDesc);

    Uint32       m_DynamicOffsetAlignment    = 0;
    VkDeviceSize m_BufferMemoryAlignedOffset = 0;

    std::vector<VulkanDynamicAllocation, STDAllocatorRawMem<VulkanDynamicAllocation>> m_DynamicAllocations;

    VulkanUtilities::BufferWrapper          m_VulkanBuffer;
    VulkanUtilities::VulkanMemoryAllocation m_MemoryAllocation;
};

} // namespace Diligent
