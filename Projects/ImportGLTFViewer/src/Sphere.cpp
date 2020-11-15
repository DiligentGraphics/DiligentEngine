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
#include <array>
#include "Sphere.h"
#include "MapHelper.hpp"
#include "BasicMath.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "CommonlyUsedStates.h"
#include "ShaderMacroHelper.hpp"
#include "FileSystem.hpp"
#include "imgui.h"
#include "imGuIZMO.h"

namespace Diligent
{

#include "Shaders/Common/public/BasicStructures.fxh"
#include "Shaders/PostProcess/ToneMapping/public/ToneMappingStructures.fxh"

namespace
{

struct EnvMapRenderAttribs
{
    ToneMappingAttribs TMAttribs;

    float AverageLogLum;
    float MipLevel;
    float Unusued1;
    float Unusued2;
};

} // namespace

Sphere::Sphere(const SampleInitInfo& InitInfo)
{
    Initialize(InitInfo);
}

void Sphere::LoadModel(const char* Path)
{
    if (m_Model)
    {
        m_GLTFRenderer->ReleaseResourceBindings(*m_Model);
        m_PlayAnimation  = false;
        m_AnimationIndex = 0;
        m_AnimationTimers.clear();
    }

    m_Model.reset(new GLTF::Model(m_pDevice, m_pImmediateContext, Path));
    m_GLTFRenderer->InitializeResourceBindings(*m_Model, m_VertexBuffer, m_VSConstants);

    // Center and scale model
    float3 ModelDim{m_Model->AABBTransform[0][0], m_Model->AABBTransform[1][1], m_Model->AABBTransform[2][2]};
    float  Scale     = (1.0f / std::max(std::max(ModelDim.x, ModelDim.y), ModelDim.z)) * 0.5f;
    auto   Translate = -float3(m_Model->AABBTransform[3][0], m_Model->AABBTransform[3][1], m_Model->AABBTransform[3][2]);
    Translate += -0.5f * ModelDim;
    float4x4 InvYAxis = float4x4::Identity();
    InvYAxis._22      = -1;
    scale             = Scale;
    position          = Translate;
    m_ContextInit     = InvYAxis;

    if (!m_Model->Animations.empty())
    {
        m_AnimationTimers.resize(m_Model->Animations.size());
        m_AnimationIndex = 0;
        m_PlayAnimation  = true;
    }
}

void Sphere::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    RefCntAutoPtr<ITexture> EnvironmentMap;
    CreateTextureFromFile("textures/papermill.ktx", TextureLoadInfo{"Environment map"}, m_pDevice, &EnvironmentMap);
    m_TextureSRV = EnvironmentMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    auto BackBufferFmt  = m_pSwapChain->GetDesc().ColorBufferFormat;
    auto DepthBufferFmt = m_pSwapChain->GetDesc().DepthBufferFormat;

    GLTF_PBR_Renderer::CreateInfo RendererCI;
    RendererCI.RTVFmt         = BackBufferFmt;
    RendererCI.DSVFmt         = DepthBufferFmt;
    RendererCI.AllowDebugView = true;
    RendererCI.UseIBL         = true;
    RendererCI.FrontCCW       = true;
    m_GLTFRenderer.reset(new GLTF_PBR_Renderer(m_pDevice, m_pImmediateContext, RendererCI));

    CreateUniformBuffer(m_pDevice, sizeof(CameraAttribs), "Camera attribs buffer", &m_VertexBuffer);
    CreateUniformBuffer(m_pDevice, sizeof(LightAttribs), "Light attribs buffer", &m_VSConstants);
    CreateUniformBuffer(m_pDevice, sizeof(EnvMapRenderAttribs), "Env map render attribs buffer", &m_IndexBuffer);
    // clang-format off
    StateTransitionDesc Barriers [] =
    {
        {m_VertexBuffer, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true},
        {m_VSConstants,  RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true},
        {m_IndexBuffer,  RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true},
        {EnvironmentMap, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true}
    };
    // clang-format on
    m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);

    m_GLTFRenderer->PrecomputeCubemaps(m_pDevice, m_pImmediateContext, m_TextureSRV);

    CreatePSO();

    m_LightDirection = normalize(float3(0.5f, -0.6f, -0.2f));

    LoadModel("models/MetalRoughSpheres/MetalRoughSpheres.gltf");
}

