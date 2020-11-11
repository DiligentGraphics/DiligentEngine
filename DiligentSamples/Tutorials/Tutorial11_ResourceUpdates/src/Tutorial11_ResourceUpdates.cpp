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

#include <math.h>
#include <cmath>

#include "Tutorial11_ResourceUpdates.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial11_ResourceUpdates();
}

namespace
{

// Layout of this structure matches the one we defined in the pipeline state
struct Vertex
{
    float3 pos;
    float2 uv;
};

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

// clang-format off
const Vertex CubeVerts[] =
{
    {float3(-1,-1,-1), float2(0,1)},
    {float3(-1,+1,-1), float2(0,0)},
    {float3(+1,+1,-1), float2(1,0)},
    {float3(+1,-1,-1), float2(1,1)},

    {float3(-1,-1,-1), float2(0,1)},
    {float3(-1,-1,+1), float2(0,0)},
    {float3(+1,-1,+1), float2(1,0)},
    {float3(+1,-1,-1), float2(1,1)},

    {float3(+1,-1,-1), float2(0,1)},
    {float3(+1,-1,+1), float2(1,1)},
    {float3(+1,+1,+1), float2(1,0)},
    {float3(+1,+1,-1), float2(0,0)},

    {float3(+1,+1,-1), float2(0,1)},
    {float3(+1,+1,+1), float2(0,0)},
    {float3(-1,+1,+1), float2(1,0)},
    {float3(-1,+1,-1), float2(1,1)},

    {float3(-1,+1,-1), float2(1,0)},
    {float3(-1,+1,+1), float2(0,0)},
    {float3(-1,-1,+1), float2(0,1)},
    {float3(-1,-1,-1), float2(1,1)},

    {float3(-1,-1,+1), float2(1,1)},
    {float3(+1,-1,+1), float2(0,1)},
    {float3(+1,+1,+1), float2(0,0)},
    {float3(-1,+1,+1), float2(1,0)}
};
// clang-format on

} // namespace

void Tutorial11_ResourceUpdates::CreatePipelineStates()
{
    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Cube PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

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
        CreateUniformBuffer(m_pDevice, sizeof(float4x4), "VS constants CB", &m_VSConstants);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "cube.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    // clang-format off
    // Define vertex shader input layout
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - texture coordinates
        LayoutElement{1, 0, 2, VT_FLOAT32, False}
    };
    // clang-format on

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

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
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
    // change and are bound directly to the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO_NoCull);
    m_pPSO_NoCull->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
}

void Tutorial11_ResourceUpdates::CreateVertexBuffers()
{
    for (Uint32 i = 0; i < _countof(m_CubeVertexBuffer); ++i)
    {
        auto& VertexBuffer = m_CubeVertexBuffer[i];

        // Create vertex buffer that stores cube vertices
        BufferDesc VertBuffDesc;
        VertBuffDesc.Name = "Cube vertex buffer";
        if (i == 0)
            VertBuffDesc.Usage = USAGE_IMMUTABLE;
        else if (i == 1)
            VertBuffDesc.Usage = USAGE_DEFAULT;
        else
        {
            VertBuffDesc.Usage          = USAGE_DYNAMIC;
            VertBuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        }

        VertBuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
        VertBuffDesc.uiSizeInBytes = sizeof(CubeVerts);
        BufferData VBData;
        VBData.pData    = CubeVerts;
        VBData.DataSize = sizeof(CubeVerts);
        m_pDevice->CreateBuffer(VertBuffDesc, i < 2 ? &VBData : nullptr, &VertexBuffer);
    }
}

void Tutorial11_ResourceUpdates::CreateIndexBuffer()
{
    // clang-format off
    Uint32 Indices[] =
    {
        2,0,1,     2,3,0,
        4,6,5,     4,7,6,
        8,10,9,    8,11,10,
        12,14,13,  12,15,14,
        16,18,17,  16,19,18,
        20,21,22,  20,22,23
    };
    // clang-format on

    // Create index buffer
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name          = "Cube index buffer";
    IndBuffDesc.Usage         = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags     = BIND_INDEX_BUFFER;
    IndBuffDesc.uiSizeInBytes = sizeof(Indices);
    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);
}

