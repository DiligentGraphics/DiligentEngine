#pragma once

#include "PlatformDefinitions.h"

#if OPENGL_SUPPORTED

#define GLEW_STATIC
#include "glew.h"
#define NOMINMAX
#include "wglew.h"
#include <GL/GL.h>

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
