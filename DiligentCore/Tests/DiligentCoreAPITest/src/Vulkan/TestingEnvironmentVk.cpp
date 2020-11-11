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

#include "Vulkan/TestingEnvironmentVk.hpp"

#include "RenderDeviceVk.h"
#include "DeviceContextVk.h"

#include "GLSLangUtils.hpp"

#define VOLK_IMPLEMENTATION
#include "volk/volk.h"

namespace Diligent
{

namespace Testing
{

void CreateTestingSwapChainVk(TestingEnvironmentVk* pEnv,
                              const SwapChainDesc&  SCDesc,
                              ISwapChain**          ppSwapChain);

TestingEnvironmentVk::TestingEnvironmentVk(const CreateInfo&    CI,
                                           const SwapChainDesc& SCDesc) :
    TestingEnvironment{CI, SCDesc}
{
#if !DILIGENT_NO_GLSLANG
    GLSLangUtils::InitializeGlslang();
#endif

    // We have to use dynamic Vulkan loader because if an application is statically linked with vulkan-1.lib
    // and the Vulkan library is not present on the system, the app will instantly crash.
    volkInitialize();

    RefCntAutoPtr<IRenderDeviceVk> pRenderDeviceVk{m_pDevice, IID_RenderDeviceVk};
    m_vkDevice = pRenderDeviceVk->GetVkDevice();

    volkLoadInstance(pRenderDeviceVk->GetVkInstance());

    RefCntAutoPtr<IDeviceContextVk> pContextVk{m_pDeviceContext, IID_DeviceContextVk};

    auto* pQeueVk          = pContextVk->LockCommandQueue();
    auto  QueueFamilyIndex = pQeueVk->GetQueueFamilyIndex();
    pContextVk->UnlockCommandQueue();

    auto vkPhysicalDevice = pRenderDeviceVk->GetVkPhysicalDevice();
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &m_MemoryProperties);

    VkCommandPoolCreateInfo CmdPoolCI = {};
    CmdPoolCI.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolCI.queueFamilyIndex        = QueueFamilyIndex;
    vkCreateCommandPool(m_vkDevice, &CmdPoolCI, nullptr, &m_vkCmdPool);
    VERIFY_EXPR(m_vkCmdPool != VK_NULL_HANDLE);

    if (m_pSwapChain == nullptr)
    {
        CreateTestingSwapChainVk(this, SCDesc, &m_pSwapChain);
    }
}

TestingEnvironmentVk::~TestingEnvironmentVk()
{
    if (m_vkCmdPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_vkDevice, m_vkCmdPool, nullptr);
    }
#if !DILIGENT_NO_GLSLANG
    GLSLangUtils::FinalizeGlslang();
#endif
}

uint32_t TestingEnvironmentVk::GetMemoryTypeIndex(uint32_t              memoryTypeBitsRequirement,
                                                  VkMemoryPropertyFlags requiredProperties) const
{
    // Iterate over all memory types available for the device
    // For each pair of elements X and Y returned in memoryTypes, X must be placed at a lower index position than Y if:
    //   * either the set of bit flags of X is a strict subset of the set of bit flags of Y.
    //   * or the propertyFlags members of X and Y are equal, and X belongs to a memory heap with greater performance

    for (uint32_t memoryIndex = 0; memoryIndex < m_MemoryProperties.memoryTypeCount; memoryIndex++)
    {
        // Each memory type returned by vkGetPhysicalDeviceMemoryProperties must have its propertyFlags set
        // to one of the following values:
        // * 0
        // * HOST_VISIBLE_BIT | HOST_COHERENT_BIT
        // * HOST_VISIBLE_BIT | HOST_CACHED_BIT
        // * HOST_VISIBLE_BIT | HOST_CACHED_BIT | HOST_COHERENT_BIT
        // * DEVICE_LOCAL_BIT
        // * DEVICE_LOCAL_BIT | HOST_VISIBLE_BIT | HOST_COHERENT_BIT
        // * DEVICE_LOCAL_BIT | HOST_VISIBLE_BIT | HOST_CACHED_BIT
        // * DEVICE_LOCAL_BIT | HOST_VISIBLE_BIT | HOST_CACHED_BIT | HOST_COHERENT_BIT
        // * DEVICE_LOCAL_BIT | LAZILY_ALLOCATED_BIT
        //
        // There must be at least one memory type with both the HOST_VISIBLE_BIT and HOST_COHERENT_BIT bits set
        // There must be at least one memory type with the DEVICE_LOCAL_BIT bit set

        const uint32_t memoryTypeBit        = (1 << memoryIndex);
        const bool     isRequiredMemoryType = (memoryTypeBitsRequirement & memoryTypeBit) != 0;
        if (isRequiredMemoryType)
        {
            const VkMemoryPropertyFlags properties            = m_MemoryProperties.memoryTypes[memoryIndex].propertyFlags;
            const bool                  hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

            if (hasRequiredProperties)
                return memoryIndex;
        }
    }
    return static_cast<uint32_t>(-1);
}

