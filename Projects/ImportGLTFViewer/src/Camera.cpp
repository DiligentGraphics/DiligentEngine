#include "Camera.h"

namespace Diligent
{

Camera::Camera() :
    m_CameraPosition({0, 0, 0, 1}),
    m_CameraRotation({0, 0, 0, 1}),
    m_CameraDist(0.9f),
    m_CameraYaw(0.0f),
    m_CameraPitch(0.0f)
{

}

}