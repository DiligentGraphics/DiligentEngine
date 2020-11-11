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

#include "TestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"
#include "BasicMath.hpp"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
void RenderDrawCommandReferenceD3D11(ISwapChain* pSwapChain, const float* pClearColor = nullptr);
#endif

#if D3D12_SUPPORTED
void RenderDrawCommandReferenceD3D12(ISwapChain* pSwapChain, const float* pClearColor = nullptr);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
void RenderDrawCommandReferenceGL(ISwapChain* pSwapChain, const float* pClearColor = nullptr);
#endif

#if VULKAN_SUPPORTED
void RenderDrawCommandReferenceVk(ISwapChain* pSwapChain, const float* pClearColor = nullptr);
#endif

#if METAL_SUPPORTED
void RenderDrawCommandReferenceMtl(ISwapChain* pSwapChain, const float* pClearColor = nullptr);
#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

#include "InlineShaders/DrawCommandTestHLSL.h"

namespace
{

namespace HLSL
{

// clang-format off
const std::string DrawTest_VS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

struct VSInput
{
    float4 Pos   : ATTRIB0;
    float3 Color : ATTRIB1; 
};

void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    PSIn.Pos   = VSIn.Pos;
    PSIn.Color = VSIn.Color;
}
)"
};


const std::string DrawTest_VSInstanced{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

struct VSInput
{
    float4 Pos       : ATTRIB0;
    float3 Color     : ATTRIB1; 
    float4 ScaleBias : ATTRIB2; 
};

void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    PSIn.Pos.xy = VSIn.Pos.xy * VSIn.ScaleBias.xy + VSIn.ScaleBias.zw;
    PSIn.Pos.zw = VSIn.Pos.zw;
    PSIn.Color  = VSIn.Color;
}
)"
};
// clang-format on

} // namespace HLSL

struct Vertex
{
    float4 Pos;
    float3 Color;
};

// clang-format off
float4 Pos[] = 
{
    float4(-1.0f,  -0.5f,  0.f,  1.f),
    float4(-0.5f,  +0.5f,  0.f,  1.f),
    float4( 0.0f,  -0.5f,  0.f,  1.f),

    float4(+0.0f,  -0.5f,  0.f,  1.f),
    float4(+0.5f,  +0.5f,  0.f,  1.f),
    float4(+1.0f,  -0.5f,  0.f,  1.f)
};

float3 Color[] =
{
    float3(1.f,  0.f,  0.f),
    float3(0.f,  1.f,  0.f),
    float3(0.f,  0.f,  1.f),
};

Vertex Vert[] = 
{
    {Pos[0], Color[0]},
    {Pos[1], Color[1]},
    {Pos[2], Color[2]},

    {Pos[3], Color[0]},
    {Pos[4], Color[1]},
    {Pos[5], Color[2]}
};

Vertex VertInst[] = 
{
    {float4(-1.0,  0.0,  0.0,  1.0), Color[0]},
    {float4( 0.0, +2.0,  0.0,  1.0), Color[1]},
    {float4(+1.0,  0.0,  0.0,  1.0), Color[2]}
};
// clang-format on

class DrawCommandTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv       = TestingEnvironment::GetInstance();
        auto* pDevice    = pEnv->GetDevice();
        auto* pSwapChain = pEnv->GetSwapChain();
        auto* pContext   = pEnv->GetDeviceContext();

        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
        if (pTestingSwapChain)
        {
            pContext->Flush();
            pContext->InvalidateState();

            auto deviceType = pDevice->GetDeviceCaps().DevType;
            switch (deviceType)
            {
#if D3D11_SUPPORTED
                case RENDER_DEVICE_TYPE_D3D11:
                    RenderDrawCommandReferenceD3D11(pSwapChain);
                    break;
#endif

#if D3D12_SUPPORTED
                case RENDER_DEVICE_TYPE_D3D12:
                    RenderDrawCommandReferenceD3D12(pSwapChain);
                    break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
                case RENDER_DEVICE_TYPE_GL:
                case RENDER_DEVICE_TYPE_GLES:
                    RenderDrawCommandReferenceGL(pSwapChain);
                    break;

#endif

#if VULKAN_SUPPORTED
                case RENDER_DEVICE_TYPE_VULKAN:
                    RenderDrawCommandReferenceVk(pSwapChain);
                    break;
#endif

#if METAL_SUPPORTED
                case RENDER_DEVICE_TYPE_METAL:
                    RenderDrawCommandReferenceMtl(pSwapChain);
                    break;
#endif

                default:
                    LOG_ERROR_AND_THROW("Unsupported device type");
            }

            pTestingSwapChain->TakeSnapshot();
        }
        TestingEnvironment::ScopedReleaseResources EnvironmentAutoReset;

        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Draw command test - procedural triangles";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pProceduralVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test procedural vertex shader";
            ShaderCI.Source          = HLSL::DrawTest_ProceduralTriangleVS.c_str();
            pDevice->CreateShader(ShaderCI, &pProceduralVS);
            ASSERT_NE(pProceduralVS, nullptr);
        }

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test vertex shader";
            ShaderCI.Source          = HLSL::DrawTest_VS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pInstancedVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test instanced vertex shader";
            ShaderCI.Source          = HLSL::DrawTest_VSInstanced.c_str();
            pDevice->CreateShader(ShaderCI, &pInstancedVS);
            ASSERT_NE(pInstancedVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Draw command test pixel shader";
            ShaderCI.Source          = HLSL::DrawTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pPS, nullptr);
        }

        PSODesc.Name = "Draw command test";

        PSOCreateInfo.pVS = pProceduralVS;
        PSOCreateInfo.pPS = pPS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDrawProceduralPSO);
        ASSERT_NE(sm_pDrawProceduralPSO, nullptr);

        InputLayoutDesc LayoutDesc;
        // clang-format off
        LayoutElement Elems[] =
        {
            LayoutElement{ 0, 0, 4, VT_FLOAT32},
            LayoutElement{ 1, 0, 3, VT_FLOAT32}
        };
        // clang-format on
        GraphicsPipeline.InputLayout.LayoutElements = Elems;
        GraphicsPipeline.InputLayout.NumElements    = _countof(Elems);

        PSOCreateInfo.pVS                  = pVS;
        PSOCreateInfo.pPS                  = pPS;
        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDrawPSO);


        PSODesc.Name = "Draw command test - 2x stride";

        Elems[0].Stride = sizeof(Vertex) * 2;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDraw_2xStride_PSO);


        PSODesc.Name = "Instanced draw command test";
        // clang-format off
        LayoutElement InstancedElems[] =
        {
            LayoutElement{ 0, 0, 4, VT_FLOAT32},
            LayoutElement{ 1, 0, 3, VT_FLOAT32},
            LayoutElement{ 2, 1, 4, VT_FLOAT32, false, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}
        };
        // clang-format on
        GraphicsPipeline.InputLayout.LayoutElements = InstancedElems;
        GraphicsPipeline.InputLayout.NumElements    = _countof(InstancedElems);

        PSOCreateInfo.pVS = pInstancedVS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pDrawInstancedPSO);
    }

    static void TearDownTestSuite()
    {
        sm_pDrawProceduralPSO.Release();
        sm_pDrawPSO.Release();
        sm_pDraw_2xStride_PSO.Release();
        sm_pDrawInstancedPSO.Release();

        auto* pEnv = TestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static void SetRenderTargets(IPipelineState* pPSO)
    {
        auto* pEnv       = TestingEnvironment::GetInstance();
        auto* pContext   = pEnv->GetDeviceContext();
        auto* pSwapChain = pEnv->GetSwapChain();

        ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const float ClearColor[] = {0.f, 0.f, 0.f, 0.0f};
        pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(pPSO);
    }

    static void Present()
    {
        auto* pEnv       = TestingEnvironment::GetInstance();
        auto* pSwapChain = pEnv->GetSwapChain();
        auto* pContext   = pEnv->GetDeviceContext();

        pSwapChain->Present();

        pContext->Flush();
        pContext->InvalidateState();
    }

    RefCntAutoPtr<IBuffer> CreateVertexBuffer(const void* VertexData, Uint32 DataSize)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name          = "Test vertex buffer";
        BuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
        BuffDesc.uiSizeInBytes = DataSize;

        BufferData InitialData;
        InitialData.pData    = VertexData;
        InitialData.DataSize = DataSize;

        auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, &InitialData, &pBuffer);
        VERIFY_EXPR(pBuffer);
        return pBuffer;
    }

    RefCntAutoPtr<IBuffer> CreateIndexBuffer(const Uint32* Indices, Uint32 NumIndices)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name          = "Test index buffer";
        BuffDesc.BindFlags     = BIND_INDEX_BUFFER;
        BuffDesc.uiSizeInBytes = sizeof(Uint32) * NumIndices;

        BufferData InitialData;
        InitialData.pData    = Indices;
        InitialData.DataSize = BuffDesc.uiSizeInBytes;

        auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, &InitialData, &pBuffer);
        VERIFY_EXPR(pBuffer);
        return pBuffer;
    }

    RefCntAutoPtr<IBuffer> CreateIndirectDrawArgsBuffer(const Uint32* Data, Uint32 DataSize)
    {
        BufferDesc BuffDesc;
        BuffDesc.Name          = "Test index buffer";
        BuffDesc.BindFlags     = BIND_INDIRECT_DRAW_ARGS;
        BuffDesc.uiSizeInBytes = DataSize;

        BufferData InitialData;
        InitialData.pData    = Data;
        InitialData.DataSize = BuffDesc.uiSizeInBytes;

        auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(BuffDesc, &InitialData, &pBuffer);
        VERIFY_EXPR(pBuffer);
        return pBuffer;
    }

    static RefCntAutoPtr<IPipelineState> sm_pDrawProceduralPSO;
    static RefCntAutoPtr<IPipelineState> sm_pDrawPSO;
    static RefCntAutoPtr<IPipelineState> sm_pDraw_2xStride_PSO;
    static RefCntAutoPtr<IPipelineState> sm_pDrawInstancedPSO;
};

RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDrawProceduralPSO;
RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDrawPSO;
RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDraw_2xStride_PSO;
RefCntAutoPtr<IPipelineState> DrawCommandTest::sm_pDrawInstancedPSO;

TEST_F(DrawCommandTest, DrawProcedural)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawProceduralPSO);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    Present();
}


// Non-indexed draw calls


TEST_F(DrawCommandTest, Draw)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto     pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_StartVertex)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, // Skip 2 vertices using StartVertexLocation
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto     pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.StartVertexLocation = 2;
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_VBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, // Skip 3 vertices using buffer offset
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto     pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {3 * sizeof(Vertex)};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_StartVertex_VBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, // Skip 3 vertices using buffer offset
        {}, {},     // Skip 2 vertices using StartVertexLocation
        Vert[0], Vert[1], Vert[2],
        Vert[3], Vert[4], Vert[5]
    };
    // clang-format on

    auto     pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {3 * sizeof(Vertex)};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.StartVertexLocation = 2;
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, Draw_StartVertex_VBOffset_2xStride)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDraw_2xStride_PSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {},     // Skip 3 * sizeof(Vertex) using buffer offset
        {}, {}, {}, {}, // Skip 2 vertices using StartVertexLocation
        Vert[0], {}, Vert[1], {}, Vert[2], {}, 
        Vert[3], {}, Vert[4], {}, Vert[5], {}
    };
    // clang-format on

    auto     pVB       = CreateVertexBuffer(Triangles, sizeof(Triangles));
    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {3 * sizeof(Vertex)};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{6, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.StartVertexLocation = 2;
    pContext->Draw(drawAttrs);

    Present();
}



// Indexed draw calls (glDrawElements/DrawIndexed)

