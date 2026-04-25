/**
 * @file fs/rootfs/bin/flx_fs_bin_cmd_utils.c
 * @brief Collection of general-purpose utility commands.
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "kernel/binfmt.h"
#include "fs/rootfs/bin/flx_fs_bin_cmd_utils.h"

/* =========================================================================
 * CAT COMMAND
 * ========================================================================= */
static void cat_fd(int fd) {
    char buf[128];
    ssize_t bytes_read;

    /* Using your POSIX read/write wrappers! */
    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, bytes_read);
    }
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_cat)
int flx_fs_bin_cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        /* No files specified, read directly from standard input */
        cat_fd(STDIN_FILENO);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        /* POSIX convention: '-' explicitly means stdin */
        if (argv[i][0] == '-' && argv[i][1] == '\0') {
            cat_fd(STDIN_FILENO);
            continue;
        }

        /* Using your POSIX open wrapper! */
        int fd = open(argv[i], O_RDONLY);

        /* Your wrapper handles the kernel return and sets errno,
         * so we can now safely use standard error printing! */
        if (fd == -1) {
            printf("cat: %s: %s\n", argv[i], strerror(errno));
            continue;
        }

        cat_fd(fd);

        /* Using your POSIX close wrapper! */
        close(fd);
    }
    return 0;
}
