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


- (void)mouseMove:(NSEvent *)theEvent {
    NSPoint curPoint = [self.view convertPoint:[theEvent locationInWindow] fromView:nil];
    
    NSRect viewRectPoints = [self.view bounds];
    NSRect viewRectPixels = [self.view convertRectToBacking:viewRectPoints];
    curPoint = [self.view convertPointToBacking:curPoint];
    
    auto* view = (ViewBase*)self.view;
    auto* theApp = [view lockApp];
    if(theApp)
        theApp->OnMouseMove(curPoint.x, viewRectPixels.size.height-1 - curPoint.y);
    [view unlockApp];
}

- (void)mouseDown:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    
    {
        auto* view = (ViewBase*)self.view;
        auto* theApp = [view lockApp];
        if(theApp)
            theApp->OnMouseDown(1);
        [view unlockApp];
    }
}

- (void)mouseUp:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    
    {
        auto* view = (ViewBase*)self.view;
        auto* theApp = [view lockApp];
        if(theApp)
            theApp->OnMouseUp(1);
        [view unlockApp];
    }
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    
    {
        auto* view = (ViewBase*)self.view;
        auto* theApp = [view lockApp];
        if(theApp)
            theApp->OnMouseDown(3);
        [view unlockApp];
    }
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
    
    {
        auto* view = (ViewBase*)self.view;
        auto* theApp = [view lockApp];
        if(theApp)
            theApp->OnMouseUp(3);
        [view unlockApp];
    }
}

- (void)mouseMoved:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent {
    [self mouseMove: theEvent];
}

- (void)keyEvent:(NSEvent *)theEvent isKeyPressed:(bool)keyPressed
{
    unichar c = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    int key = 0;
    switch(c){
        case NSLeftArrowFunctionKey:  key = 260;  break;
        case NSRightArrowFunctionKey: key = 262;  break;
        case 0x7F:                    key = '\b'; break;
        default:                      key = c;
    }

    {
        auto* view = (ViewBase*)self.view;
        auto* theApp = [view lockApp];
        if(theApp)
        {
            if (keyPressed)
                theApp->OnKeyPressed(key);
            else
                theApp->OnKeyReleased(key);
        }
        [view unlockApp];
    }
}

- (void)keyDown:(NSEvent *)theEvent
{
    [self keyEvent:theEvent isKeyPressed:true];

    [super keyDown:theEvent];
}

- (void)keyUp:(NSEvent *)theEvent
{
    [self keyEvent:theEvent isKeyPressed:false];

    [super keyUp:theEvent];
}

// Informs the receiver that the user has pressed or released a
// modifier key (Shift, Control, and so on)
- (void)flagsChanged:(NSEvent *)event
{
    auto modifierFlags = [event modifierFlags];
    {
        auto* view = (ViewBase*)self.view;
        auto* theApp = [view lockApp];
        if(theApp)
        {
            theApp->OnFlagsChanged(modifierFlags & NSEventModifierFlagShift,
                                   modifierFlags & NSEventModifierFlagControl,
                                   modifierFlags & NSEventModifierFlagOption);
        }
        [view unlockApp];
    }

    [super flagsChanged:event];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

@end
