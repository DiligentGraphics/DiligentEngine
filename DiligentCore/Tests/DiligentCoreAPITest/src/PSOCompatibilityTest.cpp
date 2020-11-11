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

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static const char* VS0 = R"(
float4 main() : SV_Position
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";

static const char* PS0 = R"(
float4 main() : SV_Target
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";

static const char* PS_Tex = R"(
Texture2D<float4> g_tex2D;
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_tex2D.Sample(g_tex2D_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_Tex2 = R"(
Texture2D<float4> g_tex2D2;
SamplerState g_tex2D2_sampler;
float4 main() : SV_Target
{
    return g_tex2D2.Sample(g_tex2D2_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_ArrOfTex = R"(
Texture2D<float4> g_tex2D[2];
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_tex2D[0].Sample(g_tex2D_sampler, float2(0.0, 0.0)) + g_tex2D[1].Sample(g_tex2D_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_TexArr = R"(
Texture2DArray<float4> g_tex2D;
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_tex2D.Sample(g_tex2D_sampler, float3(0.0, 0.0, 0.0));
}
)";


static const char* PS_CB = R"(
cbuffer Test
{
    float4 g_Test;
};

float4 main() : SV_Target
{
    return g_Test;
}
)";

static const char* PS1_CB = R"(
cbuffer Test
{
    float4 g_Test;
    float4 g_Test2;
};

float4 main() : SV_Target
{
    return g_Test + g_Test2;
}
)";

static const char* PS_2CB = R"(
cbuffer Test
{
    float4 g_Test;
};

cbuffer Test2
{
    float4 g_Test2;
};

float4 main() : SV_Target
{
    return g_Test + g_Test2;
}
)";

static const char* PS_TexCB = R"(
cbuffer Test
{
    float4 g_Test;
};

cbuffer Test2
{
    float4 g_Test2;
};

Texture2D<float4> g_tex2D;
SamplerState g_tex2D_sampler;
float4 main() : SV_Target
{
    return g_Test + g_Test2 + g_tex2D.Sample(g_tex2D_sampler, float2(0.0, 0.0));
}
)";

static const char* PS_TexCB2 = R"(
cbuffer TestA
{
    float4 g_Test;
};

cbuffer Test2A
{
    float4 g_Test2;
};

Texture2D<float4> g_tex2DA;
SamplerState g_tex2DA_sampler;
float4 main() : SV_Target
{
    return g_Test + g_Test2 + g_tex2DA.Sample(g_tex2DA_sampler, float2(0.0, 0.0));
}
)";

static const char* CS_RwBuff = R"(
RWTexture2D<float/* format=r32f */> g_RWTex;

[numthreads(1,1,1)]
void main()
{
    g_RWTex[int2(0,0)] = 0.0;
}
)";

static const char* CS_RwBuff2 = R"(
RWTexture2D<float/* format=r32f */> g_RWTex2;

[numthreads(1,1,1)]
void main()
{
    g_RWTex2[int2(0,0)] = 0.0;
}
)";

static const char* CS_RwBuff3 = R"(
RWTexture2D<float/* format=r32f */> g_RWTex;
RWTexture2D<float/* format=r32f */> g_RWTex2;

[numthreads(1,1,1)]
void main()
{
    g_RWTex[int2(0,0)] = 0.0;
    g_RWTex2[int2(0,0)] = 0.0;
}
)";


RefCntAutoPtr<IPipelineState> CreateGraphicsPSO(TestingEnvironment* pEnv, const char* VSSource, const char* PSSource)
{
    auto* pDevice = pEnv->GetDevice();

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = TEX_FORMAT_RGBA8_UNORM_SRGB;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo CreationAttrs;
    CreationAttrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    CreationAttrs.ShaderCompiler             = pEnv->GetDefaultCompiler(CreationAttrs.SourceLanguage);
    CreationAttrs.UseCombinedTextureSamplers = true;
    RefCntAutoPtr<IShader> pVS;
    {
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.EntryPoint      = "main";
        CreationAttrs.Desc.Name       = "PSO Compatibility test VS";
        CreationAttrs.Source          = VSSource;
        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS != nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.EntryPoint      = "main";
        CreationAttrs.Desc.Name       = "PSO Compatibility test PS";
        CreationAttrs.Source          = PSSource;
        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS != nullptr);
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    VERIFY_EXPR(pPSO != nullptr);

    return pPSO;
}

