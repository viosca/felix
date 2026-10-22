#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_MEM)

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <flx_test.h>

#define MEM_TEST_NUM_BLOCKS 16

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_mem)
int flx_fs_test_cmd_mem(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS Heap & Memory Allocator Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    /* 1. malloc and 8-byte 64-bit alignment */
    uint8_t *p1 = (uint8_t *)malloc(128);
    bool p1_ok = (p1 != NULL) && (((uintptr_t)p1 & 0x7UL) == 0);
    if (p1) {
        for (int i = 0; i < 128; i++) {
            p1[i] = (uint8_t)(0xAA ^ i);
        }
    }
    FLX_TEST_CHECK(&t, p1_ok, "malloc(128) 8-byte alignment (ptr=%p)", p1);

    /* 2. calloc zero-initialization */
    uint64_t *p2 = (uint64_t *)calloc(16, sizeof(uint64_t));
    bool p2_zero = (p2 != NULL);
    if (p2) {
        for (int i = 0; i < 16; i++) {
            if (p2[i] != 0ULL) {
                p2_zero = false;
            }
        }
    }
    FLX_TEST_CHECK(&t, p2_zero, "calloc(16, 8) zero-initialization (ptr=%p)", p2);

    /* 3. realloc data retention */
    bool realloc_ok = false;
    if (p1) {
        p1 = (uint8_t *)realloc(p1, 256);
        if (p1) {
            realloc_ok = true;
            for (int i = 0; i < 128; i++) {
                if (p1[i] != (uint8_t)(0xAA ^ i)) {
                    realloc_ok = false;
                    break;
                }
            }
            for (int i = 128; i < 256; i++) {
                p1[i] = 0x55;
            }
        }
    }
    FLX_TEST_CHECK(&t, realloc_ok, "realloc(p1, 256) data retention and expansion");

    /* 4. strdup heap allocation */
    const char *orig_str = "Felix OS Heap Test String";
    char *dup_str = strdup(orig_str);
    bool strdup_ok = (dup_str != NULL && strcmp(dup_str, orig_str) == 0 && dup_str != orig_str);
    FLX_TEST_CHECK(&t, strdup_ok, "strdup() allocation and string copy");

    /* 5. Multi-block fragmentation stress and interleaved free */
    void *blocks[MEM_TEST_NUM_BLOCKS] = {0};
    bool alloc_ok = true;
    for (int i = 0; i < MEM_TEST_NUM_BLOCKS; i++) {
        size_t sz = 32 + (i * 16);
        blocks[i] = malloc(sz);
        if (!blocks[i]) {
            alloc_ok = false;
            break;
        }
        memset(blocks[i], 0xCC, sz);
    }

    if (alloc_ok) {
        /* Free even indices first */
        for (int i = 0; i < MEM_TEST_NUM_BLOCKS; i += 2) {
            free(blocks[i]);
            blocks[i] = NULL;
        }
        /* Re-allocate with varying sizes */
        for (int i = 0; i < MEM_TEST_NUM_BLOCKS; i += 2) {
            blocks[i] = malloc(48);
            if (!blocks[i]) {
                alloc_ok = false;
                break;
            }
        }
    }

    /* Cleanup all */
    for (int i = 0; i < MEM_TEST_NUM_BLOCKS; i++) {
        if (blocks[i]) free(blocks[i]);
    }
    if (p1) free(p1);
    if (p2) free(p2);
    if (dup_str) free(dup_str);

    FLX_TEST_CHECK(&t, alloc_ok, "multi-block allocator stress (16 blocks, interleaved free)");

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_MEM */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_mem)
int flx_fs_test_cmd_mem(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_MEM */
