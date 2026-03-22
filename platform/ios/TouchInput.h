//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Touch Input Handler
//	Translates iOS touch events into the game's cursor/key input system.
//	Integrates virtual controls for D-pad and action buttons.
//////////////////////////////////////////////////////////////////////////////////
#import <UIKit/UIKit.h>

#include "VirtualControls.h"

@interface TouchInput : NSObject

+ (void)handleTouchesBegan:(NSSet<UITouch*>*)touches inView:(UIView*)view;
+ (void)handleTouchesMoved:(NSSet<UITouch*>*)touches inView:(UIView*)view;
+ (void)handleTouchesEnded:(NSSet<UITouch*>*)touches inView:(UIView*)view;

+ (ms::VirtualControls&)controls;
+ (void)initControls:(int16_t)width height:(int16_t)height;

@end
