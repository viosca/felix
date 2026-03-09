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

/* =========================================================================
 * Shell Auto-Completion Callback
 * ========================================================================= */

static void flx_sh_completion(const char *buf, linenoiseCompletions *lc) {
    char prefix[256];
    strncpy(prefix, buf, sizeof(prefix) - 1);
    prefix[sizeof(prefix) - 1] = '\0';

    /* Isolate the current word being typed */
    char *last_word = strrchr(prefix, ' ');
    if (last_word) {
        last_word++; /* Skip the space */
    } else {
        last_word = prefix; /* It's the first word in the buffer */
    }

    char dir_path[256] = ".";
    char search_prefix[256] = "";

    /* Separate the directory path from the partial filename */
    char *last_slash = strrchr(last_word, '/');
    if (last_slash) {
        int dir_len = last_slash - last_word;
        if (dir_len == 0) {
            strcpy(dir_path, "/"); /* e.g., typing "/bi" */
        } else {
            strncpy(dir_path, last_word, dir_len);
            dir_path[dir_len] = '\0';
        }
        strcpy(search_prefix, last_slash + 1);
    } else {
        strcpy(search_prefix, last_word);
        /* If it's the very first word and no slash, auto-complete from /bin */
        if (last_word == prefix) {
            strcpy(dir_path, "/bin");
        }
    }

    /* Ask the VFS for the directory contents! */
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    int search_len = strlen(search_prefix);

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, search_prefix, search_len) == 0) {
            /* Skip hidden files unless we explicitly typed a dot */
            if (entry->d_name[0] == '.' && search_prefix[0] != '.') continue;

            char completion[512];
            int base_len = last_word - prefix;

            /* Reconstruct the buffer string */
            if (last_slash) {
                if (strcmp(dir_path, "/") == 0) {
                    snprintf(completion, sizeof(completion), "%.*s/%s", base_len, prefix, entry->d_name);
                } else {
                    snprintf(completion, sizeof(completion), "%.*s%s/%s", base_len, prefix, dir_path, entry->d_name);
                }
            } else {
                snprintf(completion, sizeof(completion), "%.*s%s", base_len, prefix, entry->d_name);
            }

            /* Use POSIX stat to check if it's a directory, and append a slash if so */
            struct stat st;
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                strcat(completion, "/");
            } else {
                strcat(completion, " "); /* Add a space after a completed file or command! */
            }

            linenoiseAddCompletion(lc, completion);
        }
    }
    closedir(dir);
}

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

/* =========================================================================
 * Main Command Execution Pipeline
 * ========================================================================= */

void flx_sh_execute_line(const char *raw_line) {
    if (!raw_line || strlen(raw_line) == 0) return;

    char expanded_line[256];

    /* STAGE 1: History Expansion (e.g. !!, !-2) */
    if (!flx_sh_history_expand(raw_line, expanded_line, sizeof(expanded_line))) {
        return; /* History event not found */
    }

    wordexp_t p;

    /* Lexical Analysis & Variable Expansion */
    if (wordexp(expanded_line, &p, 0) != 0) {
        FLX_SH_ERR("syntax error\n");
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
        flx_sh_execute_builtin(argc, argv);
        wordfree(&p);
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

int flx_sh_main(int argc, char **argv) {
    char *line;
    char prompt[256];
    char cwd[256];

    printf("Welcome to Felix OS!\n");

    /* Initialize Linenoise Features */
    linenoiseHistorySetMaxLen(100);
    linenoiseSetCompletionCallback(flx_sh_completion);

    while (1) {
        if (getcwd(cwd, sizeof(cwd))) {
            snprintf(prompt, sizeof(prompt), "felix:%s> ", cwd);
        } else {
            snprintf(prompt, sizeof(prompt), "felix:> ");
        }

        /* Hand over terminal control to linenoise */
        line = linenoise(prompt);

        if (line == NULL) {
            break; /* EOF or Ctrl+D received */
        }

        if (line[0] != '\0') {
            char expanded_line[512];

            /* EXPAND FIRST: Pre-process the history tokens */
            if (flx_sh_history_expand(line, expanded_line, sizeof(expanded_line))) {

                /* ADD TO HISTORY: Only save the cleanly expanded command! */
                linenoiseHistoryAdd(expanded_line);

                /* EXECUTE: Push it down the pipeline */
                flx_sh_split_and_execute(expanded_line);
            }
        }

        free(line); /* Free the buffer allocated by linenoise */
    }

    return 0;
}
