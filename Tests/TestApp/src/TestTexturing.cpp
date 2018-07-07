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
#include <math.h>
#include "TestTexturing.h"
#include "GraphicsUtilities.h"
#include "BasicShaderSourceStreamFactory.h"

using namespace Diligent;

TestTexturing::TestTexturing() :
    UnitTestBase("Texturing test"),
    m_iTestTexWidth(512),
    m_iTestTexHeight(512),
    m_iMipLevels(8),
    m_TextureFormat(TEX_FORMAT_UNKNOWN)
{
}

void TestTexturing::GenerateTextureData(IRenderDevice *pRenderDevice, std::vector<Uint8> &Data, std::vector<TextureSubResData> &SubResouces, const TextureDesc &TexDesc, const float *ColorOffset)
{
    Data.clear();
    Uint32 CurrLevelOffset = 0;
    std::vector<Uint32> LevelDataOffsets(TexDesc.MipLevels);
    SubResouces.resize(TexDesc.MipLevels);
 
    auto PixelFormatAttribs = pRenderDevice->GetTextureFormatInfoExt(TexDesc.Format);
    auto PixelSize = PixelFormatAttribs.ComponentSize * PixelFormatAttribs.NumComponents;
    
    for(Uint32 Level = 0; Level < TexDesc.MipLevels; ++Level)
    {
        Uint32 MipWidth  = TexDesc.Width  >> Level;
        Uint32 MipHeight = TexDesc.Height >> Level;
        auto Stride = (MipWidth + 64) * PixelSize;

        Data.resize(Data.size() + Stride * MipHeight);
        auto *pCurrLevelPtr = &Data[CurrLevelOffset];
        LevelDataOffsets[Level] = CurrLevelOffset;
        SubResouces[Level].Stride = Stride;
        for(Uint32 j=0; j<MipHeight; ++j)
            for(Uint32 i=0; i<MipWidth; ++i)
            {
                float Color[4] = 
                {
                    (float)i/(float)MipWidth * 1.5f + (ColorOffset ? ColorOffset[0] : 0.f),
                    (float)j/(float)MipHeight * 1.7f+ (ColorOffset ? ColorOffset[1] : 0.f),
                    (float)j/(float)MipHeight / 1.3f + (float)i/(float)MipWidth/1.1f + (ColorOffset ? ColorOffset[2] : 0.f),
                    1.f + (ColorOffset ? ColorOffset[3] : 0.f) 
                };
                for(Uint32 iCmp = 0; iCmp < PixelFormatAttribs.NumComponents; ++iCmp)
                {
                    float fCurrCmpCol = Color[iCmp];
                    fCurrCmpCol = fCurrCmpCol - floor(fCurrCmpCol);
                    void *pDstCmp = pCurrLevelPtr + (i*PixelSize + iCmp * PixelFormatAttribs.ComponentSize + j*Stride);
                    switch(PixelFormatAttribs.ComponentType)
                    {
                        case COMPONENT_TYPE_FLOAT:
                            *((float*)pDstCmp) = fCurrCmpCol;
                        break;

                        case COMPONENT_TYPE_SNORM:
                            if( PixelFormatAttribs.ComponentSize == 1 )
                                *((Int8*)pDstCmp) = (Int8) std::min( std::max(fCurrCmpCol*127.f, -127.f), 127.f );
                            else if( PixelFormatAttribs.ComponentSize == 2 )
                                *((Int16*)pDstCmp) = (Int16) std::min( std::max(fCurrCmpCol*32767.f, -32767.f), 32767.f );
                            else
                                assert(false);
                        break;

                        case COMPONENT_TYPE_UNORM_SRGB:
                        case COMPONENT_TYPE_UNORM:
                            if( PixelFormatAttribs.ComponentSize == 1 )
                                *((Uint8*)pDstCmp) = (Uint8) std::min( std::max(fCurrCmpCol*255,0.f), 255.f );
                            else if( PixelFormatAttribs.ComponentSize == 2 )
                                *((Uint16*)pDstCmp) = (Uint16) std::min( std::max(fCurrCmpCol*65535.f,0.f), 65535.f );
                            else
                                assert(false);
                        break;

                        case COMPONENT_TYPE_SINT:
                            if( PixelFormatAttribs.ComponentSize == 1 )
                                *((Int8*)pDstCmp) = (Int8) std::min( std::max(fCurrCmpCol*127.f, -127.f), 127.f );
                            else if( PixelFormatAttribs.ComponentSize == 2 )
                                *((Int16*)pDstCmp) = (Int16) std::min( std::max(fCurrCmpCol*127.f, -127.f), 127.f );
                            else
                                assert(false);
                        break;

                        case COMPONENT_TYPE_UINT:
                            if( PixelFormatAttribs.ComponentSize == 1 )
                                *((Uint8*)pDstCmp) = (Uint8) std::min( std::max(fCurrCmpCol*255,0.f), 255.f );
                            else if( PixelFormatAttribs.ComponentSize == 2 )
                                *((Uint16*)pDstCmp) = (Uint16) std::min( std::max(fCurrCmpCol*255,0.f), 255.f );
                            else
                                assert(false);
                        break;

                        default: assert("Unsupport component type" && false);
                    }
                }
            }
        CurrLevelOffset += Stride * MipHeight;
    }
    for(Uint32 Level = 0; Level < TexDesc.MipLevels; ++Level)
    {
        SubResouces[Level].pData = Data.data() + LevelDataOffsets[Level];
    }
}


