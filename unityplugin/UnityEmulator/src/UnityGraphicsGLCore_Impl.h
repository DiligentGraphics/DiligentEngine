#pragma once

#include "PlatformDefinitions.h"

#if OPENGL_SUPPORTED

#if defined(PLATFORM_WIN32)

#   ifndef GLEW_STATIC
#       define GLEW_STATIC
#   endif
#   include "GL/glew.h"
#   define NOMINMAX
#   include "GL/wglew.h"
#   include <GL/GL.h>

#elif defined(PLATFORM_LINUX)

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
    ~UnityGraphicsGLCore_Impl();

    void InitGLContext(void *pNativeWndHandle, int MajorVersion, int MinorVersion);

    void ResizeSwapchain(int NewWidth, int NewHeight);

    void SwapBuffers();
    
    int GetBackBufferWidth()const { return m_BackBufferWidth; }
    int GetBackBufferHeight()const { return m_BackBufferHeight; }
    GLenum GetBackBufferFormat()const { return GL_RGBA8; }
    GLenum GetDepthBufferFormat()const { return GL_DEPTH_COMPONENT32F; }
    HGLRC GetContext() { return m_Context; }

private:
    int m_BackBufferWidth = 0;
    int m_BackBufferHeight = 0;
    HDC m_WindowHandleToDeviceContext;
    HGLRC m_Context;
};

#endif //OPENGL_SUPPORTED
