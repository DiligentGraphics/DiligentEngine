#import "MetalView.h"

#if VULKAN_SUPPORTED
#import <QuartzCore/CAMetalLayer.h>
#endif

#include "DeviceCaps.h"

@implementation MetalView

#if VULKAN_SUPPORTED
+ (Class) layerClass
{
    return [CAMetalLayer class];
}
#endif

- (instancetype) initWithCoder:(NSCoder*)coder
{
    if ((self = [super initWithCoder:coder]))
	{
        [self initApp:(int)Diligent::DeviceType::Vulkan];
    }

    return self;
}

- (void) drawView:(id)sender
{
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
}

@end
