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

#include <sstream>
#include <vector>
#include <thread>

#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

extern "C"
{
    int TestQueryCInterface(void* pQuery);
}

namespace
{

// clang-format off
const std::string QueryTest_ProceduralQuadVS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
};

void main(in uint VertId : SV_VertexID,
          out PSInput PSIn) 
{
    float HalfTexel = 0.5 / 512.0;
    float size = 0.25;
    float4 Pos[4];

    Pos[0] = float4(-size-HalfTexel, -size-HalfTexel, 0.0, 1.0);
    Pos[1] = float4(-size-HalfTexel, +size-HalfTexel, 0.0, 1.0);
    Pos[2] = float4(+size-HalfTexel, -size-HalfTexel, 0.0, 1.0);
    Pos[3] = float4(+size-HalfTexel, +size-HalfTexel, 0.0, 1.0);

    PSIn.Pos = Pos[VertId];
}
)"
};

const std::string QueryTest_PS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
};

float4 main(in PSInput PSIn) : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
)"
};
// clang-format on


class QueryTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        TextureDesc TexDesc;
        TexDesc.Name      = "Mips generation test texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.Width     = sm_TextureSize;
        TexDesc.Height    = sm_TextureSize;
        TexDesc.BindFlags = BIND_RENDER_TARGET;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;
        RefCntAutoPtr<ITexture> pRenderTarget;
        pDevice->CreateTexture(TexDesc, nullptr, &pRenderTarget);
        ASSERT_NE(pRenderTarget, nullptr) << "TexDesc:\n"
                                          << TexDesc;
        sm_pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        ASSERT_NE(sm_pRTV, nullptr);

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Query command test - procedural quad";

        PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipeline.NumRenderTargets             = 1;
        GraphicsPipeline.RTVFormats[0]                = TexDesc.Format;
        GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler             = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);
        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Query test vertex shader";
            ShaderCI.Source          = QueryTest_ProceduralQuadVS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Query test pixel shader";
            ShaderCI.Source          = QueryTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pVS, nullptr);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_pPSO);
        ASSERT_NE(sm_pPSO, nullptr);
    }

    static void TearDownTestSuite()
    {
        sm_pPSO.Release();
        sm_pRTV.Release();

        auto* pEnv = TestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static void DrawQuad()
    {
        auto* pEnv     = TestingEnvironment::GetInstance();
        auto* pContext = pEnv->GetDeviceContext();

        ITextureView* pRTVs[] = {sm_pRTV};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const float ClearColor[] = {0.f, 0.f, 0.f, 0.0f};
        pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(sm_pPSO);

        DrawAttribs drawAttrs{4, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(drawAttrs);
    }

    static constexpr Uint32 sm_NumTestQueries = 3;
    static constexpr Uint32 sm_NumFrames      = 5;

    void InitTestQueries(std::vector<RefCntAutoPtr<IQuery>>& Queries, const QueryDesc& queryDesc)
    {
        auto* pEnv     = TestingEnvironment::GetInstance();
        auto* pDevice  = pEnv->GetDevice();
        auto* pContext = pEnv->GetDeviceContext();

        if (Queries.empty())
        {
            Queries.resize(sm_NumTestQueries);
            for (auto& pQuery : Queries)
            {
                pDevice->CreateQuery(queryDesc, &pQuery);
                ASSERT_NE(pQuery, nullptr) << "Failed to create pipeline stats query";
            }
        }

        // Nested queries are not supported by OpenGL and Vulkan
        for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
        {
            pContext->BeginQuery(Queries[i]);
            for (Uint32 j = 0; j < i + 1; ++j)
                DrawQuad();
            pContext->EndQuery(Queries[i]);
            Queries[i]->GetData(nullptr, 0);
        }

        if (queryDesc.Type == QUERY_TYPE_DURATION)
        {
            // FinishFrame() must be called to finish the disjoint query
            pContext->Flush();
            pContext->FinishFrame();
        }
        pContext->WaitForIdle();
        if (pDevice->GetDeviceCaps().IsGLDevice())
        {
            // glFinish() is not a guarantee that queries will become available.
            // Even using glFenceSync + glClientWaitSync does not help.
            for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
            {
                WaitForQuery(Queries[i]);
            }
        }
    }

    static void WaitForQuery(IQuery* pQuery)
    {
        while (!pQuery->GetData(nullptr, 0))
        {
            std::this_thread::yield();
        }
    }

    static constexpr Uint32 sm_TextureSize = 512;

    static RefCntAutoPtr<ITextureView>   sm_pRTV;
    static RefCntAutoPtr<IPipelineState> sm_pPSO;
};

RefCntAutoPtr<ITextureView>   QueryTest::sm_pRTV;
RefCntAutoPtr<IPipelineState> QueryTest::sm_pPSO;

TEST_F(QueryTest, PipelineStats)
{
    const auto& deviceCaps = TestingEnvironment::GetInstance()->GetDevice()->GetDeviceCaps();
    if (!deviceCaps.Features.PipelineStatisticsQueries)
    {
        GTEST_SKIP() << "Pipeline statistics queries are not supported by this device";
    }

    const auto IsGL = deviceCaps.IsGLDevice();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Pipeline stats query";
    queryDesc.Type = QUERY_TYPE_PIPELINE_STATISTICS;

    std::vector<RefCntAutoPtr<IQuery>> Queries;
    for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
    {
        InitTestQueries(Queries, queryDesc);

        for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
        {
            Uint32 DrawCounter = 1 + i;

            QueryDataPipelineStatistics QueryData;

            auto QueryReady = Queries[i]->GetData(nullptr, 0);
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            QueryReady = Queries[i]->GetData(&QueryData, sizeof(QueryData));
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            if (!IsGL)
            {
                EXPECT_GE(QueryData.InputVertices, 4 * DrawCounter);
                EXPECT_GE(QueryData.InputPrimitives, 2 * DrawCounter);
                EXPECT_GE(QueryData.ClippingPrimitives, 2 * DrawCounter);
                EXPECT_GE(QueryData.VSInvocations, 4 * DrawCounter);
                auto NumPixels = sm_TextureSize * sm_TextureSize / 16;
                EXPECT_GE(QueryData.PSInvocations, NumPixels * DrawCounter);
            }
            EXPECT_GE(QueryData.ClippingInvocations, 2 * DrawCounter);
        }
    }
}

TEST_F(QueryTest, Occlusion)
{
    const auto& deviceCaps = TestingEnvironment::GetInstance()->GetDevice()->GetDeviceCaps();
    if (!deviceCaps.Features.OcclusionQueries)
    {
        GTEST_SKIP() << "Occlusion queries are not supported by this device";
    }

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Occlusion query";
    queryDesc.Type = QUERY_TYPE_OCCLUSION;

    std::vector<RefCntAutoPtr<IQuery>> Queries;
    for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
    {
        InitTestQueries(Queries, queryDesc);

        for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
        {
            Uint32 DrawCounter = 1 + i;

            QueryDataOcclusion QueryData;

            auto QueryReady = Queries[i]->GetData(nullptr, 0);
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            QueryReady = Queries[i]->GetData(&QueryData, sizeof(QueryData));
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            auto NumPixels = sm_TextureSize * sm_TextureSize / 16;
            EXPECT_GE(QueryData.NumSamples, NumPixels * DrawCounter);
        }
    }
}

TEST_F(QueryTest, BinaryOcclusion)
{
    const auto& deviceCaps = TestingEnvironment::GetInstance()->GetDevice()->GetDeviceCaps();
    if (!deviceCaps.Features.BinaryOcclusionQueries)
    {
        GTEST_SKIP() << "Binary occlusion queries are not supported by this device";
    }

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Binary occlusion query";
    queryDesc.Type = QUERY_TYPE_BINARY_OCCLUSION;

    std::vector<RefCntAutoPtr<IQuery>> Queries;
    for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
    {
        InitTestQueries(Queries, queryDesc);

        for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
        {
            QueryDataBinaryOcclusion QueryData;

            auto QueryReady = Queries[i]->GetData(nullptr, 0);
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            Queries[i]->GetData(&QueryData, sizeof(QueryData));
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            EXPECT_TRUE(QueryData.AnySamplePassed);
        }
    }
}

TEST_F(QueryTest, Timestamp)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    const auto& deviceCaps = pDevice->GetDeviceCaps();
    if (!deviceCaps.Features.TimestampQueries)
    {
        GTEST_SKIP() << "Timestamp queries are not supported by this device";
    }

    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Timestamp query";
    queryDesc.Type = QUERY_TYPE_TIMESTAMP;

    RefCntAutoPtr<IQuery> pQueryStart;
    pDevice->CreateQuery(queryDesc, &pQueryStart);
    ASSERT_NE(pQueryStart, nullptr) << "Failed to create tiemstamp query";

    RefCntAutoPtr<IQuery> pQueryEnd;
    pDevice->CreateQuery(queryDesc, &pQueryEnd);
    ASSERT_NE(pQueryEnd, nullptr) << "Failed to create tiemstamp query";

    for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
    {
        pContext->EndQuery(pQueryStart);
        pQueryStart->GetData(nullptr, 0);
        DrawQuad();
        pContext->EndQuery(pQueryEnd);
        pQueryEnd->GetData(nullptr, 0);

        pContext->Flush();
        pContext->FinishFrame();
        pContext->WaitForIdle();
        if (pDevice->GetDeviceCaps().IsGLDevice())
        {
            // glFinish() is not a guarantee that queries will become available
            // Even using glFenceSync + glClientWaitSync does not help.
            WaitForQuery(pQueryStart);
            WaitForQuery(pQueryEnd);
        }

        QueryDataTimestamp QueryStartData, QueryEndData;

        auto QueryReady = pQueryStart->GetData(nullptr, 0);
        ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
        QueryReady = pQueryStart->GetData(&QueryStartData, sizeof(QueryStartData));
        ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";

        QueryReady = pQueryEnd->GetData(nullptr, 0);
        ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
        QueryReady = pQueryEnd->GetData(&QueryEndData, sizeof(QueryEndData), false);
        ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
        EXPECT_EQ(TestQueryCInterface(pQueryEnd.RawPtr()), 0);
        EXPECT_TRUE(QueryStartData.Frequency == 0 || QueryEndData.Frequency == 0 || QueryEndData.Counter > QueryStartData.Counter);
    }
}


TEST_F(QueryTest, Duration)
{
    const auto& deviceCaps = TestingEnvironment::GetInstance()->GetDevice()->GetDeviceCaps();
    if (!deviceCaps.Features.DurationQueries)
    {
        GTEST_SKIP() << "Duration queries are not supported by this device";
    }

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Duration query";
    queryDesc.Type = QUERY_TYPE_DURATION;

    std::vector<RefCntAutoPtr<IQuery>> Queries;
    for (Uint32 frame = 0; frame < sm_NumFrames; ++frame)
    {
        InitTestQueries(Queries, queryDesc);

        for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
        {
            QueryDataDuration QueryData;

            auto QueryReady = Queries[i]->GetData(nullptr, 0);
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            Queries[i]->GetData(&QueryData, sizeof(QueryData));
            ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
            EXPECT_TRUE(QueryData.Frequency == 0 || QueryData.Duration > 0);
        }
    }
}

} // namespace
