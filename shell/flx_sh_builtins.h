#ifndef FLX_SH_BUILTINS_H
#define FLX_SH_BUILTINS_H

#include <stdbool.h>
#include "shell/flx_sh.h"

/**
 * @brief Checks if a command string is a registered shell builtin.
 */
bool flx_sh_is_builtin(const char *cmd);

/**
 * @brief Executes a built-in shell command.
 * @param ctx Pointer to the shell context (containing state, cwd, etc.)
 * @param argc The number of arguments.
 * @param argv NULL-terminated array of arguments.
 * @return 0 on success, non-zero on failure.
 */
int flx_sh_execute_builtin(FlxShellContext_t *ctx, int argc, char **argv);

/**
 * @brief Allows other modules (like autocompletion) to iterate over registered builtins.
 * @param index The 0-based index of the builtin.
 * @return The name of the builtin, or NULL if the index is out of bounds.
 */
const char *flx_sh_get_builtin_name(int index);

#endif /* FLX_SH_BUILTINS_H */
