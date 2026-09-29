cmake_minimum_required(VERSION 3.14)

# macOS toolchain settings have to be in place *before* project() so that the
# compiler checks (and every object file after them) are built for the right
# architecture / deployment target. They are ignored verbatim on every other
# platform, so this block is safe to evaluate unconditionally.
# The arm64 Homebrew bottles we link against (glfw, glew, freetype, lz4) are
# built with minos 15.0; matching that here keeps the linker quiet.
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS version")
endif()
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS build architectures")
endif()

project(OpenStory)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES
            "src/*.h"
            "src/*.cpp"
            "src/*/*.cpp"
            "src/*/*.h"
    )
# Embed the Windows app/window icon (Explorer exe icon + GLFW window icon).
if(WIN32)
    list(APPEND SOURCE_FILES "${CMAKE_SOURCE_DIR}/resource/OpenStory.rc")
endif(WIN32)

add_executable(OpenStory ${SOURCE_FILES})

set_target_properties(OpenStory PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/wz
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/wz
)

# Emit a PDB for optimized Release builds so crash logs (CrashLog.cpp) resolve to
# real function names + line numbers instead of raw addresses.
if(MSVC)
    target_compile_options(OpenStory PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_link_options(OpenStory PRIVATE $<$<CONFIG:Release>:/DEBUG>)
endif()

# ADD_CUSTOM_COMMAND(TARGET OpenStory POST_BUILD
#                 COMMAND copy \"${CMAKE_SOURCE_DIR}\vendor\bass24\x64\bass.dll\"  \"${RUNTIME_OUTPUT_DIRECTORY_DEBUG}/\"
#                 COMMENT "copying dlls …"
#                 )


find_package(OpenGL REQUIRED)
TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${OPENGL_INCLUDE_DIRS})
target_link_libraries(OpenStory ${OPENGL_LIBRARIES})


if(WIN32)

    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx)
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${CMAKE_SOURCE_DIR}/vendor/stb)
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${CMAKE_SOURCE_DIR}/vendor/freetype/include)
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glew-2.1.0/include/GL)
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${CMAKE_SOURCE_DIR}/vendor/bass24/c)
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glfw-3.3.2.bin.WIN32/include/GLFW)

    # Build NoLifeNx (and lz4) from source instead of the stale prebuilt
    # NoLifeNx.lib, so fixes to the bitmap/node code take effect on a normal
    # rebuild. lz4.c is compiled with MSVC here (the bundled liblz4_static.lib is
    # a MinGW build and won't link against MSVC).
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/includes/lz4_v1_8_2_win64/include)
    target_sources(OpenStory PRIVATE
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/audio.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/bitmap.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/file.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/node.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/nx.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/includes/lz4_v1_8_2_win64/lz4.c)

    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/freetype/win64/freetype.lib)
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/bass24/c/x64/bass.lib)
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/glew-2.1.0/lib/Release/x64/glew32s.lib)
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/glfw-3.3.2.bin.WIN64/lib-vc2019/glfw3.lib)
    target_link_libraries(OpenStory Iphlpapi.lib)
endif(WIN32)


