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

#import "MetalView.h"
#import <QuartzCore/CAMetalLayer.h>

#include "NativeAppBase.h"

#pragma mark -
#pragma mark MetalViewController

@implementation MetalViewController
{
    std::unique_ptr<NativeAppBase> _theApp;
}

-(void) viewDidLoad {
	[super viewDidLoad];

	self.view.wantsLayer = YES;		// Back the view with a layer created by the makeBackingLayer method.

    _theApp.reset(CreateApplication());
    _theApp->Initialize(self.view);

    // Temporary
    _theApp->Update();
    _theApp->Render();
    _theApp->Present();
}

// Resize the window to fit the size of the content as set by the sample code.
/*-(void) viewWillAppear {
	[super viewWillAppear];

	CGSize vSz = self.view.bounds.size;
	NSWindow *window = self.view.window;
	NSRect wFrm = [window contentRectForFrameRect: window.frame];
	NSRect newWFrm = [window frameRectForContentRect: NSMakeRect(wFrm.origin.x, wFrm.origin.y, vSz.width, vSz.height)];
	[window setFrame: newWFrm display: YES animate: window.isVisible];
	[window center];
}*/

@end


#pragma mark -
#pragma mark MetalView

@implementation MetalView

// Indicates that the view wants to draw using the backing layer instead of using drawRect:.
-(BOOL) wantsUpdateLayer
{
    return YES;
}

// Returns a Metal-compatible layer.
+(Class) layerClass
{
    return [CAMetalLayer class];
}

// If the wantsLayer property is set to YES, this method will be invoked to return a layer instance.
-(CALayer*) makeBackingLayer
{
    return [self.class.layerClass layer];
}

@end
