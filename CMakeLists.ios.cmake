cmake_minimum_required(VERSION 3.14)
project(OpenStoryIOS LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)

# Source is relative to this repo
set(OPENSTORY_SRC "${CMAKE_SOURCE_DIR}/src")

# Platform defines
add_definitions(-DPLATFORM_IOS)
add_definitions(-DUSE_NX)
add_definitions(-DUSE_ASIO)
add_definitions(-DUSE_CRYPTO)

###############################################################################
# Shared source files
###############################################################################

file(GLOB_RECURSE SRC_CHARACTER "${OPENSTORY_SRC}/Character/*.cpp" "${OPENSTORY_SRC}/Character/*.h")
file(GLOB_RECURSE SRC_GAMEPLAY "${OPENSTORY_SRC}/Gameplay/*.cpp" "${OPENSTORY_SRC}/Gameplay/*.h")
file(GLOB_RECURSE SRC_DATA "${OPENSTORY_SRC}/Data/*.cpp" "${OPENSTORY_SRC}/Data/*.h")
file(GLOB_RECURSE SRC_NET "${OPENSTORY_SRC}/Net/*.cpp" "${OPENSTORY_SRC}/Net/*.h")
file(GLOB SRC_TEMPLATE "${OPENSTORY_SRC}/Template/*.h")

file(GLOB SRC_IO_CORE
	"${OPENSTORY_SRC}/IO/UI.cpp"
	"${OPENSTORY_SRC}/IO/UI.h"
	"${OPENSTORY_SRC}/IO/UIElement.cpp"
	"${OPENSTORY_SRC}/IO/UIElement.h"
	"${OPENSTORY_SRC}/IO/UIScale.h"
	"${OPENSTORY_SRC}/IO/UIState*.cpp"
	"${OPENSTORY_SRC}/IO/UIState*.h"
	"${OPENSTORY_SRC}/IO/Cursor.cpp"
	"${OPENSTORY_SRC}/IO/Cursor.h"
	"${OPENSTORY_SRC}/IO/Keyboard.cpp"
	"${OPENSTORY_SRC}/IO/Keyboard.h"
	"${OPENSTORY_SRC}/IO/KeyAction.h"
	"${OPENSTORY_SRC}/IO/KeyType.h"
	"${OPENSTORY_SRC}/IO/Messages.cpp"
	"${OPENSTORY_SRC}/IO/Messages.h"
)
file(GLOB_RECURSE SRC_IO_COMPONENTS "${OPENSTORY_SRC}/IO/Components/*.cpp" "${OPENSTORY_SRC}/IO/Components/*.h")
file(GLOB_RECURSE SRC_IO_UITYPES "${OPENSTORY_SRC}/IO/UITypes/*.cpp" "${OPENSTORY_SRC}/IO/UITypes/*.h")

file(GLOB SRC_GRAPHICS
	"${OPENSTORY_SRC}/Graphics/GraphicsGL.h"
	"${OPENSTORY_SRC}/Graphics/GraphicsGL.cpp"
	"${OPENSTORY_SRC}/Graphics/Animation.cpp"
	"${OPENSTORY_SRC}/Graphics/Animation.h"
	"${OPENSTORY_SRC}/Graphics/Color.cpp"
	"${OPENSTORY_SRC}/Graphics/Color.h"
	"${OPENSTORY_SRC}/Graphics/DamageNumber.cpp"
	"${OPENSTORY_SRC}/Graphics/DamageNumber.h"
	"${OPENSTORY_SRC}/Graphics/DrawArgument.h"
	"${OPENSTORY_SRC}/Graphics/EffectLayer.cpp"
	"${OPENSTORY_SRC}/Graphics/EffectLayer.h"
	"${OPENSTORY_SRC}/Graphics/Geometry.cpp"
	"${OPENSTORY_SRC}/Graphics/Geometry.h"
	"${OPENSTORY_SRC}/Graphics/RecurringEffect.h"
	"${OPENSTORY_SRC}/Graphics/Sprite.cpp"
	"${OPENSTORY_SRC}/Graphics/Sprite.h"
	"${OPENSTORY_SRC}/Graphics/Text.cpp"
	"${OPENSTORY_SRC}/Graphics/Text.h"
	"${OPENSTORY_SRC}/Graphics/Texture.cpp"
	"${OPENSTORY_SRC}/Graphics/Texture.h"
	"${OPENSTORY_SRC}/Graphics/ColorBox.cpp"
	"${OPENSTORY_SRC}/Graphics/ColorBox.h"
)

file(GLOB SRC_UTIL
	"${OPENSTORY_SRC}/Util/Misc.cpp"
	"${OPENSTORY_SRC}/Util/Misc.h"
	"${OPENSTORY_SRC}/Util/NxFiles.cpp"
	"${OPENSTORY_SRC}/Util/NxFiles.h"
	"${OPENSTORY_SRC}/Util/QuadTree.h"
	"${OPENSTORY_SRC}/Util/Randomizer.h"
	"${OPENSTORY_SRC}/Util/Lerp.h"
	"${OPENSTORY_SRC}/Util/TimedBool.h"
)

set(SRC_ROOT
	"${OPENSTORY_SRC}/Configuration.cpp"
	"${OPENSTORY_SRC}/Configuration.h"
	"${OPENSTORY_SRC}/Constants.h"
	"${OPENSTORY_SRC}/Timer.h"
	"${OPENSTORY_SRC}/Error.h"
	"${OPENSTORY_SRC}/MapleStory.h"
)

