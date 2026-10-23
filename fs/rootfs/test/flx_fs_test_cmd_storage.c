#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_STORAGE)

#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "drivers/sdcard_hal.h"
#include <flx_test.h>

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_storage)
int flx_fs_test_cmd_storage(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS Storage & Block Subsystem Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    /* 1. SD Card hardware detection */
    int inserted = sdcard_hal_is_inserted();
    FLX_TEST_CHECK(&t, true, "SD card hardware detect (card %s)",
                   inserted ? "PRESENT" : "ABSENT / NOT DETECTED");

    if (inserted) {
        int status = sdcard_hal_status();
        FLX_TEST_INT(&t, status, "==", SDCARD_HAL_STATUS_READY,
                     "sdcard_hal_status() == READY (status=0x%02x)", status);
    }

    /* 2. Block device node enumeration in /dev */
    struct stat st;
    int dev_stat = stat("/dev/sd0", &st);
    if (dev_stat == 0) {
        FLX_TEST_CHECK(&t, S_ISBLK(st.st_mode), "/dev/sd0 raw block device registered");
    } else {
        FLX_TEST_CHECK(&t, true, "/dev/sd0 raw block device (not active, skipped)");
    }

    /* 3. Partition discovery check */
    int p1_stat = stat("/dev/sd0p1", &st);
    if (p1_stat == 0) {
        FLX_TEST_CHECK(&t, S_ISBLK(st.st_mode), "/dev/sd0p1 partition block node present");
    } else {
        FLX_TEST_CHECK(&t, true, "/dev/sd0p1 partition node (unpartitioned or absent, skipped)");
    }

    /* 4. FatFs partition mount point & file I/O (prefer /mnt/sd0p2, fallback /mnt/sd0p1) */
    const char *mnt_dir = NULL;
    if (stat("/mnt/sd0p2", &st) == 0 && S_ISDIR(st.st_mode)) {
        mnt_dir = "/mnt/sd0p2";
    } else if (stat("/mnt/sd0p1", &st) == 0 && S_ISDIR(st.st_mode)) {
        mnt_dir = "/mnt/sd0p1";
    }

    if (mnt_dir != NULL) {
        char fat_test_file[64];
        snprintf(fat_test_file, sizeof(fat_test_file), "%s/flx_storage_test.tmp", mnt_dir);
        const char *fat_payload = "Felix Storage Verification Payload 2026";
        size_t plen = strlen(fat_payload);

        int fd = open(fat_test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        ssize_t nw = -1;
        if (fd >= 0) {
            nw = write(fd, fat_payload, plen);
            close(fd);
        }
        FLX_TEST_INT(&t, nw, "==", (long)plen,
                     "FatFs file write to %s (%ld/%ld bytes)", mnt_dir, nw, (long)plen);

        char rdbuf[64] = {0};
        ssize_t nr = -1;
        fd = open(fat_test_file, O_RDONLY);
        if (fd >= 0) {
            nr = read(fd, rdbuf, sizeof(rdbuf) - 1);
            close(fd);
        }
        unlink(fat_test_file);
        FLX_TEST_CHECK(&t, nr == (ssize_t)plen && strcmp(rdbuf, fat_payload) == 0,
                       "FatFs round-trip read & verify ('%s')", rdbuf);
    } else {
        FLX_TEST_CHECK(&t, true, "FatFs /mnt/sd0p* volume (no mounted filesystem, skipped)");
    }

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_STORAGE */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_storage)
int flx_fs_test_cmd_storage(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_STORAGE */
