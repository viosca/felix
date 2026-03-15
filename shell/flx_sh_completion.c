#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "linenoise.h"
#include "shell/flx_sh_completion.h"
#include "shell/flx_sh_builtins.h"
#include "kernel/vfs.h"
#include "kernel/binfmt.h"
#include "kernel/process.h"

/* =========================================================================
 * Shell Auto-Completion Callback
 * ========================================================================= */

typedef struct {
    char prefix[256];
    char cmd_name[64];  /* NEW: Stores the base command being executed */
    char dir_path[256];
    char search_prefix[256];
    char completion[512];
    char full_path[512];
    struct stat st;
} FlxCompCtx_t;

void flx_sh_completion(const char *buf, linenoiseCompletions *lc) {
    FlxCompCtx_t *ctx = malloc(sizeof(FlxCompCtx_t));
    if (!ctx) return;

    strncpy(ctx->prefix, buf, sizeof(ctx->prefix) - 1);
    ctx->prefix[sizeof(ctx->prefix) - 1] = '\0';

    /* 1. Extract the base command name (the first word) */
    ctx->cmd_name[0] = '\0';
    char *first_space = strchr(ctx->prefix, ' ');
    if (first_space) {
        int cmd_len = first_space - ctx->prefix;
        if (cmd_len >= sizeof(ctx->cmd_name)) cmd_len = sizeof(ctx->cmd_name) - 1;
        strncpy(ctx->cmd_name, ctx->prefix, cmd_len);
        ctx->cmd_name[cmd_len] = '\0';
    } else {
        strcpy(ctx->cmd_name, ctx->prefix);
    }

    /* Isolate the current word being typed at the very end */
    char *last_word = strrchr(ctx->prefix, ' ');
    if (last_word) {
        last_word++; /* Skip the space */
    } else {
        last_word = ctx->prefix; /* It's the first word in the buffer */
    }

    /* =====================================================================
     * STAGE 1: COMMAND-SPECIFIC DELEGATION
     * If we are typing arguments, check if the command has a custom completer.
     * ===================================================================== */
    if (last_word != ctx->prefix && ctx->cmd_name[0] != '\0') {
        char bin_path[128];
        snprintf(bin_path, sizeof(bin_path), "/bin/%s", ctx->cmd_name);

        FlxVfsNode_t *cmd_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), bin_path);

        if (cmd_node && S_ISREG(cmd_node->mode) && cmd_node->data) {
            uint8_t *bytes = (uint8_t *)cmd_node->data;

            /* Is it a compiled-in Felix Executable? */
            if (bytes[0] == FLX_MAGIC_0 && bytes[1] == FLX_MAGIC_1 &&
                bytes[2] == FLX_MAGIC_2 && bytes[3] == FLX_MAGIC_3 &&
                bytes[4] == FLX_BINFMT_TYPE_LINKED) {

                FlxLinkedExec_t *exec = (FlxLinkedExec_t *)cmd_node->data;
                if (exec->complete_func) {
                    /* The command has its own completer! Delegate and exit! */
                    exec->complete_func(ctx->prefix, lc);
                    free(ctx);
                    return;
                }
            }
        }
    }

    /* =====================================================================
     * STAGE 2: DEFAULT VFS FALLBACK (Paths & Binaries)
     * If we get here, either it's the first word, or the command had no
     * custom complete_func, so we just autocomplete files/directories.
     * ===================================================================== */
    strcpy(ctx->dir_path, ".");
    strcpy(ctx->search_prefix, "");

    /* Separate the directory path from the partial filename */
    char *last_slash = strrchr(last_word, '/');
    if (last_slash) {
        int dir_len = last_slash - last_word;
        if (dir_len == 0) {
            strcpy(ctx->dir_path, "/");
        } else {
            strncpy(ctx->dir_path, last_word, dir_len);
            ctx->dir_path[dir_len] = '\0';
        }
        strcpy(ctx->search_prefix, last_slash + 1);
    } else {
        strcpy(ctx->search_prefix, last_word);

        /* If it's the very first word and no slash, auto-complete from Builtins AND /bin */
        if (last_word == ctx->prefix) {
            int b_idx = 0;
            const char *b_name;
            int search_len = strlen(ctx->search_prefix);

            /* 1. Check Shell Builtins */
            while ((b_name = flx_sh_get_builtin_name(b_idx++)) != NULL) {
                if (strncmp(b_name, ctx->search_prefix, search_len) == 0) {
                    snprintf(ctx->completion, sizeof(ctx->completion), "%s ", b_name);
                    linenoiseAddCompletion(lc, ctx->completion);
                }
            }

            /* 2. Fallback to /bin directory */
            strcpy(ctx->dir_path, "/bin");
        }
    }

    /* Ask the VFS for the directory contents! */
    DIR *dir = opendir(ctx->dir_path);
    if (!dir) {
        free(ctx);
        return;
    }

    struct dirent *entry;
    int search_len = strlen(ctx->search_prefix);
    int loop_failsafe = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (loop_failsafe++ > 1000) break;

        if (strncmp(entry->d_name, ctx->search_prefix, search_len) == 0) {
            if (entry->d_name[0] == '.' && ctx->search_prefix[0] != '.') continue;

            int base_len = last_word - ctx->prefix;

            /* Reconstruct the buffer string */
            if (last_slash) {
                if (strcmp(ctx->dir_path, "/") == 0) {
                    snprintf(ctx->completion, sizeof(ctx->completion), "%.*s/%s", base_len, ctx->prefix, entry->d_name);
                } else {
                    snprintf(ctx->completion, sizeof(ctx->completion), "%.*s%s/%s", base_len, ctx->prefix, ctx->dir_path, entry->d_name);
                }
            } else {
                snprintf(ctx->completion, sizeof(ctx->completion), "%.*s%s", base_len, ctx->prefix, entry->d_name);
            }

            snprintf(ctx->full_path, sizeof(ctx->full_path), "%s/%s", ctx->dir_path, entry->d_name);

            if (stat(ctx->full_path, &ctx->st) == 0 && S_ISDIR(ctx->st.st_mode)) {
                strcat(ctx->completion, "/");
            } else {
                strcat(ctx->completion, " ");
            }

            linenoiseAddCompletion(lc, ctx->completion);
        }
    }

    closedir(dir);
    free(ctx);
}
