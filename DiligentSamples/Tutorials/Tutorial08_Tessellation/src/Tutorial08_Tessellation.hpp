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

class Tutorial08_Tessellation final : public SampleBase
{
public:
    virtual void GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc) override final;

    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial08: Tessellation"; }

private:
    void CreatePipelineStates();
    void LoadTextures();
    void UpdateUI();

    RefCntAutoPtr<IPipelineState>         m_pPSO[2];
    RefCntAutoPtr<IShaderResourceBinding> m_SRB[2];
    RefCntAutoPtr<IBuffer>                m_ShaderConstants;
    RefCntAutoPtr<ITextureView>           m_HeightMapSRV;
    RefCntAutoPtr<ITextureView>           m_ColorMapSRV;

    float4x4 m_WorldViewProjMatrix;
    float4x4 m_WorldViewMatrix;

    bool  m_Animate              = true;
    bool  m_Wireframe            = false;
    float m_RotationAngle        = 0;
    float m_TessDensity          = 32;
    float m_Distance             = 10.f;
    bool  m_AdaptiveTessellation = true;
    int   m_BlockSize            = 32;

    unsigned int m_HeightMapWidth  = 0;
    unsigned int m_HeightMapHeight = 0;
};

} // namespace Diligent
