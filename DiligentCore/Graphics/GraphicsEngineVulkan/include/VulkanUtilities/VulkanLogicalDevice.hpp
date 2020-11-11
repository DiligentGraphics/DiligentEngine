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

#include <memory>
#include "VulkanHeaders.h"

namespace VulkanUtilities
{

// In 32-bit version, all Vulkan handles are typedefed as uint64_t, so we have to
// use VulkanHandleTypeId to distinguish objects.
enum class VulkanHandleTypeId : uint32_t
{
    CommandPool,
    CommandBuffer,
    Buffer,
    BufferView,
    Image,
    ImageView,
    DeviceMemory,
    Fence,
    RenderPass,
    Pipeline,
    ShaderModule,
    PipelineLayout,
    Sampler,
    Framebuffer,
    DescriptorPool,
    DescriptorSetLayout,
    DescriptorSet,
    Semaphore,
    Queue,
    Event,
    QueryPool
};

template <typename VulkanObjectType, VulkanHandleTypeId>
class VulkanObjectWrapper;

#define DEFINE_VULKAN_OBJECT_WRAPPER(Type) VulkanObjectWrapper<Vk##Type, VulkanHandleTypeId::Type>
using CommandPoolWrapper         = DEFINE_VULKAN_OBJECT_WRAPPER(CommandPool);
using BufferWrapper              = DEFINE_VULKAN_OBJECT_WRAPPER(Buffer);
using BufferViewWrapper          = DEFINE_VULKAN_OBJECT_WRAPPER(BufferView);
using ImageWrapper               = DEFINE_VULKAN_OBJECT_WRAPPER(Image);
using ImageViewWrapper           = DEFINE_VULKAN_OBJECT_WRAPPER(ImageView);
using DeviceMemoryWrapper        = DEFINE_VULKAN_OBJECT_WRAPPER(DeviceMemory);
using FenceWrapper               = DEFINE_VULKAN_OBJECT_WRAPPER(Fence);
using RenderPassWrapper          = DEFINE_VULKAN_OBJECT_WRAPPER(RenderPass);
using PipelineWrapper            = DEFINE_VULKAN_OBJECT_WRAPPER(Pipeline);
using ShaderModuleWrapper        = DEFINE_VULKAN_OBJECT_WRAPPER(ShaderModule);
using PipelineLayoutWrapper      = DEFINE_VULKAN_OBJECT_WRAPPER(PipelineLayout);
using SamplerWrapper             = DEFINE_VULKAN_OBJECT_WRAPPER(Sampler);
using FramebufferWrapper         = DEFINE_VULKAN_OBJECT_WRAPPER(Framebuffer);
using DescriptorPoolWrapper      = DEFINE_VULKAN_OBJECT_WRAPPER(DescriptorPool);
using DescriptorSetLayoutWrapper = DEFINE_VULKAN_OBJECT_WRAPPER(DescriptorSetLayout);
using SemaphoreWrapper           = DEFINE_VULKAN_OBJECT_WRAPPER(Semaphore);
using QueryPoolWrapper           = DEFINE_VULKAN_OBJECT_WRAPPER(QueryPool);
#undef DEFINE_VULKAN_OBJECT_WRAPPER

class VulkanLogicalDevice : public std::enable_shared_from_this<VulkanLogicalDevice>
{
public:
    static std::shared_ptr<VulkanLogicalDevice> Create(VkPhysicalDevice             vkPhysicalDevice,
                                                       const VkDeviceCreateInfo&    DeviceCI,
                                                       const VkAllocationCallbacks* vkAllocator);

    // clang-format off
    VulkanLogicalDevice             (const VulkanLogicalDevice&) = delete;
    VulkanLogicalDevice             (VulkanLogicalDevice&&)      = delete;
    VulkanLogicalDevice& operator = (const VulkanLogicalDevice&) = delete;
    VulkanLogicalDevice& operator = (VulkanLogicalDevice&&)      = delete;
    // clang-format on

    ~VulkanLogicalDevice();