void Sphere::CreatePSO()
{
    ShaderCreateInfo                               ShaderCI;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("shaders", &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.UseCombinedTextureSamplers = true;

    ShaderMacroHelper Macros;
    Macros.AddShaderMacro("TONE_MAPPING_MODE", "TONE_MAPPING_MODE_UNCHARTED2");
    ShaderCI.Macros = Macros;

    ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
    ShaderCI.Desc.Name       = "Environment map VS";
    ShaderCI.EntryPoint      = "main";
    ShaderCI.FilePath        = "env_map.vsh";
    RefCntAutoPtr<IShader> pVS;
    m_pDevice->CreateShader(ShaderCI, &pVS);

    ShaderCI.Desc.Name       = "Environment map PS";
    ShaderCI.EntryPoint      = "main";
    ShaderCI.FilePath        = "env_map.psh";
    ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
    RefCntAutoPtr<IShader> pPS;
    m_pDevice->CreateShader(ShaderCI, &pPS);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    // clang-format off
    ImmutableSamplerDesc ImmutableSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "EnvMap", Sam_LinearClamp}
    };
    // clang-format on
    PSODesc.ResourceLayout.ImmutableSamplers    = ImmutableSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);

    // clang-format off
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "cbCameraAttribs",       SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
        {SHADER_TYPE_PIXEL, "cbEnvMapRenderAttribs", SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
    };
    // clang-format on
    PSODesc.ResourceLayout.Variables    = Vars;
    PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    PSODesc.Name      = "EnvMap PSO";
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    GraphicsPipeline.RTVFormats[0]              = m_pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.NumRenderTargets           = 1;
    GraphicsPipeline.DSVFormat                  = m_pSwapChain->GetDesc().DepthBufferFormat;
    GraphicsPipeline.PrimitiveTopology          = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "cbCameraAttribs")->Set(m_VertexBuffer);
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "cbEnvMapRenderAttribs")->Set(m_IndexBuffer);
    CreateVertexBuffer();
}

void Sphere::CreateVertexBuffer()
{
    if (m_BackgroundMode != BackgroundMode::None)
    {
        m_SRB.Release();
        m_pPSO->CreateShaderResourceBinding(&m_SRB, true);
        ITextureView* pEnvMapSRV = nullptr;
        switch (m_BackgroundMode)
        {
            case BackgroundMode::EnvironmentMap:
                pEnvMapSRV = m_TextureSRV;
                break;

            case BackgroundMode::Irradiance:
                pEnvMapSRV = m_GLTFRenderer->GetIrradianceCubeSRV();
                break;

            case BackgroundMode::PrefilteredEnvMap:
                pEnvMapSRV = m_GLTFRenderer->GetPrefilteredEnvMapSRV();
                break;

            default:
                UNEXPECTED("Unexpected background mode");
        }
        m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "EnvMap")->Set(pEnvMapSRV);
    }
}

