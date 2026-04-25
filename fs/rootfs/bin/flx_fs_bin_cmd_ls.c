#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>
#include <shell/flx_sh.h>
#include <fs/rootfs/bin/flx_fs_bin_cmd_ls.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include "optparse.h"
#include "limits.h"
#include "kernel/binfmt.h"

#define MAX_LS_ENTRIES 256

/* =========================================================================
 * Shared Utility Functions
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_ls)
static int compare_names(const void *a, const void *b) {
    const char *str_a = *(const char **)a;
    const char *str_b = *(const char **)b;
    return strcmp(str_a, str_b);
}


FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_ls)
static void print_permissions(mode_t mode, char *str, size_t strlen) {
    strncpy(str, "---------", strlen);
    if (strlen > 0 && mode & S_IRUSR) str[0] = 'r';
    if (strlen > 1 && mode & S_IWUSR) str[1] = 'w';
    if (strlen > 2 && mode & S_IXUSR) str[2] = 'x';
    if (strlen > 3 && mode & S_IRGRP) str[3] = 'r';
    if (strlen > 4 && mode & S_IWGRP) str[4] = 'w';
    if (strlen > 5 && mode & S_IXGRP) str[5] = 'x';
    if (strlen > 6 && mode & S_IROTH) str[6] = 'r';
    if (strlen > 7 && mode & S_IWOTH) str[7] = 'w';
    if (strlen > 8 && mode & S_IXOTH) str[8] = 'x';
    str[strlen] = '\0';
}

/* =========================================================================
 * Command: ls
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_ls)
int flx_fs_bin_cmd_ls(int argc, char **argv) {
    struct optparse options;
    optparse_init(&options, argv);

    bool long_format = false;
    bool show_all = false;

    char *prog_name = strrchr(argv[0], '/');
    prog_name = prog_name ? prog_name + 1 : argv[0]; /* Get the basename */

    /* BusyBox alias support: if command was invoked as 'll', force long format */
    if (strcmp(prog_name, "ll") == 0) {
        long_format = true;
    }

    int option;
    while ((option = optparse(&options, "la")) != -1) {
        switch (option) {
            case 'l': long_format = true; break;
            case 'a': show_all = true; break;
            case '?':
                FLX_SH_ERR("%s: invalid option -- '%c'\n", argv[0], options.optopt);
                return 1;
        }
    }

    const char *target = optparse_arg(&options);
    if (!target) target = ".";

    DIR *dir = opendir(target);
    if (!dir) {
        struct stat st;
        if (stat(target, &st) == 0 && !S_ISDIR(st.st_mode)) {
            printf("%s\n", target);
            return 0;
        }
        FLX_SH_ERR("ls: cannot access '%s': No such file or directory\n", target);
        return 1;
    }

    /* 1. Allocate buffer for sorting */
    char **names = malloc(MAX_LS_ENTRIES * sizeof(char*));
    if (!names) {
        FLX_SH_ERR("ls: out of memory\n");
        closedir(dir);
        return 1;
    }

    /* 2. Read all directory entries into RAM */
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') continue;

        if (count < MAX_LS_ENTRIES) {
            names[count++] = strdup(entry->d_name);
        } else {
            FLX_SH_ERR("ls: warning: too many files, output truncated\n");
            break;
        }
    }
    closedir(dir);

    /* 3. Sort the names alphabetically! */
    qsort(names, count, sizeof(char*), compare_names);

    /* 4. Print the sorted results */
    for (int i = 0; i < count; i++) {
        if (long_format) {
            struct stat st;
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", target, names[i]);

            const char *user = "kitty";
            //char perms[10] = {0}; // 9 chars + null terminator
            char perms[4] = {0}; // 3 chars + null terminator (felix is not multi user).
            strncpy(perms, "---------", sizeof(perms)-1);
            perms[sizeof(perms) - 1] = '\0';

            if (stat(full_path, &st) == 0) {
                char type = '-';
                if (S_ISDIR(st.st_mode)) type = 'd';
                else if (S_ISCHR(st.st_mode)) type = 'c';
                else if (S_ISBLK(st.st_mode)) type = 'b';

                print_permissions(st.st_mode, perms, sizeof(perms)-1);

                /* If it's a device, print Major, Minor instead of Size! */
                if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
                    int major = (st.st_rdev >> 8) & 0xFF;
                    int minor = st.st_rdev & 0xFF;
                    printf("%c%s 1 %s %3d, %3d 00:00 %s\n",
                        type, perms, user, major, minor, names[i]);
                } else {
                    printf("%c%s 1 %s %8u 00:00 %s\n",
                        type, perms, user, (unsigned int)st.st_size, names[i]);
                }
            } else {
                /* Stat failed, just print the name */
                printf("?%s 1 %s        0 00:00 %s\n", perms, user, names[i]);
            }
        } else {
            /* Standard short format */
            printf("%s  ", names[i]);
        }

        /* 5. Free the duplicated string now that we are done with it */
        free(names[i]);
    }

    if (!long_format && count > 0) {
        printf("\n");
    }

    /* 6. Free the main array */
    free(names);

    return 0;
}


