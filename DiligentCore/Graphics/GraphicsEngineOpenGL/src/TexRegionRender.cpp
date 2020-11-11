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

#include "pch.h"

#include "TexRegionRender.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"
#include "../../GraphicsTools/interface/MapHelper.hpp"


namespace Diligent
{

// clang-format off
static const Char* VertexShaderSource =
{
    //To use any built-in input or output in the gl_PerVertex and
    //gl_PerFragment blocks in separable program objects, shader code must
    //redeclare those blocks prior to use. 
    //
    // Declaring this block causes compilation error on NVidia GLES
    "#ifndef GL_ES          \n"
    "out gl_PerVertex       \n"
    "{                      \n"
    "	vec4 gl_Position;   \n"
    "};                     \n"
    "#endif                 \n"

    "void main()                                    \n"
    "{                                              \n"
    "   vec4 Bounds = vec4(-1.0, -1.0, 1.0, 1.0);   \n"
    "   vec2 PosXY[4];                              \n"
    "   PosXY[0] = Bounds.xy;                       \n"
    "   PosXY[1] = Bounds.xw;                       \n"
    "   PosXY[2] = Bounds.zy;                       \n"
    "   PosXY[3] = Bounds.zw;                       \n"
    "   gl_Position = vec4(PosXY[gl_VertexID], 0.0, 1.0);\n"
    "}                                              \n"
};
// clang-format on

TexRegionRender::TexRegionRender(class RenderDeviceGLImpl* pDeviceGL)
{
    ShaderCreateInfo ShaderAttrs;
    ShaderAttrs.Desc.Name                 = "TexRegionRender : Vertex shader";
    ShaderAttrs.Desc.ShaderType           = SHADER_TYPE_VERTEX;
    ShaderAttrs.Source                    = VertexShaderSource;
    constexpr bool IsInternalDeviceObject = true;
    pDeviceGL->CreateShader(ShaderAttrs, &m_pVertexShader, IsInternalDeviceObject);


    static const char* SamplerType[RESOURCE_DIM_NUM_DIMENSIONS] = {};

    SamplerType[RESOURCE_DIM_TEX_1D]       = "sampler1D";
    SamplerType[RESOURCE_DIM_TEX_1D_ARRAY] = "sampler1DArray";
    SamplerType[RESOURCE_DIM_TEX_2D]       = "sampler2D";
    SamplerType[RESOURCE_DIM_TEX_2D_ARRAY] = "sampler2DArray",
    SamplerType[RESOURCE_DIM_TEX_3D]       = "sampler3D";
    // There is no texelFetch() for texture cube [array]
    //SamplerType[RESOURCE_DIM_TEX_CUBE]         = "samplerCube";
    //SamplerType[RESOURCE_DIM_TEX_CUBE_ARRAY]   = "smaplerCubeArray";


    static const char* SrcLocations[RESOURCE_DIM_NUM_DIMENSIONS] = {};

    SrcLocations[RESOURCE_DIM_TEX_1D]       = "int(gl_FragCoord.x) + Constants.x";
    SrcLocations[RESOURCE_DIM_TEX_1D_ARRAY] = "ivec2(int(gl_FragCoord.x) + Constants.x, Constants.z)";
    SrcLocations[RESOURCE_DIM_TEX_2D]       = "ivec2(gl_FragCoord.xy) + Constants.xy";
    SrcLocations[RESOURCE_DIM_TEX_2D_ARRAY] = "ivec3(ivec2(gl_FragCoord.xy) + Constants.xy, Constants.z)",
    SrcLocations[RESOURCE_DIM_TEX_3D]       = "ivec3(ivec2(gl_FragCoord.xy) + Constants.xy, Constants.z)";
    // There is no texelFetch() for texture cube [array]
    //CoordDim[RESOURCE_DIM_TEX_CUBE]         = "ivec2(gl_FragCoord.xy)";
    //CoordDim[RESOURCE_DIM_TEX_CUBE_ARRAY]   = "ivec2(gl_FragCoord.xy)";

    BufferDesc CBDesc;
    CBDesc.Name           = "TexRegionRender: FS constants CB";
    CBDesc.uiSizeInBytes  = sizeof(Int32) * 4;
    CBDesc.Usage          = USAGE_DYNAMIC;
    CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDeviceGL->CreateBuffer(CBDesc, nullptr, &m_pConstantBuffer, IsInternalDeviceObject);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& GraphicsPipeline                   = PSOCreateInfo.GraphicsPipeline;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    GraphicsPipeline.RasterizerDesc.FillMode = FILL_MODE_SOLID;

    GraphicsPipeline.DepthStencilDesc.DepthEnable      = False;
    GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;
    PSOCreateInfo.pVS                                  = m_pVertexShader;
    GraphicsPipeline.PrimitiveTopology                 = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    static const char* CmpTypePrefix[3] = {"", "i", "u"};
    for (Int32 Dim = RESOURCE_DIM_TEX_2D; Dim <= RESOURCE_DIM_TEX_3D; ++Dim)
    {
        const auto* SamplerDim  = SamplerType[Dim];
        const auto* SrcLocation = SrcLocations[Dim];
        for (Int32 Fmt = 0; Fmt < 3; ++Fmt)
        {
            const auto* Prefix = CmpTypePrefix[Fmt];
            String      Name   = "TexRegionRender : Pixel shader ";
            Name.append(Prefix);
            Name.append(SamplerDim);
            ShaderAttrs.Desc.Name       = Name.c_str();
            ShaderAttrs.Desc.ShaderType = SHADER_TYPE_PIXEL;

            std::stringstream SourceSS;
            SourceSS << "uniform " << Prefix << SamplerDim << " gSourceTex;\n"
                     << "layout(location = 0) out " << Prefix
                     << "vec4 Out;\n"
                        "uniform cbConstants\n"
                        "{\n"
                        "    ivec4 Constants;\n"
                        "};\n"
                        "void main()\n"
                        "{\n"
                        "    Out = texelFetch( gSourceTex, "
                     << SrcLocation
                     << ", Constants.w );\n"
                        "}\n";

            auto Source         = SourceSS.str();
            ShaderAttrs.Source  = Source.c_str();
            auto& FragmetShader = m_pFragmentShaders[Dim * 3 + Fmt];
            pDeviceGL->CreateShader(ShaderAttrs, &FragmetShader, IsInternalDeviceObject);
            PSOCreateInfo.pPS = FragmetShader;

            auto& ResourceLayout = PSOCreateInfo.PSODesc.ResourceLayout;

            ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
            ShaderResourceVariableDesc Vars[] =
                {
                    {SHADER_TYPE_PIXEL, "cbConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            ResourceLayout.NumVariables = _countof(Vars);
            ResourceLayout.Variables    = Vars;

            pDeviceGL->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO[Dim * 3 + Fmt], IsInternalDeviceObject);
        }
    }
    m_pPSO[RESOURCE_DIM_TEX_2D * 3]->CreateShaderResourceBinding(&m_pSRB);
    m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "cbConstants")->Set(m_pConstantBuffer);
    m_pSrcTexVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "gSourceTex");
}

