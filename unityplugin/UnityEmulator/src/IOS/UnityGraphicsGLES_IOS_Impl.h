#pragma once

#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>

class UnityGraphicsGLES_IOS_Impl
{
public:
    using NativeGLContextType = void*;
    
    ~UnityGraphicsGLES_IOS_Impl();

    void InitGLContext(void *eaglLayer, int majorVersion, int minorVersion);
    void ResizeSwapchain(int NewWidth, int NewHeight);

    void SwapBuffers();
    
    int GetBackBufferWidth()const { return m_BackBufferWidth; }
    int GetBackBufferHeight()const { return m_BackBufferHeight; }
    GLenum GetBackBufferFormat()const { return GL_RGBA8; }
    GLenum GetDepthBufferFormat()const { return GL_DEPTH_COMPONENT32F; }
    NativeGLContextType GetContext() { return m_Context; }
    GLuint GetDefaultFBO()const{return m_DefaultFBO;}
    
private:
    void InitRenderBuffers(bool InitFromDrawable);
    void ReleaseRenderBuffers();
    
    GLint m_BackBufferWidth = 0;
    GLint m_BackBufferHeight = 0;
    GLuint m_ColorRenderBuffer = 0;
    GLuint m_DepthRenderBuffer = 0;
    GLuint m_DefaultFBO = 0;
    void  *m_CALayer;

    NativeGLContextType m_Context;
};

