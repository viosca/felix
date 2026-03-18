/**
 * @file wordexp.h
 * @brief POSIX-compliant word expansion for embedded systems.
 */

#ifndef _FLX_WORDEXP_H
#define _FLX_WORDEXP_H

#include <stddef.h>

/* wordexp() flags */
#define WRDE_APPEND   (1 << 0)  /* Append to existing wordexp_t */
#define WRDE_DOOFFS   (1 << 1)  /* Insert we_offs NULLs before we_wordv */
#define WRDE_NOCMD    (1 << 2)  /* Command substitution is an error */
#define WRDE_REUSE    (1 << 3)  /* Reuse wordexp_t (free old contents) */
#define WRDE_SHOWERR  (1 << 4)  /* Print error messages to stderr (ignored in this impl) */
#define WRDE_UNDEF    (1 << 5)  /* Undefined variable is an error */

/* wordexp() return values */
#define WRDE_SUCCESS  0         /* Success */
#define WRDE_BADCHAR  1         /* Unquoted special character */
#define WRDE_BADVAL   2         /* Undefined variable (with WRDE_UNDEF) */
#define WRDE_CMDSUB   3         /* Command substitution attempted */
#define WRDE_NOSPACE  4         /* Out of memory */
#define WRDE_SYNTAX   5         /* Syntax error (e.g., unclosed quotes) */
#define WRDE_UNKNOWN  6

const char const *worderror(unsigned int werrno);

/* The wordexp structure */
typedef struct {
    size_t we_wordc;    /* Count of words matched */
    char **we_wordv;    /* Pointer to list of words */
    size_t we_offs;     /* Slots to reserve at the beginning of we_wordv */
} wordexp_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perform word expansion on a string.
 * * @param s String to expand
 * @param p Pointer to a wordexp_t structure to populate
 * @param flags Bitwise OR of WRDE_* flags
 * @return 0 on success, or a WRDE_* error code.
 */
int wordexp(const char *s, wordexp_t *p, int flags);

/**
 * @brief Perform word expansion without heap allocation (Two-Pass).
 * * Pass NULL to argv_buf and str_buf to measure required capacity.
 * Pass allocated buffers on the second call to fill them.
 *
 * @param s String to expand
 * @param p Pointer to a wordexp_t structure to populate
 * @param flags Bitwise OR of WRDE_* flags
 * @param argv_buf Array of pointers for the arguments (Pass NULL to measure)
 * @param argv_cap Capacity of argv_buf in number of pointers
 * @param str_buf Character buffer for the string data (Pass NULL to measure)
 * @param str_cap Capacity of str_buf in bytes
 * @param req_words Output: Number of words needed (including NULL terminator)
 * @param req_bytes Output: Number of bytes needed for string data
 * @return 0 on success, or a WRDE_* error code.
 */
int wordexp_noalloc(const char *s, wordexp_t *p, int flags,
                    char **argv_buf, size_t argv_cap,
                    char *str_buf, size_t str_cap,
                    size_t *req_words, size_t *req_bytes);

/**
 * @brief Free memory allocated by wordexp().
 * * @param p Pointer to the wordexp_t structure to free
 */
void wordfree(wordexp_t *p);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_WORDEXP_H */
