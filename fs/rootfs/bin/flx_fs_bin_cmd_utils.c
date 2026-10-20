/**
 * @file fs/rootfs/bin/flx_fs_bin_cmd_utils.c
 * @brief Collection of general-purpose utility commands (cat, touch, rm, rmdir, cp).
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "kernel/binfmt.h"
#include "fs/rootfs/bin/flx_fs_bin_cmd_utils.h"

/* =========================================================================
 * CAT COMMAND
 * ========================================================================= */
static void cat_fd(int fd) {
    char buf[128];
    ssize_t bytes_read;

    /* Using standard POSIX read/write wrappers */
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

        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            printf("cat: %s: %s\n", argv[i], strerror(errno));
            continue;
        }

        cat_fd(fd);
        close(fd);
    }
    return 0;
}

/* =========================================================================
 * TOUCH COMMAND
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_touch)
int flx_fs_bin_cmd_touch(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: touch <file>...\n");
        return 1;
    }

    int exit_code = 0;
    for (int i = 1; i < argc; i++) {
        /* Open with O_WRONLY | O_CREAT so existing files are not truncated */
        int fd = open(argv[i], O_WRONLY | O_CREAT, 0666);
        if (fd < 0) {
            printf("touch: cannot touch '%s': %s\n", argv[i], strerror(errno));
            exit_code = 1;
        } else {
            close(fd);
        }
    }
    return exit_code;
}

/* =========================================================================
 * RM COMMAND
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_rm)
int flx_fs_bin_cmd_rm(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rm [-r|-R|-f] <file>...\n");
        return 1;
    }

    bool recursive = false;
    bool force = false;
    int opt_idx = 1;

    while (opt_idx < argc && argv[opt_idx][0] == '-' && argv[opt_idx][1] != '\0') {
        const char *p = &argv[opt_idx][1];
        while (*p) {
            if (*p == 'r' || *p == 'R') recursive = true;
            else if (*p == 'f') force = true;
            else {
                printf("rm: invalid option -- '%c'\n", *p);
                return 1;
            }
            p++;
        }
        opt_idx++;
    }

    if (opt_idx >= argc) {
        if (!force) printf("rm: missing operand\n");
        return force ? 0 : 1;
    }

    int exit_code = 0;
    for (int i = opt_idx; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) < 0) {
            if (!force) {
                printf("rm: cannot remove '%s': %s\n", argv[i], strerror(errno));
                exit_code = 1;
            }
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!recursive) {
                printf("rm: cannot remove '%s': Is a directory\n", argv[i]);
                exit_code = 1;
                continue;
            }
            if (rmdir(argv[i]) < 0) {
                if (!force) {
                    printf("rm: cannot remove directory '%s': %s\n", argv[i], strerror(errno));
                    exit_code = 1;
                }
            }
        } else {
            if (unlink(argv[i]) < 0) {
                if (!force) {
                    printf("rm: cannot remove '%s': %s\n", argv[i], strerror(errno));
                    exit_code = 1;
                }
            }
        }
    }
    return exit_code;
}

/* =========================================================================
 * RMDIR COMMAND
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_rmdir)
int flx_fs_bin_cmd_rmdir(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rmdir <directory>...\n");
        return 1;
    }

    int exit_code = 0;
    for (int i = 1; i < argc; i++) {
        if (rmdir(argv[i]) < 0) {
            printf("rmdir: failed to remove '%s': %s\n", argv[i], strerror(errno));
            exit_code = 1;
        }
    }
    return exit_code;
}

/* =========================================================================
 * CP COMMAND
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_cp)
int flx_fs_bin_cmd_cp(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: cp <source> <destination>\n");
        return 1;
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        printf("cp: cannot open '%s': %s\n", src, strerror(errno));
        return 1;
    }

    struct stat st;
    if (fstat(src_fd, &st) == 0 && S_ISDIR(st.st_mode)) {
        printf("cp: -r not specified; omitting directory '%s'\n", src);
        close(src_fd);
        return 1;
    }

    /* If destination is an existing directory, append source file basename */
    char dst_path[256];
    struct stat dst_st;
    if (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode)) {
        const char *base = strrchr(src, '/');
        base = base ? base + 1 : src;
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, base);
        dst = dst_path;
    }

    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dst_fd < 0) {
        printf("cp: cannot create '%s': %s\n", dst, strerror(errno));
        close(src_fd);
        return 1;
    }

    char buf[512];
    ssize_t bytes_read;
    int exit_code = 0;

    while ((bytes_read = read(src_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < bytes_read) {
            ssize_t res = write(dst_fd, buf + written, bytes_read - written);
            if (res < 0) {
                printf("cp: error writing to '%s': %s\n", dst, strerror(errno));
                exit_code = 1;
                goto cp_done;
            }
            written += res;
        }
    }

    if (bytes_read < 0) {
        printf("cp: error reading from '%s': %s\n", src, strerror(errno));
        exit_code = 1;
    }

cp_done:
    close(src_fd);
    close(dst_fd);
    return exit_code;
}
