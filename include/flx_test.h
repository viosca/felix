/**
 * @file felix/include/flx_test.h
 * @brief Header-only lightweight unit test harness for Felix OS (/test commands).
 *
 * Provides a self-contained test context struct (FlxTestCtx_t), static inline
 * relational comparison functions ("==", "!=", "<", "<=", ">", ">="), and
 * optional call-site location macros (FLX_TEST_*) that fold __FILE__, __func__,
 * and __LINE__ into FlxTestCtx_t and clear them once consumed.
 *
 * Simple tests may call the inline functions directly (or even pass NULL for ctx)
 * without using macros.
 */

#ifndef FELIX_TEST_H
#define FELIX_TEST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "optparse.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Common optparse option string shared by all /test commands:
 *   -v : Verbose (show [file:func:line] on PASS as well as FAIL)
 *   -F : Full __FILE__ path instead of basename
 *   -q : Quiet location (hide [file:func:line] even on FAIL)
 *   -h : Print usage help and exit
 */
#define FLX_TEST_OPTSTR "vFqh"

/**
 * @brief Formatting and verbosity flags stored in FlxTestCtx_t::flags.
 */
typedef enum {
    FLX_TEST_FLAG_NONE        = 0,
    FLX_TEST_FLAG_SHOW_FILE   = (1u << 0), /**< Print __FILE__ when location is present */
    FLX_TEST_FLAG_SHOW_FUNC   = (1u << 1), /**< Print __func__ when location is present */
    FLX_TEST_FLAG_SHOW_LINE   = (1u << 2), /**< Print __LINE__ when location is present */
    FLX_TEST_FLAG_FULL_PATH   = (1u << 3), /**< Print full __FILE__ path instead of basename */
    FLX_TEST_FLAG_LOC_ON_PASS = (1u << 4), /**< Print location on PASS as well as FAIL */

    /** Default: show basename(file):func:line on FAIL only */
    FLX_TEST_FLAG_DEFAULT     = (FLX_TEST_FLAG_SHOW_FILE |
                                 FLX_TEST_FLAG_SHOW_FUNC |
                                 FLX_TEST_FLAG_SHOW_LINE),

    /** Verbose: show basename(file):func:line on both PASS and FAIL */
    FLX_TEST_FLAG_VERBOSE     = (FLX_TEST_FLAG_DEFAULT | FLX_TEST_FLAG_LOC_ON_PASS)
} FlxTestFlags_t;

/**
 * @brief Tracks cumulative test suite state and transient call-site location.
 */
typedef struct {
    const char *suite_name; /**< Name of the active test suite */
    uint32_t flags;         /**< Bitmask of FlxTestFlags_t */
    int total;              /**< Total checks executed */
    int passed;             /**< Number of checks that passed */
    int failed;             /**< Number of checks that failed */

    /* Transient call-site metadata (populated by flx_test_set_loc, cleared by flx_test_clr_loc) */
    const char *file;       /**< Source filename (__FILE__) or NULL */
    const char *func;       /**< Function name (__func__) or NULL */
    int line;               /**< Source line number (__LINE__) or 0 */
} FlxTestCtx_t;

/**
 * @brief Clears transient call-site location fields in the test context.
 */
static inline void flx_test_clr_loc(FlxTestCtx_t *ctx) {
    if (ctx) {
        ctx->file = NULL;
        ctx->func = NULL;
        ctx->line = 0;
    }
}

/**
 * @brief Latches call-site location metadata into the test context and returns ctx.
 */
static inline FlxTestCtx_t *flx_test_set_loc(FlxTestCtx_t *ctx,
                                             const char *file,
                                             const char *func,
                                             int line) {
    if (ctx) {
        ctx->file = file;
        ctx->func = func;
        ctx->line = line;
    }
    return ctx;
}

/**
 * @brief Initializes a test context with default flags (without printing the banner).
 * @param ctx        Optional pointer to test context (may be NULL).
 * @param suite_name Title of the test suite.
 */
static inline void flx_test_init(FlxTestCtx_t *ctx, const char *suite_name) {
    if (ctx) {
        memset(ctx, 0, sizeof(*ctx));
        ctx->suite_name = suite_name;
        ctx->flags = FLX_TEST_FLAG_DEFAULT;
    }
}

/**
 * @brief Prints the opening banner for a test suite.
 */
static inline void flx_test_banner(const FlxTestCtx_t *ctx) {
    const char *name = (ctx && ctx->suite_name) ? ctx->suite_name : "Felix Test Suite";
    printf("=== %s ===\n", name);
}

/**
 * @brief Initializes a test context with default flags and prints the suite banner.
 * @param ctx        Optional pointer to test context (may be NULL).
 * @param suite_name Title printed in the opening banner.
 */
