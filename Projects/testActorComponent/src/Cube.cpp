#include "Cube.h"
#include "TexturedCube.hpp"

#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "TexturedCube.hpp"

using namespace Diligent;

Cube::Cube(const SampleInitInfo& InitInfo)
{
    Initialize(InitInfo);
}

void Cube::Initialize(const SampleInitInfo& InitInfo)
{
    Actor::Initialize(InitInfo);

    std::vector<StateTransitionDesc> Barriers;
    // Create dynamic uniform buffer that will store our transformation matrices
    // Dynamic buffers can be frequently updated by the CPU
    CreateUniformBuffer(m_pDevice, sizeof(float4x4) * 2 + sizeof(float4), "VS constants CB", &m_VSConstants);
    Barriers.emplace_back(m_VSConstants, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true);

    CreatePSO();

    // Load cube

    // In this tutorial we need vertices with normals
    CreateVertexBuffer();
    // Load index buffer
    m_IndexBuffer = TexturedCube::CreateIndexBuffer(m_pDevice);
    // Explicitly transition vertex and index buffers to required states
    Barriers.emplace_back(m_VertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    Barriers.emplace_back(m_IndexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, true);
    // Load texture
    auto CubeTexture = TexturedCube::LoadTexture(m_pDevice, "DGLogo.png");
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(CubeTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    // Transition the texture to shader resource state
    Barriers.emplace_back(CubeTexture, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true);

    m_pImmediateContext->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());
}

void Cube::CreatePSO()
{
    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

    // clang-format off
    // Define vertex shader input layout
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - texture coordinates
        LayoutElement{1, 0, 2, VT_FLOAT32, False},
        // Attribute 2 - normal
        LayoutElement{2, 0, 3, VT_FLOAT32, False},
    };
    // clang-format on

    m_pPSO = TexturedCube::CreatePipelineState(m_pDevice,
                                               m_pSwapChain->GetDesc().ColorBufferFormat,
                                               m_pSwapChain->GetDesc().DepthBufferFormat,
                                               pShaderSourceFactory,
                                               "cube.vsh",
                                               "cube.psh",
                                               LayoutElems,
                                               _countof(LayoutElems));

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
    // change and are bound directly through the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    m_pPSO->CreateShaderResourceBinding(&m_SRB, true);


    // Create shadow pass PSO
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.Name = "Cube shadow PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // Shadow pass doesn't use any render target outputs
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 0;
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_UNKNOWN;
    // The DSV format is the shadow map format
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_ShadowMapFormat;
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;
    // Create shadow vertex shader
    RefCntAutoPtr<IShader> pShadowVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube Shadow VS";
        ShaderCI.FilePath        = "cube_shadow.vsh";
        m_pDevice->CreateShader(ShaderCI, &pShadowVS);
    }
    PSOCreateInfo.pVS = pShadowVS;

    // We don't use pixel shader as we are only interested in populating the depth buffer
    PSOCreateInfo.pPS = nullptr;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    if (m_pDevice->GetDeviceCaps().Features.DepthClamp)
    {
        // Disable depth clipping to render objects that are closer than near
        // clipping plane. This is not required for this tutorial, but real applications
        // will most likely want to do this.
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.DepthClipEnable = False;
    }

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pShadowPSO);
    m_pShadowPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
    m_pShadowPSO->CreateShaderResourceBinding(&m_ShadowSRB, true);
}

