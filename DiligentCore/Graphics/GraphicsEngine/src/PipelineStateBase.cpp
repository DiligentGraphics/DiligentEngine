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
#include "PipelineStateBase.hpp"

namespace Diligent
{

#define LOG_PSO_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of ", GetPipelineTypeString(PSODesc.PipelineType), " PSO '", PSODesc.Name, "' is invalid: ", ##__VA_ARGS__)

namespace
{

void ValidateRasterizerStateDesc(const PipelineStateDesc& PSODesc, const GraphicsPipelineDesc& GraphicsPipeline) noexcept(false)
{
    const auto& RSDesc = GraphicsPipeline.RasterizerDesc;
    if (RSDesc.FillMode == FILL_MODE_UNDEFINED)
        LOG_PSO_ERROR_AND_THROW("RasterizerDesc.FillMode must not be FILL_MODE_UNDEFINED");
    if (RSDesc.CullMode == CULL_MODE_UNDEFINED)
        LOG_PSO_ERROR_AND_THROW("RasterizerDesc.CullMode must not be CULL_MODE_UNDEFINED");
}

void ValidateDepthStencilDesc(const PipelineStateDesc& PSODesc, const GraphicsPipelineDesc& GraphicsPipeline) noexcept(false)
{
    const auto& DSSDesc = GraphicsPipeline.DepthStencilDesc;
    if (DSSDesc.DepthEnable && DSSDesc.DepthFunc == COMPARISON_FUNC_UNKNOWN)
        LOG_PSO_ERROR_AND_THROW("DepthStencilDesc.DepthFunc must not be COMPARISON_FUNC_UNKNOWN when depth is enabled");

    auto CheckStencilOpDesc = [&](const StencilOpDesc& OpDesc, const char* FaceName) //
    {
        if (DSSDesc.StencilEnable)
        {
            if (OpDesc.StencilFailOp == STENCIL_OP_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("DepthStencilDesc.", FaceName, ".StencilFailOp must not be STENCIL_OP_UNDEFINED when stencil is enabled");
            if (OpDesc.StencilDepthFailOp == STENCIL_OP_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("DepthStencilDesc.", FaceName, ".StencilDepthFailOp must not be STENCIL_OP_UNDEFINED when stencil is enabled");
            if (OpDesc.StencilPassOp == STENCIL_OP_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("DepthStencilDesc.", FaceName, ".StencilPassOp must not be STENCIL_OP_UNDEFINED when stencil is enabled");
            if (OpDesc.StencilFunc == COMPARISON_FUNC_UNKNOWN)
                LOG_PSO_ERROR_AND_THROW("DepthStencilDesc.", FaceName, ".StencilFunc must not be COMPARISON_FUNC_UNKNOWN when stencil is enabled");
        }
    };
    CheckStencilOpDesc(DSSDesc.FrontFace, "FrontFace");
    CheckStencilOpDesc(DSSDesc.BackFace, "BackFace");
}

void CorrectDepthStencilDesc(GraphicsPipelineDesc& GraphicsPipeline) noexcept
{
    auto& DSSDesc = GraphicsPipeline.DepthStencilDesc;
    if (!DSSDesc.DepthEnable && DSSDesc.DepthFunc == COMPARISON_FUNC_UNKNOWN)
        DSSDesc.DepthFunc = DepthStencilStateDesc{}.DepthFunc;

    auto CorrectStencilOpDesc = [&](StencilOpDesc& OpDesc) //
    {
        if (!DSSDesc.StencilEnable)
        {
            if (OpDesc.StencilFailOp == STENCIL_OP_UNDEFINED)
                OpDesc.StencilFailOp = StencilOpDesc{}.StencilFailOp;
            if (OpDesc.StencilDepthFailOp == STENCIL_OP_UNDEFINED)
                OpDesc.StencilDepthFailOp = StencilOpDesc{}.StencilDepthFailOp;
            if (OpDesc.StencilPassOp == STENCIL_OP_UNDEFINED)
                OpDesc.StencilPassOp = StencilOpDesc{}.StencilPassOp;
            if (OpDesc.StencilFunc == COMPARISON_FUNC_UNKNOWN)
                OpDesc.StencilFunc = StencilOpDesc{}.StencilFunc;
        }
    };
    CorrectStencilOpDesc(DSSDesc.FrontFace);
    CorrectStencilOpDesc(DSSDesc.BackFace);
}

void ValidateBlendStateDesc(const PipelineStateDesc& PSODesc, const GraphicsPipelineDesc& GraphicsPipeline) noexcept(false)
{
    const auto& BlendDesc = GraphicsPipeline.BlendDesc;
    for (Uint32 rt = 0; rt < MAX_RENDER_TARGETS; ++rt)
    {
        auto& RTDesc = BlendDesc.RenderTargets[rt];

        const auto BlendEnable = RTDesc.BlendEnable && (rt == 0 || (BlendDesc.IndependentBlendEnable && rt > 0));
        if (BlendEnable)
        {
            if (RTDesc.SrcBlend == BLEND_FACTOR_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("BlendDesc.RenderTargets[", rt, "].SrcBlend must not be BLEND_FACTOR_UNDEFINED");
            if (RTDesc.DestBlend == BLEND_FACTOR_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("BlendDesc.RenderTargets[", rt, "].DestBlend must not be BLEND_FACTOR_UNDEFINED");
            if (RTDesc.BlendOp == BLEND_OPERATION_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("BlendDesc.RenderTargets[", rt, "].BlendOp must not be BLEND_OPERATION_UNDEFINED");

            if (RTDesc.SrcBlendAlpha == BLEND_FACTOR_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("BlendDesc.RenderTargets[", rt, "].SrcBlendAlpha must not be BLEND_FACTOR_UNDEFINED");
            if (RTDesc.DestBlendAlpha == BLEND_FACTOR_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("BlendDesc.RenderTargets[", rt, "].DestBlendAlpha must not be BLEND_FACTOR_UNDEFINED");
            if (RTDesc.BlendOpAlpha == BLEND_OPERATION_UNDEFINED)
                LOG_PSO_ERROR_AND_THROW("BlendDesc.RenderTargets[", rt, "].BlendOpAlpha must not be BLEND_OPERATION_UNDEFINED");
        }
    }
}

void CorrectBlendStateDesc(GraphicsPipelineDesc& GraphicsPipeline) noexcept
{
    auto& BlendDesc = GraphicsPipeline.BlendDesc;
    for (Uint32 rt = 0; rt < MAX_RENDER_TARGETS; ++rt)
    {
        auto& RTDesc = BlendDesc.RenderTargets[rt];
        // clang-format off
        const auto  BlendEnable   = RTDesc.BlendEnable          && (rt == 0 || (BlendDesc.IndependentBlendEnable && rt > 0));
        const auto  LogicOpEnable = RTDesc.LogicOperationEnable && (rt == 0 || (BlendDesc.IndependentBlendEnable && rt > 0));
        // clang-format on
        if (!BlendEnable)
        {
            if (RTDesc.SrcBlend == BLEND_FACTOR_UNDEFINED)
                RTDesc.SrcBlend = RenderTargetBlendDesc{}.SrcBlend;
            if (RTDesc.DestBlend == BLEND_FACTOR_UNDEFINED)
                RTDesc.DestBlend = RenderTargetBlendDesc{}.DestBlend;
            if (RTDesc.BlendOp == BLEND_OPERATION_UNDEFINED)
                RTDesc.BlendOp = RenderTargetBlendDesc{}.BlendOp;

            if (RTDesc.SrcBlendAlpha == BLEND_FACTOR_UNDEFINED)
                RTDesc.SrcBlendAlpha = RenderTargetBlendDesc{}.SrcBlendAlpha;
            if (RTDesc.DestBlendAlpha == BLEND_FACTOR_UNDEFINED)
                RTDesc.DestBlendAlpha = RenderTargetBlendDesc{}.DestBlendAlpha;
            if (RTDesc.BlendOpAlpha == BLEND_OPERATION_UNDEFINED)
                RTDesc.BlendOpAlpha = RenderTargetBlendDesc{}.BlendOpAlpha;
        }

        if (!LogicOpEnable)
            RTDesc.LogicOp = RenderTargetBlendDesc{}.LogicOp;
    }
}

} // namespace

#define VALIDATE_SHADER_TYPE(Shader, ExpectedType, ShaderName)                                                                           \
    if (Shader != nullptr && Shader->GetDesc().ShaderType != ExpectedType)                                                               \
    {                                                                                                                                    \
        LOG_ERROR_AND_THROW(GetShaderTypeLiteralName(Shader->GetDesc().ShaderType), " is not a valid type for ", ShaderName, " shader"); \
    }

void ValidateGraphicsPipelineCreateInfo(const GraphicsPipelineStateCreateInfo& CreateInfo) noexcept(false)
{
    const auto& PSODesc = CreateInfo.PSODesc;
    if (PSODesc.PipelineType != PIPELINE_TYPE_GRAPHICS && PSODesc.PipelineType != PIPELINE_TYPE_MESH)
        LOG_PSO_ERROR_AND_THROW("Pipeline type must be GRAPHICS or MESH");

    const auto& GraphicsPipeline = CreateInfo.GraphicsPipeline;

    ValidateBlendStateDesc(PSODesc, GraphicsPipeline);
    ValidateRasterizerStateDesc(PSODesc, GraphicsPipeline);
    ValidateDepthStencilDesc(PSODesc, GraphicsPipeline);


    if (PSODesc.PipelineType == PIPELINE_TYPE_GRAPHICS)
    {
        if (CreateInfo.pVS == nullptr)
            LOG_ERROR_AND_THROW("Vertex shader must not be null");

        DEV_CHECK_ERR(CreateInfo.pAS == nullptr && CreateInfo.pMS == nullptr, "Mesh shaders are not supported in graphics pipeline");
    }
    else if (PSODesc.PipelineType == PIPELINE_TYPE_MESH)
    {
        if (CreateInfo.pMS == nullptr)
            LOG_ERROR_AND_THROW("Mesh shader must not be null");

        DEV_CHECK_ERR(CreateInfo.pVS == nullptr && CreateInfo.pGS == nullptr && CreateInfo.pDS == nullptr && CreateInfo.pHS == nullptr,
                      "Vertex, geometry and tessellation shaders are not supported in a mesh pipeline");
        DEV_CHECK_ERR(GraphicsPipeline.InputLayout.NumElements == 0, "Input layout is ignored in a mesh pipeline");
        DEV_CHECK_ERR(GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ||
                          GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_UNDEFINED,
                      "Primitive topology is ignored in a mesh pipeline, set it to undefined or keep default value (triangle list)");
    }


    VALIDATE_SHADER_TYPE(CreateInfo.pVS, SHADER_TYPE_VERTEX, "vertex")
    VALIDATE_SHADER_TYPE(CreateInfo.pPS, SHADER_TYPE_PIXEL, "pixel")
    VALIDATE_SHADER_TYPE(CreateInfo.pGS, SHADER_TYPE_GEOMETRY, "geometry")
    VALIDATE_SHADER_TYPE(CreateInfo.pHS, SHADER_TYPE_HULL, "hull")
    VALIDATE_SHADER_TYPE(CreateInfo.pDS, SHADER_TYPE_DOMAIN, "domain")
    VALIDATE_SHADER_TYPE(CreateInfo.pAS, SHADER_TYPE_AMPLIFICATION, "amplification")
    VALIDATE_SHADER_TYPE(CreateInfo.pMS, SHADER_TYPE_MESH, "mesh")


    if (GraphicsPipeline.pRenderPass != nullptr)
    {
        if (GraphicsPipeline.NumRenderTargets != 0)
            LOG_PSO_ERROR_AND_THROW("NumRenderTargets must be 0 when explicit render pass is used");
        if (GraphicsPipeline.DSVFormat != TEX_FORMAT_UNKNOWN)
            LOG_PSO_ERROR_AND_THROW("DSVFormat must be TEX_FORMAT_UNKNOWN when explicit render pass is used");

        for (Uint32 rt = 0; rt < MAX_RENDER_TARGETS; ++rt)
        {
            if (GraphicsPipeline.RTVFormats[rt] != TEX_FORMAT_UNKNOWN)
                LOG_PSO_ERROR_AND_THROW("RTVFormats[", rt, "] must be TEX_FORMAT_UNKNOWN when explicit render pass is used");
        }

        const auto& RPDesc = GraphicsPipeline.pRenderPass->GetDesc();
        if (GraphicsPipeline.SubpassIndex >= RPDesc.SubpassCount)
            LOG_PSO_ERROR_AND_THROW("Subpass index (", Uint32{GraphicsPipeline.SubpassIndex}, ") exceeds the number of subpasses (", Uint32{RPDesc.SubpassCount}, ") in render pass '", RPDesc.Name, "'");
    }
    else
    {
        for (Uint32 rt = GraphicsPipeline.NumRenderTargets; rt < _countof(GraphicsPipeline.RTVFormats); ++rt)
        {
            auto RTVFmt = GraphicsPipeline.RTVFormats[rt];
            if (RTVFmt != TEX_FORMAT_UNKNOWN)
            {
                LOG_ERROR_MESSAGE("Render target format (", GetTextureFormatAttribs(RTVFmt).Name, ") of unused slot ", rt,
                                  " must be set to TEX_FORMAT_UNKNOWN");
            }
        }

        if (GraphicsPipeline.SubpassIndex != 0)
            LOG_PSO_ERROR_AND_THROW("Subpass index (", Uint32{GraphicsPipeline.SubpassIndex}, ") must be 0 when explicit render pass is not used");
    }
}

void ValidateComputePipelineCreateInfo(const ComputePipelineStateCreateInfo& CreateInfo) noexcept(false)
{
    const auto& PSODesc = CreateInfo.PSODesc;
    if (PSODesc.PipelineType != PIPELINE_TYPE_COMPUTE)
        LOG_PSO_ERROR_AND_THROW("Pipeline type must be COMPUTE");

    if (CreateInfo.pCS == nullptr)
        LOG_ERROR_AND_THROW("Compute shader must not be null");

    VALIDATE_SHADER_TYPE(CreateInfo.pCS, SHADER_TYPE_COMPUTE, "compute");
}
#undef VALIDATE_SHADER_TYPE
#undef LOG_PSO_ERROR_AND_THROW

void CorrectGraphicsPipelineDesc(GraphicsPipelineDesc& GraphicsPipeline) noexcept
{
    CorrectBlendStateDesc(GraphicsPipeline);
    CorrectDepthStencilDesc(GraphicsPipeline);
}

} // namespace Diligent
