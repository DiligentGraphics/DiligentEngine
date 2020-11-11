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

// This file is originally based on plane_renderer.cc from
// arcore-android-sdk (https://github.com/google-ar/arcore-android-sdk)


#include "PlaneRenderer.h"

#include "Texture.h"
#include "TextureView.h"
#include "TextureUtilities.h"
#include "MapHelper.hpp"

#include "util.h"

using namespace Diligent;

namespace hello_ar
{

void PlaneRenderer::Initialize(IRenderDevice* pDevice)
{
    m_pDevice = pDevice;
    RefCntAutoPtr<ITexture> pGridTexture;
    TextureLoadInfo         loadInfo;
    loadInfo.IsSRGB = true;
    CreateTextureFromFile("textures/trigrid.png", loadInfo, pDevice, &pGridTexture);

    // Create vertex shader constant buffer
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Plane VS attribs constant buffer";
        BuffDesc.uiSizeInBytes  = sizeof(float4x4) * 2 + sizeof(float4) * 2;
        BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(BuffDesc, nullptr, &m_pShaderConstants);
        VERIFY(m_pShaderConstants, "Failed to create plane VS attribs constant buffer");
    }

    // Create pipeline state
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

        PSODesc.Name = "Plane rendering PSO";

        PSODesc.PipelineType                                                = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets                     = 1;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                        = TEX_FORMAT_RGBA8_UNORM;
        PSOCreateInfo.GraphicsPipeline.DSVFormat                            = TEX_FORMAT_D24_UNORM_S8_UINT;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = True;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable         = True;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable    = False;

        auto& RT0       = PSOCreateInfo.GraphicsPipeline.BlendDesc.RenderTargets[0];
        RT0.BlendEnable = True;
        RT0.SrcBlend    = BLEND_FACTOR_SRC_ALPHA;
        RT0.DestBlend   = BLEND_FACTOR_INV_SRC_ALPHA;
        RT0.BlendOp     = BLEND_OPERATION_ADD;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = true;

        // Create a shader source stream factory to load shaders from files.
        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Plane vertex shader";
            ShaderCI.FilePath        = "shaders/plane.vsh";
            pDevice->CreateShader(ShaderCI, &pVS);
            VERIFY(pVS, "Failed to create plane vertex shader");
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Object fragment shader";
            ShaderCI.FilePath        = "shaders/plane.psh";
            pDevice->CreateShader(ShaderCI, &pPS);
            VERIFY(pPS, "Failed to create plane fragment shader");
        }

        // clang-format off
        LayoutElement LayoutElems[] =
        {
            LayoutElement{0, 0, 3, VT_FLOAT32, False}  // Position
        };
        // clang-format on

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = 1;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPlanePSO);
        VERIFY(m_pPlanePSO, "Failed to create plane PSO");

        m_pPlanePSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_pShaderConstants);
        m_pPlanePSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(m_pShaderConstants);
        m_pPlanePSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pGridTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_pPlanePSO->CreateShaderResourceBinding(&m_pPlaneSRB, true);
    }
}

