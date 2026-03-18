#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include "linenoise.h"
#include "shell/flx_sh_completion.h"
#include "shell/flx_sh_registry.h"
#include "shell/flx_sh_builtins.h"

/* The standard POSIX Search Path array */
static const char *g_path_env[] = {
    "/bin",
    /* "/sbin", You can add more directories here in the future! */
    NULL
};

void flx_sh_completion(const char *buf, linenoiseCompletions *lc) {
    if (!buf) return;

    char line_copy[LINE_MAX];
    strncpy(line_copy, buf, LINE_MAX - 1);
    line_copy[LINE_MAX - 1] = '\0';

    char *last_word = strrchr(line_copy, ' ');
    char cmd_name[NAME_MAX] = {0};

    if (last_word) {
        /* We are typing an argument (e.g., "ls /de[TAB]") */
        *last_word = '\0';
        last_word++;

        char *first_space = strchr(line_copy, ' ');
        if (first_space) *first_space = '\0';
        strncpy(cmd_name, line_copy, NAME_MAX - 1);
    } else {
        /* We are typing the command itself (e.g., "l[TAB]") */
        last_word = line_copy;
    }

    size_t prefix_len = strlen(last_word);

    /* =====================================================================
     * STAGE 1: COMMAND-SPECIFIC DELEGATION (Arguments)
     * ===================================================================== */
    if (cmd_name[0] != '\0') {
        flx_sh_complete_cb custom_complete = flx_sh_registry_get_completer(cmd_name);
        if (custom_complete) {
            custom_complete(last_word, lc);
        }
        return; /* If we are typing arguments, don't suggest command names */
    }

    /* =====================================================================
     * STAGE 2: DYNAMIC $PATH SCANNING (First Word)
     * ===================================================================== */

    /* 1. Suggest Builtins */
    for (int i = 0; ; i++) {
        const char *b_name = flx_sh_get_builtin_name(i);
        if (!b_name) break;
        if (strncmp(b_name, last_word, prefix_len) == 0) {
            linenoiseAddCompletion(lc, b_name);
        }
    }

    /* 2. Dynamically scan all directories in our $PATH */
    for (int p = 0; g_path_env[p] != NULL; p++) {
        DIR *dir = opendir(g_path_env[p]);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, last_word, prefix_len) == 0) {
                /* Because we scan a generic directory, we don't care if it's
                 * a linked builtin or an SD-card binary. It just works! */
                linenoiseAddCompletion(lc, entry->d_name);
            }
        }
        closedir(dir);
    }
}
