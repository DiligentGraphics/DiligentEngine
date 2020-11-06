#include "Plane.h"
#include "TexturedCube.hpp"

#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "TexturedCube.hpp"

using namespace Diligent;

Plane::Plane(const SampleInitInfo& InitInfo)
{
    Initialize(InitInfo);
}

void Plane::Initialize(const SampleInitInfo& InitInfo)
{
    Actor::Initialize(InitInfo);

    std::vector<StateTransitionDesc> Barriers;
    // Create dynamic uniform buffer that will store our transformation matrices
    // Dynamic buffers can be frequently updated by the CPU
    CreateUniformBuffer(m_pDevice, sizeof(float4x4) * 2 + sizeof(float4), "VS constants CB", &m_VSConstants);
    Barriers.emplace_back(m_VSConstants, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true);

    CreatePSO();
    // In this tutorial we need vertices with normals
    CreateVertexBuffer();
    // Load index buffer
    m_IndexBuffer = TexturedCube::CreateIndexBuffer(m_pDevice);
    // Explicitly transition vertex and index buffers to required states
    Barriers.emplace_back(m_VertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, true);
    Barriers.emplace_back(m_IndexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, true);

    m_pImmediateContext->TransitionResourceStates(static_cast<Uint32>(Barriers.size()), Barriers.data());
}

void Plane::CreatePSO()
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Plane PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial renders to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    // No cull
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
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
    // Create plane vertex shader
    RefCntAutoPtr<IShader> pPlaneVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Plane VS";
        ShaderCI.FilePath        = "plane.vsh";
        m_pDevice->CreateShader(ShaderCI, &pPlaneVS);
    }

    // Create plane pixel shader
    RefCntAutoPtr<IShader> pPlanePS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Plane PS";
        ShaderCI.FilePath        = "plane.psh";
        m_pDevice->CreateShader(ShaderCI, &pPlanePS);
    }

    PSOCreateInfo.pVS = pPlaneVS;
    PSOCreateInfo.pPS = pPlanePS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] =
    {
        {SHADER_TYPE_PIXEL, "g_ShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // Define immutable comparison sampler for g_ShadowMap. Immutable samplers should be used whenever possible
    SamplerDesc ComparsionSampler;
    ComparsionSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
    ComparsionSampler.MinFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ComparsionSampler.MagFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ComparsionSampler.MipFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    // clang-format off
    ImmutableSamplerDesc ImtblSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_ShadowMap", ComparsionSampler}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
    // change and are bound directly through the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
}

void Plane::CreateVertexBuffer()
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

void Plane::RenderActor(const float4x4& CameraViewProj, bool IsShadowPass)
{
    {
        struct Constants
        {
            float4x4 CameraViewProj;
            float4x4 WorldToShadowMapUVDepth;
            float4   LightDirection;
        };
        MapHelper<Constants> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        CBConstants->CameraViewProj          = CameraViewProj.Transpose();
        CBConstants->WorldToShadowMapUVDepth = m_WorldToShadowMapUVDepthMatr.Transpose();
        CBConstants->LightDirection          = m_LightDirection;
    }

    m_pImmediateContext->SetPipelineState(m_pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    // Note that Vulkan requires shadow map to be transitioned to DEPTH_READ state, not SHADER_RESOURCE
    m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs(4, DRAW_FLAG_VERIFY_ALL);
    m_pImmediateContext->Draw(DrawAttrs);
}