/**
 * @file sh.h
 * @brief Core definitions and entry point for the Felix OS Shell.
 */

#ifndef FLX_SH_H
#define FLX_SH_H

#include <stdio.h>
#include <limits.h>
//#include <syslimits.h>
#include <stdbool.h>
#define __need_getopt_newlib
#include <getopt.h>

#if 0
typedef struct FlxShellContext {
    struct getopt_data go_data;
    char cwd[PATH_MAX];
    char prompt[PATH_MAX + 32];
    char expanded_line[LINE_MAX];
    bool running;
    int last_exit_code;
} FlxShellContext_t;
#else
typedef struct FlxShellContext {
    struct getopt_data go_data;

    char cwd[PATH_MAX]              __attribute__((aligned(8)));
    char prompt[PATH_MAX + 32]      __attribute__((aligned(8)));
    char expanded_line[LINE_MAX]    __attribute__((aligned(8)));

    int last_exit_code; /* Grouping 4-byte and 1-byte variables at the end reduces padding */
    bool running;
} FlxShellContext_t;
#endif

/**
 * @brief The official name of the shell, used as a prefix in all error messages.
 */
#define FLX_SH_NAME "felix"

/**
 * @brief Standardized error printer for the shell.
 * Automatically prepends the shell name and routes output strictly to stderr.
 * * @param[in] fmt The format string.
 * @param[in] ... Variadic arguments for the format string.
 */
#define FLX_SH_ERR(fmt, ...) fprintf(stderr, FLX_SH_NAME ": " fmt, ##__VA_ARGS__)

/**
 * @brief Main entry point for the Felix OS interactive shell.
 * * Initializes the linenoise terminal interface, sets up auto-completion callbacks,
 * and enters the main REPL (Read-Eval-Print-Loop) to parse and execute user commands.
 * * @param[in] argc The number of command-line arguments passed to the shell (usually 0 at boot).
 * @param[in] argv An array of null-terminated strings representing the arguments.
 * @return 0 on normal exit, or a non-zero error code on failure.
 */
int flx_sh_main(int argc, char **argv);

#endif /* FLX_SH_H */