static inline void flx_test_begin(FlxTestCtx_t *ctx, const char *suite_name) {
    flx_test_init(ctx, suite_name);
    flx_test_banner(ctx);
}

/**
 * @brief Prints standard command-line usage for a test executable.
 * @param prog        Program name (typically argv[0]).
 * @param extra_usage Optional string describing test-specific flags/args (or NULL).
 */
static inline void flx_test_usage(const char *prog, const char *extra_usage) {
    const char *name = prog ? prog : "test";
    if (extra_usage && extra_usage[0] != '\0') {
        printf("Usage: %s [-vFqh] %s\n", name, extra_usage);
    } else {
        printf("Usage: %s [-vFqh]\n", name);
    }
    printf("Common test options:\n");
    printf("  -v  Show [file:func:line] on PASS as well as FAIL\n");
    printf("  -F  Show full __FILE__ path instead of basename\n");
    printf("  -q  Suppress [file:func:line] location output\n");
    printf("  -h  Show this help message\n");
}

/**
 * @brief Handles a single common option parsed by optparse() using FLX_TEST_OPTSTR.
 *        Tests with custom options can call this from their optparse switch.
 * @param ctx         Pointer to test context.
 * @param opt         Option character returned by optparse().
 * @param prog        Program name (argv[0]).
 * @param extra_usage Optional description of test-specific options for -h output.
 * @return  1 if the option was a common test flag and was handled,
 *          0 if the option belongs to the caller's custom options,
 *         -1 if '-h' was handled (help printed; caller should exit with 0).
 */
static inline int flx_test_handle_opt(FlxTestCtx_t *ctx,
                                      int opt,
                                      const char *prog,
                                      const char *extra_usage) {
    switch (opt) {
        case 'v':
            if (ctx) ctx->flags |= FLX_TEST_FLAG_LOC_ON_PASS;
            return 1;
        case 'F':
            if (ctx) ctx->flags |= FLX_TEST_FLAG_FULL_PATH;
            return 1;
        case 'q':
            if (ctx) {
                ctx->flags &= ~(FLX_TEST_FLAG_SHOW_FILE |
                                FLX_TEST_FLAG_SHOW_FUNC |
                                FLX_TEST_FLAG_SHOW_LINE |
                                FLX_TEST_FLAG_LOC_ON_PASS);
            }
            return 1;
        case 'h':
            flx_test_usage(prog, extra_usage);
            return -1;
        default:
            return 0;
    }
}

/**
 * @brief Parses standard test command-line options (-v, -F, -q, -h) via optparse.
 * @param ctx  Pointer to initialized test context.
 * @param argc Argument count passed to the test command.
 * @param argv Argument vector passed to the test command.
 * @return  0 if parsing succeeded and tests should proceed,
 *          1 if '-h' was requested (caller should return 0),
 *         -1 if an invalid option was encountered (caller should return 1).
 */
static inline int flx_test_parse_args(FlxTestCtx_t *ctx, int argc, char **argv) {
    if (argc <= 1 || !argv) {
        return 0;
    }

    struct optparse options;
    optparse_init(&options, argv);

    int opt;
    while ((opt = optparse(&options, FLX_TEST_OPTSTR)) != -1) {
        if (opt == '?') {
            printf("%s: %s -- '%c'\n",
                   argv[0] ? argv[0] : "test",
                   options.errmsg,
                   options.optopt);
            flx_test_usage(argv[0], NULL);
            return -1;
        }
        int rc = flx_test_handle_opt(ctx, opt, argv[0], NULL);
        if (rc < 0) {
            return 1; /* Help requested */
        }
    }
    return 0;
}

/**
 * @brief Initializes ctx, parses standard CLI args (-vFqh), and prints the banner
 *        only if the test suite should proceed.
 * @return  0 to proceed with tests,
 *          1 if '-h' help was printed (caller should return 0),
 *         -1 if an invalid option was given (caller should return 1).
 */
static inline int flx_test_begin_args(FlxTestCtx_t *ctx,
                                      const char *suite_name,
                                      int argc,
                                      char **argv) {
    flx_test_init(ctx, suite_name);
    int rc = flx_test_parse_args(ctx, argc, argv);
    if (rc == 0) {
        flx_test_banner(ctx);
    }
    return rc;
}

/**
 * @brief Prints the final test summary and returns a shell-compatible exit code.
 * @param ctx Optional pointer to test context (may be NULL).
 * @return 0 if all tests passed (or ctx is NULL), 1 if any test failed.
 */
static inline int flx_test_end(const FlxTestCtx_t *ctx) {
    if (!ctx) {
        printf("=== Result: DONE ===\n");
        return 0;
    }
    printf("=== Result: %s (%d/%d passed, %d failures) ===\n",
           (ctx->failed == 0) ? "ALL PASSED" : "FAILED",
           ctx->passed, ctx->total, ctx->failed);
    return (ctx->failed == 0) ? 0 : 1;
}

