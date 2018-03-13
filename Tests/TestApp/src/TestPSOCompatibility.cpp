/*     Copyright 2015-2017 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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

// EngineSandbox.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "TestPSOCompatibility.h"

using namespace Diligent;

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


RefCntAutoPtr<IPipelineState> TestPSOCompatibility::CreateTestPSO(const char *VSSource, const char *PSSource)
{
    PipelineStateDesc PSODesc;
    PSODesc.IsComputePipeline = false;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM_SRGB;
    PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreationAttribs CreationAttrs;
    CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    RefCntAutoPtr<IShader> pVS;
    {
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.EntryPoint = "main";
        CreationAttrs.Desc.Name = "PSO Compatibility test VS";
        CreationAttrs.Source = VSSource;
        m_pDevice->CreateShader(CreationAttrs, &pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.EntryPoint = "main";
        CreationAttrs.Desc.Name = "PSO Compatibility test PS";
        CreationAttrs.Source = PSSource;
        m_pDevice->CreateShader(CreationAttrs, &pPS);
    }

    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;

    RefCntAutoPtr<IPipelineState> pPSO;
    m_pDevice->CreatePipelineState(PSODesc, &pPSO);

    return pPSO;
}

RefCntAutoPtr<IPipelineState> TestPSOCompatibility::CreateTestPSO(const char *CSSource)
{
    PipelineStateDesc PSODesc;
    PSODesc.IsComputePipeline = true;
    ShaderCreationAttribs CreationAttrs;
    CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    RefCntAutoPtr<IShader> pCS;
    {
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        CreationAttrs.EntryPoint = "main";
        CreationAttrs.Desc.Name = "PSO Compatibility test CS";
        CreationAttrs.Source = CSSource;
        m_pDevice->CreateShader(CreationAttrs, &pCS);
    }
    PSODesc.ComputePipeline.pCS = pCS;

    RefCntAutoPtr<IPipelineState> pPSO;
    m_pDevice->CreatePipelineState(PSODesc, &pPSO);

    return pPSO;
}

TestPSOCompatibility::TestPSOCompatibility(IRenderDevice *pDevice ) : 
    UnitTestBase("PSO Compatibility test"),
    m_pDevice(pDevice)
{
    auto DevType = pDevice->GetDeviceCaps().DevType;
    auto PSO0 = CreateTestPSO(VS0, PS0);
    VERIFY_EXPR(PSO0->IsCompatibleWith(PSO0));
    auto PSO0_1 = CreateTestPSO(VS0, PS0);
    VERIFY_EXPR(PSO0->IsCompatibleWith(PSO0_1));
    VERIFY_EXPR(PSO0_1->IsCompatibleWith(PSO0));
    
    auto PSO_Tex = CreateTestPSO(VS0, PS_Tex);
    auto PSO_Tex2 = CreateTestPSO(VS0, PS_Tex2);
    auto PSO_TexArr = CreateTestPSO(VS0, PS_TexArr);
    auto PSO_ArrOfTex = CreateTestPSO(VS0, PS_ArrOfTex);
    VERIFY_EXPR(PSO_Tex->IsCompatibleWith(PSO_Tex2));
    if(DevType!=DeviceType::D3D12)
        VERIFY_EXPR(!PSO_Tex->IsCompatibleWith(PSO_TexArr));
    VERIFY_EXPR(!PSO_Tex->IsCompatibleWith(PSO_ArrOfTex));
    if (DevType != DeviceType::D3D12)
        VERIFY_EXPR(!PSO_Tex2->IsCompatibleWith(PSO_TexArr));
    VERIFY_EXPR(!PSO_Tex2->IsCompatibleWith(PSO_ArrOfTex));
    VERIFY_EXPR(!PSO_TexArr->IsCompatibleWith(PSO_ArrOfTex));

    auto PSO_CB = CreateTestPSO(VS0, PS_CB);
    auto PSO1_CB = CreateTestPSO(VS0, PS1_CB);
    auto PSO_2CB = CreateTestPSO(VS0, PS_2CB);
    VERIFY_EXPR(PSO_CB->IsCompatibleWith(PSO1_CB));
    VERIFY_EXPR(!PSO_CB->IsCompatibleWith(PSO_2CB));

    auto PSO_TexCB = CreateTestPSO(VS0, PS_TexCB);
    auto PSO_TexCB2 = CreateTestPSO(VS0, PS_TexCB2);
    VERIFY_EXPR(PSO_TexCB->IsCompatibleWith(PSO_TexCB2));
    VERIFY_EXPR(PSO_TexCB2->IsCompatibleWith(PSO_TexCB));

    if(pDevice->GetDeviceCaps().bComputeShadersSupported)
    {
        auto PSO_RWBuff = CreateTestPSO(CS_RwBuff);
        auto PSO_RWBuff2 = CreateTestPSO(CS_RwBuff2);
        auto PSO_RWBuff3 = CreateTestPSO(CS_RwBuff3);
        VERIFY_EXPR(PSO_RWBuff->IsCompatibleWith(PSO_RWBuff2));
        VERIFY_EXPR(!PSO_RWBuff->IsCompatibleWith(PSO_RWBuff3));
    }

    SetStatus(TestResult::Succeeded);
}
