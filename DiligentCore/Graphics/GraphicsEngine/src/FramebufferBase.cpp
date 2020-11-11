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
#include "FramebufferBase.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

void ValidateFramebufferDesc(const FramebufferDesc& Desc)
{
#define LOG_FRAMEBUFFER_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of framebuffer '", (Desc.Name ? Desc.Name : ""), "' is invalid: ", ##__VA_ARGS__)

    if (Desc.pRenderPass == nullptr)
    {
        LOG_FRAMEBUFFER_ERROR_AND_THROW("render pass must not be null.");
    }

    if (Desc.AttachmentCount != 0 && Desc.ppAttachments == nullptr)
    {
        // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, and attachmentCount is not 0,
        // pAttachments must be a valid pointer to an array of attachmentCount valid VkImageView handles.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-flags-02778
        LOG_FRAMEBUFFER_ERROR_AND_THROW("attachment count is not zero, but ppAttachments is null.");
    }

    for (Uint32 i = 0; i < Desc.AttachmentCount; ++i)
    {
        if (Desc.ppAttachments[i] == nullptr)
        {
            // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, and attachmentCount is not 0,
            // pAttachments must be a valid pointer to an array of attachmentCount valid VkImageView handles.
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-flags-02778
            LOG_FRAMEBUFFER_ERROR_AND_THROW("framebuffer attachment ", i, " is null.");
        }
    }

    const auto& RPDesc = Desc.pRenderPass->GetDesc();
    if (Desc.AttachmentCount != RPDesc.AttachmentCount)
    {
        // AttachmentCount must be equal to the attachment count specified in renderPass.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-attachmentCount-00876
        LOG_FRAMEBUFFER_ERROR_AND_THROW("the number of framebuffer attachments (", Desc.AttachmentCount,
                                        ") must be equal to the number of attachments (", RPDesc.AttachmentCount,
                                        ") in the render pass '", RPDesc.Name, "'.");
    }

    for (Uint32 i = 0; i < RPDesc.AttachmentCount; ++i)
    {
        if (Desc.ppAttachments[i] == nullptr)
            continue;

        const auto& AttDesc  = RPDesc.pAttachments[i];
        const auto& ViewDesc = Desc.ppAttachments[i]->GetDesc();
        const auto& TexDesc  = Desc.ppAttachments[i]->GetTexture()->GetDesc();

        // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments
        // must have been created with a VkFormat value that matches the VkFormat specified by the corresponding
        // VkAttachmentDescription in renderPass
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00880
        if (ViewDesc.Format != AttDesc.Format)
        {
            LOG_FRAMEBUFFER_ERROR_AND_THROW("the format (", GetTextureFormatAttribs(ViewDesc.Format).Name, ") of attachment ", i,
                                            " does not match the format (", GetTextureFormatAttribs(AttDesc.Format).Name,
                                            ") defined by the render pass for the same attachment.");
        }

        // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments must have
        // been created with a samples value that matches the samples value specified by the corresponding
        // VkAttachmentDescription in renderPass
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00881
        if (TexDesc.SampleCount != AttDesc.SampleCount)
        {
            LOG_FRAMEBUFFER_ERROR_AND_THROW("the sample count (", Uint32{TexDesc.SampleCount}, ") of attachment ", i,
                                            " does not match the sample count (", Uint32{AttDesc.SampleCount},
                                            ") defined by the render pass for the same attachment.");
        }
    }

    for (Uint32 i = 0; i < RPDesc.SubpassCount; ++i)
    {
        const auto& Subpass = RPDesc.pSubpasses[i];
        for (Uint32 input_attachment = 0; input_attachment < Subpass.InputAttachmentCount; ++input_attachment)
        {
            const auto& AttchRef = Subpass.pInputAttachments[input_attachment];
            if (AttchRef.AttachmentIndex == ATTACHMENT_UNUSED)
                continue;

            VERIFY(AttchRef.AttachmentIndex < Desc.AttachmentCount,
                   "Input attachment index (", AttchRef.AttachmentIndex, ") must be less than the attachment count (",
                   Desc.AttachmentCount, ") as this is ensured by ValidateRenderPassDesc.");

            if (Desc.ppAttachments[AttchRef.AttachmentIndex] == nullptr)
            {
                LOG_FRAMEBUFFER_ERROR_AND_THROW("attachment at index ", AttchRef.AttachmentIndex,
                                                " is used as input attachment by subpass ",
                                                i, " of render pass '", RPDesc.Name,
                                                "', and must not be null.");
            }

            const auto& TexDesc = Desc.ppAttachments[AttchRef.AttachmentIndex]->GetTexture()->GetDesc();

            // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that
            // is used as an input attachment by renderPass must have been created with a usage value including
            // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00879
            if ((TexDesc.BindFlags & BIND_INPUT_ATTACHMENT) == 0)
            {
                LOG_FRAMEBUFFER_ERROR_AND_THROW("the attachment '", TexDesc.Name, "' at index ", AttchRef.AttachmentIndex,
                                                " is used as input attachment by subpass ",
                                                i, " of render pass '", RPDesc.Name,
                                                "', but was not created with BIND_INPUT_ATTACHMENT bind flag.");
            }
        }

        for (Uint32 rt_attachment = 0; rt_attachment < Subpass.RenderTargetAttachmentCount; ++rt_attachment)
        {
            const auto& AttchRef = Subpass.pRenderTargetAttachments[rt_attachment];
            if (AttchRef.AttachmentIndex == ATTACHMENT_UNUSED)
                continue;

            VERIFY(AttchRef.AttachmentIndex < Desc.AttachmentCount,
                   "Render target attachment index (", AttchRef.AttachmentIndex, ") must be less than the attachment count (",
                   Desc.AttachmentCount, ") as this is ensured by ValidateRenderPassDesc.");

            if (Desc.ppAttachments[AttchRef.AttachmentIndex] == nullptr)
            {
                LOG_FRAMEBUFFER_ERROR_AND_THROW("attachment at index ", AttchRef.AttachmentIndex,
                                                " is used as render target attachment by subpass ",
                                                i, " of render pass '", RPDesc.Name,
                                                "', and must not be null.");
            }

            const auto& TexDesc = Desc.ppAttachments[AttchRef.AttachmentIndex]->GetTexture()->GetDesc();

            // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used
            // as a color attachment or resolve attachment by renderPass must have been created with a usage value including
            // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00877
            if ((TexDesc.BindFlags & BIND_RENDER_TARGET) == 0)
            {
                LOG_FRAMEBUFFER_ERROR_AND_THROW("the attachment '", TexDesc.Name, "' at index ", AttchRef.AttachmentIndex,
                                                " is used as render target attachment by subpass ",
                                                i, " of render pass '", RPDesc.Name,
                                                "', but was not created with BIND_RENDER_TARGET bind flag.");
            }
        }

        if (Subpass.pResolveAttachments != nullptr)
        {
            for (Uint32 rslv_attachment = 0; rslv_attachment < Subpass.RenderTargetAttachmentCount; ++rslv_attachment)
            {
                const auto& AttchRef = Subpass.pResolveAttachments[rslv_attachment];
                if (AttchRef.AttachmentIndex == ATTACHMENT_UNUSED)
                    continue;

                VERIFY(AttchRef.AttachmentIndex < Desc.AttachmentCount,
                       "Resolve attachment index (", AttchRef.AttachmentIndex, ") must be less than the attachment count (",
                       Desc.AttachmentCount, ") as this is ensured by ValidateRenderPassDesc.");

                if (Desc.ppAttachments[AttchRef.AttachmentIndex] == nullptr)
                {
                    LOG_FRAMEBUFFER_ERROR_AND_THROW("attachment at index ", AttchRef.AttachmentIndex,
                                                    " is used as resolve attachment by subpass ",
                                                    i, " of render pass '", RPDesc.Name,
                                                    "', and must not be null.");
                }

                const auto& TexDesc = Desc.ppAttachments[AttchRef.AttachmentIndex]->GetTexture()->GetDesc();

                // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used
                // as a color attachment or resolve attachment by renderPass must have been created with a usage value including
                // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-00877
                if ((TexDesc.BindFlags & BIND_RENDER_TARGET) == 0)
                {
                    LOG_FRAMEBUFFER_ERROR_AND_THROW("the attachment '", TexDesc.Name, "' at index ", AttchRef.AttachmentIndex,
                                                    " is used as resolve attachment by subpass ",
                                                    i, " of render pass '", RPDesc.Name,
                                                    "', but was not created with BIND_RENDER_TARGET bind flag.");
                }
            }
        }

        if (Subpass.pDepthStencilAttachment != nullptr)
        {
            const auto& AttchRef = *Subpass.pDepthStencilAttachment;
            if (AttchRef.AttachmentIndex != ATTACHMENT_UNUSED)
            {
                VERIFY(AttchRef.AttachmentIndex < Desc.AttachmentCount,
                       "Depth-stencil attachment index (", AttchRef.AttachmentIndex, ") must be less than the attachment count (",
                       Desc.AttachmentCount, ") as this is ensured by ValidateRenderPassDesc.");

                if (Desc.ppAttachments[AttchRef.AttachmentIndex] == nullptr)
                {
                    LOG_FRAMEBUFFER_ERROR_AND_THROW("attachment at index ", AttchRef.AttachmentIndex,
                                                    " is used as detph-stencil attachment by subpass ",
                                                    i, " of render pass '", RPDesc.Name,
                                                    "', and must not be null.");
                }

                const auto& TexDesc = Desc.ppAttachments[AttchRef.AttachmentIndex]->GetTexture()->GetDesc();

                // If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is
                // used as a depth/stencil attachment by renderPass must have been created with a usage value including
                // VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkFramebufferCreateInfo-pAttachments-02633
                if ((TexDesc.BindFlags & BIND_DEPTH_STENCIL) == 0)
                {
                    LOG_FRAMEBUFFER_ERROR_AND_THROW("the attachment '", TexDesc.Name, "' at index ", AttchRef.AttachmentIndex,
                                                    " is used as detph-stencil attachment by subpass ",
                                                    i, " of render pass '", RPDesc.Name,
                                                    "', but was not created with BIND_DEPTH_STENCIL bind flag.");
                }
            }
        }
    }
}

} // namespace Diligent
