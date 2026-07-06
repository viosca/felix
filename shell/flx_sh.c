#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <wordexp.h>
#include <limits.h>

#define __need_getopt_newlib
#include <getopt.h>

#include "linenoise.h"
#include "shell/flx_sh.h"
#include "shell/flx_sh_history.h"
#include "shell/flx_sh_builtins.h"
#include "shell/flx_sh_completion.h"

#if 0
    int __getopt_r (int __argc, char *const __argv[], const char *__optstring,
             struct getopt_data * __data);

    int __getopt_long_r (int __argc, char *const __argv[], const char *__shortopts,
             const struct option * __longopts, int *__longind,
             struct getopt_data * __data);

    int __getopt_long_only_r (int __argc, char *const __argv[], const char *__shortopts,
             const struct option * __longopts, int *__longind,
             struct getopt_data * __data);
#endif

//#define CONFIG_USE_TOYBOX
/* Forward declaration for optional ToyBox */
#ifdef CONFIG_USE_TOYBOX
extern int toybox_main(int argc, char **argv);
#endif

/* =========================================================================
 * Main Command Execution Pipeline
 * ========================================================================= */

void flx_sh_execute_line(FlxShellContext_t *ctx, const char *raw_line) {
    int status = 0;

    if (!raw_line || strlen(raw_line) == 0) return;

#if 0
    /* STAGE 1: History Expansion (e.g. !!, !-2) */
    if (!flx_sh_history_expand(raw_line, ctx->expanded_line, sizeof(ctx->expanded_line))) {
        return; /* History event not found */
    }
#endif

    wordexp_t p;

    status = wordexp(ctx->expanded_line, &p, 0);
    /* Lexical Analysis & Variable Expansion */
    if (status != 0) {
        FLX_SH_ERR("syntax error: %s\n", worderror(status));
        return;
    }

    if (p.we_wordc == 0) {
        wordfree(&p);
        return;
    }

    int argc = p.we_wordc;
    char **argv = p.we_wordv;

    /* BUILTIN DELEGATION */
    if (flx_sh_is_builtin(argv[0])) {
        ctx->last_exit_code = flx_sh_execute_builtin(ctx, argc, argv);
        wordfree(&p);
        return;
    }

    /* EXTERNAL BINARY DELEGATION ($PATH search via execvp) */
    status = execvp(argv[0], argv);

    if (status == -1) {
        FLX_SH_ERR("%s: command not found\n", argv[0]);
        ctx->last_exit_code = 127;
    } else if (status != 0) {
        ctx->last_exit_code = status;
    }

    wordfree(&p);
}

/* =========================================================================
 * Shell Statement Separator Logic
 * ========================================================================= */

/**
 * @brief Parses a line for ';' separators, obeying quotes, and executes each piece.
 */
static void flx_sh_split_and_execute(FlxShellContext_t *ctx, char *line) {
    char *cmd_start = line;
    bool in_dquote = false;
    bool in_squote = false;

    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '"' && !in_squote) {
            in_dquote = !in_dquote;
        } else if (line[i] == '\'' && !in_dquote) {
            in_squote = !in_squote;
        } else if (line[i] == ';' && !in_dquote && !in_squote) {
            /* We found a valid separator outside of any quotes! */
            line[i] = '\0'; /* Null-terminate the string here */

            if (strlen(cmd_start) > 0) {
                flx_sh_execute_line(ctx, cmd_start); /* Execute the isolated piece */
            }

            cmd_start = &line[i + 1]; /* Advance pointer to the start of the next piece */
        }
    }

    /* Execute the final piece of the line (or the only piece, if no ';' was found) */
    if (strlen(cmd_start) > 0) {
        flx_sh_execute_line(ctx, cmd_start);
    }
}


/* =========================================================================
 * Shell Entry Point
 * ========================================================================= */
#define SHELL_USE_STATIC_CTX
int flx_sh_main(int argc, char **argv) {
#ifdef SHELL_USE_STATIC_CTX
    static FlxShellContext_t g_ctx __attribute__((aligned(16)));
    FlxShellContext_t *ctx = &g_ctx;
    memset(&g_ctx, 0, sizeof(g_ctx));
#else
    /* 1. Allocate context on heap for lifetime safety */
    FlxShellContext_t *ctx = calloc(1, sizeof(FlxShellContext_t));
    if (!ctx) {
        printf("Felix OS: Out of Memory! Cannot start shell.\n");
        return -1;
    }
#endif

    ctx->running = true;

    printf("\nWelcome to Felix OS!\n");

    /* Initialize the getopt context for the shell (avoid passing NULL for string arg) */
    //getopt_long_r(argc, argv, "", NULL, NULL, &ctx->go_data);

#ifdef CONFIG_USE_TOYBOX
    char *tb_argv[] = {"sh", NULL};
    toybox_main(1, tb_argv); 
#endif

    linenoiseHistorySetMaxLen(100);
    linenoiseSetCompletionCallback(flx_sh_completion);

    /* 2. Main REPL loop */
    while (ctx->running) {
        if (getcwd(ctx->cwd, sizeof(ctx->cwd))) {
            snprintf(ctx->prompt, sizeof(ctx->prompt), "felix:%s> ", ctx->cwd);
        } else {
            snprintf(ctx->prompt, sizeof(ctx->prompt), "felix:> ");
        }

        char *line = linenoise(ctx->prompt);
        if (line == NULL) {
            ctx->running = false;
        } else {
            if (line[0] != '\0') {
                /* Expansion and execution pipeline */
                if (flx_sh_history_expand(line, ctx->expanded_line, sizeof(ctx->expanded_line))) {
                    linenoiseHistoryAdd(ctx->expanded_line);

                    /* EXECUTE: Push it down the pipeline */
                    flx_sh_split_and_execute(ctx, ctx->expanded_line);
                }
            }
            linenoiseFree(line);
        }
    }
#ifndef SHELL_USE_STATIC_CTX
    free(ctx);
#endif
    printf("[Felix] Shell exiting.\n");
    return 0;
}
