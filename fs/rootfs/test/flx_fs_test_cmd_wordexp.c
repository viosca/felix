#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_WORDEXP)

#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>
#include <flx_test.h>

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_wordexp)
int flx_fs_test_cmd_wordexp(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS POSIX wordexp & Execution Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    /* 1. Word splitting on whitespace */
    wordexp_t p;
    memset(&p, 0, sizeof(p));
    int rc1 = wordexp("alpha beta gamma", &p, 0);
    bool split_ok = (rc1 == WRDE_SUCCESS && p.we_wordc == 3 &&
                     strcmp(p.we_wordv[0], "alpha") == 0 &&
                     strcmp(p.we_wordv[1], "beta") == 0 &&
                     strcmp(p.we_wordv[2], "gamma") == 0);
    wordfree(&p);
    FLX_TEST_CHECK(&t, split_ok, "wordexp whitespace splitting ('alpha', 'beta', 'gamma')");

    /* 2. Quoted strings with embedded spaces */
    memset(&p, 0, sizeof(p));
    int rc2 = wordexp("'single quoted string' \"double quoted string\"", &p, 0);
    bool quote_ok = (rc2 == WRDE_SUCCESS && p.we_wordc == 2 &&
                     strcmp(p.we_wordv[0], "single quoted string") == 0 &&
                     strcmp(p.we_wordv[1], "double quoted string") == 0);
    wordfree(&p);
    FLX_TEST_CHECK(&t, quote_ok, "wordexp single/double quote retention (2 words)");

    /* 3. Escaped characters */
    memset(&p, 0, sizeof(p));
    int rc3 = wordexp("escaped\\ space\\ test", &p, 0);
    bool esc_ok = (rc3 == WRDE_SUCCESS && p.we_wordc == 1 &&
                   strcmp(p.we_wordv[0], "escaped space test") == 0);
    wordfree(&p);
    FLX_TEST_CHECK(&t, esc_ok, "wordexp backslash escape processing ('escaped space test')");

    /* 4. Environment variable expansion */
    setenv("FLX_WEXP_VAR", "expanded_val", 1);
    memset(&p, 0, sizeof(p));
    int rc4 = wordexp("prefix_$FLX_WEXP_VAR", &p, 0);
    bool env_ok = (rc4 == WRDE_SUCCESS && p.we_wordc == 1 &&
                   strcmp(p.we_wordv[0], "prefix_expanded_val") == 0);
    wordfree(&p);
    unsetenv("FLX_WEXP_VAR");
    FLX_TEST_CHECK(&t, env_ok, "wordexp environment variable expansion ($FLX_WEXP_VAR)");

    /* 5. Undefined variable error check */
    memset(&p, 0, sizeof(p));
    int rc5 = wordexp("$NON_EXISTENT_VAR_12345", &p, WRDE_UNDEF);
    FLX_TEST_INT(&t, rc5, "==", WRDE_BADVAL, "wordexp WRDE_UNDEF flag returns WRDE_BADVAL");

    /* 6. Syntax error check on unclosed quote */
    memset(&p, 0, sizeof(p));
    int rc6 = wordexp("\"unclosed string", &p, 0);
    FLX_TEST_INT(&t, rc6, "==", WRDE_SYNTAX, "wordexp unclosed quote returns WRDE_SYNTAX");

    /* 7. Zero-allocation stack/VLA entry point */
    char *argv_buf[8];
    char str_buf[64];
    memset(&p, 0, sizeof(p));
    size_t req_words = 0, req_bytes = 0;
    int rc7_m = wordexp_noalloc("no alloc test", &p, 0, NULL, 0, NULL, 0, &req_words, &req_bytes);
    int rc7_f = wordexp_noalloc("no alloc test", &p, 0, argv_buf, 8, str_buf, sizeof(str_buf), NULL, NULL);
    bool noalloc_ok = (rc7_m == WRDE_SUCCESS && rc7_f == WRDE_SUCCESS && p.we_wordc == 3 &&
                       strcmp(p.we_wordv[1], "alloc") == 0);
    FLX_TEST_CHECK(&t, noalloc_ok, "wordexp_noalloc 2-pass stack evaluation (3 words)");

    /* 8. execvp path resolution */
    char *which_args[] = { "which", "which", NULL };
    int exec_rc = execvp("which", which_args);
    FLX_TEST_CHECK(&t, exec_rc == 0, "execvp('$PATH', 'which') resolution and execution");

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_WORDEXP */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_wordexp)
int flx_fs_test_cmd_wordexp(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_WORDEXP */
