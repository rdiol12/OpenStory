//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Hardware Info (iOS Implementation)
//	Replaces Windows API calls with iOS device identifiers.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Configuration.h"

#import <UIKit/UIKit.h>

namespace ms
{
	class HardwareInfo
	{
	public:
		HardwareInfo()
		{
			// Use identifierForVendor as a device-unique ID
			NSString* vendorID = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
			const char* uuid = [vendorID UTF8String];

			// Extract first 12 chars as HWID (MAC-like format)
			char hwid[18] = {0};
			char macs[18] = {0};
			char vsn[18] = {0};

			if (uuid && strlen(uuid) >= 12)
			{
				snprintf(hwid, sizeof(hwid), "%.12s", uuid);
				snprintf(vsn, sizeof(vsn), "%.8s", uuid + 12);
				snprintf(macs, sizeof(macs), "00-00-00-00-00-00");
			}

			Configuration::get().set_hwid(hwid, vsn);
			Configuration::get().set_macs(macs);
		}
	};
}
