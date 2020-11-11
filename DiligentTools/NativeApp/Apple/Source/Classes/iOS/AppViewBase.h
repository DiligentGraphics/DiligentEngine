
#import <UIKit/UIKit.h>
#import "BaseView.h"

@interface AppViewBase : BaseView

@property (nonatomic) NSInteger preferredFramesPerSecond;

- (void) initApp:(int)deviceType;
- (void) startAnimation;
- (void) stopAnimation;
- (void) terminate;
- (void) render;
- (NSString*)getError;

@end
