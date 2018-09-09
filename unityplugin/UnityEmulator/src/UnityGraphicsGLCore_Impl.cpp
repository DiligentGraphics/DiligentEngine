
#include "UnityGraphicsGLCore_Impl.h"

#if GL_SUPPORTED

#include <iostream>
#include "DebugUtilities.h"
#include "Errors.h"

#if PLATFORM_MACOS
#import <AppKit/AppKit.h>
#endif

#if PLATFORM_WIN32 || PLATFORM_LINUX

#ifndef APIENTRY
#   define APIENTRY
#endif

void APIENTRY openglCallbackFunction( GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam )
{
    if (id == 131185)
        return;

    std::stringstream MessageSS;

    MessageSS << "OpenGL debug message (";
    switch( type ) 
    {
        case GL_DEBUG_TYPE_ERROR:
            MessageSS << "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            MessageSS << "DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            MessageSS << "UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            MessageSS << "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            MessageSS << "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_OTHER:
            MessageSS << "OTHER";
            break;
    }

    switch( severity )
    {
        case GL_DEBUG_SEVERITY_LOW:
            MessageSS << ", low severity";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            MessageSS << ", medium severity";
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            MessageSS << ", HIGH severity";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            MessageSS << ", notification";
            break;
    }

    MessageSS << ")" << std::endl << message << std::endl;

#if PLATFORM_WIN32
    OutputDebugStringA( MessageSS.str().c_str() );
#elif PLATFORM_LINUX
    std::cout << MessageSS.str().c_str();
#else
#   error Unknown platform
#endif
}

#endif // WIN32 or LINUX

UnityGraphicsGLCore_Impl::~UnityGraphicsGLCore_Impl()
{
    if( m_Context )
	{
#if PLATFORM_WIN32
		wglMakeCurrent( m_WindowHandleToDeviceContext, 0 );
        wglDeleteContext( m_Context );
#elif PLATFORM_LINUX || PLATFORM_MACOS
        // Do nothing. Context is managed by the app
#else
#   error Unknown platform
#endif
	}
}

void UnityGraphicsGLCore_Impl::InitGLContext(void *pNativeWndHandle, 
                                             #if PLATFORM_LINUX
                                                 void *pDisplay,
                                             #endif
                                             int MajorVersion, int MinorVersion )
{
#if PLATFORM_WIN32
	HWND hWnd = reinterpret_cast<HWND>(pNativeWndHandle);
	RECT rc;
	GetClientRect( hWnd, &rc );
	m_BackBufferWidth = rc.right - rc.left;
	m_BackBufferHeight = rc.bottom - rc.top;

	// See http://www.opengl.org/wiki/Tutorial:_OpenGL_3.1_The_First_Triangle_(C%2B%2B/Win)
	//     http://www.opengl.org/wiki/Creating_an_OpenGL_Context_(WGL)
	PIXELFORMATDESCRIPTOR pfd;
	memset( &pfd, 0, sizeof( PIXELFORMATDESCRIPTOR ) );
	pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
            
	m_WindowHandleToDeviceContext = GetDC( hWnd );
	int nPixelFormat = ChoosePixelFormat( m_WindowHandleToDeviceContext, &pfd );

	if( nPixelFormat == 0 )
		LOG_ERROR_AND_THROW( "Invalid Pixel Format" );

	BOOL bResult = SetPixelFormat( m_WindowHandleToDeviceContext, nPixelFormat, &pfd );
	if( !bResult )
		LOG_ERROR_AND_THROW( "Failed to set Pixel Format" );

	// Create standard OpenGL (2.1) rendering context which will be used only temporarily, 
	HGLRC tempContext = wglCreateContext( m_WindowHandleToDeviceContext );
	// and make it current
	wglMakeCurrent( m_WindowHandleToDeviceContext, tempContext );

	// Initialize GLEW
	GLenum err = glewInit();
	if( GLEW_OK != err )
		LOG_ERROR_AND_THROW( "Failed to initialize GLEW" );
        
	if( wglewIsSupported( "WGL_ARB_create_context" ) == 1 )
	{
		// Setup attributes for a new OpenGL rendering context
		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, MajorVersion,
			WGL_CONTEXT_MINOR_VERSION_ARB, MinorVersion,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			GL_CONTEXT_PROFILE_MASK, GL_CONTEXT_CORE_PROFILE_BIT,
			0, 0
		};

#ifdef _DEBUG
		attribs[5] |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif 

		// Create new rendering context
		// In order to create new OpenGL rendering context we have to call function wglCreateContextAttribsARB(), 
		// which is an OpenGL function and requires OpenGL to be active when it is called. 
		// The only way is to create an old context, activate it, and while it is active create a new one. 
		// Very inconsistent, but we have to live with it!
		m_Context = wglCreateContextAttribsARB( m_WindowHandleToDeviceContext, 0, attribs );

		// Delete tempContext
		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( tempContext );
		// 
		wglMakeCurrent( m_WindowHandleToDeviceContext, m_Context );
		wglSwapIntervalEXT( 0 );
	}
	else
	{       //It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
		m_Context = tempContext;
	}
