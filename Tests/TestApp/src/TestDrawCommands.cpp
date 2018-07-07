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
#include "TestDrawCommands.h"
#include "MapHelper.h"
#include "BasicShaderSourceStreamFactory.h"

using namespace Diligent;

void TestDrawCommands::Init( IRenderDevice *pDevice, IDeviceContext *pDeviceContext, ISwapChain *pSwapChain, float fMinXCoord, float fMinYCoord, float fXExtent, float fYExtent )
{
    m_pRenderDevice = pDevice;
    m_pDeviceContext = pDeviceContext;

    auto DevType = m_pRenderDevice->GetDeviceCaps().DevType;
    bool bUseGLSL = DevType == DeviceType::OpenGL || DevType == DeviceType::OpenGLES || DevType == DeviceType::Vulkan;

    std::vector<float> VertexData;
    std::vector<float> VertexData2;
    std::vector<Uint32> IndexData;
    std::vector<float> InstanceData;
    Uint32 Ind = 0;
    for( int iRow = 0; iRow < TriGridSize; ++iRow )
        for( int iCol = 0; iCol < TriGridSize; ++iCol )
        {
            float fTriCenterX = (((float)iCol + 0.5f) / (float)TriGridSize) * fXExtent + fMinXCoord;
            float fTriCenterY = (((float)iRow + 0.5f) / (float)TriGridSize) * fYExtent + fMinYCoord;
            float fTriSizeX = fXExtent / (float)TriGridSize * 0.9f;
            float fTriSizeY = fYExtent / (float)TriGridSize * 0.9f;
            Define2DVertex( VertexData, fTriCenterX - 0.5f*fTriSizeX, fTriCenterY - 0.5f*fTriSizeY, 1, 0, 0 );
            Define2DVertex( VertexData, fTriCenterX + 0.5f*fTriSizeX, fTriCenterY - 0.5f*fTriSizeY, 0, 1, 0 );
            Define2DVertex( VertexData, fTriCenterX + 0.0f*fTriSizeX, fTriCenterY + 0.5f*fTriSizeY, 0, 0, 1 );

            Define2DVertex( VertexData2, fTriCenterX - 0.5f*fTriSizeX, fTriCenterY - 0.5f*fTriSizeY, 1, 1, 0 );
            Define2DVertex( VertexData2, fTriCenterX + 0.5f*fTriSizeX, fTriCenterY - 0.5f*fTriSizeY, 0, 1, 1 );
            Define2DVertex( VertexData2, fTriCenterX + 0.0f*fTriSizeX, fTriCenterY + 0.5f*fTriSizeY, 1, 0, 1 );

            InstanceData.push_back( (float)iCol / (float)TriGridSize * fXExtent );
            InstanceData.push_back( (float)iRow / (float)TriGridSize * fYExtent );

            IndexData.push_back( Ind++ );
            IndexData.push_back( Ind++ );
            IndexData.push_back( Ind++ );
        }

    {
        BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = (Uint32)VertexData.size()*sizeof( float );
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_STATIC;
        BufferData BuffData;
        BuffData.pData = VertexData.data();
        BuffData.DataSize = (Uint32)VertexData.size()*sizeof( float );
        m_pRenderDevice->CreateBuffer( BuffDesc, BuffData, &m_pVertexBuff );
    }

    {
        BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = (Uint32)VertexData2.size()*sizeof( float );
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_STATIC;
        BufferData BuffData;
        BuffData.pData = VertexData2.data();
        BuffData.DataSize = (Uint32)VertexData2.size()*sizeof( float );
        m_pRenderDevice->CreateBuffer( BuffDesc, BuffData, &m_pVertexBuff2 );
    }

    {
        BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = (Uint32)IndexData.size() * sizeof( Uint32 );
        BuffDesc.BindFlags = BIND_INDEX_BUFFER;
        BuffDesc.Usage = USAGE_STATIC;
        BufferData BuffData;
        BuffData.pData = IndexData.data();
        BuffData.DataSize = BuffDesc.uiSizeInBytes;
        m_pRenderDevice->CreateBuffer( BuffDesc, BuffData, &m_pIndexBuff );
    }

    {
        BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = (Uint32)InstanceData.size() * sizeof( float );
        BuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_STATIC;
        BufferData BuffData;
        BuffData.pData = InstanceData.data();
        BuffData.DataSize = BuffDesc.uiSizeInBytes;
        m_pRenderDevice->CreateBuffer( BuffDesc, BuffData, &m_pInstanceData );
    }

    if( m_pRenderDevice->GetDeviceCaps().bIndirectRenderingSupported )
    {
        //typedef  struct {
        //   GLuint  count;
        //   GLuint  instanceCount;
        //   GLuint  first;
        //   GLuint  baseInstance;
        //} DrawArraysIndirectCommand;

        Uint32 IndirectDrawArgs[] = { 3, 2, 0, 0 };
        BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = sizeof( IndirectDrawArgs );
        // A buffer cannot be created if no bind flags set. We thus have to set this dummy BIND_VERTEX_BUFFER flag
        // to be able to create the buffer
        BuffDesc.BindFlags = BIND_INDIRECT_DRAW_ARGS | BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pRenderDevice->CreateBuffer( BuffDesc, BufferData(), &m_pIndirectDrawArgs );
    }

    {
        //typedef  struct {
        //    GLuint  count;
        //    GLuint  instanceCount;
        //    GLuint  firstIndex;
        //    GLuint  baseVertex;
        //    GLuint  baseInstance;
        //} DrawElementsIndirectCommand;

        Uint32 IndirectDrawArgs[] = { 3, 2, 0, 0, 0 };
        BufferDesc BuffDesc;
        BuffDesc.uiSizeInBytes = sizeof( IndirectDrawArgs );
        // A buffer cannot be created if no bind flags set. We thus have to set this dummy flag
        // to be able to create the buffer
        BuffDesc.BindFlags = BIND_INDIRECT_DRAW_ARGS | BIND_VERTEX_BUFFER;
        BuffDesc.Usage = USAGE_DYNAMIC;
        BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pRenderDevice->CreateBuffer( BuffDesc, BufferData(), &m_pIndexedIndirectDrawArgs );
    }


    ShaderCreationAttribs CreationAttrs;
    BasicShaderSourceStreamFactory BasicSSSFactory;
    CreationAttrs.pShaderSourceStreamFactory = &BasicSSSFactory;
    CreationAttrs.Desc.TargetProfile = bUseGLSL ? SHADER_PROFILE_GL_4_2 : SHADER_PROFILE_DX_5_0;

    RefCntAutoPtr<IShader> pVS, pVSInst, pPS;
    {
        CreationAttrs.FilePath = bUseGLSL ? "Shaders\\minimalGL.vsh" : "Shaders\\minimalDX.vsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        m_pRenderDevice->CreateShader( CreationAttrs, &pVS );
    }

    {
        CreationAttrs.FilePath = bUseGLSL ? "Shaders\\minimalInstGL.vsh" : "Shaders\\minimalInstDX.vsh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_VERTEX;
        m_pRenderDevice->CreateShader( CreationAttrs, &pVSInst );
    }

    {
        CreationAttrs.FilePath = bUseGLSL ? "Shaders\\minimalGL.psh" : "Shaders\\minimalDX.psh";
        CreationAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        m_pRenderDevice->CreateShader( CreationAttrs, &pPS );
    }

    PipelineStateDesc PSODesc;
    PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    PSODesc.GraphicsPipeline.BlendDesc.IndependentBlendEnable = False;
    PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = False;
    PSODesc.GraphicsPipeline.NumRenderTargets = 1;
    PSODesc.GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    PSODesc.GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    PSODesc.GraphicsPipeline.pPS = pPS;

    BlendStateDesc &BSDesc = PSODesc.GraphicsPipeline.BlendDesc;
    BSDesc.IndependentBlendEnable = False;
    BSDesc.RenderTargets[0].BlendEnable = True;
    BSDesc.RenderTargets[0].SrcBlend = BLEND_FACTOR_ONE;
    BSDesc.RenderTargets[0].DestBlend = BLEND_FACTOR_ONE;
    BSDesc.RenderTargets[0].BlendOp = BLEND_OPERATION_ADD;
    BSDesc.RenderTargets[0].SrcBlendAlpha = BLEND_FACTOR_ONE;
    BSDesc.RenderTargets[0].DestBlendAlpha = BLEND_FACTOR_ZERO;
    BSDesc.RenderTargets[0].BlendOpAlpha = BLEND_OPERATION_ADD;

    PSODesc.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    {
        PSODesc.GraphicsPipeline.pVS = pVS;

        InputLayoutDesc LayoutDesc;
        LayoutElement Elems[] =
        {
            LayoutElement( 0, 0, 3, VT_FLOAT32, false, 0 ),
            LayoutElement( 1, 0, 3, VT_FLOAT32, false, sizeof( float ) * 3 )
        };
        PSODesc.GraphicsPipeline.InputLayout.LayoutElements = Elems;
        PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof( Elems );
        m_pRenderDevice->CreatePipelineState( PSODesc, &m_pPSO );

        Elems[0].Stride = sizeof(float)*6 * 2;
        m_pRenderDevice->CreatePipelineState( PSODesc, &m_pPSO_2xStride );
    }

    {
        PSODesc.GraphicsPipeline.pVS = pVSInst;

        InputLayoutDesc LayoutDesc;
        LayoutElement Elems[] =
        {
            LayoutElement( 0, 0, 3, VT_FLOAT32, false, 0 ),
            LayoutElement( 1, 0, 3, VT_FLOAT32, false, sizeof( float ) * 3 ),
            LayoutElement( 2, 1, 2, VT_FLOAT32, false, 0, 0, LayoutElement::FREQUENCY_PER_INSTANCE )
        };
        PSODesc.GraphicsPipeline.InputLayout.LayoutElements = Elems;
        PSODesc.GraphicsPipeline.InputLayout.NumElements = _countof( Elems );
        m_pRenderDevice->CreatePipelineState( PSODesc, &m_pPSOInst );
    }

    {
        BufferDesc BuffDesc;
        float UniformData[16] = { 1, 1, 1, 1 };
        BuffDesc.uiSizeInBytes = sizeof( UniformData );
        BuffDesc.BindFlags = BIND_UNIFORM_BUFFER;
        BuffDesc.Usage = USAGE_DEFAULT;
        BuffDesc.CPUAccessFlags = 0;
        BufferData BuffData;
        BuffData.pData = UniformData;
        BuffData.DataSize = sizeof( UniformData );
        RefCntAutoPtr<IBuffer> pUniformBuff3, pUniformBuff4;
        BuffDesc.Name = "Test Constant Buffer 3";
        m_pRenderDevice->CreateBuffer( BuffDesc, BuffData, &pUniformBuff3 );
        BuffDesc.Name = "Test Constant Buffer 4";
        m_pRenderDevice->CreateBuffer( BuffDesc, BuffData, &pUniformBuff4 );

        ResourceMappingDesc ResMappingDesc;
        ResourceMappingEntry pEtries[] = { { "cbTestBlock3", pUniformBuff3 }, { "cbTestBlock4", pUniformBuff4 }, { nullptr, nullptr } };
        ResMappingDesc.pEntries = pEtries;
        m_pRenderDevice->CreateResourceMapping( ResMappingDesc, &m_pResMapping );
    }

    pVS->BindResources(m_pResMapping, BIND_SHADER_RESOURCES_ALL_RESOLVED);
    pVSInst->BindResources(m_pResMapping, BIND_SHADER_RESOURCES_ALL_RESOLVED);
    pPS->BindResources(m_pResMapping, BIND_SHADER_RESOURCES_ALL_RESOLVED);
}
    
