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

#include <cmath>
#include <algorithm>
#include <array>

#include "AtmosphereSample.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "imgui.h"
#include "imGuIZMO.h"
#include "PlatformMisc.hpp"
#include "ImGuiUtils.hpp"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new AtmosphereSample();
}

AtmosphereSample::AtmosphereSample()
{}

void AtmosphereSample::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc)
{
    SampleBase::GetEngineInitializationAttribs(DeviceType, EngineCI, SCDesc);

    EngineCI.Features.ComputeShaders = DEVICE_FEATURE_STATE_ENABLED;
    EngineCI.Features.DepthClamp     = DEVICE_FEATURE_STATE_OPTIONAL;
}

void AtmosphereSample::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    const auto& deviceCaps = InitInfo.pDevice->GetDeviceCaps();
    m_bIsGLDevice          = deviceCaps.IsGLDevice();
    if (m_pDevice->GetDeviceCaps().DevType == RENDER_DEVICE_TYPE_GLES)
    {
        m_ShadowSettings.Resolution                        = 512;
        m_TerrainRenderParams.m_FilterAcrossShadowCascades = false;
        m_ShadowSettings.iFixedFilterSize                  = 3;
        m_PPAttribs.iFirstCascadeToRayMarch                = 2;
        m_PPAttribs.iSingleScatteringMode                  = SINGLE_SCTR_MODE_LUT;
        m_TerrainRenderParams.m_iNumShadowCascades         = 4;
        m_TerrainRenderParams.m_iNumRings                  = 10;
        m_TerrainRenderParams.m_TexturingMode              = RenderingParams::TM_MATERIAL_MASK;
    }

    const auto& RG16UAttribs = m_pDevice->GetTextureFormatInfoExt(TEX_FORMAT_RG16_UNORM);
    const auto& RG32FAttribs = m_pDevice->GetTextureFormatInfoExt(TEX_FORMAT_RG32_FLOAT);
    m_bRG16UFmtSupported     = RG16UAttribs.Supported && (RG16UAttribs.BindFlags & BIND_RENDER_TARGET);
    m_bRG32FFmtSupported     = RG32FAttribs.Supported && (RG32FAttribs.BindFlags & BIND_RENDER_TARGET);
    if (!m_bRG16UFmtSupported && !m_bRG32FFmtSupported)
    {
        m_PPAttribs.bUse1DMinMaxTree = FALSE;
    }
    else
    {
        if (m_bRG16UFmtSupported && !m_bRG32FFmtSupported)
            m_PPAttribs.bIs32BitMinMaxMipMap = FALSE;

        if (!m_bRG16UFmtSupported && m_bRG32FFmtSupported)
            m_PPAttribs.bIs32BitMinMaxMipMap = TRUE;
    }

    m_f3CustomRlghBeta        = m_PPAttribs.f4CustomRlghBeta;
    m_f3CustomMieBeta         = m_PPAttribs.f4CustomMieBeta;
    m_f3CustomOzoneAbsoprtion = m_PPAttribs.f4CustomOzoneAbsorption;

    m_strRawDEMDataFile       = "Terrain\\HeightMap.tif";
    m_strMtrlMaskFile         = "Terrain\\Mask.png";
    m_strTileTexPaths[0]      = "Terrain\\Tiles\\gravel_DM.dds";
    m_strTileTexPaths[1]      = "Terrain\\Tiles\\grass_DM.dds";
    m_strTileTexPaths[2]      = "Terrain\\Tiles\\cliff_DM.dds";
    m_strTileTexPaths[3]      = "Terrain\\Tiles\\snow_DM.dds";
    m_strTileTexPaths[4]      = "Terrain\\Tiles\\grassDark_DM.dds";
    m_strNormalMapTexPaths[0] = "Terrain\\Tiles\\gravel_NM.dds";
    m_strNormalMapTexPaths[1] = "Terrain\\Tiles\\grass_NM.dds";
    m_strNormalMapTexPaths[2] = "Terrain\\Tiles\\cliff_NM.dds";
    m_strNormalMapTexPaths[3] = "Terrain\\Tiles\\Snow_NM.jpg";
    m_strNormalMapTexPaths[4] = "Terrain\\Tiles\\grass_NM.dds";

    // Create data source
    try
    {
        m_pElevDataSource.reset(new ElevationDataSource(m_strRawDEMDataFile.c_str()));
        m_pElevDataSource->SetOffsets(m_TerrainRenderParams.m_iColOffset, m_TerrainRenderParams.m_iRowOffset);
        m_fMinElevation = m_pElevDataSource->GetGlobalMinElevation() * m_TerrainRenderParams.m_TerrainAttribs.m_fElevationScale;
        m_fMaxElevation = m_pElevDataSource->GetGlobalMaxElevation() * m_TerrainRenderParams.m_TerrainAttribs.m_fElevationScale;
    }
    catch (const std::exception&)
    {
        LOG_ERROR("Failed to create elevation data source");
        return;
    }

    const Char *strTileTexPaths[EarthHemsiphere::NUM_TILE_TEXTURES], *strNormalMapPaths[EarthHemsiphere::NUM_TILE_TEXTURES];
    for (int iTile = 0; iTile < _countof(strTileTexPaths); ++iTile)
    {
        strTileTexPaths[iTile]   = m_strTileTexPaths[iTile].c_str();
        strNormalMapPaths[iTile] = m_strNormalMapTexPaths[iTile].c_str();
    }

    CreateUniformBuffer(m_pDevice, sizeof(CameraAttribs), "Camera Attribs CB", &m_pcbCameraAttribs);
    CreateUniformBuffer(m_pDevice, sizeof(LightAttribs), "Light Attribs CB", &m_pcbLightAttribs);

    const auto& SCDesc = m_pSwapChain->GetDesc();
    m_pLightSctrPP.reset(new EpipolarLightScattering(m_pDevice, m_pImmediateContext, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, TEX_FORMAT_R11G11B10_FLOAT));
    auto* pcMediaScatteringParams = m_pLightSctrPP->GetMediaAttribsCB();

    m_EarthHemisphere.Create(m_pElevDataSource.get(),
                             m_TerrainRenderParams,
                             m_pDevice,
                             m_pImmediateContext,
                             m_strMtrlMaskFile.c_str(),
                             strTileTexPaths,
                             strNormalMapPaths,
                             m_pcbCameraAttribs,
                             m_pcbLightAttribs,
                             pcMediaScatteringParams);

    CreateShadowMap();
}