void Tutorial11_ResourceUpdates::LoadTextures()
{
    for (size_t i = 0; i < m_Textures.size(); ++i)
    {
        // Load texture
        TextureLoadInfo   loadInfo;
        std::stringstream FileNameSS;
        FileNameSS << "DGLogo" << i << ".png";
        auto FileName   = FileNameSS.str();
        loadInfo.IsSRGB = true;
        loadInfo.Usage  = USAGE_IMMUTABLE;
        if (i == 2)
        {
            loadInfo.Usage = USAGE_DEFAULT;
            // Disable mipmapping for simplicity as we will only update mip level 0
            loadInfo.MipLevels = 1;
        }
        else if (i == 3)
        {
            // Disable mipmapping
            loadInfo.MipLevels      = 1;
            loadInfo.Usage          = USAGE_DYNAMIC;
            loadInfo.CPUAccessFlags = CPU_ACCESS_WRITE;
        }

        auto& Tex = m_Textures[i];
        CreateTextureFromFile(FileName.c_str(), loadInfo, m_pDevice, &Tex);
        // Get shader resource view from the texture
        auto TextureSRV = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        // Since we are using mutable variable, we must create shader resource binding object
        // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
        m_pPSO->CreateShaderResourceBinding(&(m_SRBs[i]), true);
        // Set texture SRV in the SRB
        m_SRBs[i]->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(TextureSRV);
    }
}


void Tutorial11_ResourceUpdates::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    CreatePipelineStates();
    CreateVertexBuffers();
    CreateIndexBuffer();
    LoadTextures();

    {
        BufferDesc VertBuffDesc;
        VertBuffDesc.Name           = "Texture update buffer";
        VertBuffDesc.Usage          = USAGE_DYNAMIC;
        VertBuffDesc.BindFlags      = BIND_VERTEX_BUFFER; // We do not really bind the buffer, but D3D11 wants at least one bind flag bit
        VertBuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        VertBuffDesc.uiSizeInBytes  = MaxUpdateRegionSize * MaxUpdateRegionSize * 4;
        m_pDevice->CreateBuffer(VertBuffDesc, nullptr, &m_TextureUpdateBuffer);
    }
}

void Tutorial11_ResourceUpdates::DrawCube(const float4x4& WVPMatrix, Diligent::IBuffer* pVertexBuffer, Diligent::IShaderResourceBinding* pSRB)
{
    // Bind vertex buffer
    Uint32   offset   = 0;
    IBuffer* pBuffs[] = {pVertexBuffer};
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    m_pImmediateContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // Map the buffer and write current world-view-projection matrix
        MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = WVPMatrix.Transpose();
    }

    DrawIndexedAttribs DrawAttrs;     // This is an indexed draw call
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = 36;
    // Verify the state of vertex and index buffers
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->DrawIndexed(DrawAttrs);
}

// Render a frame
void Tutorial11_ResourceUpdates::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    m_pImmediateContext->SetPipelineState(m_pPSO);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    auto ViewProj = SrfPreTransform * Proj;

    auto CubeRotation = float4x4::RotationY(static_cast<float>(m_CurrTime) * 0.5f) * float4x4::RotationX(-PI_F * 0.1f) * float4x4::Translation(0, 0, 12.0f);

    DrawCube(CubeRotation * float4x4::Translation(-2.f, -2.f, 0.f) * ViewProj, m_CubeVertexBuffer[0], m_SRBs[2]);
    DrawCube(CubeRotation * float4x4::Translation(+2.f, -2.f, 0.f) * ViewProj, m_CubeVertexBuffer[0], m_SRBs[3]);

    DrawCube(CubeRotation * float4x4::Translation(-4.f, +2.f, 0.f) * ViewProj, m_CubeVertexBuffer[0], m_SRBs[0]);
    m_pImmediateContext->SetPipelineState(m_pPSO_NoCull);
    DrawCube(CubeRotation * float4x4::Translation(0.f, +2.f, 0.f) * ViewProj, m_CubeVertexBuffer[1], m_SRBs[0]);
    DrawCube(CubeRotation * float4x4::Translation(+4.f, +2.f, 0.f) * ViewProj, m_CubeVertexBuffer[2], m_SRBs[1]);
}

