#pragma once
#include "Actor.h"

namespace Diligent
{

class Plane : public Actor
{
public:
    Plane(const SampleInitInfo& InitInfo);

    void Initialize(const SampleInitInfo& InitInfo) override;

    void RenderActor(const float4x4& CameraViewProj, bool IsShadowPass) override;

private:
    void CreatePSO() override;
};

} // namespace Diligent