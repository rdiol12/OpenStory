set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(NX "${ROOT}/vendor/NoLifeNx")
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${ROOT}/src/*.cpp")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/IO/Window.cpp$")
if(PS5)
    list(FILTER GAME_SOURCES EXCLUDE REGEX "/Net/SocketAsio.cpp$")
endif()
add_executable(OpenStory ${GAME_SOURCES} WindowSDL.cpp Identity.cpp ConsoleMenu.cpp
    ${NX}/nlnx/audio.cpp ${NX}/nlnx/bitmap.cpp ${NX}/nlnx/file.cpp
    ${NX}/nlnx/node.cpp ${NX}/nlnx/nx.cpp
    ${NX}/nlnx/includes/lz4_v1_8_2_win64/lz4.c)
target_compile_definitions(OpenStory PRIVATE OPENSTORY_SDL OPENSTORY_GL_CORE
    USE_ASIO ASIO_STANDALONE SDL_MAIN_HANDLED GLFW_INCLUDE_NONE)
target_include_directories(OpenStory PRIVATE ${ROOT}/src ${NX} ${ROOT}/vendor/stb
    ${ROOT}/vendor/glfw-3.3.2.bin.WIN64/include/GLFW
    ${NX}/nlnx/includes/lz4_v1_8_2_win64/include)
if(PS5)
    find_package(PS5OpenGL CONFIG REQUIRED)
    if(NOT EXISTS "${PS5_NATIVE_DIR}/app_crt.cpp" OR NOT EXISTS "${OPENSTORY_OPENGL_PRESENTER}")
        message(FATAL_ERROR "Run platform/ps5/build.py to prepare the native runtime")
    endif()
    target_sources(OpenStory PRIVATE ../ps5/Native.cpp ../ps5/Allocation.c ../ps5/Keyboard.cpp
        ${PS5_NATIVE_DIR}/app_crt.cpp ${PS5_NATIVE_DIR}/app_cpp_runtime.cpp
        ${OPENSTORY_OPENGL_PRESENTER}
        ${PS5_LIBRARY_DIR}/ps5/frontend/ps5_audio.cpp)
    if(OPENSTORY_LOG_HOST)
        target_sources(OpenStory PRIVATE ../ps5/Diagnostics.c)
        set_source_files_properties(../ps5/Diagnostics.c PROPERTIES COMPILE_OPTIONS "-fno-builtin")
        set_source_files_properties(../ps5/Native.cpp WindowSDL.cpp PROPERTIES COMPILE_DEFINITIONS OPENSTORY_LAN_LOG)
        set_source_files_properties(../ps5/Diagnostics.c PROPERTIES
            COMPILE_DEFINITIONS "OPENSTORY_LOG_ADDRESS=${OPENSTORY_LOG_ADDRESS};OPENSTORY_CONTENT_VERSION_HEADER=\"${PS5_NATIVE_DIR}/openstory-version.h\"")
        target_link_options(OpenStory PRIVATE LINKER:--wrap=_init_env)
        target_link_options(OpenStory PRIVATE
            LINKER:--wrap=ps5_screen_prepare_present LINKER:--wrap=ps5_agc_gate2_present
            LINKER:--wrap=sceVideoOutSubmitFlip LINKER:--wrap=sceVideoOutWaitVblank
            LINKER:--wrap=sceVideoOutIsFlipPending)
    endif()
    set_source_files_properties(${ROOT}/src/MapleStory.cpp PROPERTIES COMPILE_DEFINITIONS "main=openstory_main")
    if(OPENSTORY_LOG_HOST)
        set_property(SOURCE ${ROOT}/src/MapleStory.cpp APPEND PROPERTY COMPILE_DEFINITIONS OPENSTORY_LAN_LOG)
        set_property(SOURCE ${ROOT}/src/Net/SocketPS5.cpp APPEND PROPERTY COMPILE_DEFINITIONS OPENSTORY_LAN_LOG)
        set_property(SOURCE ${ROOT}/src/Audio/Audio.cpp APPEND PROPERTY COMPILE_DEFINITIONS OPENSTORY_LAN_LOG)
        set_property(SOURCE ${ROOT}/src/IO/UI.cpp APPEND PROPERTY COMPILE_DEFINITIONS OPENSTORY_LAN_LOG)
        set_property(SOURCE ${ROOT}/src/Util/NxFiles.cpp ${NX}/nlnx/file.cpp APPEND PROPERTY COMPILE_DEFINITIONS OPENSTORY_LAN_LOG)
    endif()
    set_source_files_properties(../ps5/Allocation.c PROPERTIES COMPILE_OPTIONS "-fno-builtin")
    target_link_options(OpenStory PRIVATE -nostartfiles -nostdlib
        "LINKER:-T,${PS5_NATIVE_DIR}/native.ld"
        "LINKER:--version-script,${PS5_LIBRARY_DIR}/upstream/ProsperoTV/tooling/native/app-symbols.map"
        LINKER:--gc-sections LINKER:--exclude-libs=ALL LINKER:-e,_start
        LINKER:--wrap=fcntl LINKER:--wrap=sceAudioOutOpen LINKER:--wrap=sceAudioOutOutput
        LINKER:--wrap=sceKeyboardInit LINKER:--wrap=sceKeyboardOpen)
    foreach(hook __dlopen __dlsym __dladdr __dlclose __dlerror _ZTH23_mesa_glapi_tls_Context)
        target_link_options(OpenStory PRIVATE "LINKER:--defsym=${hook}=0")
    endforeach()
    set(PORTS "$ENV{PS5_PAYLOAD_SDK}/target/user/homebrew")
    find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
    target_compile_definitions(OpenStory PRIVATE PLATFORM_PS5)
    target_include_directories(OpenStory SYSTEM PRIVATE ${PORTS}/include
        ${PORTS}/include/SDL2 ${PORTS}/include/freetype2 ${ASIO_INCLUDE_DIR})
    target_link_directories(OpenStory PRIVATE ${PORTS}/lib ${PS5_NATIVE_DIR})
    # Resolve ordinary native memory APIs before the payload SDK's libc wrappers.
    target_link_libraries(OpenStory PRIVATE -Wl,--start-group kernel PS5OpenGL::OpenGL SDL2_mixer SDL2
        freetype png z bz2 iconv
        SceAudioOut ScePad SceUserService SceSystemService SceSysmodule
        SceVideoOut SceKeyboard SceImeDialog SceCommonDialog SceNet c++ c++abi unwind c
        -Wl,--end-group SceLibcInternal kernel)
    set_target_properties(OpenStory PROPERTIES SUFFIX ".elf")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl2 SDL2_mixer freetype2 glew)
    find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
    find_path(GLEW_FLAT_INCLUDE glew.h PATH_SUFFIXES GL REQUIRED)
    target_include_directories(OpenStory SYSTEM PRIVATE ${ASIO_INCLUDE_DIR} ${GLEW_FLAT_INCLUDE})
    target_link_libraries(OpenStory PRIVATE PkgConfig::SDL pthread GL)
endif()
