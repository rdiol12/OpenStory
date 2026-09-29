# Derive an .icns from the Windows icon art (resource/Icon.ico) with the stock
# macOS tools, so the bundle reuses the same maple-leaf artwork as the Windows
# build instead of shipping a second, hand-made icon.
#
# Usage:
#   cmake -DICON_SRC=<.ico|.png> -DICNS_OUT=<out.icns> -DWORK_DIR=<scratch dir>
#         -P make_icns.cmake
#
# The largest image inside Icon.ico is 256x256, so only sizes up to 256 are
# emitted; Finder scales that up for the (rare) 512/1024 slots rather than us
# baking a blurry upscale into the file.

foreach(_required ICON_SRC ICNS_OUT WORK_DIR)
    if(NOT DEFINED ${_required})
        message(FATAL_ERROR "make_icns.cmake: -D${_required}=... is required")
    endif()
endforeach()

if(NOT EXISTS "${ICON_SRC}")
    message(FATAL_ERROR "make_icns.cmake: source icon not found: ${ICON_SRC}")
endif()

find_program(SIPS_EXECUTABLE sips)
find_program(ICONUTIL_EXECUTABLE iconutil)

if(NOT SIPS_EXECUTABLE OR NOT ICONUTIL_EXECUTABLE)
    message(FATAL_ERROR "make_icns.cmake: sips and/or iconutil not available")
endif()

set(_iconset "${WORK_DIR}/OpenStory.iconset")

file(REMOVE_RECURSE "${_iconset}")
file(MAKE_DIRECTORY "${_iconset}")

# Flatten whatever the source is (.ico picks its largest image) into a PNG.
set(_base "${WORK_DIR}/OpenStory-base.png")

execute_process(
    COMMAND "${SIPS_EXECUTABLE}" -s format png "${ICON_SRC}" --out "${_base}"
    RESULT_VARIABLE _rc
    OUTPUT_QUIET
    ERROR_VARIABLE _err)

if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "make_icns.cmake: sips failed to read ${ICON_SRC}: ${_err}")
endif()

# <pixel size>:<iconset file name>
set(_variants
    16:icon_16x16.png
    32:icon_16x16@2x.png
    32:icon_32x32.png
    64:icon_32x32@2x.png
    128:icon_128x128.png
    256:icon_128x128@2x.png
    256:icon_256x256.png)

foreach(_variant IN LISTS _variants)
    string(REPLACE ":" ";" _parts "${_variant}")
    list(GET _parts 0 _size)
    list(GET _parts 1 _name)

    execute_process(
        COMMAND "${SIPS_EXECUTABLE}" -z "${_size}" "${_size}" "${_base}" --out "${_iconset}/${_name}"
        RESULT_VARIABLE _rc
        OUTPUT_QUIET
        ERROR_VARIABLE _err)

    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "make_icns.cmake: sips failed to resize to ${_size}px: ${_err}")
    endif()
endforeach()

get_filename_component(_out_dir "${ICNS_OUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")

execute_process(
    COMMAND "${ICONUTIL_EXECUTABLE}" --convert icns --output "${ICNS_OUT}" "${_iconset}"
    RESULT_VARIABLE _rc
    ERROR_VARIABLE _err)

if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "make_icns.cmake: iconutil failed: ${_err}")
endif()

file(REMOVE_RECURSE "${_iconset}")
file(REMOVE "${_base}")

message(STATUS "make_icns.cmake: wrote ${ICNS_OUT}")
