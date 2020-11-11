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

/// \file
/// Type conversion routines

#include <array>
#include "GraphicsTypes.h"
#include "InputLayout.h"

namespace Diligent
{

VkFormat       TexFormatToVkFormat(TEXTURE_FORMAT TexFmt);
TEXTURE_FORMAT VkFormatToTexFormat(VkFormat VkFmt);

VkFormat TypeToVkFormat(VALUE_TYPE ValType, Uint32 NumComponents, Bool bIsNormalized);

VkPipelineRasterizationStateCreateInfo RasterizerStateDesc_To_VkRasterizationStateCI(const struct RasterizerStateDesc& RasterizerDesc);
VkPipelineDepthStencilStateCreateInfo  DepthStencilStateDesc_To_VkDepthStencilStateCI(const struct DepthStencilStateDesc& DepthStencilDesc);

void BlendStateDesc_To_VkBlendStateCI(const struct BlendStateDesc&                      BSDesc,
                                      VkPipelineColorBlendStateCreateInfo&              ColorBlendStateCI,
                                      std::vector<VkPipelineColorBlendAttachmentState>& ColorBlendAttachments);

void InputLayoutDesc_To_VkVertexInputStateCI(const struct InputLayoutDesc&                                       LayoutDesc,
                                             VkPipelineVertexInputStateCreateInfo&                               VertexInputStateCI,
                                             std::array<VkVertexInputBindingDescription, MAX_LAYOUT_ELEMENTS>&   BindingDescriptions,
                                             std::array<VkVertexInputAttributeDescription, MAX_LAYOUT_ELEMENTS>& AttributeDescription);

void PrimitiveTopology_To_VkPrimitiveTopologyAndPatchCPCount(PRIMITIVE_TOPOLOGY   PrimTopology,
                                                             VkPrimitiveTopology& VkPrimTopology,
                                                             uint32_t&            PatchControlPoints);

VkCompareOp          ComparisonFuncToVkCompareOp(COMPARISON_FUNCTION CmpFunc);
VkFilter             FilterTypeToVkFilter(FILTER_TYPE FilterType);
VkSamplerMipmapMode  FilterTypeToVkMipmapMode(FILTER_TYPE FilterType);
VkSamplerAddressMode AddressModeToVkAddressMode(TEXTURE_ADDRESS_MODE AddressMode);
VkBorderColor        BorderColorToVkBorderColor(const Float32 BorderColor[]);

VkAccessFlags ResourceStateFlagsToVkAccessFlags(RESOURCE_STATE StateFlags);
VkImageLayout ResourceStateToVkImageLayout(RESOURCE_STATE StateFlag, bool IsInsideRenderPass = false);

RESOURCE_STATE VkAccessFlagsToResourceStates(VkAccessFlags AccessFlags);
RESOURCE_STATE VkImageLayoutToResourceState(VkImageLayout Layout);

SURFACE_TRANSFORM             VkSurfaceTransformFlagToSurfaceTransform(VkSurfaceTransformFlagBitsKHR vkTransformFlag);
VkSurfaceTransformFlagBitsKHR SurfaceTransformToVkSurfaceTransformFlag(SURFACE_TRANSFORM SrfTransform);

VkAttachmentLoadOp AttachmentLoadOpToVkAttachmentLoadOp(ATTACHMENT_LOAD_OP LoadOp);
ATTACHMENT_LOAD_OP VkAttachmentLoadOpToAttachmentLoadOp(VkAttachmentLoadOp VkLoadOp);

VkAttachmentStoreOp AttachmentStoreOpToVkAttachmentStoreOp(ATTACHMENT_STORE_OP StoreOp);
ATTACHMENT_STORE_OP VkAttachmentStoreOpToAttachmentStoreOp(VkAttachmentStoreOp VkStoreOp);

VkPipelineStageFlags PipelineStageFlagsToVkPipelineStageFlags(PIPELINE_STAGE_FLAGS PipelineStageFlags);
VkAccessFlags        AccessFlagsToVkAccessFlags(ACCESS_FLAGS AccessFlags);


VkShaderStageFlagBits ShaderTypeToVkShaderStageFlagBit(SHADER_TYPE ShaderType);

} // namespace Diligent
