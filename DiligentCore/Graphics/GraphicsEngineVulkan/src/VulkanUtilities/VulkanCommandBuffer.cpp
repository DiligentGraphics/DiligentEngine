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

#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace VulkanUtilities
{

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

void VulkanCommandBuffer::TransitionImageLayout(VkCommandBuffer                CmdBuffer,
                                                VkImage                        Image,
                                                VkImageLayout                  OldLayout,
                                                VkImageLayout                  NewLayout,
                                                const VkImageSubresourceRange& SubresRange,
                                                VkPipelineStageFlags           EnabledGraphicsShaderStages,
                                                VkPipelineStageFlags           SrcStages,
                                                VkPipelineStageFlags           DestStages)
{
    VERIFY_EXPR(CmdBuffer != VK_NULL_HANDLE);

    VkImageMemoryBarrier ImgBarrier = {};
    ImgBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgBarrier.pNext                = nullptr;
    ImgBarrier.srcAccessMask        = 0;
    ImgBarrier.dstAccessMask        = 0;
    ImgBarrier.oldLayout            = OldLayout;
    ImgBarrier.newLayout            = NewLayout;
    ImgBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED; // source queue family for a queue family ownership transfer.
    ImgBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED; // destination queue family for a queue family ownership transfer.
    ImgBarrier.image                = Image;
    ImgBarrier.subresourceRange     = SubresRange;
    ImgBarrier.srcAccessMask        = AccessMaskFromImageLayout(OldLayout, false);
    ImgBarrier.dstAccessMask        = AccessMaskFromImageLayout(NewLayout, true);

    if (SrcStages == 0)
    {
        if (OldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
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

    // Including a particular pipeline stage in the first synchronization scope of a command implicitly
    // includes logically earlier pipeline stages in the synchronization scope. Similarly, the second
    // synchronization scope includes logically later pipeline stages.
    // However, note that access scopes are not affected in this way - only the precise stages specified
    // are considered part of each access scope.  (6.1.2)

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
    // Each element of pMemoryBarriers, pBufferMemoryBarriers and pImageMemoryBarriers must not
    // have any access flag included in its srcAccessMask member if that bit is not supported by
    // any of the pipeline stages in srcStageMask.
    // Each element of pMemoryBarriers, pBufferMemoryBarriers and pImageMemoryBarriers must not
    // have any access flag included in its dstAccessMask member if that bit is not supported by any
    // of the pipeline stages in dstStageMask (6.6)
}


void VulkanCommandBuffer::BufferMemoryBarrier(VkCommandBuffer      CmdBuffer,
                                              VkBuffer             Buffer,
                                              VkAccessFlags        srcAccessMask,
                                              VkAccessFlags        dstAccessMask,
                                              VkPipelineStageFlags EnabledGraphicsShaderStages,
                                              VkPipelineStageFlags SrcStages,
                                              VkPipelineStageFlags DestStages)
{
    VkBufferMemoryBarrier BuffBarrier = {};
    BuffBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    BuffBarrier.pNext                 = nullptr;
    BuffBarrier.srcAccessMask         = srcAccessMask;
    BuffBarrier.dstAccessMask         = dstAccessMask;
    BuffBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    BuffBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    BuffBarrier.buffer                = Buffer;
    BuffBarrier.offset                = 0;
    BuffBarrier.size                  = VK_WHOLE_SIZE;
    if (SrcStages == 0)
    {
        if (BuffBarrier.srcAccessMask != 0)
            SrcStages = PipelineStageFromAccessFlags(BuffBarrier.srcAccessMask, EnabledGraphicsShaderStages);
        else
        {
            // An execution dependency with only VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT in the source stage
            // mask will effectively not wait for any prior commands to complete. (6.1.2)
            SrcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
    }

    if (DestStages == 0)
    {
        VERIFY(BuffBarrier.dstAccessMask != 0, "Dst access mask must not be zero");
        DestStages = PipelineStageFromAccessFlags(BuffBarrier.dstAccessMask, EnabledGraphicsShaderStages);
    }

    vkCmdPipelineBarrier(CmdBuffer,
                         SrcStages,    // must not be 0
                         DestStages,   // must not be 0
                         0,            // a bitmask specifying how execution and memory dependencies are formed
                         0,            // memoryBarrierCount
                         nullptr,      // pMemoryBarriers
                         1,            // bufferMemoryBarrierCount
                         &BuffBarrier, // pBufferMemoryBarriers
                         0,
                         nullptr);
}

void VulkanCommandBuffer::FlushBarriers()
{
}

} // namespace VulkanUtilities
