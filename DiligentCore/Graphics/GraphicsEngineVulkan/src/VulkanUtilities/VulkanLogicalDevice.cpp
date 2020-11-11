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

#include <limits>
#include "VulkanErrors.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanDebug.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"

namespace VulkanUtilities
{

std::shared_ptr<VulkanLogicalDevice> VulkanLogicalDevice::Create(VkPhysicalDevice             vkPhysicalDevice,
                                                                 const VkDeviceCreateInfo&    DeviceCI,
                                                                 const VkAllocationCallbacks* vkAllocator)
{
    auto* LogicalDevice = new VulkanLogicalDevice{vkPhysicalDevice, DeviceCI, vkAllocator};
    return std::shared_ptr<VulkanLogicalDevice>{LogicalDevice};
}

VulkanLogicalDevice::~VulkanLogicalDevice()
{
    vkDestroyDevice(m_VkDevice, m_VkAllocator);
}

VulkanLogicalDevice::VulkanLogicalDevice(VkPhysicalDevice             vkPhysicalDevice,
                                         const VkDeviceCreateInfo&    DeviceCI,
                                         const VkAllocationCallbacks* vkAllocator) :
    m_VkAllocator{vkAllocator},
    m_EnabledFeatures{*DeviceCI.pEnabledFeatures}
{
    auto res = vkCreateDevice(vkPhysicalDevice, &DeviceCI, vkAllocator, &m_VkDevice);
    CHECK_VK_ERROR_AND_THROW(res, "Failed to create logical device");

#if DILIGENT_USE_VOLK
    // Since we only use one device at this time, load device function entries
    // https://github.com/zeux/volk#optimizing-device-calls
    volkLoadDevice(m_VkDevice);
#endif

    m_EnabledGraphicsShaderStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    if (DeviceCI.pEnabledFeatures->geometryShader)
        m_EnabledGraphicsShaderStages |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    if (DeviceCI.pEnabledFeatures->tessellationShader)
        m_EnabledGraphicsShaderStages |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
}

VkQueue VulkanLogicalDevice::GetQueue(uint32_t queueFamilyIndex, uint32_t queueIndex)
{
    VkQueue vkQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_VkDevice,
                     queueFamilyIndex, // Index of the queue family to which the queue belongs
                     0,                // Index within this queue family of the queue to retrieve
                     &vkQueue);
    VERIFY_EXPR(vkQueue != VK_NULL_HANDLE);
    return vkQueue;
}

void VulkanLogicalDevice::WaitIdle() const
{
    auto err = vkDeviceWaitIdle(m_VkDevice);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to idle device");
    (void)err;
}

template <typename VkObjectType,
          VulkanHandleTypeId VkTypeId,
          typename VkCreateObjectFuncType,
          typename VkObjectCreateInfoType>
VulkanObjectWrapper<VkObjectType, VkTypeId> VulkanLogicalDevice::CreateVulkanObject(VkCreateObjectFuncType        VkCreateObject,
                                                                                    const VkObjectCreateInfoType& CreateInfo,
                                                                                    const char*                   DebugName,
                                                                                    const char*                   ObjectType) const
{
    if (DebugName == nullptr)
        DebugName = "";

    VkObjectType VkObject = VK_NULL_HANDLE;

    auto err = VkCreateObject(m_VkDevice, &CreateInfo, m_VkAllocator, &VkObject);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to create Vulkan ", ObjectType, " '", DebugName, '\'');

    if (*DebugName != 0)
        SetVulkanObjectName<VkObjectType, VkTypeId>(m_VkDevice, VkObject, DebugName);

    return VulkanObjectWrapper<VkObjectType, VkTypeId>{GetSharedPtr(), std::move(VkObject)};
}

