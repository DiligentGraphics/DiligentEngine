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

// This file is originally based on obj_renderer.cc
// arcore-android-sdk (https://github.com/google-ar/arcore-android-sdk)

#include "ObjectRenderer.h"
#include "../../../../../Tutorials/Common/src/TexturedCube.hpp"
#include "MapHelper.hpp"

using namespace Diligent;

namespace hello_ar
{

namespace
{

const float4 kLightDirection(0.0f, 1.0f, 0.0f, 0.0f);

} // namespace

ObjRenderer::ObjRenderer() :
    m_CubeTransformMat{float4x4::Identity()}
{
    // Inverse Z axis
    m_CubeTransformMat._33 = -1;
    // Translate to align bottom face with 0
    m_CubeTransformMat = float4x4::Translation(0, 1, 0).Transpose() * m_CubeTransformMat;
    // Scale
    m_CubeTransformMat = float4x4::Scale(0.075f) * m_CubeTransformMat;
}

void ObjRenderer::Initialize(IRenderDevice* pDevice)
{
    // Create shader constant buffer
    {
        BufferDesc BuffDesc;
        BuffDesc.Name           = "Object attribs constant buffer";
        BuffDesc.uiSizeInBytes  = sizeof(float4x4) * 2 + sizeof(float4) * 3;
        BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage          = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(BuffDesc, nullptr, &m_pShaderConstants);
        VERIFY(m_pShaderConstants, "Failed to create object attribs constant buffer");
    }

    // Create cube vertex and index buffers
    m_pCubeVertexBuffer = TexturedCube::CreateVertexBuffer(pDevice);
    VERIFY(m_pCubeVertexBuffer, "Failed to create cube vertex buffer");

    m_pCubeIndexBuffer = TexturedCube::CreateIndexBuffer(pDevice);
    VERIFY(m_pCubeIndexBuffer, "Failed to create cube index buffer");

    // Load cube texture
    auto pCubeTexture = TexturedCube::LoadTexture(pDevice, "textures/DGLogo.png");

    // Create pipeline state
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

        PSODesc.Name = "Object rendering PSO";

        PSODesc.PipelineType                                        = PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM;
        PSOCreateInfo.GraphicsPipeline.DSVFormat                    = TEX_FORMAT_D24_UNORM_S8_UINT;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

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
            ShaderCI.Desc.Name       = "Object vertex shader";
            ShaderCI.FilePath        = "shaders/object.vsh";
            pDevice->CreateShader(ShaderCI, &pVS);
            VERIFY(pVS, "Failed to create object vertex shader");
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Object fragment shader";
            ShaderCI.FilePath        = "shaders/object.psh";
            pDevice->CreateShader(ShaderCI, &pPS);
            VERIFY(pPS, "Failed to create object fragment shader");
        }

        // clang-format off
        LayoutElement LayoutElems[] =
        {
            LayoutElement{0, 0, 3, VT_FLOAT32, False}, // Position
            LayoutElement{1, 0, 2, VT_FLOAT32, False}  // UV
        };
        // clang-format on

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = 2;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pObjectPSO);
        VERIFY(m_pObjectPSO, "Failed to create object PSO");

        m_pObjectPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_pShaderConstants);
        m_pObjectPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "Constants")->Set(m_pShaderConstants);
        m_pObjectPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pCubeTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_pObjectPSO->CreateShaderResourceBinding(&m_pObjectSRB, true);
    }
}

void ObjRenderer::Draw(IDeviceContext* pContext,
                       const float4x4& projection_mat,
                       const float4x4& view_mat,
                       const float4x4& model_mat,
                       const float*    color_correction4,
                       const float*    object_color4)
{
    float4x4 mv_mat  = view_mat * model_mat * m_CubeTransformMat;
    float4x4 mvp_mat = projection_mat * mv_mat;


    float4 view_light_direction = normalize(mv_mat * kLightDirection);

    {
        struct ShaderConstants
        {
            float4x4 ModelView;
            float4x4 ModelViewProjection;

            float4 LightingParameters;
            float4 ColorCorrectionParameters;
            float4 ObjectColor;
        };
        MapHelper<ShaderConstants> ShaderData(pContext, m_pShaderConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        ShaderData->ModelView                 = mv_mat;
        ShaderData->ModelViewProjection       = mvp_mat;
        ShaderData->LightingParameters        = float4(view_light_direction.x, view_light_direction.y, view_light_direction.z, 1.f);
        ShaderData->ColorCorrectionParameters = float4(color_correction4[0], color_correction4[1], color_correction4[2], color_correction4[3]);
        ShaderData->ObjectColor               = float4(object_color4[0], object_color4[1], object_color4[2], object_color4[3]);
    }

    // Bind vertex and index buffers
    Uint32   offsets[] = {0};
    IBuffer* pBuffs[]  = {m_pCubeVertexBuffer};
    pContext->SetVertexBuffers(0, 1, pBuffs, offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(m_pCubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    pContext->SetPipelineState(m_pObjectPSO);
    pContext->CommitShaderResources(m_pObjectSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = 36;
    DrawAttrs.Flags      = DRAW_FLAG_NONE;
    pContext->DrawIndexed(DrawAttrs);
}

} // namespace hello_ar
