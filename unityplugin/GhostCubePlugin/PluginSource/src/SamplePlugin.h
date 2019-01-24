#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.h"
#include "RenderAPI.h"
#include "BasicMath.h"

class SamplePlugin
{
public:
    SamplePlugin(Diligent::IRenderDevice *pDevice, bool UseReverseZ, Diligent::TEXTURE_FORMAT RTVFormat, Diligent::TEXTURE_FORMAT DSVFormat);
    void Render(Diligent::IDeviceContext *pContext, const Diligent::float4x4 &ViewProjMatrix);

private:
    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_CubeVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_CubeIndexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> m_VSConstants;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_PSO;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_SRB;
};