set(SHARED_SOURCES
	${SRC_CHARACTER}
	${SRC_GAMEPLAY}
	${SRC_DATA}
	${SRC_NET}
	${SRC_TEMPLATE}
	${SRC_IO_CORE}
	${SRC_IO_COMPONENTS}
	${SRC_IO_UITYPES}
	${SRC_GRAPHICS}
	${SRC_UTIL}
	${SRC_ROOT}
)

# Exclude desktop-only implementations
list(FILTER SHARED_SOURCES EXCLUDE REGEX "Window\\.cpp$")
list(FILTER SHARED_SOURCES EXCLUDE REGEX "Window\\.h$")
list(FILTER SHARED_SOURCES EXCLUDE REGEX "Audio\\.cpp$")
# GraphicsGL.cpp now has #ifdef PLATFORM_IOS guards — keep it in the build
list(FILTER SHARED_SOURCES EXCLUDE REGEX "SocketWinsock\\.(cpp|h)$")
list(FILTER SHARED_SOURCES EXCLUDE REGEX "MapleStory\\.cpp$")

###############################################################################
# iOS platform sources
###############################################################################

set(IOS_SOURCES
	${CMAKE_SOURCE_DIR}/platform/ios/EntryPoint.mm
	${CMAKE_SOURCE_DIR}/platform/ios/AppDelegate.h
	${CMAKE_SOURCE_DIR}/platform/ios/AppDelegate.mm
	${CMAKE_SOURCE_DIR}/platform/ios/GameViewController.h
	${CMAKE_SOURCE_DIR}/platform/ios/GameViewController.mm
	${CMAKE_SOURCE_DIR}/platform/ios/TouchInput.h
	${CMAKE_SOURCE_DIR}/platform/ios/TouchInput.mm
	${CMAKE_SOURCE_DIR}/platform/ios/WindowIOS.mm
	${CMAKE_SOURCE_DIR}/platform/ios/FontPathIOS.mm
	${CMAKE_SOURCE_DIR}/platform/ios/AudioIOS.mm
	${CMAKE_SOURCE_DIR}/platform/ios/VirtualControls.h
	${CMAKE_SOURCE_DIR}/platform/ios/VirtualControls.mm
	${CMAKE_SOURCE_DIR}/platform/ios/HardwareInfoIOS.h
	${CMAKE_SOURCE_DIR}/platform/ios/ScreenResolutionIOS.h
	${CMAKE_SOURCE_DIR}/platform/shared/GLCompat.h
	${CMAKE_SOURCE_DIR}/platform/shared/PlatformConfig.h
	${CMAKE_SOURCE_DIR}/platform/shared/KeyCodes.h
)

set(IOS_RESOURCES
	${CMAKE_SOURCE_DIR}/platform/ios/LaunchScreen.storyboard
	${CMAKE_SOURCE_DIR}/resources/fonts/Roboto-Regular.ttf
	${CMAKE_SOURCE_DIR}/resources/fonts/Roboto-Bold.ttf
)

# Optionally bundle NX data files if present in resources/nx/
file(GLOB NX_DATA_FILES "${CMAKE_SOURCE_DIR}/resources/nx/*.nx")
if(NX_DATA_FILES)
	list(APPEND IOS_RESOURCES ${NX_DATA_FILES})
	message(STATUS "Bundling NX files: ${NX_DATA_FILES}")
else()
	message(STATUS "No NX files found in resources/nx/ — app will look in Documents folder at runtime")
endif()

###############################################################################
# App target
###############################################################################

add_executable(OpenStoryIOS MACOSX_BUNDLE
	${SHARED_SOURCES}
	${IOS_SOURCES}
	${IOS_RESOURCES}
)

set_target_properties(OpenStoryIOS PROPERTIES
	MACOSX_BUNDLE TRUE
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platform/ios/Info.plist
	XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
	XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
	XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.openstory.ios"
	XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
	RESOURCE "${IOS_RESOURCES}"
)

###############################################################################
# Include directories
###############################################################################

target_include_directories(OpenStoryIOS PUBLIC
	${OPENSTORY_SRC}
	${CMAKE_SOURCE_DIR}/platform/shared
	${CMAKE_SOURCE_DIR}/platform/ios
)

# Third-party — built by CI workflow
target_include_directories(OpenStoryIOS PUBLIC
	${CMAKE_SOURCE_DIR}/libs/ios/include
	${CMAKE_SOURCE_DIR}/libs/ios/include/nlnx
	${CMAKE_SOURCE_DIR}/libs/ios/include/freetype2
)

###############################################################################
# Link frameworks and static libraries
###############################################################################

target_link_libraries(OpenStoryIOS
	"-framework UIKit"
	"-framework GLKit"
	"-framework OpenGLES"
	"-framework OpenAL"
	"-framework AVFoundation"
	"-framework AudioToolbox"
	"-framework Foundation"
)

# Static libraries (built for iOS arm64 by CI)
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/ios/lib/libNoLifeNx.a")
	target_link_libraries(OpenStoryIOS ${CMAKE_SOURCE_DIR}/libs/ios/lib/libNoLifeNx.a)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/ios/lib/libfreetype.a")
	target_link_libraries(OpenStoryIOS ${CMAKE_SOURCE_DIR}/libs/ios/lib/libfreetype.a)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/ios/lib/liblz4.a")
	target_link_libraries(OpenStoryIOS ${CMAKE_SOURCE_DIR}/libs/ios/lib/liblz4.a)
endif()

###############################################################################
# Compile options
###############################################################################

target_compile_options(OpenStoryIOS PRIVATE
	-Wno-deprecated-declarations
	-Wno-shorten-64-to-32
)

target_compile_definitions(OpenStoryIOS PRIVATE
	BOOST_DATE_TIME_NO_LIB
	BOOST_REGEX_NO_LIB
	ASIO_STANDALONE
)