# ---------------------------------------------------------------------------
# macOS (Apple Silicon) — Homebrew deps + vendored BASS/NoLifeNx.
#
# NOTE: this branch must come before the if(UNIX) one below, and if(UNIX) is
# guarded with NOT APPLE, because UNIX is TRUE on macOS and that branch is dead
# HeavenClient cruft (libs/, X11, OpenAL, alure) that does not exist here.
# ---------------------------------------------------------------------------
if(APPLE)

    # Homebrew prefix: /opt/homebrew on Apple Silicon, /usr/local on Intel.
    if(NOT DEFINED HOMEBREW_PREFIX)
        if(EXISTS "/opt/homebrew/bin/brew")
            set(HOMEBREW_PREFIX "/opt/homebrew")
        else()
            set(HOMEBREW_PREFIX "/usr/local")
        endif()
    endif()
    message(STATUS "OpenStory: using Homebrew prefix ${HOMEBREW_PREFIX}")

    # Sources include bare filenames (<glew.h>, <glfw3.h>, <bass.h>), so the
    # include dirs have to point *into* the GL/ and GLFW/ subdirectories, the
    # same way the WIN32 branch adds vendor/glew-2.1.0/include/GL.
    # Each package is probed under both ${prefix}/opt/<pkg>/include (the
    # versionless Cellar symlink) and ${prefix}/include (the flat link farm).
    find_path(MS_GLEW_INCLUDE_DIR glew.h
        HINTS "${HOMEBREW_PREFIX}/opt/glew/include/GL" "${HOMEBREW_PREFIX}/include/GL")
    find_path(MS_GLFW_INCLUDE_DIR glfw3.h
        HINTS "${HOMEBREW_PREFIX}/opt/glfw/include/GLFW" "${HOMEBREW_PREFIX}/include/GLFW")
    find_path(MS_FREETYPE_INCLUDE_DIR ft2build.h
        HINTS "${HOMEBREW_PREFIX}/opt/freetype/include/freetype2" "${HOMEBREW_PREFIX}/include/freetype2")
    find_path(MS_LZ4_INCLUDE_DIR lz4.h
        HINTS "${HOMEBREW_PREFIX}/opt/lz4/include" "${HOMEBREW_PREFIX}/include")
    find_path(MS_ASIO_INCLUDE_DIR asio.hpp
        HINTS "${HOMEBREW_PREFIX}/opt/asio/include" "${HOMEBREW_PREFIX}/include")

    find_library(MS_GLEW_LIBRARY NAMES GLEW glew
        HINTS "${HOMEBREW_PREFIX}/opt/glew/lib" "${HOMEBREW_PREFIX}/lib")
    find_library(MS_GLFW_LIBRARY NAMES glfw glfw3
        HINTS "${HOMEBREW_PREFIX}/opt/glfw/lib" "${HOMEBREW_PREFIX}/lib")
    find_library(MS_FREETYPE_LIBRARY NAMES freetype
        HINTS "${HOMEBREW_PREFIX}/opt/freetype/lib" "${HOMEBREW_PREFIX}/lib")
    find_library(MS_LZ4_LIBRARY NAMES lz4
        HINTS "${HOMEBREW_PREFIX}/opt/lz4/lib" "${HOMEBREW_PREFIX}/lib")

    foreach(_ms_dep
            MS_GLEW_INCLUDE_DIR MS_GLFW_INCLUDE_DIR MS_FREETYPE_INCLUDE_DIR
            MS_LZ4_INCLUDE_DIR MS_ASIO_INCLUDE_DIR
            MS_GLEW_LIBRARY MS_GLFW_LIBRARY MS_FREETYPE_LIBRARY MS_LZ4_LIBRARY)
        if(NOT ${_ms_dep})
            message(FATAL_ERROR
                "OpenStory (macOS): ${_ms_dep} not found. Install the Homebrew deps with:\n"
                "  brew install glfw glew freetype lz4 asio")
        endif()
    endforeach()

    # BASS is vendored. Use the macOS SDK header, NOT vendor/bass24/c/bass.h
    # (that one is the Windows variant and pulls in windows.h types).
    set(MS_BASS_DIR "${CMAKE_SOURCE_DIR}/vendor/bass24/macos")
    set(MS_BASS_LIBRARY "${MS_BASS_DIR}/libbass.dylib")
    if(NOT EXISTS "${MS_BASS_LIBRARY}")
        message(FATAL_ERROR "OpenStory (macOS): missing ${MS_BASS_LIBRARY}")
    endif()

    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx        # <nlnx/nx.hpp>
        ${CMAKE_SOURCE_DIR}/vendor/stb             # <stb_image.h>
        ${CMAKE_SOURCE_DIR}/platform/shared        # PlatformConfig.h / GLCompat.h
        ${MS_BASS_DIR}                             # <bass.h> (macOS flavour)
        ${MS_GLEW_INCLUDE_DIR}                     # <glew.h>
        ${MS_GLFW_INCLUDE_DIR}                     # <glfw3.h>
        ${MS_FREETYPE_INCLUDE_DIR}                 # <ft2build.h>
        ${MS_LZ4_INCLUDE_DIR}                      # <lz4.h> (for NoLifeNx)
        ${MS_ASIO_INCLUDE_DIR}                     # "asio.hpp" (header-only)
    )

    # Build NoLifeNx from source, exactly like the WIN32 branch. It already has
    # a POSIX mmap path, so no patching is needed; lz4 comes from Homebrew
    # instead of the bundled MinGW .lib.
    target_sources(OpenStory PRIVATE
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/audio.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/bitmap.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/file.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/node.cpp
        ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/nx.cpp)

    # ASIO is used standalone (no Boost); silence the deprecated OpenGL 1.x/3.x
    # entry points Apple warns about on every single translation unit.
    target_compile_definitions(OpenStory PRIVATE
        ASIO_STANDALONE
        ASIO_HAS_STD_ADDRESSOF
        ASIO_HAS_STD_ARRAY
        ASIO_HAS_CSTDINT
        ASIO_HAS_STD_SHARED_PTR
        ASIO_HAS_STD_TYPE_TRAITS
        GL_SILENCE_DEPRECATION)

    target_link_libraries(OpenStory
        ${MS_GLFW_LIBRARY}
        ${MS_GLEW_LIBRARY}
        ${MS_FREETYPE_LIBRARY}
        ${MS_LZ4_LIBRARY}
        ${MS_BASS_LIBRARY}
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreFoundation"
        # ScreenResolution.h queries the main display through CGDisplay*.
        "-framework CoreGraphics")

    # libbass.dylib has an install name of @rpath/libbass.dylib, so the
    # executable needs an rpath that resolves it. Both layouts the binary can
    # run from are baked in at link time: @executable_path covers the copy
    # dropped next to wz/OpenStory, @executable_path/../Frameworks covers
    # Contents/Frameworks inside the .app. Doing it here rather than with
    # install_name_tool afterwards keeps the ad-hoc signature the linker puts
    # on every arm64 binary intact.
    #
    # BUILD_WITH_INSTALL_RPATH pins the list to exactly these two entries.
    # Without it CMake also appends the directory of every linked shared
    # library, which puts an absolute vendor/bass24/macos rpath in the binary
    # and quietly makes the "relocatable" bundle depend on the source tree.
    # Nothing else needs an rpath: the Homebrew dylibs are all linked by
    # absolute install name, only BASS uses @rpath.
    set_target_properties(OpenStory PROPERTIES
        MACOSX_RPATH ON
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path;@executable_path/../Frameworks"
        # Debug/Release already point at wz/ above; cover the other configs
        # (and single-config builds with no CMAKE_BUILD_TYPE) too.
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/wz)

    # -----------------------------------------------------------------------
    # OpenStory.app — the client built as a real application bundle.
    #
    # A bare Mach-O executable gets no application identity on macOS: no Dock
    # tile, no menu-bar name, and (the concrete symptom) a GLFW window that is
    # created but never becomes frontmost or shows up in the on-screen window
    # list. An Info.plist fixes all of that.
    #
    # MACOSX_BUNDLE goes on the OpenStory target itself, so CFBundleExecutable
    # *is* the game — one process, no launcher stub, and the process is called
    # "OpenStory" everywhere. The plain executable developers run from a
    # terminal is then produced as a copy in wz/ by the POST_BUILD step below;
    # the same Mach-O works in both places because it carries both rpaths and
    # because src/MapleStory.cpp resolves the data directory at startup.
    #
    #   ${CMAKE_BINARY_DIR}/OpenStory.app/Contents/Info.plist
    #                                            /MacOS/OpenStory
    #                                            /Frameworks/libbass.dylib
    #                                            /Resources/OpenStory.icns
    #                                            /Resources/DataDirectory
    #   ${CMAKE_SOURCE_DIR}/wz/OpenStory      <- same binary, run from a shell
    #   ${CMAKE_SOURCE_DIR}/wz/libbass.dylib
    #
    # The assets are NOT copied in. 3.8 GB of .nx files duplicated into a
    # bundle is unaffordable, and the client writes back into that directory
    # (Settings, screenshots/, buddymemo.txt), so the bundle only records
    # *where* the data lives, in Contents/Resources/DataDirectory.
    # See resource/macos/README.md for the full resolution order.
    # -----------------------------------------------------------------------
    option(OPENSTORY_MACOS_BUNDLE "Build OpenStory.app as well as wz/OpenStory (macOS)" ON)

    if(NOT OPENSTORY_MACOS_BUNDLE)

        # Plain executable only, straight into wz/ — the pre-bundle layout.
        add_custom_command(TARGET OpenStory POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MS_BASS_LIBRARY}"
                    "$<TARGET_FILE_DIR:OpenStory>/libbass.dylib"
            COMMENT "Copying libbass.dylib next to the OpenStory executable")

    else()

        set(OPENSTORY_BUNDLE_ID "org.openstory.client"
            CACHE STRING "CFBundleIdentifier for OpenStory.app")
        set(OPENSTORY_BUNDLE_VERSION "1.0.0"
            CACHE STRING "CFBundleShortVersionString / CFBundleVersion for OpenStory.app")
        set(OPENSTORY_DATA_DIR "${CMAKE_SOURCE_DIR}/wz"
            CACHE PATH "Directory holding the .nx assets and the Settings file")

        set(MS_BUNDLE_NAME "OpenStory")
        set(MS_BUNDLE_EXECUTABLE_NAME "OpenStory")

        # --- Icon: derive an .icns from the same art the Windows .rc embeds. --
        set(MS_ICON_SOURCE "${CMAKE_SOURCE_DIR}/resource/Icon.ico")
        set(MS_ICNS_FILE "${CMAKE_CURRENT_BINARY_DIR}/OpenStory.icns")
        set(MS_BUNDLE_ICON_ENTRY "")

        if(EXISTS "${MS_ICON_SOURCE}")
            if(NOT EXISTS "${MS_ICNS_FILE}" OR "${MS_ICON_SOURCE}" IS_NEWER_THAN "${MS_ICNS_FILE}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND}
                            "-DICON_SRC=${MS_ICON_SOURCE}"
                            "-DICNS_OUT=${MS_ICNS_FILE}"
                            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                            -P "${CMAKE_SOURCE_DIR}/resource/macos/make_icns.cmake"
                    RESULT_VARIABLE MS_ICNS_RESULT
                    OUTPUT_QUIET
                    ERROR_VARIABLE MS_ICNS_ERROR)

                if(NOT MS_ICNS_RESULT EQUAL 0)
                    message(WARNING
                        "OpenStory: could not build OpenStory.icns, the bundle will use the "
                        "generic application icon.\n${MS_ICNS_ERROR}")
                endif()
            endif()
        endif()

        if(EXISTS "${MS_ICNS_FILE}")
            # Injected verbatim into Info.plist.in (omitted entirely when there
            # is no icon, so the key is never present but empty).
            set(MS_BUNDLE_ICON_ENTRY "\t<key>CFBundleIconFile</key>\n\t<string>OpenStory</string>\n")
        endif()

        # --- Info.plist -------------------------------------------------------
        # Configured here rather than relying on CMake's built-in
        # MACOSX_BUNDLE_* substitution so that LSMinimumSystemVersion,
        # LSApplicationCategoryType and the conditional icon key can all come
        # from our own variables. The result is still installed through the
        # documented MACOSX_BUNDLE_INFO_PLIST target property.
        set(MS_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/OpenStoryInfo.plist")
        configure_file("${CMAKE_SOURCE_DIR}/resource/macos/Info.plist.in"
                       "${MS_BUNDLE_INFO_PLIST}" @ONLY)

        # --- Where the assets live -------------------------------------------
        # Copied into Contents/Resources/DataDirectory below and read back by
        # chdir_to_data_directory() in src/MapleStory.cpp.
        set(MS_BUNDLE_DATA_DIR_FILE "${CMAKE_CURRENT_BINARY_DIR}/OpenStoryDataDirectory.txt")
        file(WRITE "${MS_BUNDLE_DATA_DIR_FILE}"
            "${OPENSTORY_DATA_DIR}\n")

        # --- Turn the game target into the bundle -----------------------------
        set_target_properties(OpenStory PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${MS_BUNDLE_INFO_PLIST}"
            # Overrides the wz/ output directory set above: the .app lives in
            # the build tree, and wz/OpenStory is copied out of it below.
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}")

        if(EXISTS "${MS_ICNS_FILE}")
            target_sources(OpenStory PRIVATE "${MS_ICNS_FILE}")
            set_source_files_properties("${MS_ICNS_FILE}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources"
                GENERATED TRUE)
        endif()

        set(MS_APP_CONTENTS "$<TARGET_BUNDLE_CONTENT_DIR:OpenStory>")

        add_custom_command(TARGET OpenStory POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MS_APP_CONTENTS}/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MS_APP_CONTENTS}/Resources"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MS_BASS_LIBRARY}"
                    "${MS_APP_CONTENTS}/Frameworks/libbass.dylib"

            # Assets stay outside the bundle: record where they live, never
            # copy them in. A one-line text file rather than a symlink —
            # a symlink pointing out of the bundle trips
            # "invalid destination for symbolic link in bundle" under
            # `codesign --verify --deep --strict`.
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MS_BUNDLE_DATA_DIR_FILE}"
                    "${MS_APP_CONTENTS}/Resources/DataDirectory"

            # The terminal build: the identical binary, dropped in wz/ with its
            # BASS next to it. Taken *before* the bundle is signed so this copy
            # keeps the linker's own ad-hoc signature instead of the bundle's
            # (which seals Info.plist and CodeResources and is only meaningful
            # inside the .app).
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/wz"
            COMMAND ${CMAKE_COMMAND} -E copy
                    "$<TARGET_FILE:OpenStory>"
                    "${CMAKE_SOURCE_DIR}/wz/OpenStory"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MS_BASS_LIBRARY}"
                    "${CMAKE_SOURCE_DIR}/wz/libbass.dylib"

            # Ad-hoc signature so it launches locally without Gatekeeper noise.
            # Nested code first, then the bundle (which seals Info.plist and the
            # resources). No real identity, no notarization.
            COMMAND codesign --force --sign - "${MS_APP_CONTENTS}/Frameworks/libbass.dylib"
            COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:OpenStory>"

            COMMENT "Assembling OpenStory.app (and refreshing wz/OpenStory)")

        message(STATUS "OpenStory: bundle enabled -> ${CMAKE_BINARY_DIR}/OpenStory.app "
                       "(data directory: ${OPENSTORY_DATA_DIR})")

    endif(NOT OPENSTORY_MACOS_BUNDLE)

