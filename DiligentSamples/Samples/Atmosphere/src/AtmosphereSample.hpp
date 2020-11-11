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

#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include "EarthHemisphere.hpp"
#include "ElevationDataSource.hpp"
#include "EpipolarLightScattering.hpp"
#include "ShadowMapManager.hpp"

namespace Diligent
{

class AtmosphereSample final : public SampleBase
{
public:
    AtmosphereSample();
    ~AtmosphereSample();

    virtual void GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc) override final;

    virtual void Initialize(const SampleInitInfo& InitInfo) override final;
    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;
    virtual void WindowResize(Uint32 Width, Uint32 Height) override final;

    virtual const Char* GetSampleName() const override final { return "Atmosphere Sample"; }

private:
    void UpdateUI();
    void CreateShadowMap();
    void RenderShadowMap(IDeviceContext* pContext,
                         LightAttribs&   LightAttribs,
                         const float4x4& mCameraView,
                         const float4x4& mCameraProj);

    float3 m_f3LightDir = {-0.554699242f, -0.0599640049f, -0.829887390f};

    Quaternion m_CameraRotation = {0, 0, 0, 1};
    float3     m_f3CameraPos    = {0, 8000.f, 0};
    float4x4   m_mCameraView;
    float4x4   m_mCameraProj;

    RefCntAutoPtr<IBuffer> m_pcbCameraAttribs;
    RefCntAutoPtr<IBuffer> m_pcbLightAttribs;

    ShadowMapManager m_ShadowMapMgr;
    struct ShadowSettings
    {
        Uint32 Resolution                 = 1024;
        float  fCascadePartitioningFactor = 0.95f;
        bool   bVisualizeCascades         = false;
        int    iFixedFilterSize           = 5;
    } m_ShadowSettings;

    RefCntAutoPtr<ISampler> m_pComparisonSampler;

    RenderingParams                m_TerrainRenderParams;
    EpipolarLightScatteringAttribs m_PPAttribs;

    String m_strRawDEMDataFile;
    String m_strMtrlMaskFile;
    String m_strTileTexPaths[EarthHemsiphere::NUM_TILE_TEXTURES];
    String m_strNormalMapTexPaths[EarthHemsiphere::NUM_TILE_TEXTURES];

    float m_fMinElevation = 0, m_fMaxElevation = 0;

    std::unique_ptr<ElevationDataSource> m_pElevDataSource;
    EarthHemsiphere                      m_EarthHemisphere;
    bool                                 m_bIsGLDevice = false;

    std::unique_ptr<EpipolarLightScattering> m_pLightSctrPP;

    bool   m_bEnableLightScattering = true;
    float  m_fElapsedTime           = 0.f;
    float3 m_f3CustomRlghBeta, m_f3CustomMieBeta, m_f3CustomOzoneAbsoprtion;

    RefCntAutoPtr<ITexture> m_pOffscreenColorBuffer;
    RefCntAutoPtr<ITexture> m_pOffscreenDepthBuffer;

    float      m_fCameraYaw   = 0.23f;
    float      m_fCameraPitch = 0.18f;
    MouseState m_LastMouseState;

    bool m_bRG16UFmtSupported = false;
    bool m_bRG32FFmtSupported = false;
};

} // namespace Diligent
