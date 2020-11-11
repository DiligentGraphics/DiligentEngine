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
#include <array>
#include "PipelineStateVkImpl.hpp"
#include "ShaderVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "RenderPassVkImpl.hpp"
#include "ShaderResourceBindingVkImpl.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"


#if !DILIGENT_NO_HLSL
#    include "spirv-tools/optimizer.hpp"
#endif

namespace Diligent
{

RenderPassDesc PipelineStateVkImpl::GetImplicitRenderPassDesc(
    Uint32                                                        NumRenderTargets,
    const TEXTURE_FORMAT                                          RTVFormats[],
    TEXTURE_FORMAT                                                DSVFormat,
    Uint8                                                         SampleCount,
    std::array<RenderPassAttachmentDesc, MAX_RENDER_TARGETS + 1>& Attachments,
    std::array<AttachmentReference, MAX_RENDER_TARGETS + 1>&      AttachmentReferences,
    SubpassDesc&                                                  SubpassDesc)
{
    VERIFY_EXPR(NumRenderTargets <= MAX_RENDER_TARGETS);

    RenderPassDesc RPDesc;

    RPDesc.AttachmentCount = (DSVFormat != TEX_FORMAT_UNKNOWN ? 1 : 0) + NumRenderTargets;

    uint32_t             AttachmentInd             = 0;
    AttachmentReference* pDepthAttachmentReference = nullptr;
    if (DSVFormat != TEX_FORMAT_UNKNOWN)
    {
        auto& DepthAttachment = Attachments[AttachmentInd];

        DepthAttachment.Format      = DSVFormat;
        DepthAttachment.SampleCount = SampleCount;
        DepthAttachment.LoadOp      = ATTACHMENT_LOAD_OP_LOAD; // previous contents of the image within the render area
                                                               // will be preserved. For attachments with a depth/stencil format,
                                                               // this uses the access type VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT.
        DepthAttachment.StoreOp = ATTACHMENT_STORE_OP_STORE;   // the contents generated during the render pass and within the render
                                                               // area are written to memory. For attachments with a depth/stencil format,
                                                               // this uses the access type VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT.
        DepthAttachment.StencilLoadOp  = ATTACHMENT_LOAD_OP_LOAD;
        DepthAttachment.StencilStoreOp = ATTACHMENT_STORE_OP_STORE;
        DepthAttachment.InitialState   = RESOURCE_STATE_DEPTH_WRITE;
        DepthAttachment.FinalState     = RESOURCE_STATE_DEPTH_WRITE;

        pDepthAttachmentReference                  = &AttachmentReferences[AttachmentInd];
        pDepthAttachmentReference->AttachmentIndex = AttachmentInd;
        pDepthAttachmentReference->State           = RESOURCE_STATE_DEPTH_WRITE;

        ++AttachmentInd;
    }

    AttachmentReference* pColorAttachmentsReference = NumRenderTargets > 0 ? &AttachmentReferences[AttachmentInd] : nullptr;
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt, ++AttachmentInd)
    {
        auto& ColorAttachment = Attachments[AttachmentInd];

        ColorAttachment.Format      = RTVFormats[rt];
        ColorAttachment.SampleCount = SampleCount;
        ColorAttachment.LoadOp      = ATTACHMENT_LOAD_OP_LOAD; // previous contents of the image within the render area
                                                               // will be preserved. For attachments with a depth/stencil format,
                                                               // this uses the access type VK_ACCESS_COLOR_ATTACHMENT_READ_BIT.
        ColorAttachment.StoreOp = ATTACHMENT_STORE_OP_STORE;   // the contents generated during the render pass and within the render
                                                               // area are written to memory. For attachments with a color format,
                                                               // this uses the access type VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT.
        ColorAttachment.StencilLoadOp  = ATTACHMENT_LOAD_OP_DISCARD;
        ColorAttachment.StencilStoreOp = ATTACHMENT_STORE_OP_DISCARD;
        ColorAttachment.InitialState   = RESOURCE_STATE_RENDER_TARGET;
        ColorAttachment.FinalState     = RESOURCE_STATE_RENDER_TARGET;

        auto& ColorAttachmentRef           = AttachmentReferences[AttachmentInd];
        ColorAttachmentRef.AttachmentIndex = AttachmentInd;
        ColorAttachmentRef.State           = RESOURCE_STATE_RENDER_TARGET;
    }

    RPDesc.pAttachments    = Attachments.data();
    RPDesc.SubpassCount    = 1;
    RPDesc.pSubpasses      = &SubpassDesc;
    RPDesc.DependencyCount = 0;       // the number of dependencies between pairs of subpasses, or zero indicating no dependencies.
    RPDesc.pDependencies   = nullptr; // an array of dependencyCount number of VkSubpassDependency structures describing
                                      // dependencies between pairs of subpasses, or NULL if dependencyCount is zero.


