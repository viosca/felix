/**
 * @file felix/posix/utmpx.c
 * @brief POSIX user accounting shims for Felix OS.
 */

#include <utmpx.h>
#include <stddef.h>

/* * Felix OS is a single-user system without a utmpx database.
 * These stubs safely report that no users are currently logged
 * into a terminal session, which satisfies POSIX utilities like 'who'.
 */

void setutxent(void) {
    /* Rewind the database pointer (Do nothing) */
}

void endutxent(void) {
    /* Close the database (Do nothing) */
}

struct utmpx *getutxent(void) {
    /* Return NULL to indicate end of database / no active entries */
    return NULL;
}

struct utmpx *getutxid(const struct utmpx *id) {
    (void)id;
    return NULL;
}

struct utmpx *getutxline(const struct utmpx *line) {
    (void)line;
    return NULL;
}

struct utmpx *pututxline(const struct utmpx *ut) {
    (void)ut;
    return NULL;
}
