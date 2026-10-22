#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_IO)

#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <flx_test.h>

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_io)
int flx_fs_test_cmd_io(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS POSIX File I/O & Stream Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    const char *test_file = "/tmp/io_test_append.txt";

    /* 1. O_APPEND semantic guarantee */
    int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
    bool append_ok = false;
    if (fd >= 0) {
        write(fd, "AAAA", 4);
        lseek(fd, 0, SEEK_SET); /* Seek to beginning! */
        write(fd, "BBBB", 4);   /* With O_APPEND, this must append to EOF */
        close(fd);

        char buf[16] = {0};
        fd = open(test_file, O_RDONLY);
        if (fd >= 0) {
            ssize_t nr = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (nr == 8 && strcmp(buf, "AAAABBBB") == 0) {
                append_ok = true;
            }
        }
    }
    FLX_TEST_CHECK(&t, append_ok, "O_APPEND write semantics ('AAAABBBB' despite lseek to 0)");

    /* 2. ftruncate expansion with zero-fill */
    bool expand_ok = false;
    fd = open(test_file, O_RDWR);
    if (fd >= 0) {
        if (ftruncate(fd, 16) == 0) {
            struct stat st;
            if (fstat(fd, &st) == 0 && st.st_size == 16) {
                char zbuf[16] = {0};
                lseek(fd, 8, SEEK_SET);
                ssize_t nr = read(fd, zbuf, 8);
                bool all_zero = true;
                for (int i = 0; i < 8; i++) {
                    if (zbuf[i] != 0) all_zero = false;
                }
                expand_ok = (nr == 8 && all_zero);
            }
        }
        close(fd);
    }
    FLX_TEST_CHECK(&t, expand_ok, "ftruncate() expansion to 16 bytes with zero-fill");

    /* 3. ftruncate shrink */
    bool shrink_ok = false;
    fd = open(test_file, O_RDWR);
    if (fd >= 0) {
        if (ftruncate(fd, 4) == 0) {
            struct stat st;
            char sbuf[8] = {0};
            lseek(fd, 0, SEEK_SET);
            ssize_t nr = read(fd, sbuf, 4);
            if (fstat(fd, &st) == 0 && st.st_size == 4 && nr == 4 && strcmp(sbuf, "AAAA") == 0) {
                shrink_ok = true;
            }
        }
        close(fd);
    }
    FLX_TEST_CHECK(&t, shrink_ok, "ftruncate() shrink to 4 bytes ('AAAA')");

    /* 4. truncate by path */
    int trunc_rc = truncate(test_file, 12);
    struct stat st_trunc;
    int st_rc = stat(test_file, &st_trunc);
    unlink(test_file);
    FLX_TEST_CHECK(&t, trunc_rc == 0 && st_rc == 0 && st_trunc.st_size == 12,
                   "truncate() path-based resize to 12 bytes (rc=%d, size=%ld)",
                   trunc_rc, (long)st_trunc.st_size);

    /* 5. getline() multi-line stream read */
    const char *lines_file = "/tmp/io_lines.txt";
    fd = open(lines_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        const char *content = "First line\nSecond line\nThird line\n";
        write(fd, content, strlen(content));
        close(fd);
    }

    FILE *fp = fopen(lines_file, "r");
    bool getline_ok = false;
    if (fp) {
        char *line = NULL;
        size_t len = 0;
        ssize_t n1 = getline(&line, &len, fp);
        bool l1_ok = (n1 > 0 && strcmp(line, "First line\n") == 0);
        ssize_t n2 = getline(&line, &len, fp);
        bool l2_ok = (n2 > 0 && strcmp(line, "Second line\n") == 0);
        ssize_t n3 = getline(&line, &len, fp);
        bool l3_ok = (n3 > 0 && strcmp(line, "Third line\n") == 0);
        getline_ok = l1_ok && l2_ok && l3_ok;
        if (line) free(line);
        fclose(fp);
    }
    unlink(lines_file);
    FLX_TEST_CHECK(&t, getline_ok, "getline() dynamic buffer multi-line stream parsing");

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_IO */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_io)
int flx_fs_test_cmd_io(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_IO */
