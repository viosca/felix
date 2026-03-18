#ifndef FLX_SH_REGISTRY_H
#define FLX_SH_REGISTRY_H

#include "linenoise.h"

/**
 * @brief Function signature for custom autocompletion routines.
 */
typedef void (*flx_sh_complete_cb)(const char *prefix, linenoiseCompletions *lc);

/**
 * @brief Registers a command's UI metadata (Autocompletion and Help) with the Shell.
 * @param name The name of the command (e.g., "ls")
 * @param completer Function pointer to the completer, or NULL if none.
 * @param help_text A short description for the 'help' command.
 * @return 0 on success, -1 if the registry is full.
 */
int flx_sh_registry_add(const char *name, flx_sh_complete_cb completer, const char *help_text);

/**
 * @brief Retrieves the custom completer for a given command.
 */
flx_sh_complete_cb flx_sh_registry_get_completer(const char *name);

/**
 * @brief Retrieves the help text for a given command.
 */
const char* flx_sh_registry_get_help(const char *name);

#endif /* FLX_SH_REGISTRY_H */
