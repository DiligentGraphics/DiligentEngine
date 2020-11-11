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

#include "ShadowsSample.hpp"
#include "MapHelper.hpp"
#include "FileSystem.hpp"
#include "ShaderMacroHelper.hpp"
#include "CommonlyUsedStates.h"
#include "StringTools.hpp"
#include "GraphicsUtilities.h"
#include "AdvancedMath.hpp"
#include "imgui.h"
#include "imGuIZMO.h"
#include "ImGuiUtils.hpp"


namespace Diligent
{

SampleBase* CreateSample()
{
    return new ShadowsSample();
}

ShadowsSample::~ShadowsSample()
{
}

void ShadowsSample::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType,
                                                   EngineCreateInfo&  EngineCI,
                                                   SwapChainDesc&     SCDesc)
{
    SampleBase::GetEngineInitializationAttribs(DeviceType, EngineCI, SCDesc);

    EngineCI.Features.DepthClamp = DEVICE_FEATURE_STATE_OPTIONAL;

#if D3D12_SUPPORTED
    if (DeviceType == RENDER_DEVICE_TYPE_D3D12)
    {
        auto& D3D12CI                           = static_cast<EngineD3D12CreateInfo&>(EngineCI);
        D3D12CI.GPUDescriptorHeapSize[1]        = 1024; // Sampler descriptors
        D3D12CI.GPUDescriptorHeapDynamicSize[1] = 1024;
    }
#endif
}

void ShadowsSample::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    std::string MeshFileName = "Powerplant/Powerplant.sdkmesh";
    m_Mesh.Create(MeshFileName.c_str());
    std::string Directory;
    FileSystem::SplitFilePath(MeshFileName, &Directory, nullptr);
    m_Mesh.LoadGPUResources(Directory.c_str(), m_pDevice, m_pImmediateContext);

    m_LightAttribs.ShadowAttribs.iNumCascades     = 4;
    m_LightAttribs.ShadowAttribs.fFixedDepthBias  = 0.0025f;
    m_LightAttribs.ShadowAttribs.iFixedFilterSize = 5;
    m_LightAttribs.ShadowAttribs.fFilterWorldSize = 0.1f;

    m_LightAttribs.f4Direction    = float3(-0.522699475f, -0.481321275f, -0.703671455f);
    m_LightAttribs.f4Intensity    = float4(1, 0.8f, 0.5f, 1);
    m_LightAttribs.f4AmbientLight = float4(0.125f, 0.125f, 0.125f, 1);

    m_Camera.SetPos(float3(70, 10, 0.f));
    m_Camera.SetRotation(PI_F / 2.f, 0);
    m_Camera.SetRotationSpeed(0.005f);
    m_Camera.SetMoveSpeed(5.f);
    m_Camera.SetSpeedUpScales(5.f, 10.f);

    CreateUniformBuffer(m_pDevice, sizeof(CameraAttribs), "Camera attribs buffer", &m_CameraAttribsCB);
    CreateUniformBuffer(m_pDevice, sizeof(LightAttribs), "Light attribs buffer", &m_LightAttribsCB);
    CreatePipelineStates();

    CreateShadowMap();
}

