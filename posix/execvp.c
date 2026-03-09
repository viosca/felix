#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "kernel/process.h"

/**
 * @brief Replaces the current process image with a new process image.
 * This implementation searches the $PATH environment variable to find the executable.
 * @param[in] file The name of the file to be executed.
 * @param[in] argv A null-terminated array of character pointers to null-terminated strings.
 * @return Returns -1 on error. On success, it technically shouldn't return (in a true OS),
 * but currently returns the exit status of the synchronous command.
 */
int execvp(const char *file, char *const argv[]) {
    if (!file || !argv) return -1;

    /* Count argc since our internal flx_proc_exec syscall requires it */
    int argc = 0;
    while (argv[argc] != NULL) argc++;

    /* Direct Path Execution (no PATH search needed) */
    if (strchr(file, '/')) {
        return flx_proc_exec(file, argc, (char **)argv);
    }

    /* PATH Environment Variable Search */
    const char *path_env = getenv("PATH");
    if (!path_env) path_env = "/bin"; /* Default fallback if unset */

    char path_copy[256];
    strncpy(path_copy, path_env, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char *saveptr;
    char *dir_path = strtok_r(path_copy, ":", &saveptr);
    char target_path[256];

    while (dir_path != NULL) {
        /* Construct the absolute path to test */
        snprintf(target_path, sizeof(target_path), "%s/%s", dir_path, file);

        /* Attempt to execute it via the kernel syscall.
         * If the node exists and is executable, this will succeed. */
        int status = flx_proc_exec(target_path, argc, (char **)argv);

        if (status != -1) {
            /* Execution succeeded (or ran and returned an exit code) */
            return status;
        }

        /* Not found in this directory, check the next one in $PATH */
        dir_path = strtok_r(NULL, ":", &saveptr);
    }

    /* Command was not found anywhere in the path */
    return -1;
}
