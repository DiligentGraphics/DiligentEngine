#include "Helmet.h"

namespace Diligent
{

Helmet::Helmet(const SampleInitInfo& InitInfo, BackgroundMode backGroundP)
{
    GLTFObject::Initialize(InitInfo);
    setObjectPath("models/DamagedHelmet/DamagedHelmet.gltf");
    m_BackgroundMode = backGroundP;
}

void Helmet::UpdateActor(double CurrTime, double ElapsedTime)
{
    GLTFObject::UpdateActor(CurrTime, ElapsedTime);

    setPosition(float3(0.0f, sin(static_cast<float>(CurrTime) * 1.0f), 0.0f));
    setRotation(Quaternion::RotationFromAxisAngle(float3(0, 1, 0), static_cast<float>(CurrTime) * 1.0f));
}

} // namespace Diligent