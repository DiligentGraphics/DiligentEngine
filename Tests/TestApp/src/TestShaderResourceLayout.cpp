/*     Copyright 2015-2018 Egor Yusov
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
#include "TestShaderResourceLayout.h"
#include "BasicShaderSourceStreamFactory.h"

using namespace Diligent;

TestShaderResourceLayout::TestShaderResourceLayout( IRenderDevice *pDevice, IDeviceContext *pContext ) :
    UnitTestBase("Shader resource layout initialization test"),
    m_pDeviceContext(pContext)
{
    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory("Shaders");
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.EntryPoint = "main";

    RefCntAutoPtr<IShader> pVS;
    if(pDevice->GetDeviceCaps().DevType == DeviceType::Vulkan)
    {
        CreationAttrs.Desc.Name = "Shader resource layout test VS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.vsh";
        ShaderVariableDesc VarDesc[] = 
        {
            { "g_tex2D_Static", SHADER_VARIABLE_TYPE_STATIC },
            { "g_tex2D_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2D_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DArr_Static", SHADER_VARIABLE_TYPE_STATIC },
            { "g_tex2DArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2DArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "UniformBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "UniformBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "UniformBuffArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "UniformBuffArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "storageBuff_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "storageBuff_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "storageBuffArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "storageBuffArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DStorageImgArr_Mut", SHADER_VARIABLE_TYPE_MUTABLE },
            { "g_tex2DStorageImgArr_Dyn", SHADER_VARIABLE_TYPE_DYNAMIC },
            { "g_tex2DNoResourceTest", SHADER_VARIABLE_TYPE_DYNAMIC }
        };
        CreationAttrs.Desc.VariableDesc = VarDesc;
        CreationAttrs.Desc.NumVariables = _countof(VarDesc);

        pDevice->CreateShader(CreationAttrs, &pVS);
        VERIFY_EXPR(pVS);
    }

    RefCntAutoPtr<IShader> pPS;
    if (pDevice->GetDeviceCaps().DevType == DeviceType::Vulkan)
    {
        CreationAttrs.Desc.Name = "Shader resource layout test FS";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        CreationAttrs.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
        CreationAttrs.FilePath = "Shaders\\ShaderResLayoutTestGL.psh";
        pDevice->CreateShader(CreationAttrs, &pPS);
        VERIFY_EXPR(pPS);
    }

    PipelineStateDesc PSODesc;
    PSODesc.Name = "Shader resource layout test";
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
    PSODesc.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;

    RefCntAutoPtr<IPipelineState> pTestPSO;
    pDevice->CreatePipelineState(PSODesc, &pTestPSO);
    VERIFY_EXPR(pTestPSO);

    SetStatus(TestResult::Succeeded);
}