/**
 * @brief Returns the basename portion of a file path (after the last '/' or '\\').
 */
static inline const char *flx_test_basename(const char *path) {
    if (!path) return NULL;
    const char *base = path;
    for (const char *p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

/**
 * @brief Consumes and conditionally prints location info from ctx, then clears it.
 *        Prints nothing if both ctx->file and ctx->func are NULL.
 */
static inline void flx_test_consume_loc(FlxTestCtx_t *ctx, bool passed) {
    if (!ctx) return;

    if (ctx->file != NULL || ctx->func != NULL) {
        bool show = (!passed) || ((ctx->flags & FLX_TEST_FLAG_LOC_ON_PASS) != 0);
        uint32_t loc_mask = ctx->flags & (FLX_TEST_FLAG_SHOW_FILE |
                                          FLX_TEST_FLAG_SHOW_FUNC |
                                          FLX_TEST_FLAG_SHOW_LINE);
        if (show && loc_mask != 0) {
            bool need_sep = false;
            printf(" [");
            if ((ctx->flags & FLX_TEST_FLAG_SHOW_FILE) && ctx->file) {
                const char *f = (ctx->flags & FLX_TEST_FLAG_FULL_PATH)
                                ? ctx->file
                                : flx_test_basename(ctx->file);
                printf("%s", f);
                need_sep = true;
            }
            if ((ctx->flags & FLX_TEST_FLAG_SHOW_FUNC) && ctx->func) {
                printf("%s%s", need_sep ? ":" : "", ctx->func);
                need_sep = true;
            }
            if ((ctx->flags & FLX_TEST_FLAG_SHOW_LINE) && ctx->line > 0) {
                printf("%s%d", need_sep ? ":" : "", ctx->line);
            }
            printf("]");
        }
    }

    flx_test_clr_loc(ctx);
}

/**
 * @brief Internal helper to record a check outcome and print the prefix + message.
 */
static inline bool flx_test_record_v(FlxTestCtx_t *ctx, bool passed, const char *fmt, va_list ap) {
    int idx = 0;
    if (ctx) {
        ctx->total++;
        idx = ctx->total;
        if (passed) {
            ctx->passed++;
        } else {
            ctx->failed++;
        }
    }

    if (idx > 0) {
        printf("[%s] %d. ", passed ? "PASS" : "FAIL", idx);
    } else {
        printf("[%s] ", passed ? "PASS" : "FAIL");
    }

    if (fmt) {
        vprintf(fmt, ap);
    }
    return passed;
}

/**
 * @brief Evaluates a relational operator string ("==", "!=", "<", "<=", ">", ">=")
 *        on two signed integer values.
 */
static inline bool flx_test_eval_int(long a, const char *op, long b) {
    if (!op) return false;
    if (op[0] == '=' && op[1] == '=' && op[2] == '\0') return a == b;
    if (op[0] == '!' && op[1] == '=' && op[2] == '\0') return a != b;
    if (op[0] == '<' && op[1] == '=' && op[2] == '\0') return a <= b;
    if (op[0] == '>' && op[1] == '=' && op[2] == '\0') return a >= b;
    if (op[0] == '<' && op[1] == '\0')                 return a <  b;
    if (op[0] == '>' && op[1] == '\0')                 return a >  b;
    return false;
}

/**
 * @brief Evaluates a relational operator string ("==", "!=", "<", "<=", ">", ">=")
 *        on two unsigned / pointer-sized values.
 */
static inline bool flx_test_eval_uint(uintptr_t a, const char *op, uintptr_t b) {
    if (!op) return false;
    if (op[0] == '=' && op[1] == '=' && op[2] == '\0') return a == b;
    if (op[0] == '!' && op[1] == '=' && op[2] == '\0') return a != b;
    if (op[0] == '<' && op[1] == '=' && op[2] == '\0') return a <= b;
    if (op[0] == '>' && op[1] == '=' && op[2] == '\0') return a >= b;
    if (op[0] == '<' && op[1] == '\0')                 return a <  b;
    if (op[0] == '>' && op[1] == '\0')                 return a >  b;
    return false;
}

/**
 * @brief Evaluates a boolean condition and prints the formatted test line.
 */
__attribute__((format(printf, 3, 4)))
static inline bool flx_test_check(FlxTestCtx_t *ctx, bool cond, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    flx_test_record_v(ctx, cond, fmt, ap);
    va_end(ap);
    flx_test_consume_loc(ctx, cond);
    printf("\n");
    return cond;
}

/**
 * @brief Compares two signed integers using op ("==", "!=", "<", "<=", ">", ">=").
 */
__attribute__((format(printf, 5, 6)))
static inline bool flx_test_cmp_int(FlxTestCtx_t *ctx,
                                    long actual,
                                    const char *op,
                                    long expected,
                                    const char *fmt, ...) {
    bool cond = flx_test_eval_int(actual, op, expected);
    va_list ap;
    va_start(ap, fmt);
    flx_test_record_v(ctx, cond, fmt, ap);
    va_end(ap);
    if (!cond) {
        printf(" (failed: %ld %s %ld)", actual, op ? op : "?", expected);
    }
    flx_test_consume_loc(ctx, cond);
    printf("\n");
    return cond;
}

/**
 * @brief Compares two unsigned / uintptr_t values using op ("==", "!=", "<", "<=", ">", ">=").
 */
__attribute__((format(printf, 5, 6)))
static inline bool flx_test_cmp_uint(FlxTestCtx_t *ctx,
                                     uintptr_t actual,
                                     const char *op,
                                     uintptr_t expected,
                                     const char *fmt, ...) {
    bool cond = flx_test_eval_uint(actual, op, expected);
    va_list ap;
    va_start(ap, fmt);
    flx_test_record_v(ctx, cond, fmt, ap);
    va_end(ap);
    if (!cond) {
        printf(" (failed: 0x%lx %s 0x%lx)",
               (unsigned long)actual, op ? op : "?", (unsigned long)expected);
    }
    flx_test_consume_loc(ctx, cond);
    printf("\n");
    return cond;
}

/**
 * @brief Compares two pointers using op ("==", "!=", "<", "<=", ">", ">=").
 */
__attribute__((format(printf, 5, 6)))
static inline bool flx_test_cmp_ptr(FlxTestCtx_t *ctx,
                                    const void *actual,
                                    const char *op,
                                    const void *expected,
                                    const char *fmt, ...) {
    bool cond = flx_test_eval_uint((uintptr_t)actual, op, (uintptr_t)expected);
    va_list ap;
    va_start(ap, fmt);
    flx_test_record_v(ctx, cond, fmt, ap);
    va_end(ap);
    if (!cond) {
        printf(" (failed: %p %s %p)", actual, op ? op : "?", expected);
    }
    flx_test_consume_loc(ctx, cond);
    printf("\n");
    return cond;
}

/**
 * @brief Compares two C strings (NULL-safe) using op ("==", "!=", "<", "<=", ">", ">=").
 */
__attribute__((format(printf, 5, 6)))
static inline bool flx_test_cmp_str(FlxTestCtx_t *ctx,
                                    const char *actual,
                                    const char *op,
                                    const char *expected,
                                    const char *fmt, ...) {
    bool cond = false;
    if (actual == NULL || expected == NULL) {
        int eq = (actual == expected) ? 0 : (actual ? 1 : -1);
        cond = flx_test_eval_int(eq, op, 0);
    } else {
        int cmp = strcmp(actual, expected);
        cond = flx_test_eval_int(cmp, op, 0);
    }
    va_list ap;
    va_start(ap, fmt);
    flx_test_record_v(ctx, cond, fmt, ap);
    va_end(ap);
    if (!cond) {
        printf(" (failed: \"%s\" %s \"%s\")",
               actual ? actual : "(null)",
               op ? op : "?",
               expected ? expected : "(null)");
    }
    flx_test_consume_loc(ctx, cond);
    printf("\n");
    return cond;
}

/* =========================================================================
 * Call-Site Location Macros (Optional)
 * =========================================================================
 * Populate ctx->file, ctx->func, and ctx->line prior to invoking the inline
 * comparison function. Once printed (or skipped), flx_test_consume_loc()
 * automatically resets them via flx_test_clr_loc().
 */

#define FLX_TEST_CHECK(ctx, cond, ...) \
    flx_test_check(flx_test_set_loc((ctx), __FILE__, __func__, __LINE__), (cond), __VA_ARGS__)

#define FLX_TEST_INT(ctx, actual, op, expected, ...) \
    flx_test_cmp_int(flx_test_set_loc((ctx), __FILE__, __func__, __LINE__), (actual), (op), (expected), __VA_ARGS__)

#define FLX_TEST_UINT(ctx, actual, op, expected, ...) \
    flx_test_cmp_uint(flx_test_set_loc((ctx), __FILE__, __func__, __LINE__), (actual), (op), (expected), __VA_ARGS__)

#define FLX_TEST_PTR(ctx, actual, op, expected, ...) \
    flx_test_cmp_ptr(flx_test_set_loc((ctx), __FILE__, __func__, __LINE__), (actual), (op), (expected), __VA_ARGS__)

#define FLX_TEST_STR(ctx, actual, op, expected, ...) \
    flx_test_cmp_str(flx_test_set_loc((ctx), __FILE__, __func__, __LINE__), (actual), (op), (expected), __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* FELIX_TEST_H */