void Tutorial11_ResourceUpdates::WriteStripPattern(Uint8* pData, Uint32 Width, Uint32 Height, Uint32 Stride)
{
    auto x_scale = std::uniform_int_distribution<Uint32>{1, 8}(m_gen);
    auto y_scale = std::uniform_int_distribution<Uint32>{1, 8}(m_gen);
    auto c_scale = std::uniform_int_distribution<Uint32>{1, 64}(m_gen);
    for (Uint32 j = 0; j < Height; ++j)
    {
        for (Uint32 i = 0; i < Width; ++i)
        {
            for (int c = 0; c < 4; ++c)
                pData[i * 4 + c + j * Stride] = (i * x_scale + j * y_scale + c * c_scale) & 0xFF;
        }
    }
}

void Tutorial11_ResourceUpdates::WriteDiamondPattern(Uint8* pData, Uint32 Width, Uint32 Height, Uint32 Stride)
{
    auto x_scale = std::uniform_int_distribution<Uint32>{1, 8}(m_gen);
    auto y_scale = std::uniform_int_distribution<Uint32>{1, 8}(m_gen);
    auto c_scale = std::uniform_int_distribution<Uint32>{1, 64}(m_gen);
    for (Uint32 j = 0; j < Height; ++j)
    {
        for (Uint32 i = 0; i < Width; ++i)
        {
            for (int c = 0; c < 4; ++c)
                pData[i * 4 + c + j * Stride] = (::abs(static_cast<int>(i) - static_cast<int>(Width / 2)) * x_scale + ::abs(static_cast<int>(j) - static_cast<int>(Height / 2)) * y_scale + c * c_scale) & 0xFF;
        }
    }
}

