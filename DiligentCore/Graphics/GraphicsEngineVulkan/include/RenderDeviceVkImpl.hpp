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
/// Declaration of Diligent::RenderDeviceVkImpl class
#include <memory>

#include "RenderDeviceVk.h"
#include "RenderDeviceBase.hpp"
#include "RenderDeviceNextGenBase.hpp"
#include "DescriptorPoolManager.hpp"
#include "VulkanDynamicHeap.hpp"
#include "Atomics.hpp"
#include "CommandQueueVk.h"
#include "VulkanUtilities/VulkanInstance.hpp"
#include "VulkanUtilities/VulkanPhysicalDevice.hpp"
#include "VulkanUtilities/VulkanCommandBufferPool.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanMemoryManager.hpp"
#include "VulkanUploadHeap.hpp"
#include "FramebufferCache.hpp"
#include "RenderPassCache.hpp"
#include "CommandPoolManager.hpp"
#include "DXCompiler.hpp"

namespace Diligent
{

/// Render device implementation in Vulkan backend.
class RenderDeviceVkImpl final : public RenderDeviceNextGenBase<RenderDeviceBase<IRenderDeviceVk>, ICommandQueueVk>
{
public:
    using TRenderDeviceBase = RenderDeviceNextGenBase<RenderDeviceBase<IRenderDeviceVk>, ICommandQueueVk>;

    RenderDeviceVkImpl(IReferenceCounters*                                    pRefCounters,
                       IMemoryAllocator&                                      RawMemAllocator,
                       IEngineFactory*                                        pEngineFactory,
                       const EngineVkCreateInfo&                              EngineCI,
                       size_t                                                 CommandQueueCount,
                       ICommandQueueVk**                                      pCmdQueues,
                       std::shared_ptr<VulkanUtilities::VulkanInstance>       Instance,
                       std::unique_ptr<VulkanUtilities::VulkanPhysicalDevice> PhysicalDevice,
                       std::shared_ptr<VulkanUtilities::VulkanLogicalDevice>  LogicalDevice) noexcept(false);
    ~RenderDeviceVkImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IRenderDevice::CreateGraphicsPipelineState() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateComputePipelineState() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo, IPipelineState** ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateBuffer() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateBuffer(const BufferDesc& BuffDesc,
                                                 const BufferData* pBuffData,
                                                 IBuffer**         ppBuffer) override final;

    /// Implementation of IRenderDevice::CreateShader() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateShader(const ShaderCreateInfo& ShaderCreateInfo, IShader** ppShader) override final;

    /// Implementation of IRenderDevice::CreateTexture() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture(const TextureDesc& TexDesc,
                                                  const TextureData* pData,
                                                  ITexture**         ppTexture) override final;

    void CreateTexture(const TextureDesc& TexDesc, VkImage vkImgHandle, RESOURCE_STATE InitialState, class TextureVkImpl** ppTexture);

    /// Implementation of IRenderDevice::CreateSampler() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler) override final;

    /// Implementation of IRenderDevice::CreateFence() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateFence(const FenceDesc& Desc, IFence** ppFence) override final;

    /// Implementation of IRenderDevice::CreateQuery() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateQuery(const QueryDesc& Desc, IQuery** ppQuery) override final;

