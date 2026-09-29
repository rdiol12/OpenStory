# Core/base configuration only. These targets do not use AWT/sound/printing.
# Do not use this configuration to claim a complete JDK build.
AC_DEFUN_ONCE([CUSTOM_EARLY_HOOK], [
m4_define([LIB_DETERMINE_DEPENDENCIES], [
  NEEDS_LIB_X11=false
  NEEDS_LIB_FONTCONFIG=false
  NEEDS_LIB_CUPS=false
  NEEDS_LIB_FREETYPE=false
  NEEDS_LIB_ALSA=false
  NEEDS_LIB_FFI=true
])
])
