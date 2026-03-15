#ifndef FLX_SH_BUILTINS_H
#define FLX_SH_BUILTINS_H

#include <stdbool.h>

/**
 * @brief Checks if a command string is a registered shell builtin.
 */
bool flx_sh_is_builtin(const char *cmd);

/**
 * @brief Executes a shell builtin command.
 */
int flx_sh_execute_builtin(int argc, char **argv);

/**
 * @brief Allows other modules (like autocompletion) to iterate over registered builtins.
 * @param index The 0-based index of the builtin.
 * @return The name of the builtin, or NULL if the index is out of bounds.
 */
const char *flx_sh_get_builtin_name(int index);

#endif /* FLX_SH_BUILTINS_H */
