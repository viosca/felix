#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <wordexp.h>
#include "linenoise.h"
#include "shell/flx_sh.h"
#include "shell/flx_sh_history.h"
#include "shell/flx_sh_completion.h"
#include "shell/flx_sh_builtins.h"

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

#if 0
/* =========================================================================
 * Shell Builtins (Commands that MUST modify shell state)
 * ========================================================================= */

static bool flx_sh_is_builtin(const char *cmd) {
    if (!cmd) return false;
    return (strcmp(cmd, "cd") == 0) ||
           (strcmp(cmd, "history") == 0) ||
           (strcmp(cmd, "exit") == 0);
}

static int flx_sh_execute_builtin(int argc, char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        const char *target = (argc > 1) ? argv[1] : "/";
        if (chdir(target) != 0) {
            FLX_SH_ERR("cd: %s: No such file or directory\n", target);
            return 1;
        }
        return 0;
    }
    else if (strcmp(argv[0], "history") == 0) {
        int count = linenoiseHistoryGetCount();
        for (int i = 0; i < count; i++) {
            printf("  %d  %s\n", i + 1, linenoiseHistoryGet(i));
        }
        return 0;
    }
    else if (strcmp(argv[0], "exit") == 0) {
        printf("exit\n");
        exit(0);
    }
    return -1;
}
#endif
/* =========================================================================
 * Main Command Execution Pipeline
 * ========================================================================= */

void flx_sh_execute_line(const char *raw_line) {
    if (!raw_line || strlen(raw_line) == 0) return;

    /* Move the 256-byte array off the stack to protect the CPU */
    char *expanded_line = malloc(256);
    if (!expanded_line) {
        FLX_SH_ERR("Out of memory\n");
        return;
    }

    /* STAGE 1: History Expansion (e.g. !!, !-2) */
    if (!flx_sh_history_expand(raw_line, expanded_line, 256)) {
        free(expanded_line);
        return; /* History event not found */
    }

    wordexp_t p;

    /* Lexical Analysis & Variable Expansion */
    if (wordexp(expanded_line, &p, 0) != 0) {
        FLX_SH_ERR("syntax error\n");
        free(expanded_line);
        return;
    }

    if (p.we_wordc == 0) {
        wordfree(&p);
        free(expanded_line);
        return;
    }

    int argc = p.we_wordc;
    char **argv = p.we_wordv;

    /* BUILTIN DELEGATION */
    if (flx_sh_is_builtin(argv[0])) {
        flx_sh_execute_builtin(argc, argv);
        wordfree(&p);
        free(expanded_line);
        return;
    }

    /* EXTERNAL BINARY DELEGATION ($PATH search via execvp) */
    int status = execvp(argv[0], argv);

    if (status == -1) {
        FLX_SH_ERR("%s: command not found\n", argv[0]);
    } else if (status != 0) {
        FLX_SH_ERR("Command exited with status %d\n", status);
    }

    wordfree(&p);
    free(expanded_line);
}
/* =========================================================================
 * Shell Statement Separator Logic
 * ========================================================================= */

/**
 * @brief Parses a line for ';' separators, obeying quotes, and executes each piece.
 */
static void flx_sh_split_and_execute(char *line) {
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
                flx_sh_execute_line(cmd_start); /* Execute the isolated piece */
            }

            cmd_start = &line[i + 1]; /* Advance pointer to the start of the next piece */
        }
    }

    /* Execute the final piece of the line (or the only piece, if no ';' was found) */
    if (strlen(cmd_start) > 0) {
        flx_sh_execute_line(cmd_start);
    }
}

/* =========================================================================
 * Shell Entry Point
 * ========================================================================= */

/* Group the main shell state arrays into a heap context */
typedef struct {
    char prompt[256];
    char cwd[256];
    char expanded_line[512];
} FlxMainCtx_t;

int flx_sh_main(int argc, char **argv) {
    char *line;

    /* Allocate the shell's running state on the heap */
    FlxMainCtx_t *ctx = malloc(sizeof(FlxMainCtx_t));
    if (!ctx) {
        printf("Felix OS: Out of Memory! Cannot start shell.\n");
        return -1;
    }

    printf("Welcome to Felix OS!\n");

    /* Initialize Linenoise Features */
    linenoiseHistorySetMaxLen(100);
    linenoiseSetCompletionCallback(flx_sh_completion);

    while (1) {
        if (getcwd(ctx->cwd, sizeof(ctx->cwd))) {
            snprintf(ctx->prompt, sizeof(ctx->prompt), "felix:%s> ", ctx->cwd);
        } else {
            snprintf(ctx->prompt, sizeof(ctx->prompt), "felix:> ");
        }

        /* Hand over terminal control to linenoise */
        line = linenoise(ctx->prompt);

        if (line == NULL) {
            break; /* EOF or Ctrl+D received */
        }

        if (line[0] != '\0') {
            /* EXPAND FIRST: Pre-process the history tokens */
            if (flx_sh_history_expand(line, ctx->expanded_line, sizeof(ctx->expanded_line))) {

                /* ADD TO HISTORY: Only save the cleanly expanded command! */
                linenoiseHistoryAdd(ctx->expanded_line);

                /* EXECUTE: Push it down the pipeline */
                flx_sh_split_and_execute(ctx->expanded_line);
            }
        }

        free(line); /* Free the buffer allocated by linenoise */
    }

    free(ctx); /* Clean up if the shell loop is ever exited */
    return 0;
}