void ShadowsSample::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::gizmo3D("Light direction", reinterpret_cast<float3&>(m_LightAttribs.f4Direction), ImGui::GetTextLineHeight() * 10);

        {
            constexpr int MinShadowMapSize = 512;
            int           ShadowMapComboId = 0;
            while ((MinShadowMapSize << ShadowMapComboId) != static_cast<int>(m_ShadowSettings.Resolution))
                ++ShadowMapComboId;
            if (ImGui::Combo("Shadow map size", &ShadowMapComboId,
                             "512\0"
                             "1024\0"
                             "2048\0\0"))
            {
                m_ShadowSettings.Resolution = MinShadowMapSize << ShadowMapComboId;
                CreateShadowMap();
            }
        }

        if (ImGui::SliderInt("Num cascades", &m_LightAttribs.ShadowAttribs.iNumCascades, 1, 8))
            CreateShadowMap();

        {
            int Is32Bit = m_ShadowSettings.Format == TEX_FORMAT_D16_UNORM ? 0 : 1;
            if (ImGui::Combo("Shadow map format", &Is32Bit,
                             "16-bit\0"
                             "32-bit\0\0"))
            {
                m_ShadowSettings.Format = Is32Bit == 0 ? TEX_FORMAT_D16_UNORM : TEX_FORMAT_D32_FLOAT;
                CreatePipelineStates();
                CreateShadowMap();
            }
        }

        {
            static_assert(SHADOW_MODE_PCF == 1 && SHADOW_MODE_VSM == 2 && SHADOW_MODE_EVSM2 == 3 && SHADOW_MODE_EVSM4 == 4, "Unexpected constant");
            // clang-format off
            const char* ShadowModes[]
            {
                "PCF",
                "VSM",
                "EVSM2",
                "EVSM4"
            };
            // clang-format on
            int iShadowModeComboItem = m_ShadowSettings.iShadowMode - 1;
            if (ImGui::Combo("Shadow mode", &iShadowModeComboItem, ShadowModes, _countof(ShadowModes)))
            {
                m_ShadowSettings.iShadowMode = iShadowModeComboItem + 1;
                CreatePipelineStates();
                CreateShadowMap();
            }
        }

        {
            // clang-format off
            const std::pair<int, const char*> FilterSizes[] =
            {
                {0, "World-constant"},
                {2, "Fixed 2x2"},
                {3, "Fixed 3x3"},
                {5, "Fixed 5x5"},
                {7, "Fixed 7x7"}
            };
            // clang-format on
            if (ImGui::Combo("Shadow filter size", &m_LightAttribs.ShadowAttribs.iFixedFilterSize, FilterSizes, _countof(FilterSizes)))
            {
                m_ShadowSettings.FilterAcrossCascades = m_LightAttribs.ShadowAttribs.iFixedFilterSize > 0;
                CreatePipelineStates();
            }
        }

        if (m_ShadowSettings.iShadowMode == SHADOW_MODE_VSM ||
            m_ShadowSettings.iShadowMode == SHADOW_MODE_EVSM2 ||
            m_ShadowSettings.iShadowMode == SHADOW_MODE_EVSM4)
        {
            if (ImGui::Checkbox("32-bit filterable Format", &m_ShadowSettings.Is32BitFilterableFmt))
            {
                CreateShadowMap();
            }
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Cascade allocation"))
        {
            if (ImGui::InputFloat("Partitioning Factor", &m_ShadowSettings.PartitioningFactor, 0.001f, 0.01f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                m_ShadowSettings.PartitioningFactor = clamp(m_ShadowSettings.PartitioningFactor, 0.f, 1.f);
            }
            // clang-format off
            ImGui::Checkbox("Snap cascades",     &m_ShadowSettings.SnapCascades);
            ImGui::Checkbox("Stabilize extents", &m_ShadowSettings.StabilizeExtents);
            ImGui::Checkbox("Equalize extents",  &m_ShadowSettings.EqualizeExtents);
            // clang-format on
            if (ImGui::Checkbox("Use best cascade", &m_ShadowSettings.SearchBestCascade))
            {
                CreatePipelineStates();
            }
            ImGui::TreePop();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Filtering"))
        {
            if (ImGui::Checkbox("Filter across cascades", &m_ShadowSettings.FilterAcrossCascades))
                CreatePipelineStates();

            if (m_ShadowSettings.FilterAcrossCascades)
                ImGui::SliderFloat("Cascade transition region", &m_LightAttribs.ShadowAttribs.fCascadeTransitionRegion, 0, 0.5f);

            if (m_LightAttribs.ShadowAttribs.iFixedFilterSize == 0)
                ImGui::SliderFloat("Filter world size", &m_LightAttribs.ShadowAttribs.fFilterWorldSize, 0, 0.25f);

            if (m_ShadowSettings.iShadowMode == SHADOW_MODE_PCF)
            {
                ImGui::SliderFloat("Max depth bias slope", &m_LightAttribs.ShadowAttribs.fReceiverPlaneDepthBiasClamp, 0, 20);
                ImGui::SliderFloat("Fixed depth bias", &m_LightAttribs.ShadowAttribs.fFixedDepthBias, 0, 1, "%.4f", ImGuiSliderFlags_Logarithmic);
            }

            if (m_ShadowSettings.iShadowMode == SHADOW_MODE_EVSM2 ||
                m_ShadowSettings.iShadowMode == SHADOW_MODE_EVSM4)
                ImGui::SliderFloat("Positive EVSM Exponent", &m_LightAttribs.ShadowAttribs.fEVSMPositiveExponent, 0.1f, 40);

            if (m_ShadowSettings.iShadowMode == SHADOW_MODE_EVSM4)
                ImGui::SliderFloat("Negative EVSM Exponent", &m_LightAttribs.ShadowAttribs.fEVSMNegativeExponent, 0.1f, 40);

            if (m_ShadowSettings.iShadowMode == SHADOW_MODE_VSM ||
                m_ShadowSettings.iShadowMode == SHADOW_MODE_EVSM2 ||
                m_ShadowSettings.iShadowMode == SHADOW_MODE_EVSM4)
                ImGui::SliderFloat("Light bleeding reduction", &m_LightAttribs.ShadowAttribs.fVSMLightBleedingReduction, 0, 0.99f, "%.4f", ImGuiSliderFlags_Logarithmic);

            if (m_ShadowSettings.iShadowMode == SHADOW_MODE_VSM)
                ImGui::SliderFloat("VSM Bias", &m_LightAttribs.ShadowAttribs.fVSMBias, 0, 1, "%.4f", ImGuiSliderFlags_Logarithmic);

            ImGui::TreePop();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Visualization"))
        {
            ImGui::Checkbox("Visualize cascades", &m_LightAttribs.ShadowAttribs.bVisualizeCascades);
            ImGui::Checkbox("Shadows only", &m_LightAttribs.ShadowAttribs.bVisualizeShadowing);
            ImGui::TreePop();
        }
    }
    ImGui::End();
}


void ShadowsSample::DXSDKMESH_VERTEX_ELEMENTtoInputLayoutDesc(const DXSDKMESH_VERTEX_ELEMENT* VertexElement,
                                                              Uint32                          Stride,
                                                              InputLayoutDesc&                Layout,
                                                              std::vector<LayoutElement>&     Elements)
{
    Elements.clear();
    for (Uint32 input_elem = 0; VertexElement[input_elem].Stream != 0xFF; ++input_elem)
    {
        const auto& SrcElem    = VertexElement[input_elem];
        Int32       InputIndex = -1;
        switch (SrcElem.Usage)
        {
            case DXSDKMESH_VERTEX_SEMANTIC_POSITION:
                InputIndex = 0;
                break;

            case DXSDKMESH_VERTEX_SEMANTIC_NORMAL:
                InputIndex = 1;
                break;

            case DXSDKMESH_VERTEX_SEMANTIC_TEXCOORD:
                InputIndex = 2;
                break;
        }

        if (InputIndex >= 0)
        {
            Uint32     NumComponents = 0;
            VALUE_TYPE ValueType     = VT_UNDEFINED;
            Bool       IsNormalized  = False;
            switch (SrcElem.Type)
            {
                case DXSDKMESH_VERTEX_DATA_TYPE_FLOAT2:
                    NumComponents = 2;
                    ValueType     = VT_FLOAT32;
                    IsNormalized  = False;
                    break;

                case DXSDKMESH_VERTEX_DATA_TYPE_FLOAT3:
                    NumComponents = 3;
                    ValueType     = VT_FLOAT32;
                    IsNormalized  = False;
                    break;

                default:
                    UNEXPECTED("Unsupported data type. Please add appropriate case statement.");
            }
            Elements.emplace_back(InputIndex, SrcElem.Stream, NumComponents, ValueType, IsNormalized, SrcElem.Offset, Stride);
        }
    }
    Layout.LayoutElements = Elements.data();
    Layout.NumElements    = static_cast<Uint32>(Elements.size());
}

void ShadowsSample::CreatePipelineStates()
{
    ShaderCreateInfo                               ShaderCI;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("shaders", &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.UseCombinedTextureSamplers = true;

    ShaderMacroHelper Macros;
    // clang-format off
    Macros.AddShaderMacro( "SHADOW_MODE",            m_ShadowSettings.iShadowMode);
    Macros.AddShaderMacro( "SHADOW_FILTER_SIZE",     m_LightAttribs.ShadowAttribs.iFixedFilterSize);
    Macros.AddShaderMacro( "FILTER_ACROSS_CASCADES", m_ShadowSettings.FilterAcrossCascades);
    Macros.AddShaderMacro( "BEST_CASCADE_SEARCH",    m_ShadowSettings.SearchBestCascade );
    // clang-format on
    ShaderCI.Macros = Macros;

    ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
    ShaderCI.Desc.Name       = "Mesh VS";
    ShaderCI.EntryPoint      = "MeshVS";
    ShaderCI.FilePath        = "MeshVS.vsh";
    RefCntAutoPtr<IShader> pVS;
    m_pDevice->CreateShader(ShaderCI, &pVS);

    ShaderCI.Desc.Name       = "Mesh PS";
    ShaderCI.EntryPoint      = "MeshPS";
    ShaderCI.FilePath        = "MeshPS.psh";
    ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
    RefCntAutoPtr<IShader> pPS;
    m_pDevice->CreateShader(ShaderCI, &pPS);

    Macros.AddShaderMacro("SHADOW_PASS", true);
    ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
    ShaderCI.Desc.Name       = "Mesh VS";
    ShaderCI.EntryPoint      = "MeshVS";
    ShaderCI.FilePath        = "MeshVS.vsh";
    ShaderCI.Macros          = Macros;
    RefCntAutoPtr<IShader> pShadowVS;
    m_pDevice->CreateShader(ShaderCI, &pShadowVS);

    m_PSOIndex.resize(m_Mesh.GetNumVBs());
    m_RenderMeshPSO.clear();
    m_RenderMeshShadowPSO.clear();
    for (Uint32 vb = 0; vb < m_Mesh.GetNumVBs(); ++vb)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
        PipelineResourceLayoutDesc&     ResourceLayout   = PSODesc.ResourceLayout;
        GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        std::vector<LayoutElement> Elements;
        auto&                      InputLayout = PSOCreateInfo.GraphicsPipeline.InputLayout;
        DXSDKMESH_VERTEX_ELEMENTtoInputLayoutDesc(m_Mesh.VBElements(vb), m_Mesh.GetVertexStride(vb), InputLayout, Elements);

        //  Try to find PSO with the same layout
        Uint32 pso;
        for (pso = 0; pso < m_RenderMeshPSO.size(); ++pso)
        {
            const auto& PSOLayout = m_RenderMeshPSO[pso]->GetGraphicsPipelineDesc().InputLayout;

            bool IsSameLayout =
                PSOLayout.NumElements == InputLayout.NumElements &&
                memcmp(PSOLayout.LayoutElements, InputLayout.LayoutElements, sizeof(LayoutElement) * InputLayout.NumElements) == 0;

            if (IsSameLayout)
                break;
        }

        m_PSOIndex[vb] = pso;
        if (pso < static_cast<Uint32>(m_RenderMeshPSO.size()))
            continue;

        // clang-format off
        ImmutableSamplerDesc ImtblSampler[] =
        {
            {SHADER_TYPE_PIXEL, "g_tex2DDiffuse", Sam_Aniso4xWrap}
        };
        // clang-format on
        ResourceLayout.ImmutableSamplers    = ImtblSampler;
        ResourceLayout.NumImmutableSamplers = _countof(ImtblSampler);

        // clang-format off
        ShaderResourceVariableDesc Vars[] = 
        {
            {SHADER_TYPE_PIXEL, "g_tex2DDiffuse",   SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL, m_ShadowSettings.iShadowMode == SHADOW_MODE_PCF ? "g_tex2DShadowMap" : "g_tex2DFilterableShadowMap",   SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
        };
        // clang-format on
        ResourceLayout.Variables    = Vars;
        ResourceLayout.NumVariables = _countof(Vars);

        PSODesc.Name      = "Mesh PSO";
        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        GraphicsPipeline.RTVFormats[0]              = m_pSwapChain->GetDesc().ColorBufferFormat;
        GraphicsPipeline.NumRenderTargets           = 1;
        GraphicsPipeline.DSVFormat                  = m_pSwapChain->GetDesc().DepthBufferFormat;
        GraphicsPipeline.PrimitiveTopology          = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;

        RefCntAutoPtr<IPipelineState> pRenderMeshPSO;
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pRenderMeshPSO);
        // clang-format off
        pRenderMeshPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbCameraAttribs")->Set(m_CameraAttribsCB);
        pRenderMeshPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL,  "cbLightAttribs")->Set(m_LightAttribsCB);
        pRenderMeshPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbLightAttribs")->Set(m_LightAttribsCB);
        // clang-format on

        PSODesc.Name      = "Mesh Shadow PSO";
        PSOCreateInfo.pPS = nullptr;
        PSOCreateInfo.pVS = pShadowVS;

        GraphicsPipeline.NumRenderTargets = 0;
        GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_UNKNOWN;
        GraphicsPipeline.DSVFormat        = m_ShadowSettings.Format;

        // It is crucial to disable depth clip to allow shadows from objects
        // behind the near cascade clip plane!
        GraphicsPipeline.RasterizerDesc.DepthClipEnable = False;

        GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;

        ResourceLayout.ImmutableSamplers    = nullptr;
        ResourceLayout.NumImmutableSamplers = 0;
        ResourceLayout.Variables            = nullptr;
        ResourceLayout.NumVariables         = 0;
        RefCntAutoPtr<IPipelineState> pRenderMeshShadowPSO;
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pRenderMeshShadowPSO);
        pRenderMeshShadowPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "cbCameraAttribs")->Set(m_CameraAttribsCB);

        m_RenderMeshPSO.emplace_back(std::move(pRenderMeshPSO));
        m_RenderMeshShadowPSO.emplace_back(std::move(pRenderMeshShadowPSO));
    }
}

