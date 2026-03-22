#import <Foundation/Foundation.h>

static char normal_path_buf[1024] = {};
static char bold_path_buf[1024] = {};

const char* ios_font_path_normal()
{
	if (normal_path_buf[0] == 0)
	{
		NSString* path = [[NSBundle mainBundle] pathForResource:@"Roboto-Regular" ofType:@"ttf"];
		if (path)
			strncpy(normal_path_buf, [path UTF8String], sizeof(normal_path_buf) - 1);
		else
			return nullptr;
	}
	return normal_path_buf;
}

const char* ios_font_path_bold()
{
	if (bold_path_buf[0] == 0)
	{
		NSString* path = [[NSBundle mainBundle] pathForResource:@"Roboto-Bold" ofType:@"ttf"];
		if (path)
			strncpy(bold_path_buf, [path UTF8String], sizeof(bold_path_buf) - 1);
		else
			return nullptr;
	}
	return bold_path_buf;
}
