//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — iOS Entry Point
//	Replaces the desktop main() with UIApplicationMain.
//////////////////////////////////////////////////////////////////////////////////
#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main(int argc, char* argv[])
{
	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
