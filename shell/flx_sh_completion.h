#ifndef FLX_SH_COMPLETION_H
#define FLX_SH_COMPLETION_H

#include "linenoise.h"

/**
 * @brief Function signature for custom autocompletion routines.
 */
typedef void (*flx_sh_complete_cb)(const char *prefix, linenoiseCompletions *lc);

/**
 * @brief Autocompletion callback for linenoise.
 */
void flx_sh_completion(const char *buf, linenoiseCompletions *lc);

#endif /* FLX_SH_COMPLETION_H */