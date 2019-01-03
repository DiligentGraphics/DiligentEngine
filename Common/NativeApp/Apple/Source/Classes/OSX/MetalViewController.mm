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

#include <memory>
#include <string>

#import "MetalView.h"
#import "MetalViewController.h"
#import <QuartzCore/CAMetalLayer.h>

#include "NativeAppBase.h"

@implementation MetalViewController
{
    MetalView *_view;
    CVDisplayLinkRef    _displayLink;
    std::unique_ptr<NativeAppBase> _theApp2;
    std::string _error;
    NSRecursiveLock* appLock;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _view = (MetalView *)self.view;

    // Back the view with a layer created by the makeBackingLayer method.
    _view.wantsLayer = YES;

    try
    {
        _theApp2.reset(CreateApplication());
        _theApp2->Initialize(_view);
    }
    catch(std::runtime_error &err)
    {
        _error = err.what();
        _theApp2.reset();
    }
    _theApp = _theApp2.get();

    appLock = [[NSRecursiveLock alloc] init];

    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, (__bridge void*)self);
    CVDisplayLinkStart(_displayLink);

    [_view setPostsBoundsChangedNotifications:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(boundsDidChange:) name:NSViewBoundsDidChangeNotification object:_view];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(boundsDidChange:) name:NSViewFrameDidChangeNotification object:_view];
}

-(void)render
{
    [appLock lock];
    if (_theApp)
    {
        _theApp->Update();
        _theApp->Render();
        _theApp->Present();
    }
    [appLock unlock];
}

-(void)boundsDidChange:(NSNotification *)notification
{
    [appLock lock];
    if (_theApp)
    {
        //CVDisplayLinkStop(_displayLink);
        NSRect viewRectPoints = [_view bounds];
        NSRect viewRectPixels = [_view convertRectToBacking:viewRectPoints];
        _theApp->WindowResize(viewRectPixels.size.width, viewRectPixels.size.height);
        //CVDisplayLinkStart(_displayLink);
    }
    [appLock unlock];
}


// Rendering loop callback function for use with a CVDisplayLink.
static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* target)
{
    MetalViewController* viewController = (MetalViewController*)target;
    [viewController render];
    return kCVReturnSuccess;
}

-(void) dealloc
{
    // Stop the display link BEFORE releasing anything in the view
    // otherwise the display link thread may call into the view and crash
    // when it encounters something that has been released
    CVDisplayLinkStop(_displayLink);

    CVDisplayLinkRelease(_displayLink);

    _theApp2.reset();

    [appLock release];

    [super dealloc];
}

-(NSString*)getAppName
{
    return [NSString stringWithFormat:@"%s (Vulkan)", _theApp ? _theApp->GetAppTitle() : ""];
}

-(NSString*)getError
{
    return _error.empty() ? nil : [NSString stringWithFormat:@"%s", _error.c_str()];
}

@end
