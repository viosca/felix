/**
 * @file linenoise_wrapper.c
 * @brief Wrapper to compile linenoise without triggering IDE scanners.
 * * By including the .c file directly here, we pull the library into the
 * Felix OS build without exposing the submodule's test/example files
 * to Eclipse's aggressive auto-compilation.
 */

/* Go up from shell/ -> src/ -> felix root -> external/ */
#include "linenoise.c"