void Tutorial11_ResourceUpdates::UpdateTexture(Uint32 TexIndex)
{
    auto& Texture = *m_Textures[TexIndex];

    static constexpr const Uint32 NumUpdates = 3;
    for (Uint32 update = 0; update < NumUpdates; ++update)
    {
        const auto& TexDesc = Texture.GetDesc();

        Uint32 Width  = std::uniform_int_distribution<Uint32>{2, MaxUpdateRegionSize}(m_gen);
        Uint32 Height = std::uniform_int_distribution<Uint32>{2, MaxUpdateRegionSize}(m_gen);

        std::vector<Uint8> Data(Width * Height * 4);
        WriteStripPattern(Data.data(), Width, Height, Width * 4);

        Box UpdateBox;
        UpdateBox.MinX = std::uniform_int_distribution<Uint32>{0, TexDesc.Width - Width}(m_gen);
        UpdateBox.MinY = std::uniform_int_distribution<Uint32>{0, TexDesc.Height - Height}(m_gen);
        UpdateBox.MaxX = UpdateBox.MinX + Width;
        UpdateBox.MaxY = UpdateBox.MinY + Height;

        TextureSubResData SubresData;
        SubresData.Stride = Width * 4;
        SubresData.pData  = Data.data();
        Uint32 MipLevel   = 0;
        Uint32 ArraySlice = 0;
        m_pImmediateContext->UpdateTexture(&Texture, MipLevel, ArraySlice, UpdateBox, SubresData, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
}

void Tutorial11_ResourceUpdates::MapTexture(Uint32 TexIndex, bool MapEntireTexture)
{
    auto&       Texture = *m_Textures[TexIndex];
    const auto& TexDesc = m_Textures[2]->GetDesc();

    MappedTextureSubresource MappedSubres;
    Box                      MapRegion;
    if (MapEntireTexture)
    {
        MapRegion.MaxX = TexDesc.Width;
        MapRegion.MaxY = TexDesc.Height;
    }
    else
    {
        Uint32 Width   = std::uniform_int_distribution<Uint32>{2, MaxMapRegionSize}(m_gen);
        Uint32 Height  = std::uniform_int_distribution<Uint32>{2, MaxMapRegionSize}(m_gen);
        MapRegion.MinX = std::uniform_int_distribution<Uint32>{0, TexDesc.Width - Width}(m_gen);
        MapRegion.MinY = std::uniform_int_distribution<Uint32>{0, TexDesc.Height - Height}(m_gen);
        MapRegion.MaxX = MapRegion.MinX + Width;
        MapRegion.MaxY = MapRegion.MinY + Height;
    }
    Uint32 MipLevel   = 0;
    Uint32 ArraySlice = 0;
    m_pImmediateContext->MapTextureSubresource(&Texture, MipLevel, ArraySlice, MAP_WRITE, MAP_FLAG_DISCARD, MapEntireTexture ? nullptr : &MapRegion, MappedSubres);
    WriteDiamondPattern((Uint8*)MappedSubres.pData, MapRegion.MaxX - MapRegion.MinX, MapRegion.MaxY - MapRegion.MinY, MappedSubres.Stride);
    m_pImmediateContext->UnmapTextureSubresource(&Texture, 0, 0);
}

void Tutorial11_ResourceUpdates::UpdateBuffer(Diligent::Uint32 BufferIndex)
{
    Uint32 NumVertsToUpdate  = std::uniform_int_distribution<Uint32>{2, 8}(m_gen);
    Uint32 FirstVertToUpdate = std::uniform_int_distribution<Uint32>{0, static_cast<Uint32>(_countof(CubeVerts)) - NumVertsToUpdate}(m_gen);
    Vertex Vertices[_countof(CubeVerts)];
    for (Uint32 v = 0; v < NumVertsToUpdate; ++v)
    {
        auto        SrcInd  = FirstVertToUpdate + v;
        const auto& SrcVert = CubeVerts[SrcInd];
        Vertices[v].uv      = SrcVert.uv;
        Vertices[v].pos     = SrcVert.pos * static_cast<float>(1 + 0.2 * sin(m_CurrTime * (1.0 + SrcInd * 0.2)));
    }
    m_pImmediateContext->UpdateBuffer(
        m_CubeVertexBuffer[BufferIndex],    // Device context to use for the operation
        FirstVertToUpdate * sizeof(Vertex), // Start offset in bytes
        NumVertsToUpdate * sizeof(Vertex),  // Data size in bytes
        Vertices,                           // Data pointer
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void Tutorial11_ResourceUpdates::MapDynamicBuffer(Diligent::Uint32 BufferIndex)
{
    // Map the buffer and write current world-view-projection matrix
    MapHelper<Vertex> Vertices(m_pImmediateContext, m_CubeVertexBuffer[BufferIndex], MAP_WRITE, MAP_FLAG_DISCARD);
    for (Uint32 v = 0; v < _countof(CubeVerts); ++v)
    {
        const auto& SrcVert = CubeVerts[v];
        Vertices[v].uv      = SrcVert.uv;
        Vertices[v].pos     = SrcVert.pos * static_cast<float>(1 + 0.2 * sin(m_CurrTime * (1.0 + v * 0.2)));
    }
}


void Tutorial11_ResourceUpdates::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    m_CurrTime = CurrTime;

    static constexpr const double UpdateBufferPeriod = 0.1;
    if (CurrTime - m_LastBufferUpdateTime > UpdateBufferPeriod)
    {
        m_LastBufferUpdateTime = CurrTime;
        UpdateBuffer(1);
    }

    MapDynamicBuffer(2);

    static constexpr const double UpdateTexturePeriod = 0.5;
    if (CurrTime - m_LastTextureUpdateTime > UpdateTexturePeriod)
    {
        m_LastTextureUpdateTime = CurrTime;
        UpdateTexture(2);
    }

    static constexpr const double MapTexturePeriod = 0.05;
    const auto&                   deviceType       = m_pDevice->GetDeviceCaps().DevType;
    if (CurrTime - m_LastMapTime > MapTexturePeriod * (deviceType == RENDER_DEVICE_TYPE_D3D11 ? 10.f : 1.f))
    {
        m_LastMapTime = CurrTime;
        if (deviceType == RENDER_DEVICE_TYPE_D3D11 || deviceType == RENDER_DEVICE_TYPE_D3D12 || deviceType == RENDER_DEVICE_TYPE_VULKAN)
        {
            MapTexture(3, deviceType == RENDER_DEVICE_TYPE_D3D11);
        }
    }
}

} // namespace Diligent
