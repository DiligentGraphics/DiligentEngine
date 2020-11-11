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

#include <random>
#include <string>

#include "Tutorial16_BindlessResources.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ShaderMacroHelper.hpp"
#include "imgui.h"
#include "ImGuiUtils.hpp"

namespace Diligent
{

namespace
{

// Layout of this structure matches the one we defined in the pipeline state
struct Vertex
{
    float3 pos;
    float2 uv;
};

} // namespace

SampleBase* CreateSample()
{
    return new Tutorial16_BindlessResources();
}

void Tutorial16_BindlessResources::CreatePipelineState()
{
    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

    // This is a graphics pipeline
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSODesc.Name = "Cube PSO";

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = "cube.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        CreateUniformBuffer(m_pDevice, sizeof(float4x4) * 2, "VS constants CB", &m_VSConstants);
        StateTransitionDesc Barrier{m_VSConstants, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true};
        m_pImmediateContext->TransitionResourceStates(1, &Barrier);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS, pBindlessPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "cube.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);

        if (m_pDevice->GetDeviceCaps().Features.BindlessResources)
        {
            ShaderMacroHelper Macros;
            Macros.AddShaderMacro("BINDLESS", 1);
            Macros.AddShaderMacro("NUM_TEXTURES", NumTextures);
            ShaderCI.Macros = Macros;
            m_pDevice->CreateShader(ShaderCI, &pBindlessPS);
            ShaderCI.Macros = nullptr;
        }
    }

