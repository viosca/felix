#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "kernel/klog.h"
#include "kernel/binfmt.h"

int flx_binfmt_load(void *file_data, int argc, char **argv) {
    if (!file_data) return -1;

    uint8_t *bytes = (uint8_t *)file_data;

    /* Verify Magic Number */
    if (bytes[0] != FLX_MAGIC_0 || bytes[1] != FLX_MAGIC_1 ||
        bytes[2] != FLX_MAGIC_2 || bytes[3] != FLX_MAGIC_3) {

        /* In the future, we can check for '#!' here for shell scripts */
        kprintf("exec: unrecognized executable format\n");
        return -1;
    }

    /* Dispatch based on Payload Type */
    if (bytes[4] == FLX_BINFMT_TYPE_LINKED) {

        /* Internal OS Function - Just cast the absolute pointer and jump! */
        FlxLinkedExec_t *exec = (FlxLinkedExec_t *)file_data;
        if (exec->main_func) {
            return exec->main_func(argc, argv);
        }

    } else if (bytes[4] == FLX_BINFMT_TYPE_EXTERNAL) {

        /* External App - Calculate absolute address from the RAM offset */
        FlxExternalExec_t *exec = (FlxExternalExec_t *)file_data;
        uintptr_t base_address = (uintptr_t)file_data;
        uintptr_t entry_address = base_address + exec->entry_offset;

        int (*app_main)(int, char**) = (int (*)(int, char**))entry_address;
        if (app_main) {
            return app_main(argc, argv);
        }
    }

    kprintf("exec: invalid payload\n");
    return -1;
}
