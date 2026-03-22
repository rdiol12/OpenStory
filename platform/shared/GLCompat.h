//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — GL Compatibility Header
//	Provides OpenGL type definitions for both desktop and iOS builds.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef PLATFORM_IOS
	#include <OpenGLES/ES3/gl.h>
	#include <OpenGLES/ES3/glext.h>
	// GL_BGRA is available on iOS via GL_APPLE_texture_format_BGRA8888
	#ifndef GL_BGRA
		#define GL_BGRA GL_BGRA_EXT
	#endif
#else
	#define GLEW_STATIC
	#include <glew.h>
#endif
