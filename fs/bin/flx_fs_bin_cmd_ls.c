#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <shell/flx_sh.h>
#include <fs/bin/flx_fs_bin_cmd_ls.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include "optparse.h"

int flx_fs_bin_cmd_ls(int argc, char **argv) {
    struct optparse options;
    int option;
    bool show_all = false;
    bool long_format = false;

    optparse_init(&options, argv);
    while ((option = optparse(&options, "al")) != -1) {
        switch (option) {
            case 'a': show_all = true; break;
            case 'l': long_format = true; break;
            case '?': printf("ls: %s\n", options.errmsg); return 1;
        }
    }

    char *target = optparse_arg(&options);
    if (!target) target = ".";

    DIR *dir = opendir(target);
    if (!dir) {
        struct stat st;
        if (stat(target, &st) == 0) {
            printf("%s\n", target);
            return 0;
        }
        FLX_SH_ERR("ls: cannot access '%s': No such file or directory\n", target);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') continue;

        if (long_format) {
            struct stat st;
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", target, entry->d_name);

            if (stat(path, &st) == 0) {
                //char perms[11] = "----------";
                char perms[11] = "----";
                if (S_ISDIR(st.st_mode)) perms[0] = 'd';
                else if (S_ISCHR(st.st_mode)) perms[0] = 'c';

                if (st.st_mode & S_IRUSR) perms[1] = 'r';
                if (st.st_mode & S_IWUSR) perms[2] = 'w';
                if (st.st_mode & S_IXUSR) perms[3] = 'x';

                //printf("%s 1 root root %4ld %s\n", perms, (long)st.st_size, entry->d_name);
                printf("%s 1 kitty %4ld %s\n", perms, (long)st.st_size, entry->d_name);
            }
        } else {
            printf("%s  ", entry->d_name);
        }
    }
    if (!long_format) printf("\n");

    closedir(dir);
    return 0;
}
