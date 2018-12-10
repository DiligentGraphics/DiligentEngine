
#include "SamplePlugin.h"
#include "GraphicsUtilities.h"
#include "MapHelper.h"

using namespace Diligent;

static const Char* VSSource = R"(
cbuffer Constants
{
    float4x4 g_WorldViewProj;
};

struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float4 Color : COLOR0; 
};

PSInput main(float3 pos : ATTRIB0, float4 color : ATTRIB1) 
{
    PSInput ps; 
    ps.Pos = mul( float4(pos,1.0), g_WorldViewProj);
    ps.Color = color;
    return ps;
}
)";

static const Char* PSSource = R"(
struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float4 Color : COLOR0; 
};

float4 main(PSInput ps_in) : SV_TARGET
{
    return ps_in.Color; 
}
)";

SamplePlugin::SamplePlugin(Diligent::IRenderDevice *pDevice, bool UseReverseZ, TEXTURE_FORMAT RTVFormat, TEXTURE_FORMAT DSVFormat)
{
    auto deviceType = pDevice->GetDeviceCaps().DevType;
    {
        PipelineStateDesc PSODesc;
        PSODesc.IsComputePipeline = false;
        PSODesc.Name = "Render sample cube PSO";
        PSODesc.GraphicsPipeline.NumRenderTargets = 1;
        PSODesc.GraphicsPipeline.RTVFormats[0] = RTVFormat;
        PSODesc.GraphicsPipeline.DSVFormat = DSVFormat;
        PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
        PSODesc.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = deviceType == DeviceType::D3D11 || deviceType == DeviceType::D3D12 ? true : false;
        PSODesc.GraphicsPipeline.DepthStencilDesc.DepthFunc = UseReverseZ ? COMPARISON_FUNC_GREATER_EQUAL : COMPARISON_FUNC_LESS_EQUAL;

        PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = True;
        PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend = BLEND_FACTOR_SRC_ALPHA;
        PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend = BLEND_FACTOR_INV_SRC_ALPHA;
        PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = BLEND_FACTOR_ZERO;
        PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = BLEND_FACTOR_ONE;

        ShaderCreationAttribs CreationAttribs;
        CreationAttribs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        CreationAttribs.Desc.DefaultVariableType = SHADER_VARIABLE_TYPE_STATIC;
        CreationAttribs.UseCombinedTextureSamplers = true;

        CreateUniformBuffer(pDevice, sizeof(float4x4), "SamplePlugin: VS constants CB", &m_VSConstants);

        RefCntAutoPtr<IShader> pVS;
        {
            CreationAttribs.Desc.ShaderType = SHADER_TYPE_VERTEX;
            CreationAttribs.EntryPoint = "main";
            CreationAttribs.Desc.Name = "Sample cube VS";
            CreationAttribs.Source = VSSource;
            pDevice->CreateShader(CreationAttribs, &pVS);
            pVS->GetShaderVariable("Constants")->Set(m_VSConstants);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            CreationAttribs.Desc.ShaderType = SHADER_TYPE_PIXEL;
            CreationAttribs.EntryPoint = "main";
            CreationAttribs.Desc.Name = "Sample cube PS";
            CreationAttribs.Source = PSSource;
            pDevice->CreateShader(CreationAttribs, &pPS);
        }

        LayoutElement LayoutElems[] =
        {
            LayoutElement(0, 0, 3, VT_FLOAT32, False),
            LayoutElement(1, 0, 4, VT_FLOAT32, False)
        };

        PSODesc.GraphicsPipeline.pVS = pVS;
        PSODesc.GraphicsPipeline.pPS = pPS;
        PSODesc.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);
        pDevice->CreatePipelineState(PSODesc, &m_PSO);
        m_PSO->CreateShaderResourceBinding(&m_SRB, true);
    }

    {
        struct Vertex
        {
            float3 pos;
            float4 color;
        };
        Vertex CubeVerts[8] =
        {
            {float3(-1,-1,-1), float4(1,0,0,0.5)},
            {float3(-1,+1,-1), float4(0,1,0,0.5)},
            {float3(+1,+1,-1), float4(0,0,1,0.5)},
            {float3(+1,-1,-1), float4(1,1,1,0.5)},

            {float3(-1,-1,+1), float4(1,1,0,0.5)},
            {float3(-1,+1,+1), float4(0,1,1,0.5)},
            {float3(+1,+1,+1), float4(1,0,1,0.5)},
            {float3(+1,-1,+1), float4(0.2f,0.2f,0.2f,0.5)},
        };
        BufferDesc VertBuffDesc;
        VertBuffDesc.Name = "SamplePlugin: cube vertex buffer";
        VertBuffDesc.Usage = USAGE_DEFAULT;
        VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        VertBuffDesc.uiSizeInBytes = sizeof(CubeVerts);
        BufferData VBData;
        VBData.pData = CubeVerts;
        VBData.DataSize = sizeof(CubeVerts);
        pDevice->CreateBuffer(VertBuffDesc, VBData, &m_CubeVertexBuffer);
    }

    {
        Uint32 Indices[] =
        {
            2,0,1, 2,3,0,
            4,6,5, 4,7,6,
            0,7,4, 0,3,7,
            1,0,4, 1,4,5,
            1,5,2, 5,6,2,
            3,6,7, 3,2,6
        };
        BufferDesc IndBuffDesc;
        IndBuffDesc.Name = "SamplePlugin: cube index buffer";
        IndBuffDesc.Usage = USAGE_DEFAULT;
        IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
        IndBuffDesc.uiSizeInBytes = sizeof(Indices);
        BufferData IBData;
        IBData.pData = Indices;
        IBData.DataSize = sizeof(Indices);
        pDevice->CreateBuffer(IndBuffDesc, IBData, &m_CubeIndexBuffer);
    }
}

void SamplePlugin::Render(Diligent::IDeviceContext *pContext, const float4x4 &ViewProjMatrix)
{
    {
        MapHelper<float4x4> CBConstants(pContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = transposeMatrix(ViewProjMatrix);
    }

    Uint32 offset = 0;
    IBuffer *pBuffs[] = {m_CubeVertexBuffer};
    pContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(m_PSO);
    pContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs(36, VT_UINT32, DRAW_FLAG_VERIFY_STATES);
    pContext->Draw(DrawAttrs);
}
