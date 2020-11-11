
#import "BaseView.h"

@implementation BaseView

- (void) startAnimation
{
    _animating = TRUE;
}

- (void)stopAnimation
{
    _animating = FALSE;
}

- (void) terminate
{
    
}

@end