void ShadowsSample::InitializeResourceBindings()
{
    m_SRBs.clear();
    m_ShadowSRBs.clear();
    m_SRBs.resize(m_Mesh.GetNumMaterials());
    m_ShadowSRBs.resize(m_Mesh.GetNumMaterials());
    for (Uint32 mat = 0; mat < m_Mesh.GetNumMaterials(); ++mat)
    {
        {
            const auto& Mat = m_Mesh.GetMaterial(mat);

            RefCntAutoPtr<IShaderResourceBinding> pSRB;
            m_RenderMeshPSO[0]->CreateShaderResourceBinding(&pSRB, true);
            VERIFY(Mat.pDiffuseRV != nullptr, "Material must have diffuse color texture");
            pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DDiffuse")->Set(Mat.pDiffuseRV);
            if (m_ShadowSettings.iShadowMode == SHADOW_MODE_PCF)
            {
                pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DShadowMap")->Set(m_ShadowMapMgr.GetSRV());
            }
            else
            {
                pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DFilterableShadowMap")->Set(m_ShadowMapMgr.GetFilterableSRV());
            }
            m_SRBs[mat] = std::move(pSRB);
        }

        {
            RefCntAutoPtr<IShaderResourceBinding> pShadowSRB;
            m_RenderMeshShadowPSO[0]->CreateShaderResourceBinding(&pShadowSRB, true);
            m_ShadowSRBs[mat] = std::move(pShadowSRB);
        }
    }
}

