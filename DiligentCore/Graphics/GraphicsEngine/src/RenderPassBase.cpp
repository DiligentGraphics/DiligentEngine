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

#include "pch.h"
#include "RenderPassBase.hpp"
#include "GraphicsAccessories.hpp"
#include "Align.hpp"

namespace Diligent
{

void ValidateRenderPassDesc(const RenderPassDesc& Desc)
{
#define LOG_RENDER_PASS_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of render pass '", (Desc.Name ? Desc.Name : ""), "' is invalid: ", ##__VA_ARGS__)

    if (Desc.AttachmentCount != 0 && Desc.pAttachments == nullptr)
    {
        // If attachmentCount is not 0, pAttachments must be a valid pointer to an
        // array of attachmentCount valid VkAttachmentDescription structures.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-pAttachments-parameter
        LOG_RENDER_PASS_ERROR_AND_THROW("the attachment count (", Desc.AttachmentCount, ") is not zero, but pAttachments is null.");
    }

    if (Desc.SubpassCount == 0)
    {
        // subpassCount must be greater than 0.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-subpassCount-arraylength
        LOG_RENDER_PASS_ERROR_AND_THROW("render pass must have at least one subpass.");
    }
    if (Desc.pSubpasses == nullptr)
    {
        // pSubpasses must be a valid pointer to an array of subpassCount valid VkSubpassDescription structures.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-pSubpasses-parameter
        LOG_RENDER_PASS_ERROR_AND_THROW("pSubpasses must not be null.");
    }

    if (Desc.DependencyCount != 0 && Desc.pDependencies == nullptr)
    {
        // If dependencyCount is not 0, pDependencies must be a valid pointer to an array of
        // dependencyCount valid VkSubpassDependency structures.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-pDependencies-parameter
        LOG_RENDER_PASS_ERROR_AND_THROW("the dependency count (", Desc.DependencyCount, ") is not zero, but pDependencies is null.");
    }

    for (Uint32 i = 0; i < Desc.AttachmentCount; ++i)
    {
        const auto& Attachment = Desc.pAttachments[i];
        if (Attachment.Format == TEX_FORMAT_UNKNOWN)
            LOG_RENDER_PASS_ERROR_AND_THROW("the format of attachment ", i, " is unknown.");

        if (Attachment.SampleCount == 0)
            LOG_RENDER_PASS_ERROR_AND_THROW("the sample count of attachment ", i, " is zero.");

        if (!IsPowerOfTwo(Attachment.SampleCount))
            LOG_RENDER_PASS_ERROR_AND_THROW("the sample count (", Uint32{Attachment.SampleCount}, ") of attachment ", i, " is not power of two.");

        const auto& FmtInfo = GetTextureFormatAttribs(Attachment.Format);
        if (FmtInfo.ComponentType == COMPONENT_TYPE_DEPTH ||
            FmtInfo.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            if (Attachment.InitialState != RESOURCE_STATE_DEPTH_WRITE &&
                Attachment.InitialState != RESOURCE_STATE_DEPTH_READ &&
                Attachment.InitialState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.InitialState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_SOURCE &&
                Attachment.InitialState != RESOURCE_STATE_COPY_DEST &&
                Attachment.InitialState != RESOURCE_STATE_COPY_SOURCE &&
                Attachment.InitialState != RESOURCE_STATE_INPUT_ATTACHMENT)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the initial state of depth-stencil attachment ", i, " (", GetResourceStateString(Attachment.InitialState), ") is invalid.");
            }

            if (Attachment.FinalState != RESOURCE_STATE_DEPTH_WRITE &&
                Attachment.FinalState != RESOURCE_STATE_DEPTH_READ &&
                Attachment.FinalState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.FinalState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_SOURCE &&
                Attachment.FinalState != RESOURCE_STATE_COPY_DEST &&
                Attachment.FinalState != RESOURCE_STATE_COPY_SOURCE &&
                Attachment.FinalState != RESOURCE_STATE_INPUT_ATTACHMENT)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the final state of depth-stencil attachment ", i, " (", GetResourceStateString(Attachment.FinalState), ") is invalid.");
            }
        }
        else
        {
            if (Attachment.InitialState != RESOURCE_STATE_RENDER_TARGET &&
                Attachment.InitialState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.InitialState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_SOURCE &&
                Attachment.InitialState != RESOURCE_STATE_COPY_SOURCE &&
                Attachment.InitialState != RESOURCE_STATE_INPUT_ATTACHMENT &&
                Attachment.InitialState != RESOURCE_STATE_PRESENT)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the initial state of color attachment ", i, " (", GetResourceStateString(Attachment.InitialState), ") is invalid.");
            }

