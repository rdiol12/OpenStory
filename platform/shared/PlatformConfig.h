//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Platform Configuration
//	Central platform detection and feature flags.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

// Platform detection
#if defined(__APPLE__)
	#include <TargetConditionals.h>
	#if TARGET_OS_IPHONE
		#define PLATFORM_IOS 1
	#else
		#define PLATFORM_MACOS 1
	#endif
#elif defined(_WIN32)
	#define PLATFORM_WINDOWS 1
#else
	#define PLATFORM_LINUX 1
#endif

// Feature flags for iOS
#ifdef PLATFORM_IOS
	#define USE_NX 1
	#define USE_ASIO 1
	#define USE_CRYPTO 1
#endif