    SubpassDesc.InputAttachmentCount        = 0;
    SubpassDesc.pInputAttachments           = nullptr;
    SubpassDesc.RenderTargetAttachmentCount = NumRenderTargets;
    SubpassDesc.pRenderTargetAttachments    = pColorAttachmentsReference;
    SubpassDesc.pResolveAttachments         = nullptr;
    SubpassDesc.pDepthStencilAttachment     = pDepthAttachmentReference;
    SubpassDesc.PreserveAttachmentCount     = 0;
    SubpassDesc.pPreserveAttachments        = nullptr;

    return RPDesc;
}

static bool StripReflection(std::vector<uint32_t>& SPIRV)
{
#if DILIGENT_NO_HLSL
    return false;
#else
    std::vector<uint32_t> StrippedSPIRV;
    spvtools::Optimizer   SpirvOptimizer(SPV_ENV_VULKAN_1_0);
    // Decorations defined in SPV_GOOGLE_hlsl_functionality1 are the only instructions
    // removed by strip-reflect-info pass. SPIRV offsets become INVALID after this operation.
    SpirvOptimizer.RegisterPass(spvtools::CreateStripReflectInfoPass());
    if (SpirvOptimizer.Run(SPIRV.data(), SPIRV.size(), &StrippedSPIRV))
    {
        SPIRV = std::move(StrippedSPIRV);
        return true;
    }
    else
        return false;
#endif
}

static void InitPipelineShaderStages(const VulkanUtilities::VulkanLogicalDevice&        LogicalDevice,
                                     ShaderResourceLayoutVk::TShaderStages&             ShaderStages,
                                     std::vector<VulkanUtilities::ShaderModuleWrapper>& vkShaderModules,
                                     std::vector<VkPipelineShaderStageCreateInfo>&      vkPipelineShaderStages)
{
    for (size_t s = 0; s < ShaderStages.size(); ++s)
    {
        auto& StageInfo = ShaderStages[s];

        VkPipelineShaderStageCreateInfo StageCI = {};

        StageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        StageCI.pNext = nullptr;
        StageCI.flags = 0; //  reserved for future use
        StageCI.stage = ShaderTypeToVkShaderStageFlagBit(StageInfo.Type);

        VkShaderModuleCreateInfo ShaderModuleCI = {};

        ShaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ShaderModuleCI.pNext = nullptr;
        ShaderModuleCI.flags = 0;

        // We have to strip reflection instructions to fix the follownig validation error:
        //     SPIR-V module not valid: DecorateStringGOOGLE requires one of the following extensions: SPV_GOOGLE_decorate_string
        // Optimizer also performs validation and may catch problems with the byte code.
        if (!StripReflection(StageInfo.SPIRV))
            LOG_ERROR("Failed to strip reflection information from shader '", StageInfo.pShader->GetDesc().Name, "'. This may indicate a problem with the byte code.");

        ShaderModuleCI.codeSize = StageInfo.SPIRV.size() * sizeof(uint32_t);
        ShaderModuleCI.pCode    = StageInfo.SPIRV.data();

        vkShaderModules.push_back(LogicalDevice.CreateShaderModule(ShaderModuleCI, StageInfo.pShader->GetDesc().Name));

        StageCI.module              = vkShaderModules.back();
        StageCI.pName               = StageInfo.pShader->GetEntryPoint();
        StageCI.pSpecializationInfo = nullptr;

        vkPipelineShaderStages.push_back(StageCI);
    }

    VERIFY_EXPR(vkShaderModules.size() == vkPipelineShaderStages.size());
}


static void CreateComputePipeline(RenderDeviceVkImpl*                           pDeviceVk,
                                  std::vector<VkPipelineShaderStageCreateInfo>& Stages,
                                  const PipelineLayout&                         Layout,
                                  const PipelineStateDesc&                      PSODesc,
                                  VulkanUtilities::PipelineWrapper&             Pipeline)
{
    const auto& LogicalDevice = pDeviceVk->GetLogicalDevice();

    VkComputePipelineCreateInfo PipelineCI = {};

    PipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCI.pNext = nullptr;
#ifdef DILIGENT_DEBUG
    PipelineCI.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
#endif
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex  = -1;             // an index into the pCreateInfos parameter to use as a pipeline to derive from

    PipelineCI.stage  = Stages[0];
    PipelineCI.layout = Layout.GetVkPipelineLayout();

    Pipeline = LogicalDevice.CreateComputePipeline(PipelineCI, VK_NULL_HANDLE, PSODesc.Name);
}