    std::shared_ptr<VulkanLogicalDevice> GetSharedPtr()
    {
        return shared_from_this();
    }

    std::shared_ptr<const VulkanLogicalDevice> GetSharedPtr() const
    {
        return shared_from_this();
    }

    VkQueue GetQueue(uint32_t queueFamilyIndex, uint32_t queueIndex);

    VkDevice GetVkDevice() const
    {
        return m_VkDevice;
    }

    void WaitIdle() const;

    // clang-format off
    CommandPoolWrapper  CreateCommandPool   (const VkCommandPoolCreateInfo& CmdPoolCI,   const char* DebugName = "") const;
    BufferWrapper       CreateBuffer        (const VkBufferCreateInfo&      BufferCI,    const char* DebugName = "") const;
    BufferViewWrapper   CreateBufferView    (const VkBufferViewCreateInfo&  BuffViewCI,  const char* DebugName = "") const;
    ImageWrapper        CreateImage         (const VkImageCreateInfo&       ImageCI,     const char* DebugName = "") const;
    ImageViewWrapper    CreateImageView     (const VkImageViewCreateInfo&   ImageViewCI, const char* DebugName = "") const;
    SamplerWrapper      CreateSampler       (const VkSamplerCreateInfo&     SamplerCI,   const char* DebugName = "") const;
    FenceWrapper        CreateFence         (const VkFenceCreateInfo&       FenceCI,     const char* DebugName = "") const;
    RenderPassWrapper   CreateRenderPass    (const VkRenderPassCreateInfo&  RenderPassCI,const char* DebugName = "") const;
    DeviceMemoryWrapper AllocateDeviceMemory(const VkMemoryAllocateInfo &   AllocInfo,   const char* DebugName = "") const;

    PipelineWrapper     CreateComputePipeline (const VkComputePipelineCreateInfo&   PipelineCI, VkPipelineCache cache, const char* DebugName = "") const;
    PipelineWrapper     CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo&  PipelineCI, VkPipelineCache cache, const char* DebugName = "") const;

