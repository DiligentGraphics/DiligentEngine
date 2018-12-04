#include "PlatformBase.h"

// OpenGL Core profile (desktop) or OpenGL ES (mobile) implementation of RenderAPI.
// Supports several flavors: Core, ES3

#if SUPPORT_OPENGL_UNIFIED

#include "RenderAPI.h"
#include "RenderDeviceFactoryOpenGL.h"
#include "DeviceContextGL.h"
#include "RenderDeviceGL.h"

using namespace Diligent;


class RenderAPI_OpenGLCoreES : public RenderAPI
{
public:
    RenderAPI_OpenGLCoreES(UnityGfxRenderer apiType) : m_APIType(apiType) {}

	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)override final;

	virtual bool GetUsesReverseZ()override final { return false; }

    virtual void BeginRendering()override final;

    virtual bool IsDX()override final { return false; }

    virtual void AttachToNativeRenderTexture(void *nativeRenderTargetHandle, void *nativeDepthTextureHandle)override final;

    void CreateRenderTargetAndDepthBuffer();

private:
	UnityGfxRenderer m_APIType;
    RefCntAutoPtr<IDeviceContextGL> m_DeviceCtxGL;
    Uint32 m_GLRenderTargetHandle = 0;
    Uint32 m_GLDepthTextureHandle = 0;
    bool m_bGLTexturesUpToDate = false;
};


RenderAPI* CreateRenderAPI_OpenGLCoreES(UnityGfxRenderer apiType)
{
	return new RenderAPI_OpenGLCoreES(apiType);
}

void RenderAPI_OpenGLCoreES::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
    if (type == kUnityGfxDeviceEventInitialize)
    {
        auto *pFactoryGL = GetEngineFactoryOpenGL();
        EngineGLAttribs Attribs;
        pFactoryGL->AttachToActiveGLContext(Attribs, &m_Device, &m_Context);
        if (m_Context)
        {
            m_Context->QueryInterface(IID_DeviceContextGL, reinterpret_cast<IObject**>(static_cast<IDeviceContextGL**>(&m_DeviceCtxGL)));
        }
    }
    else if (type == kUnityGfxDeviceEventShutdown)
    {
        m_Context.Release();
        m_Device.Release();
    }
}

void RenderAPI_OpenGLCoreES::CreateRenderTargetAndDepthBuffer()
{
    RefCntAutoPtr<IRenderDeviceGL> pDeviceGL(m_Device, IID_RenderDeviceGL);

    m_RTV.Release();
    m_DSV.Release();

    RefCntAutoPtr<ITexture> pRenderTarget;
    TextureDesc RenderTargetDesc;
    RenderTargetDesc.Type = RESOURCE_DIM_TEX_2D;
    RenderTargetDesc.BindFlags = BIND_RENDER_TARGET;
    RenderTargetDesc.Usage = USAGE_DEFAULT;
    RenderTargetDesc.Name = "Unity render target";
    RenderTargetDesc.MipLevels = 1;
#if PLATFORM_IOS
    // Texture parameter queries are not supported in GLES3.0
    RenderTargetDesc.Width = 1024;
    RenderTargetDesc.Height = 1024;
    RenderTargetDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
#endif
    pDeviceGL->CreateTextureFromGLHandle(m_GLRenderTargetHandle, RenderTargetDesc, RESOURCE_STATE_UNKNOWN, &pRenderTarget);
        
    RefCntAutoPtr<ITexture> pDepthBuffer;
    TextureDesc DepthBufferDesc;
    DepthBufferDesc.Type = RESOURCE_DIM_TEX_2D;
    DepthBufferDesc.BindFlags = BIND_DEPTH_STENCIL;
    DepthBufferDesc.Usage = USAGE_DEFAULT;
    DepthBufferDesc.Name = "Unity depth buffer";
    DepthBufferDesc.MipLevels = 1;
#if PLATFORM_IOS
    // Texture parameter queries are not supported in GLES3.0
    DepthBufferDesc.Width = 1024;
    DepthBufferDesc.Height = 1024;
    DepthBufferDesc.Format = TEX_FORMAT_D32_FLOAT;
#endif
    pDeviceGL->CreateTextureFromGLHandle(m_GLDepthTextureHandle, DepthBufferDesc, RESOURCE_STATE_UNKNOWN, &pDepthBuffer);

    CreateTextureViews(pRenderTarget, pDepthBuffer);
}

void RenderAPI_OpenGLCoreES::BeginRendering()
{
    if(!m_DeviceCtxGL->UpdateCurrentGLContext())
        return;

    RenderAPI::BeginRendering();

    if (!m_bGLTexturesUpToDate)
    {
        CreateRenderTargetAndDepthBuffer();
        m_bGLTexturesUpToDate = true;
    }

    ITextureView *RTVs[] = { m_RTV };
    m_Context->SetRenderTargets(1, RTVs, m_DSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderAPI_OpenGLCoreES::AttachToNativeRenderTexture(void *nativeRenderTargetHandle, void *nativeDepthTextureHandle)
{
    m_GLRenderTargetHandle = static_cast<Uint32>(reinterpret_cast<size_t>(nativeRenderTargetHandle));
    m_GLDepthTextureHandle = static_cast<Uint32>(reinterpret_cast<size_t>(nativeDepthTextureHandle));
    m_bGLTexturesUpToDate = false;
    // There is no active OpenGL context when this function is called for the first time,
    // so we cannot create RTV and DSV here
}

#endif // #if SUPPORT_OPENGL_UNIFIED