static void CreateGraphicsPipeline(RenderDeviceVkImpl*                           pDeviceVk,
                                   std::vector<VkPipelineShaderStageCreateInfo>& Stages,
                                   const PipelineLayout&                         Layout,
                                   const PipelineStateDesc&                      PSODesc,
                                   const GraphicsPipelineDesc&                   GraphicsPipeline,
                                   VulkanUtilities::PipelineWrapper&             Pipeline,
                                   RefCntAutoPtr<IRenderPass>&                   pRenderPass)
{
    const auto& LogicalDevice  = pDeviceVk->GetLogicalDevice();
    const auto& PhysicalDevice = pDeviceVk->GetPhysicalDevice();
    auto&       RPCache        = pDeviceVk->GetImplicitRenderPassCache();

    if (pRenderPass == nullptr)
    {
        RenderPassCache::RenderPassCacheKey Key{
            GraphicsPipeline.NumRenderTargets,
            GraphicsPipeline.SmplDesc.Count,
            GraphicsPipeline.RTVFormats,
            GraphicsPipeline.DSVFormat};
        pRenderPass = RPCache.GetRenderPass(Key);
    }

    VkGraphicsPipelineCreateInfo PipelineCI = {};

    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCI.pNext = nullptr;
#ifdef DILIGENT_DEBUG
    PipelineCI.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
#endif

    PipelineCI.stageCount = static_cast<Uint32>(Stages.size());
    PipelineCI.pStages    = Stages.data();
    PipelineCI.layout     = Layout.GetVkPipelineLayout();

    VkPipelineVertexInputStateCreateInfo VertexInputStateCI = {};

    std::array<VkVertexInputBindingDescription, MAX_LAYOUT_ELEMENTS>   BindingDescriptions;
    std::array<VkVertexInputAttributeDescription, MAX_LAYOUT_ELEMENTS> AttributeDescription;
    InputLayoutDesc_To_VkVertexInputStateCI(GraphicsPipeline.InputLayout, VertexInputStateCI, BindingDescriptions, AttributeDescription);
    PipelineCI.pVertexInputState = &VertexInputStateCI;


    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};

    InputAssemblyCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.pNext                  = nullptr;
    InputAssemblyCI.flags                  = 0; // reserved for future use
    InputAssemblyCI.primitiveRestartEnable = VK_FALSE;
    PipelineCI.pInputAssemblyState         = &InputAssemblyCI;


    VkPipelineTessellationStateCreateInfo TessStateCI = {};

    TessStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    TessStateCI.pNext             = nullptr;
    TessStateCI.flags             = 0; // reserved for future use
    PipelineCI.pTessellationState = &TessStateCI;

    if (PSODesc.PipelineType == PIPELINE_TYPE_MESH)
    {
        // Input assembly is not used in the mesh pipeline, so topology may contain any value.
        // Validation layers may generate a warning if point_list topology is used, so use MAX_ENUM value.
        InputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;

        // Vertex input state and tessellation state are ignored in a mesh pipeline and should be null.
        PipelineCI.pVertexInputState  = nullptr;
        PipelineCI.pTessellationState = nullptr;
    }
    else
    {
        PrimitiveTopology_To_VkPrimitiveTopologyAndPatchCPCount(GraphicsPipeline.PrimitiveTopology, InputAssemblyCI.topology, TessStateCI.patchControlPoints);
    }

    VkPipelineViewportStateCreateInfo ViewPortStateCI = {};

    ViewPortStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewPortStateCI.pNext = nullptr;
    ViewPortStateCI.flags = 0; // reserved for future use
    ViewPortStateCI.viewportCount =
        GraphicsPipeline.NumViewports;                            // Even though we use dynamic viewports, the number of viewports used
                                                                  // by the pipeline is still specified by the viewportCount member (23.5)
    ViewPortStateCI.pViewports   = nullptr;                       // We will be using dynamic viewport & scissor states
    ViewPortStateCI.scissorCount = ViewPortStateCI.viewportCount; // the number of scissors must match the number of viewports (23.5)
                                                                  // (why the hell it is in the struct then?)
    VkRect2D ScissorRect = {};
    if (GraphicsPipeline.RasterizerDesc.ScissorEnable)
    {
        ViewPortStateCI.pScissors = nullptr; // Ignored if the scissor state is dynamic
    }
    else
    {
        const auto& Props = PhysicalDevice.GetProperties();
        // There are limitiations on the viewport width and height (23.5), but
        // it is not clear if there are limitations on the scissor rect width and
        // height
        ScissorRect.extent.width  = Props.limits.maxViewportDimensions[0];
        ScissorRect.extent.height = Props.limits.maxViewportDimensions[1];
        ViewPortStateCI.pScissors = &ScissorRect;
    }
    PipelineCI.pViewportState = &ViewPortStateCI;

    VkPipelineRasterizationStateCreateInfo RasterizerStateCI =
        RasterizerStateDesc_To_VkRasterizationStateCI(GraphicsPipeline.RasterizerDesc);
    PipelineCI.pRasterizationState = &RasterizerStateCI;

    // Multisample state (24)
    VkPipelineMultisampleStateCreateInfo MSStateCI = {};

    MSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MSStateCI.pNext = nullptr;
    MSStateCI.flags = 0; // reserved for future use
    // If subpass uses color and/or depth/stencil attachments, then the rasterizationSamples member of
    // pMultisampleState must be the same as the sample count for those subpass attachments
    MSStateCI.rasterizationSamples = static_cast<VkSampleCountFlagBits>(GraphicsPipeline.SmplDesc.Count);
    MSStateCI.sampleShadingEnable  = VK_FALSE;
    MSStateCI.minSampleShading     = 0;                                // a minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
    uint32_t SampleMask[]          = {GraphicsPipeline.SampleMask, 0}; // Vulkan spec allows up to 64 samples
    MSStateCI.pSampleMask          = SampleMask;                       // an array of static coverage information that is ANDed with
                                                                       // the coverage information generated during rasterization (25.3)
    MSStateCI.alphaToCoverageEnable = VK_FALSE;                        // whether a temporary coverage value is generated based on
                                                                       // the alpha component of the fragment's first color output
    MSStateCI.alphaToOneEnable   = VK_FALSE;                           // whether the alpha component of the fragment's first color output is replaced with one
    PipelineCI.pMultisampleState = &MSStateCI;

    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCI =
        DepthStencilStateDesc_To_VkDepthStencilStateCI(GraphicsPipeline.DepthStencilDesc);
    PipelineCI.pDepthStencilState = &DepthStencilStateCI;

    const auto& RPDesc           = pRenderPass->GetDesc();
    const auto  NumRTAttachments = RPDesc.pSubpasses[GraphicsPipeline.SubpassIndex].RenderTargetAttachmentCount;
    VERIFY_EXPR(GraphicsPipeline.pRenderPass != nullptr || GraphicsPipeline.NumRenderTargets == NumRTAttachments);
    std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachmentStates(NumRTAttachments);

    VkPipelineColorBlendStateCreateInfo BlendStateCI = {};

    BlendStateCI.pAttachments    = !ColorBlendAttachmentStates.empty() ? ColorBlendAttachmentStates.data() : nullptr;
    BlendStateCI.attachmentCount = NumRTAttachments; // must equal the colorAttachmentCount for the subpass
                                                     // in which this pipeline is used.
    BlendStateDesc_To_VkBlendStateCI(GraphicsPipeline.BlendDesc, BlendStateCI, ColorBlendAttachmentStates);
    PipelineCI.pColorBlendState = &BlendStateCI;


    VkPipelineDynamicStateCreateInfo DynamicStateCI = {};

    DynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCI.pNext = nullptr;
    DynamicStateCI.flags = 0; // reserved for future use
    std::vector<VkDynamicState> DynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT, // pViewports state in VkPipelineViewportStateCreateInfo will be ignored and must be
                                       // set dynamically with vkCmdSetViewport before any draw commands. The number of viewports
                                       // used by a pipeline is still specified by the viewportCount member of
                                       // VkPipelineViewportStateCreateInfo.

            VK_DYNAMIC_STATE_BLEND_CONSTANTS, // blendConstants state in VkPipelineColorBlendStateCreateInfo will be ignored
                                              // and must be set dynamically with vkCmdSetBlendConstants

            VK_DYNAMIC_STATE_STENCIL_REFERENCE // pecifies that the reference state in VkPipelineDepthStencilStateCreateInfo
                                               // for both front and back will be ignored and must be set dynamically
                                               // with vkCmdSetStencilReference
        };

    if (GraphicsPipeline.RasterizerDesc.ScissorEnable)
    {
        // pScissors state in VkPipelineViewportStateCreateInfo will be ignored and must be set
        // dynamically with vkCmdSetScissor before any draw commands. The number of scissor rectangles
        // used by a pipeline is still specified by the scissorCount member of
        // VkPipelineViewportStateCreateInfo.
        DynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    }
    DynamicStateCI.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
    DynamicStateCI.pDynamicStates    = DynamicStates.data();
    PipelineCI.pDynamicState         = &DynamicStateCI;


    PipelineCI.renderPass         = pRenderPass.RawPtr<IRenderPassVk>()->GetVkRenderPass();
    PipelineCI.subpass            = GraphicsPipeline.SubpassIndex;
    PipelineCI.basePipelineHandle = VK_NULL_HANDLE; // a pipeline to derive from
    PipelineCI.basePipelineIndex  = -1;             // an index into the pCreateInfos parameter to use as a pipeline to derive from

    Pipeline = LogicalDevice.CreateGraphicsPipeline(PipelineCI, VK_NULL_HANDLE, PSODesc.Name);
}

