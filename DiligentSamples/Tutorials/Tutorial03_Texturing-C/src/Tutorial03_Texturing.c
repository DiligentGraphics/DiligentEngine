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

#include <string.h>

#include "RenderDevice.h"
#include "SwapChain.h"
#include "DeviceContext.h"
#include "Shader.h"

#include "GraphicsUtilities.h"
#include "TextureUtilities.h"

IRenderDevice*          g_pDevice           = NULL;
ISwapChain*             g_pSwapChain        = NULL;
IBuffer*                g_pVSConstants      = NULL;
IPipelineState*         g_pPSO              = NULL;
IShaderResourceBinding* g_pSRB              = NULL;
IBuffer*                g_pCubeVertexBuffer = NULL;
IBuffer*                g_pCubeIndexBuffer  = NULL;

float g_WorldViewProj[16];

void CreatePipelineState(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    memset(&PSOCreateInfo, 0, sizeof(PSOCreateInfo));

    PipelineStateDesc* pPSODesc = &PSOCreateInfo._PipelineStateCreateInfo.PSODesc;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    pPSODesc->_DeviceObjectAttribs.Name = "Cube PSO";

    // This is a graphics pipeline
    pPSODesc->PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off

    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    const SwapChainDesc* pSCDesc = ISwapChain_GetDesc(pSwapChain);
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = pSCDesc->ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = pSCDesc->DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // clang-format on

    PSOCreateInfo.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask = COLOR_MASK_ALL;

    pPSODesc->CommandQueueMask                    = 1;
    PSOCreateInfo.GraphicsPipeline.SmplDesc.Count = 1;
    PSOCreateInfo.GraphicsPipeline.SampleMask     = 0xFFFFFFFF;
    PSOCreateInfo.GraphicsPipeline.NumViewports   = 1;

    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable      = True;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = True;
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthFunc        = COMPARISON_FUNC_LESS;

    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.FillMode        = FILL_MODE_SOLID;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode        = CULL_MODE_NONE;
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.DepthClipEnable = True;

    ShaderCreateInfo ShaderCI;
    memset(&ShaderCI, 0, sizeof(ShaderCI));

    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;
    ShaderCI.CombinedSamplerSuffix      = "_sampler";

    // Create a shader source stream factory to load shaders from files.
    IEngineFactory* pEngineFactory = IRenderDevice_GetEngineFactory(pDevice);

    IShaderSourceInputStreamFactory* pShaderSourceFactory = NULL;
    IEngineFactory_CreateDefaultShaderSourceStreamFactory(pEngineFactory, NULL, &pShaderSourceFactory);

    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    IShader* pVS = NULL;
    {
        ShaderCI.Desc._DeviceObjectAttribs.Name = "Cube VS";

        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.FilePath        = "cube.vsh";
        IRenderDevice_CreateShader(pDevice, &ShaderCI, &pVS);

        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        Diligent_CreateUniformBuffer(pDevice, sizeof(g_WorldViewProj), "VS constants CB", &g_pVSConstants,
                                     USAGE_DYNAMIC, BIND_UNIFORM_BUFFER, CPU_ACCESS_WRITE, NULL);
    }


    // Create a pixel shader
    IShader* pPS = NULL;
    {
        ShaderCI.Desc._DeviceObjectAttribs.Name = "Cube PS";

        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.FilePath        = "cube.psh";
        IRenderDevice_CreateShader(pDevice, &ShaderCI, &pPS);
    }

    // Define vertex shader input layout
    LayoutElement LayoutElems[2];
    LayoutElems[0].HLSLSemantic         = "ATTRIB";
    LayoutElems[0].InputIndex           = 0;
    LayoutElems[0].BufferSlot           = 0;
    LayoutElems[0].NumComponents        = 3;
    LayoutElems[0].ValueType            = VT_FLOAT32;
    LayoutElems[0].IsNormalized         = False;
    LayoutElems[0].RelativeOffset       = LAYOUT_ELEMENT_AUTO_OFFSET;
    LayoutElems[0].Stride               = LAYOUT_ELEMENT_AUTO_STRIDE;
    LayoutElems[0].Frequency            = INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    LayoutElems[0].InstanceDataStepRate = 1;

    LayoutElems[1].HLSLSemantic         = "ATTRIB";
    LayoutElems[1].InputIndex           = 1;
    LayoutElems[1].BufferSlot           = 0;
    LayoutElems[1].NumComponents        = 2;
    LayoutElems[1].ValueType            = VT_FLOAT32;
    LayoutElems[1].IsNormalized         = False;
    LayoutElems[1].RelativeOffset       = LAYOUT_ELEMENT_AUTO_OFFSET;
    LayoutElems[1].Stride               = LAYOUT_ELEMENT_AUTO_STRIDE;
    LayoutElems[1].Frequency            = INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    LayoutElems[1].InstanceDataStepRate = 1;

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = 2;

    // Define variable type that will be used by default
    pPSODesc->ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[1];
    Vars[0].ShaderStages = SHADER_TYPE_PIXEL;
    Vars[0].Name         = "g_Texture";
    Vars[0].Type         = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    pPSODesc->ResourceLayout.Variables    = Vars;
    pPSODesc->ResourceLayout.NumVariables = 1;

    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    ImmutableSamplerDesc ImtblSamplers[1];

    memset(&ImtblSamplers, 0, sizeof(ImtblSamplers));
    ImtblSamplers[0].ShaderStages         = SHADER_TYPE_PIXEL;
    ImtblSamplers[0].SamplerOrTextureName = "g_Texture";

    ImtblSamplers[0].Desc._DeviceObjectAttribs.Name = "Linear sampler";

    ImtblSamplers[0].Desc.MinFilter      = FILTER_TYPE_LINEAR;
    ImtblSamplers[0].Desc.MagFilter      = FILTER_TYPE_LINEAR;
    ImtblSamplers[0].Desc.MipFilter      = FILTER_TYPE_LINEAR;
    ImtblSamplers[0].Desc.AddressU       = TEXTURE_ADDRESS_CLAMP;
    ImtblSamplers[0].Desc.AddressV       = TEXTURE_ADDRESS_CLAMP;
    ImtblSamplers[0].Desc.AddressW       = TEXTURE_ADDRESS_CLAMP;
    ImtblSamplers[0].Desc.ComparisonFunc = COMPARISON_FUNC_NEVER,
    ImtblSamplers[0].Desc.MaxLOD         = +3.402823466e+38F;

    pPSODesc->ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    pPSODesc->ResourceLayout.NumImmutableSamplers = 1;

    IRenderDevice_CreateGraphicsPipelineState(pDevice, &PSOCreateInfo, &g_pPSO);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
    // never change and are bound directly through the pipeline state object.
    IShaderResourceVariable* pVar = IPipelineState_GetStaticVariableByName(g_pPSO, SHADER_TYPE_VERTEX, "Constants");
    IShaderResourceVariable_Set(pVar, (IDeviceObject*)g_pVSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    IPipelineState_CreateShaderResourceBinding(g_pPSO, &g_pSRB, true);

    IObject_Release(pPS);
    IObject_Release(pVS);
    IObject_Release(pShaderSourceFactory);
}


void CreateVertexBuffer(IRenderDevice* pDevice)
{
    // Layout of this structure matches the one we defined in the pipeline state
    typedef struct Vertex
    {
        float pos[3];
        float uv[2];
    } Vertex;

    // Cube vertices

    //      (-1,+1,+1)________________(+1,+1,+1)
    //               /|              /|
    //              / |             / |
    //             /  |            /  |
    //            /   |           /   |
    //(-1,-1,+1) /____|__________/(+1,-1,+1)
    //           |    |__________|____|
    //           |   /(-1,+1,-1) |    /(+1,+1,-1)
    //           |  /            |   /
    //           | /             |  /
    //           |/              | /
    //           /_______________|/
    //        (-1,-1,-1)       (+1,-1,-1)
    //

    // clang-format off
    // This time we have to duplicate verices because texture coordinates cannot
    // be shared
    Vertex CubeVerts[] =
    {
        {{-1,-1,-1}, {0,1}},
        {{-1,+1,-1}, {0,0}},
        {{+1,+1,-1}, {1,0}},
        {{+1,-1,-1}, {1,1}},

        {{-1,-1,-1}, {0,1}},
        {{-1,-1,+1}, {0,0}},
        {{+1,-1,+1}, {1,0}},
        {{+1,-1,-1}, {1,1}},

        {{+1,-1,-1}, {0,1}},
        {{+1,-1,+1}, {1,1}},
        {{+1,+1,+1}, {1,0}},
        {{+1,+1,-1}, {0,0}},

        {{+1,+1,-1}, {0,1}},
        {{+1,+1,+1}, {0,0}},
        {{-1,+1,+1}, {1,0}},
        {{-1,+1,-1}, {1,1}},

        {{-1,+1,-1}, {1,0}},
        {{-1,+1,+1}, {0,0}},
        {{-1,-1,+1}, {0,1}},
        {{-1,-1,-1}, {1,1}},

        {{-1,-1,+1}, {1,1}},
        {{+1,-1,+1}, {0,1}},
        {{+1,+1,+1}, {0,0}},
        {{-1,+1,+1}, {1,0}}
    };
    // clang-format on


    BufferDesc VertBuffDesc;
    memset(&VertBuffDesc, 0, sizeof(VertBuffDesc));
    VertBuffDesc._DeviceObjectAttribs.Name = "Cube vertex buffer";

    VertBuffDesc.Usage            = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags        = BIND_VERTEX_BUFFER;
    VertBuffDesc.uiSizeInBytes    = sizeof(CubeVerts);
    VertBuffDesc.CommandQueueMask = 1;

    BufferData VBData;
    VBData.pData    = CubeVerts;
    VBData.DataSize = sizeof(CubeVerts);

    IRenderDevice_CreateBuffer(g_pDevice, &VertBuffDesc, &VBData, &g_pCubeVertexBuffer);
}

void CreateIndexBuffer(IRenderDevice* pDevice)
{

    // clang-format off
    Uint32 Indices[] =
    {
        2,0,1,    2,3,0,
        4,6,5,    4,7,6,
        8,10,9,   8,11,10,
        12,14,13, 12,15,14,
        16,18,17, 16,19,18,
        20,21,22, 20,22,23
    };
    // clang-format on

    BufferDesc IndBuffDesc;
    memset(&IndBuffDesc, 0, sizeof(IndBuffDesc));
    IndBuffDesc._DeviceObjectAttribs.Name = "Index vertex buffer";

    IndBuffDesc.Usage            = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags        = BIND_INDEX_BUFFER;
    IndBuffDesc.uiSizeInBytes    = sizeof(Indices);
    IndBuffDesc.CommandQueueMask = 1;

    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);

    IRenderDevice_CreateBuffer(g_pDevice, &IndBuffDesc, &IBData, &g_pCubeIndexBuffer);
}

