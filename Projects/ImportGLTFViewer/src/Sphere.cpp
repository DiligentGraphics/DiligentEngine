#include "Sphere.h"

namespace Diligent
{

Sphere::Sphere(const SampleInitInfo& InitInfo, BackgroundMode backGroundP)
{
    GLTFObject::Initialize(InitInfo);
    setObjectPath("models/MetalRoughSpheres/MetalRoughSpheres.gltf");
    m_BackgroundMode = backGroundP;
}

void Sphere::UpdateActor(double CurrTime, double ElapsedTime)
{
    GLTFObject::UpdateActor(CurrTime, ElapsedTime);

    setRotation(Quaternion::RotationFromAxisAngle(float3(0, 1, 0), static_cast<float>(CurrTime) * 1.0f));
}

} // namespace Diligent
