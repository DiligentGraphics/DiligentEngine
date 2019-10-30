/*
 Copyright (C) 2015 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information
 
 Abstract:
 Window controller subclass.
 */

#import "WindowController.h"
#import "FullscreenWindow.h"
#import "ViewBase.h"

@interface WindowController ()

// Fullscreen window
@property(strong) FullscreenWindow *fullscreenWindow;

// Non-Fullscreen window (also the initial window)
@property(strong) NSWindow* standardWindow;

@end

@implementation WindowController
{
    bool CommandKeyPressed;
}

- (instancetype)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];

	if (self)
	{
		// Initialize to nil since it indicates app is not fullscreen
        _fullscreenWindow = nil;
    }

    CommandKeyPressed = false;

	return self;
}

- (void) goFullscreen
{
	// If app is already fullscreen...
    if([self fullscreenWindow])
	{
		//...don't do anything
		return;
	}

    ViewBase* view = (ViewBase*)self.window.contentView;

    // We must stop the display link while
    // switching the windows to make sure
    // that render commands are not issued
    // from another thread
    [view stopDisplayLink];

	// Allocate a new fullscreen window
    [self setFullscreenWindow: [[FullscreenWindow alloc] init]];

    [[self fullscreenWindow] setAcceptsMouseMovedEvents:YES];

	// Resize the view to screensize
	NSRect viewRect = [[self fullscreenWindow] frame];

	// Set the view to the size of the fullscreen window
	[self.window.contentView setFrameSize: viewRect.size];

	// Set the view in the fullscreen window
	[[self fullscreenWindow] setContentView:self.window.contentView];

    [self setStandardWindow:[self window]];

	// Hide non-fullscreen window so it doesn't show up when switching out
	// of this app (i.e. with CMD-TAB)
	[[self standardWindow] orderOut:self];

	// Set controller to the fullscreen window so that all input will go to
	// this controller (self)
	[self setWindow:[self fullscreenWindow]];

	// Show the window and make it the key window for input
	[[self fullscreenWindow] makeKeyAndOrderFront:self];

    // Restore display link
    [view startDisplayLink];
}

- (void) goWindow
{
	// If controller doesn't have a full screen window...
	if([self fullscreenWindow] == nil)
	{
		//...app is already windowed so don't do anything
		return;
	}

    ViewBase* view = (ViewBase*)self.window.contentView;

    // We must stop the display link while
    // switching the windows to make sure
    // that render commands are not issued
    // from another thread
    [view stopDisplayLink];

	// Get the rectangle of the original window
	NSRect viewRect = [[self standardWindow] frame];
	
	// Set the view rect to the new size
	[self.window.contentView setFrame:viewRect];

    // Hide fullscreen window
    [[self fullscreenWindow] orderOut:self];

	// Set controller to the standard window so that all input will go to
	// this controller (self)
	[self setWindow:[self standardWindow]];

	// Set the content of the orginal window to the view
	[[self window] setContentView: [self fullscreenWindow].contentView];

	// Show the window and make it the key window for input
	[[self window] makeKeyAndOrderFront:self];

	// Ensure we set fullscreen Window to nil so our checks for 
	// windowed vs. fullscreen mode elsewhere are correct
    [self setFullscreenWindow: nil];

    // Restore display link
    [view startDisplayLink];
}


- (void) keyDown:(NSEvent *)event
{
	unichar c = [[event charactersIgnoringModifiers] characterAtIndex:0];

	switch (c)
	{
		// Handle [ESC] key
		case 27:
            if([self fullscreenWindow] != nil)
			{
				[self goWindow];
			}
			return;

		// Have Command+f or Command+Enter toggle fullscreen
        case 13:
		case 'f':
            if (CommandKeyPressed)
            {
                if([self fullscreenWindow] == nil)
                {
                    [self goFullscreen];
                }
                else
                {
                    [self goWindow];
                }
            }
			return;
	}

	// Allow other character to be handled (or not and beep)
	//[super keyDown:event];
}

// Informs the receiver that the user has pressed or released a
// modifier key (Shift, Control, and so on)
- (void)flagsChanged:(NSEvent *)event
{
    auto modifierFlags = [event modifierFlags];
    CommandKeyPressed = modifierFlags & NSEventModifierFlagCommand;

    [super flagsChanged:event];
}

@end