void PlaneRenderer::Draw(IDeviceContext*  pContext,
                         const float4x4&  projection_mat,
                         const float4x4&  view_mat,
                         const ArSession& ar_session,
                         const ArPlane&   ar_plane,
                         const float3&    color)
{
    UpdateForPlane(ar_session, ar_plane);

    // Compose final mvp matrix for this plane renderer.
    auto mvp = projection_mat * view_mat * m_ModelMat;

    {
        struct ShaderConstants
        {
            float4x4 MVP;
            float4x4 ModelMat;
            float4   Normal;
            float4   Color;
        };
        MapHelper<ShaderConstants> ShaderData(pContext, m_pShaderConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        ShaderData->MVP      = mvp;
        ShaderData->ModelMat = m_ModelMat;
        ShaderData->Normal   = float4(m_Normal, 1.f);
        ShaderData->Color    = float4(color, 1.f);
    }


    // Update plane vertex buffer
    if (m_pPlaneVertexBuffer)
    {
        const auto& Desc = m_pPlaneVertexBuffer->GetDesc();
        if (Desc.uiSizeInBytes < m_Vertices.size() * sizeof(float3))
        {
            // The buffer is too small - release it and create a new one
            m_pPlaneVertexBuffer.Release();
        }
    }

    if (!m_pPlaneVertexBuffer)
    {
        BufferDesc VBDesc;
        VBDesc.Name          = "Plane vertex buffer";
        VBDesc.uiSizeInBytes = sizeof(float3) * 32;
        while (VBDesc.uiSizeInBytes < m_Vertices.size() * sizeof(float3))
            VBDesc.uiSizeInBytes *= 2;
        VBDesc.BindFlags      = BIND_VERTEX_BUFFER;
        VBDesc.Usage          = USAGE_DYNAMIC;
        VBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(VBDesc, nullptr, &m_pPlaneVertexBuffer);
        VERIFY(m_pPlaneVertexBuffer, "Failed to create plane vertex buffer");
    }

    {
        MapHelper<float3> PlaneBufferData(pContext, m_pPlaneVertexBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        memcpy(PlaneBufferData, m_Vertices.data(), sizeof(float3) * m_Vertices.size());
    }


    // Update plane index buffer
    if (m_pPlaneIndexBuffer)
    {
        const auto& Desc = m_pPlaneIndexBuffer->GetDesc();
        if (Desc.uiSizeInBytes < m_Indices.size() * sizeof(Uint32))
        {
            // The buffer is too small - release it and create a new one
            m_pPlaneIndexBuffer.Release();
        }
    }

    if (!m_pPlaneIndexBuffer)
    {
        BufferDesc IBDesc;
        IBDesc.Name          = "Plane index buffer";
        IBDesc.uiSizeInBytes = sizeof(Uint32) * 32;
        while (IBDesc.uiSizeInBytes < m_Indices.size() * sizeof(Uint32))
            IBDesc.uiSizeInBytes *= 2;
        IBDesc.BindFlags      = BIND_INDEX_BUFFER;
        IBDesc.Usage          = USAGE_DYNAMIC;
        IBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pDevice->CreateBuffer(IBDesc, nullptr, &m_pPlaneIndexBuffer);
        VERIFY(m_pPlaneIndexBuffer, "Failed to create plane vertex buffer");
    }

    {
        MapHelper<Uint32> PlaneIndexData(pContext, m_pPlaneIndexBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        memcpy(PlaneIndexData, m_Indices.data(), sizeof(Uint32) * m_Indices.size());
    }

    // Bind vertex and index buffers
    Uint32   offsets[] = {0};
    IBuffer* pBuffs[]  = {m_pPlaneVertexBuffer};
    pContext->SetVertexBuffers(0, 1, pBuffs, offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(m_pPlaneIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    pContext->SetPipelineState(m_pPlanePSO);
    pContext->CommitShaderResources(m_pPlaneSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = m_Indices.size();
    DrawAttrs.Flags      = DRAW_FLAG_NONE;
    pContext->DrawIndexed(DrawAttrs);
}

void PlaneRenderer::UpdateForPlane(const ArSession& ar_session,
                                   const ArPlane&   ar_plane)
{
    // The following code generates a triangle mesh filling a convex polygon,
    // including a feathered edge for blending.
    //
    // The indices shown in the diagram are used in comments below.
    // _______________     0_______________1
    // |             |      |4___________5|
    // |             |      | |         | |
    // |             | =>   | |         | |
    // |             |      | |         | |
    // |             |      |7-----------6|
    // ---------------     3---------------2

    m_Vertices.clear();
    m_Indices.clear();

    int32_t polygon_length;
    ArPlane_getPolygonSize(&ar_session, &ar_plane, &polygon_length);

    if (polygon_length == 0)
    {
        LOG_WARNING_MESSAGE("PlaneRenderer::UpdatePlane, no valid plane polygon is found");
        return;
    }

    const int32_t       vertices_size = polygon_length / 2;
    std::vector<float2> raw_vertices(vertices_size);
    ArPlane_getPolygon(&ar_session, &ar_plane, raw_vertices.front().Data());

    // Fill vertex 0 to 3. Note that the vertex.xy are used for x and z
    // position. vertex.z is used for alpha. The outter polygon's alpha
    // is 0.
    for (int32_t i = 0; i < vertices_size; ++i)
    {
        m_Vertices.push_back(float3(raw_vertices[i].x, raw_vertices[i].y, 0.0f));
    }

    {
        util::ScopedArPose scopedArPose(&ar_session);
        ArPlane_getCenterPose(&ar_session, &ar_plane, scopedArPose.GetArPose());
        ArPose_getMatrix(&ar_session, scopedArPose.GetArPose(), m_ModelMat.Data());
        m_ModelMat = m_ModelMat.Transpose();
        m_Normal   = util::GetPlaneNormal(ar_session, *scopedArPose.GetArPose());
    }

    // Feather distance 0.2 meters.
    const float kFeatherLength = 0.2f;
    // Feather scale over the distance between plane center and vertices.
    const float kFeatherScale = 0.2f;

    // Fill vertex 4 to 7, with alpha set to 1.
    for (int32_t i = 0; i < vertices_size; ++i)
    {
        // Vector from plane center to current point.
        float2      v = raw_vertices[i];
        const float scale =
            1.0f - std::min((kFeatherLength / length(v)), kFeatherScale);
        const float2 result_v = scale * v;

        m_Vertices.push_back(float3(result_v.x, result_v.y, 1.0f));
    }

    const int32_t vertices_length      = m_Vertices.size();
    const int32_t half_vertices_length = vertices_length / 2;

    // Generate triangle (4, 5, 6) and (4, 6, 7).
    for (int i = half_vertices_length + 1; i < vertices_length - 1; ++i)
    {
        m_Indices.push_back(half_vertices_length);
        m_Indices.push_back(i);
        m_Indices.push_back(i + 1);
    }

    // Generate triangle (0, 1, 4), (4, 1, 5), (5, 1, 2), (5, 2, 6),
    // (6, 2, 3), (6, 3, 7), (7, 3, 0), (7, 0, 4)
    for (int i = 0; i < half_vertices_length; ++i)
    {
        m_Indices.push_back(i);
        m_Indices.push_back((i + 1) % half_vertices_length);
        m_Indices.push_back(i + half_vertices_length);

        m_Indices.push_back(i + half_vertices_length);
        m_Indices.push_back((i + 1) % half_vertices_length);
        m_Indices.push_back((i + half_vertices_length + 1) % half_vertices_length +
                            half_vertices_length);
    }
}

} // namespace hello_ar
