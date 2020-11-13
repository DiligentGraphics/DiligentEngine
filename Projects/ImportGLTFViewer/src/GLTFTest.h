#pragma once
#include "Actor.h"

namespace Diligent
{

class Cube : public Actor
{
public:
    Cube(const SampleInitInfo& InitInfo);

    void Initialize(const SampleInitInfo& InitInfo) override;

    void RenderActor(const float4x4& CameraViewProj, bool IsShadowPass) override;

    void UpdateActor(double CurrTime, double ElapsedTime) override;

private:
    void CreatePSO() override;
    void CreateVertexBuffer() override;
};

} // namespace Diligent