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
#include "RenderScriptTest.h"
#include "FileSystem.h"
#include "Errors.h"
#include "ScriptParser.h"
#include "ConvenienceFunctions.h"

using namespace Diligent;

RenderScriptTest::RenderScriptTest( IRenderDevice *pRenderDevice, IDeviceContext *pContext ) :
    UnitTestBase("RenderScript test")
{
#if PLATFORM_IOS
    SetStatus(TestResult::Skipped);
    return;
#endif
    
    RefCntAutoPtr<ITexture> pTestGlobalTexture;
    {
        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Name = "Test Global Texture 2D";
        TexDesc.Width = 1024;
        TexDesc.Height = 512;
        TexDesc.MipLevels = 1;
        TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.Usage = USAGE_DYNAMIC;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pRenderDevice->CreateTexture( TexDesc, TextureData(), &pTestGlobalTexture );
    }

    auto pScript = CreateRenderScriptFromFile( "LuaTest.lua", pRenderDevice, pContext, [&]( Diligent::ScriptParser *pScriptParser )
    {
        pScriptParser->SetGlobalVariable( "TestGlobalBool", True );
        pScriptParser->SetGlobalVariable( "TestGlobalInt", (Int32)19 );
        pScriptParser->SetGlobalVariable( "TestGlobalFloat", (Float32)139.25 );
        pScriptParser->SetGlobalVariable( "TestGlobalString", "Test Global String" );
        
        {
            Diligent::BufferDesc BuffDesc;
            BuffDesc.Name = "TestGlobalBuff";
            BuffDesc.uiSizeInBytes = 256;
            BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
            BuffDesc.Usage = USAGE_DYNAMIC;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
            RefCntAutoPtr<IBuffer> pBuffer;
            pRenderDevice->CreateBuffer( BuffDesc, Diligent::BufferData(), &pBuffer );
            pScriptParser->SetGlobalVariable( "TestGlobalBuffer", pBuffer );
        }

        {
            Diligent::BufferDesc BuffDesc;
            BuffDesc.Name = "TestGlobalBuff2";
            BuffDesc.uiSizeInBytes = 64;
            BuffDesc.BindFlags = BIND_VERTEX_BUFFER | BIND_UNORDERED_ACCESS;
            BuffDesc.Usage = USAGE_DEFAULT;
            BuffDesc.Format.ValueType = VT_UINT16;
            BuffDesc.Format.NumComponents = 4;
            BuffDesc.Format.IsNormalized = true;
            BuffDesc.Mode = BUFFER_MODE_FORMATTED;
            RefCntAutoPtr<IBuffer> pBuffer;
            pRenderDevice->CreateBuffer( BuffDesc, Diligent::BufferData(), &pBuffer );
            pScriptParser->SetGlobalVariable( "TestGlobalBufferWithUAV", pBuffer );

            auto *pUAV = pBuffer->GetDefaultView( BUFFER_VIEW_UNORDERED_ACCESS );
            pScriptParser->SetGlobalVariable( "TestGlobalBuffer2UAV", pUAV );
        }

        {
            SamplerDesc SamplerDesc;
            SamplerDesc.Name = "Test Sampler";
            SamplerDesc.MinFilter = FILTER_TYPE_COMPARISON_POINT;
            SamplerDesc.MagFilter = FILTER_TYPE_COMPARISON_LINEAR;
            SamplerDesc.MipFilter = FILTER_TYPE_COMPARISON_LINEAR;
            SamplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
            SamplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
            SamplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
            SamplerDesc.MipLODBias = 4;
            SamplerDesc.MinLOD = 1.5f;
            SamplerDesc.MaxLOD = 4;
            SamplerDesc.MaxAnisotropy = 2;
            SamplerDesc.ComparisonFunc = COMPARISON_FUNC_LESS;
            SamplerDesc.BorderColor[0] = 1.5f;
            SamplerDesc.BorderColor[1] = 2.25f;
            SamplerDesc.BorderColor[2] = 3.125f;
            SamplerDesc.BorderColor[3] = 4.0625f;
            RefCntAutoPtr<ISampler> pSampler;
            pRenderDevice->CreateSampler( SamplerDesc, &pSampler );
            pScriptParser->SetGlobalVariable( "TestGlobalSampler", pSampler );
        }

        {
            pScriptParser->SetGlobalVariable( "TestGlobalTexture", pTestGlobalTexture );
        }

        {
            DrawAttribs GlobalDrawAttribs;
            GlobalDrawAttribs.Topology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            GlobalDrawAttribs.NumVertices = 123;
            GlobalDrawAttribs.IndexType = VT_UINT16;
            GlobalDrawAttribs.IsIndexed = True;
            GlobalDrawAttribs.NumInstances = 19;
            GlobalDrawAttribs.IsIndirect = True;
            GlobalDrawAttribs.BaseVertex = 97;
            GlobalDrawAttribs.IndirectDrawArgsOffset = 120;
            GlobalDrawAttribs.StartVertexLocation = 98;
            pScriptParser->SetGlobalVariable( "TestGlobalDrawAttribs", GlobalDrawAttribs );
        }

        auto DevType = pRenderDevice->GetDeviceCaps().DevType;
        if( DevType == DeviceType::D3D11 || DevType == DeviceType::D3D12 )
        {
            TextureViewDesc TestTexViewDesc;
            TestTexViewDesc.Name = "TestTextureSRV2";
            TestTexViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
            TestTexViewDesc.TextureDim = RESOURCE_DIM_TEX_2D;
            TestTexViewDesc.Format = TEX_FORMAT_RGBA8_UNORM;
            RefCntAutoPtr<ITextureView> pTestTextureView;
            pTestGlobalTexture->CreateView( TestTexViewDesc, &pTestTextureView );
            pScriptParser->SetGlobalVariable( "TestGlobalTextureView", pTestTextureView );
        }

        {
            ResourceMappingEntry Entries[] = { 
                    { "TestGlobalTextureSRV", pTestGlobalTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)},
                    ResourceMappingEntry() 
            };
            ResourceMappingDesc ResMappingDesc;
            ResMappingDesc.pEntries= Entries;
            RefCntAutoPtr<IResourceMapping> pResMapping;
            pRenderDevice->CreateResourceMapping( ResMappingDesc, &pResMapping );
            pScriptParser->SetGlobalVariable( "TestGlobalResourceMapping", pResMapping );
        }
    } );

    {
        RefCntAutoPtr<ISampler> pTestSampler;
        pScript->GetSamplerByName( "TestSampler", &pTestSampler );
        const auto &SamplerDesc = pTestSampler->GetDesc();
        assert( strcmp(SamplerDesc.Name, "Test Sampler") == 0 );
        assert( SamplerDesc.MinFilter == FILTER_TYPE_POINT );
        assert( SamplerDesc.MagFilter == FILTER_TYPE_LINEAR );
        assert( SamplerDesc.MipFilter == FILTER_TYPE_POINT );
        assert( SamplerDesc.AddressU == TEXTURE_ADDRESS_WRAP );
        assert( SamplerDesc.AddressV == TEXTURE_ADDRESS_MIRROR );
        assert( SamplerDesc.AddressW == TEXTURE_ADDRESS_CLAMP );
        assert( SamplerDesc.MipLODBias == 2 );
        assert( SamplerDesc.MinLOD == 0.5f );
        assert( SamplerDesc.MaxLOD == 10 );
        assert( SamplerDesc.MaxAnisotropy == 6 );
        assert( SamplerDesc.ComparisonFunc == COMPARISON_FUNC_GREATER_EQUAL );
        assert( SamplerDesc.BorderColor[0] == 0.5f );
        assert( SamplerDesc.BorderColor[1] == 0.25f );
        assert( SamplerDesc.BorderColor[2] == 0.125f );
        assert( SamplerDesc.BorderColor[3] == 0.0625f );
    }

    {
        RefCntAutoPtr<ISampler> pTestSampler3;
        pScript->GetSamplerByName( "TestSampler3", &pTestSampler3 );
        const auto &SamplerDesc = pTestSampler3->GetDesc();
        assert( SamplerDesc.BorderColor[0] == 0.5f * 2.f);
        assert( SamplerDesc.BorderColor[1] == 0.25f * 2.f );
        assert( SamplerDesc.BorderColor[2] == 0.125f * 2.f );
        assert( SamplerDesc.BorderColor[3] == 0.0625f * 2.f );
    }

    {
        RefCntAutoPtr<ISampler> pTestSampler2;
        pScript->GetSamplerByName( "TestSampler2", &pTestSampler2 );
        const auto &SamplerDesc = pTestSampler2->GetDesc();
        assert( strcmp(SamplerDesc.Name, "Test Sampler") == 0);
        assert( SamplerDesc.MinFilter == FILTER_TYPE_POINT );
        assert( SamplerDesc.MagFilter == FILTER_TYPE_LINEAR );
        assert( SamplerDesc.MipFilter == FILTER_TYPE_POINT );
        assert( SamplerDesc.AddressU == TEXTURE_ADDRESS_WRAP );
        assert( SamplerDesc.AddressV == TEXTURE_ADDRESS_MIRROR );
        assert( SamplerDesc.AddressW == TEXTURE_ADDRESS_CLAMP );
        assert( SamplerDesc.MipLODBias == 2 );
        assert( SamplerDesc.MinLOD == 0.5f );
        assert( SamplerDesc.MaxLOD == 10 );
        assert( SamplerDesc.MaxAnisotropy == 6 );
        assert( SamplerDesc.ComparisonFunc == COMPARISON_FUNC_GREATER_EQUAL );
        assert( SamplerDesc.BorderColor[0] == 0.5f );
        assert( SamplerDesc.BorderColor[1] == 0.25f );
        assert( SamplerDesc.BorderColor[2] == 0.125f );
        assert( SamplerDesc.BorderColor[3] == 0.0625f );
    }


    {
        RefCntAutoPtr<ISampler> pTestSampler4;
        pScript->GetSamplerByName( "TestSampler4", &pTestSampler4 );
        const auto &SamDesc = pTestSampler4->GetDesc();
        assert( strcmp(SamDesc.Name, "") == 0 );
        assert( SamDesc.MinFilter == SamplerDesc().MinFilter );
        assert( SamDesc.MagFilter == SamplerDesc().MagFilter );
        assert( SamDesc.MipFilter == SamplerDesc().MipFilter );
        assert( SamDesc.AddressU == SamplerDesc().AddressU );
        assert( SamDesc.AddressV == SamplerDesc().AddressV );
        assert( SamDesc.AddressW == SamplerDesc().AddressW );
        assert( SamDesc.MipLODBias == SamplerDesc().MipLODBias );
        assert( SamDesc.MinLOD == SamplerDesc().MinLOD );
        assert( SamDesc.MaxLOD == SamplerDesc().MaxLOD );
        assert( SamDesc.MaxAnisotropy == SamplerDesc().MaxAnisotropy );
        assert( SamDesc.ComparisonFunc == SamplerDesc().ComparisonFunc );
        assert( SamDesc.BorderColor[0] == SamplerDesc().BorderColor[0] );
        assert( SamDesc.BorderColor[1] == SamplerDesc().BorderColor[1] );
        assert( SamDesc.BorderColor[2] == SamplerDesc().BorderColor[2] );
        assert( SamDesc.BorderColor[3] == SamplerDesc().BorderColor[3] );
    }
    
    for( int iVertLayout = 0; iVertLayout < 3; ++iVertLayout )
    {
        std::stringstream ss1;
        ss1 << "TestVertexLayoutPSO";
        if( iVertLayout > 0 )
        {
            ss1 << (iVertLayout + 1);
        }
        String VariableName = ss1.str();

        RefCntAutoPtr<IPipelineState> pTestPSO;
        pScript->GetPipelineStateByName( VariableName.c_str(), &pTestPSO );
        const auto &Desc = pTestPSO->GetDesc().GraphicsPipeline.InputLayout;
        assert( Desc.NumElements == 3 );
        assert( Desc.LayoutElements[0].InputIndex == 0 );
        assert( Desc.LayoutElements[0].BufferSlot == 0 );
        assert( Desc.LayoutElements[0].NumComponents == 3 );
        assert( Desc.LayoutElements[0].ValueType == VT_FLOAT32 );
        assert( Desc.LayoutElements[0].IsNormalized == false );

        assert( Desc.LayoutElements[1].InputIndex == 1 );
        assert( Desc.LayoutElements[1].BufferSlot == 1 );
        assert( Desc.LayoutElements[1].NumComponents == 4 );
        assert( Desc.LayoutElements[1].ValueType == VT_UINT8 );
        assert( Desc.LayoutElements[1].IsNormalized == true );

        assert( Desc.LayoutElements[2].InputIndex == 2 );
        assert( Desc.LayoutElements[2].BufferSlot == 2 );
        assert( Desc.LayoutElements[2].NumComponents == 2 );
        assert( Desc.LayoutElements[2].ValueType == VT_FLOAT32 );
        assert( Desc.LayoutElements[2].IsNormalized == false );
        assert( Desc.LayoutElements[2].Frequency == LayoutElement::FREQUENCY_PER_INSTANCE );
        assert( Desc.LayoutElements[2].InstanceDataStepRate == 1 );
    }

    {
        RefCntAutoPtr<IShader> pTestVS;
        pScript->GetShaderByName( "TestVS", &pTestVS );
        const auto &Desc = pTestVS->GetDesc();
        assert( strcmp(Desc.Name, "TestVS") == 0 );
        assert( Desc.ShaderType == SHADER_TYPE_VERTEX );
    }

    {
        RefCntAutoPtr<IShader> pTestPS;
        pScript->GetShaderByName( "TestPS", &pTestPS );
        const auto &Desc = pTestPS->GetDesc();
        assert( strcmp(Desc.Name, "TestPS") == 0 );
        assert( Desc.ShaderType == SHADER_TYPE_PIXEL );
    }

    {
        RefCntAutoPtr<IShader> pTestPS2;
        pScript->GetShaderByName( "TestPS2", &pTestPS2 );
        const auto &Desc = pTestPS2->GetDesc();
        assert( strcmp(Desc.Name, "TestPS2") == 0 );
        assert( Desc.ShaderType == SHADER_TYPE_PIXEL );
    }

    Int32 MagicNumber1 = 123;
    float MagicNumber2 = 345.5f;
    String MagicString = "Magic String";
    pScript->Run( "TestRenderScriptParams", True, MagicNumber1, MagicNumber2, False, MagicString );

    {
        RefCntAutoPtr<ISampler> pTestSampler;
        pScript->GetSamplerByName( "TestSampler", &pTestSampler );
        pScript->Run( "TestSamplerArg", pTestSampler );
    }

    {
        RefCntAutoPtr<IPipelineState> pTestPSO;
        pScript->GetPipelineStateByName( "TestVertexLayoutPSO", &pTestPSO );
        pScript->Run( "TestVertexDescInPSO", pTestPSO );
    }

    {
        RefCntAutoPtr<IShader> pTestVS;
        pScript->GetShaderByName( "TestVS", &pTestVS );
        pScript->Run( "TestShaderArg", pTestVS );
    }

    {
        RefCntAutoPtr<IBuffer> pTestBuffer;
        pScript->GetBufferByName( "TestBuffer", &pTestBuffer );
        const auto &Desc = pTestBuffer->GetDesc();
        assert( strcmp(Desc.Name, "Test Buffer") == 0 );
        assert( Desc.Usage == USAGE_DEFAULT );
        assert( Desc.BindFlags == (BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE) );

        pScript->Run( "TestBufferArg", pTestBuffer );
    }

    {
        pScript->Run( "TestTextureArg", pTestGlobalTexture );
    }

    {
        DrawAttribs DrawAttribs;
        DrawAttribs.Topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        DrawAttribs.NumVertices = 34;
        DrawAttribs.IndexType = VT_UINT16;
        DrawAttribs.IsIndexed = True;
        DrawAttribs.NumInstances = 139;
        DrawAttribs.IsIndirect = True;
        DrawAttribs.BaseVertex = 937;
        DrawAttribs.IndirectDrawArgsOffset = 1205;
        DrawAttribs.StartVertexLocation = 198;
        pScript->Run( "TestDrawAttribsArg", DrawAttribs );
    }

    {
        RefCntAutoPtr<ITextureView> pDefaultTestTextureSRV;
        pScript->GetTextureViewByName( "DefaultTestSRV", &pDefaultTestTextureSRV );
        assert( pDefaultTestTextureSRV  );
        RefCntAutoPtr<ITexture> pTestTestTexture;
        pScript->GetTextureByName( "TestTexture", &pTestTestTexture );
        assert(pTestTestTexture );
        assert( pDefaultTestTextureSRV == pTestTestTexture->GetDefaultView( TEXTURE_VIEW_SHADER_RESOURCE ) );
    }

    auto DevType = pRenderDevice->GetDeviceCaps().DevType;
    if( DevType == DeviceType::D3D11 || DevType == DeviceType::D3D12 )
    {
        {
            RefCntAutoPtr<ITextureView> pTestTestTextureView;
            pScript->GetTextureViewByName( "TestTextureView", &pTestTestTextureView );
            const auto &Desc = pTestTestTextureView->GetDesc();
            assert( strcmp(Desc.Name, "TestTextureSRV") == 0 );
            assert( Desc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE );
            assert( Desc.TextureDim == RESOURCE_DIM_TEX_2D_ARRAY );
            assert( Desc.Format == TEX_FORMAT_RGBA8_UNORM );
            assert( Desc.MostDetailedMip == 1 );
            assert( Desc.NumMipLevels == 2 );
            assert( Desc.FirstArraySlice == 3 );
            assert( Desc.NumArraySlices == 4 );

            pScript->Run( "TestTextureViewArg", pTestTestTextureView );
        }
    }

    {
        RefCntAutoPtr<IResourceMapping> pResMapping;
        pScript->GetResourceMappingByName( "TestResourceMapping", &pResMapping );
        RefCntAutoPtr<IDeviceObject> pSRV, pBuff;
        pResMapping->GetResource( "TestShaderName", &pSRV );
        assert( pSRV != nullptr );
        pResMapping->GetResource( "TestBufferName", &pBuff );
        assert( pBuff != nullptr );
    }

    {
        {
            ResourceMappingEntry Entries[] = {
                    { "TestGlobalTextureSRV2", pTestGlobalTexture->GetDefaultView( TEXTURE_VIEW_SHADER_RESOURCE ) },
                    ResourceMappingEntry()
            };
            ResourceMappingDesc ResMappingDesc;
            ResMappingDesc.pEntries = Entries;
            RefCntAutoPtr<IResourceMapping> pResMapping;
            pRenderDevice->CreateResourceMapping( ResMappingDesc, &pResMapping );
            pScript->Run( "TestResourceMappingArg", pResMapping );
            RefCntAutoPtr<IDeviceObject> pRes;
            pResMapping->GetResource( "TestBufferName", &pRes );
            assert( pRes != nullptr );
        }
    }

    {
        RefCntAutoPtr<IBuffer> pBuffer;
        pScript->GetBufferByName( "TestGlobalBufferWithUAV", &pBuffer );
        RefCntAutoPtr<IBufferView> pBuffUAV;
        BufferViewDesc BuffViewDesc;
        BuffViewDesc.Name = "TestGlobalBuff2UAV";
        BuffViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
        BuffViewDesc.ByteOffset = 0;
        BuffViewDesc.ByteWidth = 32;
        pBuffer->CreateView(BuffViewDesc, &pBuffUAV);

        pScript->Run( "TestBufferViewArg", pBuffUAV );
    }

    {
        RefCntAutoPtr<IShaderVariable> pShaderVar;
        pScript->GetShaderVariableByName( "svTestBlock", &pShaderVar );
        assert( pShaderVar );

        pScript->SetGlobalVariable( "svTestBlock", pShaderVar );
        pScript->Run( "TestShaderVariable", pShaderVar );
    }
    SetStatus(TestResult::Succeeded);
}
