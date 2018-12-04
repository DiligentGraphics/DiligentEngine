#include "PlatformBase.h"

// Direct3D 11 implementation of RenderAPI.

#if SUPPORT_D3D11

#include <d3d11.h>
#include <atlbase.h>

#include "RenderAPI.h"
#include "Unity/IUnityGraphicsD3D11.h"
#include "RenderDeviceFactoryD3D11.h"
#include "RenderDeviceD3D11.h"

using namespace Diligent;


class RenderAPI_D3D11 : public RenderAPI
{
public:
	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)override final;

    virtual bool GetUsesReverseZ()override final { return (int)m_d3d11Device->GetFeatureLevel() >= (int)D3D_FEATURE_LEVEL_10_0; }

    virtual bool IsDX()override final { return true; }

    virtual void BeginRendering()override final;

    virtual void AttachToNativeRenderTexture(void *nativeRenderTargetHandle, void *nativeDepthTextureHandle)override final;


private:
	ID3D11Device* m_d3d11Device = nullptr;
};


RenderAPI* CreateRenderAPI_D3D11()
{
	return new RenderAPI_D3D11();
}

void RenderAPI_D3D11::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
    switch (type)
    {
        case kUnityGfxDeviceEventInitialize:
        {
            IUnityGraphicsD3D11* d3d = interfaces->Get<IUnityGraphicsD3D11>();
            m_d3d11Device = d3d->GetDevice();
            CComPtr<ID3D11DeviceContext> d3d11ImmediateContext;
            m_d3d11Device->GetImmediateContext(&d3d11ImmediateContext);
            auto *pFactoryD3d11 = GetEngineFactoryD3D11();
            EngineD3D11Attribs Attribs;
            pFactoryD3d11->AttachToD3D11Device(m_d3d11Device, d3d11ImmediateContext, Attribs, &m_Device, &m_Context, 0);
            break;
        }

        case kUnityGfxDeviceEventShutdown:
            m_Context.Release();
            m_Device.Release();
		    break;
	}
}


void RenderAPI_D3D11::BeginRendering()
{
    ITextureView *RTVs[] = { m_RTV };
    m_Context->SetRenderTargets(1, RTVs, m_DSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderAPI_D3D11::AttachToNativeRenderTexture(void *nativeRenderTargetHandle, void *nativeDepthTextureHandle)
{
    m_RTV.Release();
    m_DSV.Release();

    if (nativeRenderTargetHandle != nullptr && nativeDepthTextureHandle != nullptr)
    {
        RefCntAutoPtr<IRenderDeviceD3D11> pDeviceD3D11(m_Device, IID_RenderDeviceD3D11);

        auto *pd3d11RenderTarget = reinterpret_cast<ID3D11Texture2D *>(nativeRenderTargetHandle);
        RefCntAutoPtr<ITexture> pRenderTarget;
        pDeviceD3D11->CreateTextureFromD3DResource(pd3d11RenderTarget, RESOURCE_STATE_UNDEFINED, &pRenderTarget);

        auto *pd3d11DepthBuffer = reinterpret_cast<ID3D11Texture2D *>(nativeDepthTextureHandle);
        RefCntAutoPtr<ITexture> pDepthBuffer;
        pDeviceD3D11->CreateTextureFromD3DResource(pd3d11DepthBuffer, RESOURCE_STATE_UNDEFINED, &pDepthBuffer);

        CreateTextureViews(pRenderTarget, pDepthBuffer);
    }
}

#endif // #if SUPPORT_D3D11
