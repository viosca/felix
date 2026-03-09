#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "kernel/process.h"
#include "kernel/vfs.h"
#include "cwalk.h"
#include "shell/flx_sh_builtins.h"
#include "linenoise.h"

/* Define the optparse implementation exactly once in this C file */
#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

/* --- Builtin Implementations --- */

static int builtin_cd(int argc, char **argv) {
    char clean_path[256];
    const char *cwd = getenv("PWD");
    if (!cwd) cwd = "/";

    if (argc >= 2) {
        cwk_path_get_absolute(cwd, argv[1], clean_path, sizeof(clean_path));

        if (flx_vfs_chdir(clean_path) != 0) {
            printf("cd: %s: No such directory\n", clean_path);
            return 1;
        }

        /* SUCCESS! Store the new absolute path into the Environment block */
        setenv("PWD", clean_path, 1);

    } else {
        if (flx_vfs_chdir("/") == 0) {
            setenv("PWD", "/", 1);
        }
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

/* --- Dispatcher Map --- */

typedef struct {
    const char *name;
    int (*func)(int argc, char **argv);
} builtin_cmd_t;

static const builtin_cmd_t builtins[] = {
    {"cd", builtin_cd},
    {"history", builtin_history},
    {NULL, NULL} /* Array terminator */
};

/* --- Public API --- */

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
