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

namespace Diligent
{

class Tutorial20_MeshShader final : public SampleBase
{
public:
    virtual void GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc) override final;
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial20: Mesh shader"; }

private:
    void CreatePipelineState();
    void CreateCube();
    void CreateDrawTasks();
    void CreateStatisticsBuffer();
    void CreateConstantsBuffer();
    void LoadTexture();
    void UpdateUI();

    RefCntAutoPtr<IBuffer>      m_CubeBuffer;
    RefCntAutoPtr<ITextureView> m_CubeTextureSRV;

    RefCntAutoPtr<IBuffer> m_pStatisticsBuffer;
    RefCntAutoPtr<IBuffer> m_pStatisticsStaging;
    RefCntAutoPtr<IFence>  m_pStatisticsAvailable;
    Uint64                 m_FrameId               = 0;
    const Uint32           m_StatisticsHistorySize = 8;

    static constexpr Int32 ASGroupSize = 32;

    Uint32                 m_DrawTaskCount = 0;
    RefCntAutoPtr<IBuffer> m_pDrawTasks;
    RefCntAutoPtr<IBuffer> m_pConstants;

    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pSRB;

    float4x4    m_ViewProjMatrix;
    float4x4    m_ViewMatrix;
    float       m_RotationAngle  = 0;
    bool        m_Animate        = true;
    bool        m_FrustumCulling = true;
    const float m_FOV            = PI_F / 4.0f;
    const float m_CoTanHalfFov   = 1.0f / std::tan(m_FOV * 0.5f);
    float       m_LodScale       = 4.0f;
    float       m_CameraHeight   = 10.0f;
    float       m_CurrTime       = 0.0f;
    Uint32      m_VisibleCubes   = 0;
};

} // namespace Diligent
