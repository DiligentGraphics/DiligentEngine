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
#include "Vulkan/TestingSwapChainVk.hpp"

#include "DeviceContextVk.h"
#include "TextureViewVk.h"
#include "TextureVk.h"

#include "volk/volk.h"

#include "InlineShaders/DrawCommandTestGLSL.h"

namespace Diligent
{

namespace Testing
{

namespace
{

struct ReferenceTriangleRenderer
{
    ReferenceTriangleRenderer(ISwapChain*           pSwapChain,
                              VkRenderPass          vkRenderPass,
                              VkSampleCountFlagBits SampleCount         = VK_SAMPLE_COUNT_1_BIT,
                              VkImageView           InputAttachmentView = VK_NULL_HANDLE)
    {
        auto* pEnv     = TestingEnvironmentVk::GetInstance();
        auto  vkDevice = pEnv->GetVkDevice();

        const auto& SCDesc = pSwapChain->GetDesc();

        VkResult res = VK_SUCCESS;
        (void)res;

        vkVSModule = pEnv->CreateShaderModule(SHADER_TYPE_VERTEX, GLSL::DrawTest_ProceduralTriangleVS);
        VERIFY_EXPR(vkVSModule != VK_NULL_HANDLE);
        vkPSModule = pEnv->CreateShaderModule(SHADER_TYPE_PIXEL, InputAttachmentView != VK_NULL_HANDLE ? GLSL::InputAttachmentTest_FS : GLSL::DrawTest_FS);
        VERIFY_EXPR(vkPSModule != VK_NULL_HANDLE);

        VkGraphicsPipelineCreateInfo PipelineCI = {};

        PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCI.pNext = nullptr;

        VkPipelineShaderStageCreateInfo ShaderStages[2] = {};

        ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStages[0].module = vkVSModule;
        ShaderStages[0].pName  = "main";

        ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStages[1].module = vkPSModule;
        ShaderStages[1].pName  = "main";

        PipelineCI.pStages    = ShaderStages;
        PipelineCI.stageCount = _countof(ShaderStages);

        VkPipelineLayoutCreateInfo PipelineLayoutCI = {};

        PipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        VkDescriptorSetLayout vkSetLayout[1] = {};
        if (InputAttachmentView != VK_NULL_HANDLE)
        {
            VkDescriptorSetLayoutBinding Bindings[1] = {};
            Bindings[0].binding                      = 0;
            Bindings[0].descriptorType               = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            Bindings[0].descriptorCount              = 1;
            Bindings[0].stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;
            Bindings[0].pImmutableSamplers           = nullptr;

            VkDescriptorSetLayoutCreateInfo SetLayoutCI = {};

            SetLayoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            SetLayoutCI.pNext        = nullptr;
            SetLayoutCI.flags        = 0;
            SetLayoutCI.bindingCount = 1;
            SetLayoutCI.pBindings    = Bindings;

            vkCreateDescriptorSetLayout(vkDevice, &SetLayoutCI, nullptr, &vkSetLayout[0]);
            VERIFY_EXPR(vkSetLayout[0] != VK_NULL_HANDLE);

            PipelineLayoutCI.setLayoutCount = 1;
            PipelineLayoutCI.pSetLayouts    = vkSetLayout;

            VkDescriptorPoolCreateInfo DescriptorPoolCI = {};
            DescriptorPoolCI.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            DescriptorPoolCI.pNext                      = nullptr;
            DescriptorPoolCI.flags                      = 0;
            DescriptorPoolCI.maxSets                    = 1;
            VkDescriptorPoolSize PoolSizes[]            = {{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1}};
            DescriptorPoolCI.poolSizeCount              = _countof(PoolSizes);
            DescriptorPoolCI.pPoolSizes                 = PoolSizes;

            vkCreateDescriptorPool(vkDevice, &DescriptorPoolCI, nullptr, &vkDescriptorPool);
            VERIFY_EXPR(vkDescriptorPool != VK_NULL_HANDLE);

            VkDescriptorSetAllocateInfo SetAllocateInfo = {};
            SetAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            SetAllocateInfo.pNext                       = nullptr;
            SetAllocateInfo.descriptorPool              = vkDescriptorPool;
            SetAllocateInfo.descriptorSetCount          = 1;
            SetAllocateInfo.pSetLayouts                 = vkSetLayout;

            vkAllocateDescriptorSets(vkDevice, &SetAllocateInfo, &vkDescriptorSet);
            VERIFY_EXPR(vkDescriptorSet != VK_NULL_HANDLE);

            VkWriteDescriptorSet DescriptorWrites[1] = {};

            DescriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].pNext           = nullptr;
            DescriptorWrites[0].dstSet          = vkDescriptorSet;
            DescriptorWrites[0].dstBinding      = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            VkDescriptorImageInfo ImageInfo[1]  = {};
            ImageInfo[0].sampler                = VK_NULL_HANDLE;
            ImageInfo[0].imageView              = InputAttachmentView;
            ImageInfo[0].imageLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            DescriptorWrites[0].pImageInfo      = ImageInfo;

            vkUpdateDescriptorSets(vkDevice, 1, DescriptorWrites, 0, nullptr);
        }
        vkCreatePipelineLayout(vkDevice, &PipelineLayoutCI, nullptr, &vkLayout);
        VERIFY_EXPR(vkLayout != VK_NULL_HANDLE);
        if (vkSetLayout[0] != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(vkDevice, vkSetLayout[0], nullptr);
        }
        PipelineCI.layout = vkLayout;

