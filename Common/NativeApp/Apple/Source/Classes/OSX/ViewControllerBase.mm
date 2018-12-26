/*     Copyright 2015-2018 Egor Yusov
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

#import "ViewControllerBase.h"
#import "GLView.h"
#import "MetalView.h"

#ifndef SUPPORT_RETINA_RESOLUTION
#define SUPPORT_RETINA_RESOLUTION 1
#endif

@implementation ViewControllerBase
{
    
}

- (void)mouseMove:(NSEvent *)theEvent {
    NSPoint curPoint = [self.view convertPoint:[theEvent locationInWindow] fromView:nil];
    
    NSRect viewRectPoints = [self.view bounds];
#if SUPPORT_RETINA_RESOLUTION
    NSRect viewRectPixels = [self.view convertRectToBacking:viewRectPoints];
    curPoint = [self.view convertPointToBacking:curPoint];
#else
    // Points:Pixels is always 1:1 when not supporting retina resolutions
    NSRect viewRectPixels = viewRectPoints;
#endif
    
    if(_theApp)
        _theApp->OnMouseMove(curPoint.x, viewRectPixels.size.height-1 - curPoint.y);
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
        case NSLeftArrowFunctionKey:  key = 260;  break;
        case NSRightArrowFunctionKey: key = 262;  break;
        case 0x7F:                    key = '\b'; break;
        default:                      key = c;
    }
    if(_theApp)
        _theApp->OnKeyPressed(key);
    
    [super keyDown:theEvent];
}

@end
