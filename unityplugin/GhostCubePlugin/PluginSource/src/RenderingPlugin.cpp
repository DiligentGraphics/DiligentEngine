// Example low level rendering Unity plugin

#include "PlatformBase.h"
#include "Unity/IUnityGraphics.h"
#include "RenderAPI.h"
#include "SamplePlugin.h"

#include <vector>

using namespace Diligent;

static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphics* s_Graphics = nullptr;

static std::unique_ptr<RenderAPI> s_CurrentAPI;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
static std::unique_ptr<SamplePlugin> g_SamplePlugin;


static float4x4 g_Matrix;
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetMatrixFromUnity 
    (float m00, float m01, float m02, float m03,
     float m10, float m11, float m12, float m13,
     float m20, float m21, float m22, float m23,
     float m30, float m31, float m32, float m33)
{ 
    g_Matrix = float4x4( 
        m00, m01, m02, m03,
        m10, m11, m12, m13,
        m20, m21, m22, m23,
        m30, m31, m32, m33); 
}

static void* g_RenderTargetHandle = nullptr;
static void* g_DepthBufferHandle = nullptr;
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTexturesFromUnity(void* renderTargetHandle, void *depthBufferHandle)
{
    if (s_CurrentAPI)
    {
        if (g_RenderTargetHandle != renderTargetHandle ||
            g_DepthBufferHandle != depthBufferHandle)
        {
            g_RenderTargetHandle = renderTargetHandle;
            g_DepthBufferHandle = depthBufferHandle;
            g_SamplePlugin.reset();
            s_CurrentAPI->AttachToNativeRenderTexture(g_RenderTargetHandle, g_DepthBufferHandle);
        }
    }
}


// --------------------------------------------------------------------------
// UnitySetInterfaces

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);


extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
#if (defined(PLATFORM_WIN32) || defined(PLATFORM_UNIVERSAL_WINDOWS)) && (defined(_DEBUG) || defined(DEBUG))
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
	
	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

#if UNITY_WEBGL
typedef void	(UNITY_INTERFACE_API * PluginLoadFunc)(IUnityInterfaces* unityInterfaces);
typedef void	(UNITY_INTERFACE_API * PluginUnloadFunc)();

extern "C" void	UnityRegisterRenderingPlugin(PluginLoadFunc loadPlugin, PluginUnloadFunc unloadPlugin);

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API RegisterPlugin()
{
	UnityRegisterRenderingPlugin(UnityPluginLoad, UnityPluginUnload);
}
#endif

// --------------------------------------------------------------------------
// GraphicsDeviceEvent


void CreateRenderAPI(UnityGfxRenderer apiType)
{
#	if SUPPORT_D3D11
	if (apiType == kUnityGfxRendererD3D11)
	{
		extern RenderAPI* CreateRenderAPI_D3D11();
		s_CurrentAPI.reset( CreateRenderAPI_D3D11() );
	}
    #	endif // if SUPPORT_D3D11

#	if SUPPORT_D3D9
	if (apiType == kUnityGfxRendererD3D9)
	{
        UNSUPPORTED("D3D9 not supported")
	}
#	endif // if SUPPORT_D3D9

#	if SUPPORT_D3D12
	if (apiType == kUnityGfxRendererD3D12)
	{
		extern RenderAPI* CreateRenderAPI_D3D12();
		s_CurrentAPI.reset( CreateRenderAPI_D3D12() );
	}
#	endif // if SUPPORT_D3D9


#	if SUPPORT_OPENGL_UNIFIED
	if (apiType == kUnityGfxRendererOpenGLCore || apiType == kUnityGfxRendererOpenGLES30)
	{
		extern RenderAPI* CreateRenderAPI_OpenGLCoreES(UnityGfxRenderer apiType);
		s_CurrentAPI.reset( CreateRenderAPI_OpenGLCoreES(apiType) );
	}
#	endif // if SUPPORT_OPENGL_UNIFIED

#	if SUPPORT_OPENGL_LEGACY
	if (apiType == kUnityGfxRendererOpenGL)
	{
        UNSUPPORTED("Legacy Opengl not supported");
	}
#	endif // if SUPPORT_OPENGL_LEGACY

#	if SUPPORT_METAL
	if (apiType == kUnityGfxRendererMetal)
	{
        UNSUPPORTED("Metal not supported");
	}
#	endif // if SUPPORT_METAL
}


static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		VERIFY_EXPR(!s_CurrentAPI);
		s_DeviceType = s_Graphics->GetRenderer();
		CreateRenderAPI(s_DeviceType);
        if (s_CurrentAPI)
        {
            s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
        }
	}
    else if (eventType == kUnityGfxDeviceEventShutdown)
	{
        // Cleanup graphics API implementation upon shutdown
        // We must destroy all resources before releasing the API
        g_SamplePlugin.reset();
        if (s_CurrentAPI)
        {
            s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
            s_CurrentAPI.reset();
        }
		s_DeviceType = kUnityGfxRendererNull;
	}
    else if (s_CurrentAPI)
	{
        // Let the implementation process the device related events
		s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
	}
}



// --------------------------------------------------------------------------
// OnRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
    // Unknown / unsupported graphics device type? Do nothing
	if (s_CurrentAPI == nullptr || g_RenderTargetHandle == nullptr || g_DepthBufferHandle == nullptr)
		return;

    s_CurrentAPI->BeginRendering();
    
    if (!g_SamplePlugin)
    {
        auto RTFormat = s_CurrentAPI->GetRenderTargetFormat();
        auto DepthFormat = s_CurrentAPI->GetDepthBufferFormat();
        g_SamplePlugin.reset(new SamplePlugin(s_CurrentAPI->GetDevice(), s_CurrentAPI->GetUsesReverseZ(), RTFormat, DepthFormat));
    }
    g_SamplePlugin->Render(s_CurrentAPI->GetDeviceContext(), g_Matrix );

    s_CurrentAPI->EndRendering();
}


// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}

