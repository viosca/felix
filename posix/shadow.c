/**
 * @file felix/posix/shadow.c
 * @brief Shadow password shims for Felix OS.
 */

#include <shadow.h>
#include <stddef.h>

/**
 * @brief Retrieve a shadow password record.
 * Felix OS doesn't use a shadow database, so we safely return NULL.
 */
struct spwd *getspnam(const char *name) {
    (void)name;
    return NULL;
}