RefCntAutoPtr<IPipelineState> CreateComputePSO(TestingEnvironment* pEnv, const char* CSSource)
{
    auto*                          pDevice = pEnv->GetDevice();
    ComputePipelineStateCreateInfo PSOCreateInfo;

    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    ShaderCreateInfo CreationAttrs;
    CreationAttrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    CreationAttrs.ShaderCompiler             = pEnv->GetDefaultCompiler(CreationAttrs.SourceLanguage);
    CreationAttrs.UseCombinedTextureSamplers = true;
    RefCntAutoPtr<IShader> pCS;
    {
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        CreationAttrs.EntryPoint      = "main";
        CreationAttrs.Desc.Name       = "PSO Compatibility test CS";
        CreationAttrs.Source          = CSSource;
        pDevice->CreateShader(CreationAttrs, &pCS);
        VERIFY_EXPR(pCS != nullptr);
    }
    PSOCreateInfo.pCS = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateComputePipelineState(PSOCreateInfo, &pPSO);
    VERIFY_EXPR(pPSO != nullptr);

    return pPSO;
}

TEST(PSOCompatibility, IsCompatibleWith)
{
    auto* pEnv    = TestingEnvironment::GetInstance();
    auto* pDevice = pEnv->GetDevice();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto DevType = pDevice->GetDeviceCaps().DevType;
    auto PSO0    = CreateGraphicsPSO(pEnv, VS0, PS0);
    ASSERT_TRUE(PSO0);
    EXPECT_TRUE(PSO0->IsCompatibleWith(PSO0));
    auto PSO0_1 = CreateGraphicsPSO(pEnv, VS0, PS0);
    ASSERT_TRUE(PSO0_1);
    EXPECT_TRUE(PSO0->IsCompatibleWith(PSO0_1));
    EXPECT_TRUE(PSO0_1->IsCompatibleWith(PSO0));

    auto PSO_Tex      = CreateGraphicsPSO(pEnv, VS0, PS_Tex);
    auto PSO_Tex2     = CreateGraphicsPSO(pEnv, VS0, PS_Tex2);
    auto PSO_TexArr   = CreateGraphicsPSO(pEnv, VS0, PS_TexArr);
    auto PSO_ArrOfTex = CreateGraphicsPSO(pEnv, VS0, PS_ArrOfTex);
    ASSERT_TRUE(PSO_Tex);
    ASSERT_TRUE(PSO_Tex2);
    ASSERT_TRUE(PSO_TexArr);
    ASSERT_TRUE(PSO_ArrOfTex);
    EXPECT_TRUE(PSO_Tex->IsCompatibleWith(PSO_Tex2));
    if (DevType != RENDER_DEVICE_TYPE_D3D12 && DevType != RENDER_DEVICE_TYPE_VULKAN)
    {
        EXPECT_FALSE(PSO_Tex->IsCompatibleWith(PSO_TexArr));
    }
    VERIFY_EXPR(!PSO_Tex->IsCompatibleWith(PSO_ArrOfTex));
    if (DevType != RENDER_DEVICE_TYPE_D3D12 && DevType != RENDER_DEVICE_TYPE_VULKAN)
    {
        EXPECT_FALSE(PSO_Tex2->IsCompatibleWith(PSO_TexArr));
    }
    EXPECT_FALSE(PSO_Tex2->IsCompatibleWith(PSO_ArrOfTex));
    EXPECT_FALSE(PSO_TexArr->IsCompatibleWith(PSO_ArrOfTex));

    auto PSO_CB  = CreateGraphicsPSO(pEnv, VS0, PS_CB);
    auto PSO1_CB = CreateGraphicsPSO(pEnv, VS0, PS1_CB);
    auto PSO_2CB = CreateGraphicsPSO(pEnv, VS0, PS_2CB);
    EXPECT_TRUE(PSO_CB->IsCompatibleWith(PSO1_CB));
    EXPECT_FALSE(PSO_CB->IsCompatibleWith(PSO_2CB));

    auto PSO_TexCB  = CreateGraphicsPSO(pEnv, VS0, PS_TexCB);
    auto PSO_TexCB2 = CreateGraphicsPSO(pEnv, VS0, PS_TexCB2);
    EXPECT_TRUE(PSO_TexCB->IsCompatibleWith(PSO_TexCB2));
    EXPECT_TRUE(PSO_TexCB2->IsCompatibleWith(PSO_TexCB));

    if (pDevice->GetDeviceCaps().Features.ComputeShaders)
    {
        auto PSO_RWBuff  = CreateComputePSO(pEnv, CS_RwBuff);
        auto PSO_RWBuff2 = CreateComputePSO(pEnv, CS_RwBuff2);
        auto PSO_RWBuff3 = CreateComputePSO(pEnv, CS_RwBuff3);
        EXPECT_TRUE(PSO_RWBuff);
        EXPECT_TRUE(PSO_RWBuff2);
        EXPECT_TRUE(PSO_RWBuff3);
        EXPECT_TRUE(PSO_RWBuff->IsCompatibleWith(PSO_RWBuff2));
        EXPECT_FALSE(PSO_RWBuff->IsCompatibleWith(PSO_RWBuff3));
    }
}

} // namespace