void Cube::CreateVertexBuffer()
{
    // Layout of this structure matches the one we defined in pipeline state
    struct Vertex
    {
        float3 pos;
        float2 uv;
        float3 normal;
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
    Vertex CubeVerts[] =
    {
        {float3(-1,-1,-1), float2(0,1), float3(0, 0, -1)},
        {float3(-1,+1,-1), float2(0,0), float3(0, 0, -1)},
        {float3(+1,+1,-1), float2(1,0), float3(0, 0, -1)},
        {float3(+1,-1,-1), float2(1,1), float3(0, 0, -1)},
        
        {float3(-1,-1,-1), float2(0,1), float3(0, -1, 0)},
        {float3(-1,-1,+1), float2(0,0), float3(0, -1, 0)},
        {float3(+1,-1,+1), float2(1,0), float3(0, -1, 0)},
        {float3(+1,-1,-1), float2(1,1), float3(0, -1, 0)},
        
        {float3(+1,-1,-1), float2(0,1), float3(+1, 0, 0)},
        {float3(+1,-1,+1), float2(1,1), float3(+1, 0, 0)},
        {float3(+1,+1,+1), float2(1,0), float3(+1, 0, 0)},
        {float3(+1,+1,-1), float2(0,0), float3(+1, 0, 0)},
        
        {float3(+1,+1,-1), float2(0,1), float3(0, +1, 0)},
        {float3(+1,+1,+1), float2(0,0), float3(0, +1, 0)},
        {float3(-1,+1,+1), float2(1,0), float3(0, +1, 0)},
        {float3(-1,+1,-1), float2(1,1), float3(0, +1, 0)},
        
        {float3(-1,+1,-1), float2(1,0), float3(-1, 0, 0)},
        {float3(-1,+1,+1), float2(0,0), float3(-1, 0, 0)},
        {float3(-1,-1,+1), float2(0,1), float3(-1, 0, 0)},
        {float3(-1,-1,-1), float2(1,1), float3(-1, 0, 0)},
        
        {float3(-1,-1,+1), float2(1,1), float3(0, 0, +1)},
        {float3(+1,-1,+1), float2(0,1), float3(0, 0, +1)},
        {float3(+1,+1,+1), float2(0,0), float3(0, 0, +1)},
        {float3(-1,+1,+1), float2(1,0), float3(0, 0, +1)}
    };
    // clang-format on

    BufferDesc VertBuffDesc;
    VertBuffDesc.Name          = "Cube vertex buffer";
    VertBuffDesc.Usage         = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
    VertBuffDesc.uiSizeInBytes = sizeof(CubeVerts);
    BufferData VBData;
    VBData.pData    = CubeVerts;
    VBData.DataSize = sizeof(CubeVerts);
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_VertexBuffer);
}

void Cube::RenderActor(const float4x4& CameraViewProj, bool IsShadowPass)
{
    // Update constant buffer
    {
        struct Constants
        {
            float4x4 WorldViewProj;
            float4x4 NormalTranform;
            float4   LightDirection;
        };
        // Map the buffer and write current world-view-projection matrix
        MapHelper<Constants> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        CBConstants->WorldViewProj = (m_WorldMatrix * CameraViewProj).Transpose();
        auto NormalMatrix          = m_WorldMatrix.RemoveTranslation().Inverse();
        // We need to do inverse-transpose, but we also need to transpose the matrix
        // before writing it to the buffer
        CBConstants->NormalTranform = NormalMatrix;
        CBConstants->LightDirection = m_LightDirection;
    }

    // Bind vertex buffer
    Uint32   offset   = 0;
    IBuffer* pBuffs[] = {m_VertexBuffer};
    // Note that since resouces have been explicitly transitioned to required states, we use RESOURCE_STATE_TRANSITION_MODE_VERIFY flag
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    // Set pipeline state and commit resources
    if (IsShadowPass)
    {
        m_pImmediateContext->SetPipelineState(m_pShadowPSO);
        m_pImmediateContext->CommitShaderResources(m_ShadowSRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }
    else
    {
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }

    DrawIndexedAttribs DrawAttrs(36, VT_UINT32, DRAW_FLAG_VERIFY_ALL);
    m_pImmediateContext->DrawIndexed(DrawAttrs);
}

void Cube::UpdateActor(double CurrTime, double ElapsedTime)
{
    // Animate the cube
    m_WorldMatrix = float4x4::Translation(0.0f, sin(static_cast<float>(CurrTime) * 0.5f), 0.0f) * float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f);
}