void TestingEnvironmentVk::CreateImage2D(uint32_t          Width,
                                         uint32_t          Height,
                                         VkFormat          vkFormat,
                                         VkImageUsageFlags vkUsage,
                                         VkImageLayout     vkInitialLayout,
                                         VkDeviceMemory&   vkMemory,
                                         VkImage&          vkImage)
{
    VkImageCreateInfo ImageCI = {};

    ImageCI.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCI.pNext                 = nullptr;
    ImageCI.flags                 = 0;
    ImageCI.imageType             = VK_IMAGE_TYPE_2D;
    ImageCI.format                = vkFormat;
    ImageCI.extent                = VkExtent3D{Width, Height, 1};
    ImageCI.mipLevels             = 1;
    ImageCI.arrayLayers           = 1;
    ImageCI.samples               = VK_SAMPLE_COUNT_1_BIT;
    ImageCI.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCI.usage                 = vkUsage;
    ImageCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCI.queueFamilyIndexCount = 0;
    ImageCI.pQueueFamilyIndices   = nullptr;
    ImageCI.initialLayout         = vkInitialLayout;

    auto res = vkCreateImage(m_vkDevice, &ImageCI, nullptr, &vkImage);
    ASSERT_GE(res, 0);

    VkMemoryRequirements MemReqs = {};
    vkGetImageMemoryRequirements(m_vkDevice, vkImage, &MemReqs);

    VkMemoryPropertyFlags ImageMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    auto                  MemoryTypeIndex  = GetMemoryTypeIndex(MemReqs.memoryTypeBits, ImageMemoryFlags);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.pNext                = nullptr;
    AllocInfo.allocationSize       = MemReqs.size;
    AllocInfo.memoryTypeIndex      = MemoryTypeIndex;

    res = vkAllocateMemory(m_vkDevice, &AllocInfo, nullptr, &vkMemory);
    ASSERT_GE(res, 0);

    res = vkBindImageMemory(m_vkDevice, vkImage, vkMemory, 0);
    ASSERT_GE(res, 0);
}

void TestingEnvironmentVk::CreateBuffer(VkDeviceSize          Size,
                                        VkBufferUsageFlags    vkUsage,
                                        VkMemoryPropertyFlags MemoryFlags,
                                        VkDeviceMemory&       vkMemory,
                                        VkBuffer&             vkBuffer)
{
    VkBufferCreateInfo BufferCI = {};

    BufferCI.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCI.pNext                 = nullptr;
    BufferCI.flags                 = 0;
    BufferCI.size                  = Size;
    BufferCI.usage                 = vkUsage;
    BufferCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCI.queueFamilyIndexCount = 0;
    BufferCI.pQueueFamilyIndices   = nullptr;

    auto res = vkCreateBuffer(m_vkDevice, &BufferCI, nullptr, &vkBuffer);
    ASSERT_GE(res, 0);

    VkMemoryRequirements MemReqs = {};
    vkGetBufferMemoryRequirements(m_vkDevice, vkBuffer, &MemReqs);

    auto MemoryTypeIndex = GetMemoryTypeIndex(MemReqs.memoryTypeBits, MemoryFlags);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.pNext                = nullptr;
    AllocInfo.allocationSize       = MemReqs.size;
    AllocInfo.memoryTypeIndex      = MemoryTypeIndex;

    res = vkAllocateMemory(m_vkDevice, &AllocInfo, nullptr, &vkMemory);
    ASSERT_GE(res, 0);

    res = vkBindBufferMemory(m_vkDevice, vkBuffer, vkMemory, 0);
    ASSERT_GE(res, 0);
}


