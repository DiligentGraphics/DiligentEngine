/*
 Copyright (C) 2015 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 OpenGL view subclass.
 */

#include <memory>
#include <string>

#import "GLView.h"
#include "NativeAppBase.h"

#define SUPPORT_RETINA_RESOLUTION 1

@interface GLView ()
{
    std::unique_ptr<NativeAppBase> _theApp;
    NSRect _viewRectPixels;
    std::string _error;
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
    
#if SUPPORT_RETINA_RESOLUTION
    // Opt-In to Retina resolution
    [self setWantsBestResolutionOpenGLSurface:YES];
#endif // SUPPORT_RETINA_RESOLUTION
    
    _theApp.reset(CreateApplication());
}

- (void) prepareOpenGL
{
	[super prepareOpenGL];
	
	// Make all the OpenGL calls to setup rendering  
	//  and build the necessary rendering objects
	[self initGL];
	
	// Create a display link capable of being used with all active displays
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
	
	// Set the renderer output callback function
	CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, (__bridge void*)self);
	
	// Set the display link for the current renderer
	CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
	CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
	
	// Activate the display link
	CVDisplayLinkStart(displayLink);
	
	// Register to be notified when the window closes so we can stop the displaylink
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowWillClose:)
												 name:NSWindowWillCloseNotification
											   object:[self window]];
}

- (void) windowWillClose:(NSNotification*)notification
{
	// Stop the display link when the window is closing because default
	// OpenGL render buffers will be destroyed.  If display link continues to
	// fire without renderbuffers, OpenGL draw calls will set errors.

	CVDisplayLinkStop(displayLink);
    // Stop the display link BEFORE releasing anything in the view
    // otherwise the display link thread may call into the view and crash
    // when it encounters something that has been released

    _theApp.reset();
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
    try
    {
        _theApp->OnGLContextCreated();
    }
    catch(std::runtime_error &err)
    {
        _error = err.what();
        _theApp.reset();
    }
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
    
#if SUPPORT_RETINA_RESOLUTION

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
    
#else //if !SUPPORT_RETINA_RESOLUTION
    
    // App will typically render faster and use less power rendering at
    // non-retina resolutions since the GPU needs to render less pixels.
    // There is the cost of more aliasing, but it will be no-worse than
    // on a Mac without a retina display.
    
    // Points:Pixels is always 1:1 when not supporting retina resolutions
    _viewRectPixels = viewRectPoints;
    
#endif // !SUPPORT_RETINA_RESOLUTION
    
	// Set the new dimensions in our renderer
    if(_theApp)
        _theApp->WindowResize(_viewRectPixels.size.width, _viewRectPixels.size.height);

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
	
	// Avoid flickering during resize by drawiing	
	[self drawView];
}

- (void) drawView
{	 
	[[self openGLContext] makeCurrentContext];

	// We draw on a secondary thread through the display link
	// When resizing the view, -reshape is called automatically on the main
	// thread. Add a mutex around to avoid the threads accessing the context
	// simultaneously when resizing
	CGLLockContext([[self openGLContext] CGLContextObj]);

    if(_theApp)
    {
        _theApp->Update();
        _theApp->Render();
    }

	CGLFlushDrawable([[self openGLContext] CGLContextObj]);
	CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void) dealloc
{
	// Stop the display link BEFORE releasing anything in the view
    // otherwise the display link thread may call into the view and crash
    // when it encounters something that has been released
	CVDisplayLinkStop(displayLink);

	CVDisplayLinkRelease(displayLink);

	_theApp.reset();
	
    [super dealloc];
}

- (void)mouseMove:(NSEvent *)theEvent {
    NSPoint curPoint = [self convertPoint:[theEvent locationInWindow] fromView:nil];
#if SUPPORT_RETINA_RESOLUTION
    curPoint = [self convertPointToBacking:curPoint];
#endif
    if(_theApp)
        _theApp->OnMouseMove(curPoint.x, _viewRectPixels.size.height-1 - curPoint.y);
}

- (void)mouseDown:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    if(_theApp)
        _theApp->OnMouseDown(1);
}

- (void)mouseUp:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    if(_theApp)
        _theApp->OnMouseUp(1);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    if(_theApp)
        _theApp->OnMouseDown(3);
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    if(_theApp)
        _theApp->OnMouseUp(3);
}

- (void)mouseMoved:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
}

- (void)keyDown:(NSEvent *)theEvent {
    unichar c = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    int key = 0;
    switch(c){
        case NSLeftArrowFunctionKey: key = 260; break;
        case NSRightArrowFunctionKey: key = 262; break;
        case 0x7F: key = '\b'; break;
        default: key = c;
    }
    if(_theApp)
        _theApp->OnKeyPressed(key);

    [super keyDown:theEvent];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)stopDisplayLink
{
    CVDisplayLinkStop(displayLink);
}

- (void)startDisplayLink
{
    CVDisplayLinkStart(displayLink);
}

- (NSString*)getAppName
{
    return [NSString stringWithFormat:@"%s", _theApp ? _theApp->GetAppTitle() : ""];
}

- (NSString*)getError
{
    return _error.empty() ? nil : [NSString stringWithFormat:@"%s", _error.c_str()];
}

@end
