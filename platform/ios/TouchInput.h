//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Touch Input Handler
//	Translates iOS touch events into the game's cursor/key input system.
//	- Single tap = left click
//	- Touch drag = cursor movement
//	- Long press = right click
//	- Two-finger tap = pickup (Z key)
//////////////////////////////////////////////////////////////////////////////////
#import <UIKit/UIKit.h>

@interface TouchInput : NSObject

+ (void)handleTouchesBegan:(NSSet<UITouch*>*)touches inView:(UIView*)view;
+ (void)handleTouchesMoved:(NSSet<UITouch*>*)touches inView:(UIView*)view;
+ (void)handleTouchesEnded:(NSSet<UITouch*>*)touches inView:(UIView*)view;

@end
