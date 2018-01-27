/*
 Copyright (C) 2015 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 Fullscreen window class.
  All logic here could have been done in the window controller except that, by default, borderless windows cannot be made key and input cannot go to them.
  Therefore, this class exists to override canBecomeKeyWindow allowing this borderless window to accept inputs.
  This class is not part of the NIB and entirely managed in code by the window controller.
 */

#import "FullscreenWindow.h"

@implementation FullscreenWindow

-(instancetype)init
{
	// Create a screen-sized window on the display you want to take over
	NSRect screenRect = [[NSScreen mainScreen] frame];

	// Initialize the window making it size of the screen and borderless
	self = [super initWithContentRect:screenRect
							styleMask:NSWindowStyleMaskBorderless
							  backing:NSBackingStoreBuffered
								defer:YES];

	// Set the window level to be above the menu bar to cover everything else
	[self setLevel:NSMainMenuWindowLevel+1];

	// Set opaque
	[self setOpaque:YES];

	// Hide this when user switches to another window (or app)
	[self setHidesOnDeactivate:YES];

	return self;
}

-(BOOL)canBecomeKeyWindow
{
	// Return yes so that this borderless window can receive input
	return YES;
}

- (void)keyDown:(NSEvent *)event
{
	// Implement keyDown since controller will not get [ESC] key event which
	// the controller uses to kill fullscreen
	[[self windowController] keyDown:event];
}

@end
