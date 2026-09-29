cmake_minimum_required(VERSION 3.14)
project(OpenStoryPlatformChecks LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(NX "${ROOT}/vendor/NoLifeNx")
add_executable(graphics-core graphics-core.cpp
    ${ROOT}/src/Graphics/GraphicsGL.cpp ${ROOT}/src/Graphics/Color.cpp
    ${ROOT}/src/Graphics/Text.cpp ${ROOT}/src/Graphics/EmbeddedFonts.cpp
    ${ROOT}/src/Configuration.cpp ${ROOT}/src/Util/Misc.cpp
    ${NX}/nlnx/audio.cpp ${NX}/nlnx/bitmap.cpp ${NX}/nlnx/file.cpp
    ${NX}/nlnx/node.cpp ${NX}/nlnx/nx.cpp
    ${NX}/nlnx/includes/lz4_v1_8_2_win64/lz4.c)
target_compile_definitions(graphics-core PRIVATE OPENSTORY_GL_CORE)
target_include_directories(graphics-core PRIVATE ${ROOT}/src ${NX}
    ${NX}/nlnx/includes/lz4_v1_8_2_win64/include)
if(MSVC)
    target_compile_options(graphics-core PRIVATE /Gy /UNDEBUG)
    target_link_options(graphics-core PRIVATE /OPT:REF)
    target_include_directories(graphics-core PRIVATE
        ${ROOT}/vendor/freetype/include ${ROOT}/vendor/glew-2.1.0/include/GL
        ${ROOT}/vendor/glfw-3.3.2.bin.WIN64/include/GLFW)
    target_link_libraries(graphics-core PRIVATE opengl32
        ${ROOT}/vendor/freetype/win64/freetype.lib
        ${ROOT}/vendor/glew-2.1.0/lib/Release/x64/glew32s.lib
        ${ROOT}/vendor/glfw-3.3.2.bin.WIN64/lib-vc2019/glfw3.lib)
else()
    find_package(OpenGL REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    find_path(GLEW_FLAT_INCLUDE glew.h PATH_SUFFIXES GL REQUIRED)
    find_path(GLFW_FLAT_INCLUDE glfw3.h PATH_SUFFIXES GLFW REQUIRED)
    target_include_directories(graphics-core PRIVATE ${GLEW_FLAT_INCLUDE} ${GLFW_FLAT_INCLUDE})
    target_compile_options(graphics-core PRIVATE -ffunction-sections -fdata-sections -UNDEBUG)
    target_link_options(graphics-core PRIVATE -Wl,--gc-sections)
    target_link_libraries(graphics-core PRIVATE OpenGL::GL Freetype::Freetype GLEW::GLEW glfw)
endif()
enable_testing()
add_test(NAME graphics-core COMMAND graphics-core)
if(UNIX)
    add_executable(native-allocation native-allocation.c ${ROOT}/platform/ps5/Allocation.c)
    target_compile_options(native-allocation PRIVATE -fno-builtin -UNDEBUG)
    add_test(NAME native-allocation COMMAND native-allocation)
endif()
