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

#include "VulkanHeaders.h"
#include "DebugUtilities.hpp"

namespace VulkanUtilities
{

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer(VkPipelineStageFlags EnabledGraphicsShaderStages) noexcept :
        m_EnabledGraphicsShaderStages{EnabledGraphicsShaderStages}
    {}

    // clang-format off
    VulkanCommandBuffer             (const VulkanCommandBuffer&)  = delete;
    VulkanCommandBuffer             (      VulkanCommandBuffer&&) = delete;
    VulkanCommandBuffer& operator = (const VulkanCommandBuffer&)  = delete;
    VulkanCommandBuffer& operator = (      VulkanCommandBuffer&&) = delete;
    // clang-format on

    __forceinline void ClearColorImage(VkImage                        Image,
                                       const VkClearColorValue&       Color,
                                       const VkImageSubresourceRange& Subresource)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass == VK_NULL_HANDLE, "vkCmdClearColorImage() must be called outside of render pass (17.1)");
        VERIFY(Subresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT, "The aspectMask of all image subresource ranges must only include VK_IMAGE_ASPECT_COLOR_BIT (17.1)");

        vkCmdClearColorImage(
            m_VkCmdBuffer,
            Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            &Color,
            1,
            &Subresource);
    }

    __forceinline void ClearDepthStencilImage(VkImage                         Image,
                                              const VkClearDepthStencilValue& DepthStencil,
                                              const VkImageSubresourceRange&  Subresource)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass == VK_NULL_HANDLE, "vkCmdClearDepthStencilImage() must be called outside of render pass (17.1)");
        // clang-format off
        VERIFY((Subresource.aspectMask &  (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0 &&
               (Subresource.aspectMask & ~(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) == 0,
               "The aspectMask of all image subresource ranges must only include VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT(17.1)");
        // clang-format on

        vkCmdClearDepthStencilImage(
            m_VkCmdBuffer,
            Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            &DepthStencil,
            1,
            &Subresource);
    }

    __forceinline void ClearAttachment(const VkClearAttachment& Attachment, const VkClearRect& ClearRect)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "vkCmdClearAttachments() must be called inside render pass (17.2)");

        vkCmdClearAttachments(
            m_VkCmdBuffer,
            1,
            &Attachment,
            1,
            &ClearRect // The rectangular region specified by each element of pRects must be
                       // contained within the render area of the current render pass instance
        );
    }

    __forceinline void Draw(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "vkCmdDraw() must be called inside render pass (19.3)");
        VERIFY(m_State.GraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound");

        vkCmdDraw(m_VkCmdBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
    }

    __forceinline void DrawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex, int32_t VertexOffset, uint32_t FirstInstance)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "vkCmdDrawIndexed() must be called inside render pass (19.3)");
        VERIFY(m_State.GraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound");
        VERIFY(m_State.IndexBuffer != VK_NULL_HANDLE, "No index buffer bound");

        vkCmdDrawIndexed(m_VkCmdBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
    }

    __forceinline void DrawIndirect(VkBuffer Buffer, VkDeviceSize Offset, uint32_t DrawCount, uint32_t Stride)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "vkCmdDrawIndirect() must be called inside render pass (19.3)");
        VERIFY(m_State.GraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound");

        vkCmdDrawIndirect(m_VkCmdBuffer, Buffer, Offset, DrawCount, Stride);
    }

    __forceinline void DrawIndexedIndirect(VkBuffer Buffer, VkDeviceSize Offset, uint32_t DrawCount, uint32_t Stride)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "vkCmdDrawIndirect() must be called inside render pass (19.3)");
        VERIFY(m_State.GraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound");
        VERIFY(m_State.IndexBuffer != VK_NULL_HANDLE, "No index buffer bound");

        vkCmdDrawIndexedIndirect(m_VkCmdBuffer, Buffer, Offset, DrawCount, Stride);
    }

    __forceinline void DrawMesh(uint32_t TaskCount, uint32_t FirstTask)
    {
#ifdef VK_NV_mesh_shader
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "vkCmdDrawMeshTasksNV() must be called inside render pass");
        VERIFY(m_State.GraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound");

        vkCmdDrawMeshTasksNV(m_VkCmdBuffer, TaskCount, FirstTask);
#else
        UNSUPPORTED("DrawMesh is not supported in current Vulkan headers");
#endif
    }

    __forceinline void DrawMeshIndirect(VkBuffer Buffer, VkDeviceSize Offset, uint32_t DrawCount, uint32_t Stride)
    {
#ifdef VK_NV_mesh_shader
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "vkCmdDrawMeshTasksNV() must be called inside render pass");
        VERIFY(m_State.GraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound");

        vkCmdDrawMeshTasksIndirectNV(m_VkCmdBuffer, Buffer, Offset, DrawCount, Stride);
#else
        UNSUPPORTED("DrawMeshIndirect is not supported in current Vulkan headers");
#endif
    }

    __forceinline void Dispatch(uint32_t GroupCountX, uint32_t GroupCountY, uint32_t GroupCountZ)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass == VK_NULL_HANDLE, "vkCmdDispatch() must be called outside of render pass (27)");
        VERIFY(m_State.ComputePipeline != VK_NULL_HANDLE, "No compute pipeline bound");

        vkCmdDispatch(m_VkCmdBuffer, GroupCountX, GroupCountY, GroupCountZ);
    }

    __forceinline void DispatchIndirect(VkBuffer Buffer, VkDeviceSize Offset)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass == VK_NULL_HANDLE, "vkCmdDispatchIndirect() must be called outside of render pass (27)");
        VERIFY(m_State.ComputePipeline != VK_NULL_HANDLE, "No compute pipeline bound");

        vkCmdDispatchIndirect(m_VkCmdBuffer, Buffer, Offset);
    }

    __forceinline void BeginRenderPass(VkRenderPass        RenderPass,
                                       VkFramebuffer       Framebuffer,
                                       uint32_t            FramebufferWidth,
                                       uint32_t            FramebufferHeight,
                                       uint32_t            ClearValueCount = 0,
                                       const VkClearValue* pClearValues    = nullptr)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        VERIFY(m_State.RenderPass == VK_NULL_HANDLE, "Current pass has not been ended");

        if (m_State.RenderPass != RenderPass || m_State.Framebuffer != Framebuffer)
        {
            VkRenderPassBeginInfo BeginInfo;
            BeginInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            BeginInfo.pNext       = nullptr;
            BeginInfo.renderPass  = RenderPass;
            BeginInfo.framebuffer = Framebuffer;
            // The render area MUST be contained within the framebuffer dimensions (7.4)
            BeginInfo.renderArea      = {{0, 0}, {FramebufferWidth, FramebufferHeight}};
            BeginInfo.clearValueCount = ClearValueCount;
            BeginInfo.pClearValues    = pClearValues; // an array of VkClearValue structures that contains clear values for
                                                      // each attachment, if the attachment uses a loadOp value of VK_ATTACHMENT_LOAD_OP_CLEAR
                                                      // or if the attachment has a depth/stencil format and uses a stencilLoadOp value of
                                                      // VK_ATTACHMENT_LOAD_OP_CLEAR. The array is indexed by attachment number. Only elements
                                                      // corresponding to cleared attachments are used. Other elements of pClearValues are
                                                      // ignored (7.4)

            vkCmdBeginRenderPass(m_VkCmdBuffer, &BeginInfo,
                                 VK_SUBPASS_CONTENTS_INLINE // the contents of the subpass will be recorded inline in the
                                                            // primary command buffer, and secondary command buffers must not
                                                            // be executed within the subpass
            );
            m_State.RenderPass        = RenderPass;
            m_State.Framebuffer       = Framebuffer;
            m_State.FramebufferWidth  = FramebufferWidth;
            m_State.FramebufferHeight = FramebufferHeight;
        }
    }

    __forceinline void EndRenderPass()
    {
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "Render pass has not been started");
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdEndRenderPass(m_VkCmdBuffer);
        m_State.RenderPass        = VK_NULL_HANDLE;
        m_State.Framebuffer       = VK_NULL_HANDLE;
        m_State.FramebufferWidth  = 0;
        m_State.FramebufferHeight = 0;
        if (m_State.InsidePassQueries != 0)
        {
            LOG_ERROR_MESSAGE("Ending render pass while there are outstanding queries that have been started inside the pass, "
                              "but have not been ended. Vulkan requires that a query must either begin and end inside the same "
                              "subpass of a render pass instance, or must both begin and end outside of a render pass "
                              "instance (i.e. contain entire render pass instances). (17.2)");
        }
    }

    __forceinline void NextSubpass()
    {
        VERIFY(m_State.RenderPass != VK_NULL_HANDLE, "Render pass has not been started");
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdNextSubpass(m_VkCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
    }

    __forceinline void EndCommandBuffer()
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkEndCommandBuffer(m_VkCmdBuffer);
    }

    __forceinline void Reset()
    {
        m_VkCmdBuffer = VK_NULL_HANDLE;
        m_State       = StateCache{};
    }

    __forceinline void BindComputePipeline(VkPipeline ComputePipeline)
    {
        // 9.8
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.ComputePipeline != ComputePipeline)
        {
            vkCmdBindPipeline(m_VkCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ComputePipeline);
            m_State.ComputePipeline = ComputePipeline;
        }
    }

    __forceinline void BindGraphicsPipeline(VkPipeline GraphicsPipeline)
    {
        // 9.8
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.GraphicsPipeline != GraphicsPipeline)
        {
            vkCmdBindPipeline(m_VkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
            m_State.GraphicsPipeline = GraphicsPipeline;
        }
    }

    __forceinline void SetViewports(uint32_t FirstViewport, uint32_t ViewportCount, const VkViewport* pViewports)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdSetViewport(m_VkCmdBuffer, FirstViewport, ViewportCount, pViewports);
    }

    __forceinline void SetScissorRects(uint32_t FirstScissor, uint32_t ScissorCount, const VkRect2D* pScissors)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdSetScissor(m_VkCmdBuffer, FirstScissor, ScissorCount, pScissors);
    }

    __forceinline void SetStencilReference(uint32_t Reference)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdSetStencilReference(m_VkCmdBuffer, VK_STENCIL_FRONT_AND_BACK, Reference);
    }

    __forceinline void SetBlendConstants(const float BlendConstants[4])
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdSetBlendConstants(m_VkCmdBuffer, BlendConstants);
    }

    __forceinline void BindIndexBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkIndexType IndexType)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        // clang-format off
        if (m_State.IndexBuffer       != Buffer ||
            m_State.IndexBufferOffset != Offset ||
            m_State.IndexType         != IndexType)
        {
            // clang-format on
            vkCmdBindIndexBuffer(m_VkCmdBuffer, Buffer, Offset, IndexType);
            m_State.IndexBuffer       = Buffer;
            m_State.IndexBufferOffset = Offset;
            m_State.IndexType         = IndexType;
        }
    }

    __forceinline void BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdBindVertexBuffers(m_VkCmdBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
    }

    static void TransitionImageLayout(VkCommandBuffer                CmdBuffer,
                                      VkImage                        Image,
                                      VkImageLayout                  OldLayout,
                                      VkImageLayout                  NewLayout,
                                      const VkImageSubresourceRange& SubresRange,
                                      VkPipelineStageFlags           EnabledGraphicsShaderStages,
                                      VkPipelineStageFlags           SrcStages  = 0,
                                      VkPipelineStageFlags           DestStages = 0);

    __forceinline void TransitionImageLayout(VkImage                        Image,
                                             VkImageLayout                  OldLayout,
                                             VkImageLayout                  NewLayout,
                                             const VkImageSubresourceRange& SubresRange,
                                             VkPipelineStageFlags           SrcStages  = 0,
                                             VkPipelineStageFlags           DestStages = 0)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Image layout transitions within a render pass execute
            // dependencies between attachments
            EndRenderPass();
        }
        TransitionImageLayout(m_VkCmdBuffer, Image, OldLayout, NewLayout, SubresRange, m_EnabledGraphicsShaderStages, SrcStages, DestStages);
    }


    static void BufferMemoryBarrier(VkCommandBuffer      CmdBuffer,
                                    VkBuffer             Buffer,
                                    VkAccessFlags        srcAccessMask,
                                    VkAccessFlags        dstAccessMask,
                                    VkPipelineStageFlags EnabledGraphicsShaderStages,
                                    VkPipelineStageFlags SrcStages  = 0,
                                    VkPipelineStageFlags DestStages = 0);

    __forceinline void BufferMemoryBarrier(VkBuffer             Buffer,
                                           VkAccessFlags        srcAccessMask,
                                           VkAccessFlags        dstAccessMask,
                                           VkPipelineStageFlags SrcStages  = 0,
                                           VkPipelineStageFlags DestStages = 0)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Image layout transitions within a render pass execute
            // dependencies between attachments
            EndRenderPass();
        }
        BufferMemoryBarrier(m_VkCmdBuffer, Buffer, srcAccessMask, dstAccessMask, m_EnabledGraphicsShaderStages, SrcStages, DestStages);
    }

    __forceinline void BindDescriptorSets(VkPipelineBindPoint    pipelineBindPoint,
                                          VkPipelineLayout       layout,
                                          uint32_t               firstSet,
                                          uint32_t               descriptorSetCount,
                                          const VkDescriptorSet* pDescriptorSets,
                                          uint32_t               dynamicOffsetCount = 0,
                                          const uint32_t*        pDynamicOffsets    = nullptr)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdBindDescriptorSets(m_VkCmdBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
    }

    __forceinline void CopyBuffer(VkBuffer            srcBuffer,
                                  VkBuffer            dstBuffer,
                                  uint32_t            regionCount,
                                  const VkBufferCopy* pRegions)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Copy buffer operation must be performed outside of render pass.
            EndRenderPass();
        }
        vkCmdCopyBuffer(m_VkCmdBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
    }

    __forceinline void CopyImage(VkImage            srcImage,
                                 VkImageLayout      srcImageLayout,
                                 VkImage            dstImage,
                                 VkImageLayout      dstImageLayout,
                                 uint32_t           regionCount,
                                 const VkImageCopy* pRegions)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Copy operations must be performed outside of render pass.
            EndRenderPass();
        }

        vkCmdCopyImage(m_VkCmdBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }

    __forceinline void CopyBufferToImage(VkBuffer                 srcBuffer,
                                         VkImage                  dstImage,
                                         VkImageLayout            dstImageLayout,
                                         uint32_t                 regionCount,
                                         const VkBufferImageCopy* pRegions)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Copy operations must be performed outside of render pass.
            EndRenderPass();
        }

        vkCmdCopyBufferToImage(m_VkCmdBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }

    __forceinline void CopyImageToBuffer(VkImage                  srcImage,
                                         VkImageLayout            srcImageLayout,
                                         VkBuffer                 dstBuffer,
                                         uint32_t                 regionCount,
                                         const VkBufferImageCopy* pRegions)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Copy operations must be performed outside of render pass.
            EndRenderPass();
        }

        vkCmdCopyImageToBuffer(m_VkCmdBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
    }

    __forceinline void BlitImage(VkImage            srcImage,
                                 VkImageLayout      srcImageLayout,
                                 VkImage            dstImage,
                                 VkImageLayout      dstImageLayout,
                                 uint32_t           regionCount,
                                 const VkImageBlit* pRegions,
                                 VkFilter           filter)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Blit must be performed outside of render pass.
            EndRenderPass();
        }

        vkCmdBlitImage(m_VkCmdBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
    }

    __forceinline void ResolveImage(VkImage               srcImage,
                                    VkImageLayout         srcImageLayout,
                                    VkImage               dstImage,
                                    VkImageLayout         dstImageLayout,
                                    uint32_t              regionCount,
                                    const VkImageResolve* pRegions)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Resolve must be performed outside of render pass.
            EndRenderPass();
        }
        vkCmdResolveImage(m_VkCmdBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }

    __forceinline void BeginQuery(VkQueryPool         queryPool,
                                  uint32_t            query,
                                  VkQueryControlFlags flags,
                                  uint32_t            queryFlag)
    {
        // queryPool must have been created with a queryType that differs from that of any queries that
        // are active within commandBuffer (17.2). In other words, only one query of given type can be active
        // in the command buffer.

        if ((m_State.InsidePassQueries | m_State.OutsidePassQueries) & queryFlag)
        {
            LOG_ERROR_MESSAGE("Another query of the same type is already active in the command buffer. "
                              "Overlapping queries are not allowed in Vulkan. The command will be ignored.");
            return;
        }

        // A query must either begin and end inside the same subpass of a render pass instance, or must both
        // begin and end outside of a render pass instance (i.e. contain entire render pass instances) (17.2).

        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdBeginQuery(m_VkCmdBuffer, queryPool, query, flags);
        if (m_State.RenderPass != VK_NULL_HANDLE)
            m_State.InsidePassQueries |= queryFlag;
        else
            m_State.OutsidePassQueries |= queryFlag;
    }

    __forceinline void EndQuery(VkQueryPool queryPool,
                                uint32_t    query,
                                uint32_t    queryFlag)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdEndQuery(m_VkCmdBuffer, queryPool, query);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            VERIFY((m_State.InsidePassQueries & queryFlag) != 0, "No active inside-pass queries found.");
            m_State.InsidePassQueries &= ~queryFlag;
        }
        else
        {
            VERIFY((m_State.OutsidePassQueries & queryFlag) != 0, "No active outside-pass queries found.");
            m_State.OutsidePassQueries &= ~queryFlag;
        }
    }

    __forceinline void WriteTimestamp(VkPipelineStageFlagBits pipelineStage,
                                      VkQueryPool             queryPool,
                                      uint32_t                query)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        vkCmdWriteTimestamp(m_VkCmdBuffer, pipelineStage, queryPool, query);
    }

    __forceinline void ResetQueryPool(VkQueryPool queryPool,
                                      uint32_t    firstQuery,
                                      uint32_t    queryCount)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Query pool reset must be performed outside of render pass (17.2).
            EndRenderPass();
        }
        vkCmdResetQueryPool(m_VkCmdBuffer, queryPool, firstQuery, queryCount);
    }

    __forceinline void CopyQueryPoolResults(VkQueryPool        queryPool,
                                            uint32_t           firstQuery,
                                            uint32_t           queryCount,
                                            VkBuffer           dstBuffer,
                                            VkDeviceSize       dstOffset,
                                            VkDeviceSize       stride,
                                            VkQueryResultFlags flags)
    {
        VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);
        if (m_State.RenderPass != VK_NULL_HANDLE)
        {
            // Copy query results must be performed outside of render pass (17.2).
            EndRenderPass();
        }
        vkCmdCopyQueryPoolResults(m_VkCmdBuffer, queryPool, firstQuery, queryCount,
                                  dstBuffer, dstOffset, stride, flags);
    }

    void FlushBarriers();

    __forceinline void SetVkCmdBuffer(VkCommandBuffer VkCmdBuffer)
    {
        m_VkCmdBuffer = VkCmdBuffer;
    }
    VkCommandBuffer GetVkCmdBuffer() const { return m_VkCmdBuffer; }

    struct StateCache
    {
        VkRenderPass  RenderPass         = VK_NULL_HANDLE;
        VkFramebuffer Framebuffer        = VK_NULL_HANDLE;
        VkPipeline    GraphicsPipeline   = VK_NULL_HANDLE;
        VkPipeline    ComputePipeline    = VK_NULL_HANDLE;
        VkBuffer      IndexBuffer        = VK_NULL_HANDLE;
        VkDeviceSize  IndexBufferOffset  = 0;
        VkIndexType   IndexType          = VK_INDEX_TYPE_MAX_ENUM;
        uint32_t      FramebufferWidth   = 0;
        uint32_t      FramebufferHeight  = 0;
        uint32_t      InsidePassQueries  = 0;
        uint32_t      OutsidePassQueries = 0;
    };

    const StateCache& GetState() const { return m_State; }

private:
    StateCache                 m_State;
    VkCommandBuffer            m_VkCmdBuffer = VK_NULL_HANDLE;
    const VkPipelineStageFlags m_EnabledGraphicsShaderStages;
};

} // namespace VulkanUtilities
