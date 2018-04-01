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
#include "TestBufferAccess.h"
#include "MapHelper.h"
#include "BasicShaderSourceStreamFactory.h"

using namespace Diligent;

TestBufferAccess::TestBufferAccess() :
    UnitTestBase("Buffer access test"),
    m_fXExtent(0), 
    m_fYExtent(0)
{}

void TestBufferAccess::Init( IRenderDevice *pDevice, IDeviceContext *pContext, float fMinXCoord, float fMinYCoord, float fXExtent, float fYExtent )
{
    m_pRenderDevice = pDevice;
    m_pDeviceContext = pContext;
    auto DevType = m_pRenderDevice->GetDeviceCaps().DevType;
    bool bUseOpenGL = DevType == DeviceType::OpenGL || DevType == DeviceType::OpenGLES;

    m_fXExtent = fXExtent;
    m_fYExtent = fYExtent;

    float vertices[] = {	0.0f,0.0f,0.0f, 0.5f, 0.9f, 0.1f,
						    0.5f,1.f,0.0f,  0.9f, 0.3f, 0.7f,
						    1.f,0.0f,0.0f,  0.2f, 0.4f, 0.9f };
    for(int iVert=0; iVert < 3; ++iVert)
    {
        vertices[iVert*6]   = (vertices[iVert*6])   / (float)(NumRows+1) * fXExtent + fMinXCoord;
        vertices[iVert*6+1] = (vertices[iVert*6+1]) / (float)(NumRows+1) * fYExtent + fMinYCoord;
    }

    {
        Diligent::BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = sizeof(vertices);
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        Diligent::BufferData BuffData;
        BuffData.pData = vertices;
        BuffData.DataSize = BuffDesc.uiSizeInBytes;
        m_pRenderDevice->CreateBuffer(BuffDesc, BuffData, &m_pVertexBuff);
    }

    for(int InstBuff = 0; InstBuff < _countof(m_pInstBuff); ++InstBuff)
    {
        float instance_offsets[NumInstances*2] = 
        {
            1.f * fXExtent / (float)(NumRows+1), InstBuff*fYExtent / (float)(NumRows+1), 
            2.f * fXExtent / (float)(NumRows+1), InstBuff*fYExtent / (float)(NumRows+1), 
            3.f * fXExtent / (float)(NumRows+1), InstBuff*fYExtent / (float)(NumRows+1), 
        };

        Diligent::BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = sizeof(instance_offsets);
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        if( InstBuff == 3 )
        {
            BuffDesc.Usage = USAGE_DYNAMIC;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        }
        else if( InstBuff == 4 )
        {
            BuffDesc.Usage = USAGE_CPU_ACCESSIBLE;
            BuffDesc.BindFlags = BIND_NONE;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_READ;
        }
        else if( InstBuff == 5 )
        {
            BuffDesc.Usage = USAGE_CPU_ACCESSIBLE;
            BuffDesc.BindFlags = BIND_NONE;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        }
        else if( InstBuff == 6 )
        {
            BuffDesc.Usage = USAGE_CPU_ACCESSIBLE;
            BuffDesc.BindFlags = BIND_NONE;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_READ;
        }

        Diligent::BufferData BuffData;
        if(BuffDesc.Usage != USAGE_DYNAMIC && !(BuffDesc.Usage == USAGE_CPU_ACCESSIBLE && (BuffDesc.CPUAccessFlags & CPU_ACCESS_WRITE) != 0))
        {
            BuffData.pData = instance_offsets;
            BuffData.DataSize = sizeof(instance_offsets);
        }
        m_pRenderDevice->CreateBuffer(BuffDesc, BuffData, &m_pInstBuff[InstBuff]);
    }

    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory;
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.Desc.TargetProfile = bUseOpenGL ? SHADER_PROFILE_GL_4_2 : SHADER_PROFILE_DX_5_0;

    RefCntAutoPtr<Diligent::IShader> pVSInst, pPS;

    {
        CreationAttrs.FilePath = bUseOpenGL ? "Shaders\\minimalInstGL.vsh" : "Shaders\\minimalInstDX.vsh";
        CreationAttrs.Desc.ShaderType =  SHADER_TYPE_VERTEX;
        m_pRenderDevice->CreateShader( CreationAttrs, &pVSInst );
    }

    {
        CreationAttrs.FilePath = bUseOpenGL ? "Shaders\\minimalGL.psh" : "Shaders\\minimalDX.psh";
        CreationAttrs.Desc.ShaderType =  SHADER_TYPE_PIXEL;
        m_pRenderDevice->CreateShader( CreationAttrs, &pPS );
    }


    PipelineStateDesc PSODesc;
    PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    PSODesc.GraphicsPipeline.BlendDesc.IndependentBlendEnable = False;
    PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = False;
    PSODesc.GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM_SRGB;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.pVS = pVSInst;
    PSODesc.GraphicsPipeline.pPS = pPS;

    LayoutElement Elems[] =
    {
        LayoutElement( 0, 0, 3, Diligent::VT_FLOAT32, false, 0 ),
        LayoutElement( 1, 0, 3, Diligent::VT_FLOAT32, false, sizeof( float ) * 3 ),
        LayoutElement( 2, 1, 2, Diligent::VT_FLOAT32, false, 0, LayoutElement::FREQUENCY_PER_INSTANCE )
    };
    PSODesc.GraphicsPipeline.InputLayout.LayoutElements = Elems;
    PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof( Elems );
    pDevice->CreatePipelineState(PSODesc, &m_pPSO);
}
    