TEST_F(DrawCommandTest, DrawIndexed)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        Vert[0], {}, Vert[1], {}, {}, Vert[2],
        Vert[3], {}, {}, Vert[5], Vert[4]
    };
    Uint32 Indices[] = {2,4,7, 8,12,11};
    // clang-format on

    auto pVB = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pIB = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{6, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexed_IBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        Vert[0], {}, Vert[1], {}, {}, Vert[2],
        Vert[3], {}, {}, Vert[5], Vert[4]
    };
    Uint32 Indices[] = {0,0,0,0, 2,4,7, 8,12,11}; // Skip 4 indices using index buffer offset
    // clang-format on

    auto pVB = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pIB = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, sizeof(Uint32) * 4, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{6, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexed_IBOffset_BaseVertex)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawPSO);

    Uint32 bv = 2; // Base vertex
    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        Vert[0], {}, Vert[1], {}, {}, Vert[2],
        Vert[3], {}, {}, Vert[5], Vert[4]
    };
    Uint32 Indices[] = {0,0,0,0, 2-bv,4-bv,7-bv, 8-bv,12-bv,11-bv};
    // clang-format on

    auto pVB = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pIB = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB};
    Uint32   Offsets[] = {0};
    pContext->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, sizeof(Uint32) * 4, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{6, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.BaseVertex = bv;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


// Instanced non-indexed draw calls (glDrawArraysInstanced/DrawInstanced)

TEST_F(DrawCommandTest, DrawInstanced)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] = 
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2; // Draw two instances of the same triangle
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawInstanced_VBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, // Skip 2 vertices with VB offset
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] = 
    {
        {}, {}, {}, // Skip 3 instances with VB offset
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {2 * sizeof(Vertex), 3 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2; // Draw two instances of the same triangle
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawInstanced_StartVertex)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with start vertex
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] = 
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances        = 2; // Draw two instances of the same triangle
    drawAttrs.StartVertexLocation = 4; // Skip 4 vertices
    pContext->Draw(drawAttrs);

    Present();
}


// Instanced draw calls with first instance (glDrawArraysInstancedBaseInstance/DrawInstanced)

TEST_F(DrawCommandTest, DrawInstanced_FirstInstance)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstanceLocation
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->Draw(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawInstanced_FirstInstance_VBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, // Skip 3 vertices with buffer offset
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] = 
    {
        {}, {},         // Skip 2 instances with buffer offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstanceLocation
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {3 * sizeof(Vertex), 2 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->Draw(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawInstanced_FirstInstance_BaseVertex_FirstIndex_VBOffset_IBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with StartVertexLocation
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {},     // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs drawAttrs{3, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4;
    drawAttrs.StartVertexLocation   = 3;
    pContext->Draw(drawAttrs);

    Present();
}

// Instanced indexed draw calls (glDrawElementsInstanced/DrawIndexedInstanced)

TEST_F(DrawCommandTest, DrawIndexedInstanced)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] = 
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_IBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {0,0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] = 
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 5 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_VBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, // Skip 2 vertices with VBOffset
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, // Skip 4 instances with VB offset
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {2 * sizeof(Vertex), 4 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstIndex)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {0,0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] = 
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances       = 2;
    drawAttrs.FirstIndexLocation = 5;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


// Instanced indexed draw calls with first instance (glDrawElementsInstancedBaseInstance/DrawInstanced)


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_IBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 4 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_VBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_IBOffset_FirstIndex)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {0,0,0,0, 0,0,0, 4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 4 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    drawAttrs.FirstIndexLocation    = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}



// Instanced draw commands with base vertex (glDrawElementsInstancedBaseVertex/DrawInstanced)

