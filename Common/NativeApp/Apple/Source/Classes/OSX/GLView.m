/*
 Copyright (C) 2015 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 OpenGL view subclass.
 */

#include <memory>

#import "GLView.h"

@interface GLView ()
{
    NSRect _viewRectPixels;
}
@end

@implementation GLView


- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime
{
	// There is no autorelease pool when this method is called
	// because it will be called from a background thread.
    // It's important to create one or app can leak objects.
    @autoreleasepool {
        [self drawView];
    }
	return kCVReturnSuccess;
}

// This is the renderer output callback function
static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink,
									  const CVTimeStamp* now,
									  const CVTimeStamp* outputTime,
									  CVOptionFlags flagsIn,
									  CVOptionFlags* flagsOut, 
									  void* displayLinkContext)
{
    CVReturn result = [(__bridge GLView*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

// Prepares the receiver for service after it has been loaded
// from an Interface Builder archive, or nib file.
- (void) awakeFromNib
{
    [super awakeFromNib];

    NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion4_1Core,
		0
	};

	NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

	if (!pf)
	{
		NSLog(@"No OpenGL pixel format");
	}

    NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];

#if defined(DEBUG)
	// When we're using a CoreProfile context, crash if we call a legacy OpenGL function
	// This will make it much more obvious where and when such a function call is made so
	// that we can remove such calls.
	// Without this we'd simply get GL_INVALID_OPERATION error for calling legacy functions
	// but it would be more difficult to see where that function was called.
	CGLEnable([context CGLContextObj], kCGLCECrashOnRemovedFunctions);
#endif

    [self setPixelFormat:pf];

    [self setOpenGLContext:context];

    // Opt-In to Retina resolution
    [self setWantsBestResolutionOpenGLSurface:YES];
}

- (void) prepareOpenGL
{
	[super prepareOpenGL];

	// Application must be initialized befor display link is started
	[self initGL];

    CVDisplayLinkRef displayLink;
	// Create a display link capable of being used with all active displays
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    [self setDisplayLink:displayLink];

	// Set the renderer output callback function
	CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, (__bridge void*)self);

	// Set the display link for the current renderer
	CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
	CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

	// Activate the display link
	CVDisplayLinkStart(displayLink);
}

- (void) initGL
{
	// The reshape function may have changed the thread to which our OpenGL
	// context is attached before prepareOpenGL and initGL are called.  So call
	// makeCurrentContext to ensure that our OpenGL context current to this 
	// thread (i.e. makeCurrentContext directs all OpenGL calls on this thread
	// to [self openGLContext])
	[[self openGLContext] makeCurrentContext];

	// Synchronize buffer swaps with vertical refresh rate
	GLint swapInt = 1;
	[[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

	// Init the application.
    [self initApp:nil];
}

- (void)reshape
{
	[super reshape];

	// We draw on a secondary thread through the display link. However, when
	// resizing the view, -drawRect is called on the main thread.
	// Add a mutex around to avoid the threads accessing the context
	// simultaneously when resizing.
	CGLLockContext([[self openGLContext] CGLContextObj]);

	// Get the view size in Points
	NSRect viewRectPoints = [self bounds];

    // Rendering at retina resolutions will reduce aliasing, but at the potential
    // cost of framerate and battery life due to the GPU needing to render more
    // pixels.

    // Any calculations the renderer does which use pixel dimentions, must be
    // in "retina" space.  [NSView convertRectToBacking] converts point sizes
    // to pixel sizes.  Thus the renderer gets the size in pixels, not points,
    // so that it can set it's viewport and perform and other pixel based
    // calculations appropriately.
    // viewRectPixels will be larger than viewRectPoints for retina displays.
    // viewRectPixels will be the same as viewRectPoints for non-retina displays
    _viewRectPixels = [self convertRectToBacking:viewRectPoints];

	// Set the new dimensions in our renderer
    auto* theApp = [self lockApp];
    if(theApp)
    {
        theApp->WindowResize(_viewRectPixels.size.width, _viewRectPixels.size.height);
    }
    [self unlockApp];

	CGLUnlockContext([[self openGLContext] CGLContextObj]);
}


- (void)renewGState
{
	// Called whenever graphics state updated (such as window resize)

	// OpenGL rendering is not synchronous with other rendering on the OSX.
	// Therefore, call disableScreenUpdatesUntilFlush so the window server
	// doesn't render non-OpenGL content in the window asynchronously from
	// OpenGL content, which could cause flickering.  (non-OpenGL content
	// includes the title bar and drawing done by the app with other APIs)
	[[self window] disableScreenUpdatesUntilFlush];

	[super renewGState];
}

- (void) drawRect: (NSRect) theRect
{
	// Called during resize operations

	// Avoid flickering during resize by drawing	
	[self drawView];
}

- (void) drawView
{
    auto* glContext = [self openGLContext];

    [glContext makeCurrentContext];

    // We draw on a secondary thread through the display link
    // When resizing the view, -reshape is called automatically on the main
    // thread. Add a mutex around to avoid the threads accessing the context
    // simultaneously when resizing
    CGLLockContext([glContext CGLContextObj]);

    auto* theApp = [self lockApp];
    if(theApp)
    {
        theApp->Update();
        theApp->Render();
    }
    [self unlockApp];

    CGLFlushDrawable([glContext CGLContextObj]);
    CGLUnlockContext([glContext CGLContextObj]);
}

-(NSString*)getAppName
{
    auto* theApp = [self lockApp];
    auto Title = [NSString stringWithFormat:@"%s (OpenGL)", theApp ? theApp->GetAppTitle() : ""];
    [self unlockApp];
    return Title;
}

@end