void ShadowsSample::CreateShadowMap()
{
    if (m_ShadowSettings.Resolution >= 2048)
        m_LightAttribs.ShadowAttribs.fFixedDepthBias = 0.0025f;
    else if (m_ShadowSettings.Resolution >= 1024)
        m_LightAttribs.ShadowAttribs.fFixedDepthBias = 0.005f;
    else
        m_LightAttribs.ShadowAttribs.fFixedDepthBias = 0.0075f;

    ShadowMapManager::InitInfo SMMgrInitInfo;
    SMMgrInitInfo.Format               = m_ShadowSettings.Format;
    SMMgrInitInfo.Resolution           = m_ShadowSettings.Resolution;
    SMMgrInitInfo.NumCascades          = static_cast<Uint32>(m_LightAttribs.ShadowAttribs.iNumCascades);
    SMMgrInitInfo.ShadowMode           = m_ShadowSettings.iShadowMode;
    SMMgrInitInfo.Is32BitFilterableFmt = m_ShadowSettings.Is32BitFilterableFmt;

    if (!m_pComparisonSampler)
    {
        SamplerDesc ComparsionSampler;
        ComparsionSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
        // Note: anisotropic filtering requires SampleGrad to fix artifacts at
        // cascade boundaries
        ComparsionSampler.MinFilter = FILTER_TYPE_COMPARISON_LINEAR;
        ComparsionSampler.MagFilter = FILTER_TYPE_COMPARISON_LINEAR;
        ComparsionSampler.MipFilter = FILTER_TYPE_COMPARISON_LINEAR;
        m_pDevice->CreateSampler(ComparsionSampler, &m_pComparisonSampler);
    }
    SMMgrInitInfo.pComparisonSampler = m_pComparisonSampler;

    if (!m_pFilterableShadowMapSampler)
    {
        SamplerDesc SamplerDesc;
        SamplerDesc.MinFilter     = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MagFilter     = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MipFilter     = FILTER_TYPE_ANISOTROPIC;
        SamplerDesc.MaxAnisotropy = m_LightAttribs.ShadowAttribs.iMaxAnisotropy;
        m_pDevice->CreateSampler(SamplerDesc, &m_pFilterableShadowMapSampler);
    }
    SMMgrInitInfo.pFilterableShadowMapSampler = m_pFilterableShadowMapSampler;

    m_ShadowMapMgr.Initialize(m_pDevice, SMMgrInitInfo);

    InitializeResourceBindings();
}