VkRenderPassCreateInfo TestingEnvironmentVk::GetRenderPassCreateInfo(
    Uint32                                                       NumRenderTargets,
    const VkFormat                                               RTVFormats[],
    VkFormat                                                     DSVFormat,
    Uint32                                                       SampleCount,
    VkAttachmentLoadOp                                           DepthAttachmentLoadOp,
    VkAttachmentLoadOp                                           ColorAttachmentLoadOp,
    std::array<VkAttachmentDescription, MAX_RENDER_TARGETS + 1>& Attachments,
    std::array<VkAttachmentReference, MAX_RENDER_TARGETS + 1>&   AttachmentReferences,
    VkSubpassDescription&                                        SubpassDesc)
{
    VERIFY_EXPR(NumRenderTargets <= MAX_RENDER_TARGETS);

    // Prepare render pass create info (7.1)
    VkRenderPassCreateInfo RenderPassCI = {};

    RenderPassCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCI.pNext           = nullptr;
    RenderPassCI.flags           = 0; // reserved for future use
    RenderPassCI.attachmentCount = (DSVFormat != VK_FORMAT_UNDEFINED ? 1 : 0) + NumRenderTargets;

    uint32_t               AttachmentInd             = 0;
    VkSampleCountFlagBits  SampleCountFlags          = static_cast<VkSampleCountFlagBits>(SampleCount);
    VkAttachmentReference* pDepthAttachmentReference = nullptr;
    if (DSVFormat != VK_FORMAT_UNDEFINED)
    {
        auto& DepthAttachment = Attachments[AttachmentInd];

        DepthAttachment.flags   = 0; // Allowed value VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT
        DepthAttachment.format  = DSVFormat;
        DepthAttachment.samples = SampleCountFlags;
        DepthAttachment.loadOp  = DepthAttachmentLoadOp;
        DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // the contents generated during the render pass and within the render
                                                                // area are written to memory. For attachments with a depth/stencil format,
                                                                // this uses the access type VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT.
        DepthAttachment.stencilLoadOp  = DepthAttachmentLoadOp;
        DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        DepthAttachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        DepthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        pDepthAttachmentReference             = &AttachmentReferences[AttachmentInd];
        pDepthAttachmentReference->attachment = AttachmentInd;
        pDepthAttachmentReference->layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        ++AttachmentInd;
    }

    VkAttachmentReference* pColorAttachmentsReference = NumRenderTargets > 0 ? &AttachmentReferences[AttachmentInd] : nullptr;
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt, ++AttachmentInd)
    {
        auto& ColorAttachment = Attachments[AttachmentInd];

        ColorAttachment.flags   = 0; // Allowed value VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT
        ColorAttachment.format  = RTVFormats[rt];
        ColorAttachment.samples = SampleCountFlags;
        ColorAttachment.loadOp  = ColorAttachmentLoadOp;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // the contents generated during the render pass and within the render
                                                                // area are written to memory. For attachments with a color format,
                                                                // this uses the access type VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT.
        ColorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        auto& ColorAttachmentRef      = AttachmentReferences[AttachmentInd];
        ColorAttachmentRef.attachment = AttachmentInd;
        ColorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    RenderPassCI.pAttachments    = Attachments.data();
    RenderPassCI.subpassCount    = 1;
    RenderPassCI.pSubpasses      = &SubpassDesc;
    RenderPassCI.dependencyCount = 0;       // the number of dependencies between pairs of subpasses, or zero indicating no dependencies.
    RenderPassCI.pDependencies   = nullptr; // an array of dependencyCount number of VkSubpassDependency structures describing
                                            // dependencies between pairs of subpasses, or NULL if dependencyCount is zero.


    SubpassDesc.flags                   = 0;                               // All bits for this type are defined by extensions
    SubpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS; // Currently, only graphics subpasses are supported.
    SubpassDesc.inputAttachmentCount    = 0;
    SubpassDesc.pInputAttachments       = nullptr;
    SubpassDesc.colorAttachmentCount    = NumRenderTargets;
    SubpassDesc.pColorAttachments       = pColorAttachmentsReference;
    SubpassDesc.pResolveAttachments     = nullptr;
    SubpassDesc.pDepthStencilAttachment = pDepthAttachmentReference;
    SubpassDesc.preserveAttachmentCount = 0;
    SubpassDesc.pPreserveAttachments    = nullptr;

    return RenderPassCI;
}