void LoadTexture(IRenderDevice* pDevice)
{
    TextureLoadInfo loadInfo;
    memset(&loadInfo, 0, sizeof(loadInfo));
    loadInfo.IsSRGB       = true;
    loadInfo.Usage        = USAGE_IMMUTABLE;
    loadInfo.BindFlags    = BIND_SHADER_RESOURCE;
    loadInfo.GenerateMips = True;

    ITexture* pTex = NULL;
    Diligent_CreateTextureFromFile("DGLogo.png", &loadInfo, pDevice, &pTex);
    // Get shader resource view from the texture
    ITextureView* pTextureSRV = ITexture_GetDefaultView(pTex, TEXTURE_VIEW_SHADER_RESOURCE);

    // Set texture SRV in the SRB
    IShaderResourceVariable* pVar = IShaderResourceBinding_GetVariableByName(g_pSRB, SHADER_TYPE_PIXEL, "g_Texture");
    IShaderResourceVariable_Set(pVar, (IDeviceObject*)pTextureSRV);
    IObject_Release((IObject*)pTex);
}

void CreateResources(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    g_pDevice = pDevice;
    IObject_AddRef(g_pDevice);

    g_pSwapChain = pSwapChain;
    IObject_AddRef(g_pSwapChain);

    CreatePipelineState(pDevice, pSwapChain);
    CreateVertexBuffer(pDevice);
    CreateIndexBuffer(pDevice);
    LoadTexture(pDevice);
}