TEST_F(DrawCommandTest, DrawIndexedInstanced_BaseVertex)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] = 
    {
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {0, 0};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances = 2;
    drawAttrs.BaseVertex   = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_BaseVertex_VBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    drawAttrs.BaseVertex            = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


TEST_F(DrawCommandTest, DrawIndexedInstanced_FirstInstance_BaseVertex_FirstIndex_VBOffset_IBOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {0,0,0,0, 0,0,0, 4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {}, // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 4 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs drawAttrs{3, VT_UINT32, DRAW_FLAG_VERIFY_ALL};
    drawAttrs.NumInstances          = 2;
    drawAttrs.FirstInstanceLocation = 4; // Skip 4 instances
    drawAttrs.BaseVertex            = 3;
    drawAttrs.FirstIndexLocation    = 3;
    pContext->DrawIndexed(drawAttrs);

    Present();
}


//  Indirect draw calls

TEST_F(DrawCommandTest, DrawInstancedIndirect_FirstInstance_BaseVertex_FirstIndex_VBOffset_IBOffset_InstOffset)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();
    if (!pDevice->GetDeviceCaps().Features.IndirectRendering)
        GTEST_SKIP() << "Indirect rendering is not supported on this device";

    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with StartVertexLocation
        VertInst[0], VertInst[1], VertInst[2]
    };
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {}, {}, // Skip 5 instances with VB offset
        {}, {}, {}, {},     // Skip 4 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {4 * sizeof(Vertex), 5 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            6, // NumVertices
            2, // NumInstances
            3, // StartVertexLocation
            4  // FirstInstanceLocation
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    DrawIndirectAttribs drawAttrs{DRAW_FLAG_VERIFY_ALL, RESOURCE_STATE_TRANSITION_MODE_TRANSITION};
    drawAttrs.IndirectDrawArgsOffset = 5 * sizeof(Uint32);
    pContext->DrawIndirect(drawAttrs, pIndirectArgsBuff);

    Present();
}

TEST_F(DrawCommandTest, DrawIndexedInstancedIndirect_FirstInstance_BaseVertex_FirstIndex_VBOffset_IBOffset_InstOffset)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pContext = pEnv->GetDeviceContext();

    SetRenderTargets(sm_pDrawInstancedPSO);

    // clang-format off
    const Vertex Triangles[] =
    {
        {}, {}, {}, {}, // Skip 4 vertices with VB offset
        {}, {}, {},     // Skip 3 vertices with BaseVertex
        {}, {},
        VertInst[1], {}, VertInst[0], {}, {}, VertInst[2]
    };
    Uint32 Indices[] = {0,0,0, 0,0,0,0, 4, 2, 7};
    const float4 InstancedData[] = 
    {
        {}, {}, {}, {},     // Skip 4 instances with VB offset
        {}, {}, {}, {}, {}, // Skip 5 instances with FirstInstance
        float4{0.5f,  0.5f,  -0.5f, -0.5f},
        float4{0.5f,  0.5f,  +0.5f, -0.5f}
    };
    // clang-format on

    auto pVB     = CreateVertexBuffer(Triangles, sizeof(Triangles));
    auto pInstVB = CreateVertexBuffer(InstancedData, sizeof(InstancedData));
    auto pIB     = CreateIndexBuffer(Indices, _countof(Indices));

    IBuffer* pVBs[]    = {pVB, pInstVB};
    Uint32   Offsets[] = {4 * sizeof(Vertex), 4 * sizeof(float4)};
    pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(pIB, 3 * sizeof(Uint32), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    Uint32 IndirectDrawData[] =
        {
            0, 0, 0, 0, 0, // Offset

            6, // NumIndices
            2, // NumInstances
            4, // FirstIndexLocation
            3, // BaseVertex
            5, // FirstInstanceLocation
        };
    auto pIndirectArgsBuff = CreateIndirectDrawArgsBuffer(IndirectDrawData, sizeof(IndirectDrawData));

    DrawIndexedIndirectAttribs drawAttrs{VT_UINT32, DRAW_FLAG_VERIFY_ALL, RESOURCE_STATE_TRANSITION_MODE_TRANSITION};
    drawAttrs.IndirectDrawArgsOffset = 5 * sizeof(Uint32);
    pContext->DrawIndexedIndirect(drawAttrs, pIndirectArgsBuff);

    Present();
}

} // namespace
