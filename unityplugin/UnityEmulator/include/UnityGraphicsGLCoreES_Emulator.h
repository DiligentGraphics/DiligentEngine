#pragma once

#include "PlatformDefinitions.h"

#if OPENGL_SUPPORTED

#include <memory>
#include "UnityGraphicsEmulator.h"

#if defined(PLATFORM_ANDROID)
    class UnityGraphicsGLESAndroid_Impl;
    using UnityGraphicsGL_Impl = UnityGraphicsGLESAndroid_Impl;
#elif defined(PLATFORM_WIN32) || defined(PLATFORM_UNIVERSAL_WINDOWS) || defined(PLATFORM_LINUX)
    class UnityGraphicsGLCore_Impl;
    using UnityGraphicsGL_Impl = UnityGraphicsGLCore_Impl;
#else
#   error Unknown Platform
#endif


class UnityGraphicsGLCoreES_Emulator : public UnityGraphicsEmulator
{
public:
    static UnityGraphicsGLCoreES_Emulator& GetInstance();
    void InitGLContext(void *pNativeWndHandle, 
                       #ifdef PLATFORM_LINUX
                           void *pDisplay,
                       #endif
                       int MajorVersion, int MinorVersion);

    virtual void Present()override final;
    virtual void Release()override final;
    virtual void BeginFrame()override final;
    virtual void EndFrame()override final;
    virtual void ResizeSwapChain(unsigned int Width, unsigned int Height)override final;
    virtual IUnityInterface* GetUnityGraphicsAPIInterface()override final;
    static UnityGraphicsGL_Impl* GetGraphicsImpl();
    virtual UnityGfxRenderer GetUnityGfxRenderer()override final;
    virtual bool SwapChainInitialized()override final;

private:
    UnityGraphicsGLCoreES_Emulator();
    static std::unique_ptr<UnityGraphicsGL_Impl> m_GraphicsImpl;
};

#endif // OPENGL_SUPPORTED