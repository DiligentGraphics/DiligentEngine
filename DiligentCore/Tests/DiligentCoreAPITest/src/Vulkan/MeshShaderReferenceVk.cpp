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

#include "volk/volk.h"

#include "InlineShaders/MeshShaderTestGLSL.h"

namespace Diligent
{

namespace Testing
{

void MeshShaderDrawReferenceVk(ISwapChain* pSwapChain)
{
    auto* pEnv     = TestingEnvironmentVk::GetInstance();
    auto  vkDevice = pEnv->GetVkDevice();
    auto* pContext = pEnv->GetDeviceContext();

    const auto& SCDesc = pSwapChain->GetDesc();

    VkResult res = VK_SUCCESS;
    (void)res;

    auto* pTestingSwapChainVk = ValidatedCast<TestingSwapChainVk>(pSwapChain);

    auto vkMSModule = pEnv->CreateShaderModule(SHADER_TYPE_MESH, GLSL::MeshShaderTest_MS);
    ASSERT_TRUE(vkMSModule != VK_NULL_HANDLE);
    auto vkFSModule = pEnv->CreateShaderModule(SHADER_TYPE_PIXEL, GLSL::MeshShaderTest_FS);
    ASSERT_TRUE(vkFSModule != VK_NULL_HANDLE);

    VkGraphicsPipelineCreateInfo PipelineCI = {};

    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};

    ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage  = VK_SHADER_STAGE_MESH_BIT_NV;
    ShaderStages[0].module = vkMSModule;
    ShaderStages[0].pName  = "main";

    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = vkFSModule;
    ShaderStages[1].pName  = "main";

    PipelineCI.pStages    = ShaderStages;
    PipelineCI.stageCount = _countof(ShaderStages);

    VkPipelineLayoutCreateInfo PipelineLayoutCI = {};

    PipelineLayoutCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout vkLayout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(vkDevice, &PipelineLayoutCI, nullptr, &vkLayout);
    ASSERT_TRUE(vkLayout != VK_NULL_HANDLE);
    PipelineCI.layout = vkLayout;


    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};

    InputAssemblyCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.topology       = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    PipelineCI.pInputAssemblyState = &InputAssemblyCI;


    VkPipelineViewportStateCreateInfo ViewPortStateCI = {};

    ViewPortStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
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
    RasterizerStateCI.cullMode    = VK_CULL_MODE_BACK_BIT;
    RasterizerStateCI.frontFace   = VK_FRONT_FACE_CLOCKWISE;
    RasterizerStateCI.lineWidth   = 1;

    PipelineCI.pRasterizationState = &RasterizerStateCI;

    // Multisample state (24)
    VkPipelineMultisampleStateCreateInfo MSStateCI = {};

    MSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of
    // pMultisampleState must be the same as the sample count for those subpass attachments
    MSStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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

    PipelineCI.renderPass         = pTestingSwapChainVk->GetRenderPass();
    PipelineCI.subpass            = 0;
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex  = 0;              // an index into the pCreateInfos parameter to use as a pipeline to derive from

