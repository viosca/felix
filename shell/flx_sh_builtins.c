#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "linenoise.h"
#include "shell/flx_sh.h"
#include "shell/flx_sh_builtins.h"

/* =========================================================================
 * Builtin Implementations
 * ========================================================================= */

static int builtin_cd(int argc, char **argv) {
    const char *target = (argc > 1) ? argv[1] : "/";

    if (chdir(target) != 0) {
        FLX_SH_ERR("cd: %s: No such file or directory\n", target);
        return 1;
    }
    return 0;
}

static int builtin_history(int argc, char **argv) {
    int count = linenoiseHistoryGetCount();
    for (int i = 0; i < count; i++) {
        printf("  %d  %s\n", i + 1, linenoiseHistoryGet(i));
    }
    return 0;
}

static int builtin_exit(int argc, char **argv) {
    printf("exit\n");
    exit(0);
    return 0;
}

/* =========================================================================
 * Dispatcher Map
 * ========================================================================= */

typedef struct {
    const char *name;
    int (*func)(int argc, char **argv);
} builtin_cmd_t;

static const builtin_cmd_t builtins[] = {
    {"cd", builtin_cd},
    {"history", builtin_history},
    {"exit", builtin_exit},
    {NULL, NULL} /* Array terminator */
};

/* =========================================================================
 * Public API
 * ========================================================================= */

bool flx_sh_is_builtin(const char *cmd) {
    if (!cmd) return false;

    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(builtins[i].name, cmd) == 0) {
            return true;
        }
    }
    return false;
}

int flx_sh_execute_builtin(int argc, char **argv) {
    if (argc == 0 || argv[0] == NULL) return 0;

    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(builtins[i].name, argv[0]) == 0) {
            return builtins[i].func(argc, argv);
        }
    }
    return -1; /* Not found */
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