endif(APPLE)


if(UNIX AND NOT APPLE)
    target_link_libraries(OpenStory ${PROJECT_SOURCE_DIR}/libs/NoLifeNx/nlnx/build/libNoLifeNx.a)
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${PROJECT_SOURCE_DIR}/libs/NoLifeNx)
    target_link_libraries(OpenStory ${PROJECT_SOURCE_DIR}/libs/lz4/lib/liblz4.a)

    INCLUDE(FindFreetype)
    IF(NOT FREETYPE_FOUND)
        FIND_LIBRARY(FREETYPE_LIBRARIES NAMES libfreetype freetype.dll PATHS "./libs/MacOS" "./libs/Windows" DOC "Freetype library")
        FIND_PATH(FREETYPE_INCLUDE_DIRS ftbuild.h "./includes" DOC "Freetype includes")
    ENDIF(NOT FREETYPE_FOUND)
    INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(OpenStory ${FREETYPE_LIBRARIES})

    TARGET_LINK_LIBRARIES(OpenStory ${PROJECT_SOURCE_DIR}/libs/glad/build/libglad.a)
    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${PROJECT_SOURCE_DIR}/libs/glad/build/include)

    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${PROJECT_SOURCE_DIR}/libs/glfw/include)
    TARGET_LINK_LIBRARIES(OpenStory ${PROJECT_SOURCE_DIR}/libs/glfw/build/src/libglfw3.a)

    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${PROJECT_SOURCE_DIR}/libs/openal-soft/include)
    TARGET_LINK_LIBRARIES(OpenStory ${PROJECT_SOURCE_DIR}/libs/openal-soft/build/libopenal.so)

    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${PROJECT_SOURCE_DIR}/libs/alure/include)
    TARGET_LINK_LIBRARIES(OpenStory ${PROJECT_SOURCE_DIR}/libs/alure/build/libalure2.so)

    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${PROJECT_SOURCE_DIR}/libs/stb)

    TARGET_INCLUDE_DIRECTORIES(OpenStory PUBLIC ${PROJECT_SOURCE_DIR}/libs/asio/asio/include)
    target_link_libraries(OpenStory X11)
    target_link_libraries(OpenStory dl)
    target_link_libraries(OpenStory pthread)
endif(UNIX AND NOT APPLE)