VkShaderModule TestingEnvironmentVk::CreateShaderModule(const SHADER_TYPE ShaderType, const std::string& ShaderSource)
{
#if DILIGENT_NO_GLSLANG
    LOG_ERROR("GLSLang was not built. Shader compilaton is not possible.");
    return VK_NULL_HANDLE;
#else
    auto Bytecode = GLSLangUtils::GLSLtoSPIRV(ShaderType, ShaderSource.c_str(), static_cast<int>(ShaderSource.length()), nullptr, nullptr, nullptr);
    VERIFY_EXPR(!Bytecode.empty());
    if (Bytecode.empty())
        return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo ShaderModuleCI = {};

    ShaderModuleCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCI.pNext    = nullptr;
    ShaderModuleCI.flags    = 0;
    ShaderModuleCI.codeSize = Bytecode.size() * sizeof(uint32_t);
    ShaderModuleCI.pCode    = Bytecode.data();

    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
    vkCreateShaderModule(m_vkDevice, &ShaderModuleCI, nullptr, &vkShaderModule);
    VERIFY_EXPR(vkShaderModule != VK_NULL_HANDLE);

    return vkShaderModule;
#endif
}

VkCommandBuffer TestingEnvironmentVk::AllocateCommandBuffer()
{
    VkCommandBufferAllocateInfo AllocInfo = {};

    AllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.commandPool        = m_vkCmdPool;
    AllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocInfo.commandBufferCount = 1;

    VkCommandBuffer vkCmdBuff = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m_vkDevice, &AllocInfo, &vkCmdBuff);
    VERIFY_EXPR(vkCmdBuff != VK_NULL_HANDLE);

    VkCommandBufferBeginInfo BeginInfo = {};

    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    auto res        = vkBeginCommandBuffer(vkCmdBuff, &BeginInfo);
    VERIFY(res >= 0, "Failed to begin command buffer");
    (void)res;

    return vkCmdBuff;
}