            if (Attachment.FinalState != RESOURCE_STATE_RENDER_TARGET &&
                Attachment.FinalState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.FinalState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_SOURCE &&
                Attachment.FinalState != RESOURCE_STATE_COPY_SOURCE &&
                Attachment.FinalState != RESOURCE_STATE_INPUT_ATTACHMENT &&
                Attachment.FinalState != RESOURCE_STATE_PRESENT)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the final state of color attachment ", i, " (", GetResourceStateString(Attachment.FinalState), ") is invalid.");
            }
        }
    }

    for (Uint32 subpass = 0; subpass < Desc.SubpassCount; ++subpass)
    {
        const auto& Subpass = Desc.pSubpasses[subpass];
        if (Subpass.InputAttachmentCount != 0 && Subpass.pInputAttachments == nullptr)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the input attachment count (", Subpass.InputAttachmentCount, ") of subpass ", subpass,
                                            " is not zero, while pInputAttachments is null.");
        }
        if (Subpass.RenderTargetAttachmentCount != 0 && Subpass.pRenderTargetAttachments == nullptr)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the render target attachment count (", Subpass.RenderTargetAttachmentCount, ") of subpass ", subpass,
                                            " is not zero, while pRenderTargetAttachments is null.");
        }
        if (Subpass.PreserveAttachmentCount != 0 && Subpass.pPreserveAttachments == nullptr)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the preserve attachment count (", Subpass.PreserveAttachmentCount, ") of subpass ", subpass,
                                            " is not zero, while pPreserveAttachments is null.");
        }

        for (Uint32 input_attachment = 0; input_attachment < Subpass.InputAttachmentCount; ++input_attachment)
        {
            const auto& AttchRef = Subpass.pInputAttachments[input_attachment];
            if (AttchRef.AttachmentIndex == ATTACHMENT_UNUSED)
                continue;

            // If the attachment member of any element of pInputAttachments, pColorAttachments, pResolveAttachments
            // or pDepthStencilAttachment, or any element of pPreserveAttachments in any element of pSubpasses is not
            // VK_ATTACHMENT_UNUSED, it must be less than attachmentCount
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
            if (AttchRef.AttachmentIndex >= Desc.AttachmentCount)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the attachment index (", AttchRef.AttachmentIndex, ") of input attachment reference ", input_attachment,
                                                " of subpass ", subpass, " must be less than the number of attachments (", Desc.AttachmentCount, ").");
            }
        }

        for (Uint32 rt_attachment = 0; rt_attachment < Subpass.RenderTargetAttachmentCount; ++rt_attachment)
        {
            const auto& AttchRef = Subpass.pRenderTargetAttachments[rt_attachment];
            if (AttchRef.AttachmentIndex == ATTACHMENT_UNUSED)
                continue;

            // If the attachment member of any element of pInputAttachments, pColorAttachments, pResolveAttachments
            // or pDepthStencilAttachment, or any element of pPreserveAttachments in any element of pSubpasses is not
            // VK_ATTACHMENT_UNUSED, it must be less than attachmentCount
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
            if (AttchRef.AttachmentIndex >= Desc.AttachmentCount)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the attachment index (", AttchRef.AttachmentIndex, ") of render target attachment reference ", rt_attachment,
                                                " of subpass ", subpass, " must be less than the number of attachments (", Desc.AttachmentCount, ").");
            }
        }

        if (Subpass.pResolveAttachments != nullptr)
        {
            for (Uint32 rslv_attachment = 0; rslv_attachment < Subpass.RenderTargetAttachmentCount; ++rslv_attachment)
            {
                const auto& AttchRef = Subpass.pResolveAttachments[rslv_attachment];
                if (AttchRef.AttachmentIndex == ATTACHMENT_UNUSED)
                    continue;

                // If the attachment member of any element of pInputAttachments, pColorAttachments, pResolveAttachments
                // or pDepthStencilAttachment, or any element of pPreserveAttachments in any element of pSubpasses is not
                // VK_ATTACHMENT_UNUSED, it must be less than attachmentCount
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
                if (AttchRef.AttachmentIndex >= Desc.AttachmentCount)
                {
                    LOG_RENDER_PASS_ERROR_AND_THROW("the attachment index (", AttchRef.AttachmentIndex, ") of resolve attachment reference ", rslv_attachment,
                                                    " of subpass ", subpass, " must be less than the number of attachments (", Desc.AttachmentCount, ").");
                }
            }
        }

        if (Subpass.pDepthStencilAttachment != nullptr)
        {
            const auto& AttchRef = *Subpass.pDepthStencilAttachment;
            if (AttchRef.AttachmentIndex != ATTACHMENT_UNUSED)
            {
                // If the attachment member of any element of pInputAttachments, pColorAttachments, pResolveAttachments
                // or pDepthStencilAttachment, or any element of pPreserveAttachments in any element of pSubpasses is not
                // VK_ATTACHMENT_UNUSED, it must be less than attachmentCount
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-attachment-00834
                if (AttchRef.AttachmentIndex >= Desc.AttachmentCount)
                {
                    LOG_RENDER_PASS_ERROR_AND_THROW("the attachment index (", AttchRef.AttachmentIndex, ") of depth-stencil attachment reference of subpass ", subpass,
                                                    " must be less than the number of attachments (", Desc.AttachmentCount, ").");
                }
            }
        }

        for (Uint32 prsv_attachment = 0; prsv_attachment < Subpass.PreserveAttachmentCount; ++prsv_attachment)
        {
            const auto PrsvAttachment = Subpass.pPreserveAttachments[prsv_attachment];
            if (PrsvAttachment == ATTACHMENT_UNUSED)
            {
                // The attachment member of each element of pPreserveAttachments must not be VK_ATTACHMENT_UNUSED
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-attachment-00853
                LOG_RENDER_PASS_ERROR_AND_THROW("the attachment index of preserve attachment reference ", prsv_attachment,
                                                " of subpass ", subpass, " is ATTACHMENT_UNUSED.");
            }

            if (PrsvAttachment >= Desc.AttachmentCount)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the attachment index (", PrsvAttachment, ") of preserve attachment reference ", prsv_attachment,
                                                " of subpass ", subpass, " exceeds the number of attachments (", Desc.AttachmentCount, ").");
            }
        }

        if (Subpass.pResolveAttachments != nullptr)
        {
            for (Uint32 attchmnt = 0; attchmnt < Subpass.RenderTargetAttachmentCount; ++attchmnt)
            {
                const auto& RTAttachmentRef   = Subpass.pRenderTargetAttachments[attchmnt];
                const auto& RslvAttachmentRef = Subpass.pResolveAttachments[attchmnt];
                if (RslvAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED && RTAttachmentRef.AttachmentIndex == ATTACHMENT_UNUSED)
                {
                    // If pResolveAttachments is not NULL, for each resolve attachment that is not VK_ATTACHMENT_UNUSED,
                    // the corresponding color attachment must not be VK_ATTACHMENT_UNUSED
                    // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00847
                    LOG_RENDER_PASS_ERROR_AND_THROW("pResolveAttachments of subpass ", subpass, " is not null and resolve attachment reference ", attchmnt,
                                                    " is not ATTACHMENT_UNUSED, but corresponding render target attachment reference is ATTACHMENT_UNUSED.");
                }

                if (RslvAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED && Desc.pAttachments[RTAttachmentRef.AttachmentIndex].SampleCount == 1)
                {
                    // If pResolveAttachments is not NULL, for each resolve attachment that is not VK_ATTACHMENT_UNUSED,
                    // the corresponding color attachment must not have a sample count of VK_SAMPLE_COUNT_1_BIT
                    // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00848
                    LOG_RENDER_PASS_ERROR_AND_THROW("Render target attachment at index ", RTAttachmentRef.AttachmentIndex, " referenced by",
                                                    " attachment reference ", attchmnt, " of subpass ", subpass,
                                                    " is used as the source of resolve operation, but its sample count is 1.");
                }

                if (RslvAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED && Desc.pAttachments[RslvAttachmentRef.AttachmentIndex].SampleCount != 1)
                {
                    // If pResolveAttachments is not NULL, each resolve attachment that is not VK_ATTACHMENT_UNUSED must
                    // have a sample count of VK_SAMPLE_COUNT_1_BIT
                    // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00849
                    LOG_RENDER_PASS_ERROR_AND_THROW("Resolve attachment at index ", RslvAttachmentRef.AttachmentIndex, " referenced by",
                                                    " attachment reference ", attchmnt, " of subpass ", subpass,
                                                    " must have sample count of 1.");
                }

                if (RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED && RslvAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED &&
                    Desc.pAttachments[RTAttachmentRef.AttachmentIndex].Format != Desc.pAttachments[RslvAttachmentRef.AttachmentIndex].Format)
                {
                    // If pResolveAttachments is not NULL, each resolve attachment that is not VK_ATTACHMENT_UNUSED
                    // must have the same VkFormat as its corresponding color attachment.
                    // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkSubpassDescription-pResolveAttachments-00850
                    LOG_RENDER_PASS_ERROR_AND_THROW("The format (",
                                                    GetTextureFormatAttribs(Desc.pAttachments[RTAttachmentRef.AttachmentIndex].Format).Name,
                                                    ") of render target attachment at index ", RTAttachmentRef.AttachmentIndex,
                                                    " referenced by attachment reference ", attchmnt, " of subpass ", subpass,
                                                    " does not match the format (",
                                                    GetTextureFormatAttribs(Desc.pAttachments[RslvAttachmentRef.AttachmentIndex].Format).Name,
                                                    ") of the corresponding resolve attachment at index ",
                                                    RslvAttachmentRef.AttachmentIndex, ".");
                }
            }
        }
    }

    for (Uint32 i = 0; i < Desc.DependencyCount; ++i)
    {
        const auto& Dependency = Desc.pDependencies[i];

        if (Dependency.SrcStageMask == PIPELINE_STAGE_FLAG_UNDEFINED)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the source stage mask of subpass dependency ", i, " is undefined.");
        }
        if (Dependency.DstStageMask == PIPELINE_STAGE_FLAG_UNDEFINED)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the destination stage mask of subpass dependency ", i, " is undefined.");
        }
    }
}

} // namespace Diligent