    VkPipeline vkPipeline = VK_NULL_HANDLE;
    res                   = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipeline);
    ASSERT_GE(res, 0);
    ASSERT_TRUE(vkPipeline != VK_NULL_HANDLE);

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    pTestingSwapChainVk->BeginRenderPass(vkCmdBuffer, VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkCmdBindPipeline(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
    vkCmdDrawMeshTasksNV(vkCmdBuffer, 1, 0);
    pTestingSwapChainVk->EndRenderPass(vkCmdBuffer);
    res = vkEndCommandBuffer(vkCmdBuffer);
    VERIFY(res >= 0, "Failed to end command buffer");

    RefCntAutoPtr<IDeviceContextVk> pContextVk{pContext, IID_DeviceContextVk};

    auto* pQeueVk = pContextVk->LockCommandQueue();
    auto  vkQueue = pQeueVk->GetVkQueue();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pCommandBuffers    = &vkCmdBuffer;
    SubmitInfo.commandBufferCount = 1;
    vkQueueSubmit(vkQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkQueue);

    pContextVk->UnlockCommandQueue();

    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
    vkDestroyShaderModule(vkDevice, vkMSModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkFSModule, nullptr);
}


void MeshShaderIndirectDrawReferenceVk(ISwapChain* pSwapChain)
{
    auto* pEnv     = TestingEnvironmentVk::GetInstance();
    auto  vkDevice = pEnv->GetVkDevice();
    auto* pContext = pEnv->GetDeviceContext();

    const auto& SCDesc = pSwapChain->GetDesc();

    VkResult res = VK_SUCCESS;
    (void)res;

    auto* pTestingSwapChainVk = ValidatedCast<TestingSwapChainVk>(pSwapChain);

    auto vkMSModule = pEnv->CreateShaderModule(SHADER_TYPE_MESH, GLSL::MeshShaderTest_MS);
    ASSERT_TRUE(vkMSModule != VK_NULL_HANDLE);
    auto vkFSModule = pEnv->CreateShaderModule(SHADER_TYPE_PIXEL, GLSL::MeshShaderTest_FS);
    ASSERT_TRUE(vkFSModule != VK_NULL_HANDLE);

    VkGraphicsPipelineCreateInfo PipelineCI = {};

    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};

    ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage  = VK_SHADER_STAGE_MESH_BIT_NV;
    ShaderStages[0].module = vkMSModule;
    ShaderStages[0].pName  = "main";

    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = vkFSModule;
    ShaderStages[1].pName  = "main";

    PipelineCI.pStages    = ShaderStages;
    PipelineCI.stageCount = _countof(ShaderStages);

    VkPipelineLayoutCreateInfo PipelineLayoutCI = {};

    PipelineLayoutCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout vkLayout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(vkDevice, &PipelineLayoutCI, nullptr, &vkLayout);
    ASSERT_TRUE(vkLayout != VK_NULL_HANDLE);
    PipelineCI.layout = vkLayout;


    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};

    InputAssemblyCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.topology       = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    PipelineCI.pInputAssemblyState = &InputAssemblyCI;


    VkPipelineViewportStateCreateInfo ViewPortStateCI = {};

    ViewPortStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
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
    RasterizerStateCI.cullMode    = VK_CULL_MODE_BACK_BIT;
    RasterizerStateCI.frontFace   = VK_FRONT_FACE_CLOCKWISE;
    RasterizerStateCI.lineWidth   = 1;

    PipelineCI.pRasterizationState = &RasterizerStateCI;

    // Multisample state (24)
    VkPipelineMultisampleStateCreateInfo MSStateCI = {};

    MSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of
    // pMultisampleState must be the same as the sample count for those subpass attachments
    MSStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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

    PipelineCI.renderPass         = pTestingSwapChainVk->GetRenderPass();
    PipelineCI.subpass            = 0;
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex  = 0;              // an index into the pCreateInfos parameter to use as a pipeline to derive from

    VkPipeline vkPipeline = VK_NULL_HANDLE;
    res                   = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipeline);
    ASSERT_GE(res, 0);
    ASSERT_TRUE(vkPipeline != VK_NULL_HANDLE);

    VkDeviceMemory vkBufferMemory;
    VkBuffer       vkIndirectBuffer;
    pEnv->CreateBuffer(16, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkBufferMemory, vkIndirectBuffer);

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    Uint32 taskData[2] = {1, 0}; // count, first
    vkCmdUpdateBuffer(vkCmdBuffer, vkIndirectBuffer, 0, sizeof(taskData), &taskData);

    VkBufferMemoryBarrier barrier = {};
    barrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask         = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask         = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    barrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer                = vkIndirectBuffer;
    barrier.offset                = 0;
    barrier.size                  = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(vkCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

    pTestingSwapChainVk->BeginRenderPass(vkCmdBuffer, VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkCmdBindPipeline(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
    vkCmdDrawMeshTasksIndirectNV(vkCmdBuffer, vkIndirectBuffer, 0, 1, 0);
    pTestingSwapChainVk->EndRenderPass(vkCmdBuffer);
    res = vkEndCommandBuffer(vkCmdBuffer);
    VERIFY(res >= 0, "Failed to end command buffer");

    RefCntAutoPtr<IDeviceContextVk> pContextVk{pContext, IID_DeviceContextVk};

    auto* pQeueVk = pContextVk->LockCommandQueue();
    auto  vkQueue = pQeueVk->GetVkQueue();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pCommandBuffers    = &vkCmdBuffer;
    SubmitInfo.commandBufferCount = 1;
    vkQueueSubmit(vkQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkQueue);

    pContextVk->UnlockCommandQueue();

    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
    vkDestroyShaderModule(vkDevice, vkMSModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkFSModule, nullptr);
    vkDestroyBuffer(vkDevice, vkIndirectBuffer, nullptr);
    vkFreeMemory(vkDevice, vkBufferMemory, nullptr);
}


void AmplificationShaderDrawReferenceVk(ISwapChain* pSwapChain)
{
    auto* pEnv     = TestingEnvironmentVk::GetInstance();
    auto  vkDevice = pEnv->GetVkDevice();
    auto* pContext = pEnv->GetDeviceContext();

    const auto& SCDesc = pSwapChain->GetDesc();

    VkResult res = VK_SUCCESS;
    (void)res;

    auto* pTestingSwapChainVk = ValidatedCast<TestingSwapChainVk>(pSwapChain);

    auto vkTSModule = pEnv->CreateShaderModule(SHADER_TYPE_AMPLIFICATION, GLSL::AmplificationShaderTest_TS);
    ASSERT_TRUE(vkTSModule != VK_NULL_HANDLE);
    auto vkMSModule = pEnv->CreateShaderModule(SHADER_TYPE_MESH, GLSL::AmplificationShaderTest_MS);
    ASSERT_TRUE(vkMSModule != VK_NULL_HANDLE);
    auto vkFSModule = pEnv->CreateShaderModule(SHADER_TYPE_PIXEL, GLSL::AmplificationShaderTest_FS);
    ASSERT_TRUE(vkFSModule != VK_NULL_HANDLE);

    VkGraphicsPipelineCreateInfo PipelineCI = {};

    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo ShaderStages[3] = {};

    ShaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage  = VK_SHADER_STAGE_TASK_BIT_NV;
    ShaderStages[0].module = vkTSModule;
    ShaderStages[0].pName  = "main";

    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_MESH_BIT_NV;
    ShaderStages[1].module = vkMSModule;
    ShaderStages[1].pName  = "main";

    ShaderStages[2].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[2].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[2].module = vkFSModule;
    ShaderStages[2].pName  = "main";

    PipelineCI.pStages    = ShaderStages;
    PipelineCI.stageCount = _countof(ShaderStages);

    VkPipelineLayoutCreateInfo PipelineLayoutCI = {};

    PipelineLayoutCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout vkLayout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(vkDevice, &PipelineLayoutCI, nullptr, &vkLayout);
    ASSERT_TRUE(vkLayout != VK_NULL_HANDLE);
    PipelineCI.layout = vkLayout;


    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};

    InputAssemblyCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.topology       = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    PipelineCI.pInputAssemblyState = &InputAssemblyCI;


    VkPipelineViewportStateCreateInfo ViewPortStateCI = {};

    ViewPortStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
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
    RasterizerStateCI.cullMode    = VK_CULL_MODE_BACK_BIT;
    RasterizerStateCI.frontFace   = VK_FRONT_FACE_CLOCKWISE;
    RasterizerStateCI.lineWidth   = 1;

    PipelineCI.pRasterizationState = &RasterizerStateCI;

    // Multisample state (24)
    VkPipelineMultisampleStateCreateInfo MSStateCI = {};

    MSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of
    // pMultisampleState must be the same as the sample count for those subpass attachments
    MSStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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

    PipelineCI.renderPass         = pTestingSwapChainVk->GetRenderPass();
    PipelineCI.subpass            = 0;
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex  = 0;              // an index into the pCreateInfos parameter to use as a pipeline to derive from

    VkPipeline vkPipeline = VK_NULL_HANDLE;
    res                   = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipeline);
    ASSERT_GE(res, 0);
    ASSERT_TRUE(vkPipeline != VK_NULL_HANDLE);

    VkCommandBuffer vkCmdBuffer = pEnv->AllocateCommandBuffer();

    pTestingSwapChainVk->BeginRenderPass(vkCmdBuffer, VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV | VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkCmdBindPipeline(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
    vkCmdDrawMeshTasksNV(vkCmdBuffer, 8, 0);
    pTestingSwapChainVk->EndRenderPass(vkCmdBuffer);
    res = vkEndCommandBuffer(vkCmdBuffer);
    VERIFY(res >= 0, "Failed to end command buffer");

    RefCntAutoPtr<IDeviceContextVk> pContextVk{pContext, IID_DeviceContextVk};

    auto* pQeueVk = pContextVk->LockCommandQueue();
    auto  vkQueue = pQeueVk->GetVkQueue();

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pCommandBuffers    = &vkCmdBuffer;
    SubmitInfo.commandBufferCount = 1;
    vkQueueSubmit(vkQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkQueue);

    pContextVk->UnlockCommandQueue();

    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkLayout, nullptr);
    vkDestroyShaderModule(vkDevice, vkTSModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkMSModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkFSModule, nullptr);
}

} // namespace Testing

} // namespace Diligent