void TestBufferAccess::Draw(float fTime)
{
    m_pDeviceContext->SetPipelineState(m_pPSO);
    // No shader resources needed
    //m_pDeviceContext->TransitionShaderResources(m_pPSO, nullptr);
    //m_pDeviceContext->CommitShaderResources(nullptr);

    IBuffer *pBuffs[2] = {m_pVertexBuff, m_pInstBuff[0]};
    Uint32 Strides[] = {sizeof(float)*6, sizeof(float)*2};
    Uint32 Offsets[] = {0, 0};
    m_pDeviceContext->SetVertexBuffers( 0, _countof( pBuffs ), pBuffs, Strides, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET );
    
    Diligent::DrawAttribs DrawAttrs;
    DrawAttrs.Topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    DrawAttrs.NumVertices = 3;
    DrawAttrs.NumInstances = NumInstances;
    m_pDeviceContext->Draw(DrawAttrs);
    

    float fDX = m_fXExtent / (float)(NumRows+1);
    float fDY = m_fYExtent / (float)(NumRows+1);

    float instance_offsets[NumInstances*2];
    for(int Inst = 0; Inst < NumInstances; ++Inst)
    {
        instance_offsets[Inst*2] = (1+Inst) * fDX;
        instance_offsets[Inst*2+1] = 1.f * fDY + sin(fTime) * fDY * 0.3f;
    }
    m_pInstBuff[1]->UpdateData( m_pDeviceContext, sizeof( float ) * 2, sizeof( float ) * 4, &instance_offsets[2] );

    pBuffs[1] = m_pInstBuff[1];
    m_pDeviceContext->SetVertexBuffers( 0, _countof( pBuffs ), pBuffs, Strides, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET );
    
    m_pDeviceContext->Draw(DrawAttrs);


    for(int Inst = 0; Inst < NumInstances; ++Inst)
    {
        instance_offsets[Inst*2] = (1+Inst) * fDX;
        instance_offsets[Inst*2+1] = 2.f * fDY + sin(fTime*0.8f) * fDY * 0.3f;
    }
    m_pInstBuff[2]->UpdateData( m_pDeviceContext, sizeof( float ) * 2, sizeof( float ) * 4, &instance_offsets[2] );
    m_pInstBuff[1]->CopyData( m_pDeviceContext, m_pInstBuff[2], sizeof( float ) * 2, sizeof( float ) * 2, sizeof( float ) * 4 );
    
    m_pDeviceContext->Draw(DrawAttrs);

    for(int Inst = 0; Inst < NumInstances; ++Inst)
    {
        instance_offsets[Inst*2] = (1+Inst) * fDX;
        instance_offsets[Inst*2+1] = 3.f * fDY + sin(fTime*1.2f) * fDY * 0.3f;
    }

    // Test updating dynamic buffer
    {
        MapHelper<float> pInstData( m_pDeviceContext, m_pInstBuff[3], MAP_WRITE, MAP_FLAG_DISCARD );
        memcpy(pInstData, instance_offsets, sizeof(instance_offsets));
    }

    pBuffs[1] = m_pInstBuff[3];
    m_pDeviceContext->SetVertexBuffers( 0, _countof( pBuffs ), pBuffs, Strides, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET );
    
    m_pDeviceContext->Draw(DrawAttrs);

    bool TestStagingBuffers = false;
    if(TestStagingBuffers)
    {
        MapHelper<float> pStagingData;
        // Test reading data from staging resource
        {
            m_pInstBuff[4]->CopyData( m_pDeviceContext, m_pInstBuff[3], 0, 0, sizeof( instance_offsets ) );
            pStagingData.Map( m_pDeviceContext, m_pInstBuff[4], MAP_READ, 0 );
            for(int i = 0; i < _countof(instance_offsets); ++i)
                assert(pStagingData[i] == instance_offsets[i]);
            pStagingData.Unmap();
        }

        // D3D12 does not allow writing to the CPU-readable buffers
        if(m_pRenderDevice->GetDeviceCaps().DevType != DeviceType::D3D12)
        {
            // Test writing data to staging resource
            {
                pStagingData.Map( m_pDeviceContext, m_pInstBuff[5], MAP_WRITE, 0 );
                for(int Inst = 0; Inst < NumInstances; ++Inst)
                {
                    pStagingData[Inst*2] = (1+Inst) * fDX;
                    pStagingData[Inst*2+1] = 4.f * fDY + sin(fTime*1.3f) * fDY * 0.3f;
                }
                pStagingData.Unmap();
            }

            m_pInstBuff[2]->CopyData( m_pDeviceContext, m_pInstBuff[5], 0, 0, sizeof( instance_offsets ) );
            pBuffs[1] = m_pInstBuff[2];
            m_pDeviceContext->SetVertexBuffers( 0, _countof( pBuffs ), pBuffs, Strides, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET );
            m_pDeviceContext->Draw(DrawAttrs);


            // Test reading & writing data to the staging resource
            /*{
                MapHelper<float> pInstData2( m_pDeviceContext, m_pInstBuff[6], MAP_READ_WRITE, 0 );
                MapHelper<float> pInstData3( std::move(pInstData2) );
                MapHelper<float> pInstData;
                pInstData = std::move(pInstData3);
                static float fPrevTime = fTime;
                for(int Inst = 0; Inst < NumInstances; ++Inst)
                {
                    pInstData[Inst*2] += (fTime-fPrevTime) * sin(fTime)*0.1f * m_fXExtent;
                }
                fPrevTime = fTime;
            }*/

            m_pInstBuff[2]->CopyData( m_pDeviceContext, m_pInstBuff[6], 0, 0, sizeof( instance_offsets ) );
            m_pDeviceContext->Draw(DrawAttrs);
        }
    }
    
    SetStatus(TestResult::Succeeded);
}
