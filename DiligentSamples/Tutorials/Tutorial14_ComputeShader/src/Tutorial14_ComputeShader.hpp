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
#include "ResourceMapping.h"
#include "BasicMath.hpp"

namespace Diligent
{

class Tutorial14_ComputeShader final : public SampleBase
{
public:
    virtual void GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc) override final;

    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial14: Compute Shader"; }

private:
    void CreateRenderParticlePSO();
    void CreateUpdateParticlePSO();
    void CreateParticleBuffers();
    void CreateConsantBuffer();
    void UpdateUI();

    int m_NumParticles    = 2000;
    int m_ThreadGroupSize = 256;

    RefCntAutoPtr<IPipelineState>         m_pRenderParticlePSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pRenderParticleSRB;
    RefCntAutoPtr<IPipelineState>         m_pResetParticleListsPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pResetParticleListsSRB;
    RefCntAutoPtr<IPipelineState>         m_pMoveParticlesPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pMoveParticlesSRB;
    RefCntAutoPtr<IPipelineState>         m_pCollideParticlesPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pCollideParticlesSRB;
    RefCntAutoPtr<IPipelineState>         m_pUpdateParticleSpeedPSO;
    RefCntAutoPtr<IBuffer>                m_Constants;
    RefCntAutoPtr<IBuffer>                m_pParticleAttribsBuffer;
    RefCntAutoPtr<IBuffer>                m_pParticleListsBuffer;
    RefCntAutoPtr<IBuffer>                m_pParticleListHeadsBuffer;
    RefCntAutoPtr<IResourceMapping>       m_pResMapping;

    float m_fTimeDelta       = 0;
    float m_fSimulationSpeed = 1;
};

} // namespace Diligent
