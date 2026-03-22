//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — NX file path helpers (Objective-C)
//////////////////////////////////////////////////////////////////////////////////
#import <Foundation/Foundation.h>
#include <string>

std::string ios_get_bundle_resource_path()
{
	NSString* path = [[NSBundle mainBundle] resourcePath];
	if (path)
		return [path UTF8String];
	return "";
}

std::string ios_get_documents_path()
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	if ([paths count] > 0)
		return [[paths firstObject] UTF8String];
	return "";
}
