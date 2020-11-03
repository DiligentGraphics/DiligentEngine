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

#include "TexturedCube.hpp"
#include "BasicMath.hpp"
#include "TextureUtilities.h"

namespace Diligent
{

namespace TexturedCube
{

RefCntAutoPtr<IBuffer> CreateVertexBuffer(IRenderDevice* pDevice)
{
    // Layout of this structure matches the one we defined in the pipeline state
    struct Vertex
    {
        float3 pos;
        float2 uv;
    };

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
    Vertex CubeVerts[] =
    {
        {float3(-1,-1,-1), float2(0,1)},
        {float3(-1,+1,-1), float2(0,0)},
        {float3(+1,+1,-1), float2(1,0)},
        {float3(+1,-1,-1), float2(1,1)},

        {float3(-1,-1,-1), float2(0,1)},
        {float3(-1,-1,+1), float2(0,0)},
        {float3(+1,-1,+1), float2(1,0)},
        {float3(+1,-1,-1), float2(1,1)},

        {float3(+1,-1,-1), float2(0,1)},
        {float3(+1,-1,+1), float2(1,1)},
        {float3(+1,+1,+1), float2(1,0)},
        {float3(+1,+1,-1), float2(0,0)},

        {float3(+1,+1,-1), float2(0,1)},
        {float3(+1,+1,+1), float2(0,0)},
        {float3(-1,+1,+1), float2(1,0)},
        {float3(-1,+1,-1), float2(1,1)},

        {float3(-1,+1,-1), float2(1,0)},
        {float3(-1,+1,+1), float2(0,0)},
        {float3(-1,-1,+1), float2(0,1)},
        {float3(-1,-1,-1), float2(1,1)},

        {float3(-1,-1,+1), float2(1,1)},
        {float3(+1,-1,+1), float2(0,1)},
        {float3(+1,+1,+1), float2(0,0)},
        {float3(-1,+1,+1), float2(1,0)}
    };
    // clang-format on

    BufferDesc VertBuffDesc;
    VertBuffDesc.Name          = "Cube vertex buffer";
    VertBuffDesc.Usage         = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags     = BIND_VERTEX_BUFFER;
    VertBuffDesc.uiSizeInBytes = sizeof(CubeVerts);
    BufferData VBData;
    VBData.pData    = CubeVerts;
    VBData.DataSize = sizeof(CubeVerts);
    RefCntAutoPtr<IBuffer> pCubeVertexBuffer;

    pDevice->CreateBuffer(VertBuffDesc, &VBData, &pCubeVertexBuffer);

    return pCubeVertexBuffer;
}

RefCntAutoPtr<IBuffer> CreateIndexBuffer(IRenderDevice* pDevice)
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
    IndBuffDesc.Name          = "Cube index buffer";
    IndBuffDesc.Usage         = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags     = BIND_INDEX_BUFFER;
    IndBuffDesc.uiSizeInBytes = sizeof(Indices);
    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);
    RefCntAutoPtr<IBuffer> pBuffer;
    pDevice->CreateBuffer(IndBuffDesc, &IBData, &pBuffer);
    return pBuffer;
}

RefCntAutoPtr<ITexture> LoadTexture(IRenderDevice* pDevice, const char* Path)
{
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true;
    RefCntAutoPtr<ITexture> pTex;
    CreateTextureFromFile(Path, loadInfo, pDevice, &pTex);
    return pTex;
}


RefCntAutoPtr<IPipelineState> CreatePipelineState(IRenderDevice*                   pDevice,
                                                  TEXTURE_FORMAT                   RTVFormat,
                                                  TEXTURE_FORMAT                   DSVFormat,
                                                  IShaderSourceInputStreamFactory* pShaderSourceFactory,
                                                  const char*                      VSFilePath,
                                                  const char*                      PSFilePath,
                                                  LayoutElement*                   LayoutElements /*= nullptr*/,
                                                  Uint32                           NumLayoutElements /*= 0*/,
                                                  Uint8                            SampleCount /*= 1*/)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
    PipelineResourceLayoutDesc&     ResourceLayout   = PSODesc.ResourceLayout;
    GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    // This is a graphics pipeline
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSODesc.Name = "Cube PSO";

    // clang-format off
    // This tutorial will render to a single render target
    GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    GraphicsPipeline.RTVFormats[0]                = RTVFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    GraphicsPipeline.DSVFormat                    = DSVFormat;
    // Set the desired number of samples
    GraphicsPipeline.SmplDesc.Count               = SampleCount;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    // Enable depth testing
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on
    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.UseCombinedTextureSamplers = true;

    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = VSFilePath;
        pDevice->CreateShader(ShaderCI, &pVS);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = PSFilePath;
        pDevice->CreateShader(ShaderCI, &pPS);
    }

    // Define vertex shader input layout
    // This tutorial uses two types of input: per-vertex data and per-instance data.
    // clang-format off
    const LayoutElement DefaultLayoutElems[] =
    {
        // Per-vertex data - first buffer slot
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - texture coordinates
        LayoutElement{1, 0, 2, VT_FLOAT32, False}
    };
    // clang-format on

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    GraphicsPipeline.InputLayout.LayoutElements = LayoutElements != nullptr ? LayoutElements : DefaultLayoutElems;
    GraphicsPipeline.InputLayout.NumElements    = LayoutElements != nullptr ? NumLayoutElements : _countof(DefaultLayoutElems);

    // Define variable type that will be used by default
    ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    // clang-format off
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    ResourceLayout.Variables    = Vars;
    ResourceLayout.NumVariables = _countof(Vars);

    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    // clang-format off
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
    };
    // clang-format on
    ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    return pPSO;
}

} // namespace TexturedCube

} // namespace Diligent
