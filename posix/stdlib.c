#include <stdlib.h>
#include "kernel/process.h"

/* =========================================================================
 * Standard C Library Environment Overrides (<stdlib.h>)
 * ========================================================================= */

char *getenv(const char *name) {
    return flx_proc_getenv(name);
}

int setenv(const char *name, const char *value, int overwrite) {
    return flx_proc_setenv(name, value, overwrite);
}

int unsetenv(const char *name) {
    return flx_proc_unsetenv(name);
}
