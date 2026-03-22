//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — GL Compatibility Header
//	Provides OpenGL type definitions for both desktop and iOS builds.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef PLATFORM_IOS
	#include <OpenGLES/ES3/gl.h>
	#include <OpenGLES/ES3/glext.h>
#else
	#define GLEW_STATIC
	#include <glew.h>
#endif