    // clang-format off
    // Define vertex shader input layout
    // This tutorial uses two types of input: per-vertex data and per-instance data.
    LayoutElement LayoutElems[] =
    {
        // Per-vertex data - first buffer slot
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - texture coordinates
        LayoutElement{1, 0, 2, VT_FLOAT32, False},
            
        // Per-instance data - second buffer slot
        // We will use four attributes to encode instance-specific 4x4 transformation matrix
        // Attribute 2 - first row
        LayoutElement{2, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
        // Attribute 3 - second row
        LayoutElement{3, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
        // Attribute 4 - third row
        LayoutElement{4, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
        // Attribute 5 - fourth row
        LayoutElement{5, 1, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
        // Attribute 6 - texture array index
        LayoutElement{6, 1, 1, VT_UINT32,  False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
    };
    // clang-format on

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    // Define variable type that will be used by default
    PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSODesc.ResourceLayout.Variables    = Vars;
    PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // clang-format off
    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
    };
    // clang-format on
    PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
    // never change and are bound directly to the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/

    for (Uint32 i = 0; i < NumTextures; ++i)
    {
        m_pPSO->CreateShaderResourceBinding(&m_SRB[i], true);
    }

    if (pBindlessPS)
    {
        PSOCreateInfo.pPS = pBindlessPS;
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pBindlessPSO);
        m_pBindlessPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
        m_pBindlessPSO->CreateShaderResourceBinding(&m_BindlessSRB, true);
        m_BindlessMode = true;
    }
}

namespace
{

Tutorial16_BindlessResources::ObjectGeometry AddCube(std::vector<Vertex>& Vertices,
                                                     std::vector<Uint32>& Indices,
                                                     const float3&        f3TopScale,
                                                     const float3&        f3BottomScale)
{
    // Cube vertices

    //      (-1,+1,+1)________________(+1,+1,+1)
    //               /|              /|
    //              / |             / |
    //             /  |            /  |
    //            /   |           /   |
    //(-1,-1,+1) /____|__________/(+1,-1,+1)
    //           |    |__________|____|
    //           |   /(-1,+1,-1) |    /(+1,+1,-1)
    //           |  /            |   /
    //           | /             |  /
    //           |/              | /
    //           /_______________|/
    //        (-1,-1,-1)       (+1,-1,-1)
    //

    auto BaseVertex = static_cast<Uint32>(Vertices.size());

    // clang-format off
    Vertices.insert(Vertices.end(),
        {
            {float3(-1,-1,-1) * f3BottomScale, float2(0,1)},
            {float3(-1,+1,-1) * f3BottomScale, float2(0,0)},
            {float3(+1,+1,-1) * f3BottomScale, float2(1,0)},
            {float3(+1,-1,-1) * f3BottomScale, float2(1,1)},

            {float3(-1,-1,-1) * f3BottomScale, float2(0,1)},
            {float3(-1,-1,+1) * f3TopScale,    float2(0,0)},
            {float3(+1,-1,+1) * f3TopScale,    float2(1,0)},
            {float3(+1,-1,-1) * f3BottomScale, float2(1,1)},

            {float3(+1,-1,-1) * f3BottomScale, float2(0,1)},
            {float3(+1,-1,+1) * f3TopScale,    float2(1,1)},
            {float3(+1,+1,+1) * f3TopScale,    float2(1,0)},
            {float3(+1,+1,-1) * f3BottomScale, float2(0,0)},

            {float3(+1,+1,-1) * f3BottomScale, float2(0,1)},
            {float3(+1,+1,+1) * f3TopScale,    float2(0,0)},
            {float3(-1,+1,+1) * f3TopScale,    float2(1,0)},
            {float3(-1,+1,-1) * f3BottomScale, float2(1,1)},

            {float3(-1,+1,-1) * f3BottomScale, float2(1,0)},
            {float3(-1,+1,+1) * f3TopScale,    float2(0,0)},
            {float3(-1,-1,+1) * f3TopScale,    float2(0,1)},
            {float3(-1,-1,-1) * f3BottomScale, float2(1,1)},

            {float3(-1,-1,+1) * f3TopScale,    float2(1,1)},
            {float3(+1,-1,+1) * f3TopScale,    float2(0,1)},
            {float3(+1,+1,+1) * f3TopScale,    float2(0,0)},
            {float3(-1,+1,+1) * f3TopScale,    float2(1,0)}
        }
    );
    // clang-format on

    Tutorial16_BindlessResources::ObjectGeometry Geometry;
    Geometry.FirstIndex = static_cast<Uint32>(Indices.size());
    // clang-format off
    Indices.insert(Indices.end(),
        {
            2,0,1,    2,3,0,
            4,6,5,    4,7,6,
            8,10,9,   8,11,10,
            12,14,13, 12,15,14,
            16,18,17, 16,19,18,
            20,21,22, 20,22,23
        }
    );
    // clang-format on

    Geometry.NumIndices = static_cast<Uint32>(Indices.size()) - Geometry.FirstIndex;
    for (Uint32 i = Geometry.FirstIndex; i < Geometry.FirstIndex + Geometry.NumIndices; ++i)
        Indices[i] += BaseVertex;
    return Geometry;
}

Tutorial16_BindlessResources::ObjectGeometry AddPyramid(std::vector<Vertex>& Vertices, std::vector<Uint32>& Indices)
{
    //          4-7
    //           *
    //       1_______2
    //       /      /
    //      /______/
    //     0       3
    //

    auto BaseVertex = static_cast<Uint32>(Vertices.size());

    // clang-format off
    Vertices.insert(Vertices.end(),
        {
            {float3(-1,-1,-1), float2(0,1)},
            {float3(-1,+1,-1), float2(0,0)},
            {float3(+1,+1,-1), float2(1,0)},
            {float3(+1,-1,-1), float2(1,1)},

            {float3(0, 0, +1), float2(1,1)},
            {float3(0, 0, +1), float2(1,1)},
            {float3(0, 0, +1), float2(0,0)},
            {float3(0, 0, +1), float2(0,0)}
        }
    );
    // clang-format on

    Tutorial16_BindlessResources::ObjectGeometry Geometry;
    Geometry.FirstIndex = static_cast<Uint32>(Indices.size());

    // clang-format off
    Indices.insert(Indices.end(),
        {
            2,0,1, 2,3,0,
            0,4,1,
            1,5,2,
            2,6,3,
            3,7,0
        }
    );
    // clang-format on

    Geometry.NumIndices = static_cast<Uint32>(Indices.size()) - Geometry.FirstIndex;
    for (Uint32 i = Geometry.FirstIndex; i < Geometry.FirstIndex + Geometry.NumIndices; ++i)
        Indices[i] += BaseVertex;

    return Geometry;
}

} // namespace

void Tutorial16_BindlessResources::CreateGeometryBuffers()
{
    std::vector<Vertex> Vertices;
    std::vector<Uint32> Indices;

    m_Geometries.emplace_back(AddCube(Vertices, Indices, float3(1, 1, 1), float3(1, 1, 1)));
    m_Geometries.emplace_back(AddCube(Vertices, Indices, float3(1, 1, 1), float3(0.5f, 0.5f, 1.f)));
    m_Geometries.emplace_back(AddCube(Vertices, Indices, float3(0.5f, 1, 1), float3(1, 0.5f, 1.f)));
    m_Geometries.emplace_back(AddCube(Vertices, Indices, float3(1, 1, 1), float3(1, 0.5f, 0.5f)));
    m_Geometries.emplace_back(AddPyramid(Vertices, Indices));


    {
        BufferDesc VertBuffDesc;
        VertBuffDesc.Name          = "Geometry vertex buffer";
        VertBuffDesc.Usage         = USAGE_IMMUTABLE;
        VertBuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
        VertBuffDesc.uiSizeInBytes = static_cast<Uint32>(sizeof(Vertex) * Vertices.size());
        BufferData VBData;
        VBData.pData    = Vertices.data();
        VBData.DataSize = VertBuffDesc.uiSizeInBytes;
        m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_VertexBuffer);
    }

    {
        BufferDesc IndBuffDesc;
        IndBuffDesc.Name          = "Geometry index buffer";
        IndBuffDesc.Usage         = USAGE_IMMUTABLE;
        IndBuffDesc.BindFlags     = BIND_INDEX_BUFFER;
        IndBuffDesc.uiSizeInBytes = static_cast<Uint32>(sizeof(Indices[0]) * Indices.size());
        BufferData IBData;
        IBData.pData    = Indices.data();
        IBData.DataSize = IndBuffDesc.uiSizeInBytes;
        m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_IndexBuffer);
    }
    // clang-format off
    StateTransitionDesc Barriers[2] =
    {
        {m_VertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true},
        {m_IndexBuffer,  RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER,  true}
    };
    // clang-format on
    m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);
}