void ShadowsSample::RenderShadowMap()
{
    auto iNumShadowCascades = m_LightAttribs.ShadowAttribs.iNumCascades;
    for (int iCascade = 0; iCascade < iNumShadowCascades; ++iCascade)
    {
        const auto CascadeProjMatr = m_ShadowMapMgr.GetCascadeTranform(iCascade).Proj;

        auto WorldToLightViewSpaceMatr = m_LightAttribs.ShadowAttribs.mWorldToLightViewT.Transpose();
        auto WorldToLightProjSpaceMatr = WorldToLightViewSpaceMatr * CascadeProjMatr;

        CameraAttribs ShadowCameraAttribs = {};

        ShadowCameraAttribs.mViewT     = m_LightAttribs.ShadowAttribs.mWorldToLightViewT;
        ShadowCameraAttribs.mProjT     = CascadeProjMatr.Transpose();
        ShadowCameraAttribs.mViewProjT = WorldToLightProjSpaceMatr.Transpose();

        ShadowCameraAttribs.f4ViewportSize.x = static_cast<float>(m_ShadowSettings.Resolution);
        ShadowCameraAttribs.f4ViewportSize.y = static_cast<float>(m_ShadowSettings.Resolution);
        ShadowCameraAttribs.f4ViewportSize.z = 1.f / ShadowCameraAttribs.f4ViewportSize.x;
        ShadowCameraAttribs.f4ViewportSize.w = 1.f / ShadowCameraAttribs.f4ViewportSize.y;

        {
            MapHelper<CameraAttribs> CameraData(m_pImmediateContext, m_CameraAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
            *CameraData = ShadowCameraAttribs;
        }

        auto* pCascadeDSV = m_ShadowMapMgr.GetCascadeDSV(iCascade);
        m_pImmediateContext->SetRenderTargets(0, nullptr, pCascadeDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(pCascadeDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        ViewFrustumExt Frutstum;
        ExtractViewFrustumPlanesFromMatrix(WorldToLightProjSpaceMatr, Frutstum, m_pDevice->GetDeviceCaps().IsGLDevice());
        DrawMesh(m_pImmediateContext, true, Frutstum);
    }

    if (m_ShadowSettings.iShadowMode > SHADOW_MODE_PCF)
        m_ShadowMapMgr.ConvertToFilterable(m_pImmediateContext, m_LightAttribs.ShadowAttribs);
}

// Render a frame
void ShadowsSample::Render()
{
    RenderShadowMap();

    // Reset default framebuffer
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Clear the back buffer
    const float ClearColor[] = {0.23f, 0.5f, 0.74f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        MapHelper<LightAttribs> LightData(m_pImmediateContext, m_LightAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *LightData = m_LightAttribs;
    }

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    const auto  CameraView     = m_Camera.GetViewMatrix() * SrfPreTransform;
    const auto& CameraWorld    = m_Camera.GetWorldMatrix();
    float3      CameraWorldPos = float3::MakeVector(CameraWorld[3]);
    const auto& Proj           = m_Camera.GetProjMatrix();

    auto CameraViewProj = CameraView * Proj;

    {
        MapHelper<CameraAttribs> CamAttribs(m_pImmediateContext, m_CameraAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
        CamAttribs->mProjT        = Proj.Transpose();
        CamAttribs->mViewProjT    = CameraViewProj.Transpose();
        CamAttribs->mViewProjInvT = CameraViewProj.Inverse().Transpose();
        CamAttribs->f4Position    = float4(CameraWorldPos, 1);
    }

    ViewFrustumExt Frutstum;
    ExtractViewFrustumPlanesFromMatrix(CameraViewProj, Frutstum, m_pDevice->GetDeviceCaps().IsGLDevice());
    DrawMesh(m_pImmediateContext, false, Frutstum);
}


void ShadowsSample::DrawMesh(IDeviceContext* pCtx, bool bIsShadowPass, const ViewFrustumExt& Frustum)
{
    // Note that Vulkan requires shadow map to be transitioned to DEPTH_READ state, not SHADER_RESOURCE
    pCtx->TransitionShaderResources((bIsShadowPass ? m_RenderMeshShadowPSO : m_RenderMeshPSO)[0], (bIsShadowPass ? m_ShadowSRBs : m_SRBs)[0]);

    for (Uint32 meshIdx = 0; meshIdx < m_Mesh.GetNumMeshes(); ++meshIdx)
    {
        const auto& SubMesh = m_Mesh.GetMesh(meshIdx);
        BoundBox    BB;
        BB.Min = SubMesh.BoundingBoxCenter - SubMesh.BoundingBoxExtents * 0.5f;
        BB.Max = SubMesh.BoundingBoxCenter + SubMesh.BoundingBoxExtents * 0.5f;
        // Notice that for shadow pass we test against frustum with open near plane
        if (GetBoxVisibility(Frustum, BB, bIsShadowPass ? FRUSTUM_PLANE_FLAG_OPEN_NEAR : FRUSTUM_PLANE_FLAG_FULL_FRUSTUM) == BoxVisibility::Invisible)
            continue;

        IBuffer* pVBs[]    = {m_Mesh.GetMeshVertexBuffer(meshIdx, 0)};
        Uint32   Offsets[] = {0};
        pCtx->SetVertexBuffers(0, 1, pVBs, Offsets, RESOURCE_STATE_TRANSITION_MODE_VERIFY, SET_VERTEX_BUFFERS_FLAG_RESET);

        auto* pIB      = m_Mesh.GetMeshIndexBuffer(meshIdx);
        auto  IBFormat = m_Mesh.GetIBFormat(meshIdx);

        pCtx->SetIndexBuffer(pIB, 0, RESOURCE_STATE_TRANSITION_MODE_VERIFY);

        auto  PSOIndex = m_PSOIndex[SubMesh.VertexBuffers[0]];
        auto& pPSO     = (bIsShadowPass ? m_RenderMeshShadowPSO : m_RenderMeshPSO)[PSOIndex];
        pCtx->SetPipelineState(pPSO);

        // Draw all subsets
        for (Uint32 subsetIdx = 0; subsetIdx < SubMesh.NumSubsets; ++subsetIdx)
        {
            const auto& Subset = m_Mesh.GetSubset(meshIdx, subsetIdx);
            pCtx->CommitShaderResources((bIsShadowPass ? m_ShadowSRBs : m_SRBs)[Subset.MaterialID], RESOURCE_STATE_TRANSITION_MODE_VERIFY);

            DrawIndexedAttribs drawAttrs(static_cast<Uint32>(Subset.IndexCount), IBFormat, DRAW_FLAG_VERIFY_ALL);
            drawAttrs.FirstIndexLocation = static_cast<Uint32>(Subset.IndexStart);
            pCtx->DrawIndexed(drawAttrs);
        }
    }
}

void ShadowsSample::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    m_Camera.Update(m_InputController, static_cast<float>(ElapsedTime));
    {
        const auto& mouseState = m_InputController.GetMouseState();
        if (m_LastMouseState.PosX >= 0 &&
            m_LastMouseState.PosY >= 0 &&
            (m_LastMouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT) != 0)
        {
            constexpr float LightRotationSpeed = 0.001f;

            float   fYawDelta   = (mouseState.PosX - m_LastMouseState.PosX) * LightRotationSpeed;
            float   fPitchDelta = (mouseState.PosY - m_LastMouseState.PosY) * LightRotationSpeed;
            float3& f3LightDir  = reinterpret_cast<float3&>(m_LightAttribs.f4Direction);

            f3LightDir = float4(f3LightDir, 0) *
                float4x4::RotationArbitrary(m_Camera.GetWorldUp(), fYawDelta) *
                float4x4::RotationArbitrary(m_Camera.GetWorldRight(), fPitchDelta);
        }

        m_LastMouseState = mouseState;
    }


    ShadowMapManager::DistributeCascadeInfo DistrInfo;
    DistrInfo.pCameraView   = &m_Camera.GetViewMatrix();
    DistrInfo.pCameraProj   = &m_Camera.GetProjMatrix();
    float3 f3LightDirection = float3(m_LightAttribs.f4Direction.x, m_LightAttribs.f4Direction.y, m_LightAttribs.f4Direction.z);
    DistrInfo.pLightDir     = &f3LightDirection;

    DistrInfo.fPartitioningFactor = m_ShadowSettings.PartitioningFactor;
    DistrInfo.SnapCascades        = m_ShadowSettings.SnapCascades;
    DistrInfo.EqualizeExtents     = m_ShadowSettings.EqualizeExtents;
    DistrInfo.StabilizeExtents    = m_ShadowSettings.StabilizeExtents;

    m_ShadowMapMgr.DistributeCascades(DistrInfo, m_LightAttribs.ShadowAttribs);
}

void ShadowsSample::WindowResize(Uint32 Width, Uint32 Height)
{
    float NearPlane   = 0.1f;
    float FarPlane    = 250.f;
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    m_Camera.SetProjAttribs(NearPlane, FarPlane, AspectRatio, PI_F / 4.f,
                            m_pSwapChain->GetDesc().PreTransform, m_pDevice->GetDeviceCaps().IsGLDevice());
}

} // namespace Diligent
