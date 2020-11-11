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

#include <vector>
#include "SampleBase.hpp"
#include "GLTFLoader.hpp"
#include "GLTF_PBR_Renderer.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

class GLTFViewer final : public SampleBase
{
public:
    ~GLTFViewer();
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "GLTF Viewer"; }

private:
    void CreateEnvMapPSO();
    void CreateEnvMapSRB();
    void CreateBoundBoxPSO(TEXTURE_FORMAT RTVFmt, TEXTURE_FORMAT DSVFmt);
    void LoadModel(const char* Path);
    void ResetView();
    void UpdateUI();

    enum class BackgroundMode : int
    {
        None,
        EnvironmentMap,
        Irradiance,
        PrefilteredEnvMap,
        NumModes
    } m_BackgroundMode = BackgroundMode::PrefilteredEnvMap;

    GLTF_PBR_Renderer::RenderInfo m_RenderParams;

    Quaternion m_CameraRotation = {0, 0, 0, 1};
    Quaternion m_ModelRotation  = Quaternion::RotationFromAxisAngle(float3{0.f, 1.0f, 0.0f}, -PI_F / 2.f);
    float4x4   m_ModelTransform;

    float m_CameraDist = 0.9f;

    float3 m_LightDirection;
    float4 m_LightColor     = float4(1, 1, 1, 1);
    float  m_LightIntensity = 3.f;
    float  m_EnvMapMipLevel = 1.f;
    int    m_SelectedModel  = 0;

    static const std::pair<const char*, const char*> GLTFModels[];

    enum class BoundBoxMode : int
    {
        None = 0,
        Local,
        Global
    };

    BoundBoxMode       m_BoundBoxMode   = BoundBoxMode::None;
    bool               m_PlayAnimation  = false;
    int                m_AnimationIndex = 0;
    std::vector<float> m_AnimationTimers;

    std::unique_ptr<GLTF_PBR_Renderer>    m_GLTFRenderer;
    std::unique_ptr<GLTF::Model>          m_Model;
    RefCntAutoPtr<IBuffer>                m_CameraAttribsCB;
    RefCntAutoPtr<IBuffer>                m_LightAttribsCB;
    RefCntAutoPtr<IPipelineState>         m_EnvMapPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_EnvMapSRB;
    RefCntAutoPtr<ITextureView>           m_EnvironmentMapSRV;
    RefCntAutoPtr<IBuffer>                m_EnvMapRenderAttribsCB;

    RefCntAutoPtr<IPipelineState>         m_BoundBoxPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_BoundBoxSRB;

    MouseState m_LastMouseState;
    float      m_CameraYaw   = 0;
    float      m_CameraPitch = 0;
};

} // namespace Diligent