void Tutorial16_BindlessResources::CreateInstanceBuffer()
{
    // Create instance data buffer that will store transformation matrices
    BufferDesc InstBuffDesc;
    InstBuffDesc.Name = "Instance data buffer";
    // Use default usage as this buffer will only be updated when grid size changes
    InstBuffDesc.Usage         = USAGE_DEFAULT;
    InstBuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
    InstBuffDesc.uiSizeInBytes = sizeof(InstanceData) * MaxInstances;
    m_pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_InstanceBuffer);
    PopulateInstanceBuffer();
}

void Tutorial16_BindlessResources::LoadTextures()
{
    // Load a texture array
    IDeviceObject*          pTexSRVs[NumTextures] = {};
    RefCntAutoPtr<ITexture> pTex[NumTextures];
    StateTransitionDesc     Barriers[NumTextures];
    for (int tex = 0; tex < NumTextures; ++tex)
    {
        // Load current texture
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = true;

        std::stringstream FileNameSS;
        FileNameSS << "DGLogo" << tex << ".png";
        auto FileName = FileNameSS.str();
        CreateTextureFromFile(FileName.c_str(), loadInfo, m_pDevice, &pTex[tex]);

        // Get shader resource view from the texture
        auto* pTextureSRV = pTex[tex]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        m_SRB[tex]->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pTextureSRV);
        pTexSRVs[tex] = pTextureSRV;
        Barriers[tex] = StateTransitionDesc{pTex[tex], RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true};
    }
    m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);

    if (m_BindlessSRB)
    {
        m_BindlessSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->SetArray(pTexSRVs, 0, NumTextures);
    }
}

void Tutorial16_BindlessResources::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::SliderInt("Grid Size", &m_GridSize, 1, 32))
        {
            PopulateInstanceBuffer();
        }
        {
            ImGui::ScopedDisabler Disable(!m_pBindlessPSO);
            ImGui::Checkbox("Bindless mode", &m_BindlessMode);
        }
    }
    ImGui::End();
}

void Tutorial16_BindlessResources::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc)
{
    SampleBase::GetEngineInitializationAttribs(DeviceType, EngineCI, SCDesc);

    EngineCI.Features.BindlessResources = DEVICE_FEATURE_STATE_OPTIONAL;
}

void Tutorial16_BindlessResources::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    CreatePipelineState();
    CreateGeometryBuffers();
    CreateInstanceBuffer();
    LoadTextures();
}