    /// Implementation of IRenderDevice::CreateRenderPass() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateRenderPass(const RenderPassDesc& Desc,
                                                     IRenderPass**         ppRenderPass) override final;

    void CreateRenderPass(const RenderPassDesc& Desc,
                          IRenderPass**         ppRenderPass,
                          bool                  IsDeviceInternal);


    /// Implementation of IRenderDevice::CreateFramebuffer() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE CreateFramebuffer(const FramebufferDesc& Desc,
                                                      IFramebuffer**         ppFramebuffer) override final;

    /// Implementation of IRenderDeviceVk::GetVkDevice().
    virtual VkDevice DILIGENT_CALL_TYPE GetVkDevice() override final { return m_LogicalVkDevice->GetVkDevice(); }

    /// Implementation of IRenderDeviceVk::GetVkPhysicalDevice().
    virtual VkPhysicalDevice DILIGENT_CALL_TYPE GetVkPhysicalDevice() override final { return m_PhysicalDevice->GetVkDeviceHandle(); }

    /// Implementation of IRenderDeviceVk::GetVkInstance().
    virtual VkInstance DILIGENT_CALL_TYPE GetVkInstance() override final { return m_VulkanInstance->GetVkInstance(); }

    /// Implementation of IRenderDeviceVk::CreateTextureFromVulkanImage().
    virtual void DILIGENT_CALL_TYPE CreateTextureFromVulkanImage(VkImage            vkImage,
                                                                 const TextureDesc& TexDesc,
                                                                 RESOURCE_STATE     InitialState,
                                                                 ITexture**         ppTexture) override final;

    /// Implementation of IRenderDeviceVk::CreateBufferFromVulkanResource().
    virtual void DILIGENT_CALL_TYPE CreateBufferFromVulkanResource(VkBuffer          vkBuffer,
                                                                   const BufferDesc& BuffDesc,
                                                                   RESOURCE_STATE    InitialState,
                                                                   IBuffer**         ppBuffer) override final;

    /// Implementation of IRenderDevice::IdleGPU() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE IdleGPU() override final;

    // pImmediateCtx parameter is only used to make sure the command buffer is submitted from the immediate context
    // The method returns fence value associated with the submitted command buffer
    Uint64 ExecuteCommandBuffer(Uint32 QueueIndex, const VkSubmitInfo& SubmitInfo, class DeviceContextVkImpl* pImmediateCtx, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>* pSignalFences);

    void AllocateTransientCmdPool(VulkanUtilities::CommandPoolWrapper& CmdPool, VkCommandBuffer& vkCmdBuff, const Char* DebugPoolName = nullptr);
    void ExecuteAndDisposeTransientCmdBuff(Uint32 QueueIndex, VkCommandBuffer vkCmdBuff, VulkanUtilities::CommandPoolWrapper&& CmdPool);

    /// Implementation of IRenderDevice::ReleaseStaleResources() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE ReleaseStaleResources(bool ForceRelease = false) override final;

    DescriptorSetAllocation AllocateDescriptorSet(Uint64 CommandQueueMask, VkDescriptorSetLayout SetLayout, const char* DebugName = "")
    {
        return m_DescriptorSetAllocator.Allocate(CommandQueueMask, SetLayout, DebugName);
    }
    DescriptorPoolManager& GetDynamicDescriptorPool() { return m_DynamicDescriptorPool; }

    std::shared_ptr<const VulkanUtilities::VulkanInstance> GetVulkanInstance() const { return m_VulkanInstance; }

    const VulkanUtilities::VulkanPhysicalDevice& GetPhysicalDevice() const { return *m_PhysicalDevice; }
    const VulkanUtilities::VulkanLogicalDevice&  GetLogicalDevice() { return *m_LogicalVkDevice; }

    FramebufferCache& GetFramebufferCache() { return m_FramebufferCache; }
    RenderPassCache&  GetImplicitRenderPassCache() { return m_ImplicitRenderPassCache; }

    VulkanUtilities::VulkanMemoryAllocation AllocateMemory(const VkMemoryRequirements& MemReqs, VkMemoryPropertyFlags MemoryProperties)
    {
        return m_MemoryMgr.Allocate(MemReqs, MemoryProperties);
    }
    VulkanUtilities::VulkanMemoryAllocation AllocateMemory(VkDeviceSize Size, VkDeviceSize Alignment, uint32_t MemoryTypeIndex)
    {
        const auto& MemoryProps = m_PhysicalDevice->GetMemoryProperties();
        VERIFY_EXPR(MemoryTypeIndex < MemoryProps.memoryTypeCount);
        const auto MemoryFlags = MemoryProps.memoryTypes[MemoryTypeIndex].propertyFlags;
        return m_MemoryMgr.Allocate(Size, Alignment, MemoryTypeIndex, (MemoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0);
    }
    VulkanUtilities::VulkanMemoryManager& GetGlobalMemoryManager() { return m_MemoryMgr; }

    VulkanDynamicMemoryManager& GetDynamicMemoryManager() { return m_DynamicMemoryManager; }

    void FlushStaleResources(Uint32 CmdQueueIndex);

    IDXCompiler* GetDxCompiler() const { return m_pDxCompiler.get(); }

private:
    template <typename PSOCreateInfoType>
    void CreatePipelineState(const PSOCreateInfoType& PSOCreateInfo, IPipelineState** ppPipelineState);

    virtual void TestTextureFormat(TEXTURE_FORMAT TexFormat) override final;

    // Submits command buffer for execution to the command queue
    // Returns the submitted command buffer number and the fence value
    // Parameters:
    //      * SubmittedCmdBuffNumber - submitted command buffer number
    //      * SubmittedFenceValue    - fence value associated with the submitted command buffer
    void SubmitCommandBuffer(Uint32 QueueIndex, const VkSubmitInfo& SubmitInfo, Uint64& SubmittedCmdBuffNumber, Uint64& SubmittedFenceValue, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence>>>* pFences);

    std::shared_ptr<VulkanUtilities::VulkanInstance>       m_VulkanInstance;
    std::unique_ptr<VulkanUtilities::VulkanPhysicalDevice> m_PhysicalDevice;
    std::shared_ptr<VulkanUtilities::VulkanLogicalDevice>  m_LogicalVkDevice;

    EngineVkCreateInfo m_EngineAttribs;

    FramebufferCache       m_FramebufferCache;
    RenderPassCache        m_ImplicitRenderPassCache;
    DescriptorSetAllocator m_DescriptorSetAllocator;
    DescriptorPoolManager  m_DynamicDescriptorPool;

    // These one-time command pools are used by buffer and texture constructors to
    // issue copy commands. Vulkan requires that every command pool is used by one thread
    // at a time, so every constructor must allocate command buffer from its own pool.
    CommandPoolManager m_TransientCmdPoolMgr;

    VulkanUtilities::VulkanMemoryManager m_MemoryMgr;

    VulkanDynamicMemoryManager m_DynamicMemoryManager;

    std::unique_ptr<IDXCompiler> m_pDxCompiler;
};

} // namespace Diligent
