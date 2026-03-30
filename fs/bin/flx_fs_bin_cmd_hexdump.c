#if 0
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>   /* For POSIX open flags (O_RDONLY) */
#include <unistd.h>  /* For POSIX open, read, close */
#include "kernel/binfmt.h"

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_hexdump)
int flx_fs_bin_cmd_hexdump(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: hexdump <file>\n");
        return 1;
    }

    /* Open the target file using raw POSIX syscalls instead of fopen */
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("hexdump: %s: No such file or directory\n", argv[1]);
        return 1;
    }

    unsigned char buf[16];
    ssize_t bytes_read;
    size_t offset = 0;

    /* Read the file in 16-byte chunks until EOF */
    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        /* 1. Print the offset */
        printf("%08zx  ", offset);

        /* 2. Print the hex bytes */
        for (ssize_t i = 0; i < 16; i++) {
            if (i == 8) {
                printf(" "); /* Extra space in the middle for readability */
            }

            if (i < bytes_read) {
                printf("%02x ", buf[i]);
            } else {
                printf("   "); /* Pad missing bytes on the last line */
            }
        }

        printf(" |");
        /* 3. Print the ASCII characters */
        for (ssize_t i = 0; i < bytes_read; i++) {
            /* Check if the byte is a printable ASCII character */
            if (buf[i] >= 32 && buf[i] <= 126) {
                printf("%c", buf[i]);
            } else {
                printf("."); /* Non-printable characters become dots */
            }
        }

        printf("|\n");
        offset += bytes_read;
    }

    /* Check if the loop exited due to a read error */
    if (bytes_read < 0) {
        printf("hexdump: error reading file\n");
    }

    close(fd);
    return 0;
}
#else
#include <stdio.h>
#include <stddef.h>
#include "kernel/binfmt.h"

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_hexdump)
int flx_fs_bin_cmd_hexdump(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: hexdump <file>\n");
        return 1;
    }

    /* Open the target file using standard C library streams */
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("hexdump: %s: No such file or directory\n", argv[1]);
        return 1;
    }

    unsigned char buf[16];
    size_t bytes_read;
    size_t offset = 0;

    /* Read the file in 16-byte chunks until EOF */
    while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0) {
        /* 1. Print the offset */
        printf("%08zx  ", offset);

        /* 2. Print the hex bytes */
        for (size_t i = 0; i < 16; i++) {
            if (i == 8) {
                printf(" "); /* Extra space in the middle for readability */
            }

            if (i < bytes_read) {
                printf("%02x ", buf[i]);
            } else {
                printf("   "); /* Pad missing bytes on the last line */
            }
        }

        printf(" |");

        /* 3. Print the ASCII characters */
        for (size_t i = 0; i < bytes_read; i++) {
            /* Check if the byte is a printable ASCII character */
            if (buf[i] >= 32 && buf[i] <= 126) {
                printf("%c", buf[i]);
            } else {
                printf("."); /* Non-printable characters become dots */
            }
        }

        printf("|\n");
        offset += bytes_read;
    }

    /* Check if the loop exited due to a read error */
    if (ferror(fp)) {
        printf("hexdump: error reading file\n");
    }

    fclose(fp);
    return 0;
}
#endif
