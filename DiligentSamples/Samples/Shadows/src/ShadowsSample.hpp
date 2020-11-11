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
#include "DXSDKMeshLoader.hpp"
#include "FirstPersonCamera.hpp"
#include "ShadowMapManager.hpp"

namespace Diligent
{

#include "BasicStructures.fxh"

class ShadowsSample final : public SampleBase
{
public:
    ~ShadowsSample();
    virtual void GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType,
                                                EngineCreateInfo&  Attribs,
                                                SwapChainDesc&     SCDesc) override final;

    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Shadows Sample"; }

    virtual void WindowResize(Uint32 Width, Uint32 Height) override final;

private:
    void DrawMesh(IDeviceContext* pCtx, bool bIsShadowPass, const struct ViewFrustumExt& Frustum);
    void CreatePipelineStates();
    void InitializeResourceBindings();
    void CreateShadowMap();
    void RenderShadowMap();
    void UpdateUI();

    static void DXSDKMESH_VERTEX_ELEMENTtoInputLayoutDesc(const DXSDKMESH_VERTEX_ELEMENT* VertexElement,
                                                          Uint32                          Stride,
                                                          InputLayoutDesc&                Layout,
                                                          std::vector<LayoutElement>&     Elements);

    struct ShadowSettings
    {
        bool           SnapCascades         = true;
        bool           StabilizeExtents     = true;
        bool           EqualizeExtents      = true;
        bool           SearchBestCascade    = true;
        bool           FilterAcrossCascades = true;
        int            Resolution           = 2048;
        float          PartitioningFactor   = 0.95f;
        TEXTURE_FORMAT Format               = TEX_FORMAT_D16_UNORM;
        int            iShadowMode          = SHADOW_MODE_PCF;

        bool Is32BitFilterableFmt = true;
    } m_ShadowSettings;

    DXSDKMesh m_Mesh;

    LightAttribs      m_LightAttribs;
    FirstPersonCamera m_Camera;
    MouseState        m_LastMouseState;

    ShadowMapManager m_ShadowMapMgr;

    RefCntAutoPtr<IBuffer>                             m_CameraAttribsCB;
    RefCntAutoPtr<IBuffer>                             m_LightAttribsCB;
    std::vector<Uint32>                                m_PSOIndex;
    std::vector<RefCntAutoPtr<IPipelineState>>         m_RenderMeshPSO;
    std::vector<RefCntAutoPtr<IPipelineState>>         m_RenderMeshShadowPSO;
    std::vector<RefCntAutoPtr<IShaderResourceBinding>> m_SRBs;
    std::vector<RefCntAutoPtr<IShaderResourceBinding>> m_ShadowSRBs;

    RefCntAutoPtr<ISampler> m_pComparisonSampler;
    RefCntAutoPtr<ISampler> m_pFilterableShadowMapSampler;
};

} // namespace Diligent
