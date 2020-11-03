#pragma once

#include "Sample.h"
#include "Common/interface/BasicMath.hpp"

namespace Diligent
{

class Cube final : public Sample
{
public:
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial02: Cube"; }

private:
    void CreatePipelineState();
    void CreateVertexBuffer();
    void CreateIndexBuffer();

    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
    RefCntAutoPtr<IBuffer>                m_CubeVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_CubeIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_VSConstants;
    float4x4                              m_WorldViewProjMatrix;
};

} // namespace Diligent
