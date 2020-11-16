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

#include <vector>

#include <stdio.h>

#include "TestScene.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "Sphere.h"
#include "Helmet.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new TestScene();
}

void TestScene::GetEngineInitializationAttribs(RENDER_DEVICE_TYPE DeviceType, EngineCreateInfo& EngineCI, SwapChainDesc& SCDesc)
{
    SampleBase::GetEngineInitializationAttribs(DeviceType, EngineCI, SCDesc);

    EngineCI.Features.DepthClamp = DEVICE_FEATURE_STATE_OPTIONAL;
}

void TestScene::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    Init = InitInfo;

    actors.emplace_back(new Helmet(Init, m_BackgroundMode));
    actors.emplace_back(new Sphere(Init, m_BackgroundMode));

    int i = 0;

    for (auto actor : actors)
    {
        actor->setPosition(float3(0.0f, 0.0f, i * 1.0f));
        i++;
    }
}

void TestScene::ResetView()
{
    camera.m_CameraYaw      = 0;
    camera.m_CameraPitch    = 0;
    camera.m_CameraRotation = Quaternion::RotationFromAxisAngle(float3{0.75f, 0.0f, 0.75f}, PI_F);
}

// Render a frame
void TestScene::Render()
{
    // Bind main back buffer
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    const float ClearColor[] = {0.032f, 0.032f, 0.032f, 1.0f};
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto     SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
    float4x4 CameraView      = camera.m_CameraRotation.ToMatrix() * float4x4::Translation(0.f, 0.0f, camera.m_CameraDist) * SrfPreTransform;
    float4x4 CameraWorld     = CameraView.Inverse();
    float3   CameraWorldPos  = float3::MakeVector(CameraWorld[3]);

    camera.m_CameraWorldPos = CameraWorldPos;

    // Get projection matrix adjusted to the current screen orientation
    camera.m_Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    // Compute world-view-projection matrix
    camera.m_CameraViewProjMatrix = CameraView * camera.m_Proj;

    for (auto actor : actors)
    {
        actor->RenderActor(camera, false);
    }
}

void TestScene::Update(double CurrTime, double ElapsedTime)
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

    // Animate Actors
    for (auto actor : actors)
    {
        actor->Update(CurrTime, ElapsedTime);
    }
}
} // namespace Diligent
