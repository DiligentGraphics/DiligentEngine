
#import "AppViewBase.h"

#include "NativeAppBase.hpp"
#include <memory>
#include <string>

@interface AppViewBase ()
{
    std::unique_ptr<Diligent::NativeAppBase> _theApp;
    NSInteger _preferredFramesPerSecond;
    CADisplayLink* _displayLink;
    std::string _error;
}
@end

@implementation AppViewBase

- (void) initApp:(int)deviceType;
{
    try
    {
        _theApp.reset(Diligent::CreateApplication());
        // Init our renderer.
        _theApp->Initialize(deviceType, (__bridge void*)self.layer);
    }
    catch(std::runtime_error &err)
    {
        _error = err.what();
        _theApp.reset();
    }

    [super stopAnimation];
    _preferredFramesPerSecond = 60;
    _displayLink = nil;
}

- (void) render
{
    if(_theApp)
    {
        _theApp->Update();
        _theApp->Render();
        _theApp->Present();
    }
}

- (void) layoutSubviews
{
    auto bounds = [self.layer bounds];
    auto scale = [self.layer contentsScale];

    if(_theApp)
    {
        _theApp->WindowResize(bounds.size.width * scale, bounds.size.height * scale);
    }
}

- (NSInteger) preferredFramesPerSecond
{
	return _preferredFramesPerSecond;
}

- (void) setPreferredFramesPerSecond:(NSInteger)preferredFPS
{
	if (preferredFPS >= 1)
	{
		_preferredFramesPerSecond = preferredFPS;

		if (self.animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void) startAnimation
{
	if (!self.animating)
	{
        // Create the display link and set the callback to our drawView method
        _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView:)];

        // Set it to our _animationFrameInterval
        [_displayLink setPreferredFramesPerSecond:_preferredFramesPerSecond];

        // Have the display link run on the default runn loop (and the main thread)
        [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];

        [super startAnimation];
	}
}

- (void)stopAnimation
{
	if (self.animating)
	{
        [_displayLink invalidate];
        _displayLink = nil;
        [super stopAnimation];
	}
}

- (void)terminate
{
    _theApp.reset();
    [super terminate];
}

- (NSString*)getError
{
    return _error.empty() ? nil : [NSString stringWithFormat:@"%s", _error.c_str()];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches
           withEvent:(UIEvent *)event;
{
    UITouch *firstTouch = touches.allObjects[0];
    CGPoint location = [firstTouch locationInView:self];
    if(_theApp)
    {
        _theApp->OnTouchBegan(location.x, location.y);
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches
           withEvent:(UIEvent *)event;
{
    UITouch *firstTouch = touches.allObjects[0];
    CGPoint location = [firstTouch locationInView:self];
    if(_theApp)
    {
        _theApp->OnTouchMoved(location.x, location.y);
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches
           withEvent:(UIEvent *)event;
{
    UITouch *firstTouch = touches.allObjects[0];
    CGPoint location = [firstTouch locationInView:self];
    if(_theApp)
    {
        _theApp->OnTouchEnded(location.x, location.y);
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches
               withEvent:(UIEvent *)event;
{
    UITouch *firstTouch = touches.allObjects[0];
    CGPoint location = [firstTouch locationInView:self];
    if(_theApp)
    {
        _theApp->OnTouchEnded(location.x, location.y);
    }
}

@end
