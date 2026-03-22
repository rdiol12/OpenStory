//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — App Delegate Implementation
//////////////////////////////////////////////////////////////////////////////////
#import "AppDelegate.h"
#import "GameViewController.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
	self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];

	GameViewController* gameVC = [[GameViewController alloc] init];
	self.window.rootViewController = gameVC;
	[self.window makeKeyAndVisible];

	// Hide status bar for fullscreen game
	[UIApplication sharedApplication].statusBarHidden = YES;

	return YES;
}

- (void)applicationWillResignActive:(UIApplication*)application
{
	// Pause game loop when app goes to background
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
	// Resume game loop when app returns to foreground
}

- (void)applicationWillTerminate:(UIApplication*)application
{
	// Save configuration on exit
}

@end
