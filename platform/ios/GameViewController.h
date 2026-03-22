//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Game View Controller
//	GLKViewController-based game loop with OpenGL ES 3.0 rendering.
//	Replaces GLFW window + main loop from desktop build.
//////////////////////////////////////////////////////////////////////////////////
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface GameViewController : GLKViewController

@property (nonatomic, assign) BOOL gameInitialized;
@property (nonatomic, assign) BOOL gamePaused;

@end
