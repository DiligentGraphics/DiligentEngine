/*
 Copyright (C) 2015 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 The EAGLView class is a UIView subclass that renders OpenGL scene.
*/

#import "EAGLView.h"

#include "DeviceCaps.h"

@interface EAGLView ()
{
    EAGLContext* _context;
}
@end

@implementation EAGLView

// Must return the CAEAGLLayer class so that CA allocates an EAGLLayer backing for this view
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

// The GL view is stored in the storyboard file. When it's unarchived it's sent -initWithCoder:
- (instancetype) initWithCoder:(NSCoder*)coder
{
    if ((self = [super initWithCoder:coder]))
	{
        // Get the layer
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;

        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

		_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

        if (!_context || ![EAGLContext setCurrentContext:_context])
		{
            return nil;
		}

        [self initApp:(int)Diligent::DeviceType::OpenGLES];
    }

    return self;
}

- (void) drawView:(id)sender
{
    [EAGLContext setCurrentContext:_context];
 
    // There is no autorelease pool when this method is called
    // because it will be called from a background thread.
    // It's important to create one or app can leak objects.
    @autoreleasepool
    {
        [self render];
    }
}

- (void) dealloc
{
    [self terminate];

	// tear down context
    if ([EAGLContext currentContext] == _context)
        [EAGLContext setCurrentContext:nil];
}

@end