// Render a frame
void Sphere::RenderActor(const Camera cameraP, bool IsShadowPass)
{
    m_RenderParams.ModelTransform = m_WorldMatrix;

    camera = cameraP;

    {
        MapHelper<CameraAttribs> CamAttribs(m_pImmediateContext, m_VertexBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        CamAttribs->mProjT        = camera.m_Proj.Transpose();
        CamAttribs->mViewProjT    = camera.m_CameraViewProjMatrix.Transpose();
        CamAttribs->mViewProjInvT = camera.m_CameraViewProjMatrix.Inverse().Transpose();
        CamAttribs->f4Position    = float4(camera.m_CameraWorldPos, 1);

        if (m_BoundBoxMode != BoundBoxMode::None)
        {
            float4x4 BBTransform;
            if (m_BoundBoxMode == BoundBoxMode::Local)
            {
                BBTransform = m_Model->AABBTransform * m_RenderParams.ModelTransform;
            }
            else if (m_BoundBoxMode == BoundBoxMode::Global)
            {
                auto TransformedBB = BoundBox{m_Model->dimensions.min, m_Model->dimensions.max}.Transform(m_RenderParams.ModelTransform);
                BBTransform        = float4x4::Scale(TransformedBB.Max - TransformedBB.Min) * float4x4::Translation(TransformedBB.Min);
            }
            else
            {
                UNEXPECTED("Unexpected bound box mode");
            }


            for (int row = 0; row < 4; ++row)
                CamAttribs->f4ExtraData[row] = float4::MakeVector(BBTransform[row]);
        }
    }

    {
        MapHelper<LightAttribs> lightAttribs(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        lightAttribs->f4Direction = m_LightDirection;
        lightAttribs->f4Intensity = m_LightColor * m_LightIntensity;
    }

    m_GLTFRenderer->Render(m_pImmediateContext, *m_Model, m_RenderParams);

    if (m_BackgroundMode != BackgroundMode::None)
    {
        {
            MapHelper<EnvMapRenderAttribs> EnvMapAttribs(m_pImmediateContext, m_IndexBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
            EnvMapAttribs->TMAttribs.iToneMappingMode     = TONE_MAPPING_MODE_UNCHARTED2;
            EnvMapAttribs->TMAttribs.bAutoExposure        = 0;
            EnvMapAttribs->TMAttribs.fMiddleGray          = m_RenderParams.MiddleGray;
            EnvMapAttribs->TMAttribs.bLightAdaptation     = 0;
            EnvMapAttribs->TMAttribs.fWhitePoint          = m_RenderParams.WhitePoint;
            EnvMapAttribs->TMAttribs.fLuminanceSaturation = 1.0;
            EnvMapAttribs->AverageLogLum                  = m_RenderParams.AverageLogLum;
            EnvMapAttribs->MipLevel                       = m_EnvMapMipLevel;
        }
        m_pImmediateContext->SetPipelineState(m_pPSO);
        m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_VERIFY);
        DrawAttribs drawAttribs(3, DRAW_FLAG_VERIFY_ALL);
        m_pImmediateContext->Draw(drawAttribs);
    }
}

void Sphere::UpdateActor(double CurrTime, double ElapsedTime)
{
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

        constexpr float RotationSpeed = 0.005f;

        float fYawDelta   = MouseDeltaX * RotationSpeed;
        float fPitchDelta = MouseDeltaY * RotationSpeed;
        if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT)
        {
            camera.m_CameraYaw += fYawDelta;
            camera.m_CameraPitch += fPitchDelta;
            camera.m_CameraPitch = std::max(camera.m_CameraPitch, -PI_F / 2.f);
            camera.m_CameraPitch = std::min(camera.m_CameraPitch, +PI_F / 2.f);
        }

        // Apply extra rotations to adjust the view to match Khronos GLTF viewer
        camera.m_CameraRotation =
            Quaternion::RotationFromAxisAngle(float3{1, 0, 0}, -camera.m_CameraPitch) *
            Quaternion::RotationFromAxisAngle(float3{0, 1, 0}, -camera.m_CameraYaw) *
            Quaternion::RotationFromAxisAngle(float3{0.75f, 0.0f, 0.75f}, PI_F);

        if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT)
        {
            auto CameraView  = camera.m_CameraRotation.ToMatrix();
            auto CameraWorld = CameraView.Transpose();

            float3 CameraRight = float3::MakeVector(CameraWorld[0]);
            float3 CameraUp    = float3::MakeVector(CameraWorld[1]);
        }

        camera.m_CameraDist -= mouseState.WheelDelta * 0.25f;
        camera.m_CameraDist = clamp(camera.m_CameraDist, 0.1f, 5.f);
    }
    SampleBase::Update(CurrTime, ElapsedTime);

    setRotation(Quaternion::RotationFromAxisAngle(float3(0, 1, 0), static_cast<float>(CurrTime) * 1.0f));

    if (!m_Model->Animations.empty() && m_PlayAnimation)
    {
        float& AnimationTimer = m_AnimationTimers[m_AnimationIndex];
        AnimationTimer += static_cast<float>(ElapsedTime);
        AnimationTimer = std::fmod(AnimationTimer, m_Model->Animations[m_AnimationIndex].End);
        m_Model->UpdateAnimation(m_AnimationIndex, AnimationTimer);
    }
}

} // namespace Diligent