void PipelineStateVkImpl::InitResourceLayouts(const PipelineStateCreateInfo& CreateInfo,
                                              TShaderStages&                 ShaderStages)
{
    auto* const pDeviceVk     = GetDevice();
    const auto& LogicalDevice = pDeviceVk->GetLogicalDevice();

    for (size_t s = 0; s < ShaderStages.size(); ++s)
    {
        auto&      StageInfo     = ShaderStages[s];
        const auto ShaderType    = StageInfo.Type;
        const auto ShaderTypeInd = GetShaderTypePipelineIndex(ShaderType, m_Desc.PipelineType);

        m_ResourceLayoutIndex[ShaderTypeInd] = static_cast<Int8>(s);

        auto& StaticResLayout = m_ShaderResourceLayouts[GetNumShaderStages() + s];
        StaticResLayout.InitializeStaticResourceLayout(StageInfo.pShader, GetRawAllocator(), m_Desc.ResourceLayout, m_StaticResCaches[s]);

        m_StaticVarsMgrs[s].Initialize(StaticResLayout, GetRawAllocator(), nullptr, 0);
    }
    ShaderResourceLayoutVk::Initialize(pDeviceVk, ShaderStages, m_ShaderResourceLayouts, GetRawAllocator(),
                                       m_Desc.ResourceLayout, m_PipelineLayout,
                                       (CreateInfo.Flags & PSO_CREATE_FLAG_IGNORE_MISSING_VARIABLES) == 0,
                                       (CreateInfo.Flags & PSO_CREATE_FLAG_IGNORE_MISSING_IMMUTABLE_SAMPLERS) == 0);
    m_PipelineLayout.Finalize(LogicalDevice);

    if (m_Desc.SRBAllocationGranularity > 1)
    {
        std::array<size_t, MAX_SHADERS_IN_PIPELINE> ShaderVariableDataSizes = {};
        for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
        {
            const SHADER_RESOURCE_VARIABLE_TYPE AllowedVarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};

            Uint32 UnusedNumVars       = 0;
            ShaderVariableDataSizes[s] = ShaderVariableManagerVk::GetRequiredMemorySize(m_ShaderResourceLayouts[s], AllowedVarTypes, _countof(AllowedVarTypes), UnusedNumVars);
        }

        Uint32 NumSets            = 0;
        auto   DescriptorSetSizes = m_PipelineLayout.GetDescriptorSetSizes(NumSets);
        auto   CacheMemorySize    = ShaderResourceCacheVk::GetRequiredMemorySize(NumSets, DescriptorSetSizes.data());

        m_SRBMemAllocator.Initialize(m_Desc.SRBAllocationGranularity, GetNumShaderStages(), ShaderVariableDataSizes.data(), 1, &CacheMemorySize);
    }

    m_HasStaticResources    = false;
    m_HasNonStaticResources = false;
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        const auto& Layout = m_ShaderResourceLayouts[s];
        if (Layout.GetResourceCount(SHADER_RESOURCE_VARIABLE_TYPE_STATIC) != 0)
            m_HasStaticResources = true;

        if (Layout.GetResourceCount(SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE) != 0 ||
            Layout.GetResourceCount(SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC) != 0)
            m_HasNonStaticResources = true;
    }

    m_ShaderResourceLayoutHash = m_PipelineLayout.GetHash();
}

