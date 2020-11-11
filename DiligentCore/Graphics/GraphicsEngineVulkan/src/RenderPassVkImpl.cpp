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
#include <vector>

#include "RenderPassVkImpl.hpp"
#include "VulkanTypeConversions.hpp"

namespace Diligent
{

RenderPassVkImpl::RenderPassVkImpl(IReferenceCounters*   pRefCounters,
                                   RenderDeviceVkImpl*   pDevice,
                                   const RenderPassDesc& Desc,
                                   bool                  IsDeviceInternal) :
    TRenderPassBase{pRefCounters, pDevice, Desc, IsDeviceInternal}
{
    VkRenderPassCreateInfo RenderPassCI = {};

    RenderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCI.pNext = nullptr;
    RenderPassCI.flags = 0;

    std::vector<VkAttachmentDescription> vkAttachments(m_Desc.AttachmentCount);
    for (Uint32 i = 0; i < m_Desc.AttachmentCount; ++i)
    {
        const auto& Attachment      = m_Desc.pAttachments[i];
        auto&       vkAttachment    = vkAttachments[i];
        vkAttachment.flags          = 0;
        vkAttachment.format         = TexFormatToVkFormat(Attachment.Format);
        vkAttachment.samples        = static_cast<VkSampleCountFlagBits>(Attachment.SampleCount);
        vkAttachment.loadOp         = AttachmentLoadOpToVkAttachmentLoadOp(Attachment.LoadOp);
        vkAttachment.storeOp        = AttachmentStoreOpToVkAttachmentStoreOp(Attachment.StoreOp);
        vkAttachment.stencilLoadOp  = AttachmentLoadOpToVkAttachmentLoadOp(Attachment.StencilLoadOp);
        vkAttachment.stencilStoreOp = AttachmentStoreOpToVkAttachmentStoreOp(Attachment.StencilStoreOp);
        vkAttachment.initialLayout  = ResourceStateToVkImageLayout(Attachment.InitialState, /*IsInsideRenderPass = */ false);
        vkAttachment.finalLayout    = ResourceStateToVkImageLayout(Attachment.FinalState, /*IsInsideRenderPass = */ true);
    }
    RenderPassCI.attachmentCount = Desc.AttachmentCount;
    RenderPassCI.pAttachments    = vkAttachments.data();


    Uint32 TotalAttachmentReferencesCount = 0;
    Uint32 TotalPreserveAttachmentsCount  = 0;
    CountSubpassAttachmentReferences(Desc, TotalAttachmentReferencesCount, TotalPreserveAttachmentsCount);
    std::vector<VkAttachmentReference> vkAttachmentReferences(TotalAttachmentReferencesCount);
    std::vector<Uint32>                vkPreserveAttachments(TotalPreserveAttachmentsCount);

    Uint32 CurrAttachmentReferenceInd = 0;
    Uint32 CurrPreserveAttachmentInd  = 0;

    std::vector<VkSubpassDescription> vkSubpasses(Desc.SubpassCount);
    for (Uint32 i = 0; i < m_Desc.SubpassCount; ++i)
    {
        const auto& SubpassDesc        = m_Desc.pSubpasses[i];
        auto&       vkSubpass          = vkSubpasses[i];
        vkSubpass.flags                = 0;
        vkSubpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vkSubpass.inputAttachmentCount = SubpassDesc.InputAttachmentCount;

        auto ConvertAttachmentReferences = [&](Uint32 NumAttachments, const AttachmentReference* pSrcAttachments) //
        {
            auto* pCurrVkAttachmentReference = &vkAttachmentReferences[CurrAttachmentReferenceInd];
            for (Uint32 attachment = 0; attachment < NumAttachments; ++attachment, ++CurrAttachmentReferenceInd)
            {
                const auto& AttachmnetRef = pSrcAttachments[attachment];

                vkAttachmentReferences[CurrAttachmentReferenceInd].attachment = AttachmnetRef.AttachmentIndex;
                vkAttachmentReferences[CurrAttachmentReferenceInd].layout =
                    ResourceStateToVkImageLayout(AttachmnetRef.State, /*IsInsideRenderPass = */ true);
            }

            return pCurrVkAttachmentReference;
        };

        if (SubpassDesc.InputAttachmentCount != 0)
        {
            vkSubpass.pInputAttachments = ConvertAttachmentReferences(SubpassDesc.InputAttachmentCount, SubpassDesc.pInputAttachments);
        }

        vkSubpass.colorAttachmentCount = SubpassDesc.RenderTargetAttachmentCount;
        if (SubpassDesc.RenderTargetAttachmentCount != 0)
        {
            vkSubpass.pColorAttachments = ConvertAttachmentReferences(SubpassDesc.RenderTargetAttachmentCount, SubpassDesc.pRenderTargetAttachments);
            if (SubpassDesc.pResolveAttachments != nullptr)
            {
                vkSubpass.pResolveAttachments = ConvertAttachmentReferences(SubpassDesc.RenderTargetAttachmentCount, SubpassDesc.pResolveAttachments);
            }
        }

        if (SubpassDesc.pDepthStencilAttachment != nullptr)
        {
            vkSubpass.pDepthStencilAttachment = ConvertAttachmentReferences(1, SubpassDesc.pDepthStencilAttachment);
        }

        vkSubpass.preserveAttachmentCount = SubpassDesc.PreserveAttachmentCount;
        if (SubpassDesc.PreserveAttachmentCount != 0)
        {
            vkSubpass.pPreserveAttachments = &vkPreserveAttachments[CurrPreserveAttachmentInd];
            for (Uint32 prsv_attachment = 0; prsv_attachment < SubpassDesc.PreserveAttachmentCount; ++prsv_attachment, ++CurrPreserveAttachmentInd)
            {
                vkPreserveAttachments[CurrPreserveAttachmentInd] = SubpassDesc.pPreserveAttachments[prsv_attachment];
            }
        }
    }
    VERIFY_EXPR(CurrAttachmentReferenceInd == vkAttachmentReferences.size());
    VERIFY_EXPR(CurrPreserveAttachmentInd == vkPreserveAttachments.size());
    RenderPassCI.subpassCount = Desc.SubpassCount;
    RenderPassCI.pSubpasses   = vkSubpasses.data();

    std::vector<VkSubpassDependency> vkDependencies(Desc.DependencyCount);
    for (Uint32 i = 0; i < Desc.DependencyCount; ++i)
    {
        const auto& DependencyDesc = m_Desc.pDependencies[i];
        auto&       vkDependency   = vkDependencies[i];
        vkDependency.srcSubpass    = DependencyDesc.SrcSubpass;
        vkDependency.dstSubpass    = DependencyDesc.DstSubpass;
        vkDependency.srcStageMask  = DependencyDesc.SrcStageMask;
        vkDependency.dstStageMask  = DependencyDesc.DstStageMask;
        vkDependency.srcAccessMask = DependencyDesc.SrcAccessMask;
        vkDependency.dstAccessMask = DependencyDesc.DstAccessMask;

        // VK_DEPENDENCY_BY_REGION_BIT specifies that dependencies will be framebuffer-local.
        // Framebuffer-local dependencies are more optimal for most architectures; particularly
        // tile-based architectures - which can keep framebuffer-regions entirely in on-chip registers
        // and thus avoid external bandwidth across such a dependency. Including a framebuffer-global
        // dependency in your rendering will usually force all implementations to flush data to memory,
        // or to a higher level cache, breaking any potential locality optimizations.
        vkDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }
    RenderPassCI.dependencyCount = Desc.DependencyCount;
    RenderPassCI.pDependencies   = vkDependencies.data();

    const auto& LogicalDevice = pDevice->GetLogicalDevice();

    m_VkRenderPass = LogicalDevice.CreateRenderPass(RenderPassCI, Desc.Name);
    if (!m_VkRenderPass)
    {
        LOG_ERROR_AND_THROW("Failed to create Vulkan render pass");
    }
}

RenderPassVkImpl::~RenderPassVkImpl()
{
    m_pDevice->SafeReleaseDeviceObject(std::move(m_VkRenderPass), ~Uint64{0});
}

} // namespace Diligent