void TestTexturing::Init( IRenderDevice *pDevice, IDeviceContext *pDeviceContext, ISwapChain *pSwapChain, TEXTURE_FORMAT TexFormat, float fMinXCoord, float fMinYCoord, float fXExtent, float fYExtent )
{
    m_pRenderDevice = pDevice;
    m_TextureFormat = TexFormat;
    m_pDeviceContext = pDeviceContext;
    auto DevType = m_pRenderDevice->GetDeviceCaps().DevType;
    bool bUseGLSL = DevType == DeviceType::OpenGL || DevType == DeviceType::OpenGLES || DevType == DeviceType::Vulkan;

    float Vertices[] = 
    {
         0,  0, 0,   0,1,
         0,  1, 0,   0,0,
         1,  0, 0,   1,1,
         1,  1, 0,   1,0
    };
    for(int v=0; v < 4; ++v)
    {
        Vertices[v*5+0] = Vertices[v*5+0] * fXExtent + fMinXCoord;
        Vertices[v*5+1] = Vertices[v*5+1] * fYExtent + fMinYCoord;
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = sizeof(Vertices);
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_STATIC;
        Diligent::BufferData BuffData;
        BuffData.pData = Vertices;
        BuffData.DataSize = BuffDesc.uiSizeInBytes;
        m_pRenderDevice->CreateBuffer(BuffDesc, BuffData, &m_pVertexBuff);
    }
    
    auto PixelFormatAttribs = m_pRenderDevice->GetTextureFormatInfoExt(m_TextureFormat);

    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory;
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.Desc.TargetProfile = bUseGLSL ? SHADER_PROFILE_GL_4_2 : SHADER_PROFILE_DX_5_0;

    RefCntAutoPtr<Diligent::IShader> pVS, pPS;
    {
        CreationAttrs.FilePath = bUseGLSL ? "Shaders\\TextureTestGL.vsh" : "Shaders\\TextureTestDX.vsh";
        CreationAttrs.Desc.ShaderType =  SHADER_TYPE_VERTEX;
        m_pRenderDevice->CreateShader( CreationAttrs, &pVS );
    }

    bool bIsIntTexture = PixelFormatAttribs.ComponentType == COMPONENT_TYPE_UINT ||
                         PixelFormatAttribs.ComponentType == COMPONENT_TYPE_SINT;
    {
        if( bIsIntTexture )
            CreationAttrs.FilePath = bUseGLSL ? "Shaders\\TextureIntTestGL.psh" : "Shaders\\TextureIntTestDX.psh";
        else
            CreationAttrs.FilePath = bUseGLSL ? "Shaders\\TextureTestGL.psh" : "Shaders\\TextureTestDX.psh";
        CreationAttrs.Desc.ShaderType =  SHADER_TYPE_PIXEL;
        
        StaticSamplerDesc StaticSampler;
        // On Intel HW, only point filtering sampler correctly works with an integer texture.
        // If the sampler defines linear filtering, the texture is not properly bound to the
        // sampler unit and zero is always returned.
        // Note that on NVidia HW this works fine.
        auto FilterType = bIsIntTexture ? FILTER_TYPE_POINT : FILTER_TYPE_LINEAR;
        StaticSampler.Desc.MinFilter = FilterType;
        StaticSampler.Desc.MagFilter = FilterType;
        StaticSampler.Desc.MipFilter = FilterType;
        StaticSampler.TextureName = "g_tex2DTest";
        CreationAttrs.Desc.NumStaticSamplers = 1;
        CreationAttrs.Desc.StaticSamplers = &StaticSampler;
        m_pRenderDevice->CreateShader( CreationAttrs, &pPS );
    }

    {
        SamplerDesc SamplerDesc;
        // On Intel HW, only point filtering sampler correctly works with an integer texture.
        // If the sampler defines linear filtering, the texture is not properly bound to the
        // sampler unit and zero is always returned.
        // Note that on NVidia HW this works fine.
        auto FilterType = bIsIntTexture ? FILTER_TYPE_POINT : FILTER_TYPE_LINEAR;
        SamplerDesc.MinFilter = FilterType;
        SamplerDesc.MagFilter = FilterType;
        SamplerDesc.MipFilter = FilterType;
        m_pRenderDevice->CreateSampler( SamplerDesc, &m_pSampler );
    }
    
    {
        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Width = m_iTestTexWidth;
        TexDesc.Height = m_iTestTexHeight;
        TexDesc.MipLevels = m_iMipLevels;
        TexDesc.Usage = USAGE_STATIC;
        TexDesc.Format = m_TextureFormat;
        TexDesc.BindFlags = BIND_SHADER_RESOURCE;
        TexDesc.Name = "Test Texture";

        std::vector<Uint8> Data;
        std::vector<TextureSubResData> SubResouces;
        GenerateTextureData(m_pRenderDevice, Data, SubResouces, TexDesc);
        TextureData TexData;
        TexData.pSubResources = SubResouces.data();
        TexData.NumSubresources = (Uint32)SubResouces.size();

        m_pRenderDevice->CreateTexture( TexDesc, TexData, &m_pTexture );
    }
    
    {
        RefCntAutoPtr<ITextureView> pDefaultSRV;
        TextureViewDesc ViewDesc;
        ViewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
        m_pTexture->CreateView( ViewDesc, &pDefaultSRV );
        pDefaultSRV->SetSampler( m_pSampler );
        ResourceMappingEntry Entries[] = { { "g_tex2DTest", pDefaultSRV }, {nullptr, nullptr} };
        ResourceMappingDesc ResourceMapping;
        ResourceMapping.pEntries = Entries;
        m_pRenderDevice->CreateResourceMapping( ResourceMapping, &m_pResourceMapping );
    }
    PipelineStateDesc PSODesc;
    PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    PSODesc.GraphicsPipeline.BlendDesc.IndependentBlendEnable = False;
    PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = False;
    PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.pVS = pVS;
    PSODesc.GraphicsPipeline.pPS = pPS;
    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    LayoutElement Elems[] =
    {
        LayoutElement( 0, 0, 3, Diligent::VT_FLOAT32, false, 0 ),
        LayoutElement( 1, 0, 2, Diligent::VT_FLOAT32, false, sizeof( float ) * 3 )
    };
    PSODesc.GraphicsPipeline.InputLayout.LayoutElements = Elems;
    PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof( Elems );
    pDevice->CreatePipelineState(PSODesc, &m_pPSO);

    pVS->BindResources(m_pResourceMapping, 0);
    pPS->BindResources(m_pResourceMapping, 0);
    
    auto *FmtName = pDevice->GetTextureFormatInfo(TexFormat).Name;
    m_TestName.append(" (");
    m_TestName.append(FmtName);
    m_TestName.append(")");
}
    
void TestTexturing::Draw()
{
    m_pDeviceContext->SetPipelineState(m_pPSO);
    m_pDeviceContext->TransitionShaderResources(m_pPSO, nullptr);
    m_pDeviceContext->CommitShaderResources(nullptr, COMMIT_SHADER_RESOURCES_FLAG_VERIFY_STATES);
    
    IBuffer *pBuffs[] = {m_pVertexBuff};
    Uint32 Offsets[] = {0};
    m_pDeviceContext->SetVertexBuffers( 0, 1, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET );

    Diligent::DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 4; // Draw quad
    m_pDeviceContext->Draw( DrawAttrs );
    
    SetStatus(TestResult::Succeeded);
}
