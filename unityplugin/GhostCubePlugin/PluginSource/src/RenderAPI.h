#pragma once

#include "Unity/IUnityGraphics.h"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"

struct IUnityInterfaces;

class RenderAPI
{
public:
	virtual ~RenderAPI() { }

	// Process general event like initialization, shutdown, device loss/reset etc.
	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;

    virtual void AttachToNativeRenderTexture(void *nativeRenderTargetHandle, void *nativeDepthTextureHandle) = 0;

	// Is the API using "reversed" (1.0 at near plane, 0.0 at far plane) depth buffer?
	// Reversed Z is used on modern platforms, and improves depth buffer precision.
	virtual bool GetUsesReverseZ() = 0;

    virtual void BeginRendering() {}

    virtual void EndRendering() 
    {
        if (m_Context)
            m_Context->InvalidateState();
    }
    
    virtual bool IsDX() = 0;

    Diligent::IRenderDevice *GetDevice() { return m_Device; }
    Diligent::IDeviceContext *GetDeviceContext() { return m_Context;  }

    Diligent::TEXTURE_FORMAT GetRenderTargetFormat()const { return m_RenderTargetFormat; };
    Diligent::TEXTURE_FORMAT GetDepthBufferFormat()const { return m_DepthBufferFormat; };

    void CreateTextureViews(Diligent::ITexture *pRenderTarget, Diligent::ITexture *pDepthBuffer);

protected:
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_Device;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_Context;

    Diligent::RefCntAutoPtr<Diligent::ITextureView> m_RTV;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> m_DSV;

    Diligent::TEXTURE_FORMAT m_RenderTargetFormat = Diligent::TEX_FORMAT_UNKNOWN;
    Diligent::TEXTURE_FORMAT m_DepthBufferFormat = Diligent::TEX_FORMAT_UNKNOWN;
};