    ShaderModuleWrapper        CreateShaderModule       (const VkShaderModuleCreateInfo&        ShaderModuleCI, const char* DebugName = "") const;
    PipelineLayoutWrapper      CreatePipelineLayout     (const VkPipelineLayoutCreateInfo&      LayoutCI,       const char* DebugName = "") const;
    FramebufferWrapper         CreateFramebuffer        (const VkFramebufferCreateInfo&         FramebufferCI,  const char* DebugName = "") const;
    DescriptorPoolWrapper      CreateDescriptorPool     (const VkDescriptorPoolCreateInfo&      DescrPoolCI,    const char* DebugName = "") const;
    DescriptorSetLayoutWrapper CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& LayoutCI,       const char* DebugName = "") const;

    SemaphoreWrapper    CreateSemaphore(const VkSemaphoreCreateInfo& SemaphoreCI, const char* DebugName = "") const;
    QueryPoolWrapper    CreateQueryPool(const VkQueryPoolCreateInfo& QueryPoolCI, const char* DebugName = "") const;

    VkCommandBuffer     AllocateVkCommandBuffer(const VkCommandBufferAllocateInfo& AllocInfo, const char* DebugName = "") const;
    VkDescriptorSet     AllocateVkDescriptorSet(const VkDescriptorSetAllocateInfo& AllocInfo, const char* DebugName = "") const;

    void ReleaseVulkanObject(CommandPoolWrapper&&  CmdPool) const;
    void ReleaseVulkanObject(BufferWrapper&&       Buffer) const;
    void ReleaseVulkanObject(BufferViewWrapper&&   BufferView) const;
    void ReleaseVulkanObject(ImageWrapper&&        Image) const;
    void ReleaseVulkanObject(ImageViewWrapper&&    ImageView) const;
    void ReleaseVulkanObject(SamplerWrapper&&      Sampler) const;
    void ReleaseVulkanObject(FenceWrapper&&        Fence) const;
    void ReleaseVulkanObject(RenderPassWrapper&&   RenderPass) const;
    void ReleaseVulkanObject(DeviceMemoryWrapper&& Memory) const;
    void ReleaseVulkanObject(PipelineWrapper&&     Pipeline) const;
    void ReleaseVulkanObject(ShaderModuleWrapper&& ShaderModule) const;
    void ReleaseVulkanObject(PipelineLayoutWrapper&& PipelineLayout) const;
    void ReleaseVulkanObject(FramebufferWrapper&&   Framebuffer) const;
    void ReleaseVulkanObject(DescriptorPoolWrapper&& DescriptorPool) const;
    void ReleaseVulkanObject(DescriptorSetLayoutWrapper&& DescriptorSetLayout) const;
    void ReleaseVulkanObject(SemaphoreWrapper&&     Semaphore) const;
    void ReleaseVulkanObject(QueryPoolWrapper&&     QueryPool) const;

    void FreeDescriptorSet(VkDescriptorPool Pool, VkDescriptorSet Set) const;

    VkMemoryRequirements GetBufferMemoryRequirements(VkBuffer vkBuffer) const;
    VkMemoryRequirements GetImageMemoryRequirements (VkImage vkImage  ) const;

    VkResult BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) const;
    VkResult BindImageMemory (VkImage image,   VkDeviceMemory memory, VkDeviceSize memoryOffset) const;
    // clang-format on

    VkResult MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) const;
    void     UnmapMemory(VkDeviceMemory memory) const;

    VkResult InvalidateMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const;
    VkResult FlushMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const;

    VkResult GetFenceStatus(VkFence fence) const;
    VkResult ResetFence(VkFence fence) const;
    VkResult WaitForFences(uint32_t       fenceCount,
                           const VkFence* pFences,
                           VkBool32       waitAll,
                           uint64_t       timeout) const;

    void UpdateDescriptorSets(uint32_t                    descriptorWriteCount,
                              const VkWriteDescriptorSet* pDescriptorWrites,
                              uint32_t                    descriptorCopyCount,
                              const VkCopyDescriptorSet*  pDescriptorCopies) const;

    VkResult ResetCommandPool(VkCommandPool           vkCmdPool,
                              VkCommandPoolResetFlags flags = 0) const;

    VkResult ResetDescriptorPool(VkDescriptorPool           descriptorPool,
                                 VkDescriptorPoolResetFlags flags = 0) const;

    VkResult GetQueryPoolResults(VkQueryPool        queryPool,
                                 uint32_t           firstQuery,
                                 uint32_t           queryCount,
                                 size_t             dataSize,
                                 void*              pData,
                                 VkDeviceSize       stride,
                                 VkQueryResultFlags flags) const
    {
        return vkGetQueryPoolResults(m_VkDevice, queryPool, firstQuery, queryCount,
                                     dataSize, pData, stride, flags);
    }

    VkPipelineStageFlags GetEnabledGraphicsShaderStages() const { return m_EnabledGraphicsShaderStages; }

    const VkPhysicalDeviceFeatures& GetEnabledFeatures() const { return m_EnabledFeatures; }

private:
    VulkanLogicalDevice(VkPhysicalDevice             vkPhysicalDevice,
                        const VkDeviceCreateInfo&    DeviceCI,
                        const VkAllocationCallbacks* vkAllocator);

    template <typename VkObjectType,
              VulkanHandleTypeId VkTypeId,
              typename VkCreateObjectFuncType,
              typename VkObjectCreateInfoType>
    VulkanObjectWrapper<VkObjectType, VkTypeId> CreateVulkanObject(VkCreateObjectFuncType        VkCreateObject,
                                                                   const VkObjectCreateInfoType& CreateInfo,
                                                                   const char*                   DebugName,
                                                                   const char*                   ObjectType) const;

    VkDevice                           m_VkDevice = VK_NULL_HANDLE;
    const VkAllocationCallbacks* const m_VkAllocator;
    VkPipelineStageFlags               m_EnabledGraphicsShaderStages = 0;
    const VkPhysicalDeviceFeatures     m_EnabledFeatures;
};

} // namespace VulkanUtilities