        VkPipelineVertexInputStateCreateInfo VertexInputStateCI = {};

        VertexInputStateCI.sType     = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        PipelineCI.pVertexInputState = &VertexInputStateCI;


        VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};

        InputAssemblyCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyCI.pNext                  = nullptr;
        InputAssemblyCI.flags                  = 0; // reserved for future use
        InputAssemblyCI.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemblyCI.primitiveRestartEnable = VK_FALSE;
        PipelineCI.pInputAssemblyState         = &InputAssemblyCI;


        VkPipelineTessellationStateCreateInfo TessStateCI = {};

        TessStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        TessStateCI.pNext             = nullptr;
        TessStateCI.flags             = 0; // reserved for future use
        PipelineCI.pTessellationState = &TessStateCI;


        VkPipelineViewportStateCreateInfo ViewPortStateCI = {};

        ViewPortStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewPortStateCI.pNext         = nullptr;
        ViewPortStateCI.flags         = 0; // reserved for future use
        ViewPortStateCI.viewportCount = 1;

        VkViewport Viewport = {};
        Viewport.y          = static_cast<float>(SCDesc.Height);
        Viewport.width      = static_cast<float>(SCDesc.Width);
        Viewport.height     = -static_cast<float>(SCDesc.Height);
        Viewport.maxDepth   = 1;

        ViewPortStateCI.pViewports   = &Viewport;
        ViewPortStateCI.scissorCount = ViewPortStateCI.viewportCount; // the number of scissors must match the number of viewports (23.5)
                                                                      // (why the hell it is in the struct then?)

        VkRect2D ScissorRect      = {};
        ScissorRect.extent.width  = SCDesc.Width;
        ScissorRect.extent.height = SCDesc.Height;
        ViewPortStateCI.pScissors = &ScissorRect;

        PipelineCI.pViewportState = &ViewPortStateCI;

        VkPipelineRasterizationStateCreateInfo RasterizerStateCI = {};

        RasterizerStateCI.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizerStateCI.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizerStateCI.cullMode    = VK_CULL_MODE_NONE;
        RasterizerStateCI.lineWidth   = 1;

        PipelineCI.pRasterizationState = &RasterizerStateCI;

        // Multisample state (24)
        VkPipelineMultisampleStateCreateInfo MSStateCI = {};

        MSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MSStateCI.pNext = nullptr;
        MSStateCI.flags = 0; // reserved for future use
        // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of
        // pMultisampleState must be the same as the sample count for those subpass attachments
        MSStateCI.rasterizationSamples = SampleCount;
        MSStateCI.sampleShadingEnable  = VK_FALSE;
        MSStateCI.minSampleShading     = 0;               // a minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
        uint32_t SampleMask[]          = {0xFFFFFFFF, 0}; // Vulkan spec allows up to 64 samples
        MSStateCI.pSampleMask          = SampleMask;      // an array of static coverage information that is ANDed with
                                                          // the coverage information generated during rasterization (25.3)
        MSStateCI.alphaToCoverageEnable = VK_FALSE;       // whether a temporary coverage value is generated based on
                                                          // the alpha component of the fragment's first color output
        MSStateCI.alphaToOneEnable   = VK_FALSE;          // whether the alpha component of the fragment's first color output is replaced with one
        PipelineCI.pMultisampleState = &MSStateCI;


        VkPipelineDepthStencilStateCreateInfo DepthStencilStateCI = {};

        DepthStencilStateCI.sType     = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        PipelineCI.pDepthStencilState = &DepthStencilStateCI;


        VkPipelineColorBlendStateCreateInfo BlendStateCI = {};

        VkPipelineColorBlendAttachmentState Attachment = {};

        Attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        BlendStateCI.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        BlendStateCI.pAttachments    = &Attachment;
        BlendStateCI.attachmentCount = 1; //  must equal the colorAttachmentCount for the subpass
                                          // in which this pipeline is used.
        PipelineCI.pColorBlendState = &BlendStateCI;


        VkPipelineDynamicStateCreateInfo DynamicStateCI = {};

        DynamicStateCI.sType     = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        PipelineCI.pDynamicState = &DynamicStateCI;

        PipelineCI.renderPass         = vkRenderPass;
        PipelineCI.subpass            = InputAttachmentView != VK_NULL_HANDLE ? 1 : 0;
        PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
        PipelineCI.basePipelineIndex  = 0;              // an index into the pCreateInfos parameter to use as a pipeline to derive from

        res = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipeline);
        VERIFY_EXPR(res >= 0);
        VERIFY_EXPR(vkPipeline != VK_NULL_HANDLE);
    }