template <typename PSOCreateInfoType>
void PipelineStateVkImpl::InitInternalObjects(const PSOCreateInfoType&                           CreateInfo,
                                              std::vector<VkPipelineShaderStageCreateInfo>&      vkShaderStages,
                                              std::vector<VulkanUtilities::ShaderModuleWrapper>& ShaderModules)
{
    m_ResourceLayoutIndex.fill(-1);

    TShaderStages ShaderStages;
    ExtractShaders<ShaderVkImpl>(CreateInfo, ShaderStages);

    LinearAllocator MemPool{GetRawAllocator()};

    const auto NumShaderStages = GetNumShaderStages();
    VERIFY_EXPR(NumShaderStages > 0 && NumShaderStages == ShaderStages.size());

    MemPool.AddSpace<ShaderResourceCacheVk>(NumShaderStages);
    MemPool.AddSpace<ShaderResourceLayoutVk>(NumShaderStages * 2);
    MemPool.AddSpace<ShaderVariableManagerVk>(NumShaderStages);

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);

    MemPool.Reserve();

    const auto& LogicalDevice = GetDevice()->GetLogicalDevice();

    m_StaticResCaches = MemPool.ConstructArray<ShaderResourceCacheVk>(NumShaderStages, ShaderResourceCacheVk::DbgCacheContentType::StaticShaderResources);

    // The memory is now owned by PipelineStateVkImpl and will be freed by Destruct().
    auto* Ptr = MemPool.ReleaseOwnership();
    VERIFY_EXPR(Ptr == m_StaticResCaches);
    (void)Ptr;

    m_ShaderResourceLayouts = MemPool.ConstructArray<ShaderResourceLayoutVk>(NumShaderStages * 2, LogicalDevice);

    m_StaticVarsMgrs = MemPool.Allocate<ShaderVariableManagerVk>(NumShaderStages);
    for (Uint32 s = 0; s < NumShaderStages; ++s)
        new (m_StaticVarsMgrs + s) ShaderVariableManagerVk{*this, m_StaticResCaches[s]};

    InitializePipelineDesc(CreateInfo, MemPool);

    // It is important to construct all objects before initializing them because if an exception is thrown,
    // destructors will be called for all objects

    InitResourceLayouts(CreateInfo, ShaderStages);

    // Create shader modules and initialize shader stages
    InitPipelineShaderStages(GetDevice()->GetLogicalDevice(), ShaderStages, ShaderModules, vkShaderStages);
}

