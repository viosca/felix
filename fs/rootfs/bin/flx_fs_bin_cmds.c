#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "fs/rootfs/bin/flx_fs_bin_cmds.h"
#include "kernel/process.h"
#include "kernel/binfmt.h"

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_clear)
int flx_fs_bin_cmd_clear(int argc, char **argv) {
    printf("\x1b[2J\x1b[H");
    return 0;
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_date)
int flx_fs_bin_cmd_date(int argc, char **argv) {
    struct timeval tv;

    /* GET TIME: 'date' */
    if (argc == 1) {
        if (gettimeofday(&tv, NULL) == 0) {
            time_t now = tv.tv_sec;
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                char buffer[64];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
                printf("%s\n", buffer);
                return 0;
            }
        }
        fprintf(stderr, "date: failed to get time\n");
        return 1;
    }

    /* SET TIME: 'date -s "YYYY-MM-DD HH:MM:SS"' */
    if (argc >= 3 && strcmp(argv[1], "-s") == 0) {
        /* Combine remaining arguments in case the user didn't quote the time */
        char time_str[64] = {0};
        int pos = 0;
        for (int i = 2; i < argc && pos < (int)sizeof(time_str) - 2; i++) {
            pos += snprintf(&time_str[pos], sizeof(time_str) - pos, "%s%s",
                            argv[i], (i < argc - 1) ? " " : "");
        }

        struct tm tm_info = {0};
        int year, month, day, hour, min, sec;

        /* Parse the string directly */
        if (sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
            tm_info.tm_year = year - 1900;
            tm_info.tm_mon = month - 1;
            tm_info.tm_mday = day;
            tm_info.tm_hour = hour;
            tm_info.tm_min = min;
            tm_info.tm_sec = sec;

            /* Convert struct tm back into epoch seconds */
            time_t new_time = mktime(&tm_info);

            if (new_time != (time_t)-1) {
                tv.tv_sec = new_time;
                tv.tv_usec = 0;

                if (settimeofday(&tv, NULL) == 0) {
                    /* Print the new time to confirm it worked */
                    printf("Time set to: %s\n", time_str);
                    return 0;
                } else {
                    fprintf(stderr, "date: settimeofday failed\n");
                }
            }
        }
        fprintf(stderr, "date: invalid format. Use 'YYYY-MM-DD HH:MM:SS'\n");
        return 1;
    }

    /* Print Help */
    printf("Usage:\n");
    printf("  date                           (Show current date and time)\n");
    printf("  date -s \"YYYY-MM-DD HH:MM:SS\"  (Set the system clock)\n");
    return 1;
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_echo)
int flx_fs_bin_cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s%s", argv[i], i == argc - 1 ? "" : " ");
    }
    printf("\n");
    return 0;
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_help)
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
    printf("  Tab            Auto-complete\n");
    return 0;
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_mkdir)
int flx_fs_bin_cmd_mkdir(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: mkdir <directory>...\n");
        return 1;
    }

    int exit_code = 0;

    for (int i = 1; i < argc; i++) {
        /* Standard POSIX mkdir. Defaulting to 0777 permissions. */
        if (mkdir(argv[i], 0777) < 0) {
            printf("mkdir: cannot create directory '%s': %s\n", argv[i], strerror(errno));
            exit_code = 1;
        }
    }

    return exit_code;
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_pwd)
int flx_fs_bin_cmd_pwd(int argc, char **argv) {
    char buf[256];
    if (getcwd(buf, sizeof(buf))) {
        printf("%s\n", buf);
        return 0;
    }
    return 1;
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_which)
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

