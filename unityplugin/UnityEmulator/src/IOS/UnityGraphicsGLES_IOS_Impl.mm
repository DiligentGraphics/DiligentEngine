
#include "UnityGraphicsGLES_IOS_Impl.h"
#include "Errors.h"
#include "DebugUtilities.h"

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>


UnityGraphicsGLES_IOS_Impl::~UnityGraphicsGLES_IOS_Impl()
{
    ReleaseRenderBuffers();
}

void UnityGraphicsGLES_IOS_Impl::InitGLContext(void *eaglLayer, int majorVersion, int minorVersion)
{
    m_CALayer = eaglLayer;
    
    EAGLContext* CurrentCtx = [EAGLContext currentContext];
    m_Context = (__bridge void*)CurrentCtx;
    
    if (m_Context == nullptr)
    {
        LOG_ERROR_AND_THROW("No current GL context found!");
    }
    
    //Checking GL version
    const GLubyte *GLVersionString = glGetString( GL_VERSION );
    const GLubyte *GLRenderer = glGetString(GL_RENDERER);
    
    GLint MajorVersion = 0, MinorVersion = 0;
    //Or better yet, use the GL3 way to get the version number
    glGetIntegerv( GL_MAJOR_VERSION, &MajorVersion );
    glGetIntegerv( GL_MINOR_VERSION, &MinorVersion );
    LOG_INFO_MESSAGE("Attached to OpenGLES ", MajorVersion, '.', MinorVersion, " context (", GLVersionString, ", ", GLRenderer, ')');

    InitRenderBuffers(true);
}

void UnityGraphicsGLES_IOS_Impl::ReleaseRenderBuffers()
{
    if(m_DefaultFBO != 0)
    {
        glDeleteFramebuffers(1, &m_DefaultFBO);
        m_DefaultFBO = 0;
    }
    
    if(m_ColorRenderBuffer != 0)
    {
        glDeleteRenderbuffers(1, &m_ColorRenderBuffer);
        m_ColorRenderBuffer = 0;
    }
    
    if(m_DepthRenderBuffer != 0)
    {
        glDeleteRenderbuffers(1, &m_DepthRenderBuffer);
        m_DepthRenderBuffer = 0;
    }
}

void UnityGraphicsGLES_IOS_Impl::InitRenderBuffers(bool InitFromDrawable)
{
    ReleaseRenderBuffers();
    
    EAGLContext* context = [EAGLContext currentContext];
    
    glGenFramebuffers(1, &m_DefaultFBO);
    VERIFY(glGetError() == GL_NO_ERROR, "Failed to generate default framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, m_DefaultFBO);
    
    glGenRenderbuffers(1, &m_ColorRenderBuffer);
    VERIFY(glGetError() == GL_NO_ERROR, "Failed to generate color renderbuffer");
    glBindRenderbuffer(GL_RENDERBUFFER, m_ColorRenderBuffer);
    
    if(InitFromDrawable)
    {
        // This call associates the storage for the current render buffer with the
        // EAGLDrawable (our CAEAGLLayer) allowing us to draw into a buffer that
        // will later be rendered to the screen wherever the layer is (which
        // corresponds with our view).
        id<EAGLDrawable> drawable = (__bridge id<EAGLDrawable>)m_CALayer;
        [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:drawable];
    }
    else
    {
        CAEAGLLayer* layer = (__bridge CAEAGLLayer*)m_CALayer;
        [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
    }
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_ColorRenderBuffer);
    
    // Get the drawable buffer's width and height so we can create a depth buffer for the FBO
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_BackBufferWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_BackBufferHeight);
    
    // Create a depth buffer to use with our drawable FBO
    glGenRenderbuffers(1, &m_DepthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRenderBuffer);
    VERIFY(glGetError() == GL_NO_ERROR, "Failed to generate depth renderbuffer");
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_BackBufferWidth, m_BackBufferHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthRenderBuffer);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR_AND_THROW("Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
}

void UnityGraphicsGLES_IOS_Impl::ResizeSwapchain(int NewWidth, int NewHeight)
{
    InitRenderBuffers(false);
}

void UnityGraphicsGLES_IOS_Impl::SwapBuffers()
{
    EAGLContext* context = [EAGLContext currentContext];
    glBindRenderbuffer(GL_RENDERBUFFER, m_ColorRenderBuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER];
}