PipelineStateVkImpl::PipelineStateVkImpl(IReferenceCounters*                    pRefCounters,
                                         RenderDeviceVkImpl*                    pDeviceVk,
                                         const GraphicsPipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceVk, CreateInfo.PSODesc},
    m_SRBMemAllocator{GetRawAllocator()}
{
    try
    {
        std::vector<VkPipelineShaderStageCreateInfo>      vkShaderStages;
        std::vector<VulkanUtilities::ShaderModuleWrapper> ShaderModules;

        InitInternalObjects(CreateInfo, vkShaderStages, ShaderModules);

        CreateGraphicsPipeline(pDeviceVk, vkShaderStages, m_PipelineLayout, m_Desc, GetGraphicsPipelineDesc(), m_Pipeline, m_pRenderPass);
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}


PipelineStateVkImpl::PipelineStateVkImpl(IReferenceCounters*                   pRefCounters,
                                         RenderDeviceVkImpl*                   pDeviceVk,
                                         const ComputePipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceVk, CreateInfo.PSODesc},
    m_SRBMemAllocator{GetRawAllocator()}
{
    try
    {
        std::vector<VkPipelineShaderStageCreateInfo>      vkShaderStages;
        std::vector<VulkanUtilities::ShaderModuleWrapper> ShaderModules;

        InitInternalObjects(CreateInfo, vkShaderStages, ShaderModules);

        CreateComputePipeline(pDeviceVk, vkShaderStages, m_PipelineLayout, m_Desc, m_Pipeline);
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}


PipelineStateVkImpl::~PipelineStateVkImpl()
{
    Destruct();
}

void PipelineStateVkImpl::Destruct()
{
    m_pDevice->SafeReleaseDeviceObject(std::move(m_Pipeline), m_Desc.CommandQueueMask);
    m_PipelineLayout.Release(m_pDevice, m_Desc.CommandQueueMask);

    auto& RawAllocator = GetRawAllocator();
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        if (m_StaticVarsMgrs != nullptr)
        {
            m_StaticVarsMgrs[s].DestroyVariables(GetRawAllocator());
            m_StaticVarsMgrs[s].~ShaderVariableManagerVk();
        }

        if (m_ShaderResourceLayouts != nullptr)
        {
            m_ShaderResourceLayouts[s].~ShaderResourceLayoutVk();
            m_ShaderResourceLayouts[GetNumShaderStages() + s].~ShaderResourceLayoutVk();
        }

        if (m_StaticResCaches != nullptr)
        {
            m_StaticResCaches[s].~ShaderResourceCacheVk();
        }
    }

    // All internal objects are allocted in contiguous chunks of memory.
    if (void* pRawMem = m_StaticResCaches)
    {
        RawAllocator.Free(pRawMem);
    }
}

IMPLEMENT_QUERY_INTERFACE(PipelineStateVkImpl, IID_PipelineStateVk, TPipelineStateBase)


void PipelineStateVkImpl::CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources)
{
    auto& SRBAllocator  = m_pDevice->GetSRBAllocator();
    auto  pResBindingVk = NEW_RC_OBJ(SRBAllocator, "ShaderResourceBindingVkImpl instance", ShaderResourceBindingVkImpl)(this, false);
    if (InitStaticResources)
        pResBindingVk->InitializeStaticResources(nullptr);
    pResBindingVk->QueryInterface(IID_ShaderResourceBinding, reinterpret_cast<IObject**>(ppShaderResourceBinding));
}

