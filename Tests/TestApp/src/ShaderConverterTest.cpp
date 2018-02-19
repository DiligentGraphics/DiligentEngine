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
#include "ShaderConverterTest.h"
#include "FileSystem.h"
#include "Errors.h"
#include "ScriptParser.h"
#include "ConvenienceFunctions.h"
#include "BasicShaderSourceStreamFactory.h"
#include "HLSL2GLSLConverter.h"

using namespace Diligent;

ShaderConverterTest::ShaderConverterTest( IRenderDevice *pRenderDevice, IDeviceContext *pContext ) :
    UnitTestBase("Shader Converter Test")
{
    ShaderCreationAttribs CreationAttrs;
    CreationAttrs.FilePath = "Shaders\\ConverterTest.fx";
    BasicShaderSourceStreamFactory BasicSSSFactory("Shaders");
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    CreationAttrs.Desc.Name = "Test converted shader";
    RefCntAutoPtr<IHLSL2GLSLConversionStream> pStream;
    CreationAttrs.ppConversionStream = pStream.GetRawDblPtr();

    {
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.EntryPoint = "TestPS";
        RefCntAutoPtr<IShader> pShader;
        pRenderDevice->CreateShader( CreationAttrs, &pShader );
        VERIFY_EXPR( pShader );
    }

    {
        CreationAttrs.EntryPoint = "TestVS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        RefCntAutoPtr<IShader> pShader;
        pRenderDevice->CreateShader( CreationAttrs, &pShader );
        VERIFY_EXPR( pShader );
    }

    std::string status;
    if(pRenderDevice->GetDeviceCaps().bComputeShadersSupported)
    {
#if PLATFORM_LINUX
        CreationAttrs.FilePath = "Shaders\\CSConversionTest.fx";
        CreationAttrs.EntryPoint = "TestCS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        RefCntAutoPtr<IShader> pShader;
        pRenderDevice->CreateShader( CreationAttrs, &pShader );
        VERIFY_EXPR( pShader );
#elif PLATFORM_WIN32
        status = "Skipped compute shader conversion test as on Win32, only 8 image uniforms are allowed. Besides, an error is generated when passing image as a function argument.";
#else
        status = "Skipped compute shader conversion test";
#endif
    }
    else
        status = "Skipped compute shader conversion test";

    SetStatus(TestResult::Succeeded, status.c_str());
}
