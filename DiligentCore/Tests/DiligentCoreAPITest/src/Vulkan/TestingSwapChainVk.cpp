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

#include "Vulkan/TestingSwapChainVk.hpp"
#include "Vulkan/TestingEnvironmentVk.hpp"

#include "RenderDeviceVk.h"
#include "DeviceContextVk.h"

#include "volk/volk.h"

namespace Diligent
{

namespace Testing
{

TestingSwapChainVk::TestingSwapChainVk(IReferenceCounters*   pRefCounters,
                                       TestingEnvironmentVk* pEnv,
                                       const SwapChainDesc&  SCDesc) :
    TBase //
    {
        pRefCounters,
        pEnv->GetDevice(),
        pEnv->GetDeviceContext(),
        SCDesc //
    }
{
    m_vkDevice = pEnv->GetVkDevice();

    VkFormat ColorFormat = VK_FORMAT_UNDEFINED;
    switch (m_SwapChainDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            ColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
            break;

        default:
            UNSUPPORTED("Texture format ", GetTextureFormatAttribs(m_SwapChainDesc.ColorBufferFormat).Name, " is not a supported color buffer format");
    }
    pEnv->CreateImage2D(m_SwapChainDesc.Width, m_SwapChainDesc.Height, ColorFormat,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                        m_vkRenerTargetLayout,
                        m_vkRenderTargetMemory, m_vkRenderTargetImage);


    VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
    switch (m_SwapChainDesc.DepthBufferFormat)
    {
        case TEX_FORMAT_D32_FLOAT:
            DepthFormat = VK_FORMAT_D32_SFLOAT;
            break;

        default:
            UNSUPPORTED("Texture format ", GetTextureFormatAttribs(m_SwapChainDesc.DepthBufferFormat).Name, " is not a supported depth buffer format");
    }
    pEnv->CreateImage2D(m_SwapChainDesc.Width, m_SwapChainDesc.Height, DepthFormat,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                        m_vkDepthBufferLayout,
                        m_vkDepthBufferMemory, m_vkDepthBufferImage);

    {
        VERIFY(m_SwapChainDesc.ColorBufferFormat == TEX_FORMAT_RGBA8_UNORM, "Unexpected color buffer format");
        m_StagingBufferSize = m_SwapChainDesc.Width * m_SwapChainDesc.Height * 4;
        pEnv->CreateBuffer(m_StagingBufferSize, VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                           m_vkStagingBufferMemory, m_vkStagingBuffer);
    }

    VkImageViewCreateInfo ImageViewCI = {};

    ImageViewCI.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCI.pNext        = nullptr;
    ImageViewCI.flags        = 0; // reserved for future use.
    ImageViewCI.image        = m_vkRenderTargetImage;
    ImageViewCI.format       = ColorFormat;
    ImageViewCI.viewType     = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCI.subresourceRange.levelCount = 1;
    ImageViewCI.subresourceRange.layerCount = 1;

    auto res = vkCreateImageView(m_vkDevice, &ImageViewCI, nullptr, &m_vkRenderTargetView);
    VERIFY_EXPR(res >= 0);

    ImageViewCI.image                       = m_vkDepthBufferImage;
    ImageViewCI.format                      = DepthFormat;
    ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    res = vkCreateImageView(m_vkDevice, &ImageViewCI, nullptr, &m_vkDepthBufferView);
    VERIFY_EXPR(res >= 0);


    std::array<VkAttachmentDescription, MAX_RENDER_TARGETS + 1> Attachments;
    std::array<VkAttachmentReference, MAX_RENDER_TARGETS + 1>   AttachmentReferences;

    VkSubpassDescription Subpass;

    auto RenderPassCI =
        TestingEnvironmentVk::GetRenderPassCreateInfo(1, &ColorFormat, DepthFormat, 1,
                                                      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                      Attachments, AttachmentReferences, Subpass);
    res = vkCreateRenderPass(m_vkDevice, &RenderPassCI, nullptr, &m_vkRenderPass);
    VERIFY_EXPR(res >= 0);

    CreateFramebuffer();
}

void TestingSwapChainVk::CreateFramebuffer()
{
    VkFramebufferCreateInfo FramebufferCI = {};

    FramebufferCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCI.pNext           = nullptr;
    FramebufferCI.flags           = 0; // reserved for future use
    FramebufferCI.renderPass      = m_vkRenderPass;
    FramebufferCI.attachmentCount = 2;
    VkImageView Attachments[2]    = {m_vkDepthBufferView, m_vkRenderTargetView};
    FramebufferCI.pAttachments    = Attachments;
    FramebufferCI.width           = m_SwapChainDesc.Width;
    FramebufferCI.height          = m_SwapChainDesc.Height;
    FramebufferCI.layers          = 1;

    auto res = vkCreateFramebuffer(m_vkDevice, &FramebufferCI, nullptr, &m_vkFramebuffer);
    VERIFY_EXPR(res >= 0);
    (void)res;
}

TestingSwapChainVk::~TestingSwapChainVk()
{
    if (m_vkRenderTargetImage != VK_NULL_HANDLE)
        vkDestroyImage(m_vkDevice, m_vkRenderTargetImage, nullptr);
    if (m_vkRenderTargetMemory != VK_NULL_HANDLE)
        vkFreeMemory(m_vkDevice, m_vkRenderTargetMemory, nullptr);

    if (m_vkDepthBufferImage != VK_NULL_HANDLE)
        vkDestroyImage(m_vkDevice, m_vkDepthBufferImage, nullptr);
    if (m_vkDepthBufferMemory != VK_NULL_HANDLE)
        vkFreeMemory(m_vkDevice, m_vkDepthBufferMemory, nullptr);

    if (m_vkStagingBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(m_vkDevice, m_vkStagingBuffer, nullptr);
    if (m_vkStagingBufferMemory != VK_NULL_HANDLE)
        vkFreeMemory(m_vkDevice, m_vkStagingBufferMemory, nullptr);

    if (m_vkRenderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);
    if (m_vkFramebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(m_vkDevice, m_vkFramebuffer, nullptr);

    if (m_vkRenderTargetView != VK_NULL_HANDLE)
        vkDestroyImageView(m_vkDevice, m_vkRenderTargetView, nullptr);
    if (m_vkDepthBufferView != VK_NULL_HANDLE)
        vkDestroyImageView(m_vkDevice, m_vkDepthBufferView, nullptr);
}

void TestingSwapChainVk::TransitionRenderTarget(VkCommandBuffer vkCmdBuffer, VkImageLayout Layout, VkPipelineStageFlags GraphicsShaderStages)
{
    m_ActiveGraphicsShaderStages        = GraphicsShaderStages;
    VkImageSubresourceRange SubresRange = {};

    SubresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    SubresRange.layerCount = 1;
    SubresRange.levelCount = 1;
    TestingEnvironmentVk::TransitionImageLayout(vkCmdBuffer, m_vkRenderTargetImage, m_vkRenerTargetLayout,
                                                Layout, SubresRange, GraphicsShaderStages);
}

void TestingSwapChainVk::TransitionDepthBuffer(VkCommandBuffer vkCmdBuffer, VkImageLayout Layout, VkPipelineStageFlags GraphicsShaderStages)
{
    m_ActiveGraphicsShaderStages        = GraphicsShaderStages;
    VkImageSubresourceRange SubresRange = {};

    SubresRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    SubresRange.layerCount = 1;
    SubresRange.levelCount = 1;
    TestingEnvironmentVk::TransitionImageLayout(vkCmdBuffer, m_vkDepthBufferImage, m_vkDepthBufferLayout,
                                                Layout, SubresRange, GraphicsShaderStages);
}

void TestingSwapChainVk::BeginRenderPass(VkCommandBuffer vkCmdBuffer, VkPipelineStageFlags GraphicsShaderStages, const float* pClearColor)
{
    TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, GraphicsShaderStages);
    TransitionDepthBuffer(vkCmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, GraphicsShaderStages);

    VkRenderPassBeginInfo BeginInfo = {};

    BeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    BeginInfo.renderPass        = m_vkRenderPass;
    BeginInfo.framebuffer       = m_vkFramebuffer;
    BeginInfo.renderArea.extent = VkExtent2D{m_SwapChainDesc.Width, m_SwapChainDesc.Height};

    VkClearValue ClearValues[2] = {};

    ClearValues[0].depthStencil.depth = 1;
    ClearValues[1].color.float32[0]   = (pClearColor != nullptr) ? pClearColor[0] : 0;
    ClearValues[1].color.float32[1]   = (pClearColor != nullptr) ? pClearColor[1] : 0;
    ClearValues[1].color.float32[2]   = (pClearColor != nullptr) ? pClearColor[2] : 0;
    ClearValues[1].color.float32[3]   = (pClearColor != nullptr) ? pClearColor[3] : 0;

    BeginInfo.clearValueCount = 2;
    BeginInfo.pClearValues    = ClearValues;

    vkCmdBeginRenderPass(vkCmdBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void TestingSwapChainVk::EndRenderPass(VkCommandBuffer vkCmdBuffer)
{
    vkCmdEndRenderPass(vkCmdBuffer);
}

void TestingSwapChainVk::TakeSnapshot()
{
    auto* pEnv     = TestingEnvironmentVk::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_ActiveGraphicsShaderStages);

    VkBufferImageCopy BuffImgCopy = {};

    BuffImgCopy.imageExtent                 = VkExtent3D{m_SwapChainDesc.Width, m_SwapChainDesc.Height, 1};
    BuffImgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    BuffImgCopy.imageSubresource.layerCount = 1;
    VERIFY(m_SwapChainDesc.ColorBufferFormat == TEX_FORMAT_RGBA8_UNORM, "Unexpected color buffer format");
    BuffImgCopy.bufferRowLength = m_SwapChainDesc.Width; // In texels

    vkCmdCopyImageToBuffer(vkCmdBuffer, m_vkRenderTargetImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           m_vkStagingBuffer, 1, &BuffImgCopy);
    vkEndCommandBuffer(vkCmdBuffer);


    RefCntAutoPtr<IDeviceContextVk> pContextVk{pContext, IID_DeviceContextVk};

    auto* pQeueVk = pContextVk->LockCommandQueue();
    auto  vkQueue = pQeueVk->GetVkQueue();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pCommandBuffers    = &vkCmdBuffer;
    SubmitInfo.commandBufferCount = 1;
    vkQueueSubmit(vkQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkQueue);

    VERIFY_EXPR(m_StagingBufferSize == m_SwapChainDesc.Width * m_SwapChainDesc.Height * 4);
    void* pStagingDataPtr = nullptr;
    vkMapMemory(m_vkDevice, m_vkStagingBufferMemory, 0, m_StagingBufferSize, 0, &pStagingDataPtr);

    m_ReferenceDataPitch = m_SwapChainDesc.Width * 4;
    m_ReferenceData.resize(m_SwapChainDesc.Height * m_ReferenceDataPitch);
    for (Uint32 row = 0; row < m_SwapChainDesc.Height; ++row)
    {
        memcpy(&m_ReferenceData[row * m_ReferenceDataPitch],
               reinterpret_cast<const Uint8*>(pStagingDataPtr) + m_SwapChainDesc.Width * 4 * row,
               m_ReferenceDataPitch);
    }

    vkUnmapMemory(m_vkDevice, m_vkStagingBufferMemory);
    pContextVk->UnlockCommandQueue();
}

void CreateTestingSwapChainVk(TestingEnvironmentVk* pEnv,
                              const SwapChainDesc&  SCDesc,
                              ISwapChain**          ppSwapChain)
{
    TestingSwapChainVk* pTestingSC(MakeNewRCObj<TestingSwapChainVk>()(pEnv, SCDesc));
    pTestingSC->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain));
}

} // namespace Testing

} // namespace Diligent
