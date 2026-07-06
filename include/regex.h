/**
 * @file felix/include/regex.h
 * @brief Wrapper to fix Newlib's regex.h missing sys/types.h
 */
#ifndef _FLX_INCLUDE_REGEX_H
#define _FLX_INCLUDE_REGEX_H

/* 1. Define off_t before the real regex.h can panic */
#include <sys/types.h>

/* 2. GCC Magic: Tell the compiler to go find the *next* regex.h
 * in the search path (which will be the real one in the toolchain) */
#include_next <regex.h>

#endif /* _FLX_INCLUDE_REGEX_H */
