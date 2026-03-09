#ifndef FELIX_SHELL_BUILTINS_H
#define FELIX_SHELL_BUILTINS_H

#include <stdbool.h>

/**
 * @brief Checks if a given command name is a registered shell builtin.
 */
bool flx_sh_is_builtin(const char *cmd);

/**
 * @brief Executes a shell builtin.
 * @return 0 on success, non-zero on error, or -1 if the builtin was not found.
 */
int flx_sh_execute_builtin(int argc, char **argv);

#endif /* FELIX_SHELL_BUILTINS_H */
