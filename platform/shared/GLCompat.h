//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — GL Compatibility Header
//	Provides OpenGL type definitions for both desktop and iOS builds.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

// Pulled in so PLATFORM_IOS / PLATFORM_MACOS are always defined here, even for
// translation units the build system does not pass -DPLATFORM_* to.
#include "PlatformConfig.h"

#ifdef PLATFORM_IOS
	#include <OpenGLES/ES3/gl.h>
	#include <OpenGLES/ES3/glext.h>
	// GL_BGRA is available on iOS via GL_APPLE_texture_format_BGRA8888
	#ifndef GL_BGRA
		#define GL_BGRA GL_BGRA_EXT
	#endif
#elif defined(PLATFORM_MACOS)
	// GLEW is the loader here too, but Homebrew ships it as a shared library
	// (libGLEW.dylib), so GLEW_STATIC — which the Windows branch below needs
	// because it links the static glew32s.lib — is deliberately not defined.
	// On Darwin the macro only chooses between `extern` and `extern
	// __attribute__((visibility("default")))`, so either spelling links fine
	// against the dylib; it is on Windows that getting it wrong matters, where
	// its absence means __declspec(dllimport).
	//
	// Nothing in the client uses GLU, and Apple's <OpenGL/glu.h> is deprecated
	// (it warns in every TU that includes glew.h), so keep it out.
	#ifndef GLEW_NO_GLU
		#define GLEW_NO_GLU
	#endif
	// Homebrew installs the header as <prefix>/include/GL/glew.h. Accept either
	// spelling so it does not matter whether the build adds .../include or
	// .../include/GL to the header search path.
	#if defined(__has_include)
		#if __has_include(<GL/glew.h>)
			#include <GL/glew.h>
		#else
			#include <glew.h>
		#endif
	#else
		#include <GL/glew.h>
	#endif
#else
	#define GLEW_STATIC
	#include <glew.h>
#endif