    void Draw(VkCommandBuffer vkCmdBuffer)
    {
        if (vkDescriptorSet != VK_NULL_HANDLE)
            vkCmdBindDescriptorSets(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkLayout, 0, 1, &vkDescriptorSet, 0, nullptr);
        vkCmdBindPipeline(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
        vkCmdDraw(vkCmdBuffer, 6, 1, 0, 0);
    }

    ~ReferenceTriangleRenderer()
    {
        auto* pEnv     = TestingEnvironmentVk::GetInstance();
        auto  vkDevice = pEnv->GetVkDevice();

        vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
        vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
        vkDestroyShaderModule(vkDevice, vkVSModule, nullptr);
        vkDestroyShaderModule(vkDevice, vkPSModule, nullptr);
        if (vkDescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
    }

private:
    VkShaderModule   vkVSModule       = VK_NULL_HANDLE;
    VkShaderModule   vkPSModule       = VK_NULL_HANDLE;
    VkPipeline       vkPipeline       = VK_NULL_HANDLE;
    VkPipelineLayout vkLayout         = VK_NULL_HANDLE;
    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet  vkDescriptorSet  = VK_NULL_HANDLE;
};

} // namespace

void RenderDrawCommandReferenceVk(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv = TestingEnvironmentVk::GetInstance();

    auto* pTestingSwapChainVk = ValidatedCast<TestingSwapChainVk>(pSwapChain);

    ReferenceTriangleRenderer TriRenderer{pSwapChain, pTestingSwapChainVk->GetRenderPass()};

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    pTestingSwapChainVk->BeginRenderPass(vkCmdBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, pClearColor);
    TriRenderer.Draw(vkCmdBuffer);
    pTestingSwapChainVk->EndRenderPass(vkCmdBuffer);
    vkEndCommandBuffer(vkCmdBuffer);
    pEnv->SubmitCommandBuffer(vkCmdBuffer, true);
}

void RenderPassMSResolveReferenceVk(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv     = TestingEnvironmentVk::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto  vkDevice = pEnv->GetVkDevice();

    auto*       pTestingSwapChainVk = ValidatedCast<TestingSwapChainVk>(pSwapChain);
    const auto& SCDesc              = pTestingSwapChainVk->GetDesc();

    VkRenderPassCreateInfo RenderPassCI = {};

    RenderPassCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCI.pNext           = nullptr;
    RenderPassCI.flags           = 0; // reserved for future use
    RenderPassCI.attachmentCount = 2;

    VkAttachmentDescription Attachments[2] = {};

    Attachments[0].flags          = 0;
    Attachments[0].samples        = VK_SAMPLE_COUNT_4_BIT;
    Attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[0].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    switch (SCDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            Attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
            break;

        default:
            UNSUPPORTED("Unsupported swap chain format");
    }

    Attachments[1].flags          = 0;
    Attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    Attachments[1].format         = Attachments[0].format;
    Attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    Attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[1].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Attachments[1].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkAttachmentReference ColorAttachmentRef[] = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    VkAttachmentReference RslvAttachmentRef[]  = {{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

    VkSubpassDescription Subpasses[1] = {};
    Subpasses[0].flags                = 0;
    Subpasses[0].pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpasses[0].colorAttachmentCount = _countof(ColorAttachmentRef);
    Subpasses[0].pColorAttachments    = ColorAttachmentRef;
    Subpasses[0].pResolveAttachments  = RslvAttachmentRef;

    RenderPassCI.attachmentCount = _countof(Attachments);
    RenderPassCI.pAttachments    = Attachments;
    RenderPassCI.subpassCount    = _countof(Subpasses);
    RenderPassCI.pSubpasses      = Subpasses;

    VkRenderPass vkRenderPass = VK_NULL_HANDLE;
    vkCreateRenderPass(vkDevice, &RenderPassCI, nullptr, &vkRenderPass);
    ASSERT_TRUE(vkRenderPass != VK_NULL_HANDLE);

    ReferenceTriangleRenderer TriRenderer{pSwapChain, vkRenderPass, Attachments[0].samples};

    RefCntAutoPtr<ITexture> pMSTex;
    {
        TextureDesc TexDesc;
        TexDesc.Type        = RESOURCE_DIM_TEX_2D;
        TexDesc.Format      = SCDesc.ColorBufferFormat;
        TexDesc.Width       = SCDesc.Width;
        TexDesc.Height      = SCDesc.Height;
        TexDesc.BindFlags   = BIND_RENDER_TARGET;
        TexDesc.MipLevels   = 1;
        TexDesc.SampleCount = 4;
        TexDesc.Usage       = USAGE_DEFAULT;

        pDevice->CreateTexture(TexDesc, nullptr, &pMSTex);
        ASSERT_NE(pMSTex, nullptr);
    }
    RefCntAutoPtr<ITextureVk> pMSTexVK{pMSTex, IID_TextureVk};
    ASSERT_NE(pMSTexVK, nullptr);
    RefCntAutoPtr<ITextureViewVk> pMSTexViewVK{pMSTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET), IID_TextureViewVk};
    ASSERT_NE(pMSTexViewVK, nullptr);

    VkFramebufferCreateInfo FramebufferCI = {};

    FramebufferCI.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCI.pNext      = nullptr;
    FramebufferCI.flags      = 0;
    FramebufferCI.renderPass = vkRenderPass;

    VkImageView FramebufferAttachments[2];
    FramebufferAttachments[0]     = pMSTexViewVK->GetVulkanImageView();
    FramebufferAttachments[1]     = pTestingSwapChainVk->GetVkRenderTargetImageView();
    FramebufferCI.pAttachments    = FramebufferAttachments;
    FramebufferCI.attachmentCount = _countof(FramebufferAttachments);

    FramebufferCI.width  = SCDesc.Width;
    FramebufferCI.height = SCDesc.Height;
    FramebufferCI.layers = 1;

    VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
    vkCreateFramebuffer(vkDevice, &FramebufferCI, nullptr, &vkFramebuffer);
    ASSERT_TRUE(vkFramebuffer != VK_NULL_HANDLE);

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();
    pTestingSwapChainVk->TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    {
        auto CurrLayout = pMSTexVK->GetLayout();

        VkImageSubresourceRange SubResRange;
        SubResRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        SubResRange.baseMipLevel   = 0;
        SubResRange.levelCount     = 1;
        SubResRange.baseArrayLayer = 0;
        SubResRange.layerCount     = 1;
        pEnv->TransitionImageLayout(vkCmdBuffer, pMSTexVK->GetVkImage(), CurrLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, SubResRange, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    VkRenderPassBeginInfo BeginInfo = {};

    BeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    BeginInfo.renderPass        = vkRenderPass;
    BeginInfo.framebuffer       = vkFramebuffer;
    BeginInfo.renderArea.extent = VkExtent2D{SCDesc.Width, SCDesc.Height};

    VkClearValue ClearValues[1] = {};

    ClearValues[0].color.float32[0] = pClearColor[0];
    ClearValues[0].color.float32[1] = pClearColor[1];
    ClearValues[0].color.float32[2] = pClearColor[2];
    ClearValues[0].color.float32[3] = pClearColor[3];

    BeginInfo.clearValueCount = _countof(ClearValues);
    BeginInfo.pClearValues    = ClearValues;

    vkCmdBeginRenderPass(vkCmdBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    TriRenderer.Draw(vkCmdBuffer);
    vkCmdEndRenderPass(vkCmdBuffer);
    vkEndCommandBuffer(vkCmdBuffer);
    pEnv->SubmitCommandBuffer(vkCmdBuffer, true);

    vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
    vkDestroyFramebuffer(vkDevice, vkFramebuffer, nullptr);
}

void RenderPassInputAttachmentReferenceVk(ISwapChain* pSwapChain, const float* pClearColor)
{
    auto* pEnv     = TestingEnvironmentVk::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto  vkDevice = pEnv->GetVkDevice();

    auto*       pTestingSwapChainVk = ValidatedCast<TestingSwapChainVk>(pSwapChain);
    const auto& SCDesc              = pTestingSwapChainVk->GetDesc();

    VkRenderPassCreateInfo RenderPassCI = {};

    RenderPassCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCI.pNext           = nullptr;
    RenderPassCI.flags           = 0; // reserved for future use
    RenderPassCI.attachmentCount = 2;

    VkAttachmentDescription Attachments[2] = {};

    Attachments[0].flags          = 0;
    Attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    Attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[0].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    switch (SCDesc.ColorBufferFormat)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            Attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
            break;

        default:
            UNSUPPORTED("Unsupported swap chain format");
    }

    Attachments[1].flags          = 0;
    Attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    Attachments[1].format         = Attachments[0].format;
    Attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    Attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[1].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Attachments[1].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ColorAttachmentRef0[] = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    VkAttachmentReference InputAttachmentRef1[] = {{0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkAttachmentReference ColorAttachmentRef1[] = {{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

    VkSubpassDescription Subpasses[2] = {};
    Subpasses[0].flags                = 0;
    Subpasses[0].pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpasses[0].colorAttachmentCount = _countof(ColorAttachmentRef0);
    Subpasses[0].pColorAttachments    = ColorAttachmentRef0;

    Subpasses[1].flags                = 0;
    Subpasses[1].pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpasses[1].colorAttachmentCount = _countof(ColorAttachmentRef1);
    Subpasses[1].pColorAttachments    = ColorAttachmentRef1;
    Subpasses[1].inputAttachmentCount = _countof(InputAttachmentRef1);
    Subpasses[1].pInputAttachments    = InputAttachmentRef1;

    VkSubpassDependency dependencies[1] = {};
    dependencies[0].srcSubpass          = 0;
    dependencies[0].dstSubpass          = 1;
    dependencies[0].srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;

    RenderPassCI.attachmentCount = _countof(Attachments);
    RenderPassCI.pAttachments    = Attachments;
    RenderPassCI.subpassCount    = _countof(Subpasses);
    RenderPassCI.pSubpasses      = Subpasses;
    RenderPassCI.dependencyCount = _countof(dependencies);
    RenderPassCI.pDependencies   = dependencies;

    VkRenderPass vkRenderPass = VK_NULL_HANDLE;
    vkCreateRenderPass(vkDevice, &RenderPassCI, nullptr, &vkRenderPass);
    ASSERT_TRUE(vkRenderPass != VK_NULL_HANDLE);

    ReferenceTriangleRenderer TriRenderer{pSwapChain, vkRenderPass};
    RefCntAutoPtr<ITexture>   pTex;
    {
        TextureDesc TexDesc;
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = SCDesc.ColorBufferFormat;
        TexDesc.Width     = SCDesc.Width;
        TexDesc.Height    = SCDesc.Height;
        TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_INPUT_ATTACHMENT;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;

        pDevice->CreateTexture(TexDesc, nullptr, &pTex);
        ASSERT_NE(pTex, nullptr);
    }
    RefCntAutoPtr<ITextureVk> pTexVK{pTex, IID_TextureVk};
    ASSERT_NE(pTexVK, nullptr);
    RefCntAutoPtr<ITextureViewVk> pTexViewVK{pTex->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET), IID_TextureViewVk};
    ASSERT_NE(pTexViewVK, nullptr);

    ReferenceTriangleRenderer TriRenderer2{pSwapChain, vkRenderPass, VK_SAMPLE_COUNT_1_BIT, pTexViewVK->GetVulkanImageView()};

    VkFramebufferCreateInfo FramebufferCI = {};

    FramebufferCI.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCI.pNext      = nullptr;
    FramebufferCI.flags      = 0;
    FramebufferCI.renderPass = vkRenderPass;

    VkImageView FramebufferAttachments[2];
    FramebufferAttachments[0]     = pTexViewVK->GetVulkanImageView();
    FramebufferAttachments[1]     = pTestingSwapChainVk->GetVkRenderTargetImageView();
    FramebufferCI.pAttachments    = FramebufferAttachments;
    FramebufferCI.attachmentCount = _countof(FramebufferAttachments);

    FramebufferCI.width  = SCDesc.Width;
    FramebufferCI.height = SCDesc.Height;
    FramebufferCI.layers = 1;

    VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
    vkCreateFramebuffer(vkDevice, &FramebufferCI, nullptr, &vkFramebuffer);
    ASSERT_TRUE(vkFramebuffer != VK_NULL_HANDLE);

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();
    pTestingSwapChainVk->TransitionRenderTarget(vkCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    {
        auto CurrLayout = pTexVK->GetLayout();

        VkImageSubresourceRange SubResRange;
        SubResRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        SubResRange.baseMipLevel   = 0;
        SubResRange.levelCount     = 1;
        SubResRange.baseArrayLayer = 0;
        SubResRange.layerCount     = 1;
        pEnv->TransitionImageLayout(vkCmdBuffer, pTexVK->GetVkImage(), CurrLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, SubResRange, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    VkRenderPassBeginInfo BeginInfo = {};

    BeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    BeginInfo.renderPass        = vkRenderPass;
    BeginInfo.framebuffer       = vkFramebuffer;
    BeginInfo.renderArea.extent = VkExtent2D{SCDesc.Width, SCDesc.Height};

    VkClearValue ClearValues[2] = {};

    ClearValues[0].color.float32[0] = 0;
    ClearValues[0].color.float32[1] = 0;
    ClearValues[0].color.float32[2] = 0;
    ClearValues[0].color.float32[3] = 0;

    ClearValues[1].color.float32[0] = pClearColor[0];
    ClearValues[1].color.float32[1] = pClearColor[1];
    ClearValues[1].color.float32[2] = pClearColor[2];
    ClearValues[1].color.float32[3] = pClearColor[3];

    BeginInfo.clearValueCount = _countof(ClearValues);
    BeginInfo.pClearValues    = ClearValues;

    vkCmdBeginRenderPass(vkCmdBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    TriRenderer.Draw(vkCmdBuffer);
    vkCmdNextSubpass(vkCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
    TriRenderer2.Draw(vkCmdBuffer);
    vkCmdEndRenderPass(vkCmdBuffer);
    vkEndCommandBuffer(vkCmdBuffer);
    pEnv->SubmitCommandBuffer(vkCmdBuffer, true);

    vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
    vkDestroyFramebuffer(vkDevice, vkFramebuffer, nullptr);
}

} // namespace Testing

} // namespace Diligent