bool PipelineStateVkImpl::IsCompatibleWith(const IPipelineState* pPSO) const
{
    VERIFY_EXPR(pPSO != nullptr);

    if (pPSO == this)
        return true;

    const PipelineStateVkImpl* pPSOVk = ValidatedCast<const PipelineStateVkImpl>(pPSO);
    if (m_ShaderResourceLayoutHash != pPSOVk->m_ShaderResourceLayoutHash)
        return false;

    auto IsSamePipelineLayout = m_PipelineLayout.IsSameAs(pPSOVk->m_PipelineLayout);

#ifdef DILIGENT_DEBUG
    {
        bool IsCompatibleShaders = true;
        if (GetNumShaderStages() != pPSOVk->GetNumShaderStages())
            IsCompatibleShaders = false;

        if (IsCompatibleShaders)
        {
            for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
            {
                if (GetShaderStageType(s) != pPSOVk->GetShaderStageType(s))
                {
                    IsCompatibleShaders = false;
                    break;
                }

                const auto& Res0 = GetShaderResLayout(s).GetResources();
                const auto& Res1 = pPSOVk->GetShaderResLayout(s).GetResources();
                if (!Res0.IsCompatibleWith(Res1))
                {
                    IsCompatibleShaders = false;
                    break;
                }
            }
        }

        if (IsCompatibleShaders)
            VERIFY(IsSamePipelineLayout, "Compatible shaders must have same pipeline layouts");
    }
#endif

    return IsSamePipelineLayout;
}


void PipelineStateVkImpl::CommitAndTransitionShaderResources(IShaderResourceBinding*                pShaderResourceBinding,
                                                             DeviceContextVkImpl*                   pCtxVkImpl,
                                                             bool                                   CommitResources,
                                                             RESOURCE_STATE_TRANSITION_MODE         StateTransitionMode,
                                                             PipelineLayout::DescriptorSetBindInfo* pDescrSetBindInfo) const
{
    VERIFY(CommitResources || StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION, "Resources should be transitioned or committed or both");

    if (!m_HasStaticResources && !m_HasNonStaticResources)
        return;

#ifdef DILIGENT_DEVELOPMENT
    if (pShaderResourceBinding == nullptr)
    {
        LOG_ERROR_MESSAGE("Pipeline state '", m_Desc.Name, "' requires shader resource binding object to ",
                          (CommitResources ? "commit" : "transition"), " resources, but none is provided.");
        return;
    }
#endif

    auto* pResBindingVkImpl = ValidatedCast<ShaderResourceBindingVkImpl>(pShaderResourceBinding);

#ifdef DILIGENT_DEVELOPMENT
    {
        auto* pRefPSO = pResBindingVkImpl->GetPipelineState();
        if (IsIncompatibleWith(pRefPSO))
        {
            LOG_ERROR_MESSAGE("Shader resource binding is incompatible with the pipeline state '", m_Desc.Name, "'. Operation will be ignored.");
            return;
        }
    }

    if (m_HasStaticResources && !pResBindingVkImpl->StaticResourcesInitialized())
    {
        LOG_ERROR_MESSAGE("Static resources have not been initialized in the shader resource binding object being committed for PSO '", m_Desc.Name, "'. Please call IShaderResourceBinding::InitializeStaticResources().");
    }
#endif

    auto& ResourceCache = pResBindingVkImpl->GetResourceCache();

#ifdef DILIGENT_DEVELOPMENT
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        m_ShaderResourceLayouts[s].dvpVerifyBindings(ResourceCache);
    }
#endif
#ifdef DILIGENT_DEBUG
    ResourceCache.DbgVerifyDynamicBuffersCounter();
#endif

    if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_TRANSITION)
    {
        ResourceCache.TransitionResources<false>(pCtxVkImpl);
    }
#ifdef DILIGENT_DEVELOPMENT
    else if (StateTransitionMode == RESOURCE_STATE_TRANSITION_MODE_VERIFY)
    {
        ResourceCache.TransitionResources<true>(pCtxVkImpl);
    }
