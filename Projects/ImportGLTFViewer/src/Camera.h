#pragma once
#include "BasicMath.hpp"
#include "InputController.hpp"

namespace Diligent
{

class Camera
{
public:
    Camera();

    float4     m_CameraPosition = {0, 0, 0, 1};
    Quaternion m_CameraRotation = {0, 0, 0, 1};
    float4x4   m_Proj;
    float4x4   m_CameraViewProjMatrix;
    float3     m_CameraWorldPos;
    float      m_CameraDist  = 0.9f;
    float      m_CameraYaw   = 0;
    float      m_CameraPitch = 0;

private:
    InputController m_InputController;
    MouseState m_LastMouseState;
};

}