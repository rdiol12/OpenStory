cmake_minimum_required(VERSION 3.14)
project(OpenStory)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES
            "src/*.h"
            "src/*.cpp"
            "src/*/*.cpp"
            "src/*/*.h"
    )
add_executable(OpenStory ${SOURCE_FILES})

set_target_properties(OpenStory PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/wz
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/wz
)

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
    
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/freetype/win64/freetype.lib)
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/bass24/c/x64/bass.lib)
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/NoLifeNx/nlnx/x64/Debug/NoLifeNx.lib)
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/glew-2.1.0/lib/Release/x64/glew32s.lib)
    target_link_libraries(OpenStory ${CMAKE_SOURCE_DIR}/vendor/glfw-3.3.2.bin.WIN64/lib-vc2019/glfw3.lib)
    target_link_libraries(OpenStory Iphlpapi.lib)
endif(WIN32)


if(UNIX)
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
endif(UNIX)