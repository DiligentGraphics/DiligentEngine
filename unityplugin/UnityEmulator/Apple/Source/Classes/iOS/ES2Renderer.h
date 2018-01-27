/*
 Copyright (C) 2015 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 The main rendering code.
 */

#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import "OpenGLRenderer.h"

@interface ES2Renderer : OpenGLRenderer
{
	
}

- (instancetype)initWithContext:(EAGLContext*)context AndDrawable:(id<EAGLDrawable>)drawable;
- (void)render;
- (BOOL)resizeFromLayer:(CAEAGLLayer*)layer;

@end

