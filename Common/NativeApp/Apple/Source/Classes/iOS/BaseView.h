
#import <UIKit/UIKit.h>

@interface BaseView : UIView

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;

- (void) startAnimation;
- (void) stopAnimation;
- (void) terminate;

@end