#elif PLATFORM_LINUX
    m_LinuxWindow = static_cast<Window>(reinterpret_cast<size_t>(pNativeWndHandle));
    m_Display = reinterpret_cast<Display*>(pDisplay);
    m_Context = glXGetCurrentContext();
    if(m_Context == nullptr)
        LOG_ERROR_AND_THROW( "No active GL context found" );

	// Initialize GLEW
	GLenum err = glewInit();
	if( GLEW_OK != err )
		LOG_ERROR_AND_THROW( "Failed to initialize GLEW" );
#elif PLATFORM_MACOS
    NSOpenGLContext* CurrentCtx = [NSOpenGLContext currentContext];
    m_Context = CurrentCtx;
    if (m_Context == nullptr)
    {
        LOG_ERROR_AND_THROW("No current GL context found!");
    }
    // Initialize GLEW
    GLenum err = glewInit();
    if( GLEW_OK != err )
        LOG_ERROR_AND_THROW( "Failed to initialize GLEW" );
#else
#   error Unsupported platform
#endif

    //Checking GL version
    const GLubyte *GLVersionString = glGetString( GL_VERSION );

    //Or better yet, use the GL3 way to get the version number
    glGetIntegerv( GL_MAJOR_VERSION, &MajorVersion );
    glGetIntegerv( GL_MINOR_VERSION, &MinorVersion );
    LOG_INFO_MESSAGE("Initialized OpenGL ", MajorVersion, '.', MinorVersion, " context (", GLVersionString, ")");

#if PLATFORM_WIN32 || PLATFORM_LINUX
    if( glDebugMessageCallback )
    {
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        glDebugMessageCallback( openglCallbackFunction, nullptr );
        GLuint unusedIds = 0;
        glDebugMessageControl( GL_DONT_CARE,
            GL_DONT_CARE,
            GL_DONT_CARE,
            0,
            &unusedIds,
            true );
    }
#endif

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    if( glGetError() != GL_NO_ERROR )
        LOG_ERROR_MESSAGE("Failed to enable seamless cubemap filtering");

    glEnable(GL_FRAMEBUFFER_SRGB);
    if( glGetError() != GL_NO_ERROR )
        LOG_ERROR_MESSAGE("Failed to enable SRGB framebuffers");
}

void UnityGraphicsGLCore_Impl::ResizeSwapchain(int NewWidth, int NewHeight)
{
    m_BackBufferWidth = NewWidth;
    m_BackBufferHeight = NewHeight;
    // Nothing more needs to be done in GL
}

void UnityGraphicsGLCore_Impl::SwapBuffers()
{
#if PLATFORM_WIN32
    ::SwapBuffers( m_WindowHandleToDeviceContext );
#elif PLATFORM_LINUX
    glXSwapBuffers(m_Display, m_LinuxWindow);
#elif PLATFORM_MACOS
    UNEXPECTED("On MacOS, swap buffers operation is expected to be performed by the app");
#else
#   error Unsupported platform
#endif
}

#endif // GL_SUPPORTED
