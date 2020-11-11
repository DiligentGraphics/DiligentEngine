/*
* Vulkan examples debug wrapper
* 
* Appendix for VK_EXT_Debug_Report can be found at https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_report/doc/specs/vulkan/appendices/debug_report.txt
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <sstream>
#include <cstring>

#include "VulkanUtilities/VulkanDebug.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "Errors.hpp"
#include "DebugUtilities.hpp"

namespace VulkanUtilities
{

static PFN_vkCreateDebugUtilsMessengerEXT  CreateDebugUtilsMessengerEXT  = nullptr;
static PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT = nullptr;
static PFN_vkSetDebugUtilsObjectNameEXT    SetDebugUtilsObjectNameEXT    = nullptr;
static PFN_vkSetDebugUtilsObjectTagEXT     SetDebugUtilsObjectTagEXT     = nullptr;
static PFN_vkQueueBeginDebugUtilsLabelEXT  QueueBeginDebugUtilsLabelEXT  = nullptr;
static PFN_vkQueueEndDebugUtilsLabelEXT    QueueEndDebugUtilsLabelEXT    = nullptr;
static PFN_vkQueueInsertDebugUtilsLabelEXT QueueInsertDebugUtilsLabelEXT = nullptr;
static PFN_vkCmdBeginDebugUtilsLabelEXT    CmdBeginDebugUtilsLabelEXT    = nullptr;
static PFN_vkCmdEndDebugUtilsLabelEXT      CmdEndDebugUtilsLabelEXT      = nullptr;
static PFN_vkCmdInsertDebugUtilsLabelEXT   CmdInsertDebugUtilsLabelEXT   = nullptr;

static VkDebugUtilsMessengerEXT DbgMessenger = VK_NULL_HANDLE;

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                      void*                                       userData)
{
    std::stringstream debugMessage;

    // Ignore some validation messages
    if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    {
        {
            // UNASSIGNED-CoreValidation-DrawState-ClearCmdBeforeDraw:
            // vkCmdClearAttachments() issued on command buffer object 0x... prior to any Draw Cmds. It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw.
            static const char* ClearCmdBeforeDrawIdName             = "UNASSIGNED-CoreValidation-DrawState-ClearCmdBeforeDraw";
            constexpr int32_t  DeprecatedClearCmdBeforeDrawIdNumber = 64; // Starting with sdk version 1.1.85, messageIdNumber is 0 for all messages
            if ((callbackData->pMessageIdName != nullptr && strcmp(callbackData->pMessageIdName, ClearCmdBeforeDrawIdName) == 0) || callbackData->messageIdNumber == DeprecatedClearCmdBeforeDrawIdNumber)
                return VK_FALSE;
        }
    }

    Diligent::DEBUG_MESSAGE_SEVERITY MsgSeverity = Diligent::DEBUG_MESSAGE_SEVERITY_INFO;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        MsgSeverity = Diligent::DEBUG_MESSAGE_SEVERITY_INFO;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        MsgSeverity = Diligent::DEBUG_MESSAGE_SEVERITY_INFO;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        MsgSeverity = Diligent::DEBUG_MESSAGE_SEVERITY_WARNING;
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        MsgSeverity = Diligent::DEBUG_MESSAGE_SEVERITY_ERROR;
    }
    else
    {
        MsgSeverity = Diligent::DEBUG_MESSAGE_SEVERITY_INFO;
    }

    debugMessage << "Vulkan debug message (";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        debugMessage << "general";
        if (messageType & (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
            debugMessage << ", ";
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        debugMessage << "validation";
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            debugMessage << ", ";
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        debugMessage << "performance";
    debugMessage << "): ";

    // callbackData->messageIdNumber is deprecated and starting with version 1.1.85 it is always 0
    debugMessage << (callbackData->pMessageIdName != nullptr ? callbackData->pMessageIdName : "<Unknown name>");
    if (callbackData->pMessage != nullptr)
    {
        debugMessage << std::endl
                     << "                 " << callbackData->pMessage;
    }

    if (callbackData->objectCount > 0)
    {
        for (uint32_t obj = 0; obj < callbackData->objectCount; ++obj)
        {
            const auto& Object = callbackData->pObjects[obj];
            debugMessage << std::endl
                         << "                 Object[" << obj << "] (" << VkObjectTypeToString(Object.objectType)
                         << "): Handle " << std::hex << "0x" << Object.objectHandle;
            if (Object.pObjectName != nullptr)
            {
                debugMessage << ", Name: '" << Object.pObjectName << '\'';
            }
        }
    }

    if (callbackData->cmdBufLabelCount > 0)
    {
        for (uint32_t l = 0; l < callbackData->cmdBufLabelCount; ++l)
        {
            const auto& Label = callbackData->pCmdBufLabels[l];
            debugMessage << std::endl
                         << "                 Label[" << l << "]";
            if (Label.pLabelName != nullptr)
            {
                debugMessage << " - " << Label.pLabelName;
            }
            debugMessage << " {";
            debugMessage << std::fixed << std::setw(4) << Label.color[0] << ", "
                         << std::fixed << std::setw(4) << Label.color[1] << ", "
                         << std::fixed << std::setw(4) << Label.color[2] << ", "
                         << std::fixed << std::setw(4) << Label.color[3] << "}";
        }
    }

    LOG_DEBUG_MESSAGE(MsgSeverity, debugMessage.str().c_str());

    // The return value of this callback controls wether the Vulkan call that caused
    // the validation message will be aborted or not
    // We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message
    // (and return a VkResult) to abort
    return VK_FALSE;
}


void SetupDebugging(VkInstance                          instance,
                    VkDebugUtilsMessageSeverityFlagsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT     messageType,
                    void*                               pUserData)
{
    CreateDebugUtilsMessengerEXT  = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    VERIFY_EXPR(CreateDebugUtilsMessengerEXT != nullptr && DestroyDebugUtilsMessengerEXT != nullptr);

    VkDebugUtilsMessengerCreateInfoEXT DbgMessenger_CI = {};

    DbgMessenger_CI.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    DbgMessenger_CI.pNext           = NULL;
    DbgMessenger_CI.flags           = 0;
    DbgMessenger_CI.messageSeverity = messageSeverity;
    DbgMessenger_CI.messageType     = messageType;
    DbgMessenger_CI.pfnUserCallback = DebugMessengerCallback;
    DbgMessenger_CI.pUserData       = pUserData;

    VkResult err = CreateDebugUtilsMessengerEXT(instance, &DbgMessenger_CI, nullptr, &DbgMessenger);
    VERIFY(err == VK_SUCCESS, "Failed to create debug utils messenger");
    (void)err;

    // Load function pointers
    SetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
    VERIFY_EXPR(SetDebugUtilsObjectNameEXT != nullptr);
    SetDebugUtilsObjectTagEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT"));
    VERIFY_EXPR(SetDebugUtilsObjectTagEXT != nullptr);

    QueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT"));
    VERIFY_EXPR(QueueBeginDebugUtilsLabelEXT != nullptr);
    QueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT"));
    VERIFY_EXPR(QueueEndDebugUtilsLabelEXT != nullptr);
    QueueInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT"));
    VERIFY_EXPR(QueueInsertDebugUtilsLabelEXT != nullptr);

    CmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
    VERIFY_EXPR(CmdBeginDebugUtilsLabelEXT != nullptr);
    CmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
    VERIFY_EXPR(CmdEndDebugUtilsLabelEXT != nullptr);
    CmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
    VERIFY_EXPR(CmdInsertDebugUtilsLabelEXT != nullptr);
}

void FreeDebugging(VkInstance instance)
{
    if (DbgMessenger != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessengerEXT(instance, DbgMessenger, nullptr);
    }
}

void BeginCmdQueueLabelRegion(VkQueue cmdQueue, const char* pLabelName, const float* color)
{
    VkDebugUtilsLabelEXT Label = {};

    Label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    Label.pNext      = nullptr;
    Label.pLabelName = pLabelName;
    for (int i = 0; i < 4; ++i)
        Label.color[i] = color[i];
    QueueBeginDebugUtilsLabelEXT(cmdQueue, &Label);
}

void InsertCmdQueueLabel(VkQueue cmdQueue, const char* pLabelName, const float* color)
{
    VkDebugUtilsLabelEXT Label = {};

    Label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    Label.pNext      = nullptr;
    Label.pLabelName = pLabelName;
    for (int i = 0; i < 4; ++i)
        Label.color[i] = color[i];
    QueueInsertDebugUtilsLabelEXT(cmdQueue, &Label);
}

void EndCmdQueueLabelRegion(VkQueue cmdQueue)
{
    QueueEndDebugUtilsLabelEXT(cmdQueue);
}


// Start a new label region
void BeginCmdBufferLabelRegion(VkCommandBuffer cmdBuffer, const char* pLabelName, const float* color)
{
    VkDebugUtilsLabelEXT Label = {};

    Label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    Label.pNext      = nullptr;
    Label.pLabelName = pLabelName;
    for (int i = 0; i < 4; ++i)
        Label.color[i] = color[i];
    CmdBeginDebugUtilsLabelEXT(cmdBuffer, &Label);
}

// End the label region
void EndCmdBufferLabelRegion(VkCommandBuffer cmdBuffer)
{
    CmdEndDebugUtilsLabelEXT(cmdBuffer);
}

// Start a single label
void InsertCmdBufferLabel(VkCommandBuffer cmdBuffer, const char* pLabelName, const float* color)
{
    VkDebugUtilsLabelEXT Label = {};

    Label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    Label.pNext      = nullptr;
    Label.pLabelName = pLabelName;
    for (int i = 0; i < 4; ++i)
        Label.color[i] = color[i];
    CmdInsertDebugUtilsLabelEXT(cmdBuffer, &Label);
}


void SetObjectName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const char* name)
{
    // Check for valid function pointer (may not be present if not running in a debugging application)
    if (SetDebugUtilsObjectNameEXT != nullptr && name != nullptr && *name != 0)
    {
        VkDebugUtilsObjectNameInfoEXT ObjectNameInfo = {};

        ObjectNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        ObjectNameInfo.pNext        = nullptr;
        ObjectNameInfo.objectType   = objectType;
        ObjectNameInfo.objectHandle = objectHandle;
        ObjectNameInfo.pObjectName  = name;

        VkResult res = SetDebugUtilsObjectNameEXT(device, &ObjectNameInfo);
        VERIFY_EXPR(res == VK_SUCCESS);
        (void)res;
    }
}

void SetObjectTag(VkDevice device, uint64_t objectHandle, VkObjectType objectType, uint64_t name, size_t tagSize, const void* tag)
{
    // Check for valid function pointer (may not be present if not running in a debugging application)
    if (SetDebugUtilsObjectTagEXT)
    {
        VkDebugUtilsObjectTagInfoEXT tagInfo = {};

        tagInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
        tagInfo.pNext        = nullptr;
        tagInfo.objectType   = objectType;
        tagInfo.objectHandle = objectHandle;
        tagInfo.tagName      = name;
        tagInfo.tagSize      = tagSize;
        tagInfo.pTag         = tag;
        SetDebugUtilsObjectTagEXT(device, &tagInfo);
    }
}

void SetCommandPoolName(VkDevice device, VkCommandPool cmdPool, const char* name)
{
    SetObjectName(device, (uint64_t)cmdPool, VK_OBJECT_TYPE_COMMAND_POOL, name);
}

void SetCommandBufferName(VkDevice device, VkCommandBuffer cmdBuffer, const char* name)
{
    SetObjectName(device, (uint64_t)cmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
}

void SetQueueName(VkDevice device, VkQueue queue, const char* name)
{
    SetObjectName(device, (uint64_t)queue, VK_OBJECT_TYPE_QUEUE, name);
}

void SetImageName(VkDevice device, VkImage image, const char* name)
{
    SetObjectName(device, (uint64_t)image, VK_OBJECT_TYPE_IMAGE, name);
}

void SetImageViewName(VkDevice device, VkImageView imageView, const char* name)
{
    SetObjectName(device, (uint64_t)imageView, VK_OBJECT_TYPE_IMAGE_VIEW, name);
}

void SetSamplerName(VkDevice device, VkSampler sampler, const char* name)
{
    SetObjectName(device, (uint64_t)sampler, VK_OBJECT_TYPE_SAMPLER, name);
}

void SetBufferName(VkDevice device, VkBuffer buffer, const char* name)
{
    SetObjectName(device, (uint64_t)buffer, VK_OBJECT_TYPE_BUFFER, name);
}

void SetBufferViewName(VkDevice device, VkBufferView bufferView, const char* name)
{
    SetObjectName(device, (uint64_t)bufferView, VK_OBJECT_TYPE_BUFFER_VIEW, name);
}

void SetDeviceMemoryName(VkDevice device, VkDeviceMemory memory, const char* name)
{
    SetObjectName(device, (uint64_t)memory, VK_OBJECT_TYPE_DEVICE_MEMORY, name);
}

void SetShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char* name)
{
    SetObjectName(device, (uint64_t)shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, name);
}

void SetPipelineName(VkDevice device, VkPipeline pipeline, const char* name)
{
    SetObjectName(device, (uint64_t)pipeline, VK_OBJECT_TYPE_PIPELINE, name);
}

void SetPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char* name)
{
    SetObjectName(device, (uint64_t)pipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
}

void SetRenderPassName(VkDevice device, VkRenderPass renderPass, const char* name)
{
    SetObjectName(device, (uint64_t)renderPass, VK_OBJECT_TYPE_RENDER_PASS, name);
}

void SetFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char* name)
{
    SetObjectName(device, (uint64_t)framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, name);
}

void SetDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char* name)
{
    SetObjectName(device, (uint64_t)descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
}

void SetDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char* name)
{
    SetObjectName(device, (uint64_t)descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
}

void SetDescriptorPoolName(VkDevice device, VkDescriptorPool descriptorPool, const char* name)
{
    SetObjectName(device, (uint64_t)descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, name);
}

void SetSemaphoreName(VkDevice device, VkSemaphore semaphore, const char* name)
{
    SetObjectName(device, (uint64_t)semaphore, VK_OBJECT_TYPE_SEMAPHORE, name);
}

void SetFenceName(VkDevice device, VkFence fence, const char* name)
{
    SetObjectName(device, (uint64_t)fence, VK_OBJECT_TYPE_FENCE, name);
}

void SetEventName(VkDevice device, VkEvent _event, const char* name)
{
    SetObjectName(device, (uint64_t)_event, VK_OBJECT_TYPE_EVENT, name);
}

void SetQueryPoolName(VkDevice device, VkQueryPool queryPool, const char* name)
{
    SetObjectName(device, (uint64_t)queryPool, VK_OBJECT_TYPE_QUERY_POOL, name);
}


template <>
void SetVulkanObjectName<VkCommandPool, VulkanHandleTypeId::CommandPool>(VkDevice device, VkCommandPool cmdPool, const char* name)
{
    SetCommandPoolName(device, cmdPool, name);
}

template <>
void SetVulkanObjectName<VkCommandBuffer, VulkanHandleTypeId::CommandBuffer>(VkDevice device, VkCommandBuffer cmdBuffer, const char* name)
{
    SetCommandBufferName(device, cmdBuffer, name);
}

template <>
void SetVulkanObjectName<VkQueue, VulkanHandleTypeId::Queue>(VkDevice device, VkQueue queue, const char* name)
{
    SetQueueName(device, queue, name);
}

template <>
void SetVulkanObjectName<VkImage, VulkanHandleTypeId::Image>(VkDevice device, VkImage image, const char* name)
{
    SetImageName(device, image, name);
}

template <>
void SetVulkanObjectName<VkImageView, VulkanHandleTypeId::ImageView>(VkDevice device, VkImageView imageView, const char* name)
{
    SetImageViewName(device, imageView, name);
}

template <>
void SetVulkanObjectName<VkSampler, VulkanHandleTypeId::Sampler>(VkDevice device, VkSampler sampler, const char* name)
{
    SetSamplerName(device, sampler, name);
}

template <>
void SetVulkanObjectName<VkBuffer, VulkanHandleTypeId::Buffer>(VkDevice device, VkBuffer buffer, const char* name)
{
    SetBufferName(device, buffer, name);
}

template <>
void SetVulkanObjectName<VkBufferView, VulkanHandleTypeId::BufferView>(VkDevice device, VkBufferView bufferView, const char* name)
{
    SetBufferViewName(device, bufferView, name);
}

template <>
void SetVulkanObjectName<VkDeviceMemory, VulkanHandleTypeId::DeviceMemory>(VkDevice device, VkDeviceMemory memory, const char* name)
{
    SetDeviceMemoryName(device, memory, name);
}

template <>
void SetVulkanObjectName<VkShaderModule, VulkanHandleTypeId::ShaderModule>(VkDevice device, VkShaderModule shaderModule, const char* name)
{
    SetShaderModuleName(device, shaderModule, name);
}

template <>
void SetVulkanObjectName<VkPipeline, VulkanHandleTypeId::Pipeline>(VkDevice device, VkPipeline pipeline, const char* name)
{
    SetPipelineName(device, pipeline, name);
}

template <>
void SetVulkanObjectName<VkPipelineLayout, VulkanHandleTypeId::PipelineLayout>(VkDevice device, VkPipelineLayout pipelineLayout, const char* name)
{
    SetPipelineLayoutName(device, pipelineLayout, name);
}

template <>
void SetVulkanObjectName<VkRenderPass, VulkanHandleTypeId::RenderPass>(VkDevice device, VkRenderPass renderPass, const char* name)
{
    SetRenderPassName(device, renderPass, name);
}

template <>
void SetVulkanObjectName<VkFramebuffer, VulkanHandleTypeId::Framebuffer>(VkDevice device, VkFramebuffer framebuffer, const char* name)
{
    SetFramebufferName(device, framebuffer, name);
}

template <>
void SetVulkanObjectName<VkDescriptorSetLayout, VulkanHandleTypeId::DescriptorSetLayout>(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char* name)
{
    SetDescriptorSetLayoutName(device, descriptorSetLayout, name);
}

template <>
void SetVulkanObjectName<VkDescriptorSet, VulkanHandleTypeId::DescriptorSet>(VkDevice device, VkDescriptorSet descriptorSet, const char* name)
{
    SetDescriptorSetName(device, descriptorSet, name);
}

template <>
void SetVulkanObjectName<VkDescriptorPool, VulkanHandleTypeId::DescriptorPool>(VkDevice device, VkDescriptorPool descriptorPool, const char* name)
{
    SetDescriptorPoolName(device, descriptorPool, name);
}

template <>
void SetVulkanObjectName<VkSemaphore, VulkanHandleTypeId::Semaphore>(VkDevice device, VkSemaphore semaphore, const char* name)
{
    SetSemaphoreName(device, semaphore, name);
}

template <>
void SetVulkanObjectName<VkFence, VulkanHandleTypeId::Fence>(VkDevice device, VkFence fence, const char* name)
{
    SetFenceName(device, fence, name);
}

template <>
void SetVulkanObjectName<VkEvent, VulkanHandleTypeId::Event>(VkDevice device, VkEvent _event, const char* name)
{
    SetEventName(device, _event, name);
}

template <>
void SetVulkanObjectName<VkQueryPool, VulkanHandleTypeId::QueryPool>(VkDevice device, VkQueryPool queryPool, const char* name)
{
    SetQueryPoolName(device, queryPool, name);
}



const char* VkResultToString(VkResult errorCode)
{
    switch (errorCode)
    {
        // clang-format off
#define STR(r) case VK_ ##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
            // clang-format on
        default:
            return "UNKNOWN_ERROR";
    }
}

const char* VkAccessFlagBitToString(VkAccessFlagBits Bit)
{
    VERIFY(Bit != 0 && (Bit & (Bit - 1)) == 0, "Single bit is expected");
    switch (Bit)
    {
        // clang-format off
#define ACCESS_FLAG_BIT_TO_STRING(ACCESS_FLAG_BIT)case ACCESS_FLAG_BIT: return #ACCESS_FLAG_BIT;
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_INDEX_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_UNIFORM_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_SHADER_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_SHADER_WRITE_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_TRANSFER_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_TRANSFER_WRITE_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_HOST_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_HOST_WRITE_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_MEMORY_READ_BIT)
        ACCESS_FLAG_BIT_TO_STRING(VK_ACCESS_MEMORY_WRITE_BIT)
#undef ACCESS_FLAG_BIT_TO_STRING
        default: UNEXPECTED("Unexpected bit"); return "";
            // clang-format on
    }
}

const char* VkImageLayoutToString(VkImageLayout Layout)
{
    switch (Layout)
    {
        // clang-format off
#define IMAGE_LAYOUT_TO_STRING(LAYOUT)case LAYOUT: return #LAYOUT;
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_UNDEFINED)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_GENERAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_PREINITIALIZED)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        IMAGE_LAYOUT_TO_STRING(VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR)
#undef IMAGE_LAYOUT_TO_STRING
        default: UNEXPECTED("Unknown layout"); return "";
            // clang-format on
    }
}

std::string VkAccessFlagsToString(VkAccessFlags Flags)
{
    std::string FlagsString;
    while (Flags != 0)
    {
        auto Bit = Flags & ~(Flags - 1);
        if (!FlagsString.empty())
            FlagsString += ", ";
        FlagsString += VkAccessFlagBitToString(static_cast<VkAccessFlagBits>(Bit));
        Flags = Flags & (Flags - 1);
    }
    return std::move(FlagsString);
}

const char* VkObjectTypeToString(VkObjectType ObjectType)
{
    switch (ObjectType)
    {
        // clang-format off
        case VK_OBJECT_TYPE_UNKNOWN:                        return "unknown";
        case VK_OBJECT_TYPE_INSTANCE:                       return "instance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:                return "physical device";
        case VK_OBJECT_TYPE_DEVICE:                         return "device";
        case VK_OBJECT_TYPE_QUEUE:                          return "queue";
        case VK_OBJECT_TYPE_SEMAPHORE:                      return "semaphore";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:                 return "cmd buffer";
        case VK_OBJECT_TYPE_FENCE:                          return "fence";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:                  return "memory";
        case VK_OBJECT_TYPE_BUFFER:                         return "buffer";
        case VK_OBJECT_TYPE_IMAGE:                          return "image";
        case VK_OBJECT_TYPE_EVENT:                          return "event";
        case VK_OBJECT_TYPE_QUERY_POOL:                     return "query pool";
        case VK_OBJECT_TYPE_BUFFER_VIEW:                    return "buffer view";
        case VK_OBJECT_TYPE_IMAGE_VIEW:                     return "image view";
        case VK_OBJECT_TYPE_SHADER_MODULE:                  return "shader module";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:                 return "pipeline cache";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:                return "pipeline layout";
        case VK_OBJECT_TYPE_RENDER_PASS:                    return "render pass";
        case VK_OBJECT_TYPE_PIPELINE:                       return "pipeline";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:          return "descriptor set layout";
        case VK_OBJECT_TYPE_SAMPLER:                        return "sampler";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:                return "descriptor pool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:                 return "descriptor set";
        case VK_OBJECT_TYPE_FRAMEBUFFER:                    return "framebuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL:                   return "command pool";
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:       return "sampler ycbcr conversion";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:     return "descriptor update template";
        case VK_OBJECT_TYPE_SURFACE_KHR:                    return "surface KHR";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:                  return "swapchain KHR";
        case VK_OBJECT_TYPE_DISPLAY_KHR:                    return "display KHR";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:               return "display mode KHR";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:      return "debug report callback";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:      return "debug utils messenger";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:     return "acceleration structure KHR";
        case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:           return "validation cache EXT";
        case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:return "performance configuration INTEL";
        case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:         return "deferred operation KHR";
        case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:    return "indirect commands layout NV";
        case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT:          return "private data slot EXT";
        default: return "unknown";
            // clang-format on
    }
}

} // namespace VulkanUtilities
