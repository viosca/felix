/**
 * @file felix/posix/wchar.c
 * @brief POSIX Wide Character implementations.
 */

#include <wchar.h>

/**
 * @brief Determine number of column positions for a wide character
 */
int wcwidth(wchar_t wc) {
    /* Null character */
    if (wc == 0) {
        return 0;
    }

    /* Control characters (e.g., ASCII 0-31, DEL, and C1 controls) */
    if (wc < 32 || (wc >= 0x7f && wc < 0xa0)) {
        return 0;
    }

    /* Basic assumption for a lightweight RTOS console:
     * Everything else takes up exactly 1 column.
     * (A full Unicode locale implementation would check for CJK
     * characters here and return 2, but 1 is perfectly safe for Toybox.) */
    return 1;
}