//This keeps everything explicit, flexible, and completely prevents the macro from making dangerous assumptions about the layout of your C files!

/* =========================================================================
 * Command: tree
 * ========================================================================= */

/**
 * @brief Recursively prints the directory tree.
 * @param last_mask A bitmask tracking which parent branches have terminated,
 * allowing us to correctly draw '    ' vs '|   ' down the tree.
 * @param use_vt Boolean to enable UTF-8 VT box drawing symbols instead of ASCII.
 */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_tree)
static void print_tree_recursive(const char *base_path, int depth, uint32_t last_mask, bool use_vt) {
    if (depth > 31) return; /* Prevent bitmask overflow and excessive stack usage */

    DIR *dir = opendir(base_path);
    if (!dir) return;

    /* 1. Allocate buffer for sorting */
    char **names = malloc(MAX_LS_ENTRIES * sizeof(char*));
    if (!names) {
        closedir(dir);
        return;
    }

    /* 2. Read entries */
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        if (count < MAX_LS_ENTRIES) {
            names[count++] = strdup(entry->d_name);
        } else {
            break;
        }
    }
    closedir(dir);

    /* 3. Sort entries (reusing the ls comparison function!) */
    qsort(names, count, sizeof(char*), compare_names);

    /* UTF-8 Box Drawing characters vs Standard ASCII */
    const char *str_vert   = use_vt ? "\xE2\x94\x82   " : "|   ";
    const char *str_branch = use_vt ? "\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80 " : "|-- ";
    const char *str_last   = use_vt ? "\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80 " : "`-- ";

    /* 4. Print formatted branches and recurse */
    for (int i = 0; i < count; i++) {
        bool is_last = (i == count - 1);

        /* Print the vertical branch lines for previous depths */
        for (int j = 0; j < depth; j++) {
            if (last_mask & (1 << j)) {
                printf("    ");  /* Parent folder finished, leave empty space */
            } else {
                printf("%s", str_vert);  /* Parent folder still has more items, drop a line */
            }
        }

        /* Print the item connector for the current depth */
        if (is_last) {
            printf("%s%s\n", str_last, names[i]);
        } else {
            printf("%s%s\n", str_branch, names[i]);
        }

        /* Construct the absolute path */
        char path[256];
        if (strcmp(base_path, "/") == 0) {
            snprintf(path, sizeof(path), "/%s", names[i]);
        } else {
            snprintf(path, sizeof(path), "%s/%s", base_path, names[i]);
        }

        /* If it's a directory, dive deeper! */
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            uint32_t next_mask = last_mask;
            if (is_last) {
                next_mask |= (1 << depth);
            } else {
                next_mask &= ~(1 << depth);
            }
            print_tree_recursive(path, depth + 1, next_mask, use_vt);
        }

        free(names[i]);
    }

    free(names);
}

FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_tree)
int flx_fs_bin_cmd_tree(int argc, char **argv) {
    struct optparse options;
    optparse_init(&options, argv);

    bool use_vt = false;

    /* Option parsing ready for future flags like -a (all) or -d (dirs only) */
    int option;
    while ((option = optparse(&options, "v")) != -1) {
        switch (option) {
            case 'v': use_vt = true; break;
            case '?':
                FLX_SH_ERR("%s: invalid option -- '%c'\n", argv[0], options.optopt);
                return 1;
        }
    }

    const char *target = optparse_arg(&options);
    if (!target) target = ".";

    struct stat st;
    if (stat(target, &st) != 0 || !S_ISDIR(st.st_mode)) {
        FLX_SH_ERR("tree: %s: No such directory\n", target);
        return 1;
    }

    printf("%s\n", target);
    print_tree_recursive(target, 0, 0, use_vt);

    return 0;
}

