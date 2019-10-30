/*     Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#import "ViewController.h"
#import "ViewBase.h"

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Add a tracking area in order to receive mouse events whenever the mouse is within the bounds of our view
    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                                options:NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
                                                                  owner:self
                                                               userInfo:nil];
    [self.view addTrackingArea:trackingArea];
}

- (void)handleEvent : (NSEvent *)theEvent {
    auto* view = (ViewBase*)self.view;
    auto* theApp = [view lockApp];
    if(theApp){
        theApp->HandleOSXEvent(theEvent, view);
    }
    [view unlockApp];
}


- (void)mouseDown:(NSEvent *)theEvent {
    [self handleEvent:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent {
    [self handleEvent:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self handleEvent:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    [self handleEvent:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent {
    [self handleEvent:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent {
    [self handleEvent:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    [self handleEvent:theEvent];
}

- (void)keyEvent:(NSEvent *)theEvent isKeyPressed:(bool)keyPressed
{
    [self handleEvent:theEvent];
}

- (void)keyDown:(NSEvent *)theEvent
{
    [self handleEvent:theEvent];

    [super keyDown:theEvent];
}

- (void)keyUp:(NSEvent *)theEvent
{
    [self handleEvent:theEvent];

    [super keyUp:theEvent];
}

// Informs the receiver that the user has pressed or released a
// modifier key (Shift, Control, and so on)
- (void)flagsChanged:(NSEvent *)event
{
    [self handleEvent:event];

    [super flagsChanged:event];
}

- (void)scrollWheel:(NSEvent *)event
{
    [self handleEvent:event];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

@end
