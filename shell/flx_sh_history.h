#ifndef FELIX_SHELL_HISTORY_H
#define FELIX_SHELL_HISTORY_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Parses a raw input line and performs bang (!) history substitution.
 * @param input The raw input line typed by the user.
 * @param output The buffer to store the resulting/expanded command.
 * @param out_len The maximum size of the output buffer.
 * @return true if the command is valid and should be executed, false if an event was not found.
 */
bool flx_sh_history_expand(const char *input, char *output, size_t out_len);

#endif /* FELIX_SHELL_HISTORY_H */
