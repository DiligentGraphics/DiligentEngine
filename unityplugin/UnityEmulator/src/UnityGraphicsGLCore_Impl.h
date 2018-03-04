#pragma once

#include "PlatformDefinitions.h"

#if GL_SUPPORTED

#if PLATFORM_WIN32

#   ifndef GLEW_STATIC
#       define GLEW_STATIC
#   endif
#   include "GL/glew.h"
#   define NOMINMAX
#   include "GL/wglew.h"
#   include <GL/GL.h>

#elif PLATFORM_LINUX

#   ifndef GLEW_STATIC
#       define GLEW_STATIC // Must be defined to use static version of glew
#   endif
#   ifndef GLEW_NO_GLU
#       define GLEW_NO_GLU
#   endif

#   include "GL/glew.h"
#   include <GL/glx.h>

// Undefine beautiful defines from GL/glx.h -> X11/Xlib.h
#   ifdef Bool
#       undef Bool
#   endif
#   ifdef True
#       undef True
#   endif
#   ifdef False
#       undef False
#   endif
#   ifdef Status
#       undef Status
#   endif
#   ifdef Success
#       undef Success
#   endif

#elif PLATFORM_MACOS

#   ifndef GLEW_STATIC
#       define GLEW_STATIC // Must be defined to use static version of glew
#   endif
#   ifndef GLEW_NO_GLU
#       define GLEW_NO_GLU
#   endif

#   include "GL/glew.h"

#else

#   error Unsupported platform

#endif

class UnityGraphicsGLCore_Impl
{
public:

#if PLATFORM_WIN32
    typedef HGLRC NativeGLContextType;
#elif PLATFORM_LINUX
    typedef GLXContext NativeGLContextType;
#elif PLATFORM_MACOS
    typedef void* NativeGLContextType;
#else
#   error Unsupported platform
#endif

    ~UnityGraphicsGLCore_Impl();

    void InitGLContext(void *pNativeWndHandle, 
                       #if PLATFORM_LINUX
                           void *pDisplay,
                       #endif
                       int MajorVersion, int MinorVersion);

    void ResizeSwapchain(int NewWidth, int NewHeight);

    void SwapBuffers();
    
    int GetBackBufferWidth()const { return m_BackBufferWidth; }
    int GetBackBufferHeight()const { return m_BackBufferHeight; }
    GLenum GetBackBufferFormat()const { return GL_RGBA8; }
    GLenum GetDepthBufferFormat()const { return GL_DEPTH_COMPONENT32F; }
    NativeGLContextType GetContext() { return m_Context; }
    GLuint GetDefaultFBO()const{return 0;}

private:
    int m_BackBufferWidth = 0;
    int m_BackBufferHeight = 0;

#if PLATFORM_WIN32
    HDC m_WindowHandleToDeviceContext;
#elif PLATFORM_LINUX
    Window m_LinuxWindow;
    Display *m_Display;
#elif PLATFORM_MACOS
    
#else
#   error Unsupported platform
#endif
    NativeGLContextType m_Context;
};

#endif // GL_SUPPORTED
