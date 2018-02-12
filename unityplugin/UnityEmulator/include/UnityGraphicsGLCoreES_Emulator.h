#pragma once

#include "PlatformDefinitions.h"

#if OPENGL_SUPPORTED

#include <memory>
#include "UnityGraphicsEmulator.h"

#if PLATFORM_ANDROID
    class UnityGraphicsGLESAndroid_Impl;
    using UnityGraphicsGL_Impl = UnityGraphicsGLESAndroid_Impl;
#elif PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_LINUX || PLATFORM_MACOS
    class UnityGraphicsGLCore_Impl;
    using UnityGraphicsGL_Impl = UnityGraphicsGLCore_Impl;
#elif PLATFORM_IOS
    class UnityGraphicsGLES_IOS_Impl;
    using UnityGraphicsGL_Impl = UnityGraphicsGLES_IOS_Impl;
#else
#   error Unknown Platform
#endif


class UnityGraphicsGLCoreES_Emulator final : public UnityGraphicsEmulator
{
public:
    static UnityGraphicsGLCoreES_Emulator& GetInstance();
    void InitGLContext(void *pNativeWndHandle, 
                       #if PLATFORM_LINUX
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
    virtual void GetBackBufferSize(unsigned int &Width, unsigned int &Height)override final;

private:
    UnityGraphicsGLCoreES_Emulator();
    static std::unique_ptr<UnityGraphicsGL_Impl> m_GraphicsImpl;
};

#endif // OPENGL_SUPPORTED
