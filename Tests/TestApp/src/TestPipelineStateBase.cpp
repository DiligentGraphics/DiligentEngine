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
#include "TestPipelineStateBase.h"

using namespace Diligent;

static const char g_ShaderSource[] = 
"void VSMain(out float4 pos : SV_POSITION) \n"
"{                                         \n"
"	pos = float4(0.0, 0.0, 0.0, 0.0);      \n"
"}                                         \n"
"                                          \n"
"void PSMain(out float4 col : SV_TARGET)   \n"
"{                                         \n"
"	col = float4(0.0, 0.0, 0.0, 0.0);      \n"
"}                                         \n"
;

TestPipelineStateBase::TestPipelineStateBase(Diligent::IRenderDevice *pDevice, const char *Name) :
    UnitTestBase(Name),
    m_pDevice(pDevice)
{
    ShaderCreationAttribs Attrs;
    Attrs.Source = g_ShaderSource;
    Attrs.EntryPoint = "VSMain";
    Attrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
    Attrs.Desc.Name = "TrivialVS (TestPipelineStateBase)";
    Attrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    m_pDevice->CreateShader(Attrs, &m_pTrivialVS);

    Attrs.EntryPoint = "PSMain";
    Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
    Attrs.Desc.Name = "TrivialPS (TestPipelineStateBase)";
    m_pDevice->CreateShader(Attrs, &m_pTrivialPS);

    m_PSODesc.GraphicsPipeline.pVS = m_pTrivialVS;
    m_PSODesc.GraphicsPipeline.pPS = m_pTrivialPS;
    m_PSODesc.GraphicsPipeline.PrimitiveTopologyType = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    m_PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
    m_PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;
}
