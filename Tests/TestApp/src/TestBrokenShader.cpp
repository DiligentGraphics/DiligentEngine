/*     Copyright 2015-2019 Egor Yusov
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
#include "TestBrokenShader.h"

using namespace Diligent;

static const char g_BrokenShaderSource[] =
R"(
void VSMain(out float4 pos : SV_POSITION)
{
    pos = float3(0.0, 0.0, 0.0, 0.0);
}
)";

TestBrokenShader::TestBrokenShader(IRenderDevice *pDevice) : 
    UnitTestBase("Broken shader test")
{
    ShaderCreateInfo Attrs;
    Attrs.Source = g_BrokenShaderSource;
    Attrs.EntryPoint = "VSMain";
    Attrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
    Attrs.Desc.Name = "Broken shader test";
    Attrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    Attrs.UseCombinedTextureSamplers = true;
    RefCntAutoPtr<IShader> pBrokenShader;
    IDataBlob *pErrors = nullptr;
    Attrs.ppCompilerOutput = &pErrors;
    LOG_INFO_MESSAGE("\n\nNo worries, testing broken shader...");
    pDevice->CreateShader(Attrs, &pBrokenShader);
    VERIFY_EXPR(!pBrokenShader);
    VERIFY_EXPR(pErrors != nullptr);
    const char* Msg = reinterpret_cast<const char*>(pErrors->GetDataPtr());
    LOG_INFO_MESSAGE("Compiler output:\n", Msg);
    pErrors->Release();
    LOG_INFO_MESSAGE("No worries, errors above are result of the broken shader test.\n");

    SetStatus(TestResult::Succeeded);
}
