#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include "linenoise.h"
#include "shell/flx_sh.h"
#include "shell/flx_sh_completion.h"
#include "shell/flx_sh_registry.h"
#include "shell/flx_sh_builtins.h"
#include "kernel/syscalls.h"

void flx_sh_completion(const char *buf, linenoiseCompletions *lc) {
    if (!buf) return;

    /* Use the system-wide POSIX constant */
    char line_copy[LINE_MAX];
    strncpy(line_copy, buf, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    char *last_word = strrchr(line_copy, ' ');
    int base_len = 0;

    if (last_word) {
        *last_word = '\0';
        base_len = last_word - line_copy + 1;
        last_word++; /* advance past space */
    } else {
        last_word = line_copy;
    }

    char *last_slash = strrchr(last_word, '/');
    char search_dir[PATH_MAX] = {0};
    const char *search_prefix = NULL;

    if (last_slash) {
        /* We are completing a path with a directory explicitly specified */
        size_t dir_len = last_slash - last_word;
        if (dir_len == 0) {
            strncpy(search_dir, "/", sizeof(search_dir) - 1);
        } else {
            strncpy(search_dir, last_word, dir_len);
        }
        search_prefix = last_slash + 1;

        size_t prefix_len = strlen(search_prefix);
        DIR *dir = opendir(search_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strncmp(entry->d_name, search_prefix, prefix_len) == 0) {
                    if (entry->d_name[0] == '.' && search_prefix[0] != '.') continue;

                    char full_completion[LINE_MAX];
                    snprintf(full_completion, sizeof(full_completion), "%.*s%s/%s",
                             base_len, buf, search_dir, entry->d_name);
                    linenoiseAddCompletion(lc, full_completion);
                }
            }
            closedir(dir);
        }
    } else {
        /* We are completing a command name or a file in the current directory */
        search_prefix = last_word;
        size_t prefix_len = strlen(search_prefix);

        /* 1. Complete built-ins */
        int builtin_idx = 0;
        const char *builtin_name;
        while ((builtin_name = flx_sh_get_builtin_name(builtin_idx++)) != NULL) {
            if (strncmp(builtin_name, search_prefix, prefix_len) == 0) {
                char full_completion[LINE_MAX];
                snprintf(full_completion, sizeof(full_completion), "%.*s%s",
                         base_len, buf, builtin_name);
                linenoiseAddCompletion(lc, full_completion);
            }
        }

        /* 2. Complete from dynamic PATH environment */
        const char *path_env = sys_getenv("PATH");
        const char *search_path = path_env ? path_env : "/bin";

        char path_buf[LINE_MAX];
        strncpy(path_buf, search_path, sizeof(path_buf) - 1);
        path_buf[sizeof(path_buf) - 1] = '\0';

        char *saveptr;
        char *current_dir = strtok_r(path_buf, ":", &saveptr);

        while (current_dir != NULL) {
            DIR *dir = opendir(current_dir);
            if (dir) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    if (strncmp(entry->d_name, search_prefix, prefix_len) == 0) {
                        if (entry->d_name[0] == '.' && search_prefix[0] != '.') continue;

                        char full_completion[LINE_MAX];
                        snprintf(full_completion, sizeof(full_completion), "%.*s%s",
                                 base_len, buf, entry->d_name);
                        linenoiseAddCompletion(lc, full_completion);
                    }
                }
                closedir(dir);
            }
            /* Advance to the next directory in the PATH */
            current_dir = strtok_r(NULL, ":", &saveptr);
        }

        /* 3. Complete from Current Working Directory (CWD) */
        DIR *dir = opendir(".");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strncmp(entry->d_name, search_prefix, prefix_len) == 0) {
                    if (entry->d_name[0] == '.' && search_prefix[0] != '.') continue;

                    char full_completion[LINE_MAX];
                    snprintf(full_completion, sizeof(full_completion), "%.*s%s",
                             base_len, buf, entry->d_name);
                    linenoiseAddCompletion(lc, full_completion);
                }
            }
            closedir(dir);
        }
    }
}