#endif

    if (CommitResources)
    {
        VkDescriptorSet DynamicDescrSet              = VK_NULL_HANDLE;
        auto            DynamicDescriptorSetVkLayout = m_PipelineLayout.GetDynamicDescriptorSetVkLayout();
        if (DynamicDescriptorSetVkLayout != VK_NULL_HANDLE)
        {
            const char* DynamicDescrSetName = "Dynamic Descriptor Set";
#ifdef DILIGENT_DEVELOPMENT
            std::string _DynamicDescrSetName(m_Desc.Name);
            _DynamicDescrSetName.append(" - dynamic set");
            DynamicDescrSetName = _DynamicDescrSetName.c_str();
#endif
            // Allocate vulkan descriptor set for dynamic resources
            DynamicDescrSet = pCtxVkImpl->AllocateDynamicDescriptorSet(DynamicDescriptorSetVkLayout, DynamicDescrSetName);
            // Commit all dynamic resource descriptors
            for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
            {
                const auto& Layout = m_ShaderResourceLayouts[s];
                if (Layout.GetResourceCount(SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC) != 0)
                    Layout.CommitDynamicResources(ResourceCache, DynamicDescrSet);
            }
        }
        // Prepare descriptor sets, and also bind them if there are no dynamic descriptors
        VERIFY_EXPR(pDescrSetBindInfo != nullptr);
        m_PipelineLayout.PrepareDescriptorSets(pCtxVkImpl, m_Desc.IsComputePipeline(), ResourceCache, *pDescrSetBindInfo, DynamicDescrSet);
        // Dynamic descriptor sets are not released individually. Instead, all dynamic descriptor pools
        // are released at the end of the frame by DeviceContextVkImpl::FinishFrame().
    }
}

void PipelineStateVkImpl::BindStaticResources(Uint32 ShaderFlags, IResourceMapping* pResourceMapping, Uint32 Flags)
{
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        auto ShaderType = GetStaticShaderResLayout(s).GetShaderType();
        if ((ShaderType & ShaderFlags) != 0)
        {
            auto& StaticVarMgr = GetStaticVarMgr(s);
            StaticVarMgr.BindResources(pResourceMapping, Flags);
        }
    }
}

Uint32 PipelineStateVkImpl::GetStaticVariableCount(SHADER_TYPE ShaderType) const
{
    const auto LayoutInd = GetStaticVariableCountHelper(ShaderType, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return 0;

    auto& StaticVarMgr = GetStaticVarMgr(LayoutInd);
    return StaticVarMgr.GetVariableCount();
}

IShaderResourceVariable* PipelineStateVkImpl::GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name)
{
    const auto LayoutInd = GetStaticVariableByNameHelper(ShaderType, Name, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return nullptr;

    auto& StaticVarMgr = GetStaticVarMgr(LayoutInd);
    return StaticVarMgr.GetVariable(Name);
}

IShaderResourceVariable* PipelineStateVkImpl::GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    const auto LayoutInd = GetStaticVariableByIndexHelper(ShaderType, Index, m_ResourceLayoutIndex);
    if (LayoutInd < 0)
        return nullptr;

    const auto& StaticVarMgr = GetStaticVarMgr(LayoutInd);
    return StaticVarMgr.GetVariable(Index);
}


void PipelineStateVkImpl::InitializeStaticSRBResources(ShaderResourceCacheVk& ResourceCache) const
{
    for (Uint32 s = 0; s < GetNumShaderStages(); ++s)
    {
        const auto& StaticResLayout = GetStaticShaderResLayout(s);
        const auto& StaticResCache  = GetStaticResCache(s);

#ifdef DILIGENT_DEVELOPMENT
        if (!StaticResLayout.dvpVerifyBindings(StaticResCache))
        {
            LOG_ERROR_MESSAGE("Static resources in SRB of PSO '", GetDesc().Name,
                              "' will not be successfully initialized because not all static resource bindings in shader '",
                              GetShaderTypeLiteralName(GetShaderStageType(s)),
                              "' are valid. Please make sure you bind all static resources to PSO before calling InitializeStaticResources() "
                              "directly or indirectly by passing InitStaticResources=true to CreateShaderResourceBinding() method.");
        }
#endif
        const auto& ShaderResourceLayouts = GetShaderResLayout(s);
        ShaderResourceLayouts.InitializeStaticResources(StaticResLayout, StaticResCache, ResourceCache);
    }
#ifdef DILIGENT_DEBUG
    ResourceCache.DbgVerifyDynamicBuffersCounter();
#endif
}

} // namespace Diligent
