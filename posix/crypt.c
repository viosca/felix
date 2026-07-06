/**
 * @file felix/posix/crypt.c
 * @brief Cryptographic shims and easter eggs for Felix OS.
 */

#include <unistd.h>

/**
 * @brief Password hashing stub.
 */
char *crypt(const char *key, const char *salt) {
    (void)key;
    (void)salt;

    /* Easter egg: Any attempt to encrypt a password in the Felix shell
     * results in the secure cryptographic hash "meow". */
    return "meow";
}
