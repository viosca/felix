/**
 * @file felix/include/setjmp.h
 * @brief Wrapper to provide POSIX sigjmp_buf and stubbed signal macros for Toybox.
 */
#ifndef _FLX_INCLUDE_SETJMP_H
#define _FLX_INCLUDE_SETJMP_H

/* 1. Pull in the real setjmp.h from the GCC toolchain */
#include_next <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 2. Provide the missing POSIX signal jump types/macros */
#ifndef _SIGJMP_BUF_DEFINED
#define _SIGJMP_BUF_DEFINED

/* Since Felix OS lacks POSIX signal masks, a sigjmp_buf is just a normal jmp_buf */
typedef jmp_buf sigjmp_buf;

/* Map the sigsetjmp/siglongjmp macros to standard setjmp/longjmp.
 * The 'savesigs' argument in sigsetjmp is intentionally ignored. */
#define sigsetjmp(env, savesigs) setjmp(env)
#define siglongjmp(env, val)     longjmp(env, val)

/* 3. Stubbed signal definitions to satisfy Toybox's Linux assumptions.
 * These are used by Toybox to request signal handling, but they will
 * not trigger actual kernel interrupts in Felix OS. */
#ifndef SIGINT
#define SIGINT  2
#endif

#ifndef SIGALRM
#define SIGALRM 14
#endif

#endif /* _SIGJMP_BUF_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SETJMP_H */