void TexRegionRender::SetStates(DeviceContextGLImpl* pCtxGL)
{
    pCtxGL->GetRenderTargets(m_NumRenderTargets, m_pOrigRTVs, &m_pOrigDSV);

    Uint32 NumViewports = 0;
    pCtxGL->GetViewports(NumViewports, nullptr);
    m_OrigViewports.resize(NumViewports);
    pCtxGL->GetViewports(NumViewports, m_OrigViewports.data());

    pCtxGL->GetPipelineState(&m_pOrigPSO, m_OrigBlendFactors, m_OrigStencilRef);
}

void TexRegionRender::RestoreStates(DeviceContextGLImpl* pCtxGL)
{
    pCtxGL->SetRenderTargets(m_NumRenderTargets, m_pOrigRTVs, m_pOrigDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    for (Uint32 rt = 0; rt < _countof(m_pOrigRTVs); ++rt)
    {
        if (m_pOrigRTVs[rt])
            m_pOrigRTVs[rt]->Release();
        m_pOrigRTVs[rt] = nullptr;
    }
    m_pOrigDSV.Release();

    pCtxGL->SetViewports((Uint32)m_OrigViewports.size(), m_OrigViewports.data(), 0, 0);

    if (m_pOrigPSO)
        pCtxGL->SetPipelineState(m_pOrigPSO);
    m_pOrigPSO.Release();
    pCtxGL->SetStencilRef(m_OrigStencilRef);
    pCtxGL->SetBlendFactors(m_OrigBlendFactors);
}

void TexRegionRender::Render(DeviceContextGLImpl* pCtxGL,
                             ITextureView*        pSrcSRV,
                             RESOURCE_DIMENSION   TexType,
                             TEXTURE_FORMAT       TexFormat,
                             Int32                DstToSrcXOffset,
                             Int32                DstToSrcYOffset,
                             Int32                SrcZ,
                             Int32                SrcMipLevel)
{
    {
        MapHelper<int> pConstant(pCtxGL, m_pConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        pConstant[0] = DstToSrcXOffset;
        pConstant[1] = DstToSrcYOffset;
        pConstant[2] = SrcZ;
        pConstant[3] = SrcMipLevel;
    }

    const auto& TexFmtAttribs = GetTextureFormatAttribs(TexFormat);
    Uint32      FSInd         = TexType * 3;
    if (TexFmtAttribs.ComponentType == COMPONENT_TYPE_SINT)
        FSInd += 1;
    else if (TexFmtAttribs.ComponentType == COMPONENT_TYPE_UINT)
        FSInd += 2;

    if (TexFmtAttribs.ComponentType == COMPONENT_TYPE_SNORM)
    {
        LOG_WARNING_MESSAGE("CopyData() is performed by rendering to texture.\n"
                            "There might be an issue in OpenGL driver on NVidia hardware: when rendering to SNORM textures, all negative values are clamped to zero.");
    }

    DEV_CHECK_ERR(m_pPSO[FSInd], "TexRegionRender does not support this combination of texture dimension/format");

    pCtxGL->SetPipelineState(m_pPSO[FSInd]);
    m_pSrcTexVar->Set(pSrcSRV);
    pCtxGL->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 4;
    pCtxGL->Draw(DrawAttrs);

    m_pSrcTexVar->Set(nullptr);
}

} // namespace Diligent