void TestingEnvironmentVk::SubmitCommandBuffer(VkCommandBuffer vkCmdBuffer, bool WaitForIdle)
{
    RefCntAutoPtr<IDeviceContextVk> pContextVk{m_pDeviceContext, IID_DeviceContextVk};

    auto* pQeueVk = pContextVk->LockCommandQueue();
    auto  vkQueue = pQeueVk->GetVkQueue();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pCommandBuffers    = &vkCmdBuffer;
    SubmitInfo.commandBufferCount = 1;
    vkQueueSubmit(vkQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    if (WaitForIdle)
        vkQueueWaitIdle(vkQueue);

    pContextVk->UnlockCommandQueue();
}

static VkPipelineStageFlags PipelineStageFromAccessFlags(VkAccessFlags              AccessFlags,
                                                         const VkPipelineStageFlags EnabledGraphicsShaderStages)
{
    // 6.1.3
    VkPipelineStageFlags Stages = 0;

    while (AccessFlags != 0)
    {
        VkAccessFlagBits AccessFlag = static_cast<VkAccessFlagBits>(AccessFlags & (~(AccessFlags - 1)));
        VERIFY_EXPR(AccessFlag != 0 && (AccessFlag & (AccessFlag - 1)) == 0);
        AccessFlags &= ~AccessFlag;

        // An application MUST NOT specify an access flag in a synchronization command if it does not include a
        // pipeline stage in the corresponding stage mask that is able to perform accesses of that type.
        // A table that lists, for each access flag, which pipeline stages can perform that type of access is given in 6.1.3.
        switch (AccessFlag)
        {
            // Read access to an indirect command structure read as part of an indirect drawing or dispatch command
            case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                break;

            // Read access to an index buffer as part of an indexed drawing command, bound by vkCmdBindIndexBuffer
            case VK_ACCESS_INDEX_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                break;

            // Read access to a vertex buffer as part of a drawing command, bound by vkCmdBindVertexBuffers
            case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                break;

            // Read access to a uniform buffer
            case VK_ACCESS_UNIFORM_READ_BIT:
                Stages |= EnabledGraphicsShaderStages | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;

            // Read access to an input attachment within a render pass during fragment shading
            case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;

            // Read access to a storage buffer, uniform texel buffer, storage texel buffer, sampled image, or storage image
            case VK_ACCESS_SHADER_READ_BIT:
                Stages |= EnabledGraphicsShaderStages | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;

            // Write access to a storage buffer, storage texel buffer, or storage image
            case VK_ACCESS_SHADER_WRITE_BIT:
                Stages |= EnabledGraphicsShaderStages | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;

            // Read access to a color attachment, such as via blending, logic operations, or via certain subpass load operations
            case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;

            // Write access to a color or resolve attachment during a render pass or via certain subpass load and store operations
            case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
                Stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;

            // Read access to a depth/stencil attachment, via depth or stencil operations or via certain subpass load operations
            case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                break;

            // Write access to a depth/stencil attachment, via depth or stencil operations or via certain subpass load and store operations
            case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
                Stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                break;

            // Read access to an image or buffer in a copy operation
            case VK_ACCESS_TRANSFER_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            // Write access to an image or buffer in a clear or copy operation
            case VK_ACCESS_TRANSFER_WRITE_BIT:
                Stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            // Read access by a host operation. Accesses of this type are not performed through a resource, but directly on memory
            case VK_ACCESS_HOST_READ_BIT:
                Stages |= VK_PIPELINE_STAGE_HOST_BIT;
                break;

            // Write access by a host operation. Accesses of this type are not performed through a resource, but directly on memory
            case VK_ACCESS_HOST_WRITE_BIT:
                Stages |= VK_PIPELINE_STAGE_HOST_BIT;
                break;

            // Read access via non-specific entities. When included in a destination access mask, makes all available writes
            // visible to all future read accesses on entities known to the Vulkan device
            case VK_ACCESS_MEMORY_READ_BIT:
                break;

            // Write access via non-specific entities. hen included in a source access mask, all writes that are performed
            // by entities known to the Vulkan device are made available. When included in a destination access mask, makes
            // all available writes visible to all future write accesses on entities known to the Vulkan device.
            case VK_ACCESS_MEMORY_WRITE_BIT:
                break;

            default:
                UNEXPECTED("Unknown memory access flag");
        }
    }
    return Stages;
}


static VkPipelineStageFlags AccessMaskFromImageLayout(VkImageLayout Layout,
                                                      bool          IsDstMask // false - source mask
                                                                              // true  - destination mask
)
{
    VkPipelineStageFlags AccessMask = 0;
    switch (Layout)
    {
        // does not support device access. This layout must only be used as the initialLayout member
        // of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
        // When transitioning out of this layout, the contents of the memory are not guaranteed to be preserved (11.4)
        case VK_IMAGE_LAYOUT_UNDEFINED:
            if (IsDstMask)
            {
                UNEXPECTED("The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED. "
                           "This layout must only be used as the initialLayout member of VkImageCreateInfo "
                           "or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
            }
            break;

        // supports all types of device access
        case VK_IMAGE_LAYOUT_GENERAL:
            // VK_IMAGE_LAYOUT_GENERAL must be used for image load/store operations (13.1.1, 13.2.4)
            AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            break;

        // must only be used as a color or resolve attachment in a VkFramebuffer (11.4)
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        // must only be used as a depth/stencil attachment in a VkFramebuffer (11.4)
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        // must only be used as a read-only depth/stencil attachment in a VkFramebuffer and/or as a read-only image in a shader (11.4)
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        // must only be used as a read-only image in a shader (which can be read as a sampled image,
        // combined image/sampler and/or input attachment) (11.4)
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        //  must only be used as a source image of a transfer command (11.4)
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            AccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        // must only be used as a destination image of a transfer command (11.4)
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        // does not support device access. This layout must only be used as the initialLayout member
        // of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
        // When transitioning out of this layout, the contents of the memory are preserved. (11.4)
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            if (!IsDstMask)
            {
                AccessMask = VK_ACCESS_HOST_WRITE_BIT;
            }
            else
            {
                UNEXPECTED("The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED. "
                           "This layout must only be used as the initialLayout member of VkImageCreateInfo "
                           "or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
            }
            break;

        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            AccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;

        default:
            UNEXPECTED("Unexpected image layout");
            break;
    }

    return AccessMask;
}

void TestingEnvironmentVk::TransitionImageLayout(VkCommandBuffer                CmdBuffer,
                                                 VkImage                        Image,
                                                 VkImageLayout&                 CurrentLayout,
                                                 VkImageLayout                  NewLayout,
                                                 const VkImageSubresourceRange& SubresRange,
                                                 VkPipelineStageFlags           EnabledGraphicsShaderStages,
                                                 VkPipelineStageFlags           SrcStages,
                                                 VkPipelineStageFlags           DestStages)
{
    if (CurrentLayout == NewLayout)
        return;

    VkImageMemoryBarrier ImgBarrier = {};
    ImgBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgBarrier.pNext                = nullptr;
    ImgBarrier.srcAccessMask        = 0;
    ImgBarrier.dstAccessMask        = 0;
    ImgBarrier.oldLayout            = CurrentLayout;
    ImgBarrier.newLayout            = NewLayout;
    ImgBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED; // source queue family for a queue family ownership transfer.
    ImgBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED; // destination queue family for a queue family ownership transfer.
    ImgBarrier.image                = Image;
    ImgBarrier.subresourceRange     = SubresRange;
    ImgBarrier.srcAccessMask        = AccessMaskFromImageLayout(CurrentLayout, false);
    ImgBarrier.dstAccessMask        = AccessMaskFromImageLayout(NewLayout, true);

    if (SrcStages == 0)
    {
        if (CurrentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            SrcStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        else if (ImgBarrier.srcAccessMask != 0)
        {
            SrcStages = PipelineStageFromAccessFlags(ImgBarrier.srcAccessMask, EnabledGraphicsShaderStages);
        }
        else
        {
            // An execution dependency with only VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT in the source stage
            // mask will effectively not wait for any prior commands to complete. (6.1.2)
            SrcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
    }

    if (DestStages == 0)
    {
        if (NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            DestStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        else if (ImgBarrier.dstAccessMask != 0)
        {
            DestStages = PipelineStageFromAccessFlags(ImgBarrier.dstAccessMask, EnabledGraphicsShaderStages);
        }
        else
        {
            // An execution dependency with only VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT in the destination
            // stage mask will only prevent that stage from executing in subsequently submitted commands.
            // As this stage does not perform any actual execution, this is not observable - in effect,
            // it does not delay processing of subsequent commands. (6.1.2)
            DestStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
    }

    vkCmdPipelineBarrier(CmdBuffer,
                         SrcStages,  // must not be 0
                         DestStages, // must not be 0
                         0,          // a bitmask specifying how execution and memory dependencies are formed
                         0,          // memoryBarrierCount
                         nullptr,    // pMemoryBarriers
                         0,          // bufferMemoryBarrierCount
                         nullptr,    // pBufferMemoryBarriers
                         1,
                         &ImgBarrier);

    CurrentLayout = NewLayout;
}

TestingEnvironment* CreateTestingEnvironmentVk(const TestingEnvironment::CreateInfo& CI,
                                               const SwapChainDesc&                  SCDesc)
{
    return new TestingEnvironmentVk{CI, SCDesc};
}

} // namespace Testing

} // namespace Diligent
