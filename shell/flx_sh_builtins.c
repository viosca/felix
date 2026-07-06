/**
 * @file shell/flx_sh_builtins.c
 * @brief Implementation of shell built-in commands using FlxShellContext.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "linenoise.h"
#include "shell/flx_sh.h"
#include "shell/flx_sh_builtins.h"

/* =========================================================================
 * Builtin Implementations
 * ========================================================================= */

static int builtin_cd(FlxShellContext_t *ctx, int argc, char **argv) {
    const char *target = (argc > 1) ? argv[1] : "/";

    if (chdir(target) != 0) {
        FLX_SH_ERR("cd: %s: No such file or directory\n", target);
        return 1;
    }
    return 0;
}

static int builtin_history(FlxShellContext_t *ctx, int argc, char **argv) {
    int count = linenoiseHistoryGetCount();
    for (int i = 0; i < count; i++) {
        printf("  %d  %s\n", i + 1, linenoiseHistoryGet(i));
    }
    return 0;
}

static int builtin_exit(FlxShellContext_t *ctx, int argc, char **argv) {
    printf("exit\n");
    /* Gracefully signal the loop to terminate instead of calling exit() */
    ctx->running = false;
    return 0;
}

/* =========================================================================
 * Dispatcher Map
 * ========================================================================= */

typedef struct {
    const char *name;
    int (*func)(FlxShellContext_t *ctx, int argc, char **argv);
} builtin_cmd_t;

static const builtin_cmd_t builtins[] = {
    {"cd",      builtin_cd},
    {"history", builtin_history},
    {"exit",    builtin_exit},
    {NULL,      NULL}
};

/* =========================================================================
 * Public API
 * ========================================================================= */

bool flx_sh_is_builtin(const char *cmd) {
    if (!cmd) return false;
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(builtins[i].name, cmd) == 0) return true;
    }
    return false;
}

int flx_sh_execute_builtin(FlxShellContext_t *ctx, int argc, char **argv) {
    if (argc == 0 || argv[0] == NULL) return 0;

    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(builtins[i].name, argv[0]) == 0) {
            return builtins[i].func(ctx, argc, argv);
        }
    }
    return -1;
}

const char *flx_sh_get_builtin_name(int index) {
    if (index < 0) return NULL;

    for (int i = 0; builtins[i].name != NULL; i++) {
        if (i == index) {
            return builtins[i].name;
        }
    }

    return NULL;
}
