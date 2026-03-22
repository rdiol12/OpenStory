//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Screen Resolution (iOS Implementation)
//	Uses UIScreen to get device screen dimensions.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Configuration.h"

#import <UIKit/UIKit.h>

namespace ms
{
	class ScreenResolution
	{
	public:
		ScreenResolution()
		{
			CGRect bounds = [UIScreen mainScreen].nativeBounds;

			Configuration::get().set_max_width(static_cast<int16_t>(bounds.size.width));
			Configuration::get().set_max_height(static_cast<int16_t>(bounds.size.height));
		}
	};
}