void AtmosphereSample::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::gizmo3D("Light direction", static_cast<float3&>(m_f3LightDir), ImGui::GetTextLineHeight() * 10);
        ImGui::SliderFloat("Camera altitude", &m_f3CameraPos.y, 2000, 100000);

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Shadows"))
        {
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

            if (ImGui::SliderInt("Num cascades", &m_TerrainRenderParams.m_iNumShadowCascades, 1, 8))
                CreateShadowMap();

            ImGui::Checkbox("Visualize cascades", &m_ShadowSettings.bVisualizeCascades);

            ImGui::TreePop();
        }

        ImGui::Checkbox("Enable Light Scattering", &m_bEnableLightScattering);

        if (m_bEnableLightScattering)
        {
            if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Basic"))
                {
                    ImGui::Checkbox("Enable light shafts", &m_PPAttribs.bEnableLightShafts);

                    static_assert(LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING == 0 && LIGHT_SCTR_TECHNIQUE_BRUTE_FORCE == 1, "Unexpcted value");
                    ImGui::Combo("Light scattering tech", &m_PPAttribs.iLightSctrTechnique, "Epipolar\0"
                                                                                            "Brute force\0\0");

                    if (m_PPAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING)
                    {
                        {
                            static constexpr Uint32 MinSlices    = 128;
                            int                     SelectedItem = PlatformMisc::GetLSB(m_PPAttribs.uiNumEpipolarSlices / MinSlices);
                            if (ImGui::Combo("Num Slices", &SelectedItem,
                                             "128\0"
                                             "256\0"
                                             "512\0"
                                             "1024\0"
                                             "2048\0\0"))
                            {
                                m_PPAttribs.uiNumEpipolarSlices = MinSlices << SelectedItem;
                            }
                            ImGui::HelpMarker("Total number of epipolar slices (or lines). For high quality effect, set this value to (Screen Width + Screen Height)/2");
                        }

                        {
                            static constexpr Uint32 MinSamples   = 128;
                            int                     SelectedItem = PlatformMisc::GetLSB(m_PPAttribs.uiMaxSamplesInSlice / MinSamples);
                            if (ImGui::Combo("Max samples", &SelectedItem,
                                             "128\0"
                                             "256\0"
                                             "512\0"
                                             "1024\0"
                                             "2048\0\0"))
                            {
                                m_PPAttribs.uiMaxSamplesInSlice = MinSamples << SelectedItem;
                            }
                            ImGui::HelpMarker("Maximum number of samples on a single epipolar line. For high quality effect, set this value to (Screen Width + Screen Height)/2");
                        }

                        {
                            static constexpr Uint32 MinInitialStep = 4;
                            int                     SelectedItem   = PlatformMisc::GetLSB(m_PPAttribs.uiInitialSampleStepInSlice / MinInitialStep);
                            if (ImGui::Combo("Intial Step", &SelectedItem,
                                             "4\0"
                                             "8\0"
                                             "16\0"
                                             "32\0"
                                             "64\0\0"))
                            {
                                m_PPAttribs.uiInitialSampleStepInSlice = MinInitialStep << SelectedItem;
                            }
                            ImGui::HelpMarker("Initial ray marching sample spacing on an epipolar line. Additional samples are added at discontinuities.");
                        }

                        ImGui::SliderFloat("Refinement Threshold", &m_PPAttribs.fRefinementThreshold, 0.001f, 0.5f);
                        ImGui::HelpMarker("Refinement threshold controls detection of discontinuities. Smaller values produce more samples and higher quality, but at a higher performance cost.");

                        ImGui::Checkbox("Show Sampling", &m_PPAttribs.bShowSampling);

                        if (m_bRG16UFmtSupported || m_bRG32FFmtSupported)
                        {
                            ImGui::Checkbox("Use 1D min/max trees", &m_PPAttribs.bUse1DMinMaxTree);
                            ImGui::HelpMarker("Whether to use 1D min/max binary tree optimization. This improves performance for higher shadow map resolution. Test it.");
                        }

                        ImGui::Checkbox("Optimize Sample Locations", &m_PPAttribs.bOptimizeSampleLocations);
                        ImGui::HelpMarker("Optimize sample locations to avoid oversampling. This should generally be TRUE.");

                        ImGui::Checkbox("Correct Scattering At Depth Breaks", &m_PPAttribs.bCorrectScatteringAtDepthBreaks);
                        ImGui::HelpMarker("Whether to correct inscattering at depth discontinuities. Improves quality for additional cost.");

                        if (m_PPAttribs.bCorrectScatteringAtDepthBreaks)
                        {
                            ImGui::Checkbox("Show Depth Breaks", &m_PPAttribs.bShowDepthBreaks);
                            ImGui::HelpMarker("Whether to display pixels which are classified as depth discontinuities and which will be corrected.");
                        }
                    }

                    ImGui::Checkbox("Lighting Only", &m_PPAttribs.bShowLightingOnly);

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Advanced"))
                {
                    if (!m_PPAttribs.bEnableLightShafts && m_PPAttribs.iSingleScatteringMode == SINGLE_SCTR_MODE_INTEGRATION)
                    {
                        ImGui::SliderIntT("Num Integration Steps", &m_PPAttribs.uiInstrIntegralSteps, 5, 100);
                        ImGui::HelpMarker("Number of inscattering integral steps taken when computing unshadowed inscattering");
                    }

                    if (m_PPAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING)
                    {
                        int SelectedItem = PlatformMisc::GetLSB(m_PPAttribs.uiEpipoleSamplingDensityFactor);
                        if (ImGui::Combo("Epipole sampling density", &SelectedItem,
                                         "1\0"
                                         "2\0"
                                         "4\0"
                                         "8\0\0"))
                        {
                            m_PPAttribs.uiEpipoleSamplingDensityFactor = 1 << SelectedItem;
                        }
                        ImGui::HelpMarker("Sample density scale near the epipole where inscattering changes rapidly. "
                                          "Note that sampling near the epipole is very cheap since only a few steps are required to perform ray marching.");
                    }

                    // clang-format off
                    static_assert(SINGLE_SCTR_MODE_NONE         == 0 &&
                                  SINGLE_SCTR_MODE_INTEGRATION  == 1 &&
                                  SINGLE_SCTR_MODE_LUT          == 2, "Unexpected value");
                    // clang-format on
                    ImGui::Combo("Single scattering mode", &m_PPAttribs.iSingleScatteringMode, "None\0"
                                                                                               "Integration\0"
                                                                                               "Look-up table\0\0");

                    // clang-format off
                    static_assert(MULTIPLE_SCTR_MODE_NONE        == 0 &&
                                  MULTIPLE_SCTR_MODE_UNOCCLUDED  == 1 &&
                                  MULTIPLE_SCTR_MODE_OCCLUDED    == 2, "Unexpected value");
                    // clang-format on
                    ImGui::Combo("Higher-order scattering mode", &m_PPAttribs.iMultipleScatteringMode, "None\0"
                                                                                                       "Unoccluded\0"
                                                                                                       "Occluded\0\0");

                    // clang-format off
                    static_assert(CASCADE_PROCESSING_MODE_SINGLE_PASS     == 0 &&
                                  CASCADE_PROCESSING_MODE_MULTI_PASS      == 1 &&
                                  CASCADE_PROCESSING_MODE_MULTI_PASS_INST == 2, "Unexpected value");
                    // clang-format on
                    ImGui::Combo("Cascade processing mode", &m_PPAttribs.iCascadeProcessingMode,
                                 "Single pass\0"
                                 "Multi-pass\0"
                                 "Multi-pass inst\0\0");

                    ImGui::SliderInt("First Cascade to Ray March", &m_PPAttribs.iFirstCascadeToRayMarch, 0, m_TerrainRenderParams.m_iNumShadowCascades - 1);
                    ImGui::HelpMarker("First cascade to use for ray marching. Usually first few cascades are small, and ray marching them is inefficient.");

                    if (m_bRG16UFmtSupported && m_bRG32FFmtSupported)
                    {
                        ImGui::Checkbox("32-bit float min/max Shadow Map", &m_PPAttribs.bIs32BitMinMaxMipMap);
                        ImGui::HelpMarker("Whether to use 32-bit float or 16-bit UNORM min-max binary tree.");
                    }

                    if (m_PPAttribs.iLightSctrTechnique == LIGHT_SCTR_TECHNIQUE_EPIPOLAR_SAMPLING)
                    {
                        // clang-format off
                        static_assert(REFINEMENT_CRITERION_DEPTH_DIFF  == 0 &&
                                      REFINEMENT_CRITERION_INSCTR_DIFF == 1, "Unexpected value");
                        // clang-format on
                        ImGui::Combo("Refinement criterion", &m_PPAttribs.iRefinementCriterion,
                                     "Depth difference\0"
                                     "Scattering difference\0\0");
                        ImGui::HelpMarker("Epipolar sampling refinement criterion.");

                        // clang-format off
                        static_assert(EXTINCTION_EVAL_MODE_PER_PIXEL == 0 &&
                                      EXTINCTION_EVAL_MODE_EPIPOLAR  == 1, "Unexpected value");
                        // clang-format on
                        ImGui::Combo("Extinction eval mode", &m_PPAttribs.iExtinctionEvalMode,
                                     "Per pixel\0"
                                     "Epipolar\0\0");
                        ImGui::HelpMarker("Epipolar sampling refinement criterion.");
                    }

                    if (ImGui::InputFloat("Aerosol Density", &m_PPAttribs.fAerosolDensityScale, 0.1f, 0.25f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
                        m_PPAttribs.fAerosolDensityScale = clamp(m_PPAttribs.fAerosolDensityScale, 0.1f, 5.0f);

                    if (ImGui::InputFloat("Aerosol Absorption", &m_PPAttribs.fAerosolAbsorbtionScale, 0.1f, 0.25f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
                        m_PPAttribs.fAerosolAbsorbtionScale = clamp(m_PPAttribs.fAerosolAbsorbtionScale, 0.0f, 5.0f);

                    ImGui::Checkbox("Use custom scattering coeffs", &m_PPAttribs.bUseCustomSctrCoeffs);
                    ImGui::Checkbox("Use Ozone approximation", &m_PPAttribs.bUseOzoneApproximation);

                    if (m_PPAttribs.bUseCustomSctrCoeffs)
                    {
                        static constexpr float RLGH_COLOR_SCALE  = 5e-5f;
                        static constexpr float MIE_COLOR_SCALE   = 5e-5f;
                        static constexpr float OZONE_COLOR_SCALE = 5e-6f;

                        {
                            float3 RayleighColor = m_f3CustomRlghBeta / RLGH_COLOR_SCALE;
                            if (ImGui::ColorEdit3("Rayleigh Color", &RayleighColor.r))
                            {
                                m_f3CustomRlghBeta = max(RayleighColor, float3(1, 1, 1) / 255.f) * RLGH_COLOR_SCALE;
                            }
                        }

                        {
                            float3 MieColor = m_f3CustomMieBeta / MIE_COLOR_SCALE;
                            if (ImGui::ColorEdit3("Mie Color", &MieColor.r))
                            {
                                m_f3CustomMieBeta = max(MieColor, float3(1, 1, 1) / 255.f) * MIE_COLOR_SCALE;
                            }
                        }

                        if (m_PPAttribs.bUseOzoneApproximation)
                        {
                            float3 OzoneAbsorption = m_f3CustomOzoneAbsoprtion / OZONE_COLOR_SCALE;
                            if (ImGui::ColorEdit3("Ozone Absorbption", &OzoneAbsorption.r))
                            {
                                m_f3CustomOzoneAbsoprtion = max(OzoneAbsorption, float3(1, 1, 1) / 255.f) * OZONE_COLOR_SCALE;
                            }
                        }

                        if (ImGui::Button("Update coefficients"))
                        {
                            m_PPAttribs.f4CustomRlghBeta        = m_f3CustomRlghBeta;
                            m_PPAttribs.f4CustomMieBeta         = m_f3CustomMieBeta;
                            m_PPAttribs.f4CustomOzoneAbsorption = m_f3CustomOzoneAbsoprtion;
                        }
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Tone mapping"))
                {
                    {
                        std::array<const char*, 7> ToneMappingMode;
                        ToneMappingMode[TONE_MAPPING_MODE_EXP]          = "Exp";
                        ToneMappingMode[TONE_MAPPING_MODE_REINHARD]     = "Reinhard";
                        ToneMappingMode[TONE_MAPPING_MODE_REINHARD_MOD] = "Reinhard Mod";
                        ToneMappingMode[TONE_MAPPING_MODE_UNCHARTED2]   = "Uncharted 2";
                        ToneMappingMode[TONE_MAPPING_FILMIC_ALU]        = "Filmic ALU";
                        ToneMappingMode[TONE_MAPPING_LOGARITHMIC]       = "Logarithmic";
                        ToneMappingMode[TONE_MAPPING_ADAPTIVE_LOG]      = "Adaptive log";
                        ImGui::Combo("Tone Mapping Mode", &m_PPAttribs.ToneMapping.iToneMappingMode, ToneMappingMode.data(), static_cast<int>(ToneMappingMode.size()));
                    }

                    if (m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_MODE_REINHARD_MOD ||
                        m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_MODE_UNCHARTED2 ||
                        m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_LOGARITHMIC ||
                        m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_ADAPTIVE_LOG)
                    {
                        ImGui::SliderFloat("White Point", &m_PPAttribs.ToneMapping.fWhitePoint, 0.01f, 10.0f);
                    }

                    if (m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_MODE_EXP ||
                        m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_MODE_REINHARD ||
                        m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_MODE_REINHARD_MOD ||
                        m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_LOGARITHMIC ||
                        m_PPAttribs.ToneMapping.iToneMappingMode == TONE_MAPPING_ADAPTIVE_LOG)
                    {
                        ImGui::SliderFloat("Luminance Saturation", &m_PPAttribs.ToneMapping.fLuminanceSaturation, 0.01f, 2.f);
                    }

                    ImGui::SliderFloat("Middle Gray", &m_PPAttribs.ToneMapping.fMiddleGray, 0.01f, 1.f);
                    ImGui::Checkbox("Auto Exposure", &m_PPAttribs.ToneMapping.bAutoExposure);
                    if (m_PPAttribs.ToneMapping.bAutoExposure)
                        ImGui::Checkbox("Light Adaptation", &m_PPAttribs.ToneMapping.bLightAdaptation);

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
    }
    ImGui::End();
}


AtmosphereSample::~AtmosphereSample()
{
}

void AtmosphereSample::CreateShadowMap()
{
    ShadowMapManager::InitInfo SMMgrInitInfo;
    SMMgrInitInfo.Format      = m_TerrainRenderParams.ShadowMapFormat;
    SMMgrInitInfo.Resolution  = m_ShadowSettings.Resolution;
    SMMgrInitInfo.NumCascades = m_TerrainRenderParams.m_iNumShadowCascades;
    SMMgrInitInfo.ShadowMode  = SHADOW_MODE_PCF;

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

    m_ShadowMapMgr.Initialize(m_pDevice, SMMgrInitInfo);
}

void AtmosphereSample::RenderShadowMap(IDeviceContext* pContext,
                                       LightAttribs&   LightAttribs,
                                       const float4x4& mCameraView,
                                       const float4x4& mCameraProj)
{
    auto& ShadowAttribs = LightAttribs.ShadowAttribs;

    ShadowMapManager::DistributeCascadeInfo DistrInfo;
    DistrInfo.pCameraView         = &mCameraView;
    DistrInfo.pCameraProj         = &mCameraProj;
    DistrInfo.pLightDir           = &m_f3LightDir;
    DistrInfo.fPartitioningFactor = 0.95f;
    DistrInfo.SnapCascades        = true;
    DistrInfo.EqualizeExtents     = true;
    DistrInfo.StabilizeExtents    = true;
    DistrInfo.AdjustCascadeRange =
        [this](int iCascade, float& MinZ, float& MaxZ) {
            if (iCascade < 0)
            {
                // Snap camera z range to the exponential scale
                const float pw = 1.1f;
                MinZ           = std::pow(pw, std::floor(std::log(std::max(MinZ, 1.f)) / std::log(pw)));
                MinZ           = std::max(MinZ, 10.f);
                MaxZ           = std::pow(pw, std::ceil(std::log(std::max(MaxZ, 1.f)) / std::log(pw)));
            }
            else if (iCascade == m_PPAttribs.iFirstCascadeToRayMarch)
            {
                // Ray marching always starts at the camera position, not at the near plane.
                // So we must make sure that the first cascade used for ray marching covers the camera position
                MinZ = 10.f;
            }
        };
    m_ShadowMapMgr.DistributeCascades(DistrInfo, ShadowAttribs);

    // Render cascades
    for (int iCascade = 0; iCascade < m_TerrainRenderParams.m_iNumShadowCascades; ++iCascade)
    {
        auto* pCascadeDSV = m_ShadowMapMgr.GetCascadeDSV(iCascade);

        m_pImmediateContext->SetRenderTargets(0, nullptr, pCascadeDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(pCascadeDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto CascadeProjMatr = m_ShadowMapMgr.GetCascadeTranform(iCascade).Proj;

        auto WorldToLightViewSpaceMatr = ShadowAttribs.mWorldToLightViewT.Transpose();
        auto WorldToLightProjSpaceMatr = WorldToLightViewSpaceMatr * CascadeProjMatr;

        {
            MapHelper<CameraAttribs> CamAttribs(m_pImmediateContext, m_pcbCameraAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
            CamAttribs->mViewProjT = WorldToLightProjSpaceMatr.Transpose();
        }

        m_EarthHemisphere.Render(m_pImmediateContext, m_TerrainRenderParams, m_f3CameraPos, WorldToLightProjSpaceMatr, nullptr, nullptr, nullptr, true);
    }
}


// Render a frame
void AtmosphereSample::Render()
{
    float4x4 mViewProj = m_mCameraView * m_mCameraProj;

    LightAttribs LightAttrs;
    LightAttrs.f4Direction   = m_f3LightDir;
    LightAttrs.f4Direction.w = 0;

    float4 f4ExtraterrestrialSunColor = float4(10, 10, 10, 10);
    LightAttrs.f4Intensity            = f4ExtraterrestrialSunColor; // *m_fScatteringScale;
    LightAttrs.f4AmbientLight         = float4(0, 0, 0, 0);

    LightAttrs.ShadowAttribs.iNumCascades = m_TerrainRenderParams.m_iNumShadowCascades;
    if (m_ShadowSettings.Resolution >= 2048)
        LightAttrs.ShadowAttribs.fFixedDepthBias = 0.0025f;
    else if (m_ShadowSettings.Resolution >= 1024)
        LightAttrs.ShadowAttribs.fFixedDepthBias = 0.0050f;
    else
        LightAttrs.ShadowAttribs.fFixedDepthBias = 0.0075f;

    // m_iFirstCascade must be initialized before calling RenderShadowMap()!
    m_PPAttribs.iFirstCascadeToRayMarch = std::min(m_PPAttribs.iFirstCascadeToRayMarch, m_TerrainRenderParams.m_iNumShadowCascades - 1);

    RenderShadowMap(m_pImmediateContext, LightAttrs, m_mCameraView, m_mCameraProj);

    LightAttrs.ShadowAttribs.bVisualizeCascades = m_ShadowSettings.bVisualizeCascades ? TRUE : FALSE;

    {
        MapHelper<LightAttribs> LightAttribsCBData(m_pImmediateContext, m_pcbLightAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
        *LightAttribsCBData = LightAttrs;
    }

    // The first time GetAmbientSkyLightSRV() is called, the ambient sky light texture
    // is computed and render target is set. So we need to query the texture before setting
    // render targets
    auto* pAmbientSkyLightSRV = m_pLightSctrPP->GetAmbientSkyLightSRV(m_pDevice, m_pImmediateContext);

    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const float ClearColor[] = {0.350f, 0.350f, 0.350f, 1.0f};
    const float Zero[]       = {0.f, 0.f, 0.f, 0.f};
    m_pImmediateContext->ClearRenderTarget(pRTV, m_bEnableLightScattering ? Zero : ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    if (m_bEnableLightScattering)
    {
        pRTV = m_pOffscreenColorBuffer->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        pDSV = m_pOffscreenDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearRenderTarget(pRTV, Zero, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    CameraAttribs CamAttribs;
    CamAttribs.mViewT        = m_mCameraView.Transpose();
    CamAttribs.mProjT        = m_mCameraProj.Transpose();
    CamAttribs.mViewProjT    = mViewProj.Transpose();
    CamAttribs.mViewProjInvT = mViewProj.Inverse().Transpose();
    float fNearPlane = 0.f, fFarPlane = 0.f;
    m_mCameraProj.GetNearFarClipPlanes(fNearPlane, fFarPlane, m_bIsGLDevice);
    CamAttribs.fNearPlaneZ      = fNearPlane;
    CamAttribs.fFarPlaneZ       = fFarPlane * 0.999999f;
    CamAttribs.f4Position       = m_f3CameraPos;
    CamAttribs.f4ViewportSize.x = static_cast<float>(m_pSwapChain->GetDesc().Width);
    CamAttribs.f4ViewportSize.y = static_cast<float>(m_pSwapChain->GetDesc().Height);
    CamAttribs.f4ViewportSize.z = 1.f / CamAttribs.f4ViewportSize.x;
    CamAttribs.f4ViewportSize.w = 1.f / CamAttribs.f4ViewportSize.y;

    {
        MapHelper<CameraAttribs> CamAttribsCBData(m_pImmediateContext, m_pcbCameraAttribs, MAP_WRITE, MAP_FLAG_DISCARD);
        *CamAttribsCBData = CamAttribs;
    }

    // Render terrain
    auto* pPrecomputedNetDensitySRV    = m_pLightSctrPP->GetPrecomputedNetDensitySRV();
    m_TerrainRenderParams.DstRTVFormat = m_bEnableLightScattering ? m_pOffscreenColorBuffer->GetDesc().Format : m_pSwapChain->GetDesc().ColorBufferFormat;
    m_EarthHemisphere.Render(m_pImmediateContext,
                             m_TerrainRenderParams,
                             m_f3CameraPos,
                             mViewProj,
                             m_ShadowMapMgr.GetSRV(),
                             pPrecomputedNetDensitySRV,
                             pAmbientSkyLightSRV,
                             false);

    if (m_bEnableLightScattering)
    {
        EpipolarLightScattering::FrameAttribs FrameAttribs;

        FrameAttribs.pDevice        = m_pDevice;
        FrameAttribs.pDeviceContext = m_pImmediateContext;
        FrameAttribs.dElapsedTime   = m_fElapsedTime;
        FrameAttribs.pLightAttribs  = &LightAttrs;
        FrameAttribs.pCameraAttribs = &CamAttribs;

        m_PPAttribs.iNumCascades = m_TerrainRenderParams.m_iNumShadowCascades;
        m_PPAttribs.fNumCascades = (float)m_TerrainRenderParams.m_iNumShadowCascades;

        FrameAttribs.pcbLightAttribs  = m_pcbLightAttribs;
        FrameAttribs.pcbCameraAttribs = m_pcbCameraAttribs;

        m_PPAttribs.fMaxShadowMapStep = static_cast<float>(m_ShadowSettings.Resolution / 4);

        m_PPAttribs.f2ShadowMapTexelSize = float2(1.f / static_cast<float>(m_ShadowSettings.Resolution), 1.f / static_cast<float>(m_ShadowSettings.Resolution));
        m_PPAttribs.uiMaxSamplesOnTheRay = m_ShadowSettings.Resolution;

        m_PPAttribs.uiNumSamplesOnTheRayAtDepthBreak = 32u;

        // During the ray marching, on each step we move by the texel size in either horz
        // or vert direction. So resolution of min/max mipmap should be the same as the
        // resolution of the original shadow map
        m_PPAttribs.uiMinMaxShadowMapResolution    = m_ShadowSettings.Resolution;
        m_PPAttribs.uiInitialSampleStepInSlice     = std::min(m_PPAttribs.uiInitialSampleStepInSlice, m_PPAttribs.uiMaxSamplesInSlice);
        m_PPAttribs.uiEpipoleSamplingDensityFactor = std::min(m_PPAttribs.uiEpipoleSamplingDensityFactor, m_PPAttribs.uiInitialSampleStepInSlice);

        FrameAttribs.ptex2DSrcColorBufferSRV = m_pOffscreenColorBuffer->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        FrameAttribs.ptex2DSrcDepthBufferSRV = m_pOffscreenDepthBuffer->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        FrameAttribs.ptex2DDstColorBufferRTV = m_pSwapChain->GetCurrentBackBufferRTV();
        FrameAttribs.ptex2DDstDepthBufferDSV = m_pSwapChain->GetDepthBufferDSV();
        FrameAttribs.ptex2DShadowMapSRV      = m_ShadowMapMgr.GetSRV();

        // Begin new frame
        m_pLightSctrPP->PrepareForNewFrame(FrameAttribs, m_PPAttribs);

        // Render the sun
        m_pLightSctrPP->RenderSun(pRTV->GetDesc().Format, pDSV->GetDesc().Format, 1);

        // Perform the post processing
        m_pLightSctrPP->PerformPostProcessing();
    }
}


void GetRaySphereIntersection(float3        f3RayOrigin,
                              const float3& f3RayDirection,
                              const float3& f3SphereCenter,
                              float         fSphereRadius,
                              float2&       f2Intersections)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    f3RayOrigin -= f3SphereCenter;
    float A = dot(f3RayDirection, f3RayDirection);
    float B = 2 * dot(f3RayOrigin, f3RayDirection);
    float C = dot(f3RayOrigin, f3RayOrigin) - fSphereRadius * fSphereRadius;
    float D = B * B - 4 * A * C;
    // If discriminant is negative, there are no real roots hence the ray misses the
    // sphere
    if (D < 0)
    {
        f2Intersections = float2(-1, -1);
    }
    else
    {
        D = sqrt(D);

        f2Intersections = float2(-B - D, -B + D) / (2 * A); // A must be positive here!!
    }
}

void ComputeApproximateNearFarPlaneDist(const float3&   CameraPos,
                                        const float4x4& ViewMatr,
                                        const float4x4& ProjMatr,
                                        const float3&   EarthCenter,
                                        float           fEarthRadius,
                                        float           fMinRadius,
                                        float           fMaxRadius,
                                        float&          fNearPlaneZ,
                                        float&          fFarPlaneZ)
{
    float4x4 ViewProjMatr = ViewMatr * ProjMatr;
    float4x4 ViewProjInv  = ViewProjMatr.Inverse();

    // Compute maximum view distance for the current camera altitude
    float3 f3CameraGlobalPos   = CameraPos - EarthCenter;
    float  fCameraElevationSqr = dot(f3CameraGlobalPos, f3CameraGlobalPos);
    float  fMaxViewDistance =
        (float)(sqrt((double)fCameraElevationSqr - (double)fEarthRadius * fEarthRadius) +
                sqrt((double)fMaxRadius * fMaxRadius - (double)fEarthRadius * fEarthRadius));
    float fCameraElev = sqrt(fCameraElevationSqr);

    fNearPlaneZ = 50.f;
    if (fCameraElev > fMaxRadius)
    {
        // Adjust near clipping plane
        fNearPlaneZ = (fCameraElev - fMaxRadius) / sqrt(1 + 1.f / (ProjMatr._11 * ProjMatr._11) + 1.f / (ProjMatr._22 * ProjMatr._22));
    }

    fNearPlaneZ = std::max(fNearPlaneZ, 50.f);
    fFarPlaneZ  = 1000;

    const int iNumTestDirections = 5;
    for (int i = 0; i < iNumTestDirections; ++i)
    {
        for (int j = 0; j < iNumTestDirections; ++j)
        {
            float3 PosPS, PosWS, DirFromCamera;
            PosPS.x = (float)i / (float)(iNumTestDirections - 1) * 2.f - 1.f;
            PosPS.y = (float)j / (float)(iNumTestDirections - 1) * 2.f - 1.f;
            PosPS.z = 0; // Far plane is at 0 in complimentary depth buffer
            PosWS   = PosPS * ViewProjInv;

            DirFromCamera = PosWS - CameraPos;
            DirFromCamera = normalize(DirFromCamera);

            float2 IsecsWithBottomBoundSphere;
            GetRaySphereIntersection(CameraPos, DirFromCamera, EarthCenter, fMinRadius, IsecsWithBottomBoundSphere);

            float fNearIsecWithBottomSphere = IsecsWithBottomBoundSphere.x > 0 ? IsecsWithBottomBoundSphere.x : IsecsWithBottomBoundSphere.y;
            if (fNearIsecWithBottomSphere > 0)
            {
                // The ray hits the Earth. Use hit point to compute camera space Z
                float3 HitPointWS = CameraPos + DirFromCamera * fNearIsecWithBottomSphere;
                float3 HitPointCamSpace;
                HitPointCamSpace = HitPointWS * ViewMatr;
                fFarPlaneZ       = std::max(fFarPlaneZ, HitPointCamSpace.z);
            }
            else
            {
                // The ray misses the Earth. In that case the whole earth could be seen
                fFarPlaneZ = fMaxViewDistance;
            }
        }
    }
}


void AtmosphereSample::Update(double CurrTime, double ElapsedTime)
{
    const auto& mouseState = m_InputController.GetMouseState();

    float MouseDeltaX = 0;
    float MouseDeltaY = 0;
    if (m_LastMouseState.PosX >= 0 && m_LastMouseState.PosY >= 0 &&
        m_LastMouseState.ButtonFlags != MouseState::BUTTON_FLAG_NONE)
    {
        MouseDeltaX = mouseState.PosX - m_LastMouseState.PosX;
        MouseDeltaY = mouseState.PosY - m_LastMouseState.PosY;
    }
    m_LastMouseState = mouseState;

    if ((m_LastMouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT) != 0)
    {
        constexpr float CameraRotationSpeed = 0.005f;
        m_fCameraYaw += MouseDeltaX * CameraRotationSpeed;
        m_fCameraPitch += MouseDeltaY * CameraRotationSpeed;
    }
    m_CameraRotation =
        Quaternion::RotationFromAxisAngle(float3{1, 0, 0}, -m_fCameraPitch) *
        Quaternion::RotationFromAxisAngle(float3{0, 1, 0}, -m_fCameraYaw);
    m_f3CameraPos.y += mouseState.WheelDelta * 500.f;
    m_f3CameraPos.y = std::max(m_f3CameraPos.y, 2000.f);
    m_f3CameraPos.y = std::min(m_f3CameraPos.y, 100000.f);

    auto CameraRotationMatrix = m_CameraRotation.ToMatrix();

    if ((m_LastMouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT) != 0)
    {
        constexpr float LightRotationSpeed = 0.001f;

        float  fYawDelta   = MouseDeltaX * LightRotationSpeed;
        float  fPitchDelta = MouseDeltaY * LightRotationSpeed;
        float3 WorldUp{CameraRotationMatrix._12, CameraRotationMatrix._22, CameraRotationMatrix._32};
        float3 WorldRight{CameraRotationMatrix._11, CameraRotationMatrix._21, CameraRotationMatrix._31};
        m_f3LightDir = float4(m_f3LightDir, 0) *
            float4x4::RotationArbitrary(WorldUp, fYawDelta) *
            float4x4::RotationArbitrary(WorldRight, fPitchDelta);
    }

    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    m_fElapsedTime = static_cast<float>(ElapsedTime);

    const auto& SCDesc = m_pSwapChain->GetDesc();
    // Set world/view/proj matrices and global shader constants
    float aspectRatio = (float)SCDesc.Width / SCDesc.Height;

    m_mCameraView =
        float4x4::Translation(-m_f3CameraPos) *
        CameraRotationMatrix;

    // This projection matrix is only used to set up directions in view frustum
    // Actual near and far planes are ignored
    float    FOV      = PI_F / 4.f;
    float4x4 mTmpProj = float4x4::Projection(FOV, aspectRatio, 50.f, 500000.f, m_bIsGLDevice);

    float  fEarthRadius = AirScatteringAttribs().fEarthRadius;
    float3 EarthCenter(0, -fEarthRadius, 0);
    float  fNearPlaneZ, fFarPlaneZ;
    ComputeApproximateNearFarPlaneDist(m_f3CameraPos,
                                       m_mCameraView,
                                       mTmpProj,
                                       EarthCenter,
                                       fEarthRadius,
                                       fEarthRadius + m_fMinElevation,
                                       fEarthRadius + m_fMaxElevation,
                                       fNearPlaneZ,
                                       fFarPlaneZ);
    fNearPlaneZ = std::max(fNearPlaneZ, 50.f);
    fFarPlaneZ  = std::max(fFarPlaneZ, fNearPlaneZ + 100.f);
    fFarPlaneZ  = std::max(fFarPlaneZ, 1000.f);

    m_mCameraProj = float4x4::Projection(FOV, aspectRatio, fNearPlaneZ, fFarPlaneZ, m_bIsGLDevice);

#if 0
    if( m_bAnimateSun )
    {
        auto &LightOrientationMatrix = *m_pDirLightOrienationCamera->GetParentMatrix();
        float3 RotationAxis( 0.5f, 0.3f, 0.0f );
        float3 LightDir = m_pDirLightOrienationCamera->GetLook() * -1;
        float fRotationScaler = ( LightDir.y > +0.2f ) ? 50.f : 1.f;
        float4x4 RotationMatrix = float4x4RotationAxis(RotationAxis, 0.02f * (float)deltaSeconds * fRotationScaler);
        LightOrientationMatrix = LightOrientationMatrix * RotationMatrix;
        m_pDirLightOrienationCamera->SetParentMatrix(LightOrientationMatrix);
    }

    float dt = (float)ElapsedTime;
    if (m_Animate && dt > 0 && dt < 0.2f)
    {
        float3 axis;
        float angle = 0;
        AxisAngleFromRotation(axis, angle, m_Rotation);
        if (length(axis) < 1.0e-6f) 
            axis[1] = 1;
        angle += m_AnimationSpeed * dt;
        if (angle >= 2.0f*FLOAT_PI)
            angle -= 2.0f*FLOAT_PI;
        else if (angle <= 0)
            angle += 2.0f*FLOAT_PI;
        m_Rotation = RotationFromAxisAngle(axis, angle);
    }
#endif
}

void AtmosphereSample::WindowResize(Uint32 Width, Uint32 Height)
{
    m_pLightSctrPP->OnWindowResize(m_pDevice, Width, Height);
    // Flush is required because Intel driver does not release resources until
    // command buffer is flushed. When window is resized, WindowResize() is called for
    // every intermediate window size, and light scattering object creates resources
    // for the new size. This resources are then released by the light scattering object, but
    // not by Intel driver, which results in memory exhaustion.
    m_pImmediateContext->Flush();

    m_pOffscreenColorBuffer.Release();
    m_pOffscreenDepthBuffer.Release();

    TextureDesc ColorBuffDesc;
    ColorBuffDesc.Name      = "Offscreen color buffer";
    ColorBuffDesc.Type      = RESOURCE_DIM_TEX_2D;
    ColorBuffDesc.Width     = Width;
    ColorBuffDesc.Height    = Height;
    ColorBuffDesc.MipLevels = 1;
    ColorBuffDesc.Format    = TEX_FORMAT_R11G11B10_FLOAT;
    ColorBuffDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    m_pDevice->CreateTexture(ColorBuffDesc, nullptr, &m_pOffscreenColorBuffer);

    TextureDesc DepthBuffDesc = ColorBuffDesc;
    DepthBuffDesc.Name        = "Offscreen depth buffer";
    DepthBuffDesc.Format      = TEX_FORMAT_D32_FLOAT;
    DepthBuffDesc.BindFlags   = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
    m_pDevice->CreateTexture(DepthBuffDesc, nullptr, &m_pOffscreenDepthBuffer);
}

} // namespace Diligent
