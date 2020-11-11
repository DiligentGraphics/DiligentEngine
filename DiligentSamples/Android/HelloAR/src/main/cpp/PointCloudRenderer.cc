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

// This file is originally based on point_cloud_renderer.cc from
// arcore-android-sdk (https://github.com/google-ar/arcore-android-sdk)

#include "PointCloudRenderer.h"
#include "MapHelper.hpp"

using namespace Diligent;

namespace hello_ar
{

namespace
{

constexpr char kVertexShaderSource[] = R"(
cbuffer Constants
{
    float4x4 g_WorldViewProj;
    float4 g_Color;
}

struct VSInput
{
    float4 Pos : ATTRIB0;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos.xyz, 1.0), g_WorldViewProj);
    PSIn.Color = g_Color;
#ifdef GL_ES
    gl_PointSize = 5.0;
#endif
}
)";

constexpr char kFragmentShaderSource[] = R"(
struct PSInput
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = PSIn.Color;
}
)";

} // namespace

void PointCloudRenderer::Initialize(IRenderDevice* pDevice)
{
    m_pDevice = pDevice;

    // Create vertex shader constant buffer
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Point cloud VS attribs constant buffer";
        BuffDesc.uiSizeInBytes  = sizeof(float4x4) + sizeof(float4);
        BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(BuffDesc, nullptr, &m_pVSConstants);
        VERIFY(m_pVSConstants, "Failed to create point cloud VS attribs constant buffer");
    }

    // Create pipeline state
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

        PSODesc.Name = "Point cloud PSO";

        PSODesc.PipelineType                                             = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets                  = 1;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                     = TEX_FORMAT_RGBA8_UNORM;
        PSOCreateInfo.GraphicsPipeline.DSVFormat                         = TEX_FORMAT_D24_UNORM_S8_UINT;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_POINT_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode           = CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = True;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Point cloud vertex shader";
            ShaderCI.Source          = kVertexShaderSource;
            pDevice->CreateShader(ShaderCI, &pVS);
            VERIFY(pVS, "Failed to create point cloud vertex shader");
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Point cloud fragment shader";
            ShaderCI.Source          = kFragmentShaderSource;
            pDevice->CreateShader(ShaderCI, &pPS);
            VERIFY(pPS, "Failed to create point cloud fragment shader");
        }

        LayoutElement LayoutElems[] = {LayoutElement{0, 0, 4, VT_FLOAT32, False}};

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = 1;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPointCloudPSO);
        VERIFY(m_pPointCloudPSO, "Failed to create point cloud PSO");

        m_pPointCloudPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_pVSConstants);
        m_pPointCloudPSO->CreateShaderResourceBinding(&m_pPointCloudSRB, true);
    }
}

void PointCloudRenderer::Draw(IDeviceContext*           pContext,
                              const Diligent::float4x4& MVPMatrix,
                              ArSession*                ARSession,
                              ArPointCloud*             PointCloud)
{
    int32_t number_of_points = 0;
    ArPointCloud_getNumberOfPoints(ARSession, PointCloud, &number_of_points);
    if (number_of_points <= 0)
        return;

    // Create point cloud vertex buffer
    if (m_pPointCloudBuffer)
    {
        const auto& Desc = m_pPointCloudBuffer->GetDesc();
        if (Desc.uiSizeInBytes < number_of_points * sizeof(float4))
        {
            // The buffer is too small - release it and create a new one
            m_pPointCloudBuffer.Release();
        }
    }

    if (!m_pPointCloudBuffer)
    {
        BufferDesc VBDesc;
        VBDesc.Name          = "Point cloud vertex buffer";
        VBDesc.uiSizeInBytes = sizeof(float4) * 64;
        while (VBDesc.uiSizeInBytes < number_of_points * sizeof(float4))
            VBDesc.uiSizeInBytes *= 2;
        VBDesc.BindFlags      = BIND_VERTEX_BUFFER;
        VBDesc.Usage          = USAGE_DYNAMIC;
        VBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(VBDesc, nullptr, &m_pPointCloudBuffer);
        VERIFY(m_pPointCloudBuffer, "Failed to create point cloud vertex buffer");
    }

    {
        MapHelper<float4> PointCloudBufferData(pContext, m_pPointCloudBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        const float*      point_cloud_data;
        ArPointCloud_getData(ARSession, PointCloud, &point_cloud_data);
        memcpy(PointCloudBufferData, point_cloud_data, sizeof(float4) * number_of_points);
    }

    {
        struct VSConstants
        {
            float4x4 WorldViewProj;
            float4   Color;
        };
        MapHelper<VSConstants> VSData(pContext, m_pVSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        VSData->WorldViewProj = MVPMatrix;
        VSData->Color         = float4(31.0f / 255.0f, 188.0f / 255.0f, 210.0f / 255.0f, 1.0f);
    }

    pContext->SetPipelineState(m_pPointCloudPSO);
    pContext->CommitShaderResources(m_pPointCloudSRB, RESOURCE_STATE_TRANSITION_MODE_NONE);
    IBuffer* pVBs[]    = {m_pPointCloudBuffer};
    Uint32   Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    DrawAttribs Attribs{static_cast<Uint32>(number_of_points), DRAW_FLAG_NONE};
    pContext->Draw(Attribs);
}

} // namespace hello_ar