CommandPoolWrapper VulkanLogicalDevice::CreateCommandPool(const VkCommandPoolCreateInfo& CmdPoolCI,
                                                          const char*                    DebugName) const
{
    VERIFY_EXPR(CmdPoolCI.sType == VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    return CreateVulkanObject<VkCommandPool, VulkanHandleTypeId::CommandPool>(vkCreateCommandPool, CmdPoolCI, DebugName, "command pool");
}

BufferWrapper VulkanLogicalDevice::CreateBuffer(const VkBufferCreateInfo& BufferCI,
                                                const char*               DebugName) const
{
    VERIFY_EXPR(BufferCI.sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    return CreateVulkanObject<VkBuffer, VulkanHandleTypeId::Buffer>(vkCreateBuffer, BufferCI, DebugName, "buffer");
}

BufferViewWrapper VulkanLogicalDevice::CreateBufferView(const VkBufferViewCreateInfo& BuffViewCI,
                                                        const char*                   DebugName) const
{
    VERIFY_EXPR(BuffViewCI.sType == VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
    return CreateVulkanObject<VkBufferView, VulkanHandleTypeId::BufferView>(vkCreateBufferView, BuffViewCI, DebugName, "buffer view");
}

ImageWrapper VulkanLogicalDevice::CreateImage(const VkImageCreateInfo& ImageCI,
                                              const char*              DebugName) const
{
    VERIFY_EXPR(ImageCI.sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
    return CreateVulkanObject<VkImage, VulkanHandleTypeId::Image>(vkCreateImage, ImageCI, DebugName, "image");
}

ImageViewWrapper VulkanLogicalDevice::CreateImageView(const VkImageViewCreateInfo& ImageViewCI,
                                                      const char*                  DebugName) const
{
    VERIFY_EXPR(ImageViewCI.sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
    return CreateVulkanObject<VkImageView, VulkanHandleTypeId::ImageView>(vkCreateImageView, ImageViewCI, DebugName, "image view");
}

SamplerWrapper VulkanLogicalDevice::CreateSampler(const VkSamplerCreateInfo& SamplerCI, const char* DebugName) const
{
    VERIFY_EXPR(SamplerCI.sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
    return CreateVulkanObject<VkSampler, VulkanHandleTypeId::Sampler>(vkCreateSampler, SamplerCI, DebugName, "sampler");
}

FenceWrapper VulkanLogicalDevice::CreateFence(const VkFenceCreateInfo& FenceCI, const char* DebugName) const
{
    VERIFY_EXPR(FenceCI.sType == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    return CreateVulkanObject<VkFence, VulkanHandleTypeId::Fence>(vkCreateFence, FenceCI, DebugName, "fence");
}

RenderPassWrapper VulkanLogicalDevice::CreateRenderPass(const VkRenderPassCreateInfo& RenderPassCI, const char* DebugName) const
{
    VERIFY_EXPR(RenderPassCI.sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
    return CreateVulkanObject<VkRenderPass, VulkanHandleTypeId::RenderPass>(vkCreateRenderPass, RenderPassCI, DebugName, "render pass");
}

DeviceMemoryWrapper VulkanLogicalDevice::AllocateDeviceMemory(const VkMemoryAllocateInfo& AllocInfo,
                                                              const char*                 DebugName) const
{
    VERIFY_EXPR(AllocInfo.sType == VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);

    if (DebugName == nullptr)
        DebugName = "";

    VkDeviceMemory vkDeviceMem = VK_NULL_HANDLE;

    auto err = vkAllocateMemory(m_VkDevice, &AllocInfo, m_VkAllocator, &vkDeviceMem);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to allocate device memory '", DebugName, '\'');

    if (*DebugName != 0)
        SetDeviceMemoryName(m_VkDevice, vkDeviceMem, DebugName);

    return DeviceMemoryWrapper{GetSharedPtr(), std::move(vkDeviceMem)};
}

PipelineWrapper VulkanLogicalDevice::CreateComputePipeline(const VkComputePipelineCreateInfo& PipelineCI,
                                                           VkPipelineCache                    cache,
                                                           const char*                        DebugName) const
{
    VERIFY_EXPR(PipelineCI.sType == VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);

    if (DebugName == nullptr)
        DebugName = "";

    VkPipeline vkPipeline = VK_NULL_HANDLE;

    auto err = vkCreateComputePipelines(m_VkDevice, cache, 1, &PipelineCI, m_VkAllocator, &vkPipeline);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to create compute pipeline '", DebugName, '\'');

    if (*DebugName != 0)
        SetPipelineName(m_VkDevice, vkPipeline, DebugName);

    return PipelineWrapper{GetSharedPtr(), std::move(vkPipeline)};
}

PipelineWrapper VulkanLogicalDevice::CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo& PipelineCI,
                                                            VkPipelineCache                     cache,
                                                            const char*                         DebugName) const
{
    VERIFY_EXPR(PipelineCI.sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

    if (DebugName == nullptr)
        DebugName = "";

    VkPipeline vkPipeline = VK_NULL_HANDLE;

    auto err = vkCreateGraphicsPipelines(m_VkDevice, cache, 1, &PipelineCI, m_VkAllocator, &vkPipeline);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to create graphics pipeline '", DebugName, '\'');

    if (*DebugName != 0)
        SetPipelineName(m_VkDevice, vkPipeline, DebugName);

    return PipelineWrapper{GetSharedPtr(), std::move(vkPipeline)};
}

ShaderModuleWrapper VulkanLogicalDevice::CreateShaderModule(const VkShaderModuleCreateInfo& ShaderModuleCI, const char* DebugName) const
{
    VERIFY_EXPR(ShaderModuleCI.sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    return CreateVulkanObject<VkShaderModule, VulkanHandleTypeId::ShaderModule>(vkCreateShaderModule, ShaderModuleCI, DebugName, "shader module");
}

PipelineLayoutWrapper VulkanLogicalDevice::CreatePipelineLayout(const VkPipelineLayoutCreateInfo& PipelineLayoutCI, const char* DebugName) const
{
    VERIFY_EXPR(PipelineLayoutCI.sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    return CreateVulkanObject<VkPipelineLayout, VulkanHandleTypeId::PipelineLayout>(vkCreatePipelineLayout, PipelineLayoutCI, DebugName, "pipeline layout");
}

FramebufferWrapper VulkanLogicalDevice::CreateFramebuffer(const VkFramebufferCreateInfo& FramebufferCI, const char* DebugName) const
{
    VERIFY_EXPR(FramebufferCI.sType == VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
    return CreateVulkanObject<VkFramebuffer, VulkanHandleTypeId::Framebuffer>(vkCreateFramebuffer, FramebufferCI, DebugName, "framebuffer");
}

DescriptorPoolWrapper VulkanLogicalDevice::CreateDescriptorPool(const VkDescriptorPoolCreateInfo& DescrPoolCI, const char* DebugName) const
{
    VERIFY_EXPR(DescrPoolCI.sType == VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
    return CreateVulkanObject<VkDescriptorPool, VulkanHandleTypeId::DescriptorPool>(vkCreateDescriptorPool, DescrPoolCI, DebugName, "descriptor pool");
}

DescriptorSetLayoutWrapper VulkanLogicalDevice::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& LayoutCI, const char* DebugName) const
{
    VERIFY_EXPR(LayoutCI.sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    return CreateVulkanObject<VkDescriptorSetLayout, VulkanHandleTypeId::DescriptorSetLayout>(vkCreateDescriptorSetLayout, LayoutCI, DebugName, "descriptor set layout");
}

SemaphoreWrapper VulkanLogicalDevice::CreateSemaphore(const VkSemaphoreCreateInfo& SemaphoreCI, const char* DebugName) const
{
    VERIFY_EXPR(SemaphoreCI.sType == VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    return CreateVulkanObject<VkSemaphore, VulkanHandleTypeId::Semaphore>(vkCreateSemaphore, SemaphoreCI, DebugName, "semaphore");
}

QueryPoolWrapper VulkanLogicalDevice::CreateQueryPool(const VkQueryPoolCreateInfo& QueryPoolCI, const char* DebugName) const
{
    VERIFY_EXPR(QueryPoolCI.sType == VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
    return CreateVulkanObject<VkQueryPool, VulkanHandleTypeId::QueryPool>(vkCreateQueryPool, QueryPoolCI, DebugName, "query pool");
}

VkCommandBuffer VulkanLogicalDevice::AllocateVkCommandBuffer(const VkCommandBufferAllocateInfo& AllocInfo, const char* DebugName) const
{
    VERIFY_EXPR(AllocInfo.sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);

    if (DebugName == nullptr)
        DebugName = "";

    VkCommandBuffer CmdBuff = VK_NULL_HANDLE;

    auto err = vkAllocateCommandBuffers(m_VkDevice, &AllocInfo, &CmdBuff);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to allocate command buffer '", DebugName, '\'');
    (void)err;

    if (*DebugName != 0)
        SetCommandBufferName(m_VkDevice, CmdBuff, DebugName);

    return CmdBuff;
}

VkDescriptorSet VulkanLogicalDevice::AllocateVkDescriptorSet(const VkDescriptorSetAllocateInfo& AllocInfo, const char* DebugName) const
{
    VERIFY_EXPR(AllocInfo.sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
    VERIFY_EXPR(AllocInfo.descriptorSetCount == 1);

    if (DebugName == nullptr)
        DebugName = "";

    VkDescriptorSet DescrSet = VK_NULL_HANDLE;

    auto err = vkAllocateDescriptorSets(m_VkDevice, &AllocInfo, &DescrSet);
    if (err != VK_SUCCESS)
        return VK_NULL_HANDLE;

    if (*DebugName != 0)
        SetDescriptorSetName(m_VkDevice, DescrSet, DebugName);

    return DescrSet;
}

void VulkanLogicalDevice::ReleaseVulkanObject(CommandPoolWrapper&& CmdPool) const
{
    vkDestroyCommandPool(m_VkDevice, CmdPool.m_VkObject, m_VkAllocator);
    CmdPool.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(BufferWrapper&& Buffer) const
{
    vkDestroyBuffer(m_VkDevice, Buffer.m_VkObject, m_VkAllocator);
    Buffer.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(BufferViewWrapper&& BufferView) const
{
    vkDestroyBufferView(m_VkDevice, BufferView.m_VkObject, m_VkAllocator);
    BufferView.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(ImageWrapper&& Image) const
{
    vkDestroyImage(m_VkDevice, Image.m_VkObject, m_VkAllocator);
    Image.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(ImageViewWrapper&& ImageView) const
{
    vkDestroyImageView(m_VkDevice, ImageView.m_VkObject, m_VkAllocator);
    ImageView.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(SamplerWrapper&& Sampler) const
{
    vkDestroySampler(m_VkDevice, Sampler.m_VkObject, m_VkAllocator);
    Sampler.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(FenceWrapper&& Fence) const
{
    vkDestroyFence(m_VkDevice, Fence.m_VkObject, m_VkAllocator);
    Fence.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(RenderPassWrapper&& RenderPass) const
{
    vkDestroyRenderPass(m_VkDevice, RenderPass.m_VkObject, m_VkAllocator);
    RenderPass.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(DeviceMemoryWrapper&& Memory) const
{
    vkFreeMemory(m_VkDevice, Memory.m_VkObject, m_VkAllocator);
    Memory.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(PipelineWrapper&& Pipeline) const
{
    vkDestroyPipeline(m_VkDevice, Pipeline.m_VkObject, m_VkAllocator);
    Pipeline.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(ShaderModuleWrapper&& ShaderModule) const
{
    vkDestroyShaderModule(m_VkDevice, ShaderModule.m_VkObject, m_VkAllocator);
    ShaderModule.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(PipelineLayoutWrapper&& PipelineLayout) const
{
    vkDestroyPipelineLayout(m_VkDevice, PipelineLayout.m_VkObject, m_VkAllocator);
    PipelineLayout.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(FramebufferWrapper&& Framebuffer) const
{
    vkDestroyFramebuffer(m_VkDevice, Framebuffer.m_VkObject, m_VkAllocator);
    Framebuffer.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(DescriptorPoolWrapper&& DescriptorPool) const
{
    vkDestroyDescriptorPool(m_VkDevice, DescriptorPool.m_VkObject, m_VkAllocator);
    DescriptorPool.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(DescriptorSetLayoutWrapper&& DescriptorSetLayout) const
{
    vkDestroyDescriptorSetLayout(m_VkDevice, DescriptorSetLayout.m_VkObject, m_VkAllocator);
    DescriptorSetLayout.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(SemaphoreWrapper&& Semaphore) const
{
    vkDestroySemaphore(m_VkDevice, Semaphore.m_VkObject, m_VkAllocator);
    Semaphore.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::ReleaseVulkanObject(QueryPoolWrapper&& QueryPool) const
{
    vkDestroyQueryPool(m_VkDevice, QueryPool.m_VkObject, m_VkAllocator);
    QueryPool.m_VkObject = VK_NULL_HANDLE;
}

void VulkanLogicalDevice::FreeDescriptorSet(VkDescriptorPool Pool, VkDescriptorSet Set) const
{
    VERIFY_EXPR(Pool != VK_NULL_HANDLE && Set != VK_NULL_HANDLE);
    vkFreeDescriptorSets(m_VkDevice, Pool, 1, &Set);
}




VkMemoryRequirements VulkanLogicalDevice::GetBufferMemoryRequirements(VkBuffer vkBuffer) const
{
    VkMemoryRequirements MemReqs = {};
    vkGetBufferMemoryRequirements(m_VkDevice, vkBuffer, &MemReqs);
    return MemReqs;
}

VkMemoryRequirements VulkanLogicalDevice::GetImageMemoryRequirements(VkImage vkImage) const
{
    VkMemoryRequirements MemReqs = {};
    vkGetImageMemoryRequirements(m_VkDevice, vkImage, &MemReqs);
    return MemReqs;
}

VkResult VulkanLogicalDevice::BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) const
{
    return vkBindBufferMemory(m_VkDevice, buffer, memory, memoryOffset);
}

VkResult VulkanLogicalDevice::BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) const
{
    return vkBindImageMemory(m_VkDevice, image, memory, memoryOffset);
}

VkResult VulkanLogicalDevice::MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) const
{
    return vkMapMemory(m_VkDevice, memory, offset, size, flags, ppData);
}

void VulkanLogicalDevice::UnmapMemory(VkDeviceMemory memory) const
{
    vkUnmapMemory(m_VkDevice, memory);
}

VkResult VulkanLogicalDevice::InvalidateMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const
{
    return vkInvalidateMappedMemoryRanges(m_VkDevice, memoryRangeCount, pMemoryRanges);
}

VkResult VulkanLogicalDevice::FlushMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const
{
    return vkFlushMappedMemoryRanges(m_VkDevice, memoryRangeCount, pMemoryRanges);
}

VkResult VulkanLogicalDevice::GetFenceStatus(VkFence fence) const
{
    return vkGetFenceStatus(m_VkDevice, fence);
}

VkResult VulkanLogicalDevice::ResetFence(VkFence fence) const
{
    auto err = vkResetFences(m_VkDevice, 1, &fence);
    DEV_CHECK_ERR(err == VK_SUCCESS, "vkResetFences() failed");
    return err;
}

VkResult VulkanLogicalDevice::WaitForFences(uint32_t       fenceCount,
                                            const VkFence* pFences,
                                            VkBool32       waitAll,
                                            uint64_t       timeout) const
{
    return vkWaitForFences(m_VkDevice, fenceCount, pFences, waitAll, timeout);
}

void VulkanLogicalDevice::UpdateDescriptorSets(uint32_t                    descriptorWriteCount,
                                               const VkWriteDescriptorSet* pDescriptorWrites,
                                               uint32_t                    descriptorCopyCount,
                                               const VkCopyDescriptorSet*  pDescriptorCopies) const
{
    vkUpdateDescriptorSets(m_VkDevice, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VkResult VulkanLogicalDevice::ResetCommandPool(VkCommandPool           vkCmdPool,
                                               VkCommandPoolResetFlags flags) const
{
    auto err = vkResetCommandPool(m_VkDevice, vkCmdPool, flags);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to reset command pool");
    return err;
}

VkResult VulkanLogicalDevice::ResetDescriptorPool(VkDescriptorPool           vkDescriptorPool,
                                                  VkDescriptorPoolResetFlags flags) const
{
    auto err = vkResetDescriptorPool(m_VkDevice, vkDescriptorPool, flags);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to reset descriptor pool");
    return err;
}

} // namespace VulkanUtilities
