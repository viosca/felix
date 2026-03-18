#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <fs/bin/flx_fs_bin_cmds.h>
#include "kernel/process.h"

int flx_fs_bin_cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s%s", argv[i], i == argc - 1 ? "" : " ");
    }
    printf("\n");
    return 0;
}

int flx_fs_bin_cmd_pwd(int argc, char **argv) {
    char buf[256];
    if (getcwd(buf, sizeof(buf))) {
        printf("%s\n", buf);
        return 0;
    }
    return 1;
}

int flx_fs_bin_cmd_clear(int argc, char **argv) {
    printf("\x1b[2J\x1b[H");
    return 0;
}

int flx_fs_bin_cmd_help(int argc, char **argv) {
    printf("Felix OS Shell Builtins:\n");
    printf("  cd [dir]       Change working directory\n");
    printf("  pwd            Print working directory\n");

    printf("Terminal Shortcuts (linenoise):\n");
    printf("  Up / Down      Navigate command history\n");
    printf("  Left / Right   Move cursor\n");
    printf("  Ctrl+A         Move cursor to beginning of line\n");
    printf("  Ctrl+E         Move cursor to end of line\n");
    printf("  Ctrl+U         Delete from cursor to beginning of line\n");
    printf("  Ctrl+K         Delete from cursor to end of line\n");
    printf("  Ctrl+L         Clear screen\n");
    printf("  Ctrl+C         Cancel current line\n");
    printf("  Tab            Auto-complete (Coming soon!)\n");
    return 0;
}

int flx_fs_bin_cmd_which(int argc, char **argv) {
    if (argc < 2) {
        return 1;
    }

    const char *file = argv[1];

    /* 1. Direct path check (e.g. 'which /bin/ls') */
    if (strchr(file, '/')) {
        struct stat st;
        if (stat(file, &st) == 0 && S_ISREG(st.st_mode)) {
            printf("%s\n", file);
            return 0;
        }
        return 1;
    }

    /* 2. Search PATH (exactly like execvp does!) */
    const char *path_env = getenv("PATH");
    if (!path_env) path_env = "/bin";

    char path_copy[256];
    strncpy(path_copy, path_env, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char *saveptr;
    char *dir_path = strtok_r(path_copy, ":", &saveptr);
    char target_path[256];

    while (dir_path != NULL) {
        snprintf(target_path, sizeof(target_path), "%s/%s", dir_path, file);

        /* Ask the kernel if this node physically exists and is executable */
        struct stat st;
        if (stat(target_path, &st) == 0 && S_ISREG(st.st_mode)) {
            printf("%s\n", target_path);
            return 0;
        }

        dir_path = strtok_r(NULL, ":", &saveptr);
    }

    return 1; /* Command not found in PATH */
}

