#ifndef FLX_SH_COMPLETION_H
#define FLX_SH_COMPLETION_H

#include "linenoise.h"

/**
 * @brief Autocompletion callback for linenoise.
 */
void flx_sh_completion(const char *buf, linenoiseCompletions *lc);

#endif /* FLX_SH_COMPLETION_H */
