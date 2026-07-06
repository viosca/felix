/**
 * @file felix/include/byteswap.h
 * @brief GNU byteswap extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_BYTESWAP_H
#define _FLX_INCLUDE_BYTESWAP_H

#ifdef __cplusplus
extern "C" {
#endif

/* * Map standard GNU byte swapping macros directly to GCC's highly optimized
 * intrinsic functions. This ensures zero-overhead, inline assembly generation
 * for the RISC-V architecture without needing a dedicated .c implementation.
 */
#ifndef bswap_16
#define bswap_16(x) __builtin_bswap16(x)
#endif

#ifndef bswap_32
#define bswap_32(x) __builtin_bswap32(x)
#endif

#ifndef bswap_64
#define bswap_64(x) __builtin_bswap64(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_BYTESWAP_H */
