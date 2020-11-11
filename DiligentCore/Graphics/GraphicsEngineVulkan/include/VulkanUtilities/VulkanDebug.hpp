#pragma once

#include <string>
#include "VulkanHeaders.h"

namespace VulkanUtilities
{

// clang-format off
// Unified validation layer used on Desktop and Mobile platforms
static constexpr const char* ValidationLayerNames[] = 
{
    "VK_LAYER_KHRONOS_validation"
};
// clang-format on

// Load debug function pointers and set debug callback
// if callBack is NULL, default message callback will be used
void SetupDebugging(VkInstance                          instance,
                    VkDebugUtilsMessageSeverityFlagsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT     messageType,
                    void*                               pUserData = nullptr);
// Clear debug callback
void FreeDebugging(VkInstance instance);

// Setup and functions for the VK_EXT_debug_marker_extension
// Extension spec can be found at https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_marker/doc/specs/vulkan/appendices/VK_EXT_debug_marker.txt
// Note that the extension will only be present if run from an offline debugging application
// The actual check for extension presence and enabling it on the device is done in the example base class
// See VulkanExampleBase::createInstance and VulkanExampleBase::createDevice (base/vulkanexamplebase.cpp)

// Sets the debug name of an object
// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
// along with the object type
void SetObjectName(VkDevice device, uint64_t object, VkObjectType objectType, const char* name);

// Set the tag for an object
void SetObjectTag(VkDevice device, uint64_t objectHandle, VkObjectType objectType, uint64_t name, size_t tagSize, const void* tag);

// Start a new label region
void BeginCmdQueueLabelRegion(VkQueue cmdQueue, const char* pLabelName, const float* color);

// End the label region
void EndCmdQueueLabelRegion(VkQueue cmdQueue);

// Insert a single label
void InsertCmdQueueLabel(VkQueue cmdQueue, const char* pLabelName, const float* color);

// Start a new label region
void BeginCmdBufferLabelRegion(VkCommandBuffer cmdBuffer, const char* pLabelName, const float* color);

// End the label region
void EndCmdBufferLabelRegion(VkCommandBuffer cmdBuffer);

// Insert a single label
void InsertCmdBufferLabel(VkCommandBuffer cmdBuffer, const char* pLabelName, const float* color);

// clang-format off
// Object specific naming functions
void SetCommandPoolName         (VkDevice device, VkCommandPool         cmdPool,             const char * name);
void SetCommandBufferName       (VkDevice device, VkCommandBuffer       cmdBuffer,           const char * name);
void SetQueueName               (VkDevice device, VkQueue               queue,               const char * name);
void SetImageName               (VkDevice device, VkImage               image,               const char * name);
void SetImageViewName           (VkDevice device, VkImageView           imageView,           const char * name);
void SetSamplerName             (VkDevice device, VkSampler             sampler,             const char * name);
void SetBufferName              (VkDevice device, VkBuffer              buffer,              const char * name);
void SetBufferViewName          (VkDevice device, VkBufferView          bufferView,          const char * name);
void SetDeviceMemoryName        (VkDevice device, VkDeviceMemory        memory,              const char * name);
void SetShaderModuleName        (VkDevice device, VkShaderModule        shaderModule,        const char * name);
void SetPipelineName            (VkDevice device, VkPipeline            pipeline,            const char * name);
void SetPipelineLayoutName      (VkDevice device, VkPipelineLayout      pipelineLayout,      const char * name);
void SetRenderPassName          (VkDevice device, VkRenderPass          renderPass,          const char * name);
void SetFramebufferName         (VkDevice device, VkFramebuffer         framebuffer,         const char * name);
void SetDescriptorSetLayoutName (VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char * name);
void SetDescriptorSetName       (VkDevice device, VkDescriptorSet       descriptorSet,       const char * name);
void SetDescriptorPoolName      (VkDevice device, VkDescriptorPool      descriptorPool,      const char * name);
void SetSemaphoreName           (VkDevice device, VkSemaphore           semaphore,           const char * name);
void SetFenceName               (VkDevice device, VkFence               fence,               const char * name);
void SetEventName               (VkDevice device, VkEvent               _event,              const char * name);
void SetQueryPoolName           (VkDevice device, VkQueryPool           queryPool,           const char * name);

enum class VulkanHandleTypeId : uint32_t;

template<typename VulkanObjectType, VulkanHandleTypeId>
void SetVulkanObjectName(VkDevice device, VulkanObjectType vkObject, const char * name);

const char* VkResultToString       (VkResult         errorCode);
const char* VkAccessFlagBitToString(VkAccessFlagBits Bit);
const char* VkImageLayoutToString  (VkImageLayout    Layout);
std::string VkAccessFlagsToString  (VkAccessFlags    Flags);
const char* VkObjectTypeToString   (VkObjectType ObjectType);
// clang-format on

} // namespace VulkanUtilities