void Tutorial16_BindlessResources::PopulateInstanceBuffer()
{
    // Populate instance data buffer
    m_InstanceData.resize(m_GridSize * m_GridSize * m_GridSize);
    m_GeometryType.resize(m_GridSize * m_GridSize * m_GridSize);

    float fGridSize = static_cast<float>(m_GridSize);

    std::mt19937 gen; // Standard mersenne_twister_engine. Use default seed
                      // to generate consistent distribution.

    std::uniform_real_distribution<float> scale_distr(0.3f, 1.0f);
    std::uniform_real_distribution<float> offset_distr(-0.15f, +0.15f);
    std::uniform_real_distribution<float> rot_distr(-PI_F, +PI_F);
    std::uniform_int_distribution<Uint32> tex_distr(0, NumTextures - 1);
    std::uniform_int_distribution<Uint32> geom_type_distr(0, static_cast<Uint32>(m_Geometries.size()) - 1);

    float BaseScale = 0.6f / fGridSize;
    int   instId    = 0;
    for (int x = 0; x < m_GridSize; ++x)
    {
        for (int y = 0; y < m_GridSize; ++y)
        {
            for (int z = 0; z < m_GridSize; ++z)
            {
                // Add random offset from central position in the grid
                float xOffset = 2.f * (x + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                float yOffset = 2.f * (y + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                float zOffset = 2.f * (z + 0.5f + offset_distr(gen)) / fGridSize - 1.f;
                // Random scale
                float scale = BaseScale * scale_distr(gen);
                // Random rotation
                float4x4 rotation = float4x4::RotationX(rot_distr(gen)) * float4x4::RotationY(rot_distr(gen)) * float4x4::RotationZ(rot_distr(gen));
                // Combine rotation, scale and translation
                float4x4 matrix   = rotation * float4x4::Scale(scale, scale, scale) * float4x4::Translation(xOffset, yOffset, zOffset);
                auto&    CurrInst = m_InstanceData[instId];
                CurrInst.Matrix   = matrix;
                // Texture array index
                CurrInst.TextureInd = tex_distr(gen);

                m_GeometryType[instId++] = geom_type_distr(gen);
            }
        }
    }
    // Update instance data buffer
    Uint32 DataSize = static_cast<Uint32>(sizeof(InstanceData) * m_InstanceData.size());
    m_pImmediateContext->UpdateBuffer(m_InstanceBuffer, 0, DataSize, m_InstanceData.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    StateTransitionDesc Barrier(m_InstanceBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    m_pImmediateContext->TransitionResourceStates(1, &Barrier);
}


// Render a frame
void Tutorial16_BindlessResources::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // Map the buffer and write current world-view-projection matrix
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        CBConstants[0] = m_ViewProjMatrix.Transpose();
        CBConstants[1] = m_RotationMatrix.Transpose();
    }

    // Bind vertex, instance and index buffers
    Uint32   offsets[] = {0, 0};
    IBuffer* pBuffs[]  = {m_VertexBuffer, m_InstanceBuffer};
    m_pImmediateContext->SetVertexBuffers(0, _countof(pBuffs), pBuffs, offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    m_pImmediateContext->SetPipelineState(m_BindlessMode ? m_pBindlessPSO : m_pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    if (m_BindlessMode)
        m_pImmediateContext->CommitShaderResources(m_BindlessSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    auto NumObjects = m_GridSize * m_GridSize * m_GridSize;
    for (int i = 0; i < NumObjects; ++i)
    {
        if (!m_BindlessMode)
        {
            auto TexId = m_InstanceData[i].TextureInd;
            m_pImmediateContext->CommitShaderResources(m_SRB[TexId], RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        }

        const auto& Geometry = m_Geometries[m_GeometryType[i]];

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType             = VT_UINT32;
        DrawAttrs.NumIndices            = Geometry.NumIndices;
        DrawAttrs.FirstIndexLocation    = Geometry.FirstIndex;
        DrawAttrs.FirstInstanceLocation = static_cast<Uint32>(i);
        // Verify the state of vertex and index buffers
        // Also use DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT flag to inform the engine that
        // none of the dynamic buffers have changed since the last draw command.
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL | DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT;
        m_pImmediateContext->DrawIndexed(DrawAttrs);
    }
}

void Tutorial16_BindlessResources::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    // Set cube view matrix
    float4x4 View = float4x4::RotationX(-0.6f) * float4x4::Translation(0.f, 0.f, 4.0f);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    // Compute view-projection matrix
    m_ViewProjMatrix = View * SrfPreTransform * Proj;

    // Global rotation matrix
    m_RotationMatrix = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f) * float4x4::RotationX(-static_cast<float>(CurrTime) * 0.25f);
}

} // namespace Diligent