void ReleaseResources()
{
    if (g_pCubeIndexBuffer)
    {
        IObject_Release(g_pCubeIndexBuffer);
        g_pCubeIndexBuffer = NULL;
    }

    if (g_pCubeVertexBuffer)
    {
        IObject_Release(g_pCubeVertexBuffer);
        g_pCubeVertexBuffer = NULL;
    }

    if (g_pSRB)
    {
        IObject_Release(g_pSRB);
        g_pSRB = NULL;
    }

    if (g_pPSO)
    {
        IObject_Release(g_pPSO);
        g_pPSO = NULL;
    }

    if (g_pVSConstants)
    {
        IObject_Release(g_pVSConstants);
        g_pVSConstants = NULL;
    }

    if (g_pSwapChain)
    {
        IObject_Release(g_pSwapChain);
        g_pSwapChain = NULL;
    }

    if (g_pDevice)
    {
        IObject_Release(g_pDevice);
        g_pDevice = NULL;
    }
}

// Render a frame
void Render(IDeviceContext* pContext)
{
    ITextureView* pRTV = ISwapChain_GetCurrentBackBufferRTV(g_pSwapChain);
    ITextureView* pDSV = ISwapChain_GetDepthBufferDSV(g_pSwapChain);
    IDeviceContext_SetRenderTargets(pContext, 1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    // Clear the back buffer
    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    IDeviceContext_ClearRenderTarget(pContext, pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    IDeviceContext_ClearDepthStencil(pContext, pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);


    {
        // Map the buffer and write current world-view-projection matrix
        void* pCBData = NULL;
        IDeviceContext_MapBuffer(pContext, g_pVSConstants, MAP_WRITE, MAP_FLAG_DISCARD, &pCBData);
        memcpy(pCBData, g_WorldViewProj, sizeof(g_WorldViewProj));
        IDeviceContext_UnmapBuffer(pContext, g_pVSConstants, MAP_WRITE);
    }

    // Bind vertex and index buffers
    Uint32   offset = 0;
    IBuffer* pBuffs[1];
    pBuffs[0] = g_pCubeVertexBuffer;
    IDeviceContext_SetVertexBuffers(pContext, 0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    IDeviceContext_SetIndexBuffer(pContext, g_pCubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    IDeviceContext_SetPipelineState(pContext, g_pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    IDeviceContext_CommitShaderResources(pContext, g_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs; // This is an indexed draw call
    memset(&DrawAttrs, 0, sizeof(DrawAttrs));
    DrawAttrs.IndexType    = VT_UINT32; // Index type
    DrawAttrs.NumIndices   = 36;
    DrawAttrs.NumInstances = 1;
    // Verify the state of vertex and index buffers
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    IDeviceContext_DrawIndexed(pContext, &DrawAttrs);
}

void Update(float* pWorldViewProjData)
{
    memcpy(g_WorldViewProj, pWorldViewProjData, sizeof(g_WorldViewProj));
}