void TestDrawCommands::Draw()
{
    m_pDeviceContext->SetPipelineState(m_pPSO);
    m_pDeviceContext->CommitShaderResources(nullptr, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);

    IBuffer *pBuffs[2] = {m_pVertexBuff, m_pInstanceData};
    Uint32 Strides[] = {sizeof(float)*6, sizeof(float)*2};
    Uint32 Offsets[] = {0, 0};
    m_pDeviceContext->SetVertexBuffers( 0, 1, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET | BIND_SHADER_RESOURCES_ALL_RESOLVED );

    Uint32 NumTestTrianglesInRow[TriGridSize] = { 0 };
    
    
    // 1ST ROW: simple non-indexed drawing (glDrawArrays/Draw)

    // 0,1: basic drawing
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 2*3; // Draw 2 triangles
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 2,3: test StartVertex
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.StartVertexLocation = 2*3;
        DrawAttrs.NumVertices = 2*3; // Draw 2 triangles
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 4,5: test buffer offset
    Offsets[0] = 4*3*6*sizeof(float);
    m_pDeviceContext->SetVertexBuffers(0, 1, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 2*3; // Draw 2 triangles
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 6,7: test buffer offset & StartVertex
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.StartVertexLocation = 2*3;
        DrawAttrs.NumVertices = 2*3; // Draw 2 triangles
        m_pDeviceContext->Draw(DrawAttrs);
    }
    
    // 8,9: test strides
    Strides[0] *= 2;
    m_pDeviceContext->SetPipelineState(m_pPSO_2xStride);
    m_pDeviceContext->CommitShaderResources(nullptr, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
    m_pDeviceContext->SetVertexBuffers(0, 1, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.StartVertexLocation = 4*3/2; // Stride is 2x
        DrawAttrs.NumVertices = 2*3; // Draw 2 triangles
        m_pDeviceContext->Draw(DrawAttrs);
    }
    Strides[0] /= 2;
    m_pDeviceContext->SetPipelineState(m_pPSO);
    m_pDeviceContext->CommitShaderResources(nullptr, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);

    NumTestTrianglesInRow[0] = 12;
    
    

    
    // 2ND ROW: simple indexed rendering (glDrawElements/DrawIndexed)

    Offsets[0] = 1*16*3 * 6*sizeof(float);
    m_pDeviceContext->SetVertexBuffers(0, 1, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);

    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 2*3; // Draw 2 triangles
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    
    // 2,3: test index buffer offset
    m_pDeviceContext->SetIndexBuffer( m_pIndexBuff, 2 * 3 * sizeof( Uint32 ) );
    
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 2*3; // Draw 2 triangles
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    NumTestTrianglesInRow[1] = 4;
    

    
    // 3RD ROW: indexed rendering with BaseVertex (glDrawElementsBaseVertex/DrawIndexed)
    Offsets[0] = (2*16*3 - 10) * 6*sizeof(float);
    m_pDeviceContext->SetVertexBuffers(0, 1, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);
    
    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 2*3; // Draw 2 triangles
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.BaseVertex = 10;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 2,3: index buffer offset & Base Vertex
    m_pDeviceContext->SetIndexBuffer( m_pIndexBuff, 2 * 3 * sizeof( Uint32 ) );
    
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 2*3; // Draw 2 triangles
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.BaseVertex = 10;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    NumTestTrianglesInRow[2] = 4;
    
    
    // 4TH ROW: Instanced non-indexed rendering (glDrawArraysInstanced/DrawInstanced)

    m_pDeviceContext->SetPipelineState(m_pPSOInst);
    m_pDeviceContext->TransitionShaderResources(m_pPSOInst, nullptr);
    m_pDeviceContext->CommitShaderResources(nullptr, COMMIT_SHADER_RESOURCES_FLAG_VERIFY_STATES);

    Offsets[0] = 3*16*3 * 6*sizeof(float);
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    
    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        m_pDeviceContext->Draw(DrawAttrs);
    }
    
    // 2,3: Test offset in instance buffer
    Offsets[1] = 2* Strides[1];
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 4,5: test start vertex index
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.StartVertexLocation = 2*3;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    NumTestTrianglesInRow[3] = 6;
    
    


    // 5TH ROW: instanced rendering with base instance (glDrawArraysInstancedBaseInstance/DrawInstanced)
    Offsets[0] = 4*16*3 * 6*sizeof(float);
    Offsets[1] = 0;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);
    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.FirstInstanceLocation = 0;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 2,3
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.FirstInstanceLocation = 2;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 4,5: test vertex buffer offset
    Offsets[0] += 2*3 * Strides[0];
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.FirstInstanceLocation = 2;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 6,7: test instance buffer offset
    Offsets[1] += 2 * Strides[1];
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.FirstInstanceLocation = 2;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    NumTestTrianglesInRow[4] = 8;


    
    // 6TH ROW: instanced indexed rendering (glDrawElementsInstanced/DrawIndexedInstanced)

    Offsets[0] = 5*16*3 * 6*sizeof(float);
    Offsets[1] = 0;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);
    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 2,3: test index buffer offset
    m_pDeviceContext->SetIndexBuffer( m_pIndexBuff, 2 * 3 * sizeof( Uint32 ) );
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 4,5: test vertex buffer offset
    Offsets[0] += 2*3 * Strides[0];
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 6,7: test instance buffer offset
    Offsets[1] += 2 * Strides[1];
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 8,9: test first index location
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.FirstIndexLocation = 2*3;
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    NumTestTrianglesInRow[5] = 10;


    
    
    // 7TH ROW: instanced indexed rendering with base instance (glDrawElementsInstancedBaseInstance/DrawInstanced)

    Offsets[0] = 6*16*3 * 6*sizeof(float);
    Offsets[1] = 0;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);
    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    // 2,3
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.FirstInstanceLocation = 2;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 4,5: test index buffer offset
    m_pDeviceContext->SetIndexBuffer( m_pIndexBuff, 2 * 3 * sizeof( Uint32 ) );
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.FirstInstanceLocation = 2;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 6,7: test instance buffer offset
    Offsets[1] += Strides[1] * 2;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.FirstInstanceLocation = 2;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 8,9: test first index location
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.FirstInstanceLocation = 2;
        DrawAttrs.FirstIndexLocation = 2*3;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    NumTestTrianglesInRow[6] = 10;



    // 8TH ROW: instanced indexed rendering with base vertex (glDrawElementsInstancedBaseVertex/DrawInstanced)

    Offsets[0] = 7*16*3 * 6*sizeof(float);
    Offsets[1] = 0;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);
    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 2,3
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.BaseVertex = 2*3;
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    
    // 4,5: test index buffer offset
    m_pDeviceContext->SetIndexBuffer( m_pIndexBuff, 2 * 3 * sizeof( Uint32 ) );
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.BaseVertex = 2*3;
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 6,7: test instance buffer offset
    Offsets[1] += Strides[1] * 2;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.BaseVertex = 2*3;
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 8,9: Test first index location
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.BaseVertex = 2*3;
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.FirstIndexLocation = 2*3;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    NumTestTrianglesInRow[7] = 10;



    // 9TH ROW: instanced indexed rendering with base vertex & base instance (glDrawElementsInstancedBaseVertexBaseInstance/DrawInstanced)

    Offsets[0] = 8*16*3 * 6*sizeof(float);
    Offsets[1] = 0;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);
    // 0,1
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 2,3
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.BaseVertex = 3;
        DrawAttrs.FirstInstanceLocation = 1;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 4,5: test index buffer offset
    m_pDeviceContext->SetIndexBuffer( m_pIndexBuff, 2 * 3 * sizeof( Uint32 ) );
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.BaseVertex = 3;
        DrawAttrs.FirstInstanceLocation = 1;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 6,7: test instance buffer offset
    Offsets[1] += Strides[1] * 2;
    m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.BaseVertex = 3;
        DrawAttrs.FirstInstanceLocation = 1;
        DrawAttrs.IndexType = VT_UINT32;
        m_pDeviceContext->Draw(DrawAttrs);
    }

    // 8,9: test first index location
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        DrawAttrs.NumInstances = 2; // Draw 2 instances
        DrawAttrs.IsIndexed = true;
        DrawAttrs.BaseVertex = 3;
        DrawAttrs.FirstInstanceLocation = 1;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.FirstIndexLocation = 2*3;
        m_pDeviceContext->Draw(DrawAttrs);
    }
    NumTestTrianglesInRow[8] = 10;



    if( m_pRenderDevice->GetDeviceCaps().bIndirectRenderingSupported )
    {
        // 10TH ROW: instanced non-indexed indirect rendering (glDrawArraysIndirect/DrawInstancedIndirect)

        // Test indirect non-indexed drawing
        Offsets[0] = 9*16*3 * 6*sizeof(float);
        Offsets[1] = 0;
        m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);

        // 0,1
        {
            MapHelper<Uint32, true> MappedData(m_pDeviceContext, m_pIndirectDrawArgs, MAP_WRITE, MAP_FLAG_DISCARD);
            MappedData[0] = 3; // Vertex count
            MappedData[1] = 2; // Num instances
            MappedData[2] = 0; // Start vertex
            MappedData[3] = 0; // Start instance
            MappedData.Unmap();

            DrawAttribs DrawAttrs;
            DrawAttrs.IsIndirect = true;
            DrawAttrs.pIndirectDrawAttribs = m_pIndirectDrawArgs;
            m_pDeviceContext->Draw(DrawAttrs);
        }

        // 2,3: test first vertex location
        {
            MapHelper<Uint32, true> MappedData( m_pDeviceContext, m_pIndirectDrawArgs, MAP_WRITE, MAP_FLAG_DISCARD );
            MappedData[0] = 3;    // Vertex count
            MappedData[1] = 2;    // Num instances
            MappedData[2] = 3*2;  // Start vertex
            MappedData[3] = 0;    // Start instance
            MappedData.Unmap();
            DrawAttribs DrawAttrs;
            DrawAttrs.IsIndirect = true;
            DrawAttrs.pIndirectDrawAttribs = m_pIndirectDrawArgs;
            m_pDeviceContext->Draw(DrawAttrs);
        }
    
        // 4,5: test first instance location
        {
            MapHelper<Uint32, true> MappedData( m_pDeviceContext, m_pIndirectDrawArgs, MAP_WRITE, MAP_FLAG_DISCARD );
            MappedData[0] = 3;   // Vertex count
            MappedData[1] = 2;   // Num instances
            MappedData[2] = 3*2; // Start vertex
            MappedData[3] = 2;   // Start instance
            MappedData.Unmap();
            DrawAttribs DrawAttrs;
            DrawAttrs.IsIndirect = true;
            DrawAttrs.pIndirectDrawAttribs = m_pIndirectDrawArgs;
            m_pDeviceContext->Draw(DrawAttrs);
        }

        NumTestTrianglesInRow[9] = 6;




        // 11TH ROW: instanced indexed indirect rendering (glDrawElementsIndirect/DrawIndexedInstancedIndirect)

        Offsets[0] = 10*16*3 * 6*sizeof(float);
        Offsets[1] = 0;
        m_pDeviceContext->SetVertexBuffers(0, 2, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
        m_pDeviceContext->SetIndexBuffer(m_pIndexBuff, 0);

        // 0,1
        {
            MapHelper<Uint32> MappedData( m_pDeviceContext, m_pIndexedIndirectDrawArgs, MAP_WRITE, MAP_FLAG_DISCARD );
            MappedData[0] = 3; // Num indices
            MappedData[1] = 2; // Num instances
            MappedData[2] = 0; // Start index
            MappedData[3] = 0; // Base vertex
            MappedData[4] = 0; // Start instance
            MappedData.Unmap();

            DrawAttribs DrawAttrs;
            DrawAttrs.IsIndirect = true;
            DrawAttrs.IsIndexed = true;
            DrawAttrs.IndexType = VT_UINT32;
            DrawAttrs.pIndirectDrawAttribs = m_pIndexedIndirectDrawArgs;
            m_pDeviceContext->Draw(DrawAttrs);
        }

        // 2,3: test start index location
        {
            MapHelper<Uint32> MappedData( m_pDeviceContext, m_pIndexedIndirectDrawArgs, MAP_WRITE, MAP_FLAG_DISCARD );
            MappedData[0] = 3;  // Num indices
            MappedData[1] = 2;  // Num instances
            MappedData[2] = 6;  // Start index
            MappedData[3] = 0;  // Base vertex
            MappedData[4] = 0;  // Start instance
            MappedData.Unmap();

            DrawAttribs DrawAttrs;
            DrawAttrs.IsIndirect = true;
            DrawAttrs.IsIndexed = true;
            DrawAttrs.IndexType = VT_UINT32;
            DrawAttrs.pIndirectDrawAttribs = m_pIndexedIndirectDrawArgs;
            m_pDeviceContext->Draw(DrawAttrs);
        }

        // 4,5: test base vertex
        {
            MapHelper<Uint32> MappedData( m_pDeviceContext, m_pIndexedIndirectDrawArgs, MAP_WRITE, MAP_FLAG_DISCARD );
            MappedData[0] = 3;  // Num indices
            MappedData[1] = 2;  // Num instances
            MappedData[2] = 6;  // Start index
            MappedData[3] = 2*3;// Base vertex
            MappedData[4] = 0;  // Start instance
            MappedData.Unmap();

            DrawAttribs DrawAttrs;
            DrawAttrs.IsIndirect = true;
            DrawAttrs.IsIndexed = true;
            DrawAttrs.IndexType = VT_UINT32;
            DrawAttrs.pIndirectDrawAttribs = m_pIndexedIndirectDrawArgs;
            m_pDeviceContext->Draw(DrawAttrs);
        }

        // 6,7: test start instance
        {
            MapHelper<Uint32> MappedData( m_pDeviceContext, m_pIndexedIndirectDrawArgs, MAP_WRITE, MAP_FLAG_DISCARD );
            MappedData[0] = 3;  // Num indices
            MappedData[1] = 2;  // Num instances
            MappedData[2] = 6;  // Start index
            MappedData[3] = 2*3;// Base vertex
            MappedData[4] = 2;  // Start instance
            MappedData.Unmap();

            DrawAttribs DrawAttrs;
            DrawAttrs.IsIndirect = true;
            DrawAttrs.IsIndexed = true;
            DrawAttrs.IndexType = VT_UINT32;
            DrawAttrs.pIndirectDrawAttribs = m_pIndexedIndirectDrawArgs;
            m_pDeviceContext->Draw(DrawAttrs);
        }
        NumTestTrianglesInRow[10] = 8;
    }

    // Draw end triangles
    Offsets[0] = 0;
    pBuffs[0] = m_pVertexBuff2;
    m_pDeviceContext->SetVertexBuffers(0, 1, pBuffs, Offsets, SET_VERTEX_BUFFERS_FLAG_RESET);
    
    m_pDeviceContext->SetPipelineState(m_pPSO);
    m_pDeviceContext->CommitShaderResources(nullptr, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);

    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumIndices = 3; // Draw 1 triangle
        
        for(int iRow=0; iRow < TriGridSize; ++iRow)
        {
            DrawAttrs.StartVertexLocation = 16*3*iRow + 3*(1+NumTestTrianglesInRow[iRow]);
            m_pDeviceContext->Draw(DrawAttrs);
        }
    }

    m_pDeviceContext->SetVertexBuffers( 0, 0, nullptr, nullptr, SET_VERTEX_BUFFERS_FLAG_RESET );
    
    SetStatus(TestResult::Succeeded);
}


void TestDrawCommands::Define2DVertex(std::vector<float> &VertexData, float fX, float fY, float fR, float fG, float fB)
{
    VertexData.push_back(fX);
    VertexData.push_back(fY);
    VertexData.push_back(0.5f);
    VertexData.push_back(fR);
    VertexData.push_back(fG);
    VertexData.push_back(fB);
